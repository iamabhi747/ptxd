#ifndef PTXD_H
#define PTXD_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <cfg.hpp>

class PTXD
{
public:
    PTXD(const std::string& _inFile, const std::string& _outFile, std::unordered_map<std::string, std::string>& options);

    void decompile();

private:
    const std::string inFile;
    const std::string outFile;

    std::vector<std::unique_ptr<FunctionBlock>>& rawFunctions;

    void parse_ptx();
};

#endif