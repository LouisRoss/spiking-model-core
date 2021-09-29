#pragma once

#include "IModelHelper.h"
#include "IModelInitializer.h"

namespace embeddedpenguins::modelengine::sdk
{
    using embeddedpenguins::core::neuron::model::IModelHelper;
    using embeddedpenguins::core::neuron::model::IModelInitializer;

    //
    // Base class for all model initializers.  Since an initializer
    // must reference the configuration, a reference to it is captured here.
    //
    class ModelInitializer : public IModelInitializer
    {
    protected:
        IModelHelper* helper_;

    public:
        ModelInitializer(IModelHelper* helper) :
            helper_(helper)
        {
        }

    public:
        // IModelInitializer interface, must be implemented, but not used if not a proxy class.
        virtual void CreateProxy(IModelHelper* helper) override { }
    };
}
