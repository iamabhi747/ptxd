#ifndef GETDEFAULT_H
#define GETDEFAULT_H

#include <string>
#include <unordered_map>

std::string getDefault(std::unordered_map<std::string, std::string>& m, const std::string& key, const std::string& defaultVal)
{
    if (m.contains(key) && !m[key].empty()) return m[key];
    else return defaultVal;
}

#endif