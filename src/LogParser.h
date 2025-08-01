// LogParser.h
#pragma once
#include <string>
#include <optional>
#include <vector>

// Represents a single parsed log line
struct LogEntry
{
    std::string timestamp;
    std::string logLevel;
    std::string requestId;
    std::string sourceIp;
    std::string httpMethod;
    std::string endpoint;
    int statusCode;
    int responseTimeMs;
    std::string message;
};

// Parses a single line of log text into a LogEntry struct
std::optional<LogEntry> parseLine(const std::string &line);