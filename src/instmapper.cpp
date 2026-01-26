#include <instmapper.h>
#include <util/logger.h>

InstMapper::InstMapper()
{
    registerHandlers(); 
} 

void InstMapper::registerHandlers() 
{
    // ==========================================
    // 1. Data Movement
    // ==========================================
    handlers[PTXOpcode::MOV] = [](PTXInstruction* inst) {
        return inst->operands[1].raw;
    };
    
    // LD and ST (Load/Store) can are technically assignment, 
    // but syntax might differ if memory brackets [] are involved.
    handlers[PTXOpcode::LD] = [](PTXInstruction* inst) {
        return "*" + inst->operands[1].raw; // e.g. a = *ptr
    };
    handlers[PTXOpcode::CVT] = [](PTXInstruction* inst) {
        std::string typeStr;
        switch (inst->type) {
            case PTXDataType::B8:  case PTXDataType::U8:  typeStr = "uint8_t"; break;
            case PTXDataType::B16: case PTXDataType::U16: typeStr = "uint16_t"; break;
            case PTXDataType::B32: case PTXDataType::U32: typeStr = "uint32_t"; break;
            case PTXDataType::B64: case PTXDataType::U64: typeStr = "uint64_t"; break;
            case PTXDataType::S8:  typeStr = "int8_t"; break;
            case PTXDataType::S16: typeStr = "int16_t"; break;
            case PTXDataType::S32: typeStr = "int32_t"; break;
            case PTXDataType::S64: typeStr = "int64_t"; break;
            case PTXDataType::F16: typeStr = "half"; break;
            case PTXDataType::F32: typeStr = "float"; break;
            case PTXDataType::F64: typeStr = "double"; break;
            case PTXDataType::PRED: typeStr = "bool"; break;
            default: typeStr = "auto"; break;
        }
        return "(" + typeStr + ")" + inst->operands[1].raw;
    };

    // ==========================================
    // 2. Arithmetic Operations
    // ==========================================
    handlers[PTXOpcode::ADD] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " + " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::SUB] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " - " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::MUL] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " * " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::DIV] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " / " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::REM] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " % " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::MAD] = [](PTXInstruction* inst) {
        return "(" + inst->operands[1].raw + " * " + inst->operands[2].raw + ") + " + inst->operands[3].raw;
    };
    handlers[PTXOpcode::FMA] = handlers[PTXOpcode::MAD]; // FMA is logically identical to MAD
    
    handlers[PTXOpcode::ABS] = [](PTXInstruction* inst) {
        return "abs(" + inst->operands[1].raw + ")";
    };
    handlers[PTXOpcode::NEG] = [](PTXInstruction* inst) {
        return "-" + inst->operands[1].raw;
    };
    handlers[PTXOpcode::MIN] = [](PTXInstruction* inst) {
        return "min(" + inst->operands[1].raw + ", " + inst->operands[2].raw + ")";
    };
    handlers[PTXOpcode::MAX] = [](PTXInstruction* inst) {
        return "max(" + inst->operands[1].raw + ", " + inst->operands[2].raw + ")";
    };

    // ==========================================
    // 3. Logic & Bitwise
    // ==========================================
    handlers[PTXOpcode::AND] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " & " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::OR] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " | " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::XOR] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " ^ " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::NOT] = [](PTXInstruction* inst) {
        return "~" + inst->operands[1].raw;
    };
    handlers[PTXOpcode::CNOT] = [](PTXInstruction* inst) {
        return "!" + inst->operands[1].raw;
    };
    handlers[PTXOpcode::SHL] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " << " + inst->operands[2].raw;
    };
    handlers[PTXOpcode::SHR] = [](PTXInstruction* inst) {
        return inst->operands[1].raw + " >> " + inst->operands[2].raw;
    };

    // ==========================================
    // 4. Extended Math
    // ==========================================
    handlers[PTXOpcode::SQRT] = [](PTXInstruction* inst) {
        return "sqrt(" + inst->operands[1].raw + ")";
    };
    handlers[PTXOpcode::RSQRT] = [](PTXInstruction* inst) {
        return "1.0 / sqrt(" + inst->operands[1].raw + ")";
    };
    handlers[PTXOpcode::SIN] = [](PTXInstruction* inst) {
        return "sin(" + inst->operands[1].raw + ")";
    };
    handlers[PTXOpcode::COS] = [](PTXInstruction* inst) {
        return "cos(" + inst->operands[1].raw + ")";
    };
    handlers[PTXOpcode::LG2] = [](PTXInstruction* inst) {
        return "log2(" + inst->operands[1].raw + ")";
    };
    handlers[PTXOpcode::EX2] = [](PTXInstruction* inst) {
        return "exp2(" + inst->operands[1].raw + ")";
    };

    // ==========================================
    // 5. Predicate & Comparison
    // ==========================================
    handlers[PTXOpcode::SETP] = [](PTXInstruction* inst) {
        // PTX comparison (e.g. setp.eq %p1, %r1, %r2) gives you an eq, lt, gt.
        // For standard decompilation without peeking at the modifiers for now, 
        // a generic syntax can act as a placeholder.
        return "COMPARE(" + inst->operands[1].raw + ", " + inst->operands[2].raw + ")";
    };
    
    handlers[PTXOpcode::SELP] = [](PTXInstruction* inst) {
        // selp dest, a, b, pred  => dest = pred ? a : b
        if (inst->operands.size() >= 4) {
            return inst->operands[3].raw + " ? " + inst->operands[1].raw + " : " + inst->operands[2].raw;
        }
        return std::string("SELP_UNKNOWN"); 
    };


    handlers[PTXOpcode::CVTA] = [](PTXInstruction* inst) {
        return inst->operands[1].raw; 
    };
}

std::string InstMapper::getOperationLogic(PTXInstruction* inst)
{
    if(!inst || inst->operands.size() == 0) return ""; 

    auto it = handlers.find(inst->op); 
    if(it != handlers.end()){
        return it->second(inst); 
    }

    return "UNHANDLED_OPERATION" + inst->operands[0].raw; 
}