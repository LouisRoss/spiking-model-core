#pragma once

#include <limits>

namespace embeddedpenguins::core::neuron::model
{
    using std::numeric_limits;

    enum class NeuronRecordType
    {
        InputSignal,
        Decay,
        Spike,
        Refractory,
        SynapseAdjust,
        HyperSensitive
    };

    enum class NeuronType : char
    {
        Excitatory,
        Inhibitory,
        Attention
    };

    struct NeuronRecordConnection
    {
        unsigned long long int NeuronIndex { };
        unsigned int SynapseIndex { };
    };

    struct NeuronRecordSynapse
    {
        unsigned int SynapseIndex { numeric_limits<unsigned int>::max() };
        int Strength { 0 };
    };
}
