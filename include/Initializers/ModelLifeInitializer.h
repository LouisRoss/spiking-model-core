#pragma once

#include <vector>
#include <chrono>

#include "ConfigurationRepository.h"
#include "ModelEngine.h"
#include "ModelInitializer.h"

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

    //
    // This custom initializer sets up a spiking neuron model for 
    // the 'life' test, which demonstrates Conway's game of life
    // as a finite-state automaton.
    //
    template<class MODELHELPERTYPE>
    class ModelLifeInitializer : public ModelInitializer<MODELHELPERTYPE>
    {
    public:
        ModelLifeInitializer(MODELHELPERTYPE& helper) :
            ModelInitializer<MODELHELPERTYPE>(helper)
        {

        }

        virtual bool Initialize() override
        {
            this->helper_.InitializeModel();
            return true;
        }
    };
}
