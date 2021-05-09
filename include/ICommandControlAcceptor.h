#pragma once

namespace embeddedpenguins::core::neuron::model
{
    class ICommandControlAcceptor
    {
    public:
        virtual ~ICommandControlAcceptor() = default;

        virtual const string& Description() = 0;
        
        virtual bool Initialize(int argc, char* argv[]) = 0;

        virtual bool AcceptAndExecute() = 0;
    };
}
