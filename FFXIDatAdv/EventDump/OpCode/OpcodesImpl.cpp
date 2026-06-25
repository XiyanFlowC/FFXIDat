#include "OpCode.h"

// ===== 0x00 END_REQSTACK =====
// Terminates the current request stack execution
class Op00 : public OpCode {
public:
	uint8_t code() const override { return 0x00; }
	const char* name() const override { return "END_REQSTACK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
	bool isTerminal() const override { return true; }
};

// ===== 0x01 GOTO =====
// Unconditional jump: branch_offset (WORD)
class Op01 : public OpCode {
public:
	uint8_t code() const override { return 0x01; }
	const char* name() const override { return "GOTO"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"offset", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"offset", hex(ru16(d, o+1)), ru16(d, o+1)}};
	}
	std::vector<uint32_t> jumpTargets(std::span<const uint8_t> d, size_t o) const override {
		return { ru16(d, o + 1) };
	}
};

// ===== 0x02 IF =====
// Conditional branch: condition + branch_offset
class Op02 : public OpCode {
public:
	uint8_t code() const override { return 0x02; }
	const char* name() const override { return "IF"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
	std::vector<ArgDef> args() const override {
		return {{"val1", ArgType::Word}, {"val2", ArgType::Word}, {"cond_type", ArgType::Byte}, {"else_offset", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {
			{"val1", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
			{"val2", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)},
			{"cond_type", std::to_string(ru8(d, o+5)), ru8(d, o+5)},
			{"else_offset", hex(ru16(d, o+6)), ru16(d, o+6)}
		};
	}
	std::vector<uint32_t> jumpTargets(std::span<const uint8_t> d, size_t o) const override {
		uint16_t else_off = ru16(d, o + 6);
		return { else_off, static_cast<uint32_t>(o + 8) };
	}
};

// ===== 0x03 SET_WORK =====
// Copy value from source work to destination work: dst_index (WORD), src_index (WORD)
class Op03 : public OpCode {
public:
	uint8_t code() const override { return 0x03; }
	const char* name() const override { return "SET_WORK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"dst", ArgType::Word}, {"src", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {
			{"dst", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
			{"src", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}
		};
	}
};

// ===== 0x04 DEPRECATED_NOP =====
// No-op / deprecated
class Op04 : public OpCode {
public:
	uint8_t code() const override { return 0x04; }
	const char* name() const override { return "DEPRECATED_NOP"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"val", ArgType::Word}}; }
};

// ===== 0x05 SET_ONE =====
class Op05 : public OpCode {
public:
	uint8_t code() const override { return 0x05; }
	const char* name() const override { return "SET_ONE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Word}}; }
};

// ===== 0x06 SET_ZERO =====
class Op06 : public OpCode {
public:
	uint8_t code() const override { return 0x06; }
	const char* name() const override { return "SET_ZERO"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Word}}; }
};

// ===== 0x07 ADD =====
class Op07 : public OpCode {
public:
	uint8_t code() const override { return 0x07; }
	const char* name() const override { return "ADD"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"value", ArgType::Word}};
	}
};

// ===== 0x08 SUBTRACT =====
class Op08 : public OpCode {
public:
	uint8_t code() const override { return 0x08; }
	const char* name() const override { return "SUB"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"value", ArgType::Word}};
	}
};

// ===== 0x09 BIT_SET =====
class Op09 : public OpCode {
public:
	uint8_t code() const override { return 0x09; }
	const char* name() const override { return "BIT_SET"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"index", ArgType::Word}, {"bit", ArgType::Word}};
	}
};

// ===== 0x0A BIT_CLEAR =====
class Op0A : public OpCode {
public:
	uint8_t code() const override { return 0x0A; }
	const char* name() const override { return "BIT_CLEAR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"index", ArgType::Word}, {"bit", ArgType::Word}};
	}
};

// ===== 0x0B INC =====
class Op0B : public OpCode {
public:
	uint8_t code() const override { return 0x0B; }
	const char* name() const override { return "INC"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Word}}; }
};

// ===== 0x0C DEC =====
class Op0C : public OpCode {
public:
	uint8_t code() const override { return 0x0C; }
	const char* name() const override { return "DEC"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Word}}; }
};

// ===== 0x0D AND =====
class Op0D : public OpCode {
public:
	uint8_t code() const override { return 0x0D; }
	const char* name() const override { return "AND"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"mask", ArgType::Word}};
	}
};

// ===== 0x0E OR =====
class Op0E : public OpCode {
public:
	uint8_t code() const override { return 0x0E; }
	const char* name() const override { return "OR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"mask", ArgType::Word}};
	}
};

// ===== 0x0F XOR =====
class Op0F : public OpCode {
public:
	uint8_t code() const override { return 0x0F; }
	const char* name() const override { return "XOR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"mask", ArgType::Word}};
	}
};

// ===== 0x10 SHL =====
class Op10 : public OpCode {
public:
	uint8_t code() const override { return 0x10; }
	const char* name() const override { return "SHL"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"bits", ArgType::Word}};
	}
};

// ===== 0x11 SHR =====
class Op11 : public OpCode {
public:
	uint8_t code() const override { return 0x11; }
	const char* name() const override { return "SHR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"bits", ArgType::Word}};
	}
};

// ===== 0x12 RANDOM =====
class Op12 : public OpCode {
public:
	uint8_t code() const override { return 0x12; }
	const char* name() const override { return "RANDOM"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Word}}; }
};

// ===== 0x13 RANDOM_RANGE =====
class Op13 : public OpCode {
public:
	uint8_t code() const override { return 0x13; }
	const char* name() const override { return "RANDOM_RANGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"dest", ArgType::Word}, {"max", ArgType::Word}};
	}
};

// ===== 0x14 MUL =====
class Op14 : public OpCode {
public:
	uint8_t code() const override { return 0x14; }
	const char* name() const override { return "MUL"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"value", ArgType::Word}};
	}
};

// ===== 0x15 DIV =====
class Op15 : public OpCode {
public:
	uint8_t code() const override { return 0x15; }
	const char* name() const override { return "DIV"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Word}, {"value", ArgType::Word}};
	}
};

// ===== 0x16 SIN =====
class Op16 : public OpCode {
public:
	uint8_t code() const override { return 0x16; }
	const char* name() const override { return "SIN"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"out", ArgType::Word}, {"angle", ArgType::Word}, {"multiplier", ArgType::Word}};
	}
};

// ===== 0x17 COS =====
class Op17 : public OpCode {
public:
	uint8_t code() const override { return 0x17; }
	const char* name() const override { return "COS"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"out", ArgType::Word}, {"angle", ArgType::Word}, {"multiplier", ArgType::Word}};
	}
};

// ===== 0x18 ATAN2 =====
class Op18 : public OpCode {
public:
	uint8_t code() const override { return 0x18; }
	const char* name() const override { return "ATAN2"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"out", ArgType::Word}, {"y", ArgType::Word}, {"x", ArgType::Word}};
	}
};

// ===== 0x19 SWAP =====
class Op19 : public OpCode {
public:
	uint8_t code() const override { return 0x19; }
	const char* name() const override { return "SWAP"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"a", ArgType::Word}, {"b", ArgType::Word}};
	}
};

// ===== 0x1A CALL =====
class Op1A : public OpCode {
public:
	uint8_t code() const override { return 0x1A; }
	const char* name() const override { return "CALL"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"target", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"target", hex(ru16(d, o+1)), ru16(d, o+1)}};
	}
	std::vector<uint32_t> jumpTargets(std::span<const uint8_t> d, size_t o) const override {
		return { ru16(d, o + 1) };
	}
};

// ===== 0x1B RETURN =====
class Op1B : public OpCode {
public:
	uint8_t code() const override { return 0x1B; }
	const char* name() const override { return "RETURN"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
	bool isTerminal() const override { return true; }
};

// ===== 0x1C WAIT =====
class Op1C : public OpCode {
public:
	uint8_t code() const override { return 0x1C; }
	const char* name() const override { return "WAIT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"ticks", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"ticks", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};

// Helper: resolve a message argument that may be a work address.
// Returns true and sets resolvedMsg if the message IS a static string index.
// Returns false if the argument is a dynamic work address (caller should skip or placeholder).
static bool TryResolveMessage(uint16_t raw, const OpCodeContext& ctx, uint32_t& resolvedMsg)
{
	if (raw & 0x8000)
	{
		// References entry — resolve through imed_data, result IS a string index
		resolvedMsg = ctx.ResolveRef(raw);
		return true;
	}
	if (WorkAddr::IsValid(raw))
	{
		// Work area address (WorkLocal, Work_Zone, etc.) — runtime dynamic, can't resolve
		return false;
	}
	// An invalid value (not a work address, not a reference) -- game treats it as zero.
	resolvedMsg = 0;
	return true;
}

// ===== 0x1D PRINT_EVENT_MESSAGE =====
// Speaker: implicit EventEntity
class Op1D : public OpCode {
public:
	uint8_t code() const override { return 0x1D; }
	const char* name() const override { return "PRINT_EVENT_MESSAGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"message_id", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"message_id", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
	void extract(std::vector<DialogueLine>& out, const OpCodeContext& ctx,
		std::span<const uint8_t> d, size_t o) const override
	{
		uint16_t raw = ru16(d, o+1);
		uint32_t msg;
		if (TryResolveMessage(raw, ctx, msg))
			out.push_back({resolve_speaker(ctx, ctx.current_entity_id), resolve_string(ctx, msg), msg, msg});
	}
};

// ===== 0x1E LOOK_AT_AND_TALK =====
class Op1E : public OpCode {
public:
	uint8_t code() const override { return 0x1E; }
	const char* name() const override { return "LOOK_AT_AND_TALK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"target", ArgType::Dword}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"target", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)}};
	}
};

// ===== 0x1F MOVE_ENTITY =====
// Variable length: mode byte at +1. mode==0 (init): 8 bytes (opcode + mode + 3 WORDs x/z/y).
// mode==1 (update/tick): 2 bytes (opcode + mode), yields until entity reaches destination.
class Op1F : public OpCode {
public:
	uint8_t code() const override { return 0x1F; }
	const char* name() const override { return "MOVE_ENTITY"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o < d.size()) {
			uint8_t mode = ru8(d, o + 1);
			if (mode == 0) return 8;
		}
		return 2;
	}
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"x", ArgType::Word}, {"z", ArgType::Word}, {"y", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		(void)ctx;
		uint8_t mode = ru8(d, o + 1);
		if (mode == 0) {
			return {
				{"mode", std::to_string(mode), mode},
				{"x", std::to_string(ru16(d, o+2)), ru16(d, o+2)},
				{"z", std::to_string(ru16(d, o+4)), ru16(d, o+4)},
				{"y", std::to_string(ru16(d, o+6)), ru16(d, o+6)}
			};
		}
		return {{"mode", std::to_string(mode), mode}};
	}
};

// ===== 0x20 SET_CLI_EVENT_UC_FLAG =====
// Sets the CliEventUcFlag (locks/unlocks player control): flag (BYTE)
class Op20 : public OpCode {
public:
	uint8_t code() const override { return 0x20; }
	const char* name() const override { return "LOCK_PLAYER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"flag", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};

// ===== 0x21 END_EVENT =====
class Op21 : public OpCode {
public:
	uint8_t code() const override { return 0x21; }
	const char* name() const override { return "END_EVENT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
	bool isTerminal() const override { return true; }
};

// ===== 0x22 ENTITY_HIDE_FLAG =====
// Sets event hide flag for current entity: enabled (BYTE)
class Op22 : public OpCode {
public:
	uint8_t code() const override { return 0x22; }
	const char* name() const override { return "HIDE_FLAG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"enabled", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"enabled", std::to_string(ru8(d,o+1)), ru8(d,o+1)}};
	}
};

// ===== 0x23 WAIT_FOR_DIALOG =====
class Op23 : public OpCode {
public:
	uint8_t code() const override { return 0x23; }
	const char* name() const override { return "WAIT_DIALOG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x24 CREATE_DIALOG =====
class Op24 : public OpCode {
public:
	uint8_t code() const override { return 0x24; }
	const char* name() const override { return "CREATE_DIALOG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"message_id", ArgType::Word}, {"default_opt", ArgType::Word}, {"opt_flags", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"message_id", refstr(ctx, ru16(d,o+1)), ru16(d,o+1)},
				{"default", refstr(ctx, ru16(d,o+3)), ru16(d,o+3)},
				{"flags", refstr(ctx, ru16(d,o+5)), ru16(d,o+5)}};
	}
	void extract(std::vector<DialogueLine>& out, const OpCodeContext& ctx,
		std::span<const uint8_t> d, size_t o) const override
	{
		uint16_t raw = ru16(d, o+1);
		uint32_t msg;
		if (TryResolveMessage(raw, ctx, msg))
			out.push_back({resolve_speaker(ctx, ctx.current_entity_id), resolve_string(ctx, msg), msg, msg});
	}
};

// ===== 0x25 WAIT_DIALOG_SELECT =====
class Op25 : public OpCode {
public:
	uint8_t code() const override { return 0x25; }
	const char* name() const override { return "WAIT_SELECT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x26 YIELD_VM =====
class Op26 : public OpCode {
public:
	uint8_t code() const override { return 0x26; }
	const char* name() const override { return "YIELD"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x27 REQ_SET =====
// Sets a ReqStack entry: entity_id (DWORD at offset 2), priority (BYTE at offset 1), tagNum (BYTE at offset 6)
class Op27 : public OpCode {
public:
	uint8_t code() const override { return 0x27; }
	const char* name() const override { return "REQ_SET"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Dword}, {"priority", ArgType::Byte}, {"tag", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)},
				{"priority", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"tag", std::to_string(ru8(d, o+6)), ru8(d, o+6)}};
	}
};

// ===== 0x28 REQ_SET_COND =====
// Similar to 0x27 with extra condition checks: entity_id (DWORD at offset 2), priority (BYTE at offset 1), tagNum (BYTE at offset 6)
class Op28 : public OpCode {
public:
	uint8_t code() const override { return 0x28; }
	const char* name() const override { return "REQ_SET_COND"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Dword}, {"priority", ArgType::Byte}, {"tag", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)},
				{"priority", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"tag", std::to_string(ru8(d, o+6)), ru8(d, o+6)}};
	}
};

// ===== 0x29 REQ_SET_WAIT =====
// Similar to 0x28 with different wait semantics: entity_id (DWORD at offset 2), priority (BYTE at offset 1), tagNum (BYTE at offset 6)
class Op29 : public OpCode {
public:
	uint8_t code() const override { return 0x29; }
	const char* name() const override { return "REQ_SET_WAIT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Dword}, {"priority", ArgType::Byte}, {"tag", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)},
				{"priority", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"tag", std::to_string(ru8(d, o+6)), ru8(d, o+6)}};
	}
};

// ===== 0x2A GET_REQ_LEVEL =====
// Gets request priority level: priority (BYTE at offset 1), entity_id (DWORD at offset 2)
class Op2A : public OpCode {
public:
	uint8_t code() const override { return 0x2A; }
	const char* name() const override { return "GET_REQ_LEVEL"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"priority", ArgType::Byte}, {"entity_id", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"priority", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)}};
	}
};

// ===== 0x2B PRINT_ENTITY_MESSAGE =====
// Explicit speaker + message
class Op2B : public OpCode {
public:
	uint8_t code() const override { return 0x2B; }
	const char* name() const override { return "PRINT_ENTITY_MESSAGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Dword}, {"message_id", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d,o+1)), ru32(d,o+1)},
				{"message_id", refstr(ctx, ru16(d,o+5)), ru16(d,o+5)}};
	}
	void extract(std::vector<DialogueLine>& out, const OpCodeContext& ctx,
		std::span<const uint8_t> d, size_t o) const override
	{
		uint16_t raw = ru16(d, o+5);
		uint32_t msg;
		if (TryResolveMessage(raw, ctx, msg))
			out.push_back({resolve_speaker(ctx, ru32(d, o+1)), resolve_string(ctx, msg), msg, msg});
	}
};

// ===== 0x2C CREATE_SCHEDULER_TASK =====
// Schedules an action on an entity: entity1 (DWORD at offset 1), entity2 (DWORD at offset 5), action_id (DWORD at offset 9)
class Op2C : public OpCode {
public:
	uint8_t code() const override { return 0x2C; }
	const char* name() const override { return "CREATE_SCHEDULER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
	std::vector<ArgDef> args() const override {
		return {{"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action_id", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity1", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+5)), ru32(d, o+5)},
				{"action", std::to_string(ru32(d, o+9)), ru32(d, o+9)}};
	}
};

// ===== 0x2D CREATE_ZONE_SCHEDULER =====
// Zone-based scheduler: entity1 (DWORD at offset 1), entity2 (DWORD at offset 5), action_id (DWORD at offset 9)
class Op2D : public OpCode {
public:
	uint8_t code() const override { return 0x2D; }
	const char* name() const override { return "CREATE_ZONE_SCHEDULER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
	std::vector<ArgDef> args() const override {
		return {{"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action_id", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity1", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+5)), ru32(d, o+5)},
				{"action", std::to_string(ru32(d, o+9)), ru32(d, o+9)}};
	}
};

// ===== 0x2E SET_CANCEL_FLAGS =====
class Op2E : public OpCode {
public:
	uint8_t code() const override { return 0x2E; }
	const char* name() const override { return "SET_CANCEL_FLAGS"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x2F ADJUST_RENDER_FLAGS0 =====
// Adjusts Render.Flags0: flag_bit (BYTE at offset 1), entity_id (DWORD at offset 2)
class Op2F : public OpCode {
public:
	uint8_t code() const override { return 0x2F; }
	const char* name() const override { return "ADJ_RENDER0"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"flag", ArgType::Byte}, {"entity_id", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)}};
	}
};

// ===== 0x30 SET_UCOFF =====
class Op30 : public OpCode {
public:
	uint8_t code() const override { return 0x30; }
	const char* name() const override { return "SET_UCOFF"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x31 UPDATE_ENTITY_POS =====
class Op31 : public OpCode {
public:
	uint8_t code() const override { return 0x31; }
	const char* name() const override { return "UPDATE_POS"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 < d.size() && ru8(d, o+1) == 1) return 2;
		return 10;
	}
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"x", ArgType::Word}, {"z", ArgType::Word}, {"y", ArgType::Word}, {"time", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		uint8_t mode = ru8(d, o+1);
		if (mode == 0) {
			return {
				{"mode", std::to_string(mode), mode},
				{"x", refstr(ctx, ru16(d, o+2)), ru16(d, o+2)},
				{"z", refstr(ctx, ru16(d, o+4)), ru16(d, o+4)},
				{"y", refstr(ctx, ru16(d, o+6)), ru16(d, o+6)},
				{"time", refstr(ctx, ru16(d, o+8)), ru16(d, o+8)}
			};
		}
		return {{"mode", std::to_string(mode), mode}};
	}
};

// ===== 0x32 SET_SPEED =====
class Op32 : public OpCode {
public:
	uint8_t code() const override { return 0x32; }
	const char* name() const override { return "SET_SPEED"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"speed", ArgType::Word}}; }
};

// ===== 0x33 ADJ_EVENT_RENDER0 =====
class Op33 : public OpCode {
public:
	uint8_t code() const override { return 0x33; }
	const char* name() const override { return "ADJ_EV_RENDER0"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"flag", ArgType::Byte}}; }
};

// ===== 0x34 LOAD_ZONE =====
class Op34 : public OpCode {
public:
	uint8_t code() const override { return 0x34; }
	const char* name() const override { return "LOAD_ZONE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"zone_id", ArgType::Word}}; }
};

// ===== 0x35 LOAD_ZONE_NO_CLOSE =====
class Op35 : public OpCode {
public:
	uint8_t code() const override { return 0x35; }
	const char* name() const override { return "LOAD_ZONE_NC"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"zone_id", ArgType::Word}}; }
};

// ===== 0x36 SET_EVENT_POS =====
class Op36 : public OpCode {
public:
	uint8_t code() const override { return 0x36; }
	const char* name() const override { return "SET_EVENT_POS"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"x", ArgType::Word}, {"z", ArgType::Word}, {"y", ArgType::Word}};
	}
};

// ===== 0x37 UPDATE_POS_AND_DIR =====
class Op37 : public OpCode {
public:
	uint8_t code() const override { return 0x37; }
	const char* name() const override { return "UPDATE_POS_DIR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
	std::vector<ArgDef> args() const override {
		return {{"x", ArgType::Word}, {"z", ArgType::Word}, {"y", ArgType::Word}, {"dir", ArgType::Word}};
	}
};

// ===== 0x38 SET_CLIENT_MODE =====
class Op38 : public OpCode {
public:
	uint8_t code() const override { return 0x38; }
	const char* name() const override { return "SET_CLIENT_MODE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"mode", ArgType::Word}}; }
};

// ===== 0x39 SET_DIRECTION =====
class Op39 : public OpCode {
public:
	uint8_t code() const override { return 0x39; }
	const char* name() const override { return "SET_DIR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"dir", ArgType::Word}}; }
};

// ===== 0x3A CONVERT_YAW =====
class Op3A : public OpCode {
public:
	uint8_t code() const override { return 0x3A; }
	const char* name() const override { return "CONVERT_YAW"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Dword}, {"out", ArgType::Word}};
	}
};

// ===== 0x3B GET_ENTITY_POS =====
class Op3B : public OpCode {
public:
	uint8_t code() const override { return 0x3B; }
	const char* name() const override { return "GET_ENTITY_POS"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 11; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Dword}, {"out_x", ArgType::Word}, {"out_y", ArgType::Word}, {"out_z", ArgType::Word}};
	}
};

// ===== 0x3C-0x3F BIT_COND ops =====
class Op3C : public OpCode { public:
	uint8_t code() const override { return 0x3C; } const char* name() const override { return "BIT_SET_IF"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override { return {{"bit_value", ArgType::Word}, {"bound", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"bit_value", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}, {"bound", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};
class Op3D : public OpCode { public:
	uint8_t code() const override { return 0x3D; } const char* name() const override { return "BIT_CLEAR_IF"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override { return {{"bit_value", ArgType::Word}, {"bound", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"bit_value", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}, {"bound", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};
class Op3E : public OpCode { public:
	uint8_t code() const override { return 0x3E; } const char* name() const override { return "TEST_BIT"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override { return {{"bit_value", ArgType::Word}, {"else_offset", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"bit_value", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}, {"else_offset", hex(ru16(d, o+5)), ru16(d, o+5)}};
	}
};
class Op3F : public OpCode { public:
	uint8_t code() const override { return 0x3F; } const char* name() const override { return "MOD"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override { return {{"value1", ArgType::Word}, {"value2", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"value1", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}, {"value2", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};

// ===== 0x40-0x41 BIT_WORK =====
class Op40 : public OpCode {
public:
	uint8_t code() const override { return 0x40; } const char* name() const override { return "BIT_WORK_SET"; } size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
	std::vector<ArgDef> args() const override {
		return {{"bit_start", ArgType::Word}, {"bit_end", ArgType::Word}, {"src", ArgType::Word}, {"dst", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {
			{"bit_start", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
			{"bit_end", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)},
			{"src", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)},
			{"dst", refstr(ctx, ru16(d, o+7)), ru16(d, o+7)}
		};
	}
};
class Op41 : public OpCode {
public:
	uint8_t code() const override { return 0x41; } const char* name() const override { return "BIT_WORK_GET"; } size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
	std::vector<ArgDef> args() const override {
		return {{"bit_start", ArgType::Word}, {"bit_end", ArgType::Word}, {"src", ArgType::Word}, {"dst", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {
			{"bit_start", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
			{"bit_end", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)},
			{"src", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)},
			{"dst", refstr(ctx, ru16(d, o+7)), ru16(d, o+7)}
		};
	}
};

// ===== 0x42 SET_CANCEL_DATA =====
class Op42 : public OpCode {
public:
	uint8_t code() const override { return 0x42; }
	const char* name() const override { return "SET_CANCEL_DATA"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x43 SEND_EVENT_UPDATE =====
class Op43 : public OpCode {
public:
	uint8_t code() const override { return 0x43; }
	const char* name() const override { return "SEND_UPDATE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"mode", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"mode", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};

// ===== 0x44 IF_ENTITY_VALID =====
class Op44 : public OpCode {
public:
	uint8_t code() const override { return 0x44; }
	const char* name() const override { return "IF_ENTITY_VALID"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"entity_id", ArgType::Word}, {"else_offset", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {
			{"entity_id", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
			{"else_offset", hex(ru16(d, o+3)), ru16(d, o+3)}
		};
	}
};

// ===== 0x45 LOAD_SCHEDULED_TASK =====
class Op45 : public OpCode {
public:
	uint8_t code() const override { return 0x45; }
	const char* name() const override { return "LOAD_SCHEDULED_TASK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
	std::vector<ArgDef> args() const override {
		return {{"param", ArgType::Word}, {"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"scheduler_id", ArgType::Dword}, {"val", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {
			{"param", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
			{"entity1", hex(ru32(d, o+3)), ru32(d, o+3)},
			{"entity2", hex(ru32(d, o+7)), ru32(d, o+7)},
			{"scheduler_id", hex(ru32(d, o+11)), ru32(d, o+11)},
			{"val", refstr(ctx, ru16(d, o+15)), ru16(d, o+15)}
		};
	}
};

// ===== 0x46 CAMERA =====
class Op46 : public OpCode {
public:
	uint8_t code() const override { return 0x46; }
	const char* name() const override { return "CAMERA"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o < d.size()) { uint8_t m = ru8(d, o+1); if (m == 2) return 4; }
		return 2;
	}
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"out", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		uint8_t mode = ru8(d, o+1);
		if (mode == 2) return {{"mode", std::to_string(mode), mode}, {"out", std::to_string(ru16(d, o+2)), ru16(d, o+2)}};
		return {{"mode", std::to_string(mode), mode}};
	}
};

// ===== 0x47 UPDATE_PLAYER_LOC =====
class Op47 : public OpCode {
public:
	uint8_t code() const override { return 0x47; }
	const char* name() const override { return "UPDATE_PLAYER_LOC"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o < d.size() && ru8(d, o+1) == 0) return 10;
		return 2;
	}
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"pos_x", ArgType::Word}, {"pos_y", ArgType::Word}, {"pos_z", ArgType::Word}, {"rotation", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		uint8_t mode = ru8(d, o+1);
		if (mode == 0) return {
			{"mode", std::to_string(mode), mode},
			{"pos_x", refstr(ctx, ru16(d, o+2)), ru16(d, o+2)},
			{"pos_y", refstr(ctx, ru16(d, o+4)), ru16(d, o+4)},
			{"pos_z", refstr(ctx, ru16(d, o+6)), ru16(d, o+6)},
			{"rotation", refstr(ctx, ru16(d, o+8)), ru16(d, o+8)}
		};
		return {{"mode", std::to_string(mode), mode}};
	}
};

// ===== 0x48 PRINT_MESSAGE (system) =====
class Op48 : public OpCode {
public:
	uint8_t code() const override { return 0x48; }
	const char* name() const override { return "PRINT_MESSAGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"message_id", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"message_id", refstr(ctx, ru16(d,o+1)), ru16(d,o+1)}};
	}
	void extract(std::vector<DialogueLine>& out, const OpCodeContext& ctx,
		std::span<const uint8_t> d, size_t o) const override
	{
		uint16_t raw = ru16(d, o+1);
		uint32_t msg;
		if (TryResolveMessage(raw, ctx, msg))
			out.push_back({"[System]", resolve_string(ctx, msg), msg, msg});
	}
};

// ===== 0x49 PRINT_EVENT_MESSAGE_NO_SPEAKER =====
class Op49 : public OpCode {
public:
	uint8_t code() const override { return 0x49; }
	const char* name() const override { return "PRINT_MSG_NOSPK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"target", ArgType::Dword}, {"message_id", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"target", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"message_id", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
	// 0x49 does not produce visible dialogue text (confirmed by XiEvents/FFXI-EventsDump)
	// void extract(...) override - intentionally omitted
};

// ===== 0x4A ENTITY_LOOK_AT =====
class Op4A : public OpCode {
public:
	uint8_t code() const override { return 0x4A; }
	const char* name() const override { return "LOOK_AT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
	std::vector<ArgDef> args() const override {
		return {{"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity1", resolve_speaker(ctx, ru32(d,o+1)), ru32(d,o+1)},
				{"entity2", resolve_speaker(ctx, ru32(d,o+5)), ru32(d,o+5)}};
	}
};

// ===== 0x4B UPDATE_YAW =====
class Op4B : public OpCode {
public:
	uint8_t code() const override { return 0x4B; } const char* name() const override { return "UPDATE_YAW"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity", ArgType::Dword}, {"yaw", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"yaw", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};

// ===== 0x4C-0x4F entity status ops =====
class Op4C : public OpCode { public:
	uint8_t code() const override { return 0x4C; } const char* name() const override { return "STATUS_DOOR"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};
class Op4D : public OpCode { public:
	uint8_t code() const override { return 0x4D; } const char* name() const override { return "STATUS_CLOSE_DOOR"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};
class Op4E : public OpCode {
public:
	uint8_t code() const override { return 0x4E; } const char* name() const override { return "SET_HIDE_FLAG"; } size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override { return {{"hide", ArgType::Byte}, {"entity_id", ArgType::Dword}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"hide", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity_id", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)}};
	}
};
class Op4F : public OpCode {
public:
	uint8_t code() const override { return 0x4F; } const char* name() const override { return "STATUS_CUSTOM"; } size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"value", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"value", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};

// ===== 0x50-0x55 Scheduler ops =====
class Op50 : public OpCode {
public:
	uint8_t code() const override { return 0x50; } const char* name() const override { return "END_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
	std::vector<ArgDef> args() const override {
		return {{"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity1", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+5)), ru32(d, o+5)},
				{"action", std::to_string(ru32(d, o+9)), ru32(d, o+9)}};
	}
};
class Op51 : public OpCode {
public:
	uint8_t code() const override { return 0x51; } const char* name() const override { return "END_MAP_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
	std::vector<ArgDef> args() const override {
		return {{"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity1", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+5)), ru32(d, o+5)},
				{"action", std::to_string(ru32(d, o+9)), ru32(d, o+9)}};
	}
};
class Op52 : public OpCode {
public:
	uint8_t code() const override { return 0x52; } const char* name() const override { return "END_LOAD_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
	std::vector<ArgDef> args() const override {
		return {{"param", ArgType::Word}, {"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"param", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"entity1", resolve_speaker(ctx, ru32(d, o+3)), ru32(d, o+3)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+7)), ru32(d, o+7)},
				{"action", std::to_string(ru32(d, o+11)), ru32(d, o+11)}};
	}
};
class Op53 : public OpCode {
public:
	uint8_t code() const override { return 0x53; } const char* name() const override { return "WAIT_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
	std::vector<ArgDef> args() const override {
		return {{"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity1", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+5)), ru32(d, o+5)},
				{"action", std::to_string(ru32(d, o+9)), ru32(d, o+9)}};
	}
};
class Op54 : public OpCode { public:
	uint8_t code() const override { return 0x54; } const char* name() const override { return "WAIT_MAP_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
};
class Op55 : public OpCode { public:
	uint8_t code() const override { return 0x55; } const char* name() const override { return "WAIT_LOAD_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
};

// ===== 0x56 GET_ACTOR_INDEX =====
// Deprecated: reads eventgetcode2 at offset 1 but does nothing with it
class Op56 : public OpCode { public:
	uint8_t code() const override { return 0x56; } const char* name() const override { return "GET_ACTOR_INDEX"; } size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Dword}}; }
};

// ===== 0x57 CREATE_FRAME_DELAY =====
// Reads getworkofs_ at offset 1, writes frame_delay + val back
class Op57 : public OpCode { public:
	uint8_t code() const override { return 0x57; } const char* name() const override { return "FRAME_DELAY"; } size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"value", ArgType::Word}}; }
};

// ===== 0x58 YIELD_VM =====
class Op58 : public OpCode { public:
	uint8_t code() const override { return 0x58; } const char* name() const override { return "YIELD"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x59 UPDATE_ENTITY_DATA_MULTI =====
// Variable length: 4/4/4 (sub 0/2/7), 6 (sub 6), 7 (sub 5), 8 (sub 1/3/4/8)
class Op59 : public OpCode { public:
	uint8_t code() const override { return 0x59; } const char* name() const override { return "UPD_ENTITY_DATA"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 4;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
			case 0:  return 4;
			case 1:  return 8;
			case 2:  return 4;
			case 3:  return 8;
			case 4:  return 8;
			case 5:  return 7;
			case 6:  return 6;
			case 7:  return 4;
			case 8:  return 8;
			default: return 4;
		}
	}
};

// ===== 0x5A UPDATE_EVENT_POS =====
// Variable length: 8 (sub=0: init, 3 getworkofs WORDs), 2 (sub=1: update, yields until done)
class Op5A : public OpCode { public:
	uint8_t code() const override { return 0x5A; } const char* name() const override { return "UPD_EVENT_POS"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		return (ru8(d, o + 1) == 0) ? 8 : 2;
	}
};

// ===== 0x5B LOAD_EXT_SCHEDULER =====
class Op5B : public OpCode {
public:
	uint8_t code() const override { return 0x5B; }
	const char* name() const override { return "LOAD_EXT_SCHEDULER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 15; }
	std::vector<ArgDef> args() const override {
		return {{"work_off", ArgType::Word}, {"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}, {"action", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		uint16_t wo = ru16(d, o+1);
		return {{"work_off", refstr(ctx, wo), wo},
				{"entity1", resolve_speaker(ctx, ru32(d, o+3)), ru32(d, o+3)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+7)), ru32(d, o+7)},
				{"action", std::to_string(ru32(d, o+11)), ru32(d, o+11)}};
	}
};

// ===== 0x5C MUSIC =====
// Variable length: 4 bytes for cases 0-7, 6 bytes for cases 0x80-0x87/0xA0-0xA3
class Op5C : public OpCode {
public:
	uint8_t code() const override { return 0x5C; } const char* name() const override { return "MUSIC"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 4;
		uint8_t sub = ru8(d, o + 1) & 0x7;
		uint8_t cmd = ru8(d, o + 1);
		if (sub < 8 && cmd < 0x80) return 4;
		return 6;
	}
	std::vector<ArgDef> args() const override { return {{"cmd", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"cmd", std::to_string(ru8(d,o+1)), ru8(d,o+1)}};
	}
};

// ===== 0x5D SET_VOLUME =====
// Sets/eases music volume: volume (getworkofs WORD), time (getworkofs WORD)
class Op5D : public OpCode { public:
	uint8_t code() const override { return 0x5D; } const char* name() const override { return "SET_VOLUME"; } size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"volume", ArgType::Word}, {"time", ArgType::Word}}; }
};

// ===== 0x5E STOP_ACTION =====
// Stops entity action, sets idle motion: motion_id (DWORD via eventgetcode2)
class Op5E : public OpCode {
public:
	uint8_t code() const override { return 0x5E; } const char* name() const override { return "STOP_ACTION"; } size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"motion_id", ArgType::Dword}}; }
};

// ===== 0x5F MULTI_HANDLER =====
// Variable length sub-opcode dispatcher: 2 (sub 0/1), 6 (sub 2 -> 0xC1), 16 (sub 3/4 -> 0x5B p3=0), 18 (sub 5/6 -> 0x5B p3=1), 14 (sub 7 -> 0x53)
class Op5F : public OpCode { public:
	uint8_t code() const override { return 0x5F; } const char* name() const override { return "MULTI_HANDLER"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
			case 0: case 1:  return 2;
			case 2:          return 6;   // calls KILL_ACTION (5 bytes)
			case 3: case 4:  return 16;  // calls LOAD_EXT_SCHEDULER (15 bytes, param3=0)
			case 5: case 6:  return 18;  // calls LOAD_EXT_SCHEDULER (17 bytes, param3=1)
			case 7:          return 14;  // calls WAIT_SCHEDULER (13 bytes)
			default:         return 2;
		}
	}
};

// ===== 0x60 ADJ_RENDER1_MULTI =====
// Variable length sub-opcode: 2 (default/fallback), 4 (sub 0/1 adjust render), 6 (sub 2 set action)
class Op60 : public OpCode {
public:
	uint8_t code() const override { return 0x60; }
	const char* name() const override { return "ADJ_RENDER1_MULTI"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		if (sub <= 1) return 4;
		if (sub == 2) return 6;
		return 2;
	}
	std::vector<ArgDef> args() const override {
		return {{"sub_code", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub_code", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};

// ===== 0x96 UNSET_EVENT_NPC =====
class Op96 : public OpCode { public:
	uint8_t code() const override { return 0x96; } const char* name() const override { return "UNSET_EVENT_NPC"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};
class Op97 : public OpCode {
public:
	uint8_t code() const override { return 0x97; }
	const char* name() const override { return "SAVE_WIND"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"wind_base", ArgType::Word}, {"wind_width", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"wind_base", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"wind_width", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}};
	}
};
class Op98 : public OpCode { public: uint8_t code() const override { return 0x98; } const char* name() const override { return "YIELD_IF_LOADING"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op99 : public OpCode {
public:
	uint8_t code() const override { return 0x99; }
	const char* name() const override { return "WAIT_ANIM"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"entity", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"entity", hex(ru16(d, o+1)), ru16(d, o+1)}};
	}
};
class Op9A : public OpCode { public: uint8_t code() const override { return 0x9A; } const char* name() const override { return "WAIT_MUSIC"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op9B : public OpCode { public: uint8_t code() const override { return 0x9B; } const char* name() const override { return "WAIT_ENTITY_ANIM"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op9C : public OpCode { public: uint8_t code() const override { return 0x9C; } const char* name() const override { return "STORE_LANG"; } size_t length(std::span<const uint8_t>, size_t) const override { return 3; } };
class Op9D : public OpCode { public:
	uint8_t code() const override { return 0x9D; }
	const char* name() const override { return "STRING_HANDLER"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 3;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
		case 0x00: return 8;
		case 0x01: return 8;
		case 0x02: return 6;
		case 0x03: return 8;
		case 0x04: return 8;
		case 0x05: return 8;
		case 0x06: return 8;
		case 0x07: return 6;
		case 0x08: return 23;
		case 0x09: return 9;
		case 0x0A: return 10;
		case 0x0B: return 10;
		case 0x0C: return 8;
		case 0x0D: return 10;
		case 0x0E: return 10;
		case 0x0F: return 10;
		case 0x10: return 10;
		default:  return 3;
		}
	}
	std::vector<ArgDef> args() const override { return {{"sub_op", ArgType::Byte}}; }
};
class Op9E : public OpCode { public:
	uint8_t code() const override { return 0x9E; }
	const char* name() const override { return "SET_RECT_SEND"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"flag", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op9F : public OpCode { public: uint8_t code() const override { return 0x9F; } const char* name() const override { return "LOAD_SCHED_ALT"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class OpA0 : public OpCode { public: uint8_t code() const override { return 0xA0; } const char* name() const override { return "WAIT_SCHED_A0"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpA1 : public OpCode { public: uint8_t code() const override { return 0xA1; } const char* name() const override { return "END_SCHED_A1"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpA2 : public OpCode { public: uint8_t code() const override { return 0xA2; } const char* name() const override { return "WAIT_SCHED_A2"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpA3 : public OpCode { public: uint8_t code() const override { return 0xA3; } const char* name() const override { return "END_SCHED_A3"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpA4 : public OpCode { public:
	uint8_t code() const override { return 0xA4; }
	const char* name() const override { return "ADJ_RENDER_B26"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"flag", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class OpA5 : public OpCode { public:
	uint8_t code() const override { return 0xA5; }
	const char* name() const override { return "ADJ_RENDER_B11"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"flag", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class OpA6 : public OpCode { public:
	uint8_t code() const override { return 0xA6; }
	const char* name() const override { return "REQ_MAP_NO"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		if (sub == 2) return 4;
		return 2;
	}
	std::vector<ArgDef> args() const override { return {{"cmd", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"cmd", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class OpA7 : public OpCode { public:
	uint8_t code() const override { return 0xA7; }
	const char* name() const override { return "BATTLE_WAIT"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		if (sub == 1) return 4;
		return 2;
	}
	std::vector<ArgDef> args() const override { return {{"cmd", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"cmd", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
// ===== 0xA8 MAP_MARKER_CTRL =====
// mode(Byte) + val1(Word) + val2(Word) = 6 bytes
class OpA8 : public OpCode {
public:
	uint8_t code() const override { return 0xA8; }
	const char* name() const override { return "MAP_MARKER_CTRL"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"val1", ArgType::Word}, {"val2", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"mode", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"val1", refstr(ctx, ru16(d, o+2)), ru16(d, o+2)},
				{"val2", refstr(ctx, ru16(d, o+4)), ru16(d, o+4)}};
	}
};
// ===== 0xA9 DISABLE_TIME =====
// val(Word) = 3 bytes
class OpA9 : public OpCode {
public:
	uint8_t code() const override { return 0xA9; }
	const char* name() const override { return "DISABLE_TIME"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override {
		return {{"val", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"val", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};
// ===== 0xAA VDATE_CONV =====
// timestamp_src(Word) + 6 output Word indices = 17 bytes
class OpAA : public OpCode {
public:
	uint8_t code() const override { return 0xAA; }
	const char* name() const override { return "VDATE_CONV"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 17; }
	std::vector<ArgDef> args() const override {
		return {{"timestamp", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"timestamp", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};
// ===== 0xAB ADJ_ENTITY_FLAGS =====
// Variable length: 2 (sub-only), 4 (sub+word), 6 (sub+entity+word)
class OpAB : public OpCode {
public:
	uint8_t code() const override { return 0xAB; }
	const char* name() const override { return "ADJ_ENTITY_FLAGS"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o+1);
		switch (sub) {
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04:
		case 0x05: case 0x06: case 0x07: case 0x08: case 0x09:
		case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E:
		case 0x0F: case 0x10: case 0x12: case 0x13: case 0x19:
		case 0x1A: return 2;
		case 0x11: case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: return 4;
		case 0x1B: case 0x1C: return 6;
		default:   return 2;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
// ===== 0xAC ENTITY_STATUS =====
// Variable length: 4 (sub+word), 6 (sub+entity), 8 (sub+entity+word)
class OpAC : public OpCode {
public:
	uint8_t code() const override { return 0xAC; }
	const char* name() const override { return "ENTITY_STATUS"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 4;
		uint8_t sub = ru8(d, o+1);
		switch (sub) {
		case 0x00: case 0x01: return 4;
		case 0x02: case 0x03: return 6;
		case 0x04: return 8;
		default:   return 4;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
// ===== 0xAD DUAL_ENTITY_SCHED =====
// sub(Byte) + arg(Word) + entity1(Dword) + entity2(Dword) = 12 bytes
class OpAD : public OpCode {
public:
	uint8_t code() const override { return 0xAD; }
	const char* name() const override { return "DUAL_ENTITY_SCHED"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 12; }
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}, {"arg", ArgType::Word}, {"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"arg", refstr(ctx, ru16(d, o+2)), ru16(d, o+2)},
				{"entity1", resolve_speaker(ctx, ru32(d, o+4)), ru32(d, o+4)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+8)), ru32(d, o+8)}};
	}
};
// ===== 0xAE MULTI_ENTITY =====
// Variable length: 6, 8, or 10 bytes depending on sub-case
class OpAE : public OpCode {
public:
	uint8_t code() const override { return 0xAE; }
	const char* name() const override { return "MULTI_ENTITY"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o+1);
		switch (sub) {
		case 0x00: case 0x06: return 6;
		case 0x01: case 0x02: case 0x03: case 0x04: return 8;
		case 0x05: case 0x07: case 0x08: return 10;
		default:   return 2;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
// ===== 0xAF GET_CAMERA_POS =====
// mode(Byte) + out_x(Word) + out_z(Word) + out_y(Word) = 8 bytes
class OpAF : public OpCode {
public:
	uint8_t code() const override { return 0xAF; }
	const char* name() const override { return "GET_CAMERA_POS"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"mode", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
// ===== 0xB0 PRINT_EVENT_MESSAGE_EXT =====
// mode(Byte) + speaker(Dword) + listener(Dword) + msg_off(Word) = 12 bytes
class OpB0 : public OpCode {
public:
	uint8_t code() const override { return 0xB0; }
	const char* name() const override { return "PRINT_EVENT_MSG_EXT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 12; }
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"speaker", ArgType::Dword}, {"listener", ArgType::Dword}, {"msg_off", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"mode", std::to_string(ru8(d,o+1)), ru8(d,o+1)},
				{"speaker", resolve_speaker(ctx, ru32(d,o+2)), ru32(d,o+2)},
				{"listener", resolve_speaker(ctx, ru32(d,o+6)), ru32(d,o+6)},
				{"msg_off", refstr(ctx, ru16(d,o+10)), ru16(d,o+10)}};
	}
	void extract(std::vector<DialogueLine>& out, const OpCodeContext& ctx,
		std::span<const uint8_t> d, size_t o) const override
	{
		if (ru8(d, o+1) != 0) return;
		uint16_t raw = ru16(d, o+10);
		uint32_t msg;
		if (TryResolveMessage(raw, ctx, msg))
			out.push_back({resolve_speaker(ctx, ru32(d, o+2)), resolve_string(ctx, msg), msg, msg});
	}
};
// ===== 0xB1 GET_APP_FLAG =====
// mode(Byte) + out(Word) = 4 bytes
class OpB1 : public OpCode {
public:
	uint8_t code() const override { return 0xB1; }
	const char* name() const override { return "GET_APP_FLAG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"out", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"mode", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"out", std::to_string(ru16(d, o+2)), ru16(d, o+2)}};
	}
};
// ===== 0xB2 DELIVERY_BOX =====
// Variable length: 2 (sub=1) or 4 (sub=0 with wait Word)
class OpB2 : public OpCode {
public:
	uint8_t code() const override { return 0xB2; }
	const char* name() const override { return "DELIVERY_BOX"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o+1);
		switch (sub) {
		case 0x00: return 4;
		case 0x01: return 2;
		default:   return 2;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
// ===== 0xB3 RANKINGS =====
// Variable length: 2, 4, 14, or 18 bytes depending on sub-case
class OpB3 : public OpCode {
public:
	uint8_t code() const override { return 0xB3; }
	const char* name() const override { return "RANKINGS"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o+1);
		switch (sub) {
		case 0x02: case 0x08: return 2;
		case 0x00: case 0x03: case 0x04: case 0x06: case 0x07: case 0x09: return 4;
		case 0x01: return 14;
		case 0x05: return 18;
		default:   return 2;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class OpB4 : public OpCode {
public:
	uint8_t code() const override { return 0xB4; } const char* name() const override { return "UI_STRING"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		switch (ru8(d, o+1)) {
		case 0x00: case 0x13: return 20;
		case 0x01: case 0x02: case 0x04: case 0x0F: case 0x10: case 0x11: case 0x12: return 6;
		case 0x05: case 0x06: return 3;
		case 0x07: case 0x09: case 0x0A: case 0x0C: return 4;
		case 0x14: return 12;
		default:  return 2;
		}
	}
	std::vector<ArgDef> args() const override { return {{"sub", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d,o+1)), ru8(d,o+1)}};
	}
};
class OpB5 : public OpCode { public:
	uint8_t code() const override { return 0xB5; } const char* name() const override { return "SET_ENTITY_NAME"; } size_t length(std::span<const uint8_t>, size_t) const override { return 4; }
	std::vector<ArgDef> args() const override { return {{"mode", ArgType::Byte}, {"work_ofs", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"mode", std::to_string(ru8(d,o+1)), ru8(d,o+1)}, {"work_ofs", refstr(ctx, ru16(d,o+2)), ru16(d,o+2)}};
	}
};
class OpB6 : public OpCode {
public:
	uint8_t code() const override { return 0xB6; } const char* name() const override { return "ENTITY_APPEAR"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		switch (ru8(d, o+1)) {
		case 0x0B: return 20;
		case 0x0D: return 14;
		case 0x0E: return 16;
		case 0x14: case 0x15: return 6;
		case 0x10: case 0x12: case 0x13: return 2;
		default:  return 4;
		}
	}
	std::vector<ArgDef> args() const override { return {{"sub", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d,o+1)), ru8(d,o+1)}};
	}
};
class OpB7 : public OpCode {
public:
	uint8_t code() const override { return 0xB7; } const char* name() const override { return "ENTITY_DATA"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 8;
		switch (ru8(d, o+1)) {
		case 0: return 10;
		default: return 8;
		}
	}
	std::vector<ArgDef> args() const override { return {{"sub", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d,o+1)), ru8(d,o+1)}};
	}
};
class OpB8 : public OpCode {
public:
	uint8_t code() const override { return 0xB8; } const char* name() const override { return "MAP_MARKER_NAME"; } size_t length(std::span<const uint8_t>, size_t) const override { return 27; }
	std::vector<ArgDef> args() const override {
		return {{"val1", ArgType::Word}, {"val2", ArgType::Word}, {"val3", ArgType::Word}, {"val4", ArgType::Word}, {"val5", ArgType::Word}, {"name", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		char name[17]{}; memcpy(name, d.data()+o+11, 16); name[16]=0;
		return {{"val1", refstr(ctx, ru16(d,o+1)), ru16(d,o+1)},
				{"val2", refstr(ctx, ru16(d,o+3)), ru16(d,o+3)},
				{"val3", refstr(ctx, ru16(d,o+5)), ru16(d,o+5)},
				{"val4", refstr(ctx, ru16(d,o+7)), ru16(d,o+7)},
				{"val5", refstr(ctx, ru16(d,o+9)), ru16(d,o+9)},
				{"name", std::string(name), 0}};
	}
};
class OpB9 : public OpCode {
public:
	uint8_t code() const override { return 0xB9; } const char* name() const override { return "MAP_EDIT_MARKER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 8; }
	std::vector<ArgDef> args() const override {
		return {{"code", ArgType::Byte}, {"val1", ArgType::Word}, {"val2", ArgType::Word}, {"val3", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"code", std::to_string(ru8(d,o+1)), ru8(d,o+1)},
				{"val1", refstr(ctx, ru16(d,o+2)), ru16(d,o+2)},
				{"val2", refstr(ctx, ru16(d,o+4)), ru16(d,o+4)},
				{"val3", refstr(ctx, ru16(d,o+6)), ru16(d,o+6)}};
	}
};
class OpBA : public OpCode {
public:
	uint8_t code() const override { return 0xBA; } const char* name() const override { return "SET_ENTITY_POS"; } size_t length(std::span<const uint8_t>, size_t) const override { return 13; }
	std::vector<ArgDef> args() const override {
		return {{"entity", ArgType::Dword}, {"pos_x", ArgType::Word}, {"pos_z", ArgType::Word}, {"pos_y", ArgType::Word}, {"dir", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d,o+1)), ru32(d,o+1)},
				{"pos_x", refstr(ctx, ru16(d,o+5)), ru16(d,o+5)},
				{"pos_z", refstr(ctx, ru16(d,o+7)), ru16(d,o+7)},
				{"pos_y", refstr(ctx, ru16(d,o+9)), ru16(d,o+9)},
				{"dir", refstr(ctx, ru16(d,o+11)), ru16(d,o+11)}};
	}
};
class OpBB : public OpCode { public: uint8_t code() const override { return 0xBB; } const char* name() const override { return "LOAD_SCHED_ALT_B"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class OpBC : public OpCode { public: uint8_t code() const override { return 0xBC; } const char* name() const override { return "WAIT_SCHED_B"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpBD : public OpCode { public: uint8_t code() const override { return 0xBD; } const char* name() const override { return "END_SCHED_B"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpBE : public OpCode {
public:
	uint8_t code() const override { return 0xBE; } const char* name() const override { return "STORE_REQ_SERVER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"work_ofs", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"work_ofs", refstr(ctx, ru16(d,o+1)), ru16(d,o+1)}};
	}
};
class OpBF : public OpCode {
public:
	uint8_t code() const override { return 0xBF; } const char* name() const override { return "CHOCOBO_RACING"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o >= d.size()) return 8;
		switch (ru8(d, o+1)) {
		case 0x00: case 0x60: return 8;
		case 0x20: case 0x40: return 10;
		default:   return 8;
		}
	}
	std::vector<ArgDef> args() const override { return {{"sub", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d,o+1)), ru8(d,o+1)}};
	}
};
class OpC0 : public OpCode {
public:
	uint8_t code() const override { return 0xC0; }
	const char* name() const override { return "ADJ_RENDER_B12"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"index", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"index", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};
class OpC1 : public OpCode {
public:
	uint8_t code() const override { return 0xC1; }
	const char* name() const override { return "KILL_ACTION"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"entity_id", ArgType::Dword}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity_id", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)}};
	}
};
class OpC2 : public OpCode {
public:
	uint8_t code() const override { return 0xC2; }
	const char* name() const override { return "PARTY_STATE"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
		case 1: return 4;
		case 2: return 6;
		default: return 2;
		}
	}
	std::vector<ArgDef> args() const override { return {{"sub", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class OpC3 : public OpCode {
public:
	uint8_t code() const override { return 0xC3; }
	const char* name() const override { return "COPY_STR_TO_ARR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"index", ArgType::Word}, {"str_off", ArgType::Word}, {"val", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"index", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"str_off", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)},
				{"val", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};
class OpC4 : public OpCode {
public:
	uint8_t code() const override { return 0xC4; }
	const char* name() const override { return "HELPER_CALL_ALT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 12; }
	std::vector<ArgDef> args() const override { return {{"sub", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class OpC5 : public OpCode { public: uint8_t code() const override { return 0xC5; } const char* name() const override { return "LOAD_SCHED_A3"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class OpC6 : public OpCode { public: uint8_t code() const override { return 0xC6; } const char* name() const override { return "WAIT_SCHED_C6"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpC7 : public OpCode { public: uint8_t code() const override { return 0xC7; } const char* name() const override { return "END_SCHED_C7"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpC8 : public OpCode {
public:
	uint8_t code() const override { return 0xC8; }
	const char* name() const override { return "OPEN_MAP_WIN"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"val1", ArgType::Word}, {"val2", ArgType::Word}, {"val3", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"val1", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"val2", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)},
				{"val3", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};
class OpC9 : public OpCode { public: uint8_t code() const override { return 0xC9; } const char* name() const override { return "ENABLE_TIMER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class OpCA : public OpCode { public: uint8_t code() const override { return 0xCA; } const char* name() const override { return "DEPRECATED_CA"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class OpCB : public OpCode { public: uint8_t code() const override { return 0xCB; } const char* name() const override { return "DEPRECATED_CB"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class OpCC : public OpCode {
public:
	uint8_t code() const override { return 0xCC; }
	const char* name() const override { return "ITEM_INFO_WIN"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
		case 0x00: case 0x01: case 0x03: return 10;
		case 0x02: return 14;
		case 0x10: return 6;
		case 0x11: case 0x20: return 4;
		default:   return 2;
		}
	}
};
class OpCD : public OpCode { public: uint8_t code() const override { return 0xCD; } const char* name() const override { return "LOAD_SCHED_A4"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class OpCE : public OpCode { public: uint8_t code() const override { return 0xCE; } const char* name() const override { return "WAIT_SCHED_CE"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpCF : public OpCode { public: uint8_t code() const override { return 0xCF; } const char* name() const override { return "END_SCHED_CF"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpD0 : public OpCode { public: uint8_t code() const override { return 0xD0; } const char* name() const override { return "LOAD_SCHED_A5"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class OpD1 : public OpCode { public: uint8_t code() const override { return 0xD1; } const char* name() const override { return "WAIT_SCHED_D1"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpD2 : public OpCode { public: uint8_t code() const override { return 0xD2; } const char* name() const override { return "END_SCHED_D2"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpD3 : public OpCode { public: uint8_t code() const override { return 0xD3; } const char* name() const override { return "CLEAR_MOTION_Q"; } size_t length(std::span<const uint8_t>, size_t) const override { return 6; } };
class OpD4 : public OpCode {
public:
	uint8_t code() const override { return 0xD4; }
	const char* name() const override { return "MAP_QUERY_WIN"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
		case 0x00: return 8;  // opcode + sub + 0x24 helper (3 WORDS)
		case 0x01: return 8;  // opcode + sub + getworkofs*3
		case 0x02: return 8;  // opcode + sub + 0x24 helper (3 WORDS)
		case 0x03: return 6;  // opcode + sub + getworkofs*2
		case 0x04: return 12; // opcode + sub + getworkofs*5
		case 0x05: return 12; // opcode + sub + getworkofs*5
		default:   return 2;
		}
	}
};
class OpD5 : public OpCode { public: uint8_t code() const override { return 0xD5; } const char* name() const override { return "LOAD_SCHED_A8"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class OpD6 : public OpCode { public: uint8_t code() const override { return 0xD6; } const char* name() const override { return "WAIT_SCHED_D6"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpD7 : public OpCode { public: uint8_t code() const override { return 0xD7; } const char* name() const override { return "END_SCHED_D7"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class OpD8 : public OpCode {
public:
	uint8_t code() const override { return 0xD8; }
	const char* name() const override { return "SET_EVENT_DIR"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 6;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
		case 0x00: return 6;
		case 0x01: case 0x02: case 0x03: return 8;
		case 0x04: return 12;
		default:   return 6;
		}
	}
};
class OpD9 : public OpCode { public: uint8_t code() const override { return 0xD9; } const char* name() const override { return "SET_SOUND_LIMIT"; } size_t length(std::span<const uint8_t>, size_t) const override { return 2; } };

// ===== Registration (called from OpCodeRegistry constructor) =====
// ===== 0x61-0x6F misc =====
class Op61 : public OpCode { public: uint8_t code() const override { return 0x61; } const char* name() const override { return "ADJ_RENDER2"; } size_t length(std::span<const uint8_t>, size_t) const override { return 2; } };
class Op62 : public OpCode { public: uint8_t code() const override { return 0x62; } const char* name() const override { return "LOAD_EVENT_SCHEDULER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 17; } };
class Op63 : public OpCode { public: uint8_t code() const override { return 0x63; } const char* name() const override { return "PLAY_ANIM_WAIT"; } size_t length(std::span<const uint8_t>, size_t) const override { return 3; } };
class Op64 : public OpCode { public: uint8_t code() const override { return 0x64; } const char* name() const override { return "CALC_DIST"; } size_t length(std::span<const uint8_t>, size_t) const override { return 11; } };
class Op65 : public OpCode { public: uint8_t code() const override { return 0x65; } const char* name() const override { return "CALC_DIST_3D"; } size_t length(std::span<const uint8_t>, size_t) const override { return 11; } };
class Op66 : public OpCode { public: uint8_t code() const override { return 0x66; } const char* name() const override { return "LOAD_EXT_SCHED_MAIN"; } size_t length(std::span<const uint8_t>, size_t) const override { return 15; } };
class Op67 : public OpCode { public: uint8_t code() const override { return 0x67; } const char* name() const override { return "HIDE_HUD"; } size_t length(std::span<const uint8_t>, size_t) const override { return 5; } };
class Op68 : public OpCode { public: uint8_t code() const override { return 0x68; } const char* name() const override { return "SHOW_HUD"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op69 : public OpCode { public: uint8_t code() const override { return 0x69; } const char* name() const override { return "SET_SOUND_VOL"; } size_t length(std::span<const uint8_t>, size_t) const override { return 4; } };
class Op6A : public OpCode { public: uint8_t code() const override { return 0x6A; } const char* name() const override { return "CHG_SOUND_VOL"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; } };
class Op6B : public OpCode { public: uint8_t code() const override { return 0x6B; } const char* name() const override { return "ENTITY_IDLE"; } size_t length(std::span<const uint8_t>, size_t) const override { return 9; } };
// ===== 0x6C FADE_COLOR =====
// Fades entity color in/out. entity(DWORD), alpha(WORD work-ofs), time(WORD work-ofs)
class Op6C : public OpCode {
public:
	uint8_t code() const override { return 0x6C; }
	const char* name() const override { return "FADE_COLOR"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
	std::vector<ArgDef> args() const override {
		return {{"entity", ArgType::Dword}, {"alpha", ArgType::Word}, {"time", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"alpha", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)},
				{"time", refstr(ctx, ru16(d, o+7)), ru16(d, o+7)}};
	}
};

// ===== 0x6D DEPRECATED_6D =====
class Op6D : public OpCode { public: uint8_t code() const override { return 0x6D; } const char* name() const override { return "DEPRECATED_6D"; } size_t length(std::span<const uint8_t>, size_t) const override { return 7; } };

// ===== 0x6E PLAY_EMOTE =====
// Plays an emote animation on entity. entity(DWORD), emote(WORD work-ofs)
class Op6E : public OpCode {
public:
	uint8_t code() const override { return 0x6E; }
	const char* name() const override { return "PLAY_EMOTE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"entity", ArgType::Dword}, {"emote", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)},
				{"emote", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};

// ===== 0x6F WAIT_FRAME =====
// Yields until WaitTime elapses (stateful, no bytecode args)
class Op6F : public OpCode { public: uint8_t code() const override { return 0x6F; } const char* name() const override { return "WAIT_FRAME"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };

// ===== 0x70 WAIT_RENDER_FLAG =====
// Checks event entity render flag, yields if set; otherwise advances.
class Op70 : public OpCode { public:
	uint8_t code() const override { return 0x70; } const char* name() const override { return "WAIT_RENDER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ===== 0x71 HANDLE_STRING =====
// Variable-length: sub-byte at offset 1 determines total size
class Op71 : public OpCode {
public:
	uint8_t code() const override { return 0x71; }
	const char* name() const override { return "HANDLE_STRING"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		switch (ru8(d, o + 1)) {
		case 0x00: case 0x01: case 0x02: case 0x21: case 0x51: case 0x53: return 2;
		case 0x03: case 0x10: case 0x11: case 0x13: case 0x30: case 0x31: case 0x40: case 0x50: case 0x52: case 0x55: return 4;
		case 0x12: case 0x32: return 6;
		case 0x41: return 8;
		case 0x54: return 10;
		case 0x20: return 16;
		default:   return 2;
		}
	}
};

// ===== 0x72 LOAD_WEATHER =====
// Variable-length: sub-byte at offset 1 determines total size (4/6/10)
class Op72 : public OpCode {
public:
	uint8_t code() const override { return 0x72; }
	const char* name() const override { return "LOAD_WEATHER"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 4;
		switch (ru8(d, o + 1)) {
		case 0x00: return 10; // can be 4 or 10; return max
		case 0x01: return 6;
		default:   return 4;
		}
	}
};

// ===== 0x73 SCHED_MAGIC =====
// Schedules magic tasks on two entities. work(WORD), entity1(DWORD), entity2(DWORD)
class Op73 : public OpCode {
public:
	uint8_t code() const override { return 0x73; }
	const char* name() const override { return "SCHED_MAGIC"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 11; }
	std::vector<ArgDef> args() const override {
		return {{"param", ArgType::Word}, {"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"param", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"entity1", resolve_speaker(ctx, ru32(d, o+3)), ru32(d, o+3)},
				{"entity2", resolve_speaker(ctx, ru32(d, o+7)), ru32(d, o+7)}};
	}
};

// ===== 0x74 ADJ_RENDER1 =====
// Adjusts event entity Render.Flags1. flag(BYTE)
class Op74 : public OpCode {
public:
	uint8_t code() const override { return 0x74; }
	const char* name() const override { return "ADJ_RENDER1"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override { return {{"flag", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};

// ===== 0x75 LOAD_ROOM =====
// Variable-length: sub-byte at offset 1 determines total size (2 or 4)
class Op75 : public OpCode {
public:
	uint8_t code() const override { return 0x75; }
	const char* name() const override { return "LOAD_ROOM"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		switch (ru8(d, o + 1)) {
		case 0x00: return 4;
		case 0x01: return 2;
		case 0x02: return 4; // sub=2 advances by 2 net (from rewind), but total data is 4 bytes
		default:   return 2;
		}
	}
};

// ===== 0x76 CHECK_RENDER =====
// Checks entity render flags, yields if render flag3 is set. entity(DWORD)
class Op76 : public OpCode {
public:
	uint8_t code() const override { return 0x76; }
	const char* name() const override { return "CHECK_RENDER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"entity", ArgType::Dword}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)}};
	}
};

// ===== 0x77 SET_TIME_WEATHER =====
// Sets event time and weather. time(WORD work-ofs), weather(WORD work-ofs)
class Op77 : public OpCode {
public:
	uint8_t code() const override { return 0x77; }
	const char* name() const override { return "SET_TIME_WEATHER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"time", ArgType::Word}, {"weather", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"time", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"weather", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}};
	}
};
class Op78 : public OpCode { public: uint8_t code() const override { return 0x78; } const char* name() const override { return "RESET_WEATHER"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op79 : public OpCode {
public:
	uint8_t code() const override { return 0x79; } const char* name() const override { return "LOOK_AT"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 10;
		switch (ru8(d, o + 1)) {
		case 0: case 2: return 10;
		case 1: return 12;
		default: return 10;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"mode", ArgType::Byte}, {"entity1", ArgType::Dword}, {"entity2", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"mode", std::to_string(ru8(d,o+1)), ru8(d,o+1)},
				{"entity1", resolve_speaker(ctx, ru32(d,o+2)), ru32(d,o+2)},
				{"entity2", resolve_speaker(ctx, ru32(d,o+6)), ru32(d,o+6)}};
	}
};
class Op7A : public OpCode {
public:
	uint8_t code() const override { return 0x7A; }
	const char* name() const override { return "VM_CONTROL"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		switch (ru8(d, o + 1)) {
		case 0: case 2: case 5: return 6;
		case 1: return 7;
		case 3: return 2;
		case 4: return 8;
		default: return 2;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"case", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"case", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op7B : public OpCode {
public:
	uint8_t code() const override { return 0x7B; } const char* name() const override { return "UNSET_TALKING"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"entity_id", ArgType::Dword}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity_id", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)}};
	}
};
class Op7C : public OpCode {
public:
	uint8_t code() const override { return 0x7C; } const char* name() const override { return "ADJ_RENDER2_B"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"flag", ArgType::Byte}, {"entity", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)}};
	}
};
class Op7D : public OpCode {
public:
	uint8_t code() const override { return 0x7D; } const char* name() const override { return "LOAD_START_SCHED"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"work_idx", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"work_idx", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op7E : public OpCode {
public:
	uint8_t code() const override { return 0x7E; } const char* name() const override { return "CHOCOBO"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 6;
		switch (ru8(d, o + 1)) {
		case 0: case 1: case 2: case 4: case 5: case 8: return 6;
		case 3: return 16;
		case 6: return 18;
		case 7: return 8;
		default: return 6;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"case", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"case", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op7F : public OpCode { public: uint8_t code() const override { return 0x7F; } const char* name() const override { return "WAIT_SELECT_ALT"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };

// ===== 0x80-0x95 =====
class Op80 : public OpCode {
public:
	uint8_t code() const override { return 0x80; } const char* name() const override { return "LOAD_WAIT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override { return {{"entity", ArgType::Dword}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"entity", resolve_speaker(ctx, ru32(d, o+1)), ru32(d, o+1)}};
	}
};
class Op81 : public OpCode {
public:
	uint8_t code() const override { return 0x81; } const char* name() const override { return "SET_BLINK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"flag", ArgType::Byte}, {"entity", ArgType::Dword}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity", resolve_speaker(ctx, ru32(d, o+2)), ru32(d, o+2)}};
	}
};
class Op82 : public OpCode {
public:
	uint8_t code() const override { return 0x82; } const char* name() const override { return "RECT_HIT_TEST"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }
	std::vector<ArgDef> args() const override {
		return {{"value", ArgType::Dword}, {"target", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"value", refstr(ctx, ru16(d, o+1)), ru32(d, o+1)},
				{"target", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)}};
	}
};
class Op83 : public OpCode {
public:
	uint8_t code() const override { return 0x83; } const char* name() const override { return "GET_GAME_TIME"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"work_idx", ArgType::Byte}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"work_idx", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op84 : public OpCode { public: uint8_t code() const override { return 0x84; } const char* name() const override { return "ADJ_RENDER3_B0"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op85 : public OpCode { public: uint8_t code() const override { return 0x85; } const char* name() const override { return "OPEN_MOG_MENU"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op86 : public OpCode {
public:
	uint8_t code() const override { return 0x86; }
	const char* name() const override { return "ADJ_RENDER3"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"flags3", ArgType::Byte}, {"entity_ref", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"flags3", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity_ref", refstr(ctx, ru16(d, o+2)), ru16(d, o+2)}};
	}
};
class Op87 : public OpCode {
public:
	uint8_t code() const override { return 0x87; }
	const char* name() const override { return "WORLD_PASS_A"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op88 : public OpCode {
public:
	uint8_t code() const override { return 0x88; }
	const char* name() const override { return "WORLD_PASS_B"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 2; }
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op89 : public OpCode {
public:
	uint8_t code() const override { return 0x89; }
	const char* name() const override { return "OPEN_MAP"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override {
		return {{"map_id", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"map_id", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};
class Op8A : public OpCode { public: uint8_t code() const override { return 0x8A; } const char* name() const override { return "CLOSE_MAP"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op8B : public OpCode {
public:
	uint8_t code() const override { return 0x8B; }
	const char* name() const override { return "SET_MARK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 25; }
	std::vector<ArgDef> args() const override {
		return {{"map_id", ArgType::Word}, {"point_idx", ArgType::Word},
				{"pos_x", ArgType::Word}, {"pos_y", ArgType::Word},
				{"name", ArgType::ShortStr}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		std::string name(reinterpret_cast<const char*>(d.data() + o + 9), 16);
		auto nullpos = name.find('\0');
		if (nullpos != std::string::npos) name.resize(nullpos);
		return {{"map_id", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"point_idx", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)},
				{"pos_x", refstr(ctx, ru16(d, o+5)), ru16(d, o+5)},
				{"pos_y", refstr(ctx, ru16(d, o+7)), ru16(d, o+7)},
				{"name", name, 0}};
	}
};
class Op8C : public OpCode {
public:
	uint8_t code() const override { return 0x8C; }
	const char* name() const override { return "CRAFTING"; }
	size_t length(std::span<const uint8_t> d, size_t o) const override {
		if (o + 1 >= d.size()) return 2;
		uint8_t sub = ru8(d, o + 1);
		switch (sub) {
		case 0:  return 8;
		case 1:  return 2;
		case 2:  return 12;
		case 3:  return 10;
		case 4:  return 10;
		case 5:  return 14;
		default: return 2;
		}
	}
	std::vector<ArgDef> args() const override {
		return {{"sub", ArgType::Byte}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"sub", std::to_string(ru8(d, o+1)), ru8(d, o+1)}};
	}
};
class Op8D : public OpCode {
public:
	uint8_t code() const override { return 0x8D; }
	const char* name() const override { return "OPEN_MAP_PROPS"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
	std::vector<ArgDef> args() const override {
		return {{"map_id", ArgType::Word}, {"props", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"map_id", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)},
				{"props", refstr(ctx, ru16(d, o+3)), ru16(d, o+3)}};
	}
};
class Op8E : public OpCode { public: uint8_t code() const override { return 0x8E; } const char* name() const override { return "STATUS_45"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op8F : public OpCode { public: uint8_t code() const override { return 0x8F; } const char* name() const override { return "STATUS_46"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op90 : public OpCode { public: uint8_t code() const override { return 0x90; } const char* name() const override { return "ADJ_RENDER01"; } size_t length(std::span<const uint8_t>, size_t) const override { return 1; } };
class Op91 : public OpCode {
public:
	uint8_t code() const override { return 0x91; }
	const char* name() const override { return "SET_SPEED_BASE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"speed", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"speed", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};
class Op92 : public OpCode {
public:
	uint8_t code() const override { return 0x92; }
	const char* name() const override { return "ADJ_RENDER3_ALT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"flag", ArgType::Byte}, {"entity", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity", hex(ru16(d, o+2)), ru16(d, o+2)}};
	}
};
class Op93 : public OpCode {
public:
	uint8_t code() const override { return 0x93; }
	const char* name() const override { return "DISPLAY_ITEM"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"item_id", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"item_id", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
};
class Op94 : public OpCode {
public:
	uint8_t code() const override { return 0x94; }
	const char* name() const override { return "ADJ_RENDER3_B"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }
	std::vector<ArgDef> args() const override {
		return {{"flag", ArgType::Byte}, {"entity", ArgType::Word}};
	}
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext&) const override {
		return {{"flag", std::to_string(ru8(d, o+1)), ru8(d, o+1)},
				{"entity", hex(ru16(d, o+2)), ru16(d, o+2)}};
	}
};
class Op95 : public OpCode {
public:
	uint8_t code() const override { return 0x95; } const char* name() const override { return "SETUP_EVENT_NPC"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }
	std::vector<ArgDef> args() const override { return {{"npc_param", ArgType::Word}}; }
	std::vector<ArgValue> parse(std::span<const uint8_t> d, size_t o, const OpCodeContext& ctx) const override {
		return {{"npc_param", refstr(ctx, ru16(d, o+1)), ru16(d, o+1)}};
	}
	void updateCtx(OpCodeContext& ctx, std::span<const uint8_t> d, size_t o) const override {
		(void)d; (void)o; (void)ctx;
		// In full implementation: sets EventEntity to the configured NPC
	}
};

void RegisterOpcodes00_20(OpCodeRegistry& r)
{
	r.reg(std::make_unique<Op00>());
	r.reg(std::make_unique<Op01>());
	r.reg(std::make_unique<Op02>());
	r.reg(std::make_unique<Op03>());
	r.reg(std::make_unique<Op04>());
	r.reg(std::make_unique<Op05>());
	r.reg(std::make_unique<Op06>());
	r.reg(std::make_unique<Op07>());
	r.reg(std::make_unique<Op08>());
	r.reg(std::make_unique<Op09>());
	r.reg(std::make_unique<Op0A>());
	r.reg(std::make_unique<Op0B>());
	r.reg(std::make_unique<Op0C>());
	r.reg(std::make_unique<Op0D>());
	r.reg(std::make_unique<Op0E>());
	r.reg(std::make_unique<Op0F>());
	r.reg(std::make_unique<Op10>());
	r.reg(std::make_unique<Op11>());
	r.reg(std::make_unique<Op12>());
	r.reg(std::make_unique<Op13>());
	r.reg(std::make_unique<Op14>());
	r.reg(std::make_unique<Op15>());
	r.reg(std::make_unique<Op16>());
	r.reg(std::make_unique<Op17>());
	r.reg(std::make_unique<Op18>());
	r.reg(std::make_unique<Op19>());
	r.reg(std::make_unique<Op1A>());
	r.reg(std::make_unique<Op1B>());
	r.reg(std::make_unique<Op1C>());
	r.reg(std::make_unique<Op1D>());
	r.reg(std::make_unique<Op1E>());
	r.reg(std::make_unique<Op1F>());
	r.reg(std::make_unique<Op20>());
	r.reg(std::make_unique<Op21>());
	r.reg(std::make_unique<Op22>());
	r.reg(std::make_unique<Op23>());
	r.reg(std::make_unique<Op24>());
	r.reg(std::make_unique<Op25>());
	r.reg(std::make_unique<Op26>());
	r.reg(std::make_unique<Op27>());
	r.reg(std::make_unique<Op28>());
	r.reg(std::make_unique<Op29>());
	r.reg(std::make_unique<Op2A>());
	r.reg(std::make_unique<Op2B>());
	r.reg(std::make_unique<Op2C>());
	r.reg(std::make_unique<Op2D>());
	r.reg(std::make_unique<Op2E>());
	r.reg(std::make_unique<Op2F>());
	r.reg(std::make_unique<Op30>());
	r.reg(std::make_unique<Op31>());
	r.reg(std::make_unique<Op32>());
	r.reg(std::make_unique<Op33>());
	r.reg(std::make_unique<Op34>());
	r.reg(std::make_unique<Op35>());
	r.reg(std::make_unique<Op36>());
	r.reg(std::make_unique<Op37>());
	r.reg(std::make_unique<Op38>());
	r.reg(std::make_unique<Op39>());
	r.reg(std::make_unique<Op3A>());
	r.reg(std::make_unique<Op3B>());
	r.reg(std::make_unique<Op3C>());
	r.reg(std::make_unique<Op3D>());
	r.reg(std::make_unique<Op3E>());
	r.reg(std::make_unique<Op3F>());
	r.reg(std::make_unique<Op40>());
	r.reg(std::make_unique<Op41>());
	r.reg(std::make_unique<Op42>());
	r.reg(std::make_unique<Op43>());
	r.reg(std::make_unique<Op44>());
	r.reg(std::make_unique<Op45>());
	r.reg(std::make_unique<Op46>());
	r.reg(std::make_unique<Op47>());
	r.reg(std::make_unique<Op48>());
	r.reg(std::make_unique<Op49>());
	r.reg(std::make_unique<Op4A>());
	r.reg(std::make_unique<Op4B>());
	r.reg(std::make_unique<Op4C>());
	r.reg(std::make_unique<Op4D>());
	r.reg(std::make_unique<Op4E>());
	r.reg(std::make_unique<Op4F>());
	r.reg(std::make_unique<Op50>());
	r.reg(std::make_unique<Op51>());
	r.reg(std::make_unique<Op52>());
	r.reg(std::make_unique<Op53>());
	r.reg(std::make_unique<Op54>());
	r.reg(std::make_unique<Op55>());
	r.reg(std::make_unique<Op56>());
	r.reg(std::make_unique<Op57>());
	r.reg(std::make_unique<Op58>());
	r.reg(std::make_unique<Op59>());
	r.reg(std::make_unique<Op5A>());
	r.reg(std::make_unique<Op5B>());
	r.reg(std::make_unique<Op5C>());
	r.reg(std::make_unique<Op5D>());
	r.reg(std::make_unique<Op5E>());
	r.reg(std::make_unique<Op5F>());
	r.reg(std::make_unique<Op60>());
	r.reg(std::make_unique<Op61>());
	r.reg(std::make_unique<Op62>());
	r.reg(std::make_unique<Op63>());
	r.reg(std::make_unique<Op64>());
	r.reg(std::make_unique<Op65>());
	r.reg(std::make_unique<Op66>());
	r.reg(std::make_unique<Op67>());
	r.reg(std::make_unique<Op68>());
	r.reg(std::make_unique<Op69>());
	r.reg(std::make_unique<Op6A>());
	r.reg(std::make_unique<Op6B>());
	r.reg(std::make_unique<Op6C>());
	r.reg(std::make_unique<Op6D>());
	r.reg(std::make_unique<Op6E>());
	r.reg(std::make_unique<Op6F>());
	r.reg(std::make_unique<Op70>());
	r.reg(std::make_unique<Op71>());
	r.reg(std::make_unique<Op72>());
	r.reg(std::make_unique<Op73>());
	r.reg(std::make_unique<Op74>());
	r.reg(std::make_unique<Op75>());
	r.reg(std::make_unique<Op76>());
	r.reg(std::make_unique<Op77>());
	r.reg(std::make_unique<Op78>());
	r.reg(std::make_unique<Op79>());
	r.reg(std::make_unique<Op7A>());
	r.reg(std::make_unique<Op7B>());
	r.reg(std::make_unique<Op7C>());
	r.reg(std::make_unique<Op7D>());
	r.reg(std::make_unique<Op7E>());
	r.reg(std::make_unique<Op7F>());
	r.reg(std::make_unique<Op80>());
	r.reg(std::make_unique<Op81>());
	r.reg(std::make_unique<Op82>());
	r.reg(std::make_unique<Op83>());
	r.reg(std::make_unique<Op84>());
	r.reg(std::make_unique<Op85>());
	r.reg(std::make_unique<Op86>());
	r.reg(std::make_unique<Op87>());
	r.reg(std::make_unique<Op88>());
	r.reg(std::make_unique<Op89>());
	r.reg(std::make_unique<Op8A>());
	r.reg(std::make_unique<Op8B>());
	r.reg(std::make_unique<Op8C>());
	r.reg(std::make_unique<Op8D>());
	r.reg(std::make_unique<Op8E>());
	r.reg(std::make_unique<Op8F>());
	r.reg(std::make_unique<Op90>());
	r.reg(std::make_unique<Op91>());
	r.reg(std::make_unique<Op92>());
	r.reg(std::make_unique<Op93>());
	r.reg(std::make_unique<Op94>());
	r.reg(std::make_unique<Op95>());
	r.reg(std::make_unique<Op96>());
	r.reg(std::make_unique<Op97>());
	r.reg(std::make_unique<Op98>());
	r.reg(std::make_unique<Op99>());
	r.reg(std::make_unique<Op9A>());
	r.reg(std::make_unique<Op9B>());
	r.reg(std::make_unique<Op9C>());
	r.reg(std::make_unique<Op9D>());
	r.reg(std::make_unique<Op9E>());
	r.reg(std::make_unique<Op9F>());
	r.reg(std::make_unique<OpA0>());
	r.reg(std::make_unique<OpA1>());
	r.reg(std::make_unique<OpA2>());
	r.reg(std::make_unique<OpA3>());
	r.reg(std::make_unique<OpA4>());
	r.reg(std::make_unique<OpA5>());
	r.reg(std::make_unique<OpA6>());
	r.reg(std::make_unique<OpA7>());
	r.reg(std::make_unique<OpA8>());
	r.reg(std::make_unique<OpA9>());
	r.reg(std::make_unique<OpAA>());
	r.reg(std::make_unique<OpAB>());
	r.reg(std::make_unique<OpAC>());
	r.reg(std::make_unique<OpAD>());
	r.reg(std::make_unique<OpAE>());
	r.reg(std::make_unique<OpAF>());
	r.reg(std::make_unique<OpB0>());
	r.reg(std::make_unique<OpB1>());
	r.reg(std::make_unique<OpB2>());
	r.reg(std::make_unique<OpB3>());
	r.reg(std::make_unique<OpB4>());
	r.reg(std::make_unique<OpB5>());
	r.reg(std::make_unique<OpB6>());
	r.reg(std::make_unique<OpB7>());
	r.reg(std::make_unique<OpB8>());
	r.reg(std::make_unique<OpB9>());
	r.reg(std::make_unique<OpBA>());
	r.reg(std::make_unique<OpBB>());
	r.reg(std::make_unique<OpBC>());
	r.reg(std::make_unique<OpBD>());
	r.reg(std::make_unique<OpBE>());
	r.reg(std::make_unique<OpBF>());
	r.reg(std::make_unique<OpC0>());
	r.reg(std::make_unique<OpC1>());
	r.reg(std::make_unique<OpC2>());
	r.reg(std::make_unique<OpC3>());
	r.reg(std::make_unique<OpC4>());
	r.reg(std::make_unique<OpC5>());
	r.reg(std::make_unique<OpC6>());
	r.reg(std::make_unique<OpC7>());
	r.reg(std::make_unique<OpC8>());
	r.reg(std::make_unique<OpC9>());
	r.reg(std::make_unique<OpCA>());
	r.reg(std::make_unique<OpCB>());
	r.reg(std::make_unique<OpCC>());
	r.reg(std::make_unique<OpCD>());
	r.reg(std::make_unique<OpCE>());
	r.reg(std::make_unique<OpCF>());
	r.reg(std::make_unique<OpD0>());
	r.reg(std::make_unique<OpD1>());
	r.reg(std::make_unique<OpD2>());
	r.reg(std::make_unique<OpD3>());
	r.reg(std::make_unique<OpD4>());
	r.reg(std::make_unique<OpD5>());
	r.reg(std::make_unique<OpD6>());
	r.reg(std::make_unique<OpD7>());
	r.reg(std::make_unique<OpD8>());
	r.reg(std::make_unique<OpD9>());
}
