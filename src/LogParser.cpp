// LogParser.cpp
#include "LogParser.h"
#include <sstream>
#include <vector>

std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

std::optional<LogEntry> parseLine(const std::string &line)
{
    std::vector<std::string> tokens = split(line, '|');

    if (tokens.size() != 9)
    {
        return std::nullopt; // Malformed line
    }

    try
    {
        LogEntry entry;
        entry.timestamp = tokens[0];
        entry.logLevel = tokens[1];
        entry.requestId = tokens[2];
        entry.sourceIp = tokens[3];
        entry.httpMethod = tokens[4];
        entry.endpoint = tokens[5];
        entry.statusCode = std::stoi(tokens[6]);
        entry.responseTimeMs = std::stoi(tokens[7]);
        entry.message = tokens[8];
        return entry;
    }
    catch (const std::invalid_argument &e)
    {
        // Handle cases where stoi fails
        return std::nullopt;
    }
    catch (const std::out_of_range &e)
    {
        // Handle cases where stoi result is out of range
        return std::nullopt;
    }
}