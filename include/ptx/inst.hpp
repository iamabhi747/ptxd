#ifndef PTX_INST_H
#define PTX_INST_H

#include <ptx/op.hpp>
#include <ptx/operand.hpp>

#include <string>
#include <vector>
#include <unordered_map>

enum class PTXSpace
{
    NONE,
    REG, PARAM, SHARED, GLOBAL, CONST, LOCAL
};

enum class PTXDataType
{
    NONE,
    B8, B16, B32, B64,
    U8, U16, U32, U64,
    S8, S16, S32, S64,
    F16, F32, F64,
    PRED
};

enum class PTXWidth
{
    NONE,
    WIDE, LO, HI
};

enum class PTXComp
{
    NONE,
    EQ, NE, LT, LE, GT, GE, EQU, NEU, LTU, LEU, GTU, GEU,
    NUM, NANX,

    ADD, SUB, MUL, DIV, MOD,
    AND, OR, NOT, XOR, SHL, SHR,
    LAND, LOR, LNOT // Logical (&&, ||, !)
};



struct PTXVariable
{
    std::string name;
    PTXDataType type;
    PTXSpace    space;

    int ref_count = 0;
};

class PTXStmt
{
public:
    virtual std::string getName() const = 0;
    virtual ~PTXStmt() = default;
};

class PTXInstruction : public PTXStmt
{
public:
    PTXOpcode op = PTXOpcode::NONE;
    std::string predicate;

    // Modifiers
    PTXDataType type  = PTXDataType::NONE;
    PTXSpace    space = PTXSpace::NONE;
    PTXWidth    width = PTXWidth::NONE;
    PTXComp     comp  = PTXComp::NONE; 
    bool   isSaturate = false;
    std::vector<std::string> genericModifiers;

    std::vector<PTXOperand> operands;

    std::string getName() const override { return "Instruction"; };
};

class PTXCallseq : public PTXStmt
{
public:
    std::string name;

    std::vector<std::string> returnParams; 
    std::vector<std::string> arguments;
    std::unordered_map<std::string, PTXVariable> callParameters;

    std::vector<std::unique_ptr<PTXStmt>> rawInstructions;

    std::string getName() const override { return "CallSeq"; };
};

#endif