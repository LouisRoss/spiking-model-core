#pragma once

#include <string>

namespace embeddedpenguins::core::neuron::model
{
    using std::string;

    class IQueryHandler
    {
    public:
        virtual ~IQueryHandler() = default;

        virtual const string& HandleQuery(const string& query) = 0;
    };
}
