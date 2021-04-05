#pragma once

#include <iostream>

#include "IQueryHandler.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;

    class CommandControlHandler : public IQueryHandler
    {
        string response_ { };

    public:
        CommandControlHandler()
        {

        }

        // IQueryHandler implementation.
        virtual const string& HandleQuery(const string& query) override
        {
            cout << "HandleQuery: " << query << "\n";

            // TODO handle stuff.
            response_ = query;
            
            return response_;
        }
    };
}
