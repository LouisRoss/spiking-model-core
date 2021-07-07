#pragma once

#include <string>
#include <vector>

#include "NeuronRecordCommon.h"
#include "ModelEngineContext.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::string;
    using std::vector;

    using embeddedpenguins::gpu::neuron::model::ModelEngineContext;

    class ISpikeOutput
    {
    public:
    /*
        enum class SpikeEventType
        {
            SpikeTime,
            RefractoryTime,
            SpikeRecentTime
        };
    */

        virtual ~ISpikeOutput() = default;

        virtual void CreateProxy(ModelEngineContext& context) = 0;
        virtual bool Connect(const string& connectionString) = 0;
        virtual bool Disconnect() = 0;
        virtual bool IsInterestedIn(NeuronRecordType type) { return true; }
        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, NeuronRecordType type) = 0;
        virtual void Flush() = 0;
    };
}
