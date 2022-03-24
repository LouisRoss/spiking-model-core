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
    using std::filesystem::exists;
    using std::filesystem::create_directories;

    using nlohmann::json;
    
    class ConfigurationRepository
    {
        string stackConfigurationPath_ { "/configuration/configuration.json" };
        string defaultControlFile_ { "defaultcontrol.json" };

        bool valid_ { false };
        string configurationPath_ {};
        string settingsFile_ { "./ModelSettings.json" };
        string controlFile_ {};
        string configFile_ {};
        string monitorFile_ {};
    protected:
        json stackConfiguration_ {};
        json configuration_ {};
        json control_ {};
        json monitor_ {};
        json settings_ {};
        string modelName_ {};
        string deploymentName_ {};
        string engineName_ {};
        string recordDirectory_ {};
        bool recordDirectoryRead_ { false };
        string recordCacheDirectory_ {};
        string recordFile_ {};
        string wiringFile_ {};

    public:
        const bool Valid() const { return valid_; }
        json& Control() { return control_; }
        json& StackConfiguration() { return stackConfiguration_; }
        json& Configuration() { return configuration_; }
        json& Monitor() { return monitor_; }
        json& Settings() { return settings_; }
        const string& ModelName() const { return modelName_; }
        void ModelName(const string& modelName) { modelName_ = modelName; }
        const string& DeploymentName() const { return deploymentName_; }
        void DeploymentName(const string& deploymentName) { deploymentName_ = deploymentName; }
        const string& EngineName() const { return engineName_; }
        void EngineName(const string& engineName) { engineName_ = engineName; }

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

            LoadStackConfiguration();

            if (valid_)
                LoadSettings();

            if (valid_)
                LoadControl();

            if (valid_)
                LoadConfiguration();

            if (valid_)
                LoadMonitor();

            return valid_;
        }

        void UpdateSetting(const string& settingsKey, const string& settingsValue)
        {
            if (settings_.contains(settingsKey))
            {
                cout << "Changing setting '" << settingsKey << "' to '" << settingsValue << "'\n";
                settings_[settingsKey] = settingsValue;

                // If we changed the cached paths, clear the path cache so it will need to be recalculated.
                if (settingsKey == "RecordFilePath") recordDirectoryRead_ = false;
                if (settingsKey == "RecordFileCachePath") recordCacheDirectory_.clear();
            }
        }

        //
        //  Compose the record cache file path, taking into account 
        // configured settings and their various defaults.
        //
        const string ComposeRecordCachePath()
        {
            return ComposeRecordPathForModel(ExtractRecordCacheDirectory(), ExtractRecordFile());
        }

        //
        //  Compose the record file path, taking into account the configured
        // settings and their various defaults.
        //
        const string ComposeRecordPath()
        {
            auto recordDirectory = ExtractRecordDirectory();
            if (recordDirectory.empty()) return recordDirectory;

            return ComposeRecordPathForModel(recordDirectory, ExtractRecordFile());
        }

        //
        //  Compose the wiring cache file path, taking into account 
        // configured settings and their various defaults.
        //
        const string ComposeWiringCachePath()
        {
            return ComposeRecordPathForModel(ExtractRecordCacheDirectory(), ExtractRecordWiringFile());
        }

        //
        //  Look up and cache the configured record cache directory.
        // Default if unconfigured or empty is '../record/'.
        // The directory is cached so that subsequent calls will not need to look it up.
        //
        const string ExtractRecordCacheDirectory()
        {
            string recordCacheDirectory = recordCacheDirectory_;

            // Read and cache the directory only on the first call.
            // An empty string is invalid, indicating the directory needs to be read.
            if (recordCacheDirectory.empty())
            {
                recordCacheDirectory = "../record/";
                if (settings_.contains("RecordFileCachePath"))
                {
                    string cachDirectory {};
                    auto recordFileCachePathJson = settings_["RecordFileCachePath"];
                    if (recordFileCachePathJson.is_string())
                        cachDirectory = recordFileCachePathJson.get<string>();
                    if (!cachDirectory.empty())
                    {
                        if (cachDirectory[cachDirectory.length() - 1] != '/')
                            cachDirectory += '/';

                        recordCacheDirectory = cachDirectory;
                    }
               }

               recordCacheDirectory_ = recordCacheDirectory;
                cout << "Record cache directory: " << recordCacheDirectory_ << '\n';
            }

            return recordCacheDirectory;
        }

        //
        //  Look up and cache the configured record directory.
        // Default if unconfigured or empty is an empty string, ''.
        // The directory is cached so that subsequent calls will not need to look it up.
        //
        const string ExtractRecordDirectory()
        {
            // Read and cache the directory only on the first call.
            if (!recordDirectoryRead_)
            {
                if (settings_.contains("RecordFilePath"))
                {
                    auto recordFilePathJson = settings_["RecordFilePath"];
                    if (recordFilePathJson.is_string())
                    {
                        auto recordFilePath = recordFilePathJson.get<string>();

                        if (!recordFilePath.empty() && recordFilePath[recordFilePath.length() - 1] != '/')
                            recordFilePath += '/';

                        recordDirectory_ = recordFilePath;
                        cout << "Record directory: " << recordDirectory_ << '\n';
                    }
                }

                recordDirectoryRead_ = true;
            }

            return recordDirectory_;
        }

        //
        //  Given a bsse directory for a record path, compose the standard subdirectory,
        // based on the current model name and other relevant parameters.
        //
        const string ComposeRecordPathForModel(const string baseDirectory, const string fileName)
        {
            string recordPath = baseDirectory;
            recordPath += modelName_ + "/" + deploymentName_ + "/" + engineName_ + "/";

            create_directories(recordPath);

            return recordPath + fileName;
        }

        //
        //  Look up and cache the configured record file name.
        // Default if unconfigured or empty is 'ModelEngineRecord.csv'.
        // The file name is cached so that subsequent calls will not need to look it up.
        //
        const string ExtractRecordFile()
        {
            string fileName = recordFile_;
            if (fileName.empty())
            {
                fileName = "ModelEngineRecord.csv";
                if (configuration_.contains("PostProcessing"))
                {
                    auto& postProcessingJson = configuration_["PostProcessing"];
                    if (postProcessingJson.contains("RecordFile"))
                    {
                        auto& recordFileJson = postProcessingJson["RecordFile"];
                        if (recordFileJson.is_string())
                            fileName = recordFileJson.get<string>();

                        if (fileName.length() < 4 || fileName.substr(fileName.length()-4, fileName.length()) != ".csv")
                            fileName += ".csv";
                    }
                }

                recordFile_ = fileName;
            }

            return fileName;
        }

        //
        //  Look up and cache the configured wiring file name.
        // Default if unconfigured or empty is 'wiring.csv'.
        // The file name is cached so that subsequent calls will not need to look it up.
        //
        const string ExtractRecordWiringFile()
        {
            string fileName = wiringFile_;
            if (fileName.empty())
            {
                fileName = "wiring.csv";
                if (control_.contains("Wiring"))
                {
                    auto& wiringJson = control_["Wiring"];
                    if (wiringJson.is_string())
                        fileName = wiringJson.get<string>();

                    if (fileName.length() < 4 || fileName.substr(fileName.length()-4, fileName.length()) != ".csv")
                        fileName += ".csv";
                }

                wiringFile_ = fileName;
            }

            return fileName;
        }

    private:
        //
        // All services in the stack can access the same configuration at the root of the filesystem: /configuration/configuration.json.
        // Load that file into a json object.
        //
        void LoadStackConfiguration()
        {
            if (settingsFile_.length() < 5 || settingsFile_.substr(settingsFile_.length()-5, settingsFile_.length()) != ".json")
                settingsFile_ += ".json";

            if (!exists(stackConfigurationPath_))
            {
                cout << "Settings file " << stackConfigurationPath_ << " does not exist\n";
                valid_ = false;
                return;
            }

            cout << "LoadStackConfiguration from " << stackConfigurationPath_ << "\n";
            try
            {
                ifstream stackConfig(stackConfigurationPath_);
                stackConfig >> stackConfiguration_;
            }
            catch(const json::parse_error& e)
            {
                std::cerr << "Unable to read stack configuration file " << stackConfigurationPath_ << ": " << e.what() << '\n';
                valid_ = false;
            }
        }

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
            if (controlFile_.empty())
            {
                if (settings_.contains("DefaultControlFile"))
                {
                    controlFile_ = settings_["DefaultControlFile"].get<string>();
                }
                else
                {
                    controlFile_ = defaultControlFile_;
                }
            }

            if (controlFile_.length() < 5 || controlFile_.substr(controlFile_.length()-5, controlFile_.length()) != ".json")
                controlFile_ += ".json";

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
        // If any problem occurs, the valid_ field will be false.
        //
        void LoadConfiguration()
        {
            // TODO - If we can find the configuration file through configurationPath_/modelName_/deploymentName_/engineName_/
            //        use that, otherwise use the path configured in the control file.
            /*
            if (!modelName_.empty() && !deploymentName_.empty() && !engineName_.empty())
            {
                string configDirectory = configurationPath_ + "/" + modelName_ + "/" + deploymentName_ + "/" + engineName_ + "/";

            }
            */

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
