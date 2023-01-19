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
        const ConfigurationRepository& configuration_;

        bool localOffsetCalculated_ { false };
        unsigned int localOffset_ { 0 };

    public:
        inet_stream* StreamSocket() const { return streamSocket_.get(); }

    public:
        SensorInputDataSocket(inet_stream_server* ccSocket, unsigned long long int& iterations, LogLevel& loggingLevel, const ConfigurationRepository& configuration) :
            iterations_(iterations),
            loggingLevel_(loggingLevel),
            streamSocket_(ccSocket->accept2()),
            configuration_(configuration)
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
            // First field is the byte count.
            SpikeSignalLengthFieldType byteCount {};
            try
            {
                auto received = streamSocket_->rcv((void*)&byteCount, sizeof(byteCount));

                // Received zero means the other end closed the socket.
                if (received == 0) return false;

                if (received != sizeof(byteCount))
                {
                    cout << "SensorInputDataSocket::HandleInput received incorrect count field of size " << received << "\n";
                    return true;
                }

                if (loggingLevel_ == LogLevel::Diagnostic) cout << "SensorInputDataSocket::HandleInput received count field of " << received << " bytes = " << (byteCount - PopulationIndexSize) / SpikeSignalSize << " spikes = " << SpikeSignalSize << " (" << sizeof(SpikeSignal::Tick) << " + " << sizeof(SpikeSignal::NeuronIndex) << ")\n";
            }
            catch(const libsocket::socket_exception& e)
            {
                cout << "SensorInputDataSocket::HandleInput received exception reading count field: " << e.mesg << "\n";
                return false;
            }
            
            // There is nothing following if the field count is smaller than needed for at least one spike.
            if (byteCount < SpikeSignalProtocol::GetBufferSize(0)) return true;

            SpikeSignalProtocol protocol(byteCount);
            if (!BuildInputBuffer(protocol, byteCount))
            {
                return false;
            }

            if (!localOffsetCalculated_)
            {
                auto* protocolPacket = protocol.GetProtocolBuffer();
                localOffset_ = configuration_.ExpansionMap().ExpansionOffset(protocolPacket->PopulationIndex) + protocolPacket->LayerOffset;
                localOffsetCalculated_ = true;
            }

            multimap<int, unsigned long long int> data;
            for (SpikeSignalLengthFieldType index = 0; index < protocol.GetCapacity(); index++)
            {
                auto& signalSpike = protocol.GetElementAt(index);

                //cout << "  Sensor input signaling index " << signalSpike.NeuronIndex << " at time " << signalSpike.Tick << "\n";
                std::pair<int, int> element(signalSpike.Tick + iterations_, signalSpike.NeuronIndex + localOffset_);
                data.insert(element);
            }

            if (loggingLevel_ != LogLevel::None)
                cout << "Injecting input signal with " << data.size() << " spikes at tick " << iterations_ << "\n";
            injectCallback(data);

            return true;
        }

    private:
        bool BuildInputBuffer(SpikeSignalProtocol& protocol, SpikeSignalLengthFieldType byteCount)
        {
            // Skip past length field.
            SpikeSignalPacket* buffer =  protocol.GetProtocolBuffer();
            char* protocolBuffer = reinterpret_cast<char*>(buffer) + sizeof(SpikeSignalLengthFieldType);
            
            const ssize_t expectedBufferCount { byteCount };
            ssize_t remainingBufferCount { byteCount };
            ssize_t totalReceived { 0 };
            do
            {
                auto received = streamSocket_->rcv((void*)(protocolBuffer + totalReceived), remainingBufferCount);
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
