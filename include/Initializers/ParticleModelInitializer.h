#pragma once

#include "ConfigurationRepository.h"
#include "ModelInitializer.h"

#include "ParticleOperation.h"
#include "ParticleSupport.h"
#include "ParticleRecord.h"

namespace embeddedpenguins::particle::infrastructure
{
    using embeddedpenguins::core::neuron::model::ConfigurationRepository;
    using embeddedpenguins::modelengine::sdk::ModelInitializer;

    template<class MODELHELPERTYPE>
    class ParticleModelInitializer : public ModelInitializer<MODELHELPERTYPE>
    {
    public:
        ParticleModelInitializer(MODELHELPERTYPE& helper) :
            ModelInitializer<MODELHELPERTYPE>(helper)
        {
        }

        virtual void Initialize() override
        {
            this->helper_.InitializeModel();
        }
    };
}
