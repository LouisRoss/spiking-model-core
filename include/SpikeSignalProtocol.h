#pragma once
#include <memory>

namespace embeddedpenguins::core::neuron::model
{
    using std::unique_ptr;
    using std::make_unique;

    using SpikeSignalLengthFieldType = unsigned int;
    constexpr unsigned int SpikeSignalLengthSize { sizeof(SpikeSignalLengthFieldType) };

    struct SpikeSignal
    {
        int Tick;
        unsigned int NeuronIndex;
    };

    constexpr unsigned int SpikeSignalBufferCount { 250 };
    constexpr unsigned int SpikeSignalSize { sizeof(SpikeSignal) };

    using SpikeOffsetFieldType = unsigned int;
    constexpr unsigned int SpikeOffsetSize { sizeof(SpikeOffsetFieldType) };
    using PopulationIndexFieldType = unsigned int;      // The 0-based index of the population (expansion) within the model.
    constexpr unsigned int PopulationIndexSize { sizeof(PopulationIndexFieldType) };
    using LayerOffsetFieldType = unsigned int;          // The offset in neurons of the start of the layer within the expansion.
    constexpr unsigned int LayerOffsetSize { sizeof(LayerOffsetFieldType) };

    struct SpikeEnvelope
    {
        SpikeSignalLengthFieldType PacketSize { 0 };
    };

    struct SpikeHeader : public SpikeEnvelope
    {
        PopulationIndexFieldType PopulationIndex { };
        LayerOffsetFieldType LayerOffset { };
    };

    struct SpikeSignalPacket : public SpikeHeader
    {
        // Immediately following this struct in memory should be an array
        // SpikeSignal Signals[capacity_];
        SpikeSignal* GetSpikeSignals() { return reinterpret_cast<SpikeSignal*>(this + 1); }
    };

    class SpikeSignalProtocol
    {
        SpikeSignalLengthFieldType capacity_;
        SpikeSignalLengthFieldType currentSpikeBufferIndex_ { 0 };
        unique_ptr<char[]> instance_;

    public:
        //
        //  Construct a new protocol with default values.  Typically used to
        // send spike signals to a service that receives spikes for the whole model,
        // such as UI/UX.
        //
        SpikeSignalProtocol() :
            capacity_(SpikeSignalBufferCount),
            instance_(make_unique<char[]>(GetBufferSize(capacity_)))
        {
            GetProtocolBuffer()->PacketSize = GetPacketSize();
            GetProtocolBuffer()->PopulationIndex = 0;
            GetProtocolBuffer()->LayerOffset = 0;
        }

        //
        //  Construct a new protocol with specified values.  This is used for
        // interconnects between population1/layer1 -> population2/layer2.
        //
        SpikeSignalProtocol(PopulationIndexFieldType targetPopulationIndex, SpikeSignalLengthFieldType targetLayerOffset, unsigned int capacity) :
            capacity_(capacity),
            instance_(make_unique<char[]>(GetBufferSize(capacity)))
        {
            GetProtocolBuffer()->PacketSize = GetPacketSize();
            GetProtocolBuffer()->PopulationIndex = targetPopulationIndex;
            GetProtocolBuffer()->LayerOffset = targetLayerOffset;
        }

        //  Construct a new protocol with the specified byte count.
        // Used by the receiver after receiving the byte count from the envelope.
        //
        SpikeSignalProtocol(SpikeSignalLengthFieldType byteCount) :
            capacity_((byteCount - (sizeof(SpikeHeader) - sizeof(SpikeEnvelope))) / SpikeSignalSize),
            instance_(make_unique<char[]>(GetBufferSize(capacity_)))
        {
            GetProtocolBuffer()->PacketSize = byteCount;
        }

        //
        //  Add a spike signal to the current buffer.  Keep track of the remaining
        // capacity, and keep the full packet size in the envelope up to date.
        //
        bool Buffer(const SpikeSignal& signal)
        {
            auto* insertSignal = GetProtocolBuffer()->GetSpikeSignals() + currentSpikeBufferIndex_;
            *insertSignal = signal;

            if (currentSpikeBufferIndex_ >= capacity_)
            {
                return true;
            }

            currentSpikeBufferIndex_++;
            GetProtocolBuffer()->PacketSize = GetPacketSize();
            return false;
        }

        // Cast the internal byte buffer to a pointer to SpikeSignalPacket.
        SpikeSignalPacket* GetProtocolBuffer() 
        {
            return reinterpret_cast<SpikeSignalPacket*>(instance_.get());
        }

        // The full buffer size, including the envelope and header, needed for the current number of neurons.
        SpikeSignalLengthFieldType GetBufferSize() const 
        {
            return GetBufferSize(currentSpikeBufferIndex_);
        }

        // The full buffer size, including the envelope and header, needed for a given number of neurons.
        static SpikeSignalLengthFieldType GetBufferSize(SpikeSignalLengthFieldType spikeCount)
        {
            return sizeof(SpikeSignalPacket) + spikeCount * SpikeSignalSize;
        }

        // The packet size that the envelope should show for the current number of neurons.
        unsigned int GetPacketSize() const
        {
            return GetBufferSize(currentSpikeBufferIndex_) - sizeof(SpikeEnvelope);
        }

        // A read/write reference to the neuron indicated.
        SpikeSignal& GetElementAt(SpikeSignalLengthFieldType index)
        {
            return GetProtocolBuffer()->GetSpikeSignals()[index];
        }

        bool IsEmpty() const { return currentSpikeBufferIndex_ == 0; }
        unsigned int SpikeSignalBufferSize() const { return capacity_ * SpikeSignalSize; }
        SpikeSignalLengthFieldType GetCurrentBufferCount() const { return currentSpikeBufferIndex_; }
        unsigned int GetCapacity() const { return capacity_; }

        void Reset() { currentSpikeBufferIndex_ = 0; }
    };
}
