#include <unordered_map>

#include <ptxd.h>
#include <util/ptxdargs.h>

int main(int argc, char** argv)
{
    Logger& log = Logger::getInstance();
    log.verbose = 3;

    argh::parser cmdl;
    ptxdargs(cmdl);
    cmdl.parse(argc, argv);
    
    std::string infile = cmdl(1, "research/ptx/branch.ptx").str();
    std::string outfile = cmdl("o", "build/sample.cfg.txt").str();

    log.logi(2, "Input file :", infile);
    log.logi(2, "Output file:", outfile);

    std::unordered_map<std::string, std::string> opts;
    getAllOpts(cmdl, opts);

    PTXD ptxd (infile, outfile, opts);
    ptxd.decompile();
}