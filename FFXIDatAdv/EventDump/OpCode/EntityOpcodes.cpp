#include "OpCode.h"

// ─── 0x95: SETUP_EVENT_NPC ───
// Args: npc_param (WORD)
// Sets up the entity as the event NPC (speaker)
class Op95_SetupEventNpc : public OpCode
{
public:
	uint8_t code() const override { return 0x95; }
	const char* name() const override { return "SETUP_EVENT_NPC"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 3; }

	void UpdateContext(OpCodeContext& ctx, std::span<const uint8_t>, size_t) const override
	{
		(void)ctx;
		// In full implementation, this would configure which entity becomes the EventEntity
		// For Phase 1, the speaker is already the actor_number, so this is a no-op
	}
};

// ─── 0x1E: ENTITY_LOOK_AT_AND_TALK ───
// Args: target_entity (DWORD)
// EventEntity looks at target and starts talking
class Op1E_LookAtAndTalk : public OpCode
{
public:
	uint8_t code() const override { return 0x1E; }
	const char* name() const override { return "ENTITY_LOOK_AT_AND_TALK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }

	// No dialogue produced here; the subsequent PRINT_EVENT_MESSAGE does the talking
	// EventEntity remains the same (implicit speaker)
	void UpdateContext(OpCodeContext& ctx, std::span<const uint8_t>, size_t) const override
	{
		(void)ctx;
	}
};

// ─── 0x4A: ENTITY_LOOK_AT ───
// Args: entity1_id (DWORD), entity2_id (DWORD)
class Op4A_LookAt : public OpCode
{
public:
	uint8_t code() const override { return 0x4A; }
	const char* name() const override { return "ENTITY_LOOK_AT"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 9; }
};

// ─── 0x79: LOOK_AT_ENTITY ───
// Args: mode (BYTE), entity1_id (DWORD), entity2_id (DWORD), turn_param (WORD, conditional)
class Op79_LookAtEntity : public OpCode
{
public:
	uint8_t code() const override { return 0x79; }
	const char* name() const override { return "LOOK_AT_ENTITY"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable length, needs mode-byte parsing
};

// ─── 0x7B: UNSET_ENTITY_TALKING ───
// Args: entity_id (DWORD)
class Op7B_UnsetEntityTalking : public OpCode
{
public:
	uint8_t code() const override { return 0x7B; }
	const char* name() const override { return "UNSET_ENTITY_TALKING"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 5; }
};

// ─── Registration ───
void RegisterEntityOpcodes(OpCodeRegistry& reg)
{
	reg.Register(std::make_unique<Op95_SetupEventNpc>());
	reg.Register(std::make_unique<Op1E_LookAtAndTalk>());
	reg.Register(std::make_unique<Op4A_LookAt>());
	reg.Register(std::make_unique<Op79_LookAtEntity>());
	reg.Register(std::make_unique<Op7B_UnsetEntityTalking>());
}
