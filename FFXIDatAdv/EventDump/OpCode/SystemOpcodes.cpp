#include "OpCode.h"

// ─── 0x20: SET_CLI_EVENT_UC_FLAG ───
class Op20_SetCliEventUcFlag : public OpCode
{
public:
	uint8_t code() const override { return 0x20; }
	const char* name() const override { return "SET_CLI_EVENT_UC_FLAG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x23: WAIT_FOR_DIALOG_INTERACTION ───
class Op23_WaitDialogInteraction : public OpCode
{
public:
	uint8_t code() const override { return 0x23; }
	const char* name() const override { return "WAIT_FOR_DIALOG_INTERACTION"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x25: WAIT_DIALOG_SELECT ───
class Op25_WaitDialogSelect : public OpCode
{
public:
	uint8_t code() const override { return 0x25; }
	const char* name() const override { return "WAIT_DIALOG_SELECT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x42: SET_CLI_EVENT_CANCEL_DATA ───
class Op42_SetCliEventCancelData : public OpCode
{
public:
	uint8_t code() const override { return 0x42; }
	const char* name() const override { return "SET_CLI_EVENT_CANCEL_DATA"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x1C: WAIT ───
// Args: ticks (WORD)
class Op1C_Wait : public OpCode
{
public:
	uint8_t code() const override { return 0x1C; }
	const char* name() const override { return "WAIT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
};

// ─── Registration ───
void RegisterSystemOpcodes(OpCodeRegistry& reg)
{
	reg.Register(std::make_unique<Op20_SetCliEventUcFlag>());
	reg.Register(std::make_unique<Op23_WaitDialogInteraction>());
	reg.Register(std::make_unique<Op25_WaitDialogSelect>());
	reg.Register(std::make_unique<Op42_SetCliEventCancelData>());
	reg.Register(std::make_unique<Op1C_Wait>());
}
