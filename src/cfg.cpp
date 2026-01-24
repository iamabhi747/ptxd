#include <cfg.hpp>

BranchBlock::BranchBlock(int id, const std::string& lbl) : id(id), label(lbl)
{}

BranchEdge::BranchEdge(BranchBlock* _block, bool _isDiversion): block (_block), isDiversion (_isDiversion)
{}

PTXCallseq::PTXCallseq()
{
    isCallSeq = true;
}