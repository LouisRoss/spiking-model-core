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

            unsigned int* deployments = deploymentResponse->GetDeployments();
            unsigned long int expansionStart { 0 };
            for (auto i = 0; i < deploymentResponse->PopulationCount; i++)
            {
                helper_->AddExpansion(expansionStart, deployments[i]);
                expansionStart += deployments[i];

                if (deployments[i] != 0)
                {
                    cout << "Requesting expansion " << i << " for model " << modelName << "\n";
                    protocol::ModelExpansionRequest request(modelName, i);
                    auto response = socket.TransactWithServer<protocol::ModelExpansionRequest, protocol::ModelExpansionResponse>(request);
                    auto* expansionResponse = reinterpret_cast<protocol::ModelExpansionResponse*>(response.get());

                    auto engineExpansionBase = helper_->ExpansionOffset(i);
                    cout << "Initializing " << expansionResponse->ConnectionCount << " connections in expansion " << i << " with starting index " << expansionResponse->StartingNeuronOffset << " and count " << expansionResponse->NeuronCount << "\n";
                    InitializeExpansion(csvfile, i, engineExpansionBase, expansionResponse->ConnectionCount, expansionResponse->GetConnections());
                }
            }

            return true;
        }

    private:
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
