#pragma once

#include "nlohmann/json.hpp"

#include "ModelNeuronInitializer.h"

namespace embeddedpenguins::core::neuron::model
{
    using nlohmann::json;

    //
    // This custom initializer sets up a spiking neuron model for 
    // the 'layer' test, which demonstrates a continuous wave action
    // in layers.
    //
    template<class MODELHELPERTYPE>
    class ModelLayerInitializer : public ModelNeuronInitializer<MODELHELPERTYPE>
    {
    public:
        ModelLayerInitializer(MODELHELPERTYPE& helper) :
            ModelNeuronInitializer<MODELHELPERTYPE>(helper)
        {
        }

        virtual void Initialize() override
        {
            this->helper_.InitializeModel();

            this->strength_ = 101;
            this->InitializeAnInput(0, 1);
            this->InitializeAnInput(0, 2);
            this->InitializeAnInput(0, 3);
            this->InitializeAnInput(0, 4);
            this->InitializeAnInput(0, 5);

            this->strength_ = this->Configuration()["Model"]["InitialSynapticStrength"];
            for (auto row = 0; row < this->helper_.Height() - 1; row++)
            {
                InitializeARow(row, row + 1);
            }
            InitializeARow(this->helper_.Height() - 1, 0);
        }

    private:
        void InitializeARow(int row, int destRow)
        {
            for (auto column = 0; column < this->helper_.Width(); column++)
                for (auto destCol = 0; destCol < this->helper_.Width(); destCol++)
                    this->InitializeAConnection(row, column, destRow, destCol);
        }
    };
}
