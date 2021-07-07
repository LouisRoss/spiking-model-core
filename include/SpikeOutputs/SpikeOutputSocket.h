#pragma once

#include <iostream>
#include <memory>
#include <tuple>
#include <vector>

#include "libsocket/exception.hpp"
#include "libsocket/inetclientstream.hpp"

#include "nlohmann/json.hpp"

#include "ModelEngineContext.h"
#include "ConfigurationRepository.h"
#include "SpikeOutputs/ISpikeOutput.h"
#include "SpikeSignalProtocol.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::ifstream;
    using std::unique_ptr;
    using std::make_unique;
    using std::tuple;
    using std::vector;

    using libsocket::inet_stream;
    using libsocket::socket_exception;

    using nlohmann::json;

    using embeddedpenguins::gpu::neuron::model::ModelEngineContext;

    class SpikeOutputSocket : public ISpikeOutput
    {
        ModelEngineContext& context_;
        const ConfigurationRepository& configuration_;

        unique_ptr<inet_stream> streamSocket_ {};
        //json spikes_ {};
        SpikeSignalProtocol protocol_ {};

    public:
        SpikeOutputSocket(ModelEngineContext& context) :
            context_(context),
            configuration_(context_.Configuration)
        {
            cout << "SpikeOutputSocket constructor\n";
        }

        // ISpikeOutput implementaton
        virtual void CreateProxy(ModelEngineContext& context) override { }

        //
        // Interpret the connection string as a hostname and port number
        // to connect to.  The two fields are separated by a ':' character.
        //
        virtual bool Connect(const string& connectionString) override
        {
            auto [host, port] = ParseConnectionString(connectionString);
            try
            {
                cout << "SpikeOutputSocket Connect() connecting to " << host << ":" << port << "\n";
                streamSocket_ = make_unique<inet_stream>(host, port, LIBSOCKET_IPv4);
            } catch (const socket_exception& exc)
            {
                cout << "SpikeOutputSocket Connect() caught exception: " << exc.mesg;
                return false;
            }

            return true;
        }

        virtual bool Disconnect() override
        {
            streamSocket_.release();

            return true;
        }

        virtual bool IsInterestedIn(NeuronRecordType type) override 
        {
            //return type == NeuronRecordType::Spike;
            return type == NeuronRecordType::Refractory;
        }

        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, NeuronRecordType type) override
        {
            cout << "SpikeOutputSocket::StreamOutput(" << neuronIndex << "," << activation << ")\n";
            //json sample;
            //sample.push_back(context_.Iterations);
            //sample.push_back(neuronIndex);
            //spikes_.push_back(sample);

            //if (spikes_.size() > 25)
            //    Flush();

            SpikeSignal sample { static_cast<int>(context_.Iterations), static_cast<unsigned int>(neuronIndex) };
            if (protocol_.Buffer(sample)) Flush();
        }

        virtual void Flush() override
        {
            if (protocol_.CurrentSpikeBufferIndex == 0) return;

            try
            {
                // First field is the count of structs in the array, not the byte count.
                SpikeSignalLengthFieldType bufferLength = protocol_.GetCurrentBufferCount();
                streamSocket_->snd((void*)&bufferLength, SpikeSignalLengthSize);
                streamSocket_->snd((void*)&protocol_.SpikeBuffer, protocol_.GetBufferSize(bufferLength));
            } catch (const socket_exception& exc)
            {
                cout << "SpikeOutputSocket Flush() caught exception: " << exc.mesg;
            }
            
            protocol_.CurrentSpikeBufferIndex = 0;

            /*
            if (spikes_.empty()) return;

            try
            {
                auto spikesDump = spikes_.dump();
                cout << "SpikeOutputSocket flushing existing buffer of " << spikes_.size() << ": " << spikesDump << "\n";
                *streamSocket_ << spikes_.dump();
            } catch (const socket_exception& exc)
            {
                cout << "SpikeOutputSocket Flush() caught exception: " << exc.mesg;
            }

            spikes_.clear();
            */

        }

    private:
        tuple<string, string> ParseConnectionString(const string& connectionString)
        {
            string host {"localhost"};
            string port {"8001"};

            auto colonPos = connectionString.find(":");
            if (colonPos != string::npos)
            {
                auto tempHost = connectionString.substr(0, colonPos);
                if (!tempHost.empty()) host = tempHost;
                auto tempPort = connectionString.substr(colonPos + 1);
                if (!tempPort.empty()) port = tempPort;
            }

            return {host, port};
        }
    };
}
