#pragma once

#include "Models.h"
#include "OpCode/OpCode.h"
#include <string>
#include <vector>
#include <span>
#include <unordered_map>

class BytecodeAnalyzer
{
public:
	BytecodeAnalyzer();

	void ExtractDialogues(
		std::span<const uint8_t> bytecode,
		uint32_t actor_number,
		const std::vector<uint32_t>& imed_data,
		const std::unordered_map<uint32_t, EntityEntry>& entity_map,
		const std::vector<std::u8string>& zone_strings,
		std::vector<DialogueLine>& out_dialogues,
		size_t start_offset = 0);

	std::vector<std::string> Disassemble(
		std::span<const uint8_t> bytecode,
		uint32_t actor_number,
		const std::vector<uint32_t>& imed_data,
		const std::unordered_map<uint32_t, EntityEntry>& entity_map,
		const std::vector<std::u8string>& zone_strings,
		size_t start_offset = 0) const;

private:
	OpCodeRegistry registry_;
};
