#include <ptxd.h>

#include <iostream>
#include <fstream>
#include <unordered_map>

namespace fs = std::filesystem;
extern FILE* yyin;
extern int yyparse();
extern std::vector<std::unique_ptr<FunctionBlock>> parsedFunctions;

PTXD::PTXD(const std::string& _inFile, const std::string& _outFile, std::unordered_map<std::string, std::string>& options) : inFile (_inFile), outFile (_outFile), rawFunctions (parsedFunctions)
{
    if (!fs::is_regular_file(inFile))
    {
        std::cerr << "Invalid input file" << std::endl;
        exit(1);
    }
}

void PTXD::parse_ptx()
{
    FILE* iFile = fopen(inFile.c_str(), "r");
    if (!iFile)
    {
        std::cerr << "Failed to input open file!" << std::endl;
        exit(1);
    }
    yyin = iFile;

    if (yyparse() != 0 || rawFunctions.size() == 0)
    {
        std::cerr << "Parser Error!" << std::endl;
        exit(1);
    }
}

void PTXD::decompile()
{
    parse_ptx();

    std::cout << "Parser Worked!!" << std::endl;

    std::ofstream oFile (outFile);
    if (oFile.is_open())
    {
        oFile << printCFG(parsedFunctions);
    }
}