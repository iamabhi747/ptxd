#include <transformer.h>
#include <util/logger.h>

#include <algorithm>

class PredicateTransformer : public Transformer
{
private:
    FunctionBlock* curFunc;
    BranchBlock* curBlock;
    bool moveStmts = false;

public:
    std::string getName() const override { return "Predicate"; }

    bool run() override
    {
        log.logi(3, "Called Predicate Transformer Run.");

        if (ptxd == nullptr || ptxd->rawFunctions.size() == 0)
        {
            log.logw(1, "PTXD not initilized properly.");
            return false;
        }

        for (auto& func: ptxd->rawFunctions)
        {
            if (!runFunc(func))
            {
                log.logw(1, getName(), "Transformer exausted at function", func->name);
                return false;
            }
        }

        return true;
    }

    bool runFunc(std::unique_ptr<FunctionBlock>& func)
    {
        curFunc = func.get();
        int initial_size = func->blocks.size();
        for (int i = 0; i < initial_size; i++)
        {
            auto block = std::move(func->blocks[i]);
            if (block == nullptr) continue;

            if (!runBlock(block))
            {
                log.logw(1, "Block (", block->id, "-", block->label, ") exausted.");
                func->blocks[i] = std::move(block);
                return false;
            }

            func->blocks[i] = std::move(block);
        }
        curFunc = nullptr;

        std::erase_if(func->blocks, [](const std::unique_ptr<BranchBlock>& ptr) {
            return ptr == nullptr; 
        });

        return true;
    }

    bool runBlock(std::unique_ptr<BranchBlock>& block)
    {
        curBlock = block.get();
        int initial_size = block->statements.size();
        for (int i = 0; i < initial_size; i++)
        {
            auto stmt = std::move(block->statements[i]);
            if (stmt == nullptr) continue;

            if (!runStmt(stmt))
            {
                log.logw(1, "Stmt (<TODO: REPR Stmt>) exausted.");
                block->statements[i] = std::move(stmt);
                return false;
            }

            block->statements[i] = std::move(stmt);
        }
        curBlock = nullptr;

        std::erase_if(block->statements, [](const std::unique_ptr<PTXStmt>& ptr) {
            return ptr == nullptr; 
        });

        if (block->statements.size() == 0)
        {
            for (const auto& pred : block->predecessors)
            {
                auto targetptr = block.get();
                std::erase_if(pred.block->successors, [targetptr](const BranchEdge& s) {
                    return s.block == targetptr;
                });
                
                pred.block->successors.insert(pred.block->successors.end(), block->successors.begin(), block->successors.end());
            }
            block.reset();
        }


        return true;
    }

    bool runStmt(std::unique_ptr<PTXStmt>& stmt)
    {
        if (!stmt->predicate.empty())
        {
            log.logi(1, "Found predicate (", stmt->predicate, ")");

            bool isNot = stmt->predicate[0] == '!';
            std::string reg = stmt->predicate.substr(stmt->predicate.find("%"));

            if (!stmt->isCallSeq)
            {
                PTXInstruction* inst = dynamic_cast<PTXInstruction*> (stmt.get());

                if (inst == nullptr)
                {
                    log.loge(1, "Failed to dynamic cast Stmt to Instruction.");
                    exit(1);
                }

                if (inst->op == PTXOpcode::BRA)
                {
                    curFunc->blocks.push_back(std::make_unique<BranchBlock>((int)curFunc->blocks.size(), curBlock->label + "-N"));
                    BranchBlock* nb = curFunc->blocks.back().get();

                    nb->successors = std::move(curBlock->successors);
                    curBlock->successors.clear();
                    curBlock->successors.emplace_back(nb, false);
                    nb->predecessors.emplace_back(curBlock, false);

                    if (inst->operands.size() != 1)
                    {
                        log.loge(1, "BRA instruction has more than one Argument.");
                        exit(1);
                    }

                    if (inst->operands[0].type != OperandType::LABEL)
                    {
                        log.loge(1, "BRA instruction has non label operand.");
                        exit(1);
                    }

                    std::string label = inst->operands[0].name;
                    bool found_label = false; 
                    for (const auto& bk : curFunc->blocks)
                    {
                        if (bk == nullptr) continue;

                        if (bk->label == label)
                        {
                            curBlock->successors.emplace_back(bk.get(), true);
                            bk->predecessors.emplace_back(curBlock, true);
                            found_label = true;
                            break;
                        }
                    }
                    if (curBlock->label == label)
                    {
                        curBlock->successors.emplace_back(curBlock, true);
                        curBlock->predecessors.emplace_back(curBlock, true);
                        found_label = true;
                    }

                    if (!found_label)
                    {
                        log.loge(1, "given label for Branch not found in current function");
                        exit(1);
                    }

                    curBlock->statements.push_back(std::move(stmt));
                    curBlock = nb;
                }
                else
                {
                    curFunc->blocks.push_back(std::make_unique<BranchBlock>((int)curFunc->blocks.size(), curBlock->label + "-Y"));
                    BranchBlock* nb = curFunc->blocks.back().get();

                    nb->statements.push_back(std::move(stmt));

                    curFunc->blocks.push_back(std::make_unique<BranchBlock>((int)curFunc->blocks.size(), curBlock->label + "-N"));
                    BranchBlock * nb2 = curFunc->blocks.back().get();

                    nb2->successors = std::move(curBlock->successors);
                    curBlock->successors.clear();

                    curBlock->successors.emplace_back(nb2, false);
                    curBlock->successors.emplace_back(nb, true);

                    nb2->predecessors.emplace_back(curBlock, false);
                    nb2->predecessors.emplace_back(nb, true);

                    nb->successors.emplace_back(nb2, false);
                    nb->predecessors.emplace_back(curBlock, true);

                    stmt = std::make_unique<PTXInstruction>();
                    stmt->predicate = nb->statements[0]->predicate;
                    PTXInstruction* ninst = dynamic_cast<PTXInstruction*> (stmt.get());
                    ninst->op = PTXOpcode::BRA;
                    ninst->operands.push_back({});
                    ninst->operands[0].type = OperandType::LABEL;
                    ninst->operands[0].name = nb->label;

                    nb->statements.back()->predicate = "";

                    curBlock = nb2;
                }

                moveStmts = true;
            }
            else
            {
                log.logw(1, "Unhandled case! predicate on CallSeq.");
            }
        }
        else if (moveStmts)
        {
            curBlock->statements.push_back(std::move(stmt));
        }

        return true;
    }
};

static struct PredicateRegister
{
    PredicateRegister()
    {
        Transformer::registerTransformer("predicate", []() {
            return std::make_unique<PredicateTransformer>();
        });
    }
} register_pridicate;