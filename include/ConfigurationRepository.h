#pragma once

#include "sys/stat.h"

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#include "nlohmann/json.hpp"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::ifstream;
    using std::filesystem::create_directories;

    using nlohmann::json;
    
    class ConfigurationRepository
    {
        bool valid_ { false };
        string configurationPath_ {};
        string settingsFile_ { "./ModelSettings.json" };
        string controlFile_ {};
        string configFile_ {};
        string monitorFile_ {};
    protected:
        json configuration_ {};
        json control_ {};
        json monitor_ {};
        json settings_ {};

    public:
        const bool Valid() const { return valid_; }
        const json& Control() const { return control_; }
        const json& Configuration() const { return configuration_; }
        const json& Monitor() const { return monitor_; }
        const json& Settings() const { return settings_; }

    public:
        ConfigurationRepository() = default;

        //
        // Load or reload a new configuration, overwriting any existing
        // configuration.  The configuration is directed completely by
        // the contents of the control file, whose name is passed in the controlFile
        // parameter (with an assumed extension of .json), and will be found
        // in the configuration file path, specified by:
        // * The settings file at the settingsFile_ field, typicially ./ModelSettings.json
        // * contains a json property called 'ConfigFilePath'.
        //
        // NOTE: The controlFile parameter should be only the file name
        //       (or may include the relative path) and will be resolved 
        //       in the configuration path as specified in the control file.
        //       It may containe the '.json' extension, but the extension may
        //       optionally be omitted.
        //
        bool InitializeConfiguration(const string& controlFile)
        {
            controlFile_ = controlFile;
            valid_ = true;

            LoadSettings();

            if (valid_)
                LoadControl();

            if (valid_)
                LoadConfiguration();

            if (valid_)
                LoadMonitor();

            return valid_;
        }

        const string ComposeRecordPath() const
        {
            auto recordDirectory = ExtractRecordDirectory();

            string fileName {"ModelEngineRecord.csv"};
            auto file(configuration_["PostProcessing"]["RecordFile"]);
            if (file.is_string())
                fileName = file.get<string>();

            return recordDirectory + fileName;
        }

        const string ExtractRecordDirectory() const
        {
            string recordDirectory {"./"};
            auto path = configuration_["PostProcessing"]["RecordLocation"];
            if (path.is_string())
                recordDirectory = path.get<string>();

            if (recordDirectory[recordDirectory.length() - 1] != '/')
                recordDirectory += '/';

            auto project = control_["Configuration"];
            if (project.is_string())
            {
                auto configuration= project.get<string>();
                auto lastSlashPos = configuration.rfind('/');
                if (lastSlashPos != configuration.npos)
                    configuration = configuration.substr(lastSlashPos + 1, configuration.size() - lastSlashPos);

                auto jsonExtensionPos = configuration.rfind(".json");
                if (jsonExtensionPos != configuration.npos)
                    configuration = configuration.substr(0, jsonExtensionPos);

                recordDirectory += configuration;
            }

            if (recordDirectory[recordDirectory.length() - 1] != '/')
                recordDirectory += '/';

            cout << "Record directory: " << recordDirectory << '\n';

            create_directories(recordDirectory);
            return recordDirectory;
        }

    private:
        //
        // Load the settings from the JSON file speicified by the settingsFile_
        // field into the settings_ field.  As a side effect, also load the 'ConfigFilePath'
        // json property into the configurationPath_ field, if it exists.
        // If any problem occurs, the valud_ field will be false.
        //
        void LoadSettings()
        {
            if (settingsFile_.length() < 5 || settingsFile_.substr(settingsFile_.length()-5, settingsFile_.length()) != ".json")
                settingsFile_ += ".json";

            struct stat buffer;   
            if (!(stat (settingsFile_.c_str(), &buffer) == 0))
            {
                cout << "Settings file " << settingsFile_ << " does not exist, using defaults\n";
                valid_ = false;
                return;
            }

            cout << "LoadSettings from " << settingsFile_ << "\n";
            try
            {
                ifstream settings(settingsFile_);
                settings >> settings_;

                configurationPath_ = "./";
                if (settings_.contains("ConfigFilePath"))
                {
                    auto configFilePathJson = settings_["ConfigFilePath"];
                    if (configFilePathJson.is_string())
                        configurationPath_ = configFilePathJson.get<string>();
                }
            }
            catch(const json::parse_error& e)
            {
                std::cerr << "Unable to read settings file " << settingsFile_ << ": " << e.what() << '\n';
                valid_ = false;
            }
        }

        //
        // Load the JSON control file as specified by the controlFile_ field
        // set in the constructor.  The file is resolved in the path specified
        // the by the configurationPath_ field read from the settings file.
        // If any problem occurs, the valud_ field will be false.
        //
        void LoadControl()
        {
            controlFile_ = configurationPath_ + "/" + controlFile_;
            cout << "Using control file " << controlFile_ << "\n";

            struct stat buffer;   
            if (!(stat (controlFile_.c_str(), &buffer) == 0))
            {
                cout << "Control file " << controlFile_ << " does not exist\n";
                valid_ = false;
                return;
            }

            cout << "LoadControl from " << controlFile_ << "\n";
            try
            {
                ifstream control(controlFile_);
                control >> control_;
            }
            catch(const json::parse_error& e)
            {
                std::cerr << "Unable to read control file " << controlFile_ << ": " << e.what() << '\n';
                valid_ = false;
            }
        }

        //
        // Load the JSON configuration file as specified by the 'Configuration' property
        // of the control file, as previously read into the control_ field.
        // The file is resolved in the path specified the by the configurationPath_ field
        // read from the settings file.
        // If any problem occurs, the valud_ field will be false.
        //
        void LoadConfiguration()
        {
            if (!control_.contains("Configuration"))
            {
                cout << "Control file " << controlFile_ << " does not contain a 'Configuration' property\n";
                valid_ = false;
                return;
            }

            configFile_ = control_["Configuration"].get<string>();
            if (configFile_.length() < 5 || configFile_.substr(configFile_.length()-5, configFile_.length()) != ".json")
                configFile_ += ".json";

            configFile_ = configurationPath_ + "/" + configFile_;
            cout << "Using config file " << configFile_ << "\n";

            struct stat buffer;   
            if (!(stat (configFile_.c_str(), &buffer) == 0))
            {
                cout << "Configuration file " << configFile_ << " does not exist\n";
                valid_ = false;
                return;
            }

            cout << "LoadConfiguration from " << configFile_ << "\n";
            try
            {
                ifstream config(configFile_);
                config >> configuration_;
            }
            catch(const json::parse_error& e)
            {
                std::cerr << "Unable to read configuration file " << configFile_ << ": " << e.what() << '\n';
                valid_ = false;
            }
        }

        //
        // Load the JSON monitor file as specified by the 'Monitor' property
        // of the control file, as previously read into the control_ field.
        // The file is resolved in the path specified the by the configurationPath_ field
        // read from the settings file.
        // If any problem occurs, the valud_ field will be false.
        //
        void LoadMonitor()
        {
            if (!control_.contains("Monitor"))
            {
                cout << "Control file " << controlFile_ << " does not contain a 'Monitor' property\n";
                valid_ = false;
                return;
            }

            monitorFile_ = control_["Monitor"].get<string>();
            if (monitorFile_.length() < 5 || monitorFile_.substr(monitorFile_.length()-5, monitorFile_.length()) != ".json")
                monitorFile_ += ".json";

            monitorFile_ = configurationPath_ + "/" + monitorFile_;
            cout << "Using monitor file " << monitorFile_ << "\n";

            struct stat buffer;   
            if (!(stat (monitorFile_.c_str(), &buffer) == 0))
            {
                // Non-fatal error, leave valid_ as-is.
                cout << "Monitor file " << monitorFile_ << " does not exist\n";
                return;
            }

            cout << "LoadMonitor from " << monitorFile_ << "\n";
            try
            {
                ifstream monitor(monitorFile_);
                monitor >> monitor_;
            }
            catch(const json::parse_error& e)
            {
                std::cerr << "Unable to read monitor file " << monitorFile_ << ": " << e.what() << '\n';
                valid_ = false;
            }
        }
    };
}
