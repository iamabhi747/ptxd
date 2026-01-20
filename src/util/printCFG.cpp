#include <cfg.hpp>
#include <ptx/inst.hpp>

#include <string>
#include <sstream>

std::string opToString(PTXOpcode op) {
    switch(op) {
        case PTXOpcode::BRA: return "bra"; case PTXOpcode::BRX: return "brx";
        case PTXOpcode::CALL: return "call"; case PTXOpcode::RET: return "ret";
        case PTXOpcode::EXIT: return "exit"; case PTXOpcode::TRAP: return "trap";
        case PTXOpcode::BRKPT: return "brkpt"; case PTXOpcode::YIELD: return "yield";
        case PTXOpcode::NANOSLEEP: return "nanosleep"; case PTXOpcode::MOV: return "mov";
        case PTXOpcode::LD: return "ld"; case PTXOpcode::ST: return "st";
        case PTXOpcode::LDU: return "ldu"; case PTXOpcode::LDG: return "ldg";
        case PTXOpcode::CVT: return "cvt"; case PTXOpcode::CVTA: return "cvta";
        case PTXOpcode::ISSPACEP: return "isspacep"; case PTXOpcode::PREFETCH: return "prefetch";
        case PTXOpcode::PREFETCHU: return "prefetchu"; case PTXOpcode::PACK: return "pack";
        case PTXOpcode::UNPACK: return "unpack"; case PTXOpcode::SHFL: return "shfl";
        case PTXOpcode::PRMT: return "prmt"; case PTXOpcode::SET: return "set";
        case PTXOpcode::SETP: return "setp"; case PTXOpcode::SELP: return "selp";
        case PTXOpcode::SLCT: return "slct"; case PTXOpcode::ADD: return "add";
        case PTXOpcode::ADDC: return "addc"; case PTXOpcode::SUB: return "sub";
        case PTXOpcode::SUBC: return "subc"; case PTXOpcode::MUL: return "mul";
        case PTXOpcode::MUL24: return "mul24"; case PTXOpcode::MAD: return "mad";
        case PTXOpcode::MAD24: return "mad24"; case PTXOpcode::FMA: return "fma";
        case PTXOpcode::SAD: return "sad"; case PTXOpcode::DIV: return "div";
        case PTXOpcode::REM: return "rem"; case PTXOpcode::ABS: return "abs";
        case PTXOpcode::NEG: return "neg"; case PTXOpcode::MIN: return "min";
        case PTXOpcode::MAX: return "max"; case PTXOpcode::DP4A: return "dp4a";
        case PTXOpcode::DP2A: return "dp2a"; case PTXOpcode::RCP: return "rcp";
        case PTXOpcode::SQRT: return "sqrt"; case PTXOpcode::RSQRT: return "rsqrt";
        case PTXOpcode::SIN: return "sin"; case PTXOpcode::COS: return "cos";
        case PTXOpcode::LG2: return "lg2"; case PTXOpcode::EX2: return "ex2";
        case PTXOpcode::TANH: return "tanh"; case PTXOpcode::AND: return "and";
        case PTXOpcode::OR: return "or"; case PTXOpcode::XOR: return "xor";
        case PTXOpcode::NOT: return "not"; case PTXOpcode::CNOT: return "cnot";
        case PTXOpcode::SHL: return "shl"; case PTXOpcode::SHR: return "shr";
        case PTXOpcode::LOP3: return "lop3"; case PTXOpcode::POPC: return "popc";
        case PTXOpcode::CLZ: return "clz"; case PTXOpcode::BFIND: return "bfind";
        case PTXOpcode::FNS: return "fns"; case PTXOpcode::BREV: return "brev";
        case PTXOpcode::BFE: return "bfe"; case PTXOpcode::BFI: return "bfi";
        case PTXOpcode::BAR: return "bar"; case PTXOpcode::BARRIER: return "barrier";
        case PTXOpcode::MEMBAR: return "membar"; case PTXOpcode::FENCE: return "fence";
        case PTXOpcode::ATOM: return "atom"; case PTXOpcode::RED: return "red";
        case PTXOpcode::REDUX: return "redux"; case PTXOpcode::VOTE: return "vote";
        case PTXOpcode::MATCH: return "match"; case PTXOpcode::ACTIVEMASK: return "activemask";
        case PTXOpcode::CP: return "cp"; case PTXOpcode::MBARRIER: return "mbarrier";
        case PTXOpcode::TEX: return "tex"; case PTXOpcode::TLD4: return "tld4";
        case PTXOpcode::TXQ: return "txq"; case PTXOpcode::SULD: return "suld";
        case PTXOpcode::SUST: return "sust"; case PTXOpcode::SURED: return "sured";
        case PTXOpcode::SUQ: return "suq"; case PTXOpcode::VPRINTF: return "vprintf";
        case PTXOpcode::PMEVENT: return "pmevent"; default: return "unknown";
    }
}

std::string dtToString(PTXDataType dt) {
    switch(dt) {
        case PTXDataType::B8: return ".b8"; case PTXDataType::B16: return ".b16";
        case PTXDataType::B32: return ".b32"; case PTXDataType::B64: return ".b64";
        case PTXDataType::U8: return ".u8"; case PTXDataType::U16: return ".u16";
        case PTXDataType::U32: return ".u32"; case PTXDataType::U64: return ".u64";
        case PTXDataType::S8: return ".s8"; case PTXDataType::S16: return ".s16";
        case PTXDataType::S32: return ".s32"; case PTXDataType::S64: return ".s64";
        case PTXDataType::F16: return ".f16"; case PTXDataType::F32: return ".f32";
        case PTXDataType::F64: return ".f64"; case PTXDataType::PRED: return ".pred";
        default: return "";
    }
}

std::string spaceToString(PTXSpace sp) {
    switch(sp) {
        case PTXSpace::REG: return ".reg"; case PTXSpace::PARAM: return ".param";
        case PTXSpace::SHARED: return ".shared"; case PTXSpace::GLOBAL: return ".global";
        case PTXSpace::CONST: return ".const"; case PTXSpace::LOCAL: return ".local";
        default: return "";
    }
}

std::string widthToString(PTXWidth w) {
    switch(w) {
        case PTXWidth::WIDE: return ".wide"; case PTXWidth::LO: return ".lo";
        case PTXWidth::HI: return ".hi"; default: return "";
    }
}

std::string compToString(PTXComp c) {
    switch(c) {
        case PTXComp::EQ: return ".eq"; case PTXComp::NE: return ".ne";
        case PTXComp::LT: return ".lt"; case PTXComp::LE: return ".le";
        case PTXComp::GT: return ".gt"; case PTXComp::GE: return ".ge";
        case PTXComp::EQU: return ".equ"; case PTXComp::NEU: return ".neu";
        case PTXComp::LTU: return ".ltu"; case PTXComp::LEU: return ".leu";
        case PTXComp::GTU: return ".gtu"; case PTXComp::GEU: return ".geu";
        case PTXComp::NUM: return ".num"; case PTXComp::NANX: return ".nan";
        default: return "";
    }
}

std::string printCFG(std::vector<std::unique_ptr<FunctionBlock>>& functions)
{
    std::stringstream out;

    for (const auto& f : functions) {
        out << (f->isKernel ? ".entry " : ".func ") << f->name << "(\n";
        bool first = true;
        for (const auto& [name, param] : f->parameters) {
            out << (first ? "" : ",\n") << "    " << spaceToString(param.space) << dtToString(param.type) << " " << param.name;
            first = false;
        }
        out << "\n)\n{\n";

        for (const auto& [name, reg] : f->registers) {
            out << "    " << spaceToString(reg.space) << dtToString(reg.type) << " " << reg.name;
            if (reg.arrSize > 1) out << "<" << reg.arrSize << ">";
            out << ";\n";
        }
        out << "\n";

        for (const auto& bb : f->blocks) {
            out << bb->label << ":\n";
            for (const auto& stmt : bb->statements) {
                if (!stmt->predicate.empty()) {
                    out << "    @" << stmt->predicate << " ";
                } else {
                    out << "    ";
                }
                
                if (auto pInst = dynamic_cast<PTXInstruction*>(stmt.get())) {
                    out << opToString(pInst->op) << widthToString(pInst->width) 
                        << compToString(pInst->comp) << dtToString(pInst->type) 
                        << spaceToString(pInst->space);
                    for (const auto& g : pInst->genericModifiers) out << g;
                    out << "\t";
                    bool firstOp = true;
                    for (const auto& op : pInst->operands) {
                        out << (firstOp ? "" : ", ") << op.raw;
                        firstOp = false;
                    }
                    out << ";\n";
                } else if (auto pCall = dynamic_cast<PTXCallseq*>(stmt.get())) {
                    out << "callseq " << pCall->name << " {\n";
                    for (const auto& rInst : pCall->rawInstructions) {
                        out << "        " << opToString(rInst.op) << dtToString(rInst.type) << "\t";
                        bool firstOp = true;
                        for (const auto& op : rInst.operands) {
                            out << (firstOp ? "" : ", ") << op.raw;
                            firstOp = false;
                        }
                        out << ";\n";
                    }
                    out << "    }\n";
                }
            }
        }
        out << "}\n\n";
    }

    return out.str();
}
