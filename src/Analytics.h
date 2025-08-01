// Analytics.h
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <numeric>

struct Analytics
{
    uint64_t totalLines = 0;
    uint64_t errorCount = 0;
    uint64_t warningCount = 0;
    long double totalResponseTimeMs = 0;
    std::map<int, uint64_t> statusCodeCounts;
    std::map<std::string, uint64_t> httpMethodCounts;

    // Function to merge results from another Analytics object
    void merge(const Analytics &other)
    {
        totalLines += other.totalLines;
        errorCount += other.errorCount;
        warningCount += other.warningCount;
        totalResponseTimeMs += other.totalResponseTimeMs;

        for (const auto &pair : other.statusCodeCounts)
        {
            statusCodeCounts[pair.first] += pair.second;
        }
        for (const auto &pair : other.httpMethodCounts)
        {
            httpMethodCounts[pair.first] += pair.second;
        }
    }
};