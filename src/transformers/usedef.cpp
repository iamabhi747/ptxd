#include <transformer.h>
#include <util/logger.h>

#include <set>

class UseDefTransformer : public Transformer
{
private:
    const std::set<PTXOpcode> skipableOpcodes = {
        PTXOpcode::BRA, PTXOpcode::CALL, PTXOpcode::RET, PTXOpcode::MBARRIER
    };
public:
    std::string getName() const override { return "UseDef"; }
    
    bool runFunc(std::unique_ptr<FunctionBlock>& func) override
    {
        if (!Transformer::runFunc(func)) return false;

        log.logi(4, "Refcounts for function", func->name);
        for (const auto& [regName, reg] : func->registers)
        {
            log.logi(4, regName, "\t:", reg.ref_count);
        }

        return true;
    }

    bool runStmt(std::unique_ptr<PTXStmt>& stmt) override
    {
        if (stmt->getName() == "Instruction")
        {
            PTXInstruction* inst = dynamic_cast<PTXInstruction*> (stmt.get());

            if (inst->op == PTXOpcode::BRA && !inst->predicate.empty())
            {
                std::string predReg = inst->predicate.substr(inst->predicate.find("%"));
                if (!crossCheckExistance(predReg)) return false;
                curFunc->registers[predReg].ref_count += 2;
            }

            if(skipableOpcodes.contains(inst->op) || inst->operands.size() <= 1) return true;

            // Possible operands formats that can reach here:
            // 1. <dest>, <operands...>
            // 2. [<dest>], <operands...>

            if (inst->operands[0].type == OperandType::REGISTER)
            {}
            else if (inst->operands[0].type == OperandType::MEMORY)
            {
                if (!inst->operands[0].baseReg.empty())
                {
                    if (!crossCheckExistance(inst->operands[0].baseReg)) return false;
                    curFunc->registers[inst->operands[0].baseReg].ref_count++;
                }
            }
            else
            {
                log.logw(1, "Unknown operand format. <TODO: Inst REPR>");
                return false;
            }

            for (int i = 1; i < inst->operands.size(); i++)
            {
                const auto& operand = inst->operands[i];

                if (operand.type == OperandType::REGISTER)
                {
                    if (!crossCheckExistance(operand.name)) return false;
                    curFunc->registers[operand.name].ref_count++;
                }
                else if (operand.type == OperandType::MEMORY)
                {
                    if (!operand.baseReg.empty())
                    {
                        if (!crossCheckExistance(operand.baseReg)) return false;
                        curFunc->registers[operand.baseReg].ref_count++;
                    }
                }
            }
        }
        else if (stmt->getName() == "CallSeq")
        {
            PTXCallseq* cseq = dynamic_cast<PTXCallseq*>(stmt.get());

            for (auto& sub_inst : cseq->rawInstructions)
            {
                runStmt(sub_inst);
            }
        }

        return true;
    }

    bool crossCheckExistance(const std::string& reg)
    {
        if (!curFunc->registers.contains(reg))
        {
            log.loge(1, "Register does not exits in current scope! (", reg, ")");
            return false;
        }
        return true;
    }

};

static struct UseDefRegister
{
    UseDefRegister()
    {
        Transformer::registerTransformer("usedef", 10, []() {
            return std::make_unique<UseDefTransformer>();
        });
    }
} register_usedef;