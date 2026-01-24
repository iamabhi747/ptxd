#ifndef CFG_H
#define CFG_H

#include <ptx/inst.hpp>

#include <vector>
#include <string>
#include <memory>

class BranchEdge;

class BranchBlock
{
public:
    int id;
    std::string label;

    std::vector<std::unique_ptr<PTXStmt>> statements;

    std::vector<BranchEdge> successors;
    std::vector<BranchEdge> predecessors;

    BranchBlock(int id, const std::string& lbl = "");
    ~BranchBlock() = default;
};

class BranchEdge
{
public:
    BranchBlock* block;
    bool isDiversion;

    BranchEdge(BranchBlock* _block, bool _isDiversion);
    ~BranchEdge() = default;
};

class FunctionBlock
{
public:
    std::string name;
    bool isKernel;

    std::vector<PTXVariable> returnVariables;

    std::unordered_map<std::string, PTXVariable> parameters;
    std::unordered_map<std::string, PTXVariable> registers;

    BranchBlock* entryBlock;
    std::vector<std::unique_ptr<BranchBlock>> blocks;
};


std::string printCFG(std::vector<std::unique_ptr<FunctionBlock>>& functions);

#endif