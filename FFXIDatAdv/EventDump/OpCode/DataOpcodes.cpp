#include "OpCode.h"

// ─── 0x03: SET_WORK_ZONE ───
// Args: index (WORD), value (DWORD)
class Op03_SetWorkZone : public OpCode
{
public:
	uint8_t code() const override { return 0x03; }
	const char* name() const override { return "SET_WORK_ZONE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
};

// ─── 0x04: COPY_WORD ───
class Op04_CopyWord : public OpCode
{
public:
	uint8_t code() const override { return 0x04; }
	const char* name() const override { return "COPY_WORD"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};

// ─── 0x07: ADD ───
class Op07_Add : public OpCode
{
public:
	uint8_t code() const override { return 0x07; }
	const char* name() const override { return "ADD"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable
};

// ─── 0x08: SUBTRACT ───
class Op08_Subtract : public OpCode
{
public:
	uint8_t code() const override { return 0x08; }
	const char* name() const override { return "SUBTRACT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable
};

// ─── 0x09: BIT_SET ───
class Op09_BitSet : public OpCode
{
public:
	uint8_t code() const override { return 0x09; }
	const char* name() const override { return "BIT_SET"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable
};

// ─── Registration ───
void RegisterDataOpcodes(OpCodeRegistry& reg)
{
	reg.Register(std::make_unique<Op03_SetWorkZone>());
	reg.Register(std::make_unique<Op04_CopyWord>());
	reg.Register(std::make_unique<Op07_Add>());
	reg.Register(std::make_unique<Op08_Subtract>());
	reg.Register(std::make_unique<Op09_BitSet>());
}
