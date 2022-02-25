#pragma once
#include <memory>

namespace embeddedpenguins::core::neuron::model
{
    using std::unique_ptr;
    using std::make_unique;

    struct SpikeSignal
    {
        int Tick;
        unsigned int NeuronIndex;
    };

    constexpr unsigned int SpikeSignalBufferCount { 250 };
    constexpr unsigned int SpikeSignalSize { sizeof(SpikeSignal) };

    using SpikeSignalLengthFieldType = unsigned short int;
    constexpr unsigned int SpikeSignalLengthSize { sizeof(SpikeSignalLengthFieldType) };

    class SpikeSignalProtocol
    {
        unsigned int capacity_;
        unsigned int currentSpikeBufferIndex_ { 0 };
        unique_ptr<SpikeSignal[]> spikeBuffer_;

    public:
        SpikeSignalProtocol(unsigned int capacity = SpikeSignalBufferCount) :
            capacity_(capacity),
            spikeBuffer_(make_unique<SpikeSignal[]>(capacity))
        {

        }

        bool Buffer(const SpikeSignal& signal)
        {
            auto& insertSignal = spikeBuffer_[currentSpikeBufferIndex_];
            insertSignal = signal;

            if (currentSpikeBufferIndex_ >= capacity_)
            {
                return true;
            }

            currentSpikeBufferIndex_++;
            return false;
        }

        SpikeSignal* SpikeBuffer() { return spikeBuffer_.get(); }
        SpikeSignal& GetElementAt(SpikeSignalLengthFieldType index) { return spikeBuffer_[index]; }
        bool Empty() const { return currentSpikeBufferIndex_ == 0; }
        void Reset() { currentSpikeBufferIndex_ = 0; }
        unsigned int SpikeSignalBufferSize() const { return capacity_ * SpikeSignalSize; }
        SpikeSignalLengthFieldType GetCurrentBufferCount() const { return currentSpikeBufferIndex_; }
        SpikeSignalLengthFieldType GetBufferSize(SpikeSignalLengthFieldType spikeBufferIndex) const { return spikeBufferIndex * SpikeSignalSize; }
    };
}
