// main.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>

#include "src/LogParser.h"
#include "src/Analytics.h"

// The function each thread will execute
Analytics process_chunk(const std::string &filename, long long start, long long end)
{
    Analytics local_analytics;
    std::ifstream file(filename);
    file.seekg(start);

    // If not at the beginning, find the start of the next line
    if (start != 0)
    {
        std::string junk;
        std::getline(file, junk);
    }

    std::string line;
    while (file.tellg() < end && std::getline(file, line))
    {
        if (line.empty())
            continue;

        if (auto entry_opt = parseLine(line))
        {
            LogEntry &entry = *entry_opt;
            local_analytics.totalLines++;
            local_analytics.totalResponseTimeMs += entry.responseTimeMs;
            local_analytics.statusCodeCounts[entry.statusCode]++;
            local_analytics.httpMethodCounts[entry.httpMethod]++;

            if (entry.logLevel == "ERROR")
            {
                local_analytics.errorCount++;
            }
            else if (entry.logLevel == "WARN")
            {
                local_analytics.warningCount++;
            }
        }
    }
    return local_analytics;
}

void print_results(const Analytics &final_analytics, double duration_s)
{
    std::cout << "\n--- 📈 Log Analysis Report ---\n";
    std::cout << "Processing Time: " << std::fixed << std::setprecision(2) << duration_s << " seconds\n";
    std::cout << "---------------------------------\n";
    std::cout << "Total Requests Processed: " << final_analytics.totalLines << "\n";
    std::cout << "Total Errors: " << final_analytics.errorCount << "\n";
    std::cout << "Total Warnings: " << final_analytics.warningCount << "\n";

    if (final_analytics.totalLines > 0)
    {
        double error_rate = (static_cast<double>(final_analytics.errorCount) / final_analytics.totalLines) * 100.0;
        double avg_response_time = final_analytics.totalResponseTimeMs / final_analytics.totalLines;
        std::cout << "Error Rate: " << std::fixed << std::setprecision(2) << error_rate << "%\n";
        std::cout << "Average Response Time: " << avg_response_time << " ms\n";
    }

    std::cout << "\n--- HTTP Status Codes ---\n";
    for (const auto &pair : final_analytics.statusCodeCounts)
    {
        std::cout << "  " << pair.first << ": " << pair.second << " requests\n";
    }

    std::cout << "\n--- HTTP Method Distribution ---\n";
    for (const auto &pair : final_analytics.httpMethodCounts)
    {
        std::cout << "  " << pair.first << ": " << pair.second << " requests\n";
    }
    std::cout << "---------------------------------\n";
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <logfile.log>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::ate); // Open at the end to get size
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return 1;
    }

    long long file_size = file.tellg();
    file.close();

    // Determine number of threads - use hardware concurrency if available
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
    {
        num_threads = 4; // Default to 4 if detection fails
    }
    std::cout << "Starting analysis with " << num_threads << " threads...\n";

    std::vector<std::thread> threads;
    std::vector<Analytics> results(num_threads);
    long long chunk_size = file_size / num_threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 0; i < num_threads; ++i)
    {
        long long start = i * chunk_size;
        long long end = (i == num_threads - 1) ? file_size : (i + 1) * chunk_size;
        threads.emplace_back([&results, i, filename, start, end]()
                             { results[i] = process_chunk(filename, start, end); });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    // Merge results
    Analytics final_analytics;
    for (const auto &res : results)
    {
        final_analytics.merge(res);
    }

    print_results(final_analytics, duration.count());

    return 0;
}