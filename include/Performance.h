#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <chrono>

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::vector;
    using std::ifstream;
    using std::getline;
    using std::istringstream;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    using std::chrono::high_resolution_clock;

    constexpr int numberOfCounters {10};
    constexpr int user_jiffies       {0};   // time spent in user mode.
    constexpr int nice_jiffies       {1};   // time spent in uer mode with low priority.
    constexpr int system_jiffies     {2};   // time spent in system mode.
    constexpr int idle_jiffies       {3};   // time spent in the idle task.
    constexpr int iowait_jiffies     {4};   // time waiting for I/O to complete.
    constexpr int irq_jiffies        {5};   // time servicing hardware interrupts.
    constexpr int softirq_jiffies    {6};   // time servicing software interrupts.
    constexpr int steal_jiffies      {7};   // time spent in other operating systems when running in a virtualized environment.
    constexpr int guest_jiffies      {8};   // time spent running a virtual CPU for guest operating systems.
    constexpr int guest_nice_jiffies {9};   // time spent running a low priority virtual CPU for guest operating systems.

    constexpr duration minimumPerformanceInterval {milliseconds(1'000)};

    class Performance
    {
        struct CpuData
        {
            string cpu;
            std::size_t times[numberOfCounters];
        };

        vector<CpuData> lastCounters_;
        high_resolution_clock::time_point lastCountersTime_;
        vector<float> lastCpuAverage_{};
        CpuData lastTotalCounters_;
        high_resolution_clock::time_point lastTotalCountersTime_;
        float lastTotalCpuAverage_{};

    public:
        Performance()
        {
            ReadCpuCounters(lastCounters_);
            lastCountersTime_ = high_resolution_clock::now() - minimumPerformanceInterval;
            ReadCpuTotalCounters(lastTotalCounters_);
            lastTotalCountersTime_ = high_resolution_clock::now() - minimumPerformanceInterval;
        }

        vector<float> GetActiveCpu()
        {
            if (high_resolution_clock::now() - lastCountersTime_ >= minimumPerformanceInterval)
            {
                vector<float> averageCpus;

                vector<CpuData> nextCounters;
                ReadCpuCounters(nextCounters);
                if (nextCounters.size() == lastCounters_.size())
                {
                    for (auto i = 0; i < nextCounters.size(); i++)
                    {
                        auto lastIdleTime = GetIdleTime(lastCounters_[i]);
                        auto nextIdleTime = GetIdleTime(nextCounters[i]);
                        auto deltaIdleTime = nextIdleTime - lastIdleTime;

                        auto lastActiveTime = GetActiveTime(lastCounters_[i]);
                        auto nextActiveTime = GetActiveTime(nextCounters[i]);
                        auto deltaActiveTime = nextActiveTime - lastActiveTime;

                        averageCpus.push_back((float)deltaActiveTime / (float)(deltaActiveTime + deltaIdleTime));
                    }

                    lastCounters_ = nextCounters;
                    lastCpuAverage_ = averageCpus;
                    lastCountersTime_ = high_resolution_clock::now();
                }
            }

            return lastCpuAverage_;
        }

        float GetActiveTotalCpu()
        {
            if (high_resolution_clock::now() - lastTotalCountersTime_ >= minimumPerformanceInterval)
            {
                CpuData nextTotalCounters;
                if (ReadCpuTotalCounters(nextTotalCounters))
                {
                    auto lastIdleTime = GetIdleTime(lastTotalCounters_);
                    auto nextIdleTime = GetIdleTime(nextTotalCounters);
                    auto deltaIdleTime = nextIdleTime - lastIdleTime;

                    auto lastActiveTime = GetActiveTime(lastTotalCounters_);
                    auto nextActiveTime = GetActiveTime(nextTotalCounters);
                    auto deltaActiveTime = nextActiveTime - lastActiveTime;

                    lastTotalCounters_ = nextTotalCounters;
                    lastTotalCountersTime_ = high_resolution_clock::now();

                    lastTotalCpuAverage_ = (float)deltaActiveTime / (float)(deltaActiveTime + deltaIdleTime);
                }
            }

            return lastTotalCpuAverage_;
        }

    private:
        void ReadCpuCounters(vector<CpuData>& entries)
        {
            const string CpuName{"cpu"};
            const std::size_t CpuNameLength = CpuName.size();
            const string Total{"tot"};

            ifstream fileStat("/proc/stat");
            string line;
            while(getline(fileStat, line))
            {
                if (!line.compare(0, CpuNameLength, CpuName))
                {
                    istringstream cpuLine(line);

                    entries.emplace_back(CpuData());
                    auto& entry = entries.back();

                    cpuLine >> entry.cpu;
                    if (entry.cpu.size() > CpuNameLength)
                        entry.cpu.erase(0, CpuNameLength);
                    else
                        entry.cpu = Total;

                    for (auto i = 0; i < numberOfCounters; ++i)
                        cpuLine >> entry.times[i];
                }
            }
        }

        bool ReadCpuTotalCounters(CpuData& entry)
        {
            auto found {false};

            const string CpuName{"cpu"};
            const std::size_t CpuNameLength = CpuName.size();
            const string Total{"tot"};

            ifstream fileStat("/proc/stat");
            string line;
            while(getline(fileStat, line))
            {
                if (!line.compare(0, CpuNameLength, CpuName))
                {
                    istringstream cpuLine(line);
                    string lineName;
                    cpuLine >> lineName;
                    if (lineName.size() == CpuNameLength)
                    {
                        found = true;

                        entry.cpu = Total;

                        for (auto i = 0; i < numberOfCounters; ++i)
                            cpuLine >> entry.times[i];
                    }
                }
            }

            return found;
        }

        std::size_t GetIdleTime(const CpuData& entry)
        {
            return entry.times[idle_jiffies] + entry.times[iowait_jiffies];
        }

        std::size_t GetActiveTime(const CpuData& entry)
        {
            return  entry.times[user_jiffies] +
                    entry.times[nice_jiffies] +
                    entry.times[system_jiffies] +
                    entry.times[irq_jiffies] +
                    entry.times[softirq_jiffies] +
                    entry.times[steal_jiffies] +
                    entry.times[guest_jiffies] +
                    entry.times[guest_nice_jiffies];
        }
    };
}
