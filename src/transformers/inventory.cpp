#include <transformer.h>
#include <util/logger.h>
#include <unordered_set>  
#include <instmapper.h>

class InventoryTransformer : public Transformer
{
private: 
    struct MapFormat{
        int ref_count; 
        std::string operation; 
        bool isPure; 
        int idx;
        PTXOpcode INST_TYPE;  
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

public:
    std::string getName() const override { return "Mapper"; }

    
    void traverseBlock(BranchBlock* block, int& idx)
    {
        if(!block || bmap.contains(block->id)) return ; 

        bmap.insert(block->id); 

        for(auto& stmt : block->statements)
            {
                PTXInstruction* inst = dynamic_cast<PTXInstruction*>(stmt.get()); 

                if(inst != nullptr && !inst->operands.empty())
                {
                    if(inst->op == PTXOpcode::BRA 
                        || inst->op == PTXOpcode::CALL
                        || inst->op == PTXOpcode::RET)
                    {
                        idx ++; 
                        continue; 
                    }
                    std::string assigner = inst->operands[0].name; 
                    

                    if(assigner.empty()){
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
                        std::string predicate_name = [inst](){
                            if(inst->predicate.empty()) return std::string(""); 

                            int start = (inst->predicate[0] == '@' ? 1 : 0); 

                            if(start < inst->predicate.size() && inst->predicate[start] == '!'){
                                start ++; 
                            }

                            return inst->predicate.substr(start); 
                        }();

                        if (!predicate_name.empty() && !base_logic.empty()) {
                            final_logic = predicate_name + " ? (" + base_logic + ") : " + assigner;
                        }
                    }


                    MapFormat newEntry; 
                    newEntry.ref_count = 0; 
                    newEntry.operation = final_logic; 
                    newEntry.isPure = pureOpcodes.contains(inst->op);   
                    newEntry.idx = idx; 
                    newEntry.INST_TYPE = inst->op; 

                    mp[assigner] = newEntry;  
                }

                idx ++; 
            }

            for(auto& sucessorEdge : block->successors)
            {
                traverseBlock(sucessorEdge.block, idx);  
            }
    }


    bool runFunc(std::unique_ptr<FunctionBlock>& func) override
    {
        log.logi(1, "Processing the function", func->name); 
        
        int idx = 0; 
        bmap.clear();
        mp.clear();
        if(!func->blocks.empty())
        {
            traverseBlock(func->blocks.front().get(), idx);  
        }


        log.logi(3, "Mapper Output for ", func->name);
        for (const auto& [assigner, data] : mp)
        {
            log.logi(3, "Var: ", assigner, 
                        " | Idx: ", data.idx, 
                        " | Refs: ", data.ref_count, 
                        " | Pure: ", (data.isPure ? "Yes" : "No"), 
                        " | Logic: ", data.operation);
        }


        return true;
    }
};

static struct Inventory
{
    Inventory()
    {
        Transformer::registerTransformer("inventory", []() {
            return std::make_unique<InventoryTransformer>();
        });
    }
} register_sampleanalysis;