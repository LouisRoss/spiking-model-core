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
        const ConfigurationRepository& configuration_;
        unsigned long long int& ticks_;
        Recorder<RECORDTYPE> recorder_;
        unsigned int lineCount_ { };

    public:
        SpikeOutputRecord(ModelEngineContext& context) :
            context_(context),
            configuration_(context_.Configuration),
            ticks_(context_.Iterations),
            recorder_(ticks_, configuration_)
        {
            cout << "SpikOutputRecord constructor\n";
        }

        // ISpikeOutput implementaton
        virtual void CreateProxy(ModelEngineContext& context) override { }

        virtual bool Connect(const string& connectionString) override
        {
            return true;
        }

        virtual bool Disconnect() override
        {
            return true;
        }

        virtual void StreamOutput(unsigned long long neuronIndex, short int activation, NeuronRecordType type) override
        {
            if (!context_.RecordEnable) return;
            
            RECORDTYPE record(type, neuronIndex, activation);
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