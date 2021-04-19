#pragma once

#include <iostream>

#include "nlohmann/json.hpp"

#include "IQueryHandler.h"
#include "Recorder.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;

    using nlohmann::json;


    template<class RECORDTYPE>
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
            try
            {
                auto jsonQuery = json::parse(query);
                cout << "HandleQuery: " << jsonQuery << "\n";

                ParseQuery(jsonQuery);
            }
            catch (json::exception ex)
            {
                cout << "HandleQuery failed to parse query string '" << query << "'\n";
                json response;
                response_ = BuildError(response, "format", (string(ex.what()) + " parsing json query").c_str()).dump();
            }
            
            return response_;
        }

    private:
        void ParseQuery(const json& jsonQuery)
        {
            json response;
            response["Query"] = jsonQuery;

            string query;
            if (jsonQuery.contains("Query"))
                query = jsonQuery["Query"].get<string>();

            if (query == "Status")
                response_ = BuildStatus(response).dump();
            else
                response_ = BuildError(response, "unrecognized", "Json query contains no recognized request").dump();
        }

        json& BuildStatus(json& response)
        {
            json statusResponse;
            statusResponse["RecordEnable"] = Recorder<RECORDTYPE>::Enabled();

            response["Response"] = statusResponse;

            return response;
        }

        json& BuildError(json& response, const char* error, const char* errorDetail)
        {
            json errorResponse;
            errorResponse["Error"] = error;
            errorResponse["ErrorDetail"] = errorDetail;

            response["Error"] = errorResponse;

            return response;
        }
    };
}
