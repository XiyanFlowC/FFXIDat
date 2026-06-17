#include "OpCode.h"

// ─── 0x00: END_REQSTACK ───
class Op00_EndReqStack : public OpCode
{
public:
	uint8_t code() const override { return 0x00; }
	const char* name() const override { return "END_REQSTACK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x01: GOTO ───
// Args: address (WORD)
class Op01_Goto : public OpCode
{
public:
	uint8_t code() const override { return 0x01; }
	const char* name() const override { return "GOTO"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }

	std::vector<uint32_t> JumpTargets(
		std::span<const uint8_t> data, size_t offset) const override
	{
		uint16_t target = ReadU16(data, offset + 1);
		return { target };
	}
};

// ─── 0x02: IF_CONDITIONAL ───
// Variable length, at minimum 2 bytes + branch
class Op02_If : public OpCode
{
public:
	uint8_t code() const override { return 0x02; }
	const char* name() const override { return "IF"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable, skip for Phase 1
};

// ─── 0x1A: CALL / JUMP_TO_POSITION ───
// Args: address (WORD)
class Op1A_Call : public OpCode
{
public:
	uint8_t code() const override { return 0x1A; }
	const char* name() const override { return "CALL"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }

	std::vector<uint32_t> JumpTargets(
		std::span<const uint8_t> data, size_t offset) const override
	{
		uint16_t target = ReadU16(data, offset + 1);
		return { target };
	}
};

// ─── 0x1B: RETURN ───
class Op1B_Return : public OpCode
{
public:
	uint8_t code() const override { return 0x1B; }
	const char* name() const override { return "RETURN"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x21: END_EVENT ───
class Op21_EndEvent : public OpCode
{
public:
	uint8_t code() const override { return 0x21; }
	const char* name() const override { return "END_EVENT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── Registration ───
void RegisterControlFlowOpcodes(OpCodeRegistry& reg)
{
	reg.Register(std::make_unique<Op00_EndReqStack>());
	reg.Register(std::make_unique<Op01_Goto>());
	reg.Register(std::make_unique<Op02_If>());
	reg.Register(std::make_unique<Op1A_Call>());
	reg.Register(std::make_unique<Op1B_Return>());
	reg.Register(std::make_unique<Op21_EndEvent>());
}
