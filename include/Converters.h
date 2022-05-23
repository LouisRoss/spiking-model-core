#pragma once

#include "Initializers/PackageInitializerProtocol.h"
#include "CoreCommon.h"

namespace embeddedpenguins::core::neuron::model
{
    using embeddedpenguins::core::neuron::model::initializerprotocol::ModelExpansionResponse;

    SynapseType ToModelType(ModelExpansionResponse::ConnectionType wireType)
    {
        switch (wireType)
        {
            case ModelExpansionResponse::ConnectionType::Excitatory:
                return SynapseType::Excitatory;

            case ModelExpansionResponse::ConnectionType::Inhibitory:
                return SynapseType::Inhibitory;

            case ModelExpansionResponse::ConnectionType::Attention:
                return SynapseType::Attention;

            default:
                return SynapseType::Excitatory;
        }

    }
}
