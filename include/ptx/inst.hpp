#ifndef PTX_INST_H
#define PTX_INST_H

#include <ptx/op.hpp>
#include <ptx/operand.hpp>

#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>

enum class PTXSpace
{
    NONE,
    REG, PARAM, SHARED, GLOBAL, CONST, LOCAL
};
constexpr std::string_view REPR(PTXSpace obj);
std::ostream& operator<<(std::ostream& os, PTXSpace obj);

enum class PTXDataType
{
    NONE,
    B8, B16, B32, B64,
    U8, U16, U32, U64,
    S8, S16, S32, S64,
    F16, F32, F64,
    PRED
};
constexpr std::string_view REPR(PTXDataType obj);
std::ostream& operator<<(std::ostream& os, PTXDataType obj);

enum class PTXWidth
{
    NONE,
    WIDE, LO, HI
};
constexpr std::string_view REPR(PTXWidth obj);
std::ostream& operator<<(std::ostream& os, PTXWidth obj);

enum class PTXComp
{
    NONE,
    EQ, NE, LT, LE, GT, GE, EQU, NEU, LTU, LEU, GTU, GEU,
    NUM, NANX,

    ADD, SUB, MUL, DIV, MOD,
    AND, OR, NOT, XOR, SHL, SHR,
    LAND, LOR, LNOT // Logical (&&, ||, !)
};
constexpr std::string_view REPR(PTXComp obj);
std::ostream& operator<<(std::ostream& os, PTXComp obj);


struct PTXVariable
{
    std::string name;
    PTXDataType type;
    PTXSpace    space;

    int ref_count = 0;

    std::string_view REPR();
    friend std::ostream& operator<<(std::ostream& os, PTXVariable& obj);
};

class PTXStmt
{
public:
    virtual std::string getName() const = 0;
    virtual ~PTXStmt() = default;

    virtual std::string_view REPR() = 0;
    friend std::ostream& operator<<(std::ostream& os, PTXStmt& obj);
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
    std::string_view REPR() override;
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
    std::string_view REPR() override;
};

#endif