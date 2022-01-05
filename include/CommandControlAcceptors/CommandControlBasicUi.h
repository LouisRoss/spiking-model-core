#pragma once

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "IModelRunner.h"
#include "ICommandControlAcceptor.h"
#include "IModelHelper.h"
#include "KeyListener.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;

    using nlohmann::json;

    class CommandControlBasicUi : public ICommandControlAcceptor
    {
        IModelRunner& modelRunner_;

    public:
        CommandControlBasicUi(IModelRunner& modelRunner) :
            modelRunner_(modelRunner)
        {
        }

        // ICommandControlAcceptor implementation.
    public:
        virtual const string& Description() override
        {
            static string description { "Basic UI" };
            return description;
        }

        virtual bool ParseArguments(int argc, char* argv[]) override
        {
            cout << "Basic UI C/C initializing\n";

            for (auto i = 0; i < argc; i++)
            {
                string arg = argv[i];
                if (arg[0] == '-')
                {
                    if (arg[1] == 'm')
                    {
                        string modelName = arg.substr(2);
                        modelRunner_.getConfigurationRepository().ModelName(modelName);
                        cout << "Found " << arg << " flag, setting model name to " << modelName << "\n";
                    }
                    if (arg[1] == 'd')
                    {
                        string deployment = arg.substr(2);
                        modelRunner_.getConfigurationRepository().DeploymentName(deployment);
                        cout << "Found " << arg << " flag, setting deployment name to " << deployment << "\n";
                    }
                    if (arg[1] == 'e')
                    {
                        string engineName = arg.substr(2);
                        modelRunner_.getConfigurationRepository().EngineName(engineName);
                        cout << "Found " << arg << " flag, setting engine name to " << engineName << "\n";
                    }
                }
            }

            return true;
        }

        virtual bool Initialize() override
        {
            return true;
        }

        virtual bool AcceptAndExecute(unique_ptr<IQueryHandler> const & queryHandler) override
        {
            KeyListener listener;

            bool quit {false};

            char c {' '};
            auto gotChar = listener.Listen(50'000, c);
            if (gotChar)
            {
                switch (c)
                {
                    case 'p':
                    case 'P':
                    {
                        json pauseQuery = { {"query","control"}, {"values", {{"pause", true}}} };
                        queryHandler->HandleQuery(pauseQuery.dump());
                        break;
                    }

                    case 'r':
                    case 'R':
                    {
                        json resumeQuery = { {"query","control"}, {"values", {{"pause", false}}} };
                        queryHandler->HandleQuery(resumeQuery.dump());
                        break;
                    }

                    case 'd':
                    case 'D':
                    {
                        json deployQuery = { 
                            {"query","deploy"}, 
                            {"model", modelRunner_.getConfigurationRepository().ModelName()}, 
                            {"deployment", modelRunner_.getConfigurationRepository().DeploymentName()}, 
                            {"engine", modelRunner_.getConfigurationRepository().EngineName()}
                        };

                        const string& response = queryHandler->HandleQuery(deployQuery.dump());
                        cout << response << "\n";
                        break;
                    }

                    case 'q':
                    case 'Q':
                        quit = true;
                        break;

                    default:
                        break;
                }
            }

            return quit;
        }
    };
}
