#pragma once

namespace embeddedpenguins::core::neuron::model
{
    class ICommandControlAcceptor
    {
    public:
        virtual ~ICommandControlAcceptor() = default;

        virtual bool Initialize() = 0;

        virtual void AcceptAndExecute() = 0;
    };
}
