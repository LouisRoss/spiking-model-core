#pragma once

#include "IModelHelper.h"
#include "ModelNeuronInitializer.h"

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
            // TODO!!
            this->helper_->InitializeModel();

        }
    };
}
