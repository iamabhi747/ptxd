#include <unordered_map>

#include <ptxd.h>

int main(int argc, char** argv)
{
    std::string infile = "research/ptx/branch.ptx";
    std::string outfile = "build/sample.cfg.txt";

    if (argc >= 2) infile = argv[1];
    if (argc >= 3) outfile = argv[2];

    std::unordered_map<std::string, std::string> opts;

    PTXD ptxd (infile, outfile, opts);
    ptxd.decompile();
}