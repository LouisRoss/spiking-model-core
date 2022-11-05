#pragma once
#include <fstream>

#include "IModelHelper.h"
#include "ModelNeuronInitializer.h"
#include "PackageInitializerProtocol.h"
#include "PackageInitializerDataSocket.h"
#include "Converters.h"

namespace protocol = embeddedpenguins::core::neuron::model::initializerprotocol;

namespace embeddedpenguins::core::neuron::model
{
    using std::ofstream;

    using embeddedpenguins::core::neuron::model::ToModelType;

    //
    //
    class ModelPackageInitializer : public ModelNeuronInitializer
    {
        vector<SpikeOutputDescriptor> spikeOutputDescriptors_ { };

    public:
        ModelPackageInitializer(IModelHelper* helper) :
            ModelNeuronInitializer(helper)
        {
        }

    public:
        // IModelInitializer implementaton
        virtual bool Initialize() override
        {
            const string& modelName { this->helper_->ModelName() };
            const string& deploymentName { this->helper_->DeploymentName() };
            const string& engineName { this->helper_->EngineName() };

            if (modelName.empty())
            {
                cout << "ModelPackageInitializer::Initialize can find no model, not initializing\n";
                return false;
            }

            cout << "ModelPackageInitializer::Initialize model '" << modelName << "' depoyment '" << deploymentName << "' engine '" << engineName << "'\n";
            PackageInitializerDataSocket socket(this->helper_->StackConfiguration());

            protocol::ModelDeploymentRequest deploymentRequest(modelName, deploymentName, engineName);

            auto response = socket.TransactWithServer<protocol::ModelDeploymentRequest, protocol::ModelDeploymentResponse>(deploymentRequest);
            auto* deploymentResponse = reinterpret_cast<protocol::ModelDeploymentResponse*>(response.get());

            cout << "ModelPackageInitializer::Initialize retrieved model size from packager of " << deploymentResponse->NeuronCount << " neurons and " << deploymentResponse->PopulationCount << " populations\n";

            this->helper_->AllocateModel(deploymentResponse->NeuronCount);
            this->helper_->InitializeModel();

            ofstream csvfile;
            auto wiringFilename = helper_->GetWiringFilename();
            if (!wiringFilename.empty())
            {
                cout << "Wiring file name: " << wiringFilename << "\n";
                csvfile.open(wiringFilename);
                csvfile << "expansion,offset,presynaptic,expansion,offset,postsynaptic,weight,type\n";
            }

            InitializeExpansionsForEngine(deploymentResponse, modelName, socket, csvfile);
            InitializeInterconnects(modelName, deploymentName, engineName, socket);

            return true;
        }

        void InitializeInterconnects(const string& modelName, const string& deploymentName, const string& engineName, PackageInitializerDataSocket& socket)
        {
            cout << "Requesting interconnects for model " << modelName << ", deployment " << deploymentName << ", engine " << engineName << "\n";
            protocol::ModelInterconnectRequest request(modelName, deploymentName, engineName);
            auto response = socket.TransactWithServer<protocol::ModelInterconnectRequest, protocol::ModelInterconnectResponse>(request);
            auto* interconnectionsResponse = reinterpret_cast<protocol::ModelInterconnectResponse*>(response.get());

            cout << "ModelPackageInitializer::InitializeInterconnects retrieved " << interconnectionsResponse->InterconnecCount << " interconnects\n";
            protocol::ModelInterconnectResponse::Interconnect* interconnects = interconnectionsResponse->GetInterconnections();

            spikeOutputDescriptors_.clear();
            for (auto i = 0; i < interconnectionsResponse->InterconnecCount; i++)
            {
                auto& interconnect = interconnects[i];

                auto& fromEngine = this->helper_->GetExpansionMap().ExpansionEngine(interconnect.FromExpansionIndex);
                if (fromEngine == engineName)
                {
                    cout 
                        << "Creating spike output descriptor with From expansion index " << interconnect.FromExpansionIndex
                        << ", which maps to Host '" << fromEngine 
                        << "' and To expansion index " << interconnect.ToExpansionIndex 
                        <<  ", which maps to Host '" << this->helper_->GetExpansionMap().ExpansionEngine(interconnect.ToExpansionIndex) 
                        << "', To layer offset "  << interconnect.ToLayerOffset << "\n";
                    SpikeOutputDescriptor interconnectDescriptor;
                    interconnectDescriptor.Host = this->helper_->GetExpansionMap().ExpansionEngine(interconnect.ToExpansionIndex);
                    interconnectDescriptor.ModelSequence = interconnect.ToExpansionIndex;
                    interconnectDescriptor.ModelOffset = interconnect.ToLayerOffset;
                    interconnectDescriptor.Size = interconnect.FromLayerCount;
                    interconnectDescriptor.LocalModelSequence = interconnect.FromExpansionIndex;
                    interconnectDescriptor.LocalModelOffset = interconnect.FromLayerOffset;
                    interconnectDescriptor.LocalStart = this->helper_->GetExpansionMap().ExpansionOffset(interconnect.FromExpansionIndex);

                    spikeOutputDescriptors_.push_back(interconnectDescriptor);
                }
            }
        }

        virtual const vector<SpikeOutputDescriptor>& GetInitializedOutputs() const override
        {
            return spikeOutputDescriptors_;
        }

    private:
        void InitializeExpansionsForEngine(protocol::ModelDeploymentResponse* deploymentResponse, const string& modelName, PackageInitializerDataSocket& socket, ofstream& csvfile)
        {
            protocol::ModelDeploymentResponse::Deployment* deployments = deploymentResponse->GetDeployments();
            unsigned long int expansionStart { 0 };
            for (auto i = 0; i < deploymentResponse->PopulationCount; i++)
            {
                deployments[i].EngineName[79] = '\0';
                string engineName(deployments[i].EngineName);
                helper_->AddExpansion(engineName, expansionStart, deployments[i].NeuronCount);
                expansionStart += deployments[i].NeuronCount;

                if (deployments[i].NeuronCount != 0)
                {
                    cout << "Requesting expansion " << i << " for model " << modelName << "\n";
                    protocol::ModelExpansionRequest request(modelName, i);
                    auto response = socket.TransactWithServer<protocol::ModelExpansionRequest, protocol::ModelExpansionResponse>(request);
                    auto* expansionResponse = reinterpret_cast<protocol::ModelExpansionResponse*>(response.get());

                    auto engineExpansionBase = helper_->GetExpansionMap().ExpansionOffset(i);
                    cout << "Initializing " << expansionResponse->ConnectionCount << " connections in expansion " << i << " with starting index " << expansionResponse->StartingNeuronOffset << " and count " << expansionResponse->NeuronCount << "\n";
                    InitializeExpansion(csvfile, i, engineExpansionBase, expansionResponse->ConnectionCount, expansionResponse->GetConnections());
                }
            }
        }

        void InitializeExpansion(ofstream& csvfile, unsigned int modelExpansion, unsigned int engineOffset, unsigned int connectionCount, protocol::ModelExpansionResponse::Connection* connections)
        {
            for (auto i = 0; i < connectionCount; i++, connections++)
            {
                protocol::ModelExpansionResponse::Connection& connection = *connections;
                if (csvfile.is_open()) 
                    csvfile 
                    << modelExpansion << "," << engineOffset << "," << connection.PreSynapticNeuron + engineOffset << "," 
                    << modelExpansion << "," << engineOffset << "," << connection.PostSynapticNeuron + engineOffset << "," 
                    << (int)connection.SynapticStrength << "," 
                    << (int)connection.Type 
                    << "\n";
                this->helper_->Wire(connection.PreSynapticNeuron + engineOffset, connection.PostSynapticNeuron + engineOffset, (int)connection.SynapticStrength, ToModelType(connection.Type));
            }
        }
    };
}
