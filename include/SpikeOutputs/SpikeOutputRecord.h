#pragma once

#include <iostream>
#include <fstream>
#include <map>

#include "nlohmann/json.hpp"

#include "ConfigurationRepository.h"
#include "Recorder.h"
#include "SpikeOutputs/ISpikeOutput.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::ifstream;
    using std::multimap;
    using std::make_pair;

    using nlohmann::json;

    constexpr unsigned int MaxBufferBeforeFlush { 10'000 };

    template <class RECORDTYPE>
    class SpikeOutputRecord : public ISpikeOutput
    {
        ModelEngineContext& context_;
        ConfigurationRepository& configuration_;
        unsigned long long int& ticks_;
        Recorder<RECORDTYPE> recorder_;
        unsigned int lineCount_ { };

    public:
        SpikeOutputRecord(ModelEngineContext& context) :
            context_(context),
            configuration_(context_.Configuration),
            ticks_(context_.Measurements.Iterations),
            recorder_(ticks_, configuration_)
        {
            cout << "SpikOutputRecord constructor\n";
        }

        // ISpikeOutput implementaton
        virtual void CreateProxy(ModelEngineContext& context) override { }

        virtual bool Connect() override
        {
            return true;
        }

        virtual bool Disconnect() override
        {
            cout << "Recorder disconnecting\n";
            Recorder<RECORDTYPE>::Finalize();
            return true;
        }

        //
        // We obey the disable flag, we can be turned off.
        //
        virtual bool RespectDisableFlag() override { return true; }
        
        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, unsigned short synapseIndex, short int synapseStrength, NeuronRecordType type) override
        {
            // If the configured record file path is empth, don't both recording.
            if (configuration_.ComposeRecordPath().empty()) return;
            
            RECORDTYPE record(type, neuronIndex, activation, synapseIndex, synapseStrength);
            recorder_.Record(record);

            FlushWhenBufferFull();
        }

        virtual void Flush() override
        {
            Recorder<RECORDTYPE>::Merge(recorder_);
            recorder_.Print();
            
            lineCount_ = 0;
        }

    private:
        void FlushWhenBufferFull()
        {
            ++lineCount_;
            if (lineCount_ >= MaxBufferBeforeFlush)
            {
                Recorder<RECORDTYPE>::Merge(recorder_);
                recorder_.Print();
                
                lineCount_ = 0;
            }
        }
    };
}