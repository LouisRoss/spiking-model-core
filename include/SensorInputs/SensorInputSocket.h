#pragma once

#include <iostream>
#include <fstream>
#include <tuple>
#include <map>
#include <memory>
#include <mutex>

#include "nlohmann/json.hpp"

#include "WorkerThread.h"
#include "Log.h"
#include "ConfigurationRepository.h"
#include "SensorInputs/ISensorInput.h"
#include "SensorInputs/SensorInputListenSocket.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::ifstream;
    using std::tuple;
    using std::multimap;
    using std::make_pair;
    using std::unique_ptr;
    using std::make_unique;
    using std::mutex;

    using nlohmann::json;

    class SensorInputSocket : public ISensorInput
    {
        ConfigurationRepository& configuration_;
        unsigned long long int& iterations_;
        LogLevel& loggingLevel_;

        mutex mutex_ {};
        multimap<int, unsigned long long int> signalToInject_ {};

        vector<unsigned long long> signalToReturn_ {};

        unique_ptr<SensorInputListenSocket> sensorInput_ {};
        unique_ptr<WorkerThread<SensorInputListenSocket>> sensorInputWorkerThread_ {};

    public:
        SensorInputSocket(ConfigurationRepository& configuration, unsigned long long int& iterations, LogLevel& loggingLevel) :
            configuration_(configuration),
            iterations_(iterations),
            loggingLevel_(loggingLevel)
        {
        }

        // ISensorInput implementaton
        virtual void CreateProxy(ConfigurationRepository& configuration, unsigned long long int& iterations, LogLevel& loggingLevel) override { }

        //
        // Interpret the connection string as a hostname and port number
        // to listen on.  The two fields are separated by a ':' character.
        //
        virtual bool Connect(const string& connectionString) override
        {
            auto [host, port] = ParseConnectionString(connectionString);

            sensorInput_ = std::move(make_unique<SensorInputListenSocket>(host, port, configuration_, iterations_, loggingLevel_,
                [this](const multimap<int, unsigned long long int>& signalToInject) {
                    unique_lock<mutex> lock(mutex_);
                    signalToInject_.insert(signalToInject.begin(), signalToInject.end());
                }));
            sensorInput_->Initialize();

            sensorInputWorkerThread_ = std::move(make_unique<WorkerThread<SensorInputListenSocket>>(*sensorInput_.get()));
            sensorInputWorkerThread_->StartContinuous();

            return true;
        }

        virtual bool Disconnect() override
        {
            sensorInputWorkerThread_->StopContinuous();
            return true;
        }

        virtual vector<unsigned long long>& AcquireBuffer() override
        {
            return signalToReturn_;
        }

        virtual vector<unsigned long long>& StreamInput(unsigned long long int tickNow) override
        {
            signalToReturn_.clear();

            auto done = signalToInject_.empty();

            unique_lock<mutex> lock(mutex_);
            while (!done)
            {
                auto nextSignal = signalToInject_.begin();
                //cout << "Sensor socket considering signal at tick " << nextSignal->first << "\n";
                if (nextSignal->first < 0 || nextSignal->first <= tickNow)
                {
                    //cout << "Sensor socket adding offset " << nextSignal->second << " at tick " << nextSignal->first << " to signal to return\n";
                    signalToReturn_.push_back(nextSignal->second);
                    signalToInject_.extract(nextSignal);
                    done = signalToInject_.empty();
                }
                else
                {
                    done = true;
                    if (loggingLevel_ == LogLevel::Diagnostic && !signalToReturn_.empty())
                    {
                        cout << "Sensor socket injecting offsets ";
                        for (auto& offset : signalToReturn_)
                        {
                            cout << offset << " ";
                        }
                        cout << " at tick " << tickNow << "\n";
                    }
                }
            };

            return signalToReturn_;
        }

    private:
        tuple<string, string> ParseConnectionString(const string& connectionString)
        {
            string host {"0.0.0.0"};
            string port {"8001"};

            auto colonPos = connectionString.find(":");
            if (colonPos != string::npos)
            {
                auto tempHost = connectionString.substr(0, colonPos);
                if (!tempHost.empty()) host = tempHost;
                auto tempPort = connectionString.substr(colonPos + 1);
                if (!tempPort.empty()) port = tempPort;
            }

            return {host, port};
        }
    };
}
