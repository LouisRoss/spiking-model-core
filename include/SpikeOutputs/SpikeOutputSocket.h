#pragma once

#include <iostream>
#include <memory>
#include <limits>
#include <tuple>
#include <vector>

#include "libsocket/exception.hpp"
#include "libsocket/inetclientstream.hpp"

#include "nlohmann/json.hpp"

#include "ModelContext.h"
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
    using std::numeric_limits;
    using std::tuple;
    using std::vector;

    using libsocket::inet_stream;
    using libsocket::socket_exception;

    using nlohmann::json;

    class SpikeOutputSocket : public ISpikeOutput
    {
        ModelContext& context_;
        const ConfigurationRepository& configuration_;

        unique_ptr<inet_stream> streamSocket_ {};
        SpikeSignalProtocol protocol_ {};
        unsigned int filterBottom_ {};
        unsigned int filterTop_ { numeric_limits<unsigned int>::max() };

    public:
        SpikeOutputSocket(ModelContext& context) :
            context_(context),
            configuration_(context_.Configuration)
        {
            cout << "SpikeOutputSocket constructor\n";
        }

        // ISpikeOutput implementaton
        virtual void CreateProxy(ModelContext& context) override { }

        //
        //  For connection to configured services, such as visualizers.  Develop the connection
        // string from the configuration, leave filtering alone.
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

        //
        //  For connection as an interconnect between an expansion/layer on one engine
        // and an expansion/layer on an engine that may be the same or different.
        // This form manages a filter, so that only spikes from the source expansion/layer
        // are sent to the specified connection.
        //
        virtual bool Connect(const string& connectionString, unsigned int filterBottom, unsigned int filterLength, unsigned int toIndex, unsigned int toOffset) override
        {
            filterBottom_ = filterBottom;
            filterTop_ = filterBottom + filterLength;
            protocol_ = SpikeSignalProtocol(toIndex, toOffset, 250);

            bool connected { false };

            auto [host, port] = ParseConnectionString(connectionString, "localhost", "8001");
            connected = TryConnect(host, port);

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

        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, short int hypersensitive, unsigned short synapseIndex, short int synapseStrength, NeuronRecordType type) override
        {
            // The default filter used by the default Connect(), is wide open.
            if (neuronIndex < filterBottom_ || neuronIndex >= filterTop_)
                return;

            SpikeSignal sample { static_cast<int>(context_.Measurements.Iterations), static_cast<unsigned int>(neuronIndex) - filterBottom_ };
            if (protocol_.Buffer(sample)) Flush();
        }

        virtual void Flush() override
        {
            if (protocol_.IsEmpty()) return;

            try
            {
                if (context_.LoggingLevel != LogLevel::None)
                {
                    cout << "Spike output socket sending " << protocol_.GetCurrentBufferCount() << " spikes: ";

                    if (context_.LoggingLevel == LogLevel::Diagnostic)
                    {
                        auto* spike = protocol_.GetProtocolBuffer()->GetSpikeSignals();
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
                streamSocket_->snd((void*)protocol_.GetProtocolBuffer(), protocol_.GetBufferSize());
            } catch (const socket_exception& exc)
            {
                cout << "SpikeOutputSocket Flush() caught exception: " << exc.mesg;
            }
            
            protocol_.Reset();
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
