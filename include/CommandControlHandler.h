#pragma once

#include <iostream>
#include <string>
#include <limits>
#include <filesystem>

#include "nlohmann/json.hpp"

#include "IQueryHandler.h"
#include "IModelRunner.h"
#include "Recorder.h"
#include "Log.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::numeric_limits;
    using std::filesystem::directory_iterator;
    using std::filesystem::exists;

    using nlohmann::json;


    template<class RECORDTYPE>
    class CommandControlHandler : public IQueryHandler
    {
        IModelRunner& runner_;
        string response_ { };

    public:
        CommandControlHandler(IModelRunner& runner) :
            runner_(runner)
        {

        }

        // IQueryHandler implementation.
        virtual const string& HandleQuery(const string& query) override
        {
            try
            {
                response_.clear();

                auto jsonQuery = json::parse(query);
                if (!jsonQuery.contains("query") || !jsonQuery["query"].is_string() || jsonQuery["query"].get<string>() != "fullstatus")
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
            response["query"] = jsonQuery;

            string query;
            if (jsonQuery.contains("query"))
                query = jsonQuery["query"].get<string>();

            if (query == "fullstatus")
                response_ = BuildFullStatusResponse(response).dump();
            else if (query == "dynamicstatus")
                response_ = BuildDynamicStatusResponse(response).dump();
            else if (query == "runmeasurements")
                response_ = BuildRunMeasurementsResponse(response).dump();
            else if (query == "settings")
                response_ = BuildSettingsResponse(jsonQuery, response).dump();
            else if (query == "control")
                response_ = BuildControlResponse(jsonQuery, response).dump();
            else if (query == "deploy")
                response_ = BuildDeployResponse(jsonQuery, response).dump();
            else
                response_ = BuildErrorResponse(response, "unrecognized", "Json query contains no recognized request").dump();
        }

        json& BuildFullStatusResponse(json& response)
        {
            json responseResponse;
            responseResponse["status"] = BuildFullStatusElement();
            responseResponse["result"] = "ok";

            response["response"] = responseResponse;

            return response;
        }

        json& BuildDynamicStatusResponse(json& response)
        {
            json responseResponse;
            responseResponse["status"] = runner_.RenderDynamicStatus();
            responseResponse["result"] = "ok";

            response["response"] = responseResponse;

            return response;
        }

        json& BuildRunMeasurementsResponse(json& response)
        {
            json responseResponse;
            responseResponse["runmeasurements"] = runner_.RenderRunMeasurements();
            responseResponse["result"] = "ok";

            response["response"] = responseResponse;

            return response;
        }

        json& BuildSettingsResponse(const json& jsonQuery, json& response)
        {
            json settingsResponse;

            if (jsonQuery.contains("values"))
            {
                const json& valuesJson = jsonQuery["values"];
                if (valuesJson.is_array())
                {
                    auto valuesArray = valuesJson.get<std::vector<std::vector<string>>>();
                    for(auto& settingsKeyValuePair: valuesArray)
                    {
                        auto& settingsKey = settingsKeyValuePair[0];
                        auto& settingsValue = settingsKeyValuePair[1];

                        runner_.getConfigurationRepository().UpdateSetting(settingsKey, settingsValue);
                    };
                }
                else
                {
                    return BuildErrorResponse(response, "values element is not an array", "Query json must contain a 'Values' element that is an array type");
                }

                auto success = true;

                settingsResponse["status"] = BuildFullStatusElement();
                if (success)
                {
                    settingsResponse["result"] = "ok";
                }
                else
                {
                    settingsResponse["result"] = "fail";
                    settingsResponse["error"] = "Unable to modify all settings requested";
                }

                response["response"] = settingsResponse;
            }
            else
            {
                return BuildErrorResponse(response, "missing Values", "Query json must contain a 'Values' element at the root");
            }

            return response;
        }

        json& BuildControlResponse(const json& jsonQuery, json& response)
        {
            json controlResponse;

            if (jsonQuery.contains("values"))
            {
                auto& controlValues = jsonQuery["values"];
                if (controlValues.contains("logenable"))
                {
                    Log::Enable(controlValues["logenable"].get<bool>());
                }
                if (controlValues.contains("run"))
                {
                    if (controlValues["run"].get<bool>())
                    {
                        /*
                        if (controlValues.contains("configuration"))
                        {
                            string configuration = controlValues["configuration"].get<string>();
                            runner_.RunWithNewModel(configuration);
                        }
                        else
                        */
                        {
                            runner_.RunWithExistingModel();
                        }
                    }
                    else
                    {
                        runner_.Quit();
                    }
                }
                if (controlValues.contains("pause"))
                {
                    if (controlValues["pause"].get<bool>())
                        runner_.Pause();
                    else
                        runner_.Continue();
                }
                // Do more as they come up...

                // Let the context capture what it knows about.
                auto success = runner_.SetValue(controlValues);

                controlResponse["status"] = BuildFullStatusElement();
                if (success)
                {
                    controlResponse["result"] = "ok";
                }
                else
                {
                    BuildErrorResponse(controlResponse, "Control failure", "Model is not in a valid state for control changes");
                }

                response["response"] = controlResponse;
            }
            else
            {
                return BuildErrorResponse(response, "missing Values", "Query json must contain a 'Values' element at the root");
            }

            return response;
        }

        json& BuildDeployResponse(const json& jsonQuery, json& response)
        {
            string modelName {""};
            if (jsonQuery.contains("model"))
            {
                modelName = jsonQuery["model"].get<string>();
            }

            string deploymentName {""};
            if (jsonQuery.contains("deployment"))
            {
                deploymentName = jsonQuery["deployment"].get<string>();
            }

            string engineName {""};
            if (jsonQuery.contains("engine"))
            {
                engineName = jsonQuery["engine"].get<string>();
            }
            cout << "Command Control handler for Deploy(model: '" << modelName << "', deployment: '" << deploymentName << "', engine: '" << engineName << "')\n";

            // Let the runner manage the deployment.
            auto success = runner_.DeployModel(modelName, deploymentName, engineName);

            json deployResponse;
            if (success)
            {
                deployResponse["result"] = "ok";
            }
            else
            {
                BuildErrorResponse(deployResponse, "Deploy failure", ("Deploy failure for model: " + modelName + ", deployment: " + deploymentName + ", engine: " + engineName).c_str());
            }

            response["response"] = deployResponse;
            return response;
        }

        json& BuildErrorResponse(json& response, const char* error, const char* errorDetail)
        {
            json errorResponse;
            errorResponse["result"] = "fail";
            errorResponse["error"] = error;
            errorResponse["errordetail"] = errorDetail;

            response["response"] = errorResponse;

            return response;
        }

        json BuildFullStatusElement()
        {
            json statusResponse = runner_.RenderStatus();
/*
            auto filename = runner_.ControlFile();
            if (filename.length() > 5 && filename.substr(filename.length()-5, filename.length()) == ".json")
                filename = filename.substr(0, filename.length()-5);
*/
            statusResponse["controlfile"] = "defaultcontrol";

            statusResponse["logenable"] = Log::Enabled();

            return statusResponse;
        }
    };
}
