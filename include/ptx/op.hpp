#ifndef PTX_OP_H
#define PTX_OP_H

#include <iostream>
#include <string_view>

enum class PTXOpcode {
    NONE,

    // ==========================================
    // 1. Control Flow & Program Execution
    // ==========================================
    BRA,        // Branch (unconditional or predicated)
    BRX,        // Branch indirect (jump table)
    CALL,       // Call function
    RET,        // Return from function
    EXIT,       // Exit program
    TRAP,       // Abort execution
    BRKPT,      // Breakpoint
    YIELD,      // Yield control (thread scheduling)
    NANOSLEEP,  // Suspend thread for nanoseconds

    // ==========================================
    // 2. Data Movement & Memory
    // ==========================================
    MOV,        // Move data (register to register, or read special registers like %tid)
    LD,         // Load from memory
    ST,         // Store to memory
    LDU,        // Load uniform (bypass L1 cache)
    LDG,        // Load global (read-only data cache)
    CVT,        // Convert type/size
    CVTA,       // Convert address space (e.g., local to generic)
    ISSPACEP,   // Is space pointer (check if pointer points to specific memory space)
    PREFETCH,   // Prefetch line to cache
    PREFETCHU,  // Prefetch uniform line
    PACK,       // Pack data into vectors/matrix
    UNPACK,     // Unpack data
    SHFL,       // Warp shuffle
    PRMT,       // Permute bytes

    // ==========================================
    // 3. Predicate & Comparison
    // ==========================================
    SET,        // Set integer/float based on comparison
    SETP,       // Set predicate register based on comparison
    SELP,       // Select between two values based on predicate
    SLCT,       // Select between two values based on a third being >= 0

    // ==========================================
    // 4. Integer & Floating-Point Arithmetic
    // ==========================================
    ADD,        // Addition
    ADDC,       // Addition with carry
    SUB,        // Subtraction
    SUBC,       // Subtraction with borrow
    MUL,        // Multiplication
    MUL24,      // 24-bit integer multiplication (legacy/optimization)
    MAD,        // Multiply and add (a*b + c)
    MAD24,      // 24-bit multiply and add
    FMA,        // Fused multiply-add (no intermediate rounding)
    SAD,        // Sum of absolute differences
    DIV,        // Division
    REM,        // Remainder (Modulo)
    ABS,        // Absolute value
    NEG,        // Negate
    MIN,        // Minimum
    MAX,        // Maximum
    DP4A,       // 4-element dot product (Int8)
    DP2A,       // 2-element dot product (Int16)

    // ==========================================
    // 5. Extended Math & Transcendentals (Usually fast hardware limits)
    // ==========================================
    RCP,        // Reciprocal (1/x)
    SQRT,       // Square root
    RSQRT,      // Reciprocal square root (1/sqrt(x))
    SIN,        // Sine
    COS,        // Cosine
    LG2,        // Log base 2
    EX2,        // 2 ^ x
    TANH,       // Hyperbolic tangent

    // ==========================================
    // 6. Logic & Bitwise Operations
    // ==========================================
    AND,        // Bitwise AND
    OR,         // Bitwise OR
    XOR,        // Bitwise XOR
    NOT,        // Bitwise NOT
    CNOT,       // C/C++ style logical NOT (!x)
    SHL,        // Shift left
    SHR,        // Shift right
    LOP3,       // Arbitrary 3-input logic operation
    POPC,       // Population count (count number of 1 bits)
    CLZ,        // Count leading zeros
    BFIND,      // Find most significant non-sign bit
    FNS,        // Find nth set bit
    BREV,       // Bit reverse
    BFE,        // Bit field extract
    BFI,        // Bit field insert

    // ==========================================
    // 7. Synchronization & Parallel Communication
    // ==========================================
    BAR,        // Barrier synchronization (warp/block level)
    BARRIER,    // Deprecated alias for BAR
    MEMBAR,     // Memory barrier
    FENCE,      // Generic memory fence
    ATOM,       // Atomic memory operation
    RED,        // Memory reduction operation
    REDUX,      // Warp-level reduction
    VOTE,       // Warp-level vote (all/any/ballot)
    MATCH,      // Warp-level match across threads
    ACTIVEMASK, // Get mask of active threads in warp
    CP,         // Memory copy (e.g., async global to shared)
    MBARRIER,   // Asynchronous memory barrier

    // ==========================================
    // 8. Textures & Surfaces (Graphics / Hardened pipelines)
    // ==========================================
    TEX,        // Texture fetch
    TLD4,       // Texture load 4 texels
    TXQ,        // Texture query
    SULD,       // Surface load
    SUST,       // Surface store
    SURED,      // Surface reduction
    SUQ,        // Surface query

    // ==========================================
    // 9. Misc & System Calls
    // ==========================================
    VPRINTF,    // Variadic print (printf)
    PMEVENT     // Performance monitor event
};

constexpr std::string_view REPR(PTXOpcode obj);
std::ostream& operator<<(std::ostream& os, PTXOpcode obj);

#endif