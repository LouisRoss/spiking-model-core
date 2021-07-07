#pragma once

namespace embeddedpenguins::core::neuron::model
{
    struct SpikeSignal
    {
        int Tick;
        unsigned int NeuronIndex;
    };

    constexpr unsigned int SpikeSignalBufferCount { 250 };
    constexpr unsigned int SpikeSignalSize { sizeof(SpikeSignal) };
    constexpr unsigned int SpikeSignalBufferSize { SpikeSignalBufferCount * SpikeSignalSize };

    using SpikeSignalLengthFieldType = unsigned short int;
    constexpr unsigned int SpikeSignalLengthSize { sizeof(SpikeSignalLengthFieldType) };

    class SpikeSignalProtocol
    {
    public:
        SpikeSignal SpikeBuffer[SpikeSignalBufferCount];
        unsigned int CurrentSpikeBufferIndex { 0 };

        bool Buffer(const SpikeSignal& signal)
        {
            auto& insertSignal = SpikeBuffer[CurrentSpikeBufferIndex];
            insertSignal = signal;

            if (CurrentSpikeBufferIndex >= SpikeSignalBufferCount)
            {
                return true;
            }

            CurrentSpikeBufferIndex++;
            return false;
        }

        SpikeSignalLengthFieldType GetCurrentBufferCount() { return CurrentSpikeBufferIndex; }
        SpikeSignalLengthFieldType GetBufferSize(SpikeSignalLengthFieldType spikeBufferIndex) { return spikeBufferIndex * SpikeSignalSize; }
    };
}
