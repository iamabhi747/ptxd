%{
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <set>

#include "util/logger.h"
#include "cfg.hpp"

extern int yylex();
extern int yylineno;
void yyerror(const char* s)
{
    Logger& log = Logger::getInstance();
    log.loge(1, "Parser error at line", yylineno, ":", s);
}

std::vector<std::unique_ptr<FunctionBlock>> parsedFunctions;

FunctionBlock* currentFunc = nullptr;
BranchBlock* currentBlock = nullptr;
PTXCallseq* currentCallseq = nullptr;

std::set<std::string> specialRegisters = {
    // Thread and Block Identification
    "%tid.x", "%tid.y", "%tid.z",
    "%ntid.x", "%ntid.y", "%ntid.z",
    "%ctaid.x", "%ctaid.y", "%ctaid.z",
    "%nctaid.x", "%nctaid.y", "%nctaid.z",

    // Warp Management and Masking
    "%laneid", "%warpid", "%nwarpid",
    "%lanemask_eq", "%lanemask_le", "%lanemask_lt", 
    "%lanemask_ge", "%lanemask_gt",

    // Hardware and Device Topology
    "%smid", "%nsmid", "%gridid",

    // Timing, Profiling, and Memory State
    "%clock", "%clock64", "%globaltimer",
    "%total_smem_size", "%dynamic_smem_size",

    // Performance Monitoring (pm0 to pm7)
    "%pm0", "%pm1", "%pm2", "%pm3", "%pm4", "%pm5", "%pm6", "%pm7",

    // Environment Registers (envreg0 to envreg31)
    "%envreg0", "%envreg1", "%envreg2", "%envreg3", 
    "%envreg4", "%envreg5", "%envreg6", "%envreg7",
    "%envreg8", "%envreg9", "%envreg10", "%envreg11", 
    "%envreg12", "%envreg13", "%envreg14", "%envreg15",
    "%envreg16", "%envreg17", "%envreg18", "%envreg19", 
    "%envreg20", "%envreg21", "%envreg22", "%envreg23",
    "%envreg24", "%envreg25", "%envreg26", "%envreg27", 
    "%envreg28", "%envreg29", "%envreg30", "%envreg31"
};

PTXOpcode stringToOpcode(const std::string& op)
{
    static const std::unordered_map<std::string, PTXOpcode> opMap = {
        {"bra", PTXOpcode::BRA}, {"brx", PTXOpcode::BRX}, {"call", PTXOpcode::CALL},
        {"ret", PTXOpcode::RET}, {"exit", PTXOpcode::EXIT}, {"trap", PTXOpcode::TRAP},
        {"brkpt", PTXOpcode::BRKPT}, {"yield", PTXOpcode::YIELD}, {"nanosleep", PTXOpcode::NANOSLEEP},
        {"mov", PTXOpcode::MOV}, {"ld", PTXOpcode::LD}, {"st", PTXOpcode::ST},
        {"ldu", PTXOpcode::LDU}, {"ldg", PTXOpcode::LDG}, {"cvt", PTXOpcode::CVT},
        {"cvta", PTXOpcode::CVTA}, {"isspacep", PTXOpcode::ISSPACEP}, {"prefetch", PTXOpcode::PREFETCH},
        {"prefetchu", PTXOpcode::PREFETCHU}, {"pack", PTXOpcode::PACK}, {"unpack", PTXOpcode::UNPACK},
        {"shfl", PTXOpcode::SHFL}, {"prmt", PTXOpcode::PRMT}, {"set", PTXOpcode::SET},
        {"setp", PTXOpcode::SETP}, {"selp", PTXOpcode::SELP}, {"slct", PTXOpcode::SLCT},
        {"add", PTXOpcode::ADD}, {"addc", PTXOpcode::ADDC}, {"sub", PTXOpcode::SUB},
        {"subc", PTXOpcode::SUBC}, {"mul", PTXOpcode::MUL}, {"mul24", PTXOpcode::MUL24},
        {"mad", PTXOpcode::MAD}, {"mad24", PTXOpcode::MAD24}, {"fma", PTXOpcode::FMA},
        {"sad", PTXOpcode::SAD}, {"div", PTXOpcode::DIV}, {"rem", PTXOpcode::REM},
        {"abs", PTXOpcode::ABS}, {"neg", PTXOpcode::NEG}, {"min", PTXOpcode::MIN},
        {"max", PTXOpcode::MAX}, {"dp4a", PTXOpcode::DP4A}, {"dp2a", PTXOpcode::DP2A},
        {"rcp", PTXOpcode::RCP}, {"sqrt", PTXOpcode::SQRT}, {"rsqrt", PTXOpcode::RSQRT},
        {"sin", PTXOpcode::SIN}, {"cos", PTXOpcode::COS}, {"lg2", PTXOpcode::LG2},
        {"ex2", PTXOpcode::EX2}, {"tanh", PTXOpcode::TANH}, {"and", PTXOpcode::AND},
        {"or", PTXOpcode::OR}, {"xor", PTXOpcode::XOR}, {"not", PTXOpcode::NOT},
        {"cnot", PTXOpcode::CNOT}, {"shl", PTXOpcode::SHL}, {"shr", PTXOpcode::SHR},
        {"lop3", PTXOpcode::LOP3}, {"popc", PTXOpcode::POPC}, {"clz", PTXOpcode::CLZ},
        {"bfind", PTXOpcode::BFIND}, {"fns", PTXOpcode::FNS}, {"brev", PTXOpcode::BREV},
        {"bfe", PTXOpcode::BFE}, {"bfi", PTXOpcode::BFI}, {"bar", PTXOpcode::BAR},
        {"barrier", PTXOpcode::BARRIER}, {"membar", PTXOpcode::MEMBAR}, {"fence", PTXOpcode::FENCE},
        {"atom", PTXOpcode::ATOM}, {"red", PTXOpcode::RED}, {"redux", PTXOpcode::REDUX},
        {"vote", PTXOpcode::VOTE}, {"match", PTXOpcode::MATCH}, {"activemask", PTXOpcode::ACTIVEMASK},
        {"cp", PTXOpcode::CP}, {"mbarrier", PTXOpcode::MBARRIER}, {"tex", PTXOpcode::TEX},
        {"tld4", PTXOpcode::TLD4}, {"txq", PTXOpcode::TXQ}, {"suld", PTXOpcode::SULD},
        {"sust", PTXOpcode::SUST}, {"sured", PTXOpcode::SURED}, {"suq", PTXOpcode::SUQ},
        {"vprintf", PTXOpcode::VPRINTF}, {"pmevent", PTXOpcode::PMEVENT}
    };

    auto it = opMap.find(op);
    if (it != opMap.end())
    {
        return it->second;
    }
    return PTXOpcode::NONE; // Fallback
}

PTXDataType stringToType(const std::string& t)
{
    if (t == ".b8") return PTXDataType::B8;
    if (t == ".b16") return PTXDataType::B16;
    if (t == ".b32") return PTXDataType::B32;
    if (t == ".b64") return PTXDataType::B64;
    if (t == ".u8") return PTXDataType::U8;
    if (t == ".u16") return PTXDataType::U16;
    if (t == ".u32") return PTXDataType::U32;
    if (t == ".u64") return PTXDataType::U64;
    if (t == ".s8") return PTXDataType::S8;
    if (t == ".s16") return PTXDataType::S16;
    if (t == ".s32") return PTXDataType::S32;
    if (t == ".s64") return PTXDataType::S64;
    if (t == ".f16") return PTXDataType::F16;
    if (t == ".f32") return PTXDataType::F32;
    if (t == ".f64") return PTXDataType::F64;
    if (t == ".pred") return PTXDataType::PRED;
    return PTXDataType::NONE;
}

void applyModifier(PTXInstruction* inst, const std::string& mod)
{
    PTXDataType dt = stringToType(mod);
    if (dt != PTXDataType::NONE) { inst->type = dt; return; }

    static const std::unordered_map<std::string, PTXSpace> spaceMap = {
        {".global", PTXSpace::GLOBAL}, {".shared", PTXSpace::SHARED}, 
        {".local", PTXSpace::LOCAL}, {".const", PTXSpace::CONST}, 
        {".param", PTXSpace::PARAM}, {".reg", PTXSpace::REG}
    };
    auto spaceIt = spaceMap.find(mod);
    if (spaceIt != spaceMap.end()) { inst->space = spaceIt->second; return; }

    static const std::unordered_map<std::string, PTXWidth> widthMap = {
        {".wide", PTXWidth::WIDE}, {".lo", PTXWidth::LO}, {".hi", PTXWidth::HI}
    };
    auto widthIt = widthMap.find(mod);
    if (widthIt != widthMap.end()) { inst->width = widthIt->second; return; }

    static const std::unordered_map<std::string, PTXComp> compMap = {
        {".eq", PTXComp::EQ}, {".ne", PTXComp::NE}, {".lt", PTXComp::LT}, 
        {".le", PTXComp::LE}, {".gt", PTXComp::GT}, {".ge", PTXComp::GE}, 
        {".equ", PTXComp::EQU}, {".neu", PTXComp::NEU}, {".ltu", PTXComp::LTU}, 
        {".leu", PTXComp::LEU}, {".gtu", PTXComp::GTU}, {".geu", PTXComp::GEU}, 
        {".num", PTXComp::NUM}, {".nan", PTXComp::NANX}
    };
    auto compIt = compMap.find(mod);
    if (compIt != compMap.end()) { inst->comp = compIt->second; return; }

    if (mod == ".sat") { inst->isSaturate = true; return; }

    inst->genericModifiers.push_back(mod);
}
%}

%code requires {
    #include <string>
    #include <vector>
    #include "cfg.hpp"
}

%union {
    std::string* str;
    long long num;
    double fnum;
    std::vector<std::string>* str_list;
    PTXOperand* operand;
    std::vector<PTXOperand>* operand_list;
    PTXInstruction* inst;
    std::vector<PTXVariable>* var_list;
}

%token DIR_VERSION DIR_TARGET DIR_ADDRESS_SIZE
%token DIR_VISIBLE DIR_ENTRY DIR_FUNC DIR_PARAM DIR_REG

%token <str> IDENTIFIER LABEL REGISTER PREDICATE
%token <str> IMM_HEX IMM_FLOAT
%token <num> IMM_INT
%token <fnum> IMM_DEC

%type <str_list> modifiers register_list
%type <str> modifier 
%type <operand> operand
%type <operand_list> operands
%type <inst> instruction
%type <var_list> variable_decl register_decl

%%

module:
    directives functions
    | functions
    ;

directives:
    directive
    | directives directive
    ;

directive:
    DIR_VERSION IMM_DEC
    | DIR_TARGET IDENTIFIER
    | DIR_ADDRESS_SIZE IMM_INT
    ;

functions:
    function
    | functions function
    ;

function:
    DIR_VISIBLE DIR_ENTRY IDENTIFIER 
    {
        currentFunc = new FunctionBlock();
        currentFunc->name = *$3;
        currentFunc->isKernel = true;
        currentBlock = new BranchBlock(0, "entry");
        currentFunc->entryBlock = currentBlock;
        currentFunc->blocks.push_back(std::unique_ptr<BranchBlock>(currentBlock));
    }
    '(' func_params ')' '{' func_body '}'
    {
        parsedFunctions.push_back(std::unique_ptr<FunctionBlock>(currentFunc));
        currentFunc = nullptr;
        currentBlock = nullptr;
        delete $3;
    }
    | DIR_FUNC 
    {
        currentFunc = new FunctionBlock();
        currentFunc->isKernel = false;
    }
    '(' func_returns ')' IDENTIFIER 
    {
        currentFunc->name = *$6;
        currentBlock = new BranchBlock(0, "entry");
        currentFunc->entryBlock = currentBlock;
        currentFunc->blocks.push_back(std::unique_ptr<BranchBlock>(currentBlock));
    }
    '(' func_params ')' '{' func_body '}'
    {
        parsedFunctions.push_back(std::unique_ptr<FunctionBlock>(currentFunc));
        currentFunc = nullptr;
        currentBlock = nullptr;
        delete $6;
    }
    ;

func_returns:
    /* empty */
    | DIR_PARAM modifiers IDENTIFIER
    {
        PTXVariable var;
        var.name = *$3;
        var.space = PTXSpace::PARAM;
        for (auto& m : *$2)
        {
            PTXDataType dt = stringToType(m);
            if (dt != PTXDataType::NONE) var.type = dt;
        }
        currentFunc->returnVariables.push_back(var);
        delete $2; delete $3;
    }
    ;

func_params:
    /* empty */
    | func_param
    | func_params ',' func_param
    ;

func_param:
    DIR_PARAM modifiers IDENTIFIER
    {
        PTXVariable var;
        var.name = *$3;
        var.space = PTXSpace::PARAM;
        for (auto& m : *$2)
        {
            PTXDataType dt = stringToType(m);
            if (dt != PTXDataType::NONE) var.type = dt;
        }
        currentFunc->parameters[var.name] = var;
        delete $2; delete $3;
    }
    ;

func_body:
    /* empty */
    | func_body statement
    ;

statement:
    variable_decl
    {
        if (currentCallseq) {
            for (auto& v : *$1) currentCallseq->callParameters[v.name] = v;
        } else {
            for (auto& v : *$1) currentFunc->registers[v.name] = v; 
        }
        delete $1;
    }
    | instruction
    {
        if (currentCallseq)
        {
            // Check for CALL
            if ($1->op == PTXOpcode::CALL)
            {
               currentCallseq->name = "call"; // Store specifics
            }
            currentCallseq->rawInstructions.push_back(*$1);
        } else {
            currentBlock->statements.push_back(std::unique_ptr<PTXStmt>($1));
        }
    }
    | LABEL ':'
    {
        BranchBlock* newBlock = new BranchBlock(currentFunc->blocks.size(), *$1);
        currentFunc->blocks.push_back(std::unique_ptr<BranchBlock>(newBlock));
        // Add explicit edge from previous
        currentBlock->successors.push_back({newBlock, false});
        newBlock->predecessors.push_back({currentBlock, false});
        currentBlock = newBlock;
        delete $1;
    }
    | '{' 
    {
        PTXCallseq* cs = new PTXCallseq();
        currentCallseq = cs;
    }
    callseq_body '}'
    {
        currentBlock->statements.push_back(std::unique_ptr<PTXStmt>(currentCallseq));
        currentCallseq = nullptr;
    }
    ;

callseq_body:
    /* empty */
    | callseq_body statement
    ;

variable_decl:
    DIR_REG modifiers register_list ';'
    {
        $$ = new std::vector<PTXVariable>();
        PTXDataType dt = PTXDataType::NONE;
        for (auto& m : *$2)
        {
            PTXDataType temp = stringToType(m);
            if (temp != PTXDataType::NONE) dt = temp;
        }
        for (auto& r : *$3)
        {
            PTXVariable v; v.name = r; v.type = dt; v.space = PTXSpace::REG;
            $$->push_back(v);
        }
        delete $2; delete $3;
    }
    | DIR_PARAM modifiers IDENTIFIER ';' // for callseq local param
    {
        $$ = new std::vector<PTXVariable>();
        PTXDataType dt = PTXDataType::NONE;
        for (auto& m : *$2)
        {
            PTXDataType temp = stringToType(m);
            if (temp != PTXDataType::NONE) dt = temp;
        }
        PTXVariable v; v.name = *$3; v.type = dt; v.space = PTXSpace::PARAM;
        $$->push_back(v);
        delete $2; delete $3;
    }
    ;

register_list:
    register_decl { 
        $$ = new std::vector<std::string>(); 
        for(auto& v : *$1) $$->push_back(v.name);
        delete $1;
    }
    | register_list ',' register_decl {
        $$ = $1;
        for(auto& v : *$3) $$->push_back(v.name);
        delete $3;
    }
    ;

register_decl:
    REGISTER
    {
        $$ = new std::vector<PTXVariable>();
        PTXVariable v; v.name = *$1;
        $$->push_back(v);
        delete $1;
    }
    | IDENTIFIER
    {
        $$ = new std::vector<PTXVariable>();
        PTXVariable v; v.name = *$1;
        $$->push_back(v);
        delete $1;
    }
    | REGISTER '<' IMM_INT '>'
    {
        $$ = new std::vector<PTXVariable>();
        for (int i=1; i<$3; ++i)
        {
            PTXVariable v; v.name = *$1 + std::to_string(i);
            $$->push_back(v);
        }
        delete $1;
    }
    | IDENTIFIER '<' IMM_INT '>'
    {
        $$ = new std::vector<PTXVariable>();
        for (int i=1; i<$3; ++i)
        {
            PTXVariable v; v.name = *$1 + std::to_string(i);
            $$->push_back(v);
        }
        delete $1;
    }
    ;

instruction:
    IDENTIFIER modifiers operands ';'
    {
        $$ = new PTXInstruction();
        $$->op = stringToOpcode(*$1);
        for (auto& m : *$2) applyModifier($$, m);
        if ($3) { $$->operands = *$3; delete $3; }
        delete $1; delete $2;
    }
    | PREDICATE IDENTIFIER modifiers operands ';'
    {
        $$ = new PTXInstruction();
        $$->predicate = *$1;
        $$->op = stringToOpcode(*$2);
        for (auto& m : *$3) applyModifier($$, m);
        if ($4) { $$->operands = *$4; delete $4; }
        delete $1; delete $2; delete $3;
    }
    ;

modifiers:
    /* empty */ { $$ = new std::vector<std::string>(); }
    | modifiers modifier { $$ = $1; $$->push_back(*$2); delete $2; }
    ;

modifier:
    '.' IDENTIFIER { $$ = new std::string("." + *$2); delete $2; }
    | DIR_PARAM { $$ = new std::string(".param"); }
    | DIR_REG { $$ = new std::string(".reg"); }
    | DIR_VISIBLE { $$ = new std::string(".visible"); }
    | DIR_ENTRY { $$ = new std::string(".entry"); }
    | DIR_FUNC { $$ = new std::string(".func"); }
    | DIR_VERSION { $$ = new std::string(".version"); }
    | DIR_TARGET { $$ = new std::string(".target"); }
    | DIR_ADDRESS_SIZE { $$ = new std::string(".address_size"); }
    ;

operands:
    /* empty */ { $$ = new std::vector<PTXOperand>(); }
    | operand { $$ = new std::vector<PTXOperand>(); $$->push_back(*$1); delete $1; }
    | operands ',' operand { $$ = $1; $$->push_back(*$3); delete $3; }
    ;

operand:
    REGISTER {
        $$ = new PTXOperand(); $$->type = OperandType::REGISTER; 
        $$->name = *$1; $$->raw = *$1;
        if (specialRegisters.contains(*$1)) $$->type = OperandType::SPECIAL_REGISTER;
        delete $1;
    }
    | IMM_INT {
        $$ = new PTXOperand(); $$->type = OperandType::IMMEDIATE_INT;
        $$->immInt = $1; $$->raw = std::to_string($1);
    }
    | IMM_HEX {
        $$ = new PTXOperand(); $$->type = OperandType::IMMEDIATE_INT;
        $$->immInt = std::stoull(*$1, nullptr, 16); $$->raw = *$1; delete $1;
    }
    | IMM_FLOAT {
        $$ = new PTXOperand(); $$->type = OperandType::IMMEDIATE_FLOAT;
        $$->raw = *$1; delete $1;
    }
    | LABEL {
        $$ = new PTXOperand(); $$->type = OperandType::LABEL;
        $$->name = *$1; $$->raw = *$1; delete $1;
    }
    | IDENTIFIER { // Labels without $, e.g. function calls
        $$ = new PTXOperand(); $$->type = OperandType::LABEL;
        $$->name = *$1; $$->raw = *$1; delete $1;
    }
    | '[' REGISTER ']' {
        $$ = new PTXOperand(); $$->type = OperandType::MEMORY;
        $$->baseReg = *$2; $$->offset = 0; $$->raw = "[" + *$2 + "]"; delete $2;
    }
    | '[' IDENTIFIER ']' {
        $$ = new PTXOperand(); $$->type = OperandType::MEMORY;
        $$->name = *$2; $$->offset = 0; $$->raw = "[" + *$2 + "]"; delete $2;
    }
    | '[' REGISTER '+' IMM_INT ']' {
        $$ = new PTXOperand(); $$->type = OperandType::MEMORY;
        $$->baseReg = *$2; $$->offset = $4;
        $$->raw = "[" + *$2 + "+" + std::to_string($4) + "]"; delete $2;
    }
    | '[' IDENTIFIER '+' IMM_INT ']' {
        $$ = new PTXOperand(); $$->type = OperandType::MEMORY;
        $$->name = *$2; $$->offset = $4;
        $$->raw = "[" + *$2 + "+" + std::to_string($4) + "]"; delete $2;
    }
    | '(' IDENTIFIER ')' {
        $$ = new PTXOperand(); $$->type = OperandType::LABEL;
        $$->name = *$2; $$->raw = "(" + *$2 + ")"; delete $2;
    }
    | '(' REGISTER ')' {
        $$ = new PTXOperand(); $$->type = OperandType::REGISTER;
        $$->name = *$2; $$->raw = "(" + *$2 + ")"; delete $2;
    }
    ;

%%

