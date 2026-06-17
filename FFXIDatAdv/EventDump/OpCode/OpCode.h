#pragma once

#include "../Models.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <span>
#include <unordered_map>
#include <sstream>

// --- Argument type ---
enum class ArgType { None, Byte, Word, Dword, Entity, ShortStr, Offset };

struct ArgDef { const char* name; ArgType type; };
struct ArgValue { std::string name; std::string repr; uint32_t raw; };

// --- Work area address decoding ---
// Based on XiEvent::getworkofs. Used by SET_WORK/ADD/etc. opcodes to
// reference runtime state. Not used for message_id resolution.
struct WorkAddr
{
	static bool IsValid(uint16_t v)
	{
		if (v >= 0x8000 && v <= 0x8FFF) return true; // References
		if (v < 2048) return v < 80;                  // WorkLocal[0-79]
		if (v >= 4096 && v < 4352) return true;       // Work_Zone[0-95]
		if (v >= 4352 && v < 4608) return true;       // Work_Zone_Memorize[0-95]
		if (v >= 5888 && v < 6144) return true;       // Work_Zone_1700[0-95]
		return false;
	}

	static std::string Decode(uint16_t v)
	{
		if (v >= 0x8000 && v <= 0x8FFF)
			return "References[" + std::to_string(v & 0x7FFF) + "]";
		if (v < 2048) return "WorkLocal[" + std::to_string(v) + "]";
		if (v < 4096) { char b[32]; snprintf(b, sizeof(b), "Invalid[%u]", v); return b; }
		if (v < 4352) return "Work_Zone[" + std::to_string(v - 4096) + "]";
		if (v < 4608) return "Work_Zone_Memorize[" + std::to_string(v - 4352) + "]";
		if (v < 5888) { char b[32]; snprintf(b, sizeof(b), "Invalid[%u]", v); return b; }
		if (v < 6144) return "Work_Zone_1700[" + std::to_string(v - 5888) + "]";
		{ char b[32]; snprintf(b, sizeof(b), "0x%04X", v); return b; }
	}
};

// --- Context ---
struct OpCodeContext
{
	uint32_t current_entity_id;
	const std::vector<uint32_t>* imed_data = nullptr;
	const std::unordered_map<uint32_t, EntityEntry>* entity_map = nullptr;
	const std::vector<std::u8string>* zone_strings = nullptr;

	// Resolve a References entry: if bit 0x8000 is set, the low 15 bits
	// index into imed_data (the actor's initial work variables).
	uint32_t ResolveRef(uint16_t v) const
	{
		if (imed_data && (v & 0x8000))
		{
			uint32_t idx = v & 0x7FFF;
			if (idx < imed_data->size())
				return (*imed_data)[idx];
		}
		return v;
	}
};

// --- Base ---
class OpCode
{
public:
	virtual ~OpCode() = default;

	virtual uint8_t code() const = 0;
	virtual const char* name() const = 0;
	virtual size_t length(std::span<const uint8_t> data, size_t offset) const = 0;

	// Argument definitions
	virtual std::vector<ArgDef> args() const { return {}; }

	// Parse arguments from bytecode at given offset
	virtual std::vector<ArgValue> parse(std::span<const uint8_t> data, size_t offset, const OpCodeContext& ctx) const
	{
		(void)data; (void)offset; (void)ctx;
		return {};
	}

	// Format as disassembly line (without address prefix)
	virtual std::string disasm(std::span<const uint8_t> data, size_t offset, const OpCodeContext& ctx) const
	{
		auto a = args();
		auto v = parse(data, offset, ctx);
		if (v.empty()) return name();
		std::string r = name(); r += "(";
		for (size_t i = 0; i < v.size(); ++i) { if (i) r += ", "; r += v[i].repr; }
		r += ")";
		return r;
	}

	// Dialogue extraction (for message opcodes)
	virtual void extract(std::vector<DialogueLine>& out, const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset) const
	{ (void)out; (void)ctx; (void)data; (void)offset; }

	// Entity context update
	virtual void updateCtx(OpCodeContext& ctx, std::span<const uint8_t> data, size_t offset) const
	{ (void)ctx; (void)data; (void)offset; }

	// Jump targets for control flow (default: none = fall-through)
	virtual std::vector<uint32_t> jumpTargets(std::span<const uint8_t> data, size_t offset) const
	{ (void)data; (void)offset; return {}; }

	// Whether this instruction terminates linear execution (end/return)
	virtual bool isTerminal() const { return false; }
};

// --- Helpers (unchecked) ---
inline uint8_t ru8(std::span<const uint8_t> d, size_t o) { return d[o]; }
inline uint16_t ru16(std::span<const uint8_t> d, size_t o) { uint16_t v; memcpy(&v, d.data() + o, 2); return v; }
inline uint32_t ru32(std::span<const uint8_t> d, size_t o) { uint32_t v; memcpy(&v, d.data() + o, 4); return v; }

// --- Bounds-checked helpers (return false on OOB, log warning) ---
#include <iostream>
inline bool try_ru8(std::span<const uint8_t> d, size_t o, uint8_t& out) {
	if (o >= d.size()) { std::cerr << "[OOB] try_ru8 at 0x" << std::hex << o << std::dec << " (size=" << d.size() << ")\n"; return false; }
	out = d[o]; return true;
}
inline bool try_ru16(std::span<const uint8_t> d, size_t o, uint16_t& out) {
	if (o + 2 > d.size()) { std::cerr << "[OOB] try_ru16 at 0x" << std::hex << o << std::dec << " (size=" << d.size() << ")\n"; return false; }
	memcpy(&out, d.data() + o, 2); return true;
}
inline bool try_ru32(std::span<const uint8_t> d, size_t o, uint32_t& out) {
	if (o + 4 > d.size()) { std::cerr << "[OOB] try_ru32 at 0x" << std::hex << o << std::dec << " (size=" << d.size() << ")\n"; return false; }
	memcpy(&out, d.data() + o, 4); return true;
}

inline std::string hex(uint32_t v) {
	char b[32]; snprintf(b, sizeof(b), "0x%04X", v); return b;
}

inline std::string refstr(const OpCodeContext& ctx, uint16_t v) {
	if ((v & 0x8000) && ctx.imed_data)
	{
		uint32_t idx = v & 0x7FFF;
		if (idx < ctx.imed_data->size())
		{
			auto resolved = (*ctx.imed_data)[idx];
			return "References[" + std::to_string(idx) + "]=" + std::to_string(resolved) + "*";
		}
		return "References[" + std::to_string(idx) + "]=[OOB]";
	}
	if (WorkAddr::IsValid(v))
		return WorkAddr::Decode(v);
	return std::to_string(v);
}

inline std::string resolve_speaker(const OpCodeContext& ctx, uint32_t eid) {
	if (eid == 0x7FFFFFF8) eid = ctx.current_entity_id;
	if (ctx.entity_map) {
		auto it = ctx.entity_map->find(eid);
		if (it != ctx.entity_map->end() && !it->second.name.empty()) return it->second.name;
	}
	switch (eid) {
	case 0x7FFFFFF0: return "Zone Events";
	case 0x7FFFFFFF: return "Zone/Player Events";
	case 0x7FFFFFC0: return "LocalPlayer";
	case 0x7FFFFFC1: return "PartyMember 1";
	case 0x7FFFFFC2: return "PartyMember 2";
	case 0x7FFFFFC3: return "PartyMember 3";
	case 0x7FFFFFC4: return "PartyMember 4";
	case 0x7FFFFFC5: return "PartyMember 5";
	default: break;
	}
	char b[32]; snprintf(b, sizeof(b), "0x%08X", eid); return b;
}

struct StringResolveError : std::runtime_error {
	StringResolveError(uint32_t id, size_t size)
		: std::runtime_error("string resolve OOB: id=" + std::to_string(id) + " size=" + std::to_string(size)) {}
};

inline std::string resolve_string(const OpCodeContext& ctx, uint32_t id) {
	if (!ctx.zone_strings)
		throw StringResolveError(id, 0);
	if (id >= ctx.zone_strings->size())
		throw StringResolveError(id, ctx.zone_strings->size());
	const auto& u = (*ctx.zone_strings)[id];
	return std::string(u.begin(), u.end());
}

// --- Unknown ---
class UnknownOpCode : public OpCode
{
	uint8_t code_;
public:
	UnknownOpCode(uint8_t c = 0xFF) : code_(c) {}
	UnknownOpCode& operator=(uint8_t c) { code_ = c; return *this; }
	uint8_t code() const override { return code_; }
	const char* name() const override { return "UNKNOWN"; }
	size_t length(std::span<const uint8_t>, size_t) const override {
		return 1; // fallback for unknown opcodes
	}
};

// --- Registry ---
class OpCodeRegistry
{
	std::array<std::unique_ptr<OpCode>, 256> table_;
	mutable UnknownOpCode unknown_{0xFF};
public:
	OpCodeRegistry();
	void reg(std::unique_ptr<OpCode> op);
	const OpCode& resolve(uint8_t c) const {
		if (table_[c]) return *table_[c];
		unknown_ = UnknownOpCode(c);
		return unknown_;
	}
};


