#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "nlohmann/json.hpp"

#include "NeuronRecordCommon.h"
#include "CoreCommon.h"

//
//  Generic model helper interface.  Hides carrier implementation, providing
// a common API without clients needing to know the details of the carrier.
//
namespace embeddedpenguins::core::neuron::model
{
    using std::string;
    using std::vector;
    using std::tuple;

    using nlohmann::json;

    using embeddedpenguins::core::neuron::model::SynapseType;
    using embeddedpenguins::core::neuron::model::NeuronRecordType;

    class IModelHelper
    {
    public:
        virtual ~IModelHelper() = default;
        virtual json& Configuration() = 0;
        virtual const json& StackConfiguration() const = 0;
        virtual const string& ModelName() const = 0;
        virtual const string& DeploymentName() const = 0;
        virtual const string& EngineName() const = 0;
        virtual const string GetWiringFilename() const = 0;
        virtual const unsigned int Width() const = 0;
        virtual const unsigned int Height() const= 0;
        virtual bool AllocateModel(unsigned long int modelSize) = 0;
        virtual bool InitializeModel() = 0;
        virtual unsigned long long int GetIndex(const int row, const int column) const = 0;
        virtual unsigned long int GetNeuronTicksSinceLastSpike(const unsigned long int source) const = 0;
        virtual bool IsSynapseUsed(const unsigned long int neuronIndex, const unsigned int synapseId) const = 0;
        virtual int GetSynapticStrength(const unsigned long int neuronIndex, const unsigned int synapseId) const = 0;
        virtual unsigned long int GetPresynapticNeuron(const unsigned long int neuronIndex, const unsigned int synapseId) const = 0;
        virtual void WireInput(unsigned long int sourceNodeIndex, int synapticWeight, SynapseType type) = 0;
        virtual void Wire(unsigned long int sourceNodeIndex, unsigned long int targetNodeIndex, int synapticWeight, SynapseType type) = 0;
        virtual short GetNeuronActivation(const unsigned long int source) const = 0;
        virtual vector<tuple<unsigned long long, short int, short int, unsigned short, short int, NeuronRecordType>> CollectRelevantNeurons(bool includeSynapses, bool includeActivation, bool includeHypersensitive) = 0;
        virtual unsigned long int FindRequiredSynapseCounts() = 0;
   };
}
