#pragma once

#include <vector>

#include "nlohmann/json.hpp"

#include "NeuronRecordCommon.h"
#include "ModelContext.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::vector;

    using nlohmann::json;

    class ISpikeOutput
    {
    public:
        virtual ~ISpikeOutput() = default;

        virtual void CreateProxy(ModelContext& context) = 0;
        virtual bool Connect() = 0;
        virtual bool Connect(const string& connectionString, unsigned int filterBottom, unsigned int filterLength, unsigned int toIndex, unsigned int toOffset) = 0;
        virtual bool Disconnect() = 0;
        virtual bool RespectDisableFlag() = 0;
        virtual bool IsInterestedIn(NeuronRecordType type) { return true; }
        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, short int hypersensitive, unsigned short synapseIndex, short int synapseStrength, NeuronRecordType type) = 0;
        virtual void Flush() = 0;
    };
}
