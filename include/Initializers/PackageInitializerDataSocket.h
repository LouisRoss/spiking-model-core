#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <memory>
#include <algorithm>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "libsocket/socket.hpp"
#include "libsocket/inetclientstream.hpp"
#include "libsocket/select.hpp"
#include "libsocket/exception.hpp"

#include "PackageInitializerProtocol.h"

namespace protocol = embeddedpenguins::core::neuron::model::initializerprotocol;

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::tuple;
    using std::unique_ptr;
    using std::make_unique;
    using std::copy;

    using nlohmann::json;

    using libsocket::socket;
    using libsocket::inet_stream;
    using libsocket::selectset;
    using libsocket::socket_exception;

    class PackageInitializerDataSocket
    {
        const json& stackConfiguration_;
        unique_ptr<inet_stream> streamSocket_ { };

        unique_ptr<selectset<socket>> selectSet_ { };

        // Response structs, allowing us to return references.
        protocol::ModelDescriptorResponse modelDescriptor_ { };

    public:
        PackageInitializerDataSocket(const json& stackConfiguration) :
            stackConfiguration_(stackConfiguration)
        {
            cout << "PackageInitializerDataSocket main ctor\n";
            Connect();
        }

        PackageInitializerDataSocket(const PackageInitializerDataSocket& other) = delete;
        PackageInitializerDataSocket& operator=(const PackageInitializerDataSocket& other) = delete;
        PackageInitializerDataSocket(PackageInitializerDataSocket&& other) noexcept = delete;
        PackageInitializerDataSocket& operator=(PackageInitializerDataSocket&& other) noexcept = delete;

        virtual ~PackageInitializerDataSocket()
        {
            cout << "PackageInitializerDataSocket dtor \n";

            Disconnect();
        }

        template<class ReqType, class ResType>
        unique_ptr<char[]> TransactWithServer(const ReqType& request)
        {
            streamSocket_->snd((void*)&request, sizeof(request));

            // First field is the byte count.
            protocol::LengthFieldType bufferCount { };
            try
            {
                if (!WaitForInput(10'000'000))
                {
                    cout << "PackageInitializerDataSocket::TransactWithServer timed out waiting for count field\n";
                    return unique_ptr<char[]> { };
                }
                auto received = streamSocket_->rcv((void*)&bufferCount, sizeof(bufferCount));

                if (received != sizeof(bufferCount))
                {
                    cout << "PackageInitializerDataSocket::TransactWithServer received incorrect count field of size " << received << "\n";
                    return unique_ptr<char[]> { };
                }

                if (std::is_same<ResType, protocol::ValidateSize>::value && bufferCount != sizeof(ResType))
                {
                    cout << "PackageInitializerDataSocket::TransactWithServer received incorrect count field of " << bufferCount << ", instead of " << sizeof(ResType) << "\n";
                    return unique_ptr<char[]> { };
                }
            }
            catch(const libsocket::socket_exception& e)
            {
                cout << "PackageInitializerDataSocket::TransactWithServer received exception reading count field: " << e.mesg << "\n";
                return unique_ptr<char[]> { };
            }

            auto response = make_unique<char[]>(bufferCount);

            auto completed { false };
            auto totalReceived { 0 };
            do
            {
                auto received = streamSocket_->rcv((void*)(response.get() + totalReceived), bufferCount - totalReceived);

                if (received == bufferCount - totalReceived)
                {
                    completed = true;
                }
                else
                {
                    totalReceived += received;

                    if (!WaitForInput(100'000))
                    {
                        cout << "SensorInputDataSocket::TransactWithServer received incorrect buffer length " << totalReceived << ", instead of " << bufferCount << "\n";
                        return unique_ptr<char[]> { };
                    }
                }
            } while (!completed);
            

            return std::move(response);
        }

    private:
        bool Connect()
        {
            auto [host, port] = GetConfiguredConnectionString();
            try
            {
                cout << "PackageInitializerDataSocket Connect() connecting to " << host << ":" << port << "\n";
                streamSocket_ = make_unique<inet_stream>(host, port, LIBSOCKET_IPv4);

                selectSet_.reset(new selectset<socket>());
                selectSet_->add_fd(*streamSocket_.get(), LIBSOCKET_READ);

                // An attempt to stop the re-use timer at the system level, so we can restart immediately.
                int reuseaddr = 1;
                streamSocket_->set_sock_opt(streamSocket_->getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));
            } catch (const socket_exception& exc)
            {
                cout << "PackageInitializerDataSocket Connect() caught exception: " << exc.mesg;
                return false;
            }

            return true;
        }

        bool Disconnect()
        {
            streamSocket_->shutdown(LIBSOCKET_READ | LIBSOCKET_WRITE);
            streamSocket_.release();

            return true;
        }

        tuple<string, string> GetConfiguredConnectionString()
        {
            string host {"localhost"};
            string port {"4000"};

            if (stackConfiguration_.contains("services"))
            {
                const json& servicesJson = stackConfiguration_["services"];
                if (!servicesJson.is_null() && servicesJson.contains("initializerShim"))
                {
                    const json& controlConnectorJson = servicesJson["initializerShim"];
                    if (!controlConnectorJson.is_null())
                    {
                        if (controlConnectorJson.contains("host"))
                        {
                            const json& hostJson = controlConnectorJson["host"];
                            if (hostJson.is_string())
                                host = hostJson.get<string>();
                        }

                        if (controlConnectorJson.contains("rawport"))
                        {
                            const json& portJson = controlConnectorJson["rawport"];
                            if (portJson.is_string())
                                port = portJson.get<string>();
                        }
                    }
                }
            }

            return {host, port};
        }

        bool WaitForInput(long long waitNanoseconds)
        {
            cout << "Waitng for socket input ready...";
            auto [readSockets, _] = selectSet_->wait(waitNanoseconds);

            for (auto readSocket : readSockets)
            {
                inet_stream* dataSocket = dynamic_cast<inet_stream*>(readSocket);
                if (dataSocket != nullptr)
                {
                    cout << "Ready!\n";
                    return true;
                }
            }

            cout << "Timed out ;-(\n";
            return false;
        }

    };
}
