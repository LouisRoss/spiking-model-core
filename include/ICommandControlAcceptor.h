#pragma once

#include <string>
#include <functional>

namespace embeddedpenguins::core::neuron::model
{
    using std::string;
    using std::function;

    class ICommandControlAcceptor
    {
    public:
        virtual ~ICommandControlAcceptor() = default;

        virtual const string& Description() = 0;
        
        virtual bool ParseArguments(int argc, char* argv[]) = 0;

        virtual bool Initialize() = 0;

        virtual bool AcceptAndExecute(function<void(const string&)> commandHandler) = 0;
    };
}
