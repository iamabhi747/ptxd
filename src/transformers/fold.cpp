#include <ast.hpp>
#include <transformer.h>
#include <util/logger.h>

#include <set>
#include <algorithm>

class FoldTransformer : public Transformer
{
private:
    std::unordered_map<std::string, std::unique_ptr<PTXStmt>> foldables;

public:
    std::string getName() const override { return "Fold"; }

    bool runFunc(std::unique_ptr<FunctionBlock>& func) override
    {
        foldables.clear();
        Transformer::runFunc(func);
        foldables.clear();
        return true;
    }

    bool runStmt(std::unique_ptr<PTXStmt>& stmt) override
    {
        if (stmt->getName() == "Instruction")
        {
            auto inst = dynamic_cast<PTXInstruction*>(stmt.get());
            std::vector<std::unique_ptr<PTXStmt>> args;

            switch (inst->op)
            {
                // Control Flow & Program Execution
                case PTXOpcode::BRA:        // Branch (unconditional or predicated)
                    break;
                case PTXOpcode::BRX:        // Branch indirect (jump table)
                    break;
                case PTXOpcode::CALL:       // Call function
                    break;
                case PTXOpcode::RET:        // Return from function
                    foldInplace(stmt, std::make_unique<ReturnNode>());
                    break;
                case PTXOpcode::EXIT:       // Exit program
                    args.push_back(std::make_unique<IntLiteral>(PTXDataType::U32, 0));
                    foldInplace(stmt, std::make_unique<FunctionCall>("exit", nullptr, std::move(args)));
                    break;
                case PTXOpcode::TRAP:       // Abort execution
                    foldInplace(stmt, std::make_unique<FunctionCall>("abort", nullptr, std::move(args)));
                    break;
                case PTXOpcode::BRKPT:      // Breakpoint
                    foldInplace(stmt, std::make_unique<FunctionCall>("__debugbreak", nullptr, std::move(args)));
                    break;
                case PTXOpcode::YIELD:      // Yield control (thread scheduling)
                    foldInplace(stmt, std::make_unique<FunctionCall>("sched_yield", nullptr, std::move(args)));
                    break;
                case PTXOpcode::NANOSLEEP:  // Suspend thread for nanoseconds
                    if (!assertOperands(inst, {1})) return false;
                    args.push_back(foldOperand(inst->operands[0]));
                    foldInplace(stmt, std::make_unique<FunctionCall>("__nanosleep", nullptr, std::move(args)));
                    break;


                // Data Movement & Memory
                case PTXOpcode::MOV:        // Move data (register to register, or read special registers like %tid)
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, foldOperand(inst->operands[1]));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), foldOperand(inst->operands[1])));
                    break;
                case PTXOpcode::LD:         // Load from memory
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::MEMORY}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, foldOperand(inst->operands[1]));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), foldOperand(inst->operands[1])));
                    break;
                case PTXOpcode::ST:         // Store to memory
                    if (!assertOperands(inst, {2}, {{OperandType::MEMORY,OperandType::NONE}})) return false;
                    foldInplace(stmt, std::make_unique<AssignmentNode>(foldOperand(inst->operands[0]), foldOperand(inst->operands[1])));
                    break;
                case PTXOpcode::LDU:        // Load uniform (bypass L1 cache)
                case PTXOpcode::LDG:        // Load global (read-only data cache)
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::MEMORY}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("__ldg", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("__ldg", nullptr, std::move(args))));
                    break;
                case PTXOpcode::CVT:        // Convert type/size
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::REGISTER}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<TypeCastNode>(inst->type, foldOperand(inst->operands[1])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<TypeCastNode>(inst->type, foldOperand(inst->operands[1]))));
                    break;
                case PTXOpcode::CVTA:       // Convert address space (e.g., local to generic)
                {
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::REGISTER}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("cvta", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("cvta", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::ISSPACEP:   // Is space pointer (check if pointer points to specific memory space)
                {
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::REGISTER}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("isspacep", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("isspacep", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::PREFETCH:   // Prefetch line to cache
                case PTXOpcode::PREFETCHU:  // Prefetch uniform line
                {
                    if (!assertOperands(inst, {1})) return false;
                    std::string func_name = (inst->op == PTXOpcode::PREFETCH) ? "__prefetch" : "__prefetchu";
                    args.push_back(foldOperand(inst->operands[0]));
                    foldInplace(stmt, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                    break;
                }
                case PTXOpcode::PACK:       // Pack data into vectors/matrix
                case PTXOpcode::UNPACK:     // Unpack data
                {
                    std::string func_name = (inst->op == PTXOpcode::PACK) ? "pack" : "unpack";
                    for (size_t i = 1; i < inst->operands.size(); i++) args.push_back(foldOperand(inst->operands[i]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>(func_name, nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::SHFL:       // Warp shuffle
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[3])); // mask
                    args.push_back(foldOperand(inst->operands[1])); // src
                    args.push_back(foldOperand(inst->operands[2])); // lane
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("__shfl_sync", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("__shfl_sync", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::PRMT:       // Permute bytes
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    args.push_back(foldOperand(inst->operands[3]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("__byte_perm", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("__byte_perm", nullptr, std::move(args))));
                    break;
                }


                // Predicate & Comparison
                case PTXOpcode::SET:        // Set integer/float based on comparison
                {
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    auto cmpOp = std::make_unique<BinaryOPNode>(inst->comp, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]));
                    auto truthy = std::make_unique<IntLiteral>(PTXDataType::B32, 1);
                    auto falsy = std::make_unique<IntLiteral>(PTXDataType::B32, 0);
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<TernaryOPNode>(std::move(cmpOp), std::move(truthy), std::move(falsy)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<TernaryOPNode>(std::move(cmpOp), std::move(truthy), std::move(falsy))));
                    break;
                }
                case PTXOpcode::SETP:       // Set predicate register based on comparison
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(inst->comp, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(inst->comp, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::SELP:       // Select between two values based on predicate
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::REGISTER}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<TernaryOPNode>(foldOperand(inst->operands[3]), foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<TernaryOPNode>(foldOperand(inst->operands[3]), foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::SLCT:       // Select between two values based on a third being >= 0
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    auto cmpOp = std::make_unique<BinaryOPNode>(PTXComp::GE, foldOperand(inst->operands[3]), std::make_unique<IntLiteral>(PTXDataType::S32, 0));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<TernaryOPNode>(std::move(cmpOp), foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<TernaryOPNode>(std::move(cmpOp), foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                }


                // Integer & Floating-Point Arithmetic
                case PTXOpcode::ADD:        // Addition
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::ADD, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::ADD, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::ADDC:       // Addition with carry
                {
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    auto addOp = std::make_unique<BinaryOPNode>(PTXComp::ADD, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(addOp), std::make_unique<VariableNode>("carry")));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(addOp), std::make_unique<VariableNode>("carry"))));
                    break;
                }
                case PTXOpcode::SUB:        // Subtraction
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::SUB, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::SUB, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::SUBC:       // Subtraction with borrow
                {
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    auto subOp = std::make_unique<BinaryOPNode>(PTXComp::SUB, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::SUB, std::move(subOp), std::make_unique<VariableNode>("borrow")));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::SUB, std::move(subOp), std::make_unique<VariableNode>("borrow"))));
                    break;
                }
                case PTXOpcode::MUL:        // Multiplication
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::MUL, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::MUL, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::MUL24:      // 24-bit integer multiplication (legacy/optimization)
                {
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("__mul24", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("__mul24", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::MAD:        // Multiply and add (a*b + c)
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    auto mulOp = std::make_unique<BinaryOPNode>(PTXComp::MUL, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(mulOp), foldOperand(inst->operands[3])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(mulOp), foldOperand(inst->operands[3]))));
                    break;
                }
                case PTXOpcode::MAD24:      // 24-bit multiply and add
                case PTXOpcode::FMA:        // Fused multiply-add (no intermediate rounding)
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    std::string func_name = (inst->op == PTXOpcode::MAD24) ? "__mad24" : "fma";
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    args.push_back(foldOperand(inst->operands[3]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>(func_name, nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::SAD:        // Sum of absolute differences
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    auto subOp = std::make_unique<BinaryOPNode>(PTXComp::SUB, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]));
                    args.push_back(std::move(subOp));
                    auto absCall = std::make_unique<FunctionCall>("abs", nullptr, std::move(args));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(absCall), foldOperand(inst->operands[3])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(absCall), foldOperand(inst->operands[3]))));
                    break;
                }
                case PTXOpcode::DIV:        // Division
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::DIV, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::DIV, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::REM:        // Remainder (Modulo)
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::MOD, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::MOD, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::NEG:        // Negate
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<UnaryOPNode>(PTXComp::SUB, foldOperand(inst->operands[1])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<UnaryOPNode>(PTXComp::SUB, foldOperand(inst->operands[1]))));
                    break;
                case PTXOpcode::MIN:        // Minimum
                case PTXOpcode::MAX:        // Maximum
                {
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    std::string func_name = (inst->op == PTXOpcode::MIN) ? "min" : "max";
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>(func_name, nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::DP4A:       // 4-element dot product (Int8)
                case PTXOpcode::DP2A:       // 2-element dot product (Int16)
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    std::string func_name = (inst->op == PTXOpcode::DP4A) ? "dot4" : "dot2";
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    auto dotOp = std::make_unique<FunctionCall>(func_name, nullptr, std::move(args));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(dotOp), foldOperand(inst->operands[3])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::ADD, std::move(dotOp), foldOperand(inst->operands[3]))));
                    break;
                }


                // Extended Math & Transcendentals
                case PTXOpcode::RCP:        // Reciprocal (1/x)
                case PTXOpcode::SQRT:       // Square root
                case PTXOpcode::RSQRT:      // Reciprocal square root (1/sqrt(x))
                case PTXOpcode::SIN:        // Sine
                case PTXOpcode::COS:        // Cosine
                case PTXOpcode::LG2:        // Log base 2
                case PTXOpcode::EX2:        // 2 ^ x
                case PTXOpcode::TANH:       // Hyperbolic tangent
                case PTXOpcode::ABS:        // Absolute value
                case PTXOpcode::CLZ:        // Count leading zeros
                case PTXOpcode::POPC:       // Population count (count number of 1 bits)
                case PTXOpcode::BREV:       // Bit reverse
                case PTXOpcode::BFIND:      // Find most significant non-sign bit
                {
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::NONE}})) return false;
                    std::string func_name = "";
                    if (inst->op == PTXOpcode::RCP) func_name = "rcp";
                    else if (inst->op == PTXOpcode::SQRT) func_name = "sqrt";
                    else if (inst->op == PTXOpcode::RSQRT) func_name = "rsqrt";
                    else if (inst->op == PTXOpcode::SIN) func_name = "sin";
                    else if (inst->op == PTXOpcode::COS) func_name = "cos";
                    else if (inst->op == PTXOpcode::LG2) func_name = "log2";
                    else if (inst->op == PTXOpcode::EX2) func_name = "exp2";
                    else if (inst->op == PTXOpcode::TANH) func_name = "tanh";
                    else if (inst->op == PTXOpcode::ABS) func_name = "abs";
                    else if (inst->op == PTXOpcode::CLZ) func_name = "__clz";
                    else if (inst->op == PTXOpcode::POPC) func_name = "__popc";
                    else if (inst->op == PTXOpcode::BREV) func_name = "__brev";
                    else if (inst->op == PTXOpcode::BFIND) func_name = "msb";
                    
                    args.push_back(foldOperand(inst->operands[1]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>(func_name, nullptr, std::move(args))));
                    break;
                }


                // Logic & Bitwise Operations
                case PTXOpcode::AND:        // Bitwise AND
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::AND, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::AND, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::OR:         // Bitwise OR
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::OR, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::OR, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::XOR:        // Bitwise XOR
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::XOR, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::XOR, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::NOT:        // Bitwise NOT
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<UnaryOPNode>(PTXComp::NOT, foldOperand(inst->operands[1])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<UnaryOPNode>(PTXComp::NOT, foldOperand(inst->operands[1]))));
                    break;
                case PTXOpcode::CNOT:       // C/C++ style logical NOT (!x)
                    if (!assertOperands(inst, {2}, {{OperandType::REGISTER,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<UnaryOPNode>(PTXComp::LNOT, foldOperand(inst->operands[1])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<UnaryOPNode>(PTXComp::LNOT, foldOperand(inst->operands[1]))));
                    break;
                case PTXOpcode::SHL:        // Shift left
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::SHL, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::SHL, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::SHR:        // Shift right
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<BinaryOPNode>(PTXComp::SHR, foldOperand(inst->operands[1]), foldOperand(inst->operands[2])));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<BinaryOPNode>(PTXComp::SHR, foldOperand(inst->operands[1]), foldOperand(inst->operands[2]))));
                    break;
                case PTXOpcode::LOP3:       // Arbitrary 3-input logic operation
                {
                    if (!assertOperands(inst, {5}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    args.push_back(foldOperand(inst->operands[3]));
                    args.push_back(foldOperand(inst->operands[4]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("lop3", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("lop3", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::FNS:        // Find nth set bit
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    args.push_back(foldOperand(inst->operands[3]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("find_nth_set", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("find_nth_set", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::BFE:        // Bit field extract
                {
                    if (!assertOperands(inst, {4}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    args.push_back(foldOperand(inst->operands[3]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("extract", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("extract", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::BFI:        // Bit field insert
                {
                    if (!assertOperands(inst, {5}, {{OperandType::REGISTER,OperandType::NONE,OperandType::NONE,OperandType::NONE,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1])); // a
                    args.push_back(foldOperand(inst->operands[2])); // b
                    args.push_back(foldOperand(inst->operands[3])); // pos
                    args.push_back(foldOperand(inst->operands[4])); // len
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("insert", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("insert", nullptr, std::move(args))));
                    break;
                }


                // Synchronization & Parallel Communication
                case PTXOpcode::BAR:        // Barrier synchronization (warp/block level)
                case PTXOpcode::BARRIER:    // Deprecated alias for BAR
                    foldInplace(stmt, std::make_unique<FunctionCall>("__syncthreads", nullptr, std::move(args)));
                    break;
                case PTXOpcode::MEMBAR:     // Memory barrier
                    foldInplace(stmt, std::make_unique<FunctionCall>("__threadfence", nullptr, std::move(args)));
                    break;
                case PTXOpcode::FENCE:      // Generic memory fence
                    foldInplace(stmt, std::make_unique<FunctionCall>("cuda::atomic_thread_fence", nullptr, std::move(args)));
                    break;
                case PTXOpcode::ATOM:       // Atomic memory operation
                {
                    if (!assertOperands(inst, {3}, {{OperandType::REGISTER,OperandType::MEMORY,OperandType::NONE}})) return false;
                    args.push_back(foldOperand(inst->operands[1]));
                    args.push_back(foldOperand(inst->operands[2]));
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("atomicAdd", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("atomicAdd", nullptr, std::move(args))));
                    break;
                }
                case PTXOpcode::RED:        // Memory reduction operation
                case PTXOpcode::REDUX:      // Warp-level reduction
                case PTXOpcode::VOTE:       // Warp-level vote (all/any/ballot)
                case PTXOpcode::MATCH:      // Warp-level match across threads
                case PTXOpcode::CP:         // Memory copy (e.g., async global to shared)
                case PTXOpcode::MBARRIER:   // Asynchronous memory barrier
                case PTXOpcode::TEX:        // Texture fetch
                case PTXOpcode::TLD4:       // Texture load 4 texels
                case PTXOpcode::TXQ:        // Texture query
                case PTXOpcode::SULD:       // Surface load
                case PTXOpcode::SUST:       // Surface store
                case PTXOpcode::SURED:      // Surface reduction
                case PTXOpcode::SUQ:        // Surface query
                {
                    std::string func_name = "";
                    if (inst->op == PTXOpcode::RED) func_name = "reduction";
                    else if (inst->op == PTXOpcode::REDUX) func_name = "redux";
                    else if (inst->op == PTXOpcode::VOTE) func_name = "vote";
                    else if (inst->op == PTXOpcode::MATCH) func_name = "match";
                    else if (inst->op == PTXOpcode::CP) func_name = "memcpy_async";
                    else if (inst->op == PTXOpcode::MBARRIER) func_name = "mbarrier";
                    else if (inst->op == PTXOpcode::TEX) func_name = "tex";
                    else if (inst->op == PTXOpcode::TLD4) func_name = "tld4";
                    else if (inst->op == PTXOpcode::TXQ) func_name = "txq";
                    else if (inst->op == PTXOpcode::SULD) func_name = "suld";
                    else if (inst->op == PTXOpcode::SUST) func_name = "sust";
                    else if (inst->op == PTXOpcode::SURED) func_name = "sured";
                    else if (inst->op == PTXOpcode::SUQ) func_name = "suq";
                    
                    int start_idx = (inst->operands.size() > 0 && inst->operands[0].type == OperandType::REGISTER) ? 1 : 0;
                    for (size_t i = start_idx; i < inst->operands.size(); i++) args.push_back(foldOperand(inst->operands[i]));
                    
                    if (start_idx == 1) {
                        if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                        else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>(func_name, nullptr, std::move(args))));
                    } else {
                        foldInplace(stmt, std::make_unique<FunctionCall>(func_name, nullptr, std::move(args)));
                    }
                    break;
                }
                case PTXOpcode::ACTIVEMASK: // Get mask of active threads in warp
                {
                    if (!assertOperands(inst, {1}, {{OperandType::REGISTER}})) return false;
                    if (isFoldable(inst->operands[0].name)) foldOut(stmt, inst->operands[0].name, std::make_unique<FunctionCall>("__activemask", nullptr, std::move(args)));
                    else foldInplace(stmt, std::make_unique<AssignmentNode>(std::make_unique<RegisterNode>(inst->operands[0].name), std::make_unique<FunctionCall>("__activemask", nullptr, std::move(args))));
                    break;
                }


                // Misc & System Calls
                case PTXOpcode::VPRINTF:    // Variadic print (printf)
                {
                    for (size_t i = 0; i < inst->operands.size(); i++) args.push_back(foldOperand(inst->operands[i]));
                    foldInplace(stmt, std::make_unique<FunctionCall>("vprintf", nullptr, std::move(args)));
                    break;
                }
                case PTXOpcode::PMEVENT:    // Performance monitor event
                {
                    if (!assertOperands(inst, {1})) return false;
                    args.push_back(foldOperand(inst->operands[0]));
                    foldInplace(stmt, std::make_unique<FunctionCall>("__prof_trigger", nullptr, std::move(args)));
                    break;
                }

                default:
                    log.loge(1, "Unknwn Instruction Opcode!");
                    return false;
                    break;
            }
        }

        return true;
    }

    void foldInplace(std::unique_ptr<PTXStmt>& stmt, std::unique_ptr<PTXStmt> newstmt)
    {
        stmt.reset();
        stmt = std::move(newstmt);
    }

    void foldOut(std::unique_ptr<PTXStmt>& stmt, std::string reg, std::unique_ptr<PTXStmt> foldedVal)
    {
        if (foldables.contains(reg))
        {
            log.logw(2, "Foldable register already exists! invalid SSA.");
        }

        foldables[reg] = std::move(foldedVal);
        stmt.reset();
    }

    std::unique_ptr<PTXStmt> foldOperand(const PTXOperand& operand)
    {
        if (operand.type == OperandType::REGISTER)
        {
            if (foldables.contains(operand.name))
            {
                if (foldables[operand.name] == nullptr)
                {
                    log.loge(1, "Tried to access folded value again.");
                    return nullptr;
                }

                return std::move(foldables[operand.name]);
            }
            else
            {
                return std::make_unique<RegisterNode>(operand.name);
            }
        }
        else if (operand.type == OperandType::SPECIAL_REGISTER)
        {
            return std::make_unique<VariableNode>(operand.name);
        }
        else if (operand.type == OperandType::MEMORY)
        {
            if (!operand.baseReg.empty())
            {
                if (foldables.contains(operand.baseReg))
                {
                    if (foldables[operand.baseReg] == nullptr)
                    {
                        log.loge(1, "Tried to access folded value again.");
                        return nullptr;
                    }

                    return std::make_unique<AddressRefNode>(std::move(foldables[operand.baseReg]), operand.offset);
                }

                return std::make_unique<AddressRefNode>(std::make_unique<RegisterNode>(operand.baseReg), operand.offset);
            }
            else if (!operand.name.empty())
            {
                if (!curFunc->parameters.contains(operand.name) && !std::ranges::any_of(curFunc->returnVariables, [operand](const PTXVariable& obj) {return obj.name == operand.name;}))
                {
                    log.loge(1, "Invalid Label (", operand.name, ")");
                    return nullptr;
                }

                return std::make_unique<AddressRefNode>(std::make_unique<VariableNode>(operand.name), operand.offset);
            }
            else
            {
                log.loge(1, "Invalid Operand of type MEMORY. both baseReg & name are missing!");
                return nullptr;
            }
        }
        else if (operand.type == OperandType::IMMEDIATE_INT)
        {
            return std::make_unique<IntLiteral>(PTXDataType::S64, operand.immInt);
        }
        else if (operand.type == OperandType::IMMEDIATE_FLOAT)
        {
            return std::make_unique<FloatLiteral>(PTXDataType::F64, operand.immFloat);
        }
        else if (operand.type == OperandType::LABEL)
        {
            // Can't fold Label!!
            // need to handle in instrcuton only
            log.logw(1, "Called foldOperand on LABEL.");
            return nullptr;
        }

        log.logw(1, "Called foldOperand on Unknown Type. (", (int)operand.type, ")");
        return nullptr;
    }

    bool assertOperands(PTXInstruction* inst, std::set<size_t> expected_lengths, std::vector<std::vector<OperandType>> ordered_types = {}, bool inverseMatch = false)
    {
        if (!expected_lengths.contains(inst->operands.size()))
        {
            log.loge(1, "Unexpected length of operands. (", inst->operands.size(), ") expecting {<TODO: REPR Set>}");
            return false;
        }

        if (ordered_types.size() == 0) return true;

        for (const auto& order : ordered_types)
        {
            if (order.size() != inst->operands.size()) continue;
            bool failed = false; 

            for (int i = 0; i < order.size(); i++)
            {
                if (order[i] == OperandType::NONE) continue;

                if ((inverseMatch && order[i] == inst->operands[i].type) || (!inverseMatch && order[i] != inst->operands[i].type))
                {
                    failed = true;
                    break;
                }
            }

            if (!failed) return true;
        }

        log.loge(1, "Constraint on operands failed. <TODO: REPR inst> ", (int)inst->op);
        for (const auto& operand : inst->operands)
        {
            log.logw(1, (int)operand.type, " - ", operand.name);
        }
        return false;
    }

    bool isFoldable(std::string reg)
    {
        if (curFunc->registers.contains(reg))
        {
            return curFunc->registers[reg].ref_count == 1;
        }
        else
        {
            log.logw(2, "Requesting invalid register (", reg, ")");
            return false;
        }
    }
};

static struct FoldRegister
{
    FoldRegister()
    {
        Transformer::registerTransformer("fold", 20, []() {
            return std::make_unique<FoldTransformer>();
        });
    }
} register_fold;