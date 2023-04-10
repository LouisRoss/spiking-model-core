#pragma once

#include <string>
#include <vector>

#include "IModelHelper.h"
#include "ModelContext.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::string;
    using std::vector;

    //
    // All model initializers must implement this interface, to allow
    // interchangability between initializers.
    //
    class IModelInitializer
    {
    public:
        struct SpikeOutputDescriptor
        {
            string Host {};
            unsigned int ModelSequence {};
            unsigned long ModelOffset {};
            unsigned long Size {};
            unsigned int LocalModelSequence {};
            unsigned long LocalModelOffset {};
            unsigned long LocalStart {};
        };

    public:
        virtual ~IModelInitializer() = default;

        //
        // Proxy only: Load the shared library that this is a proxy for.
        // Implementations may stube this out, but must provide it.
        //
        virtual void CreateProxy(IModelHelper* helper, ModelContext* context) = 0;

        //
        // Called before the model is run, this required method must 
        // initialize any state needed for a specific use case.
        //
        virtual bool Initialize() = 0;

        //
        // After calling Initialize(), some initializers may have developed
        // a vector of spike output descriptors that may be used to
        // create ISpikeOutput implementations.
        //
        virtual const vector<SpikeOutputDescriptor>& GetInitializedOutputs() const = 0;
    };
}
