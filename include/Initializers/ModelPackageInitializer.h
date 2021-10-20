#pragma once

#include "IModelHelper.h"
#include "ModelNeuronInitializer.h"
#include "PackageInitializerDataSocket.h"

namespace embeddedpenguins::core::neuron::model
{
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
        virtual void Initialize() override
        {
            cout << "ModelPackageInitializer::Initialize()\n";
            this->helper_->InitializeModel();

            this->strength_ = 101;
            this->InitializeAnInput(0, 1);
            this->InitializeAnInput(0, 2);
            this->InitializeAnInput(0, 3);
            this->InitializeAnInput(0, 4);
            this->InitializeAnInput(0, 5);

            // TODO - develop the model name from somewhere!
            string modelName { "layer" };

            PackageInitializerDataSocket socket(this->helper_->StackConfiguration());

            for (auto i = 0; i < this->helper_->ExpansionCount(); i++)
            {
                cout << "Requesting expansion " << i << " for model " << modelName << "\n";
                protocol::ModelExpansionRequest request(modelName, i);
                auto response = socket.TransactWithServer<protocol::ModelExpansionRequest, protocol::ModelExpansionResponse>(request);
                auto* expansionResponse = reinterpret_cast<protocol::ModelExpansionResponse*>(response.get());

                cout << "Initializing " << expansionResponse->ConnectionCount << " connections in expansion " << i << " with starting index " << expansionResponse->StartingNeuronOffset << " and count " << expansionResponse->NeuronCount << "\n";
                InitializeExpansion(expansionResponse->StartingNeuronOffset, expansionResponse->ConnectionCount, expansionResponse->GetConnections());
            }
        }

    private:
        void InitializeExpansion(unsigned int offsetIndex, unsigned int connectionCount, protocol::ModelExpansionResponse::ConnectionType* connections)
        {
            for (auto i = 0; i < connectionCount; i++, connections++)
            {
                protocol::ModelExpansionResponse::ConnectionType& connection = *connections;
                this->helper_->Wire(connection[0] + offsetIndex, connection[1] + offsetIndex, connection[2]);
            }
        }
    };
}
