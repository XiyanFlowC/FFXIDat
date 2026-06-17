#include "OpCode.h"

// ─── 0x5B: LOAD_EXT_SCHEDULER ───
// Args: varies (scheduler name + entity list + work)
class Op5B_LoadExtScheduler : public OpCode
{
public:
	uint8_t code() const override { return 0x5B; }
	const char* name() const override { return "LOAD_EXT_SCHEDULER"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable, skip for Phase 1
};

// ─── 0x45: LOAD_SCHEDULED_TASK ───
// Args: scheduler name + entity pairs + work values
class Op45_LoadScheduledTask : public OpCode
{
public:
	uint8_t code() const override { return 0x45; }
	const char* name() const override { return "LOAD_SCHEDULED_TASK"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; } // variable
};

// ─── 0x6F: WAIT_FRAME_DELAY ───
class Op6F_WaitFrameDelay : public OpCode
{
public:
	uint8_t code() const override { return 0x6F; }
	const char* name() const override { return "WAIT_FRAME_DELAY"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── 0x70: WAIT_ENTITY_RENDER_FLAG ───
class Op70_WaitEntityRenderFlag : public OpCode
{
public:
	uint8_t code() const override { return 0x70; }
	const char* name() const override { return "WAIT_ENTITY_RENDER_FLAG"; }
	size_t length(std::span<const uint8_t>, size_t) const override { return 1; }
};

// ─── Registration ───
void RegisterSchedulerOpcodes(OpCodeRegistry& reg)
{
	reg.Register(std::make_unique<Op5B_LoadExtScheduler>());
	reg.Register(std::make_unique<Op45_LoadScheduledTask>());
	reg.Register(std::make_unique<Op6F_WaitFrameDelay>());
	reg.Register(std::make_unique<Op70_WaitEntityRenderFlag>());
}
