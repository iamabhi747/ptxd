#include <transformer.h>
#include <util/logger.h>
#include <unordered_set>  
#include <instmapper.h>

class InventoryTransformer : public Transformer
{
private: 
    struct MapFormat
    {
        int ref_count; 
        std::string operation; 
        bool isPure; 
        int idx;
        PTXOpcode INST_TYPE;  
        PTXInstruction* inst; 
    };

    inline static const std::unordered_set<PTXOpcode> pureOpcodes = {
        PTXOpcode::ADD, PTXOpcode::SUB, PTXOpcode::MUL, PTXOpcode::DIV,
        PTXOpcode::REM, PTXOpcode::MAD, PTXOpcode::FMA, PTXOpcode::ABS,
        PTXOpcode::NEG, PTXOpcode::MIN, PTXOpcode::MAX, PTXOpcode::AND,
        PTXOpcode::OR,  PTXOpcode::XOR, PTXOpcode::NOT, PTXOpcode::SHL,
        PTXOpcode::SHR, PTXOpcode::MOV, PTXOpcode::CVT, PTXOpcode::SETP,
        PTXOpcode::SELP, PTXOpcode::SQRT, PTXOpcode::SIN, PTXOpcode::COS
    }; 

    std::unordered_map<std::string, MapFormat> mp; 
    std::unordered_set<int> bmap; 
    InstMapper instMapper; 

    bool isAnchor(PTXInstruction* inst)
    {
        if (inst->op == PTXOpcode::ST || inst->op == PTXOpcode::BRA || 
            inst->op == PTXOpcode::RET || inst->op == PTXOpcode::CALL || 
            inst->op == PTXOpcode::EXIT || inst->op == PTXOpcode::ATOM || 
            inst->op == PTXOpcode::BAR) 
        {
            return true; 
        }

        return !pureOpcodes.contains(inst->op); 
    }

    std::string getFoldedExpr(const std::string& key)
    {
        if(!mp.contains(key)) return key; 

        MapFormat& def = mp[key]; 

        if(def.ref_count > 1 || !def.isPure || def.inst == nullptr)
        {
            return key; 
        }

        PTXInstruction* def_inst = def.inst; 

        for(int i = 1; i < def_inst->operands.size(); i ++)
        {
            std::string& operand_name = def_inst->operands[i].name; 

            if(!operand_name.empty() && operand_name != key)
            {
                std::string folded = getFoldedExpr(operand_name); 
                if(folded != operand_name){
                    operand_name = folded; 
                    def_inst->operands[i].raw = folded; 
                }
            }
        }

        std::string base_logic = instMapper.getOperationLogic(def_inst); 
        std::string final_logic = base_logic; 

        if(!def_inst->predicate.empty() && !base_logic.empty())
        {
            std::string predicate_name = [def_inst]()
            {
                int start = (def_inst->predicate[0] == '@' ? 1 : 0); 
                if(start < def_inst->predicate.size() && def_inst->predicate[start] == '!')
                {
                    start ++; 
                }

                return def_inst->predicate.substr(start); 
            }(); 

            if(!predicate_name.empty())
            {
                final_logic = predicate_name + " ? (" + base_logic + ") : " + def_inst->operands[0].name; 
            }
        }


        return "(" + final_logic + ")"; 

    }

    void foldAnchorsInBlock(BranchBlock* block)
    {
        if(!block || bmap.contains(block->id)) return ; 

        bmap.insert(block->id); 

        for(auto stmt = block->statements.rbegin(); stmt != block->statements.rend(); stmt ++)
        {
            PTXInstruction* inst = dynamic_cast<PTXInstruction*>(stmt->get()); 

            if(!inst) continue; 

            if(isAnchor(inst))
            {
                for(int i = 0; i < inst->operands.size(); i ++)
                {
                    std::string& operandName = inst->operands[i].name; 
                    if(!operandName.empty())
                    {
                        std::string folded_expr = getFoldedExpr(operandName); 
                        if(folded_expr != operandName)
                        {
                            log.logi(2, "FOLDED ANCHOR: ", operandName, " -> ", folded_expr);
                            operandName = folded_expr; 
                        }
                    }
                }
            }
        }

        for(auto& nextEdge : block->successors)
        {
            foldAnchorsInBlock(nextEdge.block);  
        }

    }


    void traverseBlock(BranchBlock* block, int& idx)
    {
        if(!block || bmap.contains(block->id)) return ; 
    
        bmap.insert(block->id); 
    
        for(auto& stmt : block->statements)
        {
            PTXInstruction* inst = dynamic_cast<PTXInstruction*>(stmt.get()); 
    
            if(inst != nullptr && !inst->operands.empty())
            {
                if(inst->op == PTXOpcode::BRA || inst->op == PTXOpcode::CALL || inst->op == PTXOpcode::RET)
                {
                    idx ++; 
                    continue; 
                }
                std::string assigner = inst->operands[0].name; 
                    
    
                if(assigner.empty())
                {
                    idx ++; 
                    continue; 
                }
                    
                std::string base_logic = instMapper.getOperationLogic(inst); 
                std::string final_logic = base_logic; 
                    
                for(int i = 1; i < inst->operands.size(); i ++)
                {
                    std::string operand = inst->operands[i].name; 
                    if(!operand.empty() && mp.contains(operand)) 
                    {
                        mp[operand].ref_count += 1;  
                    }
                }
    
                if (!inst->predicate.empty()) 
                {
                    std::string predicate_name = [inst]()
                    {
                        if(inst->predicate.empty()) return std::string(""); 
    
                        int start = (inst->predicate[0] == '@' ? 1 : 0); 
    
                        if(start < inst->predicate.size() && inst->predicate[start] == '!')
                        {
                            start++; 
                        }
    
                        return inst->predicate.substr(start); 
                    }();
    
                    if (!predicate_name.empty() && !base_logic.empty())
                    {
                        final_logic = predicate_name + " ? (" + base_logic + ") : " + assigner;
                    }
                }
    
    
                MapFormat newEntry; 
                newEntry.ref_count = 0; 
                newEntry.operation = final_logic; 
                newEntry.isPure = pureOpcodes.contains(inst->op);   
                newEntry.idx = idx; 
                newEntry.INST_TYPE = inst->op; 
                newEntry.inst = inst;
    
                mp[assigner] = newEntry;  
            }
    
            idx ++; 
        }
    
        for(auto& sucessorEdge : block->successors)
        {
            traverseBlock(sucessorEdge.block, idx);  
        }
    }
public:
    std::string getName() const override { return "Inventory"; }

    bool runFunc(std::unique_ptr<FunctionBlock>& func) override
    {
        log.logi(3, "Processing the function", func->name); 
        
        int idx = 0; 
        bmap.clear();
        mp.clear();
        if(!func->blocks.empty())
        {
            traverseBlock(func->blocks.front().get(), idx);  
        }


        log.logi(3, "Inventory Output for ", func->name);
        for (const auto& [assigner, data] : mp)
        {
            log.logi(3, "Var: ", assigner, 
                        " | Idx: ", data.idx, 
                        " | Refs: ", data.ref_count, 
                        " | Pure: ", (data.isPure ? "Yes" : "No"), 
                        " | Logic: ", data.operation,
                        " | AST Op: ", data.inst ? (int)data.inst->op : -1);
        }

        bmap.clear(); 
        if(!func->blocks.empty())
        {
            foldAnchorsInBlock(func->blocks.front().get()); 
        }

        return true;
    }
};

static struct InventoryRegister
{
    InventoryRegister()
    {
        Transformer::registerTransformer("inventory", []() {
            return std::make_unique<InventoryTransformer>();
        });
    }
} register_inventory;