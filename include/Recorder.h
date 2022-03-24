#pragma once

#include <iomanip>
#include <chrono>
#include <string>
#include <fstream>
#include <filesystem>
#include <map>
#include <tuple>

#include "ConfigurationRepository.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::multimap;
    using std::pair;
    using std::tuple;
    using std::make_tuple;
    using std::string;
    using std::begin;
    using std::end;
    using time_point = std::chrono::high_resolution_clock::time_point;
    using Clock = std::chrono::high_resolution_clock;
    using std::ofstream;

    //
    // Allow recording of significant state changes with minimal impact 
    // to real-time execution.  Collect all record messages for a single thread, 
    // then merge and log them to a file only after execution is over.
    // A possible future enhancement will pause occasionally to
    // merge/log the record buffers and flush memory.
    //
    template<class RECORDTYPE>
    class Recorder
    {
        unsigned long long int& ticks_;
        multimap<unsigned long long int, tuple<time_point, RECORDTYPE>> records_;

        static string& RecordFile()
        {
            static string recordFile;
            return recordFile;
        }

        static ConfigurationRepository*& Configuration()
        {
            static ConfigurationRepository* pConfiguration {};
            return pConfiguration;
        }

    public:
        Recorder(unsigned long long int& ticks, ConfigurationRepository& configuration) :
            ticks_(ticks)
        {
            Configuration() = &configuration;
            RecordFile() = configuration.ComposeRecordCachePath();

            cout << "Writing record header, overwriting previous record file at " << RecordFile() << "\n";
            ofstream recordfile;
            recordfile.open(RecordFile(), std::ofstream::out | std::ofstream::trunc);

            recordfile << "tick,time," << RECORDTYPE::Header() << "\n";
        }

        void Record(const RECORDTYPE& record)
        {
            records_.insert(pair<unsigned long long int, tuple<time_point, RECORDTYPE>>(ticks_, make_tuple(Clock::now(), record)));
        }

        static multimap<unsigned long long int, tuple<time_point, RECORDTYPE>>& MergedRecords()
        {
            static multimap<unsigned long long int, tuple<time_point, RECORDTYPE>> mr;
            return mr;
        }

        static void Merge(Recorder& other)
        {
            MergedRecords().insert(begin(other.records_), end(other.records_));
            other.records_.clear();
        }

        static void Print()
        {
            ofstream recordfile;
            recordfile.open(RecordFile(), std::ofstream::out | std::ofstream::app);

            for (auto& record : MergedRecords())
            {
                auto rawtimestamp = std::get<0>(record.second);
                std::time_t timestamp = std::chrono::system_clock::to_time_t(rawtimestamp);
                string s(30, '\0');
                std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
                string formattedTime(s.c_str());

                auto lastSecond = std::chrono::floor<std::chrono::seconds>(std::get<0>(record.second));
                auto fractionOfSecond = rawtimestamp - lastSecond;

                recordfile 
                    << record.first
                    << ","
                    << formattedTime
                    << "." 
                    << std::setfill('0') << std::setw(9) << fractionOfSecond.count() 
                    << "," 
                    << std::get<1>(record.second).Format()
                    << "\n";
            }

            MergedRecords().clear();
        }

        static void Finalize()
        {
            if (Configuration()->ExtractRecordDirectory().empty()) return;

            const auto copyOptions =  std::filesystem::copy_options::overwrite_existing
                                    | std::filesystem::copy_options::recursive;

            cout << "Copying records from " << Configuration()->ExtractRecordCacheDirectory() << " to " << Configuration()->ExtractRecordDirectory() << "\n";

            std::filesystem::create_directories(Configuration()->ExtractRecordDirectory());

            std::filesystem::copy(Configuration()->ExtractRecordCacheDirectory(), Configuration()->ExtractRecordDirectory(), copyOptions); 
            std::filesystem::remove_all(Configuration()->ExtractRecordCacheDirectory());
        }
    };
}
