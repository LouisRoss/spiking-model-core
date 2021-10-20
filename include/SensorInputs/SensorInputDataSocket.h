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
#include "SpikeSignalProtocol.h"

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
        unique_ptr<inet_stream> streamSocket_;

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
        
        bool HandleInput(function<void(const multimap<int, unsigned long long int>&)> injectCallback)
        {
            cout << "SensorInputDataSocket::HandleInput\n";
            // First field is the count of structs following, not the byte count.
            SpikeSignalLengthFieldType bufferCount {};
            try
            {
                auto received = streamSocket_->rcv((void*)&bufferCount, sizeof(bufferCount));

                if (received != sizeof(bufferCount))
                {
                    cout << "SensorInputDataSocket::HandleInput received incorrect count field of size " << received << "\n";
                    return false;
                }

                cout << "SensorInputSocket::HandleInput received count field of " << received << " bytes = " << bufferCount << " structs of size " << SpikeSignalSize << " (" << sizeof(SpikeSignal::Tick) << " + " << sizeof(SpikeSignal::NeuronIndex) << ")\n";
            }
            catch(const libsocket::socket_exception& e)
            {
                cout << "SensorInputSocket::HandleInput received exception reading count field: " << e.mesg << "\n";
                return false;
            }
            
            // There is nothing following if the field count is zero.
            if (bufferCount == 0) return true;

            SpikeSignalProtocol protocol {};
            auto received = streamSocket_->rcv((void*)protocol.SpikeBuffer, protocol.GetBufferSize(bufferCount));

            if (received != protocol.GetBufferSize(bufferCount))
            {
                cout << "SensorInputDataSocket::HandleInput received incorrect buffer length " << received << "\n";
                return false;
            }

            multimap<int, unsigned long long int> data;
            for (SpikeSignalLengthFieldType index = 0; index < bufferCount; index++)
            {
                auto& signalSpike = protocol.SpikeBuffer[index];

                cout << "  Sensor input signaling index " << signalSpike.NeuronIndex << " at time " << signalSpike.Tick << "\n";
                std::pair<int, int> element(signalSpike.Tick, signalSpike.NeuronIndex);
                data.insert(element);
            }

            injectCallback(data);

            return true;
        }
    };
}
