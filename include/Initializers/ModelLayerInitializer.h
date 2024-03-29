#pragma once

#include <vector>

#include "nlohmann/json.hpp"

#include "IModelHelper.h"
#include "ModelNeuronInitializer.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::vector;

    using nlohmann::json;

    //
    // This custom initializer sets up a spiking neuron model for 
    // the 'layer' test, which demonstrates a continuous wave action
    // in layers.
    //
    class ModelLayerInitializer : public ModelNeuronInitializer
    {
        vector<IModelInitializer::SpikeOutputDescriptor> outputDescriptors_ { };

    public:
        ModelLayerInitializer(IModelHelper* helper, ModelContext* context) :
            ModelNeuronInitializer(helper, context)
        {
        }

        virtual bool Initialize() override
        {
            this->helper_->InitializeModel();

            this->strength_ = 101;
            this->InitializeAnInput(0, 1);
            this->InitializeAnInput(0, 2);
            this->InitializeAnInput(0, 3);
            this->InitializeAnInput(0, 4);
            this->InitializeAnInput(0, 5);

            int initialStrength = 102;
            this->strength_ = initialStrength;

            for (auto row = 0; row < this->helper_->Height() - 1; row++)
            {
                InitializeARow(row, row + 1);
            }
            InitializeARow(this->helper_->Height() - 1, 0);

            return true;
        }

        virtual const vector<SpikeOutputDescriptor>& GetInitializedOutputs() const override
        {
            return outputDescriptors_;
        }

    private:
        void InitializeARow(int row, int destRow)
        {
            for (auto column = 0; column < this->helper_->Width(); column++)
                for (auto destCol = 0; destCol < this->helper_->Width(); destCol++)
                    this->InitializeAConnection(row, column, destRow, destCol);
        }
    };
}
