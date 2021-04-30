#pragma once

#include <iostream>

#include "nlohmann/json.hpp"

#include "IQueryHandler.h"
#include "Recorder.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;

    using nlohmann::json;


    template<class RECORDTYPE, class CONTEXTTYPE>
    class CommandControlHandler : public IQueryHandler
    {
        CONTEXTTYPE& context_;
        string response_ { };

    public:
        CommandControlHandler(CONTEXTTYPE& context) :
            context_(context)
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
                response_ = BuildErrorResponse(response, "format", (string(ex.what()) + " parsing json query").c_str()).dump();
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
                response_ = BuildStatusResponse(response).dump();
            else if (query == "Control")
                response_ = BuildControlResponse(jsonQuery, response).dump();
            else
                response_ = BuildErrorResponse(response, "unrecognized", "Json query contains no recognized request").dump();
        }

        json& BuildStatusResponse(json& response)
        {
            json statusResponse = context_.Render();
            statusResponse["RecordEnable"] = Recorder<RECORDTYPE>::Enabled();

            response["Response"] = statusResponse;

            return response;
        }

        json& BuildControlResponse(const json& jsonQuery, json& response)
        {
            json controlResponse;

            if (jsonQuery.contains("Values"))
            {
                auto& controlValues = jsonQuery["Values"];
                if (controlValues.contains("RecordEnable"))
                {
                    Recorder<RECORDTYPE>::Enable(controlValues["RecordEnable"].get<bool>());
                }
                // Do more as they come up...

                controlResponse["Result"] = "Ok";
                response["Response"] = controlResponse;
            }
            else
            {
                return BuildErrorResponse(response, "missing Values", "Query json must contain a 'Values' element at the root");
            }

            return response;
        }

        json& BuildErrorResponse(json& response, const char* error, const char* errorDetail)
        {
            json errorResponse;
            errorResponse["Error"] = error;
            errorResponse["ErrorDetail"] = errorDetail;

            response["Error"] = errorResponse;

            return response;
        }
    };
}
