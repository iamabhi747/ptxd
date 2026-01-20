#include <util/ptxdargs.h>

void ptxdargs(argh::parser& cmdl)
{
    // Valued options
    cmdl.add_param({"-o", "--output"});
    
    // Boolean
    // -cfg : insted of decompilation outputs reconstructed CFG

    // Positional
    // cmdl(1) : Input file
}

void getAllOpts(argh::parser& cmdl, std::unordered_map<std::string, std::string>& opts)
{
    opts.insert(cmdl.params().begin(), cmdl.params().end());
    for (const auto& flag : cmdl.flags()) opts[flag] = "Y";
}