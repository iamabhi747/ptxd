#ifndef PTXD_H
#define PTXD_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <cfg.hpp>
#include <util/logger.h>

class PTXD
{
public:
    PTXD(const std::string& _inFile, const std::string& _outFile, std::unordered_map<std::string, std::string>& options);

    void decompile(std::vector<std::string>& enabledTransformers);


    Logger& log;
    std::unordered_map<std::string, std::string>& opts;
    std::vector<std::unique_ptr<FunctionBlock>>& rawFunctions;
    
    const std::string inFile;
    const std::string outFile;


    void parse_ptx();
};

#endif