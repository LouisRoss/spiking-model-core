#pragma once

#include <vector>
#include <chrono>

#include "ConfigurationRepository.h"
#include "ModelEngine.h"
#include "sdk/ModelInitializer.h"

#include "LifeNode.h"
#include "LifeOperation.h"
#include "LifeSupport.h"
#include "LifeRecord.h"
#include "LifeModelCarrier.h"

namespace embeddedpenguins::life::infrastructure
{
    using std::vector;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    using std::chrono::hours;
    using std::chrono::duration_cast;

    using embeddedpenguins::core::neuron::model::ConfigurationRepository;
    using embeddedpenguins::modelengine::ModelEngine;
    using embeddedpenguins::modelengine::sdk::ModelInitializer;

    template<class MODELHELPERTYPE>
    class ModelLifeInitializer : public ModelInitializer<MODELHELPERTYPE>
    {
    public:
        ModelLifeInitializer(MODELHELPERTYPE& helper) :
            ModelInitializer<MODELHELPERTYPE>(helper)
        {

        }

        virtual void Initialize() override
        {
            this->helper_.InitializeModel();
        }
    };
}
