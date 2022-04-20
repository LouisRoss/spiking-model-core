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
#include "Log.h"
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
        unsigned long long int& iterations_;
        LogLevel& loggingLevel_;

    public:
        inet_stream* StreamSocket() const { return streamSocket_.get(); }

    public:
        SensorInputDataSocket(inet_stream_server* ccSocket, unsigned long long int& iterations, LogLevel& loggingLevel) :
            iterations_(iterations),
            loggingLevel_(loggingLevel),
            streamSocket_(ccSocket->accept2())
        {
            // Louis Ross - I modified libsocket to always apply SO_REUSEADDR=1 just before a server socket binds.
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
            // First field is the count of structs following, not the byte count.
            SpikeSignalLengthFieldType bufferCount {};
            try
            {
                auto received = streamSocket_->rcv((void*)&bufferCount, sizeof(bufferCount));

                // Received zero means the other end closed the socket.
                if (received == 0) return false;

                if (received != sizeof(bufferCount))
                {
                    cout << "SensorInputDataSocket::HandleInput received incorrect count field of size " << received << "\n";
                    return true;
                }

                if (loggingLevel_ == LogLevel::Diagnostic) cout << "SensorInputDataSocket::HandleInput received count field of " << received << " bytes = " << bufferCount << " structs of size " << SpikeSignalSize << " (" << sizeof(SpikeSignal::Tick) << " + " << sizeof(SpikeSignal::NeuronIndex) << ")\n";
            }
            catch(const libsocket::socket_exception& e)
            {
                cout << "SensorInputDataSocket::HandleInput received exception reading count field: " << e.mesg << "\n";
                return false;
            }
            
            // There is nothing following if the field count is zero.
            if (bufferCount == 0) return true;

            SpikeSignalProtocol protocol(bufferCount);
            if (!BuildInputBuffer(protocol, bufferCount))
            {
                return false;
            }

            multimap<int, unsigned long long int> data;
            for (SpikeSignalLengthFieldType index = 0; index < bufferCount; index++)
            {
                auto& signalSpike = protocol.GetElementAt(index);

                //cout << "  Sensor input signaling index " << signalSpike.NeuronIndex << " at time " << signalSpike.Tick << "\n";
                std::pair<int, int> element(signalSpike.Tick + iterations_, signalSpike.NeuronIndex);
                data.insert(element);
            }

            if (loggingLevel_ != LogLevel::None)
                cout << "Injecting input signal with " << data.size() << " spikes at tick " << iterations_ << "\n";
            injectCallback(data);

            return true;
        }

    private:
        bool BuildInputBuffer(SpikeSignalProtocol& protocol, SpikeSignalLengthFieldType bufferCount)
        {
            ssize_t expectedBufferCount { protocol.GetBufferSize(bufferCount) };
            ssize_t remainingBufferCount { protocol.GetBufferSize(bufferCount) };
            ssize_t totalReceived { 0 };
            do
            {
                auto received = streamSocket_->rcv((void*)((char*)protocol.SpikeBuffer() + totalReceived), remainingBufferCount);
                totalReceived += received;
                remainingBufferCount -= received;

                if (received == 0)
                {
                    cout << "SensorInputDataSocket::BuildInputBuffer received incorrect buffer length " << totalReceived << ", expected " << expectedBufferCount << "\n";
                    return false;
                }
            } while (totalReceived != expectedBufferCount);

            return true;
        }
    };
}
