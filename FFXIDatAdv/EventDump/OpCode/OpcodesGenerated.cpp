// Auto-generated from Python opcode definitions. DO NOT EDIT.

#include "OpCode.h"

// Variadic length calculation functions

static size_t VariadicLength_0x02(std::span<const uint8_t>, size_t)
{
    return 8; // IF_CONDITIONAL
}

static size_t VariadicLength_0x07(std::span<const uint8_t>, size_t)
{
    return 5; // ADD_VALUES
}

static size_t VariadicLength_0x08(std::span<const uint8_t>, size_t)
{
    return 5; // SUBTRACT_VALUES
}

static size_t VariadicLength_0x09(std::span<const uint8_t>, size_t)
{
    return 5; // SET_BIT_FLAG
}

static size_t VariadicLength_0x0A(std::span<const uint8_t>, size_t)
{
    return 5; // CLEAR_BIT_FLAG
}

static size_t VariadicLength_0x0B(std::span<const uint8_t>, size_t)
{
    return 3; // INCREMENT_VALUE
}

static size_t VariadicLength_0x0C(std::span<const uint8_t>, size_t)
{
    return 3; // DECREMENT_VALUE
}

static size_t VariadicLength_0x0D(std::span<const uint8_t>, size_t)
{
    return 5; // BITWISE_AND
}

static size_t VariadicLength_0x0E(std::span<const uint8_t>, size_t)
{
    return 5; // BITWISE_OR
}

static size_t VariadicLength_0x0F(std::span<const uint8_t>, size_t)
{
    return 5; // BITWISE_XOR
}

static size_t VariadicLength_0x10(std::span<const uint8_t>, size_t)
{
    return 5; // BITWISE_LEFT_SHIFT
}

static size_t VariadicLength_0x11(std::span<const uint8_t>, size_t)
{
    return 5; // BITWISE_RIGHT_SHIFT
}

static size_t VariadicLength_0x15(std::span<const uint8_t>, size_t)
{
    return 5; // DIVIDE_VALUES
}

static size_t VariadicLength_0x16(std::span<const uint8_t>, size_t)
{
    return 7; // SINE_CALCULATION
}

static size_t VariadicLength_0x17(std::span<const uint8_t>, size_t)
{
    return 7; // COSINE_CALCULATION
}

static size_t VariadicLength_0x18(std::span<const uint8_t>, size_t)
{
    return 7; // ATAN2_CALCULATION
}

static size_t VariadicLength_0x19(std::span<const uint8_t>, size_t)
{
    return 5; // SWAP_VALUES
}

static size_t VariadicLength_0x45(std::span<const uint8_t>, size_t)
{
    return 17; // LOAD_SCHEDULED_TASK
}

static size_t VariadicLength_0x59(std::span<const uint8_t>, size_t)
{
    return 10; // UPDATE_ENTITY_DATA_MULTI
}

static size_t VariadicLength_0x5A(std::span<const uint8_t>, size_t)
{
    return 10; // UPDATE_EVENT_POSITION
}

static size_t VariadicLength_0x5B(std::span<const uint8_t>, size_t)
{
    return 15; // LOAD_EXT_SCHEDULER
}

static size_t VariadicLength_0x5C(std::span<const uint8_t>, size_t)
{
    return 4; // MUSIC_CONTROL
}

static size_t VariadicLength_0x62(std::span<const uint8_t>, size_t)
{
    return 5; // LOAD_EVENT_SCHEDULER
}

static size_t VariadicLength_0x79(std::span<const uint8_t>, size_t)
{
    return 10; // LOOK_AT_ENTITY
}

static size_t VariadicLength_0x82(std::span<const uint8_t>, size_t)
{
    return 7; // RECT_HIT_TEST_BRANCH
}

static size_t VariadicLength_0x8C(std::span<const uint8_t>, size_t)
{
    return 5; // CRAFTING_HANDLER
}

static size_t VariadicLength_0x8E(std::span<const uint8_t>, size_t)
{
    return 5; // SET_ENTITY_STATUS_EVENT_45
}

static size_t VariadicLength_0x9D(std::span<const uint8_t>, size_t)
{
    return 5; // OPCODE_9D
}

static size_t VariadicLength_0xAB(std::span<const uint8_t>, size_t)
{
    return 3; // ADJUST_ENTITY_FLAGS
}

static size_t VariadicLength_0xAC(std::span<const uint8_t>, size_t)
{
    return 5; // ENTITY_STATUS_HANDLER
}

static size_t VariadicLength_0xAD(std::span<const uint8_t>, size_t)
{
    return 12; // DUAL_ENTITY_SCHEDULER_HANDLER
}

static size_t VariadicLength_0xAE(std::span<const uint8_t>, size_t)
{
    return 5; // MULTI_PURPOSE_ENTITY_HANDLER
}

static size_t VariadicLength_0xB1(std::span<const uint8_t>, size_t)
{
    return 5; // GET_APP_FLAG
}

static size_t VariadicLength_0xB4(std::span<const uint8_t>, size_t)
{
    return 5; // UI_WINDOW_STRING_HANDLER
}

static size_t VariadicLength_0xB6(std::span<const uint8_t>, size_t)
{
    return 10; // ENTITY_APPEARANCE_HANDLER
}

static size_t VariadicLength_0xB7(std::span<const uint8_t>, size_t)
{
    return 5; // ENTITY_DATA_HANDLER
}

static size_t VariadicLength_0xB8(std::span<const uint8_t>, size_t)
{
    return 17; // MAP_ADD_MARKER_WITH_NAME
}

static size_t VariadicLength_0xC4(std::span<const uint8_t>, size_t)
{
    return 5; // HELPER_CALL_ALT
}

static size_t VariadicLength_0xCE(std::span<const uint8_t>, size_t)
{
    return 3; // WAIT_LOAD_SCHEDULER_ALT4
}

static size_t VariadicLength_0xD1(std::span<const uint8_t>, size_t)
{
    return 3; // WAIT_LOAD_SCHEDULER_ALT5
}
// Auto-generated from Python opcode definitions

// Default byte length for each opcode (1 = unknown/minimum)
// 0xFF = variable length, needs runtime calculation
static constexpr uint8_t kOpLen[256] = {
    /* 00 */   1,   3,   8,   5,   3,   1,   1,   1,   1,   1,   1,   3,   3,   5,   5,   5,
    /* 10 */   5,   5,   3,   5,   5,   5,   7,   7,   7,   5,   3,   1,   3,   3,   5,   5,
    /* 20 */   2,   1,   2,   1,   7,   1,   1,   7,   7,   7,   6,   7,  13,  13,   1,   6,
    /* 30 */   1,   5,   3,   2,   3,   3,   7,   9,   3,   3,   7,  11,   7,   7,   7,   7,
    /* 40 */   9,   9,   1,   2,   5,  17,   2,   3,   3,   7,   9,   7,   1,   1,   6,   3,
    /* 50 */  13,  13,  15,  13,  13,  15,   5,   3,   1,  10,  10,  15,   4,   5,   5,   3,
    /* 60 */   3,   2,  17,   3,  11,  11,  15,   5,   1,   4,   7,   9,   9,   7,   7,   1,
    /* 70 */   1,   2,   4,  11,   2,   2,   5,   5,   1,  10,   6,   5,   6,   3,   2,   1,
    /* 80 */   5,   6,   7,   3,   1,   1,   6,   2,   2,   3,   1,   9,   1,   5,   1,   1,
    /* 90 */   1,   1,   6,   3,   6,   3,   1,   5,   1,   5,   1,   1,   3, 0xFF,   2,  17,
    /* A0 */  15,  15,  15,  15,   2,   2,   2,   2,   6,   3,  17,   2,   3,  12,   2,   8,
    /* B0 */  12,   4,   2,   2,   2,   4,   2,   2,   1,   8,  13,  17,  15,  15,   3,   2,
    /* C0 */   3,   5,   2,   7,  17,  17,  15,  15,   7,   1,   1,   1,   2,  17,  15,  15,
    /* D0 */  17,  15,  15,   6,   2,  17,  15,  15,   8,   2,   1,   1,   1,   1,   1,   1,
    /* E0 */   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    /* F0 */   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
};

// Resolve opcode length at runtime
// Returns >0 for known fixed lengths, calls variadic_handler for variable-length
size_t ResolveOpcodeLength(uint8_t op, std::span<const uint8_t> data, size_t offset)
{
    if (kOpLen[op] != 0xFF)
        return kOpLen[op];

    switch (op)
    {
    case 0x02: // IF_CONDITIONAL
        return VariadicLength_0x02(data, offset);
    case 0x07: // ADD_VALUES
        return VariadicLength_0x07(data, offset);
    case 0x08: // SUBTRACT_VALUES
        return VariadicLength_0x08(data, offset);
    case 0x09: // SET_BIT_FLAG
        return VariadicLength_0x09(data, offset);
    case 0x0A: // CLEAR_BIT_FLAG
        return VariadicLength_0x0A(data, offset);
    case 0x0B: // INCREMENT_VALUE
        return VariadicLength_0x0B(data, offset);
    case 0x0C: // DECREMENT_VALUE
        return VariadicLength_0x0C(data, offset);
    case 0x0D: // BITWISE_AND
        return VariadicLength_0x0D(data, offset);
    case 0x0E: // BITWISE_OR
        return VariadicLength_0x0E(data, offset);
    case 0x0F: // BITWISE_XOR
        return VariadicLength_0x0F(data, offset);
    case 0x10: // BITWISE_LEFT_SHIFT
        return VariadicLength_0x10(data, offset);
    case 0x11: // BITWISE_RIGHT_SHIFT
        return VariadicLength_0x11(data, offset);
    case 0x15: // DIVIDE_VALUES
        return VariadicLength_0x15(data, offset);
    case 0x16: // SINE_CALCULATION
        return VariadicLength_0x16(data, offset);
    case 0x17: // COSINE_CALCULATION
        return VariadicLength_0x17(data, offset);
    case 0x18: // ATAN2_CALCULATION
        return VariadicLength_0x18(data, offset);
    case 0x19: // SWAP_VALUES
        return VariadicLength_0x19(data, offset);
    case 0x45: // LOAD_SCHEDULED_TASK
        return VariadicLength_0x45(data, offset);
    case 0x59: // UPDATE_ENTITY_DATA_MULTI
        return VariadicLength_0x59(data, offset);
    case 0x5A: // UPDATE_EVENT_POSITION
        return VariadicLength_0x5A(data, offset);
    case 0x5B: // LOAD_EXT_SCHEDULER
        return VariadicLength_0x5B(data, offset);
    case 0x5C: // MUSIC_CONTROL
        return VariadicLength_0x5C(data, offset);
    case 0x62: // LOAD_EVENT_SCHEDULER
        return VariadicLength_0x62(data, offset);
    case 0x79: // LOOK_AT_ENTITY
        return VariadicLength_0x79(data, offset);
    case 0x82: // RECT_HIT_TEST_BRANCH
        return VariadicLength_0x82(data, offset);
    case 0x8C: // CRAFTING_HANDLER
        return VariadicLength_0x8C(data, offset);
    case 0x8E: // SET_ENTITY_STATUS_EVENT_45
        return VariadicLength_0x8E(data, offset);
    case 0x9D: // OPCODE_9D
        return VariadicLength_0x9D(data, offset);
    case 0xAB: // ADJUST_ENTITY_FLAGS
        return VariadicLength_0xAB(data, offset);
    case 0xAC: // ENTITY_STATUS_HANDLER
        return VariadicLength_0xAC(data, offset);
    case 0xAD: // DUAL_ENTITY_SCHEDULER_HANDLER
        return VariadicLength_0xAD(data, offset);
    case 0xAE: // MULTI_PURPOSE_ENTITY_HANDLER
        return VariadicLength_0xAE(data, offset);
    case 0xB1: // GET_APP_FLAG
        return VariadicLength_0xB1(data, offset);
    case 0xB4: // UI_WINDOW_STRING_HANDLER
        return VariadicLength_0xB4(data, offset);
    case 0xB6: // ENTITY_APPEARANCE_HANDLER
        return VariadicLength_0xB6(data, offset);
    case 0xB7: // ENTITY_DATA_HANDLER
        return VariadicLength_0xB7(data, offset);
    case 0xB8: // MAP_ADD_MARKER_WITH_NAME
        return VariadicLength_0xB8(data, offset);
    case 0xC4: // HELPER_CALL_ALT
        return VariadicLength_0xC4(data, offset);
    case 0xCE: // WAIT_LOAD_SCHEDULER_ALT4
        return VariadicLength_0xCE(data, offset);
    case 0xD1: // WAIT_LOAD_SCHEDULER_ALT5
        return VariadicLength_0xD1(data, offset);
    default:
        return 1;
    }
}

// === Auto-generated OpCode Classes ===

class Op00_END_REQSTACK : public OpCode
{
public:
    uint8_t code() const override { return 0x00; }
    const char* name() const override { return "END_REQSTACK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op01_GOTO : public OpCode
{
public:
    uint8_t code() const override { return 0x01; }
    const char* name() const override { return "GOTO"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op02_IF_CONDITIONAL : public OpCode
{
public:
    uint8_t code() const override { return 0x02; }
    const char* name() const override { return "IF_CONDITIONAL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
};


class Op03_COPY_WORK_VALUE : public OpCode
{
public:
    uint8_t code() const override { return 0x03; }
    const char* name() const override { return "COPY_WORK_VALUE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op04_DEPRECATED_NOP : public OpCode
{
public:
    uint8_t code() const override { return 0x04; }
    const char* name() const override { return "DEPRECATED_NOP"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op05_SET_ONE : public OpCode
{
public:
    uint8_t code() const override { return 0x05; }
    const char* name() const override { return "SET_ONE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op06_SET_ZERO : public OpCode
{
public:
    uint8_t code() const override { return 0x06; }
    const char* name() const override { return "SET_ZERO"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op07_ADD_VALUES : public OpCode
{
public:
    uint8_t code() const override { return 0x07; }
    const char* name() const override { return "ADD_VALUES"; }
    size_t length(std::span<const uint8_t>, size_t) const override
    {
        return 1; // VARIABLE - override needed for accurate parsing
    }
};


class Op08_SUBTRACT_VALUES : public OpCode
{
public:
    uint8_t code() const override { return 0x08; }
    const char* name() const override { return "SUBTRACT_VALUES"; }
    size_t length(std::span<const uint8_t>, size_t) const override
    {
        return 1; // VARIABLE - override needed for accurate parsing
    }
};


class Op09_SET_BIT_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x09; }
    const char* name() const override { return "SET_BIT_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override
    {
        return 1; // VARIABLE - override needed for accurate parsing
    }
};


class Op0A_CLEAR_BIT_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x0A; }
    const char* name() const override { return "CLEAR_BIT_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override
    {
        return 1; // VARIABLE - override needed for accurate parsing
    }
};


class Op0B_INCREMENT_VALUE : public OpCode
{
public:
    uint8_t code() const override { return 0x0B; }
    const char* name() const override { return "INCREMENT_VALUE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op0C_DECREMENT_VALUE : public OpCode
{
public:
    uint8_t code() const override { return 0x0C; }
    const char* name() const override { return "DECREMENT_VALUE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op0D_BITWISE_AND : public OpCode
{
public:
    uint8_t code() const override { return 0x0D; }
    const char* name() const override { return "BITWISE_AND"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op0E_BITWISE_OR : public OpCode
{
public:
    uint8_t code() const override { return 0x0E; }
    const char* name() const override { return "BITWISE_OR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op0F_BITWISE_XOR : public OpCode
{
public:
    uint8_t code() const override { return 0x0F; }
    const char* name() const override { return "BITWISE_XOR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op10_BITWISE_LEFT_SHIFT : public OpCode
{
public:
    uint8_t code() const override { return 0x10; }
    const char* name() const override { return "BITWISE_LEFT_SHIFT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op11_BITWISE_RIGHT_SHIFT : public OpCode
{
public:
    uint8_t code() const override { return 0x11; }
    const char* name() const override { return "BITWISE_RIGHT_SHIFT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op12_GENERATE_RANDOM : public OpCode
{
public:
    uint8_t code() const override { return 0x12; }
    const char* name() const override { return "GENERATE_RANDOM"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op13_GENERATE_RANDOM_RANGE : public OpCode
{
public:
    uint8_t code() const override { return 0x13; }
    const char* name() const override { return "GENERATE_RANDOM_RANGE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op14_MULTIPLY_VALUES : public OpCode
{
public:
    uint8_t code() const override { return 0x14; }
    const char* name() const override { return "MULTIPLY_VALUES"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op15_DIVIDE_VALUES : public OpCode
{
public:
    uint8_t code() const override { return 0x15; }
    const char* name() const override { return "DIVIDE_VALUES"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op16_SINE_CALCULATION : public OpCode
{
public:
    uint8_t code() const override { return 0x16; }
    const char* name() const override { return "SINE_CALCULATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op17_COSINE_CALCULATION : public OpCode
{
public:
    uint8_t code() const override { return 0x17; }
    const char* name() const override { return "COSINE_CALCULATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op18_ATAN2_CALCULATION : public OpCode
{
public:
    uint8_t code() const override { return 0x18; }
    const char* name() const override { return "ATAN2_CALCULATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op19_SWAP_VALUES : public OpCode
{
public:
    uint8_t code() const override { return 0x19; }
    const char* name() const override { return "SWAP_VALUES"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op1A_JUMP_TO_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0x1A; }
    const char* name() const override { return "JUMP_TO_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op1B_RETURN_FROM_JUMP : public OpCode
{
public:
    uint8_t code() const override { return 0x1B; }
    const char* name() const override { return "RETURN_FROM_JUMP"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op1C_WAIT_TIME : public OpCode
{
public:
    uint8_t code() const override { return 0x1C; }
    const char* name() const override { return "WAIT_TIME"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op1F_MOVE_ENTITY : public OpCode
{
public:
    uint8_t code() const override { return 0x1F; }
    const char* name() const override { return "MOVE_ENTITY"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op20_SET_CLI_EVENT_UC_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x20; }
    const char* name() const override { return "SET_CLI_EVENT_UC_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op21_END_EVENT : public OpCode
{
public:
    uint8_t code() const override { return 0x21; }
    const char* name() const override { return "END_EVENT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op22_ENTITY_HIDE_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x22; }
    const char* name() const override { return "ENTITY_HIDE_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op23_WAIT_FOR_DIALOG_INTERACTION : public OpCode
{
public:
    uint8_t code() const override { return 0x23; }
    const char* name() const override { return "WAIT_FOR_DIALOG_INTERACTION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op25_WAIT_DIALOG_SELECT : public OpCode
{
public:
    uint8_t code() const override { return 0x25; }
    const char* name() const override { return "WAIT_DIALOG_SELECT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op26_YIELD_VM : public OpCode
{
public:
    uint8_t code() const override { return 0x26; }
    const char* name() const override { return "YIELD_VM"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op27_REQ_SET : public OpCode
{
public:
    uint8_t code() const override { return 0x27; }
    const char* name() const override { return "REQ_SET"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op28_REQ_SET_WITH_CONDITIONS : public OpCode
{
public:
    uint8_t code() const override { return 0x28; }
    const char* name() const override { return "REQ_SET_WITH_CONDITIONS"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op29_REQ_SET_WAIT : public OpCode
{
public:
    uint8_t code() const override { return 0x29; }
    const char* name() const override { return "REQ_SET_WAIT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op2A_GET_REQ_LEVEL : public OpCode
{
public:
    uint8_t code() const override { return 0x2A; }
    const char* name() const override { return "GET_REQ_LEVEL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op2C_CREATE_SCHEDULER_TASK : public OpCode
{
public:
    uint8_t code() const override { return 0x2C; }
    const char* name() const override { return "CREATE_SCHEDULER_TASK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class Op2D_CREATE_ZONE_SCHEDULER_TASK : public OpCode
{
public:
    uint8_t code() const override { return 0x2D; }
    const char* name() const override { return "CREATE_ZONE_SCHEDULER_TASK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class Op2E_SET_CLI_EVENT_CANCEL_FLAGS : public OpCode
{
public:
    uint8_t code() const override { return 0x2E; }
    const char* name() const override { return "SET_CLI_EVENT_CANCEL_FLAGS"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op2F_ADJUST_RENDER_FLAGS0 : public OpCode
{
public:
    uint8_t code() const override { return 0x2F; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS0"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op30_SET_UCOFF_CONTINUE_ZERO : public OpCode
{
public:
    uint8_t code() const override { return 0x30; }
    const char* name() const override { return "SET_UCOFF_CONTINUE_ZERO"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op31_UPDATE_ENTITY_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0x31; }
    const char* name() const override { return "UPDATE_ENTITY_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op32_SET_MAIN_SPEED : public OpCode
{
public:
    uint8_t code() const override { return 0x32; }
    const char* name() const override { return "SET_MAIN_SPEED"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op33_ADJUST_EVENT_RENDER_FLAGS0 : public OpCode
{
public:
    uint8_t code() const override { return 0x33; }
    const char* name() const override { return "ADJUST_EVENT_RENDER_FLAGS0"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op34_LOAD_UNLOAD_ZONE : public OpCode
{
public:
    uint8_t code() const override { return 0x34; }
    const char* name() const override { return "LOAD_UNLOAD_ZONE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op35_LOAD_ZONE_NO_CLOSE : public OpCode
{
public:
    uint8_t code() const override { return 0x35; }
    const char* name() const override { return "LOAD_ZONE_NO_CLOSE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op36_SET_ENTITY_EVENT_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0x36; }
    const char* name() const override { return "SET_ENTITY_EVENT_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op37_UPDATE_EVENT_POSITION_AND_DIR : public OpCode
{
public:
    uint8_t code() const override { return 0x37; }
    const char* name() const override { return "UPDATE_EVENT_POSITION_AND_DIR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};


class Op38_SET_CLIENT_EVENT_MODE : public OpCode
{
public:
    uint8_t code() const override { return 0x38; }
    const char* name() const override { return "SET_CLIENT_EVENT_MODE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op39_SET_ENTITY_DIRECTION : public OpCode
{
public:
    uint8_t code() const override { return 0x39; }
    const char* name() const override { return "SET_ENTITY_DIRECTION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op3A_CONVERT_YAW_TO_BYTE : public OpCode
{
public:
    uint8_t code() const override { return 0x3A; }
    const char* name() const override { return "CONVERT_YAW_TO_BYTE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op3B_GET_ENTITY_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0x3B; }
    const char* name() const override { return "GET_ENTITY_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 11; }
};


class Op3C_SET_BIT_FLAG_CONDITIONAL : public OpCode
{
public:
    uint8_t code() const override { return 0x3C; }
    const char* name() const override { return "SET_BIT_FLAG_CONDITIONAL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op3D_CLEAR_BIT_FLAG_CONDITIONAL : public OpCode
{
public:
    uint8_t code() const override { return 0x3D; }
    const char* name() const override { return "CLEAR_BIT_FLAG_CONDITIONAL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op3E_TEST_BIT_AND_BRANCH : public OpCode
{
public:
    uint8_t code() const override { return 0x3E; }
    const char* name() const override { return "TEST_BIT_AND_BRANCH"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op3F_MODULO_OPERATION : public OpCode
{
public:
    uint8_t code() const override { return 0x3F; }
    const char* name() const override { return "MODULO_OPERATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op40_SET_BIT_WORK_RANGE : public OpCode
{
public:
    uint8_t code() const override { return 0x40; }
    const char* name() const override { return "SET_BIT_WORK_RANGE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};


class Op41_GET_BIT_WORK_RANGE : public OpCode
{
public:
    uint8_t code() const override { return 0x41; }
    const char* name() const override { return "GET_BIT_WORK_RANGE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};


class Op42_SET_CLI_EVENT_CANCEL_DATA : public OpCode
{
public:
    uint8_t code() const override { return 0x42; }
    const char* name() const override { return "SET_CLI_EVENT_CANCEL_DATA"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op43_SEND_EVENT_UPDATE : public OpCode
{
public:
    uint8_t code() const override { return 0x43; }
    const char* name() const override { return "SEND_EVENT_UPDATE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op44_IF_ENTITY_VALID : public OpCode
{
public:
    uint8_t code() const override { return 0x44; }
    const char* name() const override { return "IF_ENTITY_VALID"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op45_LOAD_SCHEDULED_TASK : public OpCode
{
public:
    uint8_t code() const override { return 0x45; }
    const char* name() const override { return "LOAD_SCHEDULED_TASK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class Op46_CAMERA_CONTROL : public OpCode
{
public:
    uint8_t code() const override { return 0x46; }
    const char* name() const override { return "CAMERA_CONTROL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op47_UPDATE_PLAYER_LOCATION : public OpCode
{
public:
    uint8_t code() const override { return 0x47; }
    const char* name() const override { return "UPDATE_PLAYER_LOCATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op4B_UPDATE_ENTITY_YAW : public OpCode
{
public:
    uint8_t code() const override { return 0x4B; }
    const char* name() const override { return "UPDATE_ENTITY_YAW"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op4C_SET_ENTITY_STATUS_EVENT_DOOR : public OpCode
{
public:
    uint8_t code() const override { return 0x4C; }
    const char* name() const override { return "SET_ENTITY_STATUS_EVENT_DOOR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op4D_SET_ENTITY_STATUS_EVENT_CLOSE_DOOR : public OpCode
{
public:
    uint8_t code() const override { return 0x4D; }
    const char* name() const override { return "SET_ENTITY_STATUS_EVENT_CLOSE_DOOR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op4E_SET_ENTITY_HIDE_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x4E; }
    const char* name() const override { return "SET_ENTITY_HIDE_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op4F_SET_ENTITY_STATUS_EVENT_CUSTOM : public OpCode
{
public:
    uint8_t code() const override { return 0x4F; }
    const char* name() const override { return "SET_ENTITY_STATUS_EVENT_CUSTOM"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op50_END_SCHEDULER_TASK : public OpCode
{
public:
    uint8_t code() const override { return 0x50; }
    const char* name() const override { return "END_SCHEDULER_TASK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class Op51_END_MAP_SCHEDULER : public OpCode
{
public:
    uint8_t code() const override { return 0x51; }
    const char* name() const override { return "END_MAP_SCHEDULER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class Op52_END_LOAD_SCHEDULER : public OpCode
{
public:
    uint8_t code() const override { return 0x52; }
    const char* name() const override { return "END_LOAD_SCHEDULER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class Op53_WAIT_SCHEDULER_TASK : public OpCode
{
public:
    uint8_t code() const override { return 0x53; }
    const char* name() const override { return "WAIT_SCHEDULER_TASK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class Op54_WAIT_MAP_SCHEDULER : public OpCode
{
public:
    uint8_t code() const override { return 0x54; }
    const char* name() const override { return "WAIT_MAP_SCHEDULER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class Op55_WAIT_LOAD_SCHEDULER : public OpCode
{
public:
    uint8_t code() const override { return 0x55; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class Op56_GET_ACTOR_INDEX_DEPRECATED : public OpCode
{
public:
    uint8_t code() const override { return 0x56; }
    const char* name() const override { return "GET_ACTOR_INDEX_DEPRECATED"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op57_CREATE_FRAME_DELAY : public OpCode
{
public:
    uint8_t code() const override { return 0x57; }
    const char* name() const override { return "CREATE_FRAME_DELAY"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op58_YIELD_EVENT_VM : public OpCode
{
public:
    uint8_t code() const override { return 0x58; }
    const char* name() const override { return "YIELD_EVENT_VM"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op59_UPDATE_ENTITY_DATA_MULTI : public OpCode
{
public:
    uint8_t code() const override { return 0x59; }
    const char* name() const override { return "UPDATE_ENTITY_DATA_MULTI"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 10; }
};


class Op5A_UPDATE_EVENT_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0x5A; }
    const char* name() const override { return "UPDATE_EVENT_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 10; }
};


class Op5B_LOAD_EXT_SCHEDULER : public OpCode
{
public:
    uint8_t code() const override { return 0x5B; }
    const char* name() const override { return "LOAD_EXT_SCHEDULER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class Op5C_MUSIC_CONTROL : public OpCode
{
public:
    uint8_t code() const override { return 0x5C; }
    const char* name() const override { return "MUSIC_CONTROL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
};


class Op5D_SET_MUSIC_VOLUME : public OpCode
{
public:
    uint8_t code() const override { return 0x5D; }
    const char* name() const override { return "SET_MUSIC_VOLUME"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op5E_STOP_ENTITY_ACTION_RESET_IDLE : public OpCode
{
public:
    uint8_t code() const override { return 0x5E; }
    const char* name() const override { return "STOP_ENTITY_ACTION_RESET_IDLE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op5F_MULTI_HANDLER_COMPLEX : public OpCode
{
public:
    uint8_t code() const override { return 0x5F; }
    const char* name() const override { return "MULTI_HANDLER_COMPLEX"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op60_ADJUST_RENDER_FLAGS1_MULTI : public OpCode
{
public:
    uint8_t code() const override { return 0x60; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS1_MULTI"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op61_ADJUST_RENDER_FLAGS2 : public OpCode
{
public:
    uint8_t code() const override { return 0x61; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS2"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op62_LOAD_EVENT_SCHEDULER : public OpCode
{
public:
    uint8_t code() const override { return 0x62; }
    const char* name() const override { return "LOAD_EVENT_SCHEDULER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class Op63_PLAY_ANIMATION_WAIT : public OpCode
{
public:
    uint8_t code() const override { return 0x63; }
    const char* name() const override { return "PLAY_ANIMATION_WAIT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op64_CALCULATE_DISTANCE : public OpCode
{
public:
    uint8_t code() const override { return 0x64; }
    const char* name() const override { return "CALCULATE_DISTANCE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 11; }
};


class Op65_CALCULATE_3D_DISTANCE : public OpCode
{
public:
    uint8_t code() const override { return 0x65; }
    const char* name() const override { return "CALCULATE_3D_DISTANCE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 11; }
};


class Op66_LOAD_EXT_SCHEDULER_MAIN : public OpCode
{
public:
    uint8_t code() const override { return 0x66; }
    const char* name() const override { return "LOAD_EXT_SCHEDULER_MAIN"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class Op67_HIDE_HUD_ELEMENTS : public OpCode
{
public:
    uint8_t code() const override { return 0x67; }
    const char* name() const override { return "HIDE_HUD_ELEMENTS"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op68_SHOW_HUD_ELEMENTS : public OpCode
{
public:
    uint8_t code() const override { return 0x68; }
    const char* name() const override { return "SHOW_HUD_ELEMENTS"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op69_SET_SOUND_VOLUME : public OpCode
{
public:
    uint8_t code() const override { return 0x69; }
    const char* name() const override { return "SET_SOUND_VOLUME"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
};


class Op6A_CHANGE_SOUND_VOLUME : public OpCode
{
public:
    uint8_t code() const override { return 0x6A; }
    const char* name() const override { return "CHANGE_SOUND_VOLUME"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op6B_ENTITY_IDLE_MOTION : public OpCode
{
public:
    uint8_t code() const override { return 0x6B; }
    const char* name() const override { return "ENTITY_IDLE_MOTION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};


class Op6C_FADE_ENTITY_COLOR : public OpCode
{
public:
    uint8_t code() const override { return 0x6C; }
    const char* name() const override { return "FADE_ENTITY_COLOR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};


class Op6D_DEPRECATED_OPCODE : public OpCode
{
public:
    uint8_t code() const override { return 0x6D; }
    const char* name() const override { return "DEPRECATED_OPCODE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op6E_PLAY_EMOTE : public OpCode
{
public:
    uint8_t code() const override { return 0x6E; }
    const char* name() const override { return "PLAY_EMOTE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op6F_WAIT_FRAME_DELAY : public OpCode
{
public:
    uint8_t code() const override { return 0x6F; }
    const char* name() const override { return "WAIT_FRAME_DELAY"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op70_WAIT_ENTITY_RENDER_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x70; }
    const char* name() const override { return "WAIT_ENTITY_RENDER_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op71_HANDLE_STRING_INPUT : public OpCode
{
public:
    uint8_t code() const override { return 0x71; }
    const char* name() const override { return "HANDLE_STRING_INPUT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op72_LOAD_EVENT_WEATHER : public OpCode
{
public:
    uint8_t code() const override { return 0x72; }
    const char* name() const override { return "LOAD_EVENT_WEATHER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
};


class Op73_SCHEDULE_MAGIC_CASTING : public OpCode
{
public:
    uint8_t code() const override { return 0x73; }
    const char* name() const override { return "SCHEDULE_MAGIC_CASTING"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 11; }
};


class Op74_ADJUST_RENDER_FLAGS1 : public OpCode
{
public:
    uint8_t code() const override { return 0x74; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS1"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op75_LOAD_ROOM : public OpCode
{
public:
    uint8_t code() const override { return 0x75; }
    const char* name() const override { return "LOAD_ROOM"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op76_CHECK_ENTITY_RENDER_FLAGS : public OpCode
{
public:
    uint8_t code() const override { return 0x76; }
    const char* name() const override { return "CHECK_ENTITY_RENDER_FLAGS"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op77_SET_EVENT_TIME_WEATHER : public OpCode
{
public:
    uint8_t code() const override { return 0x77; }
    const char* name() const override { return "SET_EVENT_TIME_WEATHER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op78_ENABLE_GAME_TIMER_RESET_WEATHER : public OpCode
{
public:
    uint8_t code() const override { return 0x78; }
    const char* name() const override { return "ENABLE_GAME_TIMER_RESET_WEATHER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op7A_VM_CONTROL : public OpCode
{
public:
    uint8_t code() const override { return 0x7A; }
    const char* name() const override { return "VM_CONTROL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op7C_ADJUST_RENDER_FLAGS2 : public OpCode
{
public:
    uint8_t code() const override { return 0x7C; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS2"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op7D_LOAD_START_SCHEDULER_PLAYER : public OpCode
{
public:
    uint8_t code() const override { return 0x7D; }
    const char* name() const override { return "LOAD_START_SCHEDULER_PLAYER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op7E_CHOCOBO_MOUNT_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0x7E; }
    const char* name() const override { return "CHOCOBO_MOUNT_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op7F_WAIT_DIALOG_SELECT_ALT : public OpCode
{
public:
    uint8_t code() const override { return 0x7F; }
    const char* name() const override { return "WAIT_DIALOG_SELECT_ALT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op80_LOAD_WAIT : public OpCode
{
public:
    uint8_t code() const override { return 0x80; }
    const char* name() const override { return "LOAD_WAIT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op81_SET_ENTITY_BLINKING : public OpCode
{
public:
    uint8_t code() const override { return 0x81; }
    const char* name() const override { return "SET_ENTITY_BLINKING"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op82_RECT_HIT_TEST_BRANCH : public OpCode
{
public:
    uint8_t code() const override { return 0x82; }
    const char* name() const override { return "RECT_HIT_TEST_BRANCH"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class Op83_GET_GAME_TIME : public OpCode
{
public:
    uint8_t code() const override { return 0x83; }
    const char* name() const override { return "GET_GAME_TIME"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op84_ADJUST_RENDER_FLAGS3_BIT0 : public OpCode
{
public:
    uint8_t code() const override { return 0x84; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3_BIT0"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op85_OPEN_MOOGLE_MENU : public OpCode
{
public:
    uint8_t code() const override { return 0x85; }
    const char* name() const override { return "OPEN_MOOGLE_MENU"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op86_ADJUST_RENDER_FLAGS3 : public OpCode
{
public:
    uint8_t code() const override { return 0x86; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op87_WORLD_PASS_HANDLER_A : public OpCode
{
public:
    uint8_t code() const override { return 0x87; }
    const char* name() const override { return "WORLD_PASS_HANDLER_A"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op88_WORLD_PASS_HANDLER_B : public OpCode
{
public:
    uint8_t code() const override { return 0x88; }
    const char* name() const override { return "WORLD_PASS_HANDLER_B"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op89_OPEN_MAP : public OpCode
{
public:
    uint8_t code() const override { return 0x89; }
    const char* name() const override { return "OPEN_MAP"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op8A_CLOSE_MAP : public OpCode
{
public:
    uint8_t code() const override { return 0x8A; }
    const char* name() const override { return "CLOSE_MAP"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op8B_SET_EVENT_MARK : public OpCode
{
public:
    uint8_t code() const override { return 0x8B; }
    const char* name() const override { return "SET_EVENT_MARK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};


class Op8C_CRAFTING_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0x8C; }
    const char* name() const override { return "CRAFTING_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op8D_OPEN_MAP_WITH_PROPERTIES : public OpCode
{
public:
    uint8_t code() const override { return 0x8D; }
    const char* name() const override { return "OPEN_MAP_WITH_PROPERTIES"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op8E_SET_ENTITY_STATUS_EVENT_45 : public OpCode
{
public:
    uint8_t code() const override { return 0x8E; }
    const char* name() const override { return "SET_ENTITY_STATUS_EVENT_45"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op8F_SET_ENTITY_STATUS_EVENT_46 : public OpCode
{
public:
    uint8_t code() const override { return 0x8F; }
    const char* name() const override { return "SET_ENTITY_STATUS_EVENT_46"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op90_ADJUST_ENTITY_RENDER_FLAGS0_FLAGS1 : public OpCode
{
public:
    uint8_t code() const override { return 0x90; }
    const char* name() const override { return "ADJUST_ENTITY_RENDER_FLAGS0_FLAGS1"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op92_ADJUST_RENDER_FLAGS3 : public OpCode
{
public:
    uint8_t code() const override { return 0x92; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op93_DISPLAY_ITEM_INFO : public OpCode
{
public:
    uint8_t code() const override { return 0x93; }
    const char* name() const override { return "DISPLAY_ITEM_INFO"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op94_ADJUST_RENDER_FLAGS3_ALT : public OpCode
{
public:
    uint8_t code() const override { return 0x94; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3_ALT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class Op96_UNSET_EVENT_NPC : public OpCode
{
public:
    uint8_t code() const override { return 0x96; }
    const char* name() const override { return "UNSET_EVENT_NPC"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op97_SAVE_SET_WIND_VALUES : public OpCode
{
public:
    uint8_t code() const override { return 0x97; }
    const char* name() const override { return "SAVE_SET_WIND_VALUES"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op98_YIELD_IF_ZONE_LOADING : public OpCode
{
public:
    uint8_t code() const override { return 0x98; }
    const char* name() const override { return "YIELD_IF_ZONE_LOADING"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op99_WAIT_ANIMATION : public OpCode
{
public:
    uint8_t code() const override { return 0x99; }
    const char* name() const override { return "WAIT_ANIMATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class Op9A_WAIT_MUSIC_SERVER : public OpCode
{
public:
    uint8_t code() const override { return 0x9A; }
    const char* name() const override { return "WAIT_MUSIC_SERVER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op9B_WAIT_ENTITY_ANIMATION : public OpCode
{
public:
    uint8_t code() const override { return 0x9B; }
    const char* name() const override { return "WAIT_ENTITY_ANIMATION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class Op9C_STORE_CLIENT_LANGUAGE_ID : public OpCode
{
public:
    uint8_t code() const override { return 0x9C; }
    const char* name() const override { return "STORE_CLIENT_LANGUAGE_ID"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op9D_OPCODE_9D : public OpCode
{
public:
    uint8_t code() const override { return 0x9D; }
    const char* name() const override { return "OPCODE_9D"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class Op9E_SET_RECT_EVENT_SEND_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0x9E; }
    const char* name() const override { return "SET_RECT_EVENT_SEND_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class Op9F_LOAD_SCHEDULED_TASK_ALT : public OpCode
{
public:
    uint8_t code() const override { return 0x9F; }
    const char* name() const override { return "LOAD_SCHEDULED_TASK_ALT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpA0_WAIT_LOAD_SCHEDULER_MAIN_ALT2 : public OpCode
{
public:
    uint8_t code() const override { return 0xA0; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_MAIN_ALT2"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpA1_END_LOAD_SCHEDULER_MAIN : public OpCode
{
public:
    uint8_t code() const override { return 0xA1; }
    const char* name() const override { return "END_LOAD_SCHEDULER_MAIN"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpA2_WAIT_LOAD_SCHEDULER_MAIN : public OpCode
{
public:
    uint8_t code() const override { return 0xA2; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_MAIN"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpA3_END_LOAD_SCHEDULER_MAIN_ALT : public OpCode
{
public:
    uint8_t code() const override { return 0xA3; }
    const char* name() const override { return "END_LOAD_SCHEDULER_MAIN_ALT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpA4_ADJUST_RENDER_FLAGS3_BIT26 : public OpCode
{
public:
    uint8_t code() const override { return 0xA4; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3_BIT26"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpA5_ADJUST_RENDER_FLAGS3_BIT11 : public OpCode
{
public:
    uint8_t code() const override { return 0xA5; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3_BIT11"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpA6_REQUEST_EVENT_MAP_NUMBER : public OpCode
{
public:
    uint8_t code() const override { return 0xA6; }
    const char* name() const override { return "REQUEST_EVENT_MAP_NUMBER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpA7_BATTLEFIELD_SERVER_RESPONSE_WAIT : public OpCode
{
public:
    uint8_t code() const override { return 0xA7; }
    const char* name() const override { return "BATTLEFIELD_SERVER_RESPONSE_WAIT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpA8_MAP_MARKER_CONTROL : public OpCode
{
public:
    uint8_t code() const override { return 0xA8; }
    const char* name() const override { return "MAP_MARKER_CONTROL"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class OpA9_DISABLE_GAME_TIME_SET_SPECIFIC : public OpCode
{
public:
    uint8_t code() const override { return 0xA9; }
    const char* name() const override { return "DISABLE_GAME_TIME_SET_SPECIFIC"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class OpAA_VANA_DIEL_TIMESTAMP_CONVERTER : public OpCode
{
public:
    uint8_t code() const override { return 0xAA; }
    const char* name() const override { return "VANA_DIEL_TIMESTAMP_CONVERTER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpAB_ADJUST_ENTITY_FLAGS : public OpCode
{
public:
    uint8_t code() const override { return 0xAB; }
    const char* name() const override { return "ADJUST_ENTITY_FLAGS"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpAC_ENTITY_STATUS_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xAC; }
    const char* name() const override { return "ENTITY_STATUS_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class OpAD_DUAL_ENTITY_SCHEDULER_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xAD; }
    const char* name() const override { return "DUAL_ENTITY_SCHEDULER_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 12; }
};


class OpAE_MULTI_PURPOSE_ENTITY_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xAE; }
    const char* name() const override { return "MULTI_PURPOSE_ENTITY_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpAF_GET_CAMERA_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0xAF; }
    const char* name() const override { return "GET_CAMERA_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
};


class OpB1_GET_APP_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0xB1; }
    const char* name() const override { return "GET_APP_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
};


class OpB2_DELIVERY_BOX_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xB2; }
    const char* name() const override { return "DELIVERY_BOX_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpB3_RANKINGS_BOARD_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xB3; }
    const char* name() const override { return "RANKINGS_BOARD_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpB4_UI_WINDOW_STRING_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xB4; }
    const char* name() const override { return "UI_WINDOW_STRING_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpB5_SET_EVENT_ENTITY_NAME : public OpCode
{
public:
    uint8_t code() const override { return 0xB5; }
    const char* name() const override { return "SET_EVENT_ENTITY_NAME"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
};


class OpB6_ENTITY_APPEARANCE_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xB6; }
    const char* name() const override { return "ENTITY_APPEARANCE_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpB7_ENTITY_DATA_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xB7; }
    const char* name() const override { return "ENTITY_DATA_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpB8_MAP_ADD_MARKER_WITH_NAME : public OpCode
{
public:
    uint8_t code() const override { return 0xB8; }
    const char* name() const override { return "MAP_ADD_MARKER_WITH_NAME"; }
    size_t length(std::span<const uint8_t>, size_t) const override
    {
        return 1; // VARIABLE - override needed for accurate parsing
    }
};


class OpB9_MAP_EDIT_MARKER_FROM_BUFFER : public OpCode
{
public:
    uint8_t code() const override { return 0xB9; }
    const char* name() const override { return "MAP_EDIT_MARKER_FROM_BUFFER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
};


class OpBA_SET_ENTITY_POSITION : public OpCode
{
public:
    uint8_t code() const override { return 0xBA; }
    const char* name() const override { return "SET_ENTITY_POSITION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};


class OpBB_LOAD_EVENT_SCHEDULER_ALT : public OpCode
{
public:
    uint8_t code() const override { return 0xBB; }
    const char* name() const override { return "LOAD_EVENT_SCHEDULER_ALT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpBC_WAIT_LOAD_SCHEDULER_ALT2 : public OpCode
{
public:
    uint8_t code() const override { return 0xBC; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_ALT2"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpBD_END_LOAD_SCHEDULER_MAIN_ALT6 : public OpCode
{
public:
    uint8_t code() const override { return 0xBD; }
    const char* name() const override { return "END_LOAD_SCHEDULER_MAIN_ALT6"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpBE_STORE_REQ_WHO_SERVER_ID : public OpCode
{
public:
    uint8_t code() const override { return 0xBE; }
    const char* name() const override { return "STORE_REQ_WHO_SERVER_ID"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class OpBF_CHOCOBO_RACING_PARAMETER_GETTER : public OpCode
{
public:
    uint8_t code() const override { return 0xBF; }
    const char* name() const override { return "CHOCOBO_RACING_PARAMETER_GETTER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpC0_ADJUST_RENDER_FLAGS3_BIT12 : public OpCode
{
public:
    uint8_t code() const override { return 0xC0; }
    const char* name() const override { return "ADJUST_RENDER_FLAGS3_BIT12"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};


class OpC1_KILL_ENTITY_ACTION : public OpCode
{
public:
    uint8_t code() const override { return 0xC1; }
    const char* name() const override { return "KILL_ENTITY_ACTION"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};


class OpC2_PARTY_STATE_CHECK : public OpCode
{
public:
    uint8_t code() const override { return 0xC2; }
    const char* name() const override { return "PARTY_STATE_CHECK"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpC3_COPY_STRING_TO_ARRAY : public OpCode
{
public:
    uint8_t code() const override { return 0xC3; }
    const char* name() const override { return "COPY_STRING_TO_ARRAY"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class OpC4_HELPER_CALL_ALT : public OpCode
{
public:
    uint8_t code() const override { return 0xC4; }
    const char* name() const override { return "HELPER_CALL_ALT"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpC5_LOAD_SCHEDULED_TASK_ALT3 : public OpCode
{
public:
    uint8_t code() const override { return 0xC5; }
    const char* name() const override { return "LOAD_SCHEDULED_TASK_ALT3"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpC6_WAIT_LOAD_SCHEDULER_ALT3 : public OpCode
{
public:
    uint8_t code() const override { return 0xC6; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_ALT3"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpC7_END_LOAD_SCHEDULER_ALT3 : public OpCode
{
public:
    uint8_t code() const override { return 0xC7; }
    const char* name() const override { return "END_LOAD_SCHEDULER_ALT3"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpC8_OPEN_MAP_WINDOW : public OpCode
{
public:
    uint8_t code() const override { return 0xC8; }
    const char* name() const override { return "OPEN_MAP_WINDOW"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};


class OpC9_ENABLE_GAME_TIMER : public OpCode
{
public:
    uint8_t code() const override { return 0xC9; }
    const char* name() const override { return "ENABLE_GAME_TIMER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class OpCA_DEPRECATED_OPCODE_CA : public OpCode
{
public:
    uint8_t code() const override { return 0xCA; }
    const char* name() const override { return "DEPRECATED_OPCODE_CA"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class OpCB_DEPRECATED_OPCODE_CB : public OpCode
{
public:
    uint8_t code() const override { return 0xCB; }
    const char* name() const override { return "DEPRECATED_OPCODE_CB"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};


class OpCC_ITEM_INFO_WINDOW_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xCC; }
    const char* name() const override { return "ITEM_INFO_WINDOW_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpCD_LOAD_SCHEDULED_TASK_ALT4 : public OpCode
{
public:
    uint8_t code() const override { return 0xCD; }
    const char* name() const override { return "LOAD_SCHEDULED_TASK_ALT4"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpCE_WAIT_LOAD_SCHEDULER_ALT4 : public OpCode
{
public:
    uint8_t code() const override { return 0xCE; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_ALT4"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpCF_END_LOAD_SCHEDULER_ALT4 : public OpCode
{
public:
    uint8_t code() const override { return 0xCF; }
    const char* name() const override { return "END_LOAD_SCHEDULER_ALT4"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpD0_LOAD_SCHEDULED_TASK_ALT5 : public OpCode
{
public:
    uint8_t code() const override { return 0xD0; }
    const char* name() const override { return "LOAD_SCHEDULED_TASK_ALT5"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpD1_WAIT_LOAD_SCHEDULER_ALT5 : public OpCode
{
public:
    uint8_t code() const override { return 0xD1; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_ALT5"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpD2_END_LOAD_SCHEDULER_MAIN_ALT7 : public OpCode
{
public:
    uint8_t code() const override { return 0xD2; }
    const char* name() const override { return "END_LOAD_SCHEDULER_MAIN_ALT7"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpD3_CLEAR_ENTITY_MOTION_QUEUE : public OpCode
{
public:
    uint8_t code() const override { return 0xD3; }
    const char* name() const override { return "CLEAR_ENTITY_MOTION_QUEUE"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
};


class OpD4_MAP_QUERY_WINDOW_HANDLER : public OpCode
{
public:
    uint8_t code() const override { return 0xD4; }
    const char* name() const override { return "MAP_QUERY_WINDOW_HANDLER"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


class OpD5_LOAD_EVENT_SCHEDULER_ALT8 : public OpCode
{
public:
    uint8_t code() const override { return 0xD5; }
    const char* name() const override { return "LOAD_EVENT_SCHEDULER_ALT8"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
};


class OpD6_WAIT_LOAD_SCHEDULER_ALT6 : public OpCode
{
public:
    uint8_t code() const override { return 0xD6; }
    const char* name() const override { return "WAIT_LOAD_SCHEDULER_ALT6"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpD7_END_LOAD_SCHEDULER_ALT6 : public OpCode
{
public:
    uint8_t code() const override { return 0xD7; }
    const char* name() const override { return "END_LOAD_SCHEDULER_ALT6"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};


class OpD8_SET_ENTITY_EVENT_DIR : public OpCode
{
public:
    uint8_t code() const override { return 0xD8; }
    const char* name() const override { return "SET_ENTITY_EVENT_DIR"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
};


class OpD9_SET_SOUND_EFFECT_LIMIT_FLAG : public OpCode
{
public:
    uint8_t code() const override { return 0xD9; }
    const char* name() const override { return "SET_SOUND_EFFECT_LIMIT_FLAG"; }
    size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
};


// Registration function called by OpCodeRegistry constructor
void RegisterAllGeneratedOpcodes(OpCodeRegistry& reg)
{
    reg.Register(std::make_unique<Op00_END_REQSTACK>());
    reg.Register(std::make_unique<Op01_GOTO>());
    reg.Register(std::make_unique<Op02_IF_CONDITIONAL>());
    reg.Register(std::make_unique<Op03_COPY_WORK_VALUE>());
    reg.Register(std::make_unique<Op04_DEPRECATED_NOP>());
    reg.Register(std::make_unique<Op05_SET_ONE>());
    reg.Register(std::make_unique<Op06_SET_ZERO>());
    reg.Register(std::make_unique<Op07_ADD_VALUES>());
    reg.Register(std::make_unique<Op08_SUBTRACT_VALUES>());
    reg.Register(std::make_unique<Op09_SET_BIT_FLAG>());
    reg.Register(std::make_unique<Op0A_CLEAR_BIT_FLAG>());
    reg.Register(std::make_unique<Op0B_INCREMENT_VALUE>());
    reg.Register(std::make_unique<Op0C_DECREMENT_VALUE>());
    reg.Register(std::make_unique<Op0D_BITWISE_AND>());
    reg.Register(std::make_unique<Op0E_BITWISE_OR>());
    reg.Register(std::make_unique<Op0F_BITWISE_XOR>());
    reg.Register(std::make_unique<Op10_BITWISE_LEFT_SHIFT>());
    reg.Register(std::make_unique<Op11_BITWISE_RIGHT_SHIFT>());
    reg.Register(std::make_unique<Op12_GENERATE_RANDOM>());
    reg.Register(std::make_unique<Op13_GENERATE_RANDOM_RANGE>());
    reg.Register(std::make_unique<Op14_MULTIPLY_VALUES>());
    reg.Register(std::make_unique<Op15_DIVIDE_VALUES>());
    reg.Register(std::make_unique<Op16_SINE_CALCULATION>());
    reg.Register(std::make_unique<Op17_COSINE_CALCULATION>());
    reg.Register(std::make_unique<Op18_ATAN2_CALCULATION>());
    reg.Register(std::make_unique<Op19_SWAP_VALUES>());
    reg.Register(std::make_unique<Op1A_JUMP_TO_POSITION>());
    reg.Register(std::make_unique<Op1B_RETURN_FROM_JUMP>());
    reg.Register(std::make_unique<Op1C_WAIT_TIME>());
    // 0x1D PRINT_EVENT_MESSAGE - specialized (registered in category file)
    // 0x1E ENTITY_LOOK_AT_AND_TALK - specialized (registered in category file)
    reg.Register(std::make_unique<Op1F_MOVE_ENTITY>());
    reg.Register(std::make_unique<Op20_SET_CLI_EVENT_UC_FLAG>());
    reg.Register(std::make_unique<Op21_END_EVENT>());
    reg.Register(std::make_unique<Op22_ENTITY_HIDE_FLAG>());
    reg.Register(std::make_unique<Op23_WAIT_FOR_DIALOG_INTERACTION>());
    // 0x24 CREATE_DIALOG - specialized (registered in category file)
    reg.Register(std::make_unique<Op25_WAIT_DIALOG_SELECT>());
    reg.Register(std::make_unique<Op26_YIELD_VM>());
    reg.Register(std::make_unique<Op27_REQ_SET>());
    reg.Register(std::make_unique<Op28_REQ_SET_WITH_CONDITIONS>());
    reg.Register(std::make_unique<Op29_REQ_SET_WAIT>());
    reg.Register(std::make_unique<Op2A_GET_REQ_LEVEL>());
    // 0x2B PRINT_ENTITY_MESSAGE - specialized (registered in category file)
    reg.Register(std::make_unique<Op2C_CREATE_SCHEDULER_TASK>());
    reg.Register(std::make_unique<Op2D_CREATE_ZONE_SCHEDULER_TASK>());
    reg.Register(std::make_unique<Op2E_SET_CLI_EVENT_CANCEL_FLAGS>());
    reg.Register(std::make_unique<Op2F_ADJUST_RENDER_FLAGS0>());
    reg.Register(std::make_unique<Op30_SET_UCOFF_CONTINUE_ZERO>());
    reg.Register(std::make_unique<Op31_UPDATE_ENTITY_POSITION>());
    reg.Register(std::make_unique<Op32_SET_MAIN_SPEED>());
    reg.Register(std::make_unique<Op33_ADJUST_EVENT_RENDER_FLAGS0>());
    reg.Register(std::make_unique<Op34_LOAD_UNLOAD_ZONE>());
    reg.Register(std::make_unique<Op35_LOAD_ZONE_NO_CLOSE>());
    reg.Register(std::make_unique<Op36_SET_ENTITY_EVENT_POSITION>());
    reg.Register(std::make_unique<Op37_UPDATE_EVENT_POSITION_AND_DIR>());
    reg.Register(std::make_unique<Op38_SET_CLIENT_EVENT_MODE>());
    reg.Register(std::make_unique<Op39_SET_ENTITY_DIRECTION>());
    reg.Register(std::make_unique<Op3A_CONVERT_YAW_TO_BYTE>());
    reg.Register(std::make_unique<Op3B_GET_ENTITY_POSITION>());
    reg.Register(std::make_unique<Op3C_SET_BIT_FLAG_CONDITIONAL>());
    reg.Register(std::make_unique<Op3D_CLEAR_BIT_FLAG_CONDITIONAL>());
    reg.Register(std::make_unique<Op3E_TEST_BIT_AND_BRANCH>());
    reg.Register(std::make_unique<Op3F_MODULO_OPERATION>());
    reg.Register(std::make_unique<Op40_SET_BIT_WORK_RANGE>());
    reg.Register(std::make_unique<Op41_GET_BIT_WORK_RANGE>());
    reg.Register(std::make_unique<Op42_SET_CLI_EVENT_CANCEL_DATA>());
    reg.Register(std::make_unique<Op43_SEND_EVENT_UPDATE>());
    reg.Register(std::make_unique<Op44_IF_ENTITY_VALID>());
    reg.Register(std::make_unique<Op45_LOAD_SCHEDULED_TASK>());
    reg.Register(std::make_unique<Op46_CAMERA_CONTROL>());
    reg.Register(std::make_unique<Op47_UPDATE_PLAYER_LOCATION>());
    // 0x48 PRINT_MESSAGE - specialized (registered in category file)
    // 0x49 PRINT_EVENT_MESSAGE_NO_SPEAKER - specialized (registered in category file)
    // 0x4A ENTITY_LOOK_AT - specialized (registered in category file)
    reg.Register(std::make_unique<Op4B_UPDATE_ENTITY_YAW>());
    reg.Register(std::make_unique<Op4C_SET_ENTITY_STATUS_EVENT_DOOR>());
    reg.Register(std::make_unique<Op4D_SET_ENTITY_STATUS_EVENT_CLOSE_DOOR>());
    reg.Register(std::make_unique<Op4E_SET_ENTITY_HIDE_FLAG>());
    reg.Register(std::make_unique<Op4F_SET_ENTITY_STATUS_EVENT_CUSTOM>());
    reg.Register(std::make_unique<Op50_END_SCHEDULER_TASK>());
    reg.Register(std::make_unique<Op51_END_MAP_SCHEDULER>());
    reg.Register(std::make_unique<Op52_END_LOAD_SCHEDULER>());
    reg.Register(std::make_unique<Op53_WAIT_SCHEDULER_TASK>());
    reg.Register(std::make_unique<Op54_WAIT_MAP_SCHEDULER>());
    reg.Register(std::make_unique<Op55_WAIT_LOAD_SCHEDULER>());
    reg.Register(std::make_unique<Op56_GET_ACTOR_INDEX_DEPRECATED>());
    reg.Register(std::make_unique<Op57_CREATE_FRAME_DELAY>());
    reg.Register(std::make_unique<Op58_YIELD_EVENT_VM>());
    reg.Register(std::make_unique<Op59_UPDATE_ENTITY_DATA_MULTI>());
    reg.Register(std::make_unique<Op5A_UPDATE_EVENT_POSITION>());
    reg.Register(std::make_unique<Op5B_LOAD_EXT_SCHEDULER>());
    reg.Register(std::make_unique<Op5C_MUSIC_CONTROL>());
    reg.Register(std::make_unique<Op5D_SET_MUSIC_VOLUME>());
    reg.Register(std::make_unique<Op5E_STOP_ENTITY_ACTION_RESET_IDLE>());
    reg.Register(std::make_unique<Op5F_MULTI_HANDLER_COMPLEX>());
    reg.Register(std::make_unique<Op60_ADJUST_RENDER_FLAGS1_MULTI>());
    reg.Register(std::make_unique<Op61_ADJUST_RENDER_FLAGS2>());
    reg.Register(std::make_unique<Op62_LOAD_EVENT_SCHEDULER>());
    reg.Register(std::make_unique<Op63_PLAY_ANIMATION_WAIT>());
    reg.Register(std::make_unique<Op64_CALCULATE_DISTANCE>());
    reg.Register(std::make_unique<Op65_CALCULATE_3D_DISTANCE>());
    reg.Register(std::make_unique<Op66_LOAD_EXT_SCHEDULER_MAIN>());
    reg.Register(std::make_unique<Op67_HIDE_HUD_ELEMENTS>());
    reg.Register(std::make_unique<Op68_SHOW_HUD_ELEMENTS>());
    reg.Register(std::make_unique<Op69_SET_SOUND_VOLUME>());
    reg.Register(std::make_unique<Op6A_CHANGE_SOUND_VOLUME>());
    reg.Register(std::make_unique<Op6B_ENTITY_IDLE_MOTION>());
    reg.Register(std::make_unique<Op6C_FADE_ENTITY_COLOR>());
    reg.Register(std::make_unique<Op6D_DEPRECATED_OPCODE>());
    reg.Register(std::make_unique<Op6E_PLAY_EMOTE>());
    reg.Register(std::make_unique<Op6F_WAIT_FRAME_DELAY>());
    reg.Register(std::make_unique<Op70_WAIT_ENTITY_RENDER_FLAG>());
    reg.Register(std::make_unique<Op71_HANDLE_STRING_INPUT>());
    reg.Register(std::make_unique<Op72_LOAD_EVENT_WEATHER>());
    reg.Register(std::make_unique<Op73_SCHEDULE_MAGIC_CASTING>());
    reg.Register(std::make_unique<Op74_ADJUST_RENDER_FLAGS1>());
    reg.Register(std::make_unique<Op75_LOAD_ROOM>());
    reg.Register(std::make_unique<Op76_CHECK_ENTITY_RENDER_FLAGS>());
    reg.Register(std::make_unique<Op77_SET_EVENT_TIME_WEATHER>());
    reg.Register(std::make_unique<Op78_ENABLE_GAME_TIMER_RESET_WEATHER>());
    // 0x79 LOOK_AT_ENTITY - specialized (registered in category file)
    reg.Register(std::make_unique<Op7A_VM_CONTROL>());
    // 0x7B UNSET_ENTITY_TALKING - specialized (registered in category file)
    reg.Register(std::make_unique<Op7C_ADJUST_RENDER_FLAGS2>());
    reg.Register(std::make_unique<Op7D_LOAD_START_SCHEDULER_PLAYER>());
    reg.Register(std::make_unique<Op7E_CHOCOBO_MOUNT_HANDLER>());
    reg.Register(std::make_unique<Op7F_WAIT_DIALOG_SELECT_ALT>());
    reg.Register(std::make_unique<Op80_LOAD_WAIT>());
    reg.Register(std::make_unique<Op81_SET_ENTITY_BLINKING>());
    reg.Register(std::make_unique<Op82_RECT_HIT_TEST_BRANCH>());
    reg.Register(std::make_unique<Op83_GET_GAME_TIME>());
    reg.Register(std::make_unique<Op84_ADJUST_RENDER_FLAGS3_BIT0>());
    reg.Register(std::make_unique<Op85_OPEN_MOOGLE_MENU>());
    reg.Register(std::make_unique<Op86_ADJUST_RENDER_FLAGS3>());
    reg.Register(std::make_unique<Op87_WORLD_PASS_HANDLER_A>());
    reg.Register(std::make_unique<Op88_WORLD_PASS_HANDLER_B>());
    reg.Register(std::make_unique<Op89_OPEN_MAP>());
    reg.Register(std::make_unique<Op8A_CLOSE_MAP>());
    reg.Register(std::make_unique<Op8B_SET_EVENT_MARK>());
    reg.Register(std::make_unique<Op8C_CRAFTING_HANDLER>());
    reg.Register(std::make_unique<Op8D_OPEN_MAP_WITH_PROPERTIES>());
    reg.Register(std::make_unique<Op8E_SET_ENTITY_STATUS_EVENT_45>());
    reg.Register(std::make_unique<Op8F_SET_ENTITY_STATUS_EVENT_46>());
    reg.Register(std::make_unique<Op90_ADJUST_ENTITY_RENDER_FLAGS0_FLAGS1>());
    reg.Register(std::make_unique<Op92_ADJUST_RENDER_FLAGS3>());
    reg.Register(std::make_unique<Op93_DISPLAY_ITEM_INFO>());
    reg.Register(std::make_unique<Op94_ADJUST_RENDER_FLAGS3_ALT>());
    // 0x95 SETUP_EVENT_NPC - specialized (registered in category file)
    reg.Register(std::make_unique<Op96_UNSET_EVENT_NPC>());
    reg.Register(std::make_unique<Op97_SAVE_SET_WIND_VALUES>());
    reg.Register(std::make_unique<Op98_YIELD_IF_ZONE_LOADING>());
    reg.Register(std::make_unique<Op99_WAIT_ANIMATION>());
    reg.Register(std::make_unique<Op9A_WAIT_MUSIC_SERVER>());
    reg.Register(std::make_unique<Op9B_WAIT_ENTITY_ANIMATION>());
    reg.Register(std::make_unique<Op9C_STORE_CLIENT_LANGUAGE_ID>());
    reg.Register(std::make_unique<Op9D_OPCODE_9D>());
    reg.Register(std::make_unique<Op9E_SET_RECT_EVENT_SEND_FLAG>());
    reg.Register(std::make_unique<Op9F_LOAD_SCHEDULED_TASK_ALT>());
    reg.Register(std::make_unique<OpA0_WAIT_LOAD_SCHEDULER_MAIN_ALT2>());
    reg.Register(std::make_unique<OpA1_END_LOAD_SCHEDULER_MAIN>());
    reg.Register(std::make_unique<OpA2_WAIT_LOAD_SCHEDULER_MAIN>());
    reg.Register(std::make_unique<OpA3_END_LOAD_SCHEDULER_MAIN_ALT>());
    reg.Register(std::make_unique<OpA4_ADJUST_RENDER_FLAGS3_BIT26>());
    reg.Register(std::make_unique<OpA5_ADJUST_RENDER_FLAGS3_BIT11>());
    reg.Register(std::make_unique<OpA6_REQUEST_EVENT_MAP_NUMBER>());
    reg.Register(std::make_unique<OpA7_BATTLEFIELD_SERVER_RESPONSE_WAIT>());
    reg.Register(std::make_unique<OpA8_MAP_MARKER_CONTROL>());
    reg.Register(std::make_unique<OpA9_DISABLE_GAME_TIME_SET_SPECIFIC>());
    reg.Register(std::make_unique<OpAA_VANA_DIEL_TIMESTAMP_CONVERTER>());
    reg.Register(std::make_unique<OpAB_ADJUST_ENTITY_FLAGS>());
    reg.Register(std::make_unique<OpAC_ENTITY_STATUS_HANDLER>());
    reg.Register(std::make_unique<OpAD_DUAL_ENTITY_SCHEDULER_HANDLER>());
    reg.Register(std::make_unique<OpAE_MULTI_PURPOSE_ENTITY_HANDLER>());
    reg.Register(std::make_unique<OpAF_GET_CAMERA_POSITION>());
    // 0xB0 PRINT_EVENT_MESSAGE - specialized (registered in category file)
    reg.Register(std::make_unique<OpB1_GET_APP_FLAG>());
    reg.Register(std::make_unique<OpB2_DELIVERY_BOX_HANDLER>());
    reg.Register(std::make_unique<OpB3_RANKINGS_BOARD_HANDLER>());
    reg.Register(std::make_unique<OpB4_UI_WINDOW_STRING_HANDLER>());
    reg.Register(std::make_unique<OpB5_SET_EVENT_ENTITY_NAME>());
    reg.Register(std::make_unique<OpB6_ENTITY_APPEARANCE_HANDLER>());
    reg.Register(std::make_unique<OpB7_ENTITY_DATA_HANDLER>());
    reg.Register(std::make_unique<OpB8_MAP_ADD_MARKER_WITH_NAME>());
    reg.Register(std::make_unique<OpB9_MAP_EDIT_MARKER_FROM_BUFFER>());
    reg.Register(std::make_unique<OpBA_SET_ENTITY_POSITION>());
    reg.Register(std::make_unique<OpBB_LOAD_EVENT_SCHEDULER_ALT>());
    reg.Register(std::make_unique<OpBC_WAIT_LOAD_SCHEDULER_ALT2>());
    reg.Register(std::make_unique<OpBD_END_LOAD_SCHEDULER_MAIN_ALT6>());
    reg.Register(std::make_unique<OpBE_STORE_REQ_WHO_SERVER_ID>());
    reg.Register(std::make_unique<OpBF_CHOCOBO_RACING_PARAMETER_GETTER>());
    reg.Register(std::make_unique<OpC0_ADJUST_RENDER_FLAGS3_BIT12>());
    reg.Register(std::make_unique<OpC1_KILL_ENTITY_ACTION>());
    reg.Register(std::make_unique<OpC2_PARTY_STATE_CHECK>());
    reg.Register(std::make_unique<OpC3_COPY_STRING_TO_ARRAY>());
    reg.Register(std::make_unique<OpC4_HELPER_CALL_ALT>());
    reg.Register(std::make_unique<OpC5_LOAD_SCHEDULED_TASK_ALT3>());
    reg.Register(std::make_unique<OpC6_WAIT_LOAD_SCHEDULER_ALT3>());
    reg.Register(std::make_unique<OpC7_END_LOAD_SCHEDULER_ALT3>());
    reg.Register(std::make_unique<OpC8_OPEN_MAP_WINDOW>());
    reg.Register(std::make_unique<OpC9_ENABLE_GAME_TIMER>());
    reg.Register(std::make_unique<OpCA_DEPRECATED_OPCODE_CA>());
    reg.Register(std::make_unique<OpCB_DEPRECATED_OPCODE_CB>());
    reg.Register(std::make_unique<OpCC_ITEM_INFO_WINDOW_HANDLER>());
    reg.Register(std::make_unique<OpCD_LOAD_SCHEDULED_TASK_ALT4>());
    reg.Register(std::make_unique<OpCE_WAIT_LOAD_SCHEDULER_ALT4>());
    reg.Register(std::make_unique<OpCF_END_LOAD_SCHEDULER_ALT4>());
    reg.Register(std::make_unique<OpD0_LOAD_SCHEDULED_TASK_ALT5>());
    reg.Register(std::make_unique<OpD1_WAIT_LOAD_SCHEDULER_ALT5>());
    reg.Register(std::make_unique<OpD2_END_LOAD_SCHEDULER_MAIN_ALT7>());
    reg.Register(std::make_unique<OpD3_CLEAR_ENTITY_MOTION_QUEUE>());
    reg.Register(std::make_unique<OpD4_MAP_QUERY_WINDOW_HANDLER>());
    reg.Register(std::make_unique<OpD5_LOAD_EVENT_SCHEDULER_ALT8>());
    reg.Register(std::make_unique<OpD6_WAIT_LOAD_SCHEDULER_ALT6>());
    reg.Register(std::make_unique<OpD7_END_LOAD_SCHEDULER_ALT6>());
    reg.Register(std::make_unique<OpD8_SET_ENTITY_EVENT_DIR>());
    reg.Register(std::make_unique<OpD9_SET_SOUND_EFFECT_LIMIT_FLAG>());
}