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

    using embeddedpenguins::core::neuron::model::ToRecordType;

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
        void InitializeExpansion(unsigned int offsetIndex, unsigned int connectionCount, protocol::ModelExpansionResponse::Connection* connections)
        {
            auto wiringFilename = helper_->GetWiringFilename();
            bool useWiringFile = !wiringFilename.empty();

            ofstream csvfile;
            if (useWiringFile)
            {
                csvfile.open(wiringFilename);
                csvfile << "presynaptic,postsynaptic,weight,type\n";
            }

            for (auto i = 0; i < connectionCount; i++, connections++)
            {
                protocol::ModelExpansionResponse::Connection& connection = *connections;
                if (useWiringFile) csvfile << connection.PreSynapticNeuron + offsetIndex << "," << connection.PostSynapticNeuron + offsetIndex << "," << (int)connection.SynapticStrength << "," << (int)connection.Type << "\n";
                this->helper_->Wire(connection.PreSynapticNeuron + offsetIndex, connection.PostSynapticNeuron + offsetIndex, (int)connection.SynapticStrength, ToRecordType(connection.Type));
            }
        }
    };
}
