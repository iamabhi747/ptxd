#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include <cfg.hpp>

extern FILE* yyin;
extern int yyparse();
extern std::vector<std::unique_ptr<FunctionBlock>> parsedFunctions;

int main(int argc, char** argv)
{
    std::string infile = "research/ptx/branch.ptx";
    std::string outfile = "build/sample.cfg.txt";

    if (argc >= 2) infile = argv[1];
    if (argc >= 3) outfile = argv[2];

    FILE* iFile = fopen(infile.c_str(), "r");
    if (!iFile)
    {
        std::cerr << "[-] Could not open input file!" << std::endl;
        return 1;
    }
    yyin = iFile;

    if (yyparse() == 0 && parsedFunctions.size() != 0)
    {
        std::cout << "Parser Worked!!" << std::endl;

        std::ofstream oFile (outfile);
        if (oFile.is_open())
        {
            oFile << printCFG(parsedFunctions);
        }
    }
    else
    {
        std::cerr << "Failed!!" << std::endl;
    }
}