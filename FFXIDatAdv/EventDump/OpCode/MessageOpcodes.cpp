#include "OpCode.h"
#include <string>
#include <cstdio>

// ─── Forward declarations of helpers (defined at bottom) ───
static std::string ResolveEntityName(const OpCodeContext& ctx, uint32_t entity_id);
static std::string ResolveString(const OpCodeContext& ctx, uint32_t msg_id);

// ─── 0x1D: PRINT_EVENT_MESSAGE ───
class Op1D_PrintEventMessage : public OpCode
{
public:
	uint8_t code() const override { return 0x1D; }
	const char* name() const override { return "PRINT_EVENT_MESSAGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }

	void ExtractDialog(
		const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset,
		std::vector<DialogueLine>& out) const override
	{
		uint16_t raw_msg = ReadU16(data, offset + 1);
		uint32_t msg_id = ctx.ResolveReference(raw_msg);

		DialogueLine dl;
		dl.speaker = ResolveEntityName(ctx, ctx.current_entity_id);
		dl.text = ResolveString(ctx, msg_id);
		out.push_back(dl);
	}
};

// ─── 0x2B: PRINT_ENTITY_MESSAGE ───
class Op2B_PrintEntityMessage : public OpCode
{
public:
	uint8_t code() const override { return 0x2B; }
	const char* name() const override { return "PRINT_ENTITY_MESSAGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }

	void ExtractDialog(
		const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset,
		std::vector<DialogueLine>& out) const override
	{
		uint32_t entity_id = ReadU32(data, offset + 1);
		uint16_t raw_msg = ReadU16(data, offset + 5);
		uint32_t msg_id = ctx.ResolveReference(raw_msg);

		DialogueLine dl;
		dl.speaker = ResolveEntityName(ctx, entity_id);
		dl.text = ResolveString(ctx, msg_id);
		out.push_back(dl);
	}
};

// ─── 0xB0: PRINT_EVENT_MESSAGE (extended) ───
class OpB0_PrintEventMessageExt : public OpCode
{
public:
	uint8_t code() const override { return 0xB0; }
	const char* name() const override { return "PRINT_EVENT_MESSAGE_EXT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 11; }

	void ExtractDialog(
		const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset,
		std::vector<DialogueLine>& out) const override
	{
		uint32_t speaker_id = ReadU32(data, offset + 2);
		uint16_t raw_msg = ReadU16(data, offset + 9);
		uint32_t msg_id = ctx.ResolveReference(raw_msg);

		DialogueLine dl;
		dl.speaker = ResolveEntityName(ctx, speaker_id);
		dl.text = ResolveString(ctx, msg_id);
		out.push_back(dl);
	}
};

// ─── 0x48: PRINT_MESSAGE ───
class Op48_PrintMessage : public OpCode
{
public:
	uint8_t code() const override { return 0x48; }
	const char* name() const override { return "PRINT_MESSAGE"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }

	void ExtractDialog(
		const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset,
		std::vector<DialogueLine>& out) const override
	{
		uint16_t raw_msg = ReadU16(data, offset + 1);
		uint32_t msg_id = ctx.ResolveReference(raw_msg);

		DialogueLine dl;
		dl.speaker = "[System]";
		dl.text = ResolveString(ctx, msg_id);
		out.push_back(dl);
	}
};

// ─── 0x49: PRINT_EVENT_MESSAGE_NO_SPEAKER ───
class Op49_PrintEventMessageNoSpeaker : public OpCode
{
public:
	uint8_t code() const override { return 0x49; }
	const char* name() const override { return "PRINT_EVENT_MESSAGE_NO_SPEAKER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 6; }

	void ExtractDialog(
		const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset,
		std::vector<DialogueLine>& out) const override
	{
		uint16_t raw_msg = ReadU16(data, offset + 5);
		uint32_t msg_id = ctx.ResolveReference(raw_msg);

		DialogueLine dl;
		dl.speaker = "(none)";
		dl.text = ResolveString(ctx, msg_id);
		out.push_back(dl);
	}
};

// ─── 0x24: CREATE_DIALOG ───
class Op24_CreateDialog : public OpCode
{
public:
	uint8_t code() const override { return 0x24; }
	const char* name() const override { return "CREATE_DIALOG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 7; }

	void ExtractDialog(
		const OpCodeContext& ctx,
		std::span<const uint8_t> data, size_t offset,
		std::vector<DialogueLine>& out) const override
	{
		uint16_t raw_msg = ReadU16(data, offset + 1);
		uint32_t msg_id = ctx.ResolveReference(raw_msg);

		DialogueLine dl;
		dl.speaker = ResolveEntityName(ctx, ctx.current_entity_id);
		dl.text = ResolveString(ctx, msg_id) + " [dialog]";
		out.push_back(dl);
	}
};

// ─── Helper: entity name resolution ───
static std::string ResolveEntityName(const OpCodeContext& ctx, uint32_t entity_id)
{
	if (entity_id == 0x7FFFFFF8)
		entity_id = ctx.current_entity_id;

	if (ctx.entity_map)
	{
		auto it = ctx.entity_map->find(entity_id);
		if (it != ctx.entity_map->end() && !it->second.name.empty())
			return it->second.name;
	}

	switch (entity_id)
	{
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

	char buf[32];
	snprintf(buf, sizeof(buf), "0x%08X", entity_id);
	return buf;
}

// ─── Helper: string resolution ───
static std::string ResolveString(const OpCodeContext& ctx, uint32_t msg_id)
{
	if (!ctx.zone_strings)
		return std::to_string(msg_id) + "*";

	if (msg_id < ctx.zone_strings->size())
	{
		const auto& u8str = (*ctx.zone_strings)[msg_id];
		return std::string(u8str.begin(), u8str.end());
	}

	return std::to_string(msg_id) + "*";
}

// ─── Registration ───
void RegisterMessageOpcodes(OpCodeRegistry& reg)
{
	reg.Register(std::make_unique<Op1D_PrintEventMessage>());
	reg.Register(std::make_unique<Op2B_PrintEntityMessage>());
	reg.Register(std::make_unique<OpB0_PrintEventMessageExt>());
	reg.Register(std::make_unique<Op48_PrintMessage>());
	reg.Register(std::make_unique<Op49_PrintEventMessageNoSpeaker>());
	reg.Register(std::make_unique<Op24_CreateDialog>());
}
