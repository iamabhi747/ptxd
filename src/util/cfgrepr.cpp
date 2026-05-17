#include <cfg.hpp>
#include <sstream>
#include <string>

static thread_local std::string tls_cfg_repr_buffer;
static std::string_view to_cfg_repr(const std::string& str) {
    tls_cfg_repr_buffer = str;
    return tls_cfg_repr_buffer;
}

std::string_view BranchBlock::REPR() {
    std::stringstream ss;
    ss << "Block_" << id;
    if (!label.empty()) {
        ss << " [" << label << "]";
    }
    return to_cfg_repr(ss.str());
}

std::ostream& operator<<(std::ostream& os, BranchBlock& obj) {
    os << obj.REPR() << ":\n";
    for (const auto& stmt : obj.statements) {
        os << "  " << stmt->REPR() << "\n";
    }
    return os;
}

std::string_view BranchEdge::REPR() {
    std::stringstream ss;
    ss << "-> Block_" << block->id;
    if (isDiversion) ss << " (diversion)";
    return to_cfg_repr(ss.str());
}

std::ostream& operator<<(std::ostream& os, BranchEdge& obj) {
    os << obj.REPR();
    return os;
}

std::string_view FunctionBlock::REPR() {
    std::stringstream ss;
    if (isKernel) ss << "__global__ ";
    ss << name << "(";
    bool first = true;
    for (const auto& [n, p] : parameters) {
        if (!first) ss << ", ";
        ss << n;
        first = false;
    }
    ss << ")";
    return to_cfg_repr(ss.str());
}

std::ostream& operator<<(std::ostream& os, FunctionBlock& obj) {
    os << "Function: " << obj.REPR() << "\n";
    for (const auto& bb : obj.blocks) {
        os << *bb;
        if (!bb->successors.empty()) {
            os << "  Successors: ";
            for (auto& succ : bb->successors) {
                os << succ << "; ";
            }
            os << "\n";
        }
    }
    return os;
}


std::string_view PTXVariable::REPR() {
    std::stringstream ss;
    ss << name << " [" << type << ", " << space << "]";
    return to_cfg_repr(ss.str());
}

std::ostream& operator<<(std::ostream& os, PTXVariable& obj) {
    return os << obj.name;
}

std::string_view PTXInstruction::REPR() {
    std::stringstream ss;
    if (!predicate.empty()) ss << "@" << predicate << " ";
    ss << op;
    if (type != PTXDataType::NONE) ss << "." << type;
    if (space != PTXSpace::NONE) ss << "." << space;
    if (width != PTXWidth::NONE) ss << "." << width;
    if (comp != PTXComp::NONE) ss << "." << comp;
    if (isSaturate) ss << ".sat";
    for (const auto& mod : genericModifiers) {
        ss << "." << mod;
    }
    ss << " ";
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << operands[i].REPR();
    }
    ss << ";";
    return to_cfg_repr(ss.str());
}

std::string_view PTXCallseq::REPR() {
    std::stringstream ss;
    ss << "call ";
    if (!returnParams.empty()) {
        ss << "(";
        for (size_t i = 0; i < returnParams.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << returnParams[i];
        }
        ss << "), ";
    }
    ss << name << ", (";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << arguments[i];
    }
    ss << ");";
    return to_cfg_repr(ss.str());
}

std::ostream& operator<<(std::ostream& os, PTXStmt& obj) {
    return os << obj.REPR();
}

constexpr std::string_view REPR(PTXSpace obj) {
    switch (obj) {
        case PTXSpace::REG: return "reg";
        case PTXSpace::PARAM: return "param";
        case PTXSpace::SHARED: return "shared";
        case PTXSpace::GLOBAL: return "global";
        case PTXSpace::CONST: return "const";
        case PTXSpace::LOCAL: return "local";
        default: return "";
    }
}
std::ostream& operator<<(std::ostream& os, PTXSpace obj) { return os << REPR(obj); }

constexpr std::string_view REPR(PTXDataType obj) {
    switch (obj) {
        case PTXDataType::B8: return "b8";
        case PTXDataType::B16: return "b16";
        case PTXDataType::B32: return "b32";
        case PTXDataType::B64: return "b64";
        case PTXDataType::U8: return "u8";
        case PTXDataType::U16: return "u16";
        case PTXDataType::U32: return "u32";
        case PTXDataType::U64: return "u64";
        case PTXDataType::S8: return "s8";
        case PTXDataType::S16: return "s16";
        case PTXDataType::S32: return "s32";
        case PTXDataType::S64: return "s64";
        case PTXDataType::F16: return "f16";
        case PTXDataType::F32: return "f32";
        case PTXDataType::F64: return "f64";
        case PTXDataType::PRED: return "pred";
        default: return "";
    }
}
std::ostream& operator<<(std::ostream& os, PTXDataType obj) { return os << REPR(obj); }

constexpr std::string_view REPR(PTXWidth obj) {
    switch (obj) {
        case PTXWidth::WIDE: return "wide";
        case PTXWidth::LO: return "lo";
        case PTXWidth::HI: return "hi";
        default: return "";
    }
}
std::ostream& operator<<(std::ostream& os, PTXWidth obj) { return os << REPR(obj); }

constexpr std::string_view REPR(PTXComp obj) {
    switch (obj) {
        case PTXComp::EQ: return "eq";
        case PTXComp::NE: return "ne";
        case PTXComp::LT: return "lt";
        case PTXComp::LE: return "le";
        case PTXComp::GT: return "gt";
        case PTXComp::GE: return "ge";
        case PTXComp::EQU: return "equ";
        case PTXComp::NEU: return "neu";
        case PTXComp::LTU: return "ltu";
        case PTXComp::LEU: return "leu";
        case PTXComp::GTU: return "gtu";
        case PTXComp::GEU: return "geu";
        case PTXComp::NUM: return "num";
        case PTXComp::NANX: return "nan";
        
        case PTXComp::ADD: return "+";
        case PTXComp::SUB: return "-";
        case PTXComp::MUL: return "*";
        case PTXComp::DIV: return "/";
        case PTXComp::MOD: return "%";
        case PTXComp::AND: return "&";
        case PTXComp::OR: return "|";
        case PTXComp::NOT: return "~";
        case PTXComp::XOR: return "^";
        case PTXComp::SHL: return "<<";
        case PTXComp::SHR: return ">>";
        case PTXComp::LAND: return "&&";
        case PTXComp::LOR: return "||";
        case PTXComp::LNOT: return "!";
        default: return "";
    }
}
std::ostream& operator<<(std::ostream& os, PTXComp obj) { return os << REPR(obj); }

constexpr std::string_view REPR(OperandType obj) {
    switch (obj) {
        case OperandType::REGISTER: return "register";
        case OperandType::SPECIAL_REGISTER: return "special_reg";
        case OperandType::IMMEDIATE_INT: return "imm_int";
        case OperandType::IMMEDIATE_FLOAT: return "imm_float";
        case OperandType::MEMORY: return "memory";
        case OperandType::LABEL: return "label";
        default: return "";
    }
}
std::ostream& operator<<(std::ostream& os, OperandType obj) { return os << REPR(obj); }

std::string_view PTXOperand::REPR() {
    return to_cfg_repr(raw);
}

std::ostream& operator<<(std::ostream& os, PTXOperand& obj) {
    return os << obj.REPR();
}

constexpr std::string_view REPR(PTXOpcode obj) {
    switch (obj) {
        case PTXOpcode::BRA: return "bra";
        case PTXOpcode::BRX: return "brx";
        case PTXOpcode::CALL: return "call";
        case PTXOpcode::RET: return "ret";
        case PTXOpcode::EXIT: return "exit";
        case PTXOpcode::TRAP: return "trap";
        case PTXOpcode::BRKPT: return "brkpt";
        case PTXOpcode::YIELD: return "yield";
        case PTXOpcode::NANOSLEEP: return "nanosleep";

        case PTXOpcode::MOV: return "mov";
        case PTXOpcode::LD: return "ld";
        case PTXOpcode::ST: return "st";
        case PTXOpcode::LDU: return "ldu";
        case PTXOpcode::LDG: return "ldg";
        case PTXOpcode::CVT: return "cvt";
        case PTXOpcode::CVTA: return "cvta";
        case PTXOpcode::ISSPACEP: return "isspacep";
        case PTXOpcode::PREFETCH: return "prefetch";
        case PTXOpcode::PREFETCHU: return "prefetchu";
        case PTXOpcode::PACK: return "pack";
        case PTXOpcode::UNPACK: return "unpack";
        case PTXOpcode::SHFL: return "shfl";
        case PTXOpcode::PRMT: return "prmt";

        case PTXOpcode::SET: return "set";
        case PTXOpcode::SETP: return "setp";
        case PTXOpcode::SELP: return "selp";
        case PTXOpcode::SLCT: return "slct";

        case PTXOpcode::ADD: return "add";
        case PTXOpcode::ADDC: return "addc";
        case PTXOpcode::SUB: return "sub";
        case PTXOpcode::SUBC: return "subc";
        case PTXOpcode::MUL: return "mul";
        case PTXOpcode::MUL24: return "mul24";
        case PTXOpcode::MAD: return "mad";
        case PTXOpcode::MAD24: return "mad24";
        case PTXOpcode::FMA: return "fma";
        case PTXOpcode::SAD: return "sad";
        case PTXOpcode::DIV: return "div";
        case PTXOpcode::REM: return "rem";
        case PTXOpcode::ABS: return "abs";
        case PTXOpcode::NEG: return "neg";
        case PTXOpcode::MIN: return "min";
        case PTXOpcode::MAX: return "max";
        case PTXOpcode::DP4A: return "dp4a";
        case PTXOpcode::DP2A: return "dp2a";

        case PTXOpcode::RCP: return "rcp";
        case PTXOpcode::SQRT: return "sqrt";
        case PTXOpcode::RSQRT: return "rsqrt";
        case PTXOpcode::SIN: return "sin";
        case PTXOpcode::COS: return "cos";
        case PTXOpcode::LG2: return "lg2";
        case PTXOpcode::EX2: return "ex2";
        case PTXOpcode::TANH: return "tanh";

        case PTXOpcode::AND: return "and";
        case PTXOpcode::OR: return "or";
        case PTXOpcode::XOR: return "xor";
        case PTXOpcode::NOT: return "not";
        case PTXOpcode::CNOT: return "cnot";
        case PTXOpcode::SHL: return "shl";
        case PTXOpcode::SHR: return "shr";
        case PTXOpcode::LOP3: return "lop3";
        case PTXOpcode::POPC: return "popc";
        case PTXOpcode::CLZ: return "clz";
        case PTXOpcode::BFIND: return "bfind";
        case PTXOpcode::FNS: return "fns";
        case PTXOpcode::BREV: return "brev";
        case PTXOpcode::BFE: return "bfe";
        case PTXOpcode::BFI: return "bfi";

        case PTXOpcode::BAR: return "bar";
        case PTXOpcode::BARRIER: return "barrier";
        case PTXOpcode::MEMBAR: return "membar";
        case PTXOpcode::FENCE: return "fence";
        case PTXOpcode::ATOM: return "atom";
        case PTXOpcode::RED: return "red";
        case PTXOpcode::REDUX: return "redux";
        case PTXOpcode::VOTE: return "vote";
        case PTXOpcode::MATCH: return "match";
        case PTXOpcode::ACTIVEMASK: return "activemask";
        case PTXOpcode::CP: return "cp";
        case PTXOpcode::MBARRIER: return "mbarrier";

        case PTXOpcode::TEX: return "tex";
        case PTXOpcode::TLD4: return "tld4";
        case PTXOpcode::TXQ: return "txq";
        case PTXOpcode::SULD: return "suld";
        case PTXOpcode::SUST: return "sust";
        case PTXOpcode::SURED: return "sured";
        case PTXOpcode::SUQ: return "suq";

        case PTXOpcode::VPRINTF: return "vprintf";
        case PTXOpcode::PMEVENT: return "pmevent";

        default: return "";
    }
}
std::ostream& operator<<(std::ostream& os, PTXOpcode obj) { return os << REPR(obj); }
