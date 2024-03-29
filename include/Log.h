#pragma once

#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <tuple>

namespace embeddedpenguins::core::neuron::model
{
    using std::chrono::system_clock;
    using std::chrono::high_resolution_clock;
    using std::chrono::nanoseconds;
    using std::map;
    using std::pair;
    using std::tuple;
    using std::make_tuple;
    using std::string;
    using std::ostringstream;
    using std::ofstream;

    enum class LogLevel
    {
        None,
        Status,
        Diagnostic
    };

    //
    // A logger with minimal impact to real-time execution.
    // Collect all log messages for a single thread, then merge
    // and print them to a file only after execution is over.
    // A possible future enhancement will pause occasionally to
    // merge/print the log buffers and flush memory.
    //
    class Log
    {
        int id_;
        map<high_resolution_clock::time_point, tuple<int, string>> messages_;
        ostringstream stream_ {};
        ostringstream nullstream_ {};

        static bool Enable(bool enable, bool readback)
        {
            static bool enabled {false};

            if (!readback) enabled = enable;
            return enabled;
        }

    public:
        static const bool Enabled() { return Enable(true, true); }
        static void Enable(const bool enable) { Enable(enable, false); }

    public:
        Log() : id_(0)
        {

        }

        void SetId(int id)
        {
            id_ = id;
        }

        ostringstream& Logger()
        {
            if (Enabled())
                return stream_;

            return nullstream_;
        }

        void Logit(ostringstream& str)
        {
            if (Enabled())
                messages_.insert(pair<high_resolution_clock::time_point, tuple<int, string>>(high_resolution_clock::now(), make_tuple(id_, str.str())));

            nullstream_.clear();
        }

        void Logit()
        {
            if (Enabled())
            {
                messages_.insert(pair<high_resolution_clock::time_point, tuple<int, string>>(high_resolution_clock::now(), make_tuple(id_, stream_.str())));
                stream_.clear();
                stream_.str("");
            }

            nullstream_.clear();
        }

        static map<high_resolution_clock::time_point, tuple<int, string>>& MergedMessages() { static map<high_resolution_clock::time_point, tuple<int, string>> mm; return mm; }
        static void Merge(Log& other)
        {
            MergedMessages().insert(other.messages_.begin(), other.messages_.end());
        }

        static void Print(const char* file)
        {
            ofstream logfile;
            logfile.open(file);
            for (auto& message : MergedMessages())
            {
                std::time_t timestamp = std::chrono::system_clock::to_time_t(message.first);
                string s(30, '\0');
                std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
                string formattedTime(s.c_str());

                auto lastSecond = std::chrono::floor<std::chrono::seconds>(message.first);
                auto fractionOfSecond = message.first - lastSecond;

                logfile 
                    << "[" 
                    << std::get<0>(message.second) 
                    << "] " 
                    << formattedTime
                    << "." 
                    << std::setfill('0') << std::setw(9) << fractionOfSecond.count() 
                    << ": " 
                    << std::get<1>(message.second);
            }
        }

        static string FormatTime(high_resolution_clock::time_point time)
        {
            std::time_t timestamp = std::chrono::system_clock::to_time_t(time);
            string s(30, '\0');
            std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
            string formattedTime(s.c_str());

            auto lastSecond = std::chrono::floor<std::chrono::seconds>(time);
            auto fractionOfSecond = time - lastSecond;

            ostringstream str;
            str << formattedTime << '.' << std::setfill('0') << std::setw(9) << fractionOfSecond.count();

            return str.str();
        }
    };
}
