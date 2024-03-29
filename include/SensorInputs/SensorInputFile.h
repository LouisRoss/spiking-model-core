#pragma once

#include <iostream>
#include <fstream>
#include <map>

#include "nlohmann/json.hpp"

#include "ConfigurationRepository.h"
#include "SensorInputs/ISensorInput.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::ifstream;
    using std::multimap;
    using std::make_pair;

    using nlohmann::json;

    class SensorInputFile : public ISensorInput
    {
        ConfigurationRepository& configuration_;
        unsigned long long int& iterations_;
        LogLevel& loggingLevel_;
        nlohmann::ordered_json inputStream_ {};
        multimap<int, unsigned long long int> signalToInject_ {};
        vector<unsigned long long> signalToReturn_ {};

    public:
        SensorInputFile(ConfigurationRepository& configuration, unsigned long long int& iterations, LogLevel& loggingLevel) :
            configuration_(configuration),
            iterations_(iterations),
            loggingLevel_(loggingLevel)
        {
        }

        // ISensorInput implementaton
        virtual void CreateProxy(ConfigurationRepository& configuration, unsigned long long int& iterations, LogLevel& loggingLevel) override { }

        virtual bool Connect(const string& connectionString) override
        {
            auto sensorFile = connectionString;
            if (configuration_.Control().contains("SensorInputFile"))
            {
                const auto& sensorInputFile = configuration_.Control()["SensorInputFile"];
                if (sensorInputFile.is_string())
                    sensorFile = sensorInputFile.get<string>();
            }

            if (sensorFile.empty())
            {
                cout << "Connect found no sensor file named or configured, cannot connect\n";
                return false;
            }

            sensorFile = configuration_.ExtractRecordDirectory() + sensorFile;
            cout << "Connect using sensor file " << sensorFile << "\n";

            ifstream sensorStream(sensorFile);
            if (!sensorStream)
            {
                cout << "SensorInputFile cannot open file '" << sensorFile << "', have you generated it with a Python utility?\n";
                return false;
            }

            sensorStream >> inputStream_;
            cout << inputStream_ << "\n";
            return true;
        }

        virtual bool Disconnect() override
        {
            return true;
        }

        virtual vector<unsigned long long>& AcquireBuffer() override
        {
            return signalToReturn_;
        }

        virtual vector<unsigned long long>& StreamInput(unsigned long long int tickNow) override
        {
            signalToReturn_.clear();

            auto done = inputStream_.empty();
            while (!done)
            {
                auto nextSignal = inputStream_.begin();
                auto nextSignalTick = std::stoi(nextSignal.key());
                if (nextSignalTick < 0 || nextSignalTick <= tickNow)
                {
                    auto& indexList = nextSignal.value();
                    if (indexList.is_array())
                    {
                        const auto& indexes = indexList.get<vector<int>>();
                        signalToReturn_.insert(signalToReturn_.end(), indexes.begin(), indexes.end());
                    }

                    inputStream_.erase(nextSignal);
                    done = inputStream_.empty();
                }
                else
                {
                    done = true;
                }
            };

/*
            auto done = signalToInject_.empty();
            while (!done)
            {
                auto nextSignal = signalToInject_.begin();
                if (nextSignal->first < 0 || nextSignal->first <= tickNow)
                {
                    signalToReturn_.push_back(nextSignal->second);
                    signalToInject_.extract(nextSignal);
                    done = signalToInject_.empty();
                }
                else
                {
                    done = true;
                }
            };
*/
            return signalToReturn_;
        }

    private:
    };
}
