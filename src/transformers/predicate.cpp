#include <transformer.h>
#include <util/logger.h>

#include <algorithm>

class PredicateTransformer : public Transformer
{
private:
    bool moveStmts = false;

public:
    std::string getName() const override { return "Predicate"; }

    bool runStmt(std::unique_ptr<PTXStmt>& stmt) override
    {
        if (stmt->getName() == "Instruction")
        {
            PTXInstruction* inst = dynamic_cast<PTXInstruction*> (stmt.get());

            if (inst == nullptr)
            {
                log.loge(1, "Failed to dynamic cast Stmt to Instruction.");
                exit(1);
            }

            if (!inst->predicate.empty())
            {
                log.logi(4, "Found predicate (", inst->predicate, ")");

                bool isNot = inst->predicate[0] == '!';
                std::string reg = inst->predicate.substr(inst->predicate.find("%"));

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

                    std::string curPredicate = std::move(inst->predicate);
                    inst->predicate.clear();
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
                    PTXInstruction* ninst = dynamic_cast<PTXInstruction*> (stmt.get());
                    ninst->predicate = curPredicate;
                    ninst->op = PTXOpcode::BRA;
                    ninst->operands.push_back({});
                    ninst->operands[0].type = OperandType::LABEL;
                    ninst->operands[0].name = nb->label;

                    curBlock = nb2;
                }

                moveStmts = true;
            }
            else if (moveStmts)
            {
                curBlock->statements.push_back(std::move(stmt));
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
        Transformer::registerTransformer("predicate", 1, []() {
            return std::make_unique<PredicateTransformer>();
        });
    }
} register_pridicate;