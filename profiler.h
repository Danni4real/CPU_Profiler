//
// Created by dan on 2026/7/16.
//

#ifndef PROFILER_H
#define PROFILER_H

#include <ctime>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#define PROFILE_FUNCTION() \
    static FunctionCpuUsageMonitor _function_monitor_(__PRETTY_FUNCTION__); \
    static std::atomic_bool _registered_{false}; \
    if (!_registered_) { \
        _registered_ = true; \
        CpuUsageManager::instance().register_(&_function_monitor_); \
    } \
    RunScopeOnceCpuUsageMonitor _scope_moitor_(&_function_monitor_);

class FunctionCpuUsageMonitor {
public:
    FunctionCpuUsageMonitor(const std::string& func_name) {
        m_function_name = func_name;
    }

    std::string get_function_name() {
        return m_function_name;
    }

    void add_used_cpu_time_ns(uint64_t cpu_time) {
        m_used_cpu_time_ns += cpu_time;
    }

    uint64_t get_used_cpu_time_ns() {
        return m_used_cpu_time_ns;
    }

    void reset_used_cpu_time() {
        m_used_cpu_time_ns = 0;
    }

    bool operator>(const FunctionCpuUsageMonitor& other) const {
        return m_used_cpu_time_ns > other.m_used_cpu_time_ns;
    }

private:
    std::string m_function_name;
    std::atomic<uint64_t> m_used_cpu_time_ns{0};
};

class RunScopeOnceCpuUsageMonitor {
public:
    RunScopeOnceCpuUsageMonitor(FunctionCpuUsageMonitor* function_monitor) {
        m_function_monitor = function_monitor;

        timespec ts{};
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
        m_cpu_time_start_ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000 + static_cast<uint64_t>(ts.tv_nsec);
    }

    ~RunScopeOnceCpuUsageMonitor() {
        timespec ts{};
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
        m_cpu_time_end_ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000 + static_cast<uint64_t>(ts.tv_nsec);

        if (m_function_monitor) {
            m_function_monitor->add_used_cpu_time_ns(m_cpu_time_end_ns - m_cpu_time_start_ns);
        }
    }

private:
    uint64_t m_cpu_time_start_ns{0};
    uint64_t m_cpu_time_end_ns{0};

    FunctionCpuUsageMonitor* m_function_monitor{nullptr};
};

class CpuUsageManager {
public:
    static CpuUsageManager& instance() {
        static CpuUsageManager instance;

        return instance;
    }

    void register_(FunctionCpuUsageMonitor* monitor) {
        std::scoped_lock lk(m_function_monitor_list_mtx);
        m_function_monitor_list.insert(monitor);
    }

    void print_cpu_usage_ranking(uint64_t process_used_cpu_time_ns) {
        std::unordered_set<FunctionCpuUsageMonitor*> function_monitor_list_copy{};

        {
            std::scoped_lock lk(m_function_monitor_list_mtx);
            function_monitor_list_copy = m_function_monitor_list;
        }

        std::vector vec(function_monitor_list_copy.begin(), function_monitor_list_copy.end());

        std::sort(vec.begin(), vec.end(),
                  [](const FunctionCpuUsageMonitor* a, const FunctionCpuUsageMonitor* b) {
                      return *a > *b;
                  });

        int count = 10;
        for (auto item : vec) {
            if (count-- > 0 && process_used_cpu_time_ns > 0) {
                std::cout
                    << "Percent: " << item->get_used_cpu_time_ns() * 100 / process_used_cpu_time_ns << "%"
                    << "  Time: " << item->get_used_cpu_time_ns() << "ns"
                    << "  Func: " << item->get_function_name() << std::endl;
            }

            item->reset_used_cpu_time();
        }
    }

private:
    CpuUsageManager() {
        std::thread([this] {
            while (true) {
                timespec ts{};

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
                auto start_time = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;

                std::this_thread::sleep_for(std::chrono::seconds(30));

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
                auto end_time = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;

                print_cpu_usage_ranking(end_time - start_time);
            }
        }).detach();
    }

    std::mutex m_function_monitor_list_mtx{};
    std::unordered_set<FunctionCpuUsageMonitor*> m_function_monitor_list{};
};
#endif //PROFILER_H
