#include <ptxd.h>
#include <util/getdefault.h>
#include <transformer.h>

#include <iostream>
#include <fstream>
#include <unordered_map>

namespace fs = std::filesystem;
extern FILE* yyin;
extern int yyparse();
extern std::vector<std::unique_ptr<FunctionBlock>> parsedFunctions;

PTXD* Transformer::ptxd = nullptr;

PTXD::PTXD(const std::string& _inFile, const std::string& _outFile, std::unordered_map<std::string, std::string>& options) : inFile (_inFile), outFile (_outFile), log (Logger::getInstance()), opts (options), rawFunctions (parsedFunctions)
{
    if (!fs::is_regular_file(inFile))
    {
        log.loge(1, "Input file does not exists or not valid. (", inFile, ")");
        exit(1);
    }

    Transformer::ptxd = this;
}

void PTXD::parse_ptx()
{
    FILE* iFile = fopen(inFile.c_str(), "r");
    if (!iFile)
    {
        log.loge(1, "Failed to open input file. (", inFile, ")");
        exit(1);
    }
    yyin = iFile;

    if (yyparse() != 0 || rawFunctions.size() == 0)
    {
        log.loge(1, "Decompilation failed due to syntax error in input file.");
        exit(1);
    }

    log.logs(2, "Successfully parsed input PTX ISA file.");
}

void PTXD::decompile(std::vector<std::string>& enabledTransformers)
{
    parse_ptx();

    std::ofstream oFile (outFile);
    if (!oFile.is_open())
    {
        log.loge(1, "Failed to open output file. (", outFile, ")");
        exit(1);   
    }

    log.logi(3, "TODO: Decompilation...");

    if (enabledTransformers.empty())
    {
        Transformer::getAllTransformers(enabledTransformers);
    }

    for (const auto& transformerName : enabledTransformers)
    {
        Transformer::run(transformerName);
    }

    for (const auto& func : rawFunctions)
    {
        oFile << *func << std::endl << std::endl;
    }
}