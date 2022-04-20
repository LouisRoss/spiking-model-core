#pragma once

#include <iostream>
#include <memory>
#include <tuple>
#include <vector>

#include "libsocket/exception.hpp"
#include "libsocket/inetclientstream.hpp"

#include "nlohmann/json.hpp"

#include "ModelEngineContext.h"
#include "Log.h"
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
        //
        virtual bool Connect() override
        {
            bool connected { false };

            auto [connectionString, service] = GetConnectionStrings();

            if (!connected && !service.empty())
            {
                cout << "Developing connection string from service " << service << "\n";
                auto [serviceName, portType] = ParseConnectionString(service, "", "");
                cout << "Developing connection string from service " << serviceName << ", port " << portType << "\n";
                auto [host, port] = GetServiceConnection(serviceName, portType);
                cout << "Connecting to host " << host << ", port " << port << "\n";
                connected = TryConnect(host, port);
            }

            if (!connected && !connectionString.empty())
            {
                auto [host, port] = ParseConnectionString(connectionString, "localhost", "8001");
                connected = TryConnect(host, port);
            }

            return connected;
        }

        virtual bool Disconnect() override
        {
            streamSocket_.release();

            return true;
        }

        //
        // We always stream output, without respect to the disable flag.  Cannot be turned off.
        //
        virtual bool RespectDisableFlag() override { return false; }
        
        virtual bool IsInterestedIn(NeuronRecordType type) override 
        {
            return type == NeuronRecordType::Spike;
            //return type == NeuronRecordType::Refractory;
        }

        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, unsigned short synapseIndex, short int synapseStrength, NeuronRecordType type) override
        {
            //cout << "SpikeOutputSocket::StreamOutput(" << neuronIndex << "," << activation << ")\n";
            //json sample;
            //sample.push_back(context_.Iterations);
            //sample.push_back(neuronIndex);
            //spikes_.push_back(sample);

            //if (spikes_.size() > 25)
            //    Flush();

            SpikeSignal sample { static_cast<int>(context_.Measurements.Iterations), static_cast<unsigned int>(neuronIndex) };
            if (protocol_.Buffer(sample)) Flush();
        }

        virtual void Flush() override
        {
            if (protocol_.Empty()) return;

            try
            {
                if (context_.LoggingLevel != LogLevel::None)
                {
                    cout << "Spike output socket sending " << protocol_.GetCurrentBufferCount() << " spikes: ";

                    if (context_.LoggingLevel == LogLevel::Diagnostic)
                    {
                        SpikeSignal* spike = protocol_.SpikeBuffer();
                        auto baseTick = spike->Tick;
                        for (auto spikeIndex = 0; spikeIndex < protocol_.GetCurrentBufferCount(); spikeIndex++, spike++)
                        {
                            spike->Tick -= baseTick;
                            if (context_.LoggingLevel == LogLevel::Diagnostic) cout << "(" << spike->Tick << "," << spike->NeuronIndex << ") ";
                        }
                    }
                    cout << " at tick " << context_.Measurements.Iterations << "\n";
                }
                
                // First field is the count of structs in the array, not the byte count.
                SpikeSignalLengthFieldType bufferLength = protocol_.GetCurrentBufferCount();
                streamSocket_->snd((void*)&bufferLength, SpikeSignalLengthSize);
                streamSocket_->snd((void*)protocol_.SpikeBuffer(), protocol_.GetBufferSize(bufferLength));
            } catch (const socket_exception& exc)
            {
                cout << "SpikeOutputSocket Flush() caught exception: " << exc.mesg;
            }
            
            protocol_.Reset();

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
        bool TryConnect(const string& host, const string& port)
        {
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

        tuple<string, string> ParseConnectionString(const string& connectionString, const string& defaultHost, const string& defaultPort)
        {
            string host {defaultHost};
            string port {defaultPort};

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

        tuple<string, string> GetServiceConnection(const string& serviceName, const string& portType)
        {
            string hostName;
            string port;

            if (!context_.Configuration.StackConfiguration().contains("services"))
            {
                cout << "Stack configuration contains no 'services' element, not creating output streamer\n";
                return {hostName, port};
            }
            cout << "Found 'services' section of stack configuration\n";

            const json& servicesJson = context_.Configuration.StackConfiguration()["services"];
            if (!servicesJson.contains(serviceName))
            {
                cout << "Stack configuration 'services' element contains no '" << serviceName << "' subelement, not creating output streamer\n";
                return {hostName, port};
            }
            cout << "Found '" << serviceName << "' subsection of 'services' section of stack configuration\n";

            const json& serviceJson = servicesJson[serviceName.c_str()];
            if (serviceJson.contains("host"))
            {
                hostName = serviceJson["host"];
            }

            if (serviceJson.contains(portType))
            {
                port = serviceJson[portType.c_str()];
            }

            return {hostName, port};
        }

        //
        // Find the two allowed strings, and return the tuple <'ConnectionString', 'Service'>.
        // Either or both may be empty strings if not present in configuration.
        //
        tuple<string, string> GetConnectionStrings()
        {
            string connectionString;
            string service;

            if (!context_.Configuration.Control().contains("Execution"))
            {
                cout << "Configuration contains no 'Execution' element, not creating output streamer\n";
                return {connectionString, service};
            }

            const json& executionJson = context_.Configuration.Control()["Execution"];
            if (!executionJson.contains("OutputStreamers"))
            {
                cout << "Configuration 'Execution' element contains no 'OutputStreamers' subelement, not creating output streamer\n";
                return {connectionString, service};
            }

            const json& outputStreamersJson = executionJson["OutputStreamers"];
            if (outputStreamersJson.is_array())
            {
                for (auto& [key, outputStreamerJson] : outputStreamersJson.items())
                {
                    if (outputStreamerJson.is_object())
                    {
                        string outputStreamerLocation { "" };
                        if (outputStreamerJson.contains("Location"))
                        {
                            const json& locationJson = outputStreamerJson["Location"];
                            if (locationJson.is_string())
                            {
                                outputStreamerLocation = locationJson.get<string>();
                            }
                        }

                        if (outputStreamerLocation.find("SpikeOutputSocket") != string::npos)
                        {
                            if (outputStreamerJson.contains("ConnectionString"))
                            {
                                const json& connectionStringJson = outputStreamerJson["ConnectionString"];
                                if (connectionStringJson.is_string())
                                    connectionString = connectionStringJson.get<string>();
                            }

                            if (outputStreamerJson.contains("Service"))
                            {
                                const json& serviceJson = outputStreamerJson["Service"];
                                if (serviceJson.is_string())
                                    service = serviceJson.get<string>();
                            }
                        }
                    }
                }
            }

            return {connectionString, service};
        }
    };
}
