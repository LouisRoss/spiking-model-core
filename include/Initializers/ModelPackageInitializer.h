#pragma once
#include <fstream>

#include "IModelHelper.h"
#include "ModelNeuronInitializer.h"
#include "PackageInitializerDataSocket.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::ofstream;

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

            cout << "ModelPackageHelper retrieved model size from packager of " << deploymentResponse->NeuronCount << " neurons and " << deploymentResponse->PopulationCount << " populations\n";

            this->helper_->AllocateModel(deploymentResponse->NeuronCount);
            this->helper_->InitializeModel();

            this->strength_ = 101;
            this->InitializeAnInput(0, 1);
            this->InitializeAnInput(0, 2);
            this->InitializeAnInput(0, 3);
            this->InitializeAnInput(0, 4);
            this->InitializeAnInput(0, 5);

            unsigned int* deployments = deploymentResponse->GetDeployments();
            for (auto i = 0; i < deploymentResponse->PopulationCount; i++)
            {
                if (deployments[i] != 0)
                {
                    cout << "Requesting expansion " << i << " for model " << modelName << "\n";
                    protocol::ModelExpansionRequest request(modelName, i);
                    auto response = socket.TransactWithServer<protocol::ModelExpansionRequest, protocol::ModelExpansionResponse>(request);
                    auto* expansionResponse = reinterpret_cast<protocol::ModelExpansionResponse*>(response.get());

                    cout << "Initializing " << expansionResponse->ConnectionCount << " connections in expansion " << i << " with starting index " << expansionResponse->StartingNeuronOffset << " and count " << expansionResponse->NeuronCount << "\n";
                    InitializeExpansion(expansionResponse->StartingNeuronOffset, expansionResponse->ConnectionCount, expansionResponse->GetConnections());
                }
            }

            return true;
        }

    private:
        void InitializeExpansion(unsigned int offsetIndex, unsigned int connectionCount, protocol::ModelExpansionResponse::ConnectionType* connections)
        {
            ofstream csvfile;
            csvfile.open("./wiring.csv");
            csvfile << "presynaptic,postsynaptic,weight\n";

            for (auto i = 0; i < connectionCount; i++, connections++)
            {
                protocol::ModelExpansionResponse::ConnectionType& connection = *connections;
                csvfile << connection[0] + offsetIndex << "," << connection[1] + offsetIndex << "," << (int)connection[2] << "\n";
                this->helper_->Wire(connection[0] + offsetIndex, connection[1] + offsetIndex, (int)connection[2]);
            }
        }
    };
}
