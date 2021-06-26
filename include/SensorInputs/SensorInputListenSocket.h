#pragma once

#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <utility>
#include <functional>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"
#include "libsocket/select.hpp"
#include "libsocket/socket.hpp"


#include "ConfigurationRepository.h"
#include "SensorInputDataSocket.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::map;
    using std::multimap;
    using std::string;
    using std::unique_ptr;
    using std::make_unique;
    using std::begin;
    using std::end;
    using std::function;

    using libsocket::socket;
    using libsocket::inet_stream;
    using libsocket::inet_stream_server;
    using libsocket::selectset;

    class SensorInputListenSocket
    {
        inet_stream_server server_;
        const ConfigurationRepository& configuration_;

        unique_ptr<selectset<socket>> selectSet_ { };

        map<socket*, unique_ptr<SensorInputDataSocket>> ccSockets_ { };

        multimap<int, unsigned long long int> signalToInject_ { };
        function<void(const multimap<int, unsigned long long int>&)> InjectCallback_;

    public:
        SensorInputListenSocket(const string& host, const string& port, const ConfigurationRepository& configuration, function<void(const multimap<int, unsigned long long int>&)> injectCallback) :
            server_(host, port, LIBSOCKET_IPv4),
            configuration_(configuration),
            InjectCallback_(injectCallback)
        {
            cout << "SensorInputListenSocket with fd=" << server_.getfd() << " listening at " << host << ":" << port << "\n";
            // An attempt to stop the re-use timer at the system level, so we can restart immediately.
            int reuseaddr = 1;
            server_.set_sock_opt(server_.getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));
        }

        SensorInputListenSocket(const SensorInputListenSocket& other) = delete;
        SensorInputListenSocket& operator=(const SensorInputListenSocket& other) = delete;
        SensorInputListenSocket(SensorInputListenSocket&& other) noexcept = delete;
        SensorInputListenSocket& operator=(SensorInputListenSocket&& other) noexcept = delete;

        virtual ~SensorInputListenSocket()
        {
            cout << "SensorInputListenSocket dtor\n";
            server_.destroy();
        }

        bool Initialize()
        {
            MakeSelectSet();
            cout << "SensorInputListenSocket Initialize\n";
            return true;
        }

        //
        // The main action handler.  Call periodically from the main loop.
        // Wait a few milliseconds for one of the sockets in the select set
        // to be readable, then handle all that are.
        // Use dynamic_cast to distinguish between socket types.
        //
        bool Process()
        {
            auto [readSockets, _] = selectSet_->wait(10'000);

            for (auto readSocket : readSockets)
            {
                inet_stream_server* listenSocket = dynamic_cast<inet_stream_server*>(readSocket);
                if (listenSocket != nullptr)
                    AcceptNewConnection(readSocket, listenSocket);

                inet_stream* dataSocket = dynamic_cast<inet_stream*>(readSocket);
                if (dataSocket != nullptr)
                    HandleInput(readSocket, dataSocket);
            }

            return false;
        }

    private:
        void AcceptNewConnection(socket* readSocket, inet_stream_server* listenSocket)
        {
            if (ccSockets_.find(readSocket) == end(ccSockets_))
            {
                cout << "SensorInputListenSocket found readable socket is listen socket, creating new connection\n";
                auto dataSocket = make_unique<SensorInputDataSocket>(listenSocket);
                ccSockets_[dataSocket->StreamSocket()] = std::move(dataSocket);
                MakeSelectSet();
            }
        }

        void HandleInput(socket* readSocket, inet_stream* dataSocket)
        {
            cout << "SensorInputListenSocket found readable data socket, handling request\n";
            auto iSocket = ccSockets_.find(readSocket);
            if (iSocket != end(ccSockets_))
            {
                if (!iSocket->second->HandleInput(InjectCallback_))
                {
                    cout << "SensorInputListenSocket: readable data socket closed by client\n";
                    ccSockets_.erase(iSocket);
                    MakeSelectSet();
                }
            }
        }

        //
        // Throw away the existing select set (if any) and create a new one
        // with read function for the listen server socket and the streaming
        // sockets of all current data sockets.
        //
        void MakeSelectSet()
        {
            selectSet_.reset(new selectset<socket>());

            selectSet_->add_fd(server_, LIBSOCKET_READ);
            for (auto iSocket = begin(ccSockets_); iSocket != end(ccSockets_); iSocket++)
            {
                selectSet_->add_fd(*iSocket->second->StreamSocket(), LIBSOCKET_READ);
            }
        }
    };
}
