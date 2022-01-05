#pragma once

#include "IModelHelper.h"
#include "ModelNeuronInitializer.h"

namespace embeddedpenguins::core::neuron::model
{
    //
    // This custom initializer sets up a spiking neuron model for 
    // the 'anticipate' test, which demonstrates STDP over repeated
    // spikes.
    //
    class ModelAnticipateInitializer : public ModelNeuronInitializer
    {
    public:
        ModelAnticipateInitializer(IModelHelper* helper) :
            ModelNeuronInitializer(helper)
        {
        }

    public:
        // IModelInitializer implementaton
        virtual bool Initialize() override
        {
            this->helper_->InitializeModel();

            const Neuron2Dim I1 { this->ResolveNeuron("I1") };
            const Neuron2Dim I2 { this->ResolveNeuron("I2") };
            const Neuron2Dim Inh1  { this->ResolveNeuron("Inh1") };
            const Neuron2Dim Inh2  { this->ResolveNeuron("Inh2") };
            const Neuron2Dim N1  { this->ResolveNeuron("N1") };
            const Neuron2Dim N2  { this->ResolveNeuron("N2") };

            this->strength_ = 102;
            this->InitializeAConnection(I1, N1);
            this->strength_ = 51;
            this->InitializeAConnection(N2, N1);

            this->strength_ = 102;
            this->InitializeAConnection(I2, N2);
            this->strength_ = 51;
            this->InitializeAConnection(N1, N2);

            this->strength_ = 102;
            this->InitializeAnInput(I1);
            this->InitializeAnInput(I2);

            this->strength_ = 102;
            this->SetInhibitoryNeuronType(Inh1);
            this->InitializeAConnection(N1, Inh1);
            this->InitializeAConnection(Inh1, I1);

            this->strength_ = 102;
            this->SetInhibitoryNeuronType(Inh2);
            this->InitializeAConnection(N2, Inh2);
            this->InitializeAConnection(Inh2, I2);

            return true;
        }
    };
}
