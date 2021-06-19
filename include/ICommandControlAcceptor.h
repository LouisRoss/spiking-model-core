#pragma once

#include <string>
#include <memory>

#include "IQueryHandler.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::string;
    using std::unique_ptr;

    class ICommandControlAcceptor
    {
    public:
        virtual ~ICommandControlAcceptor() = default;

        virtual const string& Description() = 0;
        
        virtual bool ParseArguments(int argc, char* argv[]) = 0;

        virtual bool Initialize() = 0;

        virtual bool AcceptAndExecute(unique_ptr<IQueryHandler> const & queryHandler) = 0;
    };
}
