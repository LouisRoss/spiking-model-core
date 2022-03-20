#pragma once

#include "Initializers/PackageInitializerProtocol.h"
#include "NeuronRecordCommon.h"

namespace embeddedpenguins::core::neuron::model
{
    using embeddedpenguins::core::neuron::model::initializerprotocol::ModelExpansionResponse;
    using embeddedpenguins::core::neuron::model::NeuronType;

    NeuronType ToRecordType(ModelExpansionResponse::ConnectionType wireType)
    {
        switch (wireType)
        {
            case ModelExpansionResponse::ConnectionType::Excitatory:
                return NeuronType::Excitatory;

            case ModelExpansionResponse::ConnectionType::Inhibitory:
                return NeuronType::Inhibitory;

            case ModelExpansionResponse::ConnectionType::Attention:
                return NeuronType::Attention;

            default:
                return NeuronType::Excitatory;
        }

    }
}
