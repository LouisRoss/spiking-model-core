#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <dlfcn.h>

#include "ConfigurationRepository.h"
#include "SensorInputs/ISensorInput.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::string;
    using std::vector;
    using std::cout;

    using embeddedpenguins::core::neuron::model::ConfigurationRepository;

    class SensorInputProxy : public ISensorInput
    {
        using SensorInputCreator = ISensorInput* (*)(const ConfigurationRepository&, unsigned long long int&, LogLevel&);
        using SensorInputDeleter = void (*)(ISensorInput*);

        const string sensorInputSharedLibraryPath_ {};

        bool valid_ { false };
        void* sensorInputLibrary_ {};
        SensorInputCreator createSensorInput_ {};
        SensorInputDeleter deleteSensorInput_ {};
        ISensorInput* sensorInput_ {};
        string errorReason_ {};
        vector<unsigned long long> dummyInput_ {};

    public:
        const string& ErrorReason() const { return errorReason_; }
        const bool Valid() const { return valid_; }

    public:
        //
        // Use of a two-step creation process is mandatory:  Instantiate a proxy with the
        // path to the shared library, then call CreateProxy() with the model.
        //
        SensorInputProxy(const string& sensorInputSharedLibraryPath) :
            sensorInputSharedLibraryPath_(sensorInputSharedLibraryPath)
        {
        }

        ~SensorInputProxy() override
        {
            if (deleteSensorInput_ != nullptr)
                deleteSensorInput_(sensorInput_);

            if (sensorInputLibrary_ != nullptr)
                dlclose(sensorInputLibrary_);
        }

    public:
        // ISensorInput implementaton
        virtual void CreateProxy(ConfigurationRepository& configuration, unsigned long long int& iterations, LogLevel& loggingLevel) override
        {
            LoadISensorInput();
            if (createSensorInput_ != nullptr)
                sensorInput_ = createSensorInput_(configuration, iterations, loggingLevel);
        }

        virtual bool Connect(const string& connectionString) override
        {
            errorReason_.clear();

            if (sensorInput_ && valid_)
                return sensorInput_->Connect(connectionString);

            if (!sensorInput_)
            {
                std::ostringstream os;
                os << "Error calling Connect(" << connectionString <<"): sensor input library " 
                    << sensorInputSharedLibraryPath_ << " not loaded";
                errorReason_ = os.str();
            }

            if (!valid_)
            {
                std::ostringstream os;
                os << "Error calling Connect(" << connectionString <<"): invalid sensor input library " 
                    << sensorInputSharedLibraryPath_;
                errorReason_ = os.str();
            }

            return false;
        }

        virtual bool Disconnect() override
        {
            errorReason_.clear();

            if (sensorInput_ && valid_)
                return sensorInput_->Disconnect();

            if (!sensorInput_)
            {
                std::ostringstream os;
                os << "Error calling Disconnect(): sensor input library " 
                    << sensorInputSharedLibraryPath_ << " not loaded";
                errorReason_ = os.str();
            }

            if (!valid_)
            {
                std::ostringstream os;
                os << "Error calling Disconnect(): invalid sensor input library " 
                    << sensorInputSharedLibraryPath_;
                errorReason_ = os.str();
            }

            return false;
        }

        virtual vector<unsigned long long>& AcquireBuffer() override
        {
            errorReason_.clear();

            if (sensorInput_ && valid_)
                return sensorInput_->AcquireBuffer();

            if (!sensorInput_)
            {
                std::ostringstream os;
                os << "Error calling AcquireBuffer(): sensor input library " 
                    << sensorInputSharedLibraryPath_ << " not loaded";
                errorReason_ = os.str();
            }

            if (!valid_)
            {
                std::ostringstream os;
                os << "Error calling AcquireBuffer(): invalid sensor input library " 
                    << sensorInputSharedLibraryPath_;
                errorReason_ = os.str();
            }

            return dummyInput_;
        }

        virtual vector<unsigned long long>& StreamInput(unsigned long long int tickNow) override
        {
            errorReason_.clear();

            if (sensorInput_ && valid_)
                return sensorInput_->StreamInput(tickNow);

            if (!sensorInput_)
            {
                std::ostringstream os;
                os << "Error calling StreamInput(): sensor input library " 
                    << sensorInputSharedLibraryPath_ << " not loaded";
                errorReason_ = os.str();
            }

            if (!valid_)
            {
                std::ostringstream os;
                os << "Error calling StreamInput(): invalid sensor input library " 
                    << sensorInputSharedLibraryPath_;
                errorReason_ = os.str();
            }

            return dummyInput_;
        }

    private:
        void LoadISensorInput()
        {
            errorReason_.clear();
            cout << "Loading sensor input library from " << sensorInputSharedLibraryPath_ << "\n";

            sensorInputLibrary_ = dlopen(sensorInputSharedLibraryPath_.c_str(), RTLD_LAZY);
            if (!sensorInputLibrary_)
            {
                if (errorReason_.empty())
                {
                    std::ostringstream os;
                    os << "Cannot load library '" << sensorInputSharedLibraryPath_ << "': " << dlerror();
                    errorReason_ = os.str();
                }
                cout << errorReason_ << "\n";
                valid_ = false;
                return;
            }

            // reset errors
            dlerror();
            createSensorInput_ = (SensorInputCreator)dlsym(sensorInputLibrary_, "create");
            const char* dlsym_error = dlerror();
            if (dlsym_error)
            {
                if (errorReason_.empty())
                {
                    std::ostringstream os;
                    os << "Cannot load symbol 'create': " << dlsym_error;
                    errorReason_ = os.str();
                }
                cout << errorReason_ << "\n";
                valid_ = false;
                return;
            }

            // reset errors
            dlerror();
            deleteSensorInput_ = (SensorInputDeleter)dlsym(sensorInputLibrary_, "destroy");
            dlsym_error = dlerror();
            if (dlsym_error)
            {
                if (errorReason_.empty())
                {
                    std::ostringstream os;
                    os << "Cannot load symbol 'destroy': " << dlsym_error;
                    errorReason_ = os.str();
                }
                cout << errorReason_ << "\n";
                valid_ = false;
                return;
            }

            valid_ = true;
        }
    };
}
