#pragma once

#include <iostream>
#include <string>
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
            else if (query == "configurations")
                response_ = BuildConfigurationsResponse(response).dump();
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

        json& BuildConfigurationsResponse(json& response)
        {
            cout << "Received configurations query\n";
            json configuationsResponse;
            json configurationOptions;

            const auto& configurationSettings = runner_.Settings();

            auto ok { false };
            string errorDetail;
            vector<string> configurations;
            if (configurationSettings.contains("ConfigFilePath"))
            {
                const json& configFilePathJson = configurationSettings["ConfigFilePath"];
                if (configFilePathJson.is_string())
                {
                    string configFilePath = configFilePathJson.get<string>();
                    cout << "Configuration file path: " << configFilePathJson << "\n";
                    if (exists(configFilePath))
                    {
                        for (auto& file : directory_iterator(configFilePath))
                        {
                            if (file.is_regular_file())
                            {
                                string filename = file.path().filename();
                                cout << "Found configuration file " << filename << "\n";
                                if (filename.length() > 5 && filename.substr(filename.length()-5, filename.length()) == ".json")
                                    configurations.push_back(filename.substr(0, filename.length()-5));
                            }
                        }

                        cout << "Returning:\n";
                        for (auto& entry : configurations)
                            cout << entry << "\n";

                        ok = true;
                    }
                    else
                    {
                        errorDetail = "Configured path " + configFilePath + " does not exist";
                    }
                }
                else
                {
                        errorDetail = "Configured path is not a valid string";
                }
            }
            else
            {
                errorDetail = "No configured path";
            }

            configurationOptions["configurations"] = configurations;
            configuationsResponse["options"] = configurationOptions;

            configuationsResponse["result"] = ok ? "ok" : "fail";
            if (!ok)
            {
                configuationsResponse["error"] = "No Configuration files";
                configuationsResponse["errordetail"] = errorDetail;
            }

            response["response"] = configuationsResponse;
            cout << "Returning json = " << response.dump() << "\n";
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
                controlResponse["result"] = success ? "ok" : "fail";

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
            deployResponse["result"] = success ? "ok" : "fail";

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
