#pragma once

#include <string>
#include <algorithm>
#include <tuple>

namespace embeddedpenguins::core::neuron::model::initializerprotocol
{
    using std::string;
    using std::copy;
    using std::tuple;

    using LengthFieldType = unsigned int;
    using CommandFieldType = unsigned int;

    enum class PackageInitializerCommand : CommandFieldType
    {
        GetModelDescriptor = 0,
        GetModelExpansion = 1,
        GetModelDeployment = 2,
        GetModelInterconnects = 3,
        GetFullModelDeployment = 4
    };

    struct PackageInitializerEnvelope
    {
        LengthFieldType PacketSize;
    };

    struct ModelDescriptorRequest : public PackageInitializerEnvelope
    {
        PackageInitializerCommand Command { PackageInitializerCommand::GetModelDescriptor };
        char ModelName[80];

        ModelDescriptorRequest() { PacketSize = sizeof(ModelDescriptorRequest) - sizeof(PackageInitializerEnvelope); }
        ModelDescriptorRequest(const string& modelName) : ModelDescriptorRequest()
        {
            string name { modelName };
            name.resize(sizeof(ModelName));
            copy(name.c_str(), name.c_str() + sizeof(ModelName), ModelName);
        }
    };

    struct ModelExpansionRequest : public PackageInitializerEnvelope
    {
        PackageInitializerCommand Command { PackageInitializerCommand::GetModelExpansion };
        unsigned int Sequence { 0 };
        char ModelName[80];

        ModelExpansionRequest() { PacketSize = sizeof(ModelExpansionRequest) - sizeof(PackageInitializerEnvelope); }
        ModelExpansionRequest(const string& modelName, unsigned int sequence) : ModelExpansionRequest()
        {
            Sequence = sequence;

            string name { modelName };
            name.resize(sizeof(ModelName));
            copy(name.c_str(), name.c_str() + sizeof(ModelName), ModelName);
        }
    };

    struct ModelDeploymentRequest : PackageInitializerEnvelope
    {
        PackageInitializerCommand Command { PackageInitializerCommand::GetModelDeployment };
        char ModelName[80];
        char DeploymentName[80];
        char EngineName[80];

        ModelDeploymentRequest() { PacketSize = sizeof(ModelDeploymentRequest) - sizeof(PackageInitializerEnvelope); }
        ModelDeploymentRequest(const string& modelName, const string& deploymentName, const string& engineName) : ModelDeploymentRequest()
        {
            string name { modelName };
            name.resize(sizeof(ModelName));
            copy(name.c_str(), name.c_str() + sizeof(ModelName), ModelName);

            name = deploymentName;
            name.resize(sizeof(DeploymentName));
            copy(name.c_str(), name.c_str() + sizeof(DeploymentName), DeploymentName);

            name = engineName;
            name.resize(sizeof(EngineName));
            copy(name.c_str(), name.c_str() + sizeof(EngineName), EngineName);
        }
    };

    struct ModelFullDeploymentRequest : PackageInitializerEnvelope
    {
        PackageInitializerCommand Command { PackageInitializerCommand::GetFullModelDeployment };
        char ModelName[80];
        char DeploymentName[80];
        unsigned int RecordFlag;            // True (nonzero) to indicate we are recording, leave a file of full deployment.

        ModelFullDeploymentRequest(const bool recordFlag = false) : RecordFlag(recordFlag ? 1 : 0) { PacketSize = sizeof(ModelFullDeploymentRequest) - sizeof(PackageInitializerEnvelope); }
        ModelFullDeploymentRequest(const string& modelName, const string& deploymentName, const bool recordFlag = false) : 
            ModelFullDeploymentRequest(recordFlag)
        {
            string name { modelName };
            name.resize(sizeof(ModelName));
            copy(name.c_str(), name.c_str() + sizeof(ModelName), ModelName);

            name = deploymentName;
            name.resize(sizeof(DeploymentName));
            copy(name.c_str(), name.c_str() + sizeof(DeploymentName), DeploymentName);

            RecordFlag = recordFlag;
        }
    };

    struct ModelInterconnectRequest : PackageInitializerEnvelope
    {
        PackageInitializerCommand Command { PackageInitializerCommand::GetModelInterconnects };
        char ModelName[80];
        char DeploymentName[80];
        char EngineName[80];

        ModelInterconnectRequest() { PacketSize = sizeof(ModelInterconnectRequest) - sizeof(PackageInitializerEnvelope); }
        ModelInterconnectRequest(const string& modelName, const string& deploymentName, const string& engineName) : ModelInterconnectRequest()
        {
            string name { modelName };
            name.resize(sizeof(ModelName));
            copy(name.c_str(), name.c_str() + sizeof(ModelName), ModelName);

            name = deploymentName;
            name.resize(sizeof(DeploymentName));
            copy(name.c_str(), name.c_str() + sizeof(DeploymentName), DeploymentName);

            name = engineName;
            name.resize(sizeof(EngineName));
            copy(name.c_str(), name.c_str() + sizeof(EngineName), EngineName);
        }
    };

    struct ValidateSize { };

    struct ModelDescriptorResponse : public ValidateSize
    {
        unsigned int NeuronCount { 0 };
        unsigned int ExpansionCount { 0 };
    };

    struct ModelDeploymentResponse
    {
        unsigned int NeuronCount { 0 };
        unsigned int PopulationCount { 0 };

        struct Deployment
        {
            char EngineName[80];
            unsigned int NeuronCount;
        };

        // Immediately following this struct in memory should be an array
        // unsigned int Deploy[PopulationCount];
        Deployment* GetDeployments() { return reinterpret_cast<Deployment*>(this + 1); }
    };

    struct ModelFullDeploymentResponse
    {
        unsigned int PopulationCount { 0 };

        struct Deployment
        {
            char EngineName[80];
            unsigned int NeuronOffset;
            unsigned int NeuronCount;
        };

        // Immediately following this struct in memory should be an array
        // unsigned int Deploy[PopulationCount];
        Deployment* GetDeployments() { return reinterpret_cast<Deployment*>(this + 1); }
    };

    struct ModelExpansionResponse
    {
        enum class ConnectionType : unsigned short
        {
            Excitatory = 0,
            Inhibitory = 1,
            Attention = 2
        };

        struct Connection
        {
            unsigned int PreSynapticNeuron;
            unsigned int PostSynapticNeuron;
            short int SynapticStrength;
            ConnectionType Type; 
        };

        unsigned int StartingNeuronOffset { 0 };
        unsigned int NeuronCount { 0 };
        unsigned int ConnectionCount { 0 };

        // Immediately following this struct in memory should be an array
        // Connection Connection[ConnectionCount];
        Connection* GetConnections() { return (Connection*)(this + 1); }
    };

    struct ModelInterconnectResponse
    {
        struct Interconnect
        {
            unsigned int FromExpansionIndex { 0 };
            unsigned int FromLayerOffset { 0 };
            unsigned int FromLayerCount { 0 };
            unsigned int ToExpansionIndex { 0 };
            unsigned int ToLayerOffset { 0 };
            unsigned int ToLayerCount { 0 };
        };

        unsigned int InterconnecCount { 0 };

        // Immediately following this struct in memory should be an array
        // Interconnect Interconnect[InterconnecCount];
        Interconnect* GetInterconnections() { return reinterpret_cast<Interconnect*>(this + 1); }
    };
}
