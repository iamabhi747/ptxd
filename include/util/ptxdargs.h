#ifndef PTXDARGS_H
#define PTXDARGS_H

#include <string>
#include <unordered_map>

#include <util/argh.h>

void ptxdargs(argh::parser& cmdl);
void getAllOpts(argh::parser& cmdl, std::unordered_map<std::string, std::string>& opts);


#endif