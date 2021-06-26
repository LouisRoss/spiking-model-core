#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include <nlohmann/json.hpp>

#include "libsocket/exception.hpp"
#include "libsocket/inetserverstream.hpp"

#include "IQueryHandler.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::vector;
    using std::multimap;
    using std::unique_ptr;
    using std::function;

    using nlohmann::json;

    using libsocket::inet_stream;
    using libsocket::inet_stream_server;

    class SensorInputDataSocket
    {
        struct InputSpike
        {
            int Tick;
            unsigned long long int NeuronIndex;
        };

        unique_ptr<inet_stream> streamSocket_;

        string query_ { };

    public:
        inet_stream* StreamSocket() const { return streamSocket_.get(); }

    public:
        SensorInputDataSocket(inet_stream_server* ccSocket) :
            streamSocket_(ccSocket->accept2())
        {
            // An attempt to stop the re-use timer at the system level, so we can restart immediately.
            int reuseaddr = 1;
            streamSocket_->set_sock_opt(streamSocket_->getfd(), SO_REUSEADDR, (const char*)(&reuseaddr), sizeof(int));

            cout << "SensorInputDataSocket main ctor\n";
        }

        SensorInputDataSocket(const SensorInputDataSocket& other) = delete;
        SensorInputDataSocket& operator=(const SensorInputDataSocket& other) = delete;
        SensorInputDataSocket(SensorInputDataSocket&& other) noexcept = delete;
        SensorInputDataSocket& operator=(SensorInputDataSocket&& other) noexcept = delete;

        virtual ~SensorInputDataSocket()
        {
            cout << "SensorInputDataSocket dtor \n";
            streamSocket_->shutdown(LIBSOCKET_READ | LIBSOCKET_WRITE);
        }

        //
        // The main operation.  If this socket becomes readable, call this method.
        // We read the query from the command and control client and process it.
        // If the read is empty, assume the socket was closed by the client.
        // Return true unless the client closed the socket and we need to clean up this end.
        //
        bool HandleInput(function<void(const multimap<int, unsigned long long int>&)> injectCallback)
        {
            string queryFragment;
            queryFragment.resize(1000);
            *streamSocket_ >> queryFragment;

            if (queryFragment.empty()) return false;

            BuildInputString(queryFragment);
            cout << "Sensor input received '" << query_ << "'\n";

            multimap<int, unsigned long long int> data;
            try
            {
                json dataStream = json::parse(query_);

                const auto& signal = dataStream.get<vector<vector<int>>>();
                for (auto& signalElement : signal)
                {
                    cout << "  Sensor input signaling index " << signalElement[1] << " at time " << signalElement[0] << "\n";
                    std::pair<int, int> element(signalElement[0], signalElement[1]);
                    data.insert(element);
                }
            }
            catch (json::exception ex)
            {
                cout << "HandleInput failed to parse data string '" << query_ << "'\n";
                return false;
            }

            injectCallback(data);

            return true;
        }

        void BuildInputString(string& queryFragment)
        {
            query_.clear();

            do
            {
                query_ += queryFragment;

                if (queryFragment.size() == 1000)
                {
                    queryFragment.clear();
                    queryFragment.resize(1000);
                    *streamSocket_ >> queryFragment;

                    if (!queryFragment.empty()) query_ += queryFragment;
                }
            } while (queryFragment.size() == 1000);
        }
    };
}
