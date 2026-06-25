#include "BytecodeAnalyzer.h"
#include <format>
#include <set>
#include <stack>
#include <iostream>

BytecodeAnalyzer::BytecodeAnalyzer()
{
}

static bool IsWithinBounds(std::span<const uint8_t> data, size_t offset, size_t needed, uint32_t actor_number = 0)
{
	if (offset + needed > data.size())
	{
		uint8_t opbyte = (offset < data.size()) ? data[offset] : 0xFF;
		std::ostringstream msg;
		msg << "[OOB] Actor 0x" << std::hex << actor_number << std::dec
			<< " Instruction at 0x" << std::hex << offset << std::dec
			<< " (opcode 0x" << std::hex << (int)opbyte << std::dec << ")"
			<< " needs " << needed << " bytes, but buffer has " << data.size();
		std::cerr << msg.str() << std::endl;
		return false;
	}
	return true;
}

void BytecodeAnalyzer::ExtractDialogues(
	std::span<const uint8_t> bytecode,
	uint32_t actor_number,
	const std::vector<uint32_t>& imed_data,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map,
	const std::vector<std::u8string>& zone_strings,
	std::vector<DialogueLine>& out_dialogues,
	size_t start_offset)
{
	if (bytecode.empty() || start_offset >= bytecode.size())
		return;

	OpCodeContext ctx;
	ctx.current_entity_id = actor_number;
	ctx.imed_data = &imed_data;
	ctx.entity_map = &entity_map;
	ctx.zone_strings = &zone_strings;

	std::set<size_t> visited;
	std::stack<size_t> worklist;
	std::stack<size_t> call_stack;

	worklist.push(start_offset);

	while (!worklist.empty())
	{
		size_t offset = worklist.top();
		worklist.pop();

		while (offset < bytecode.size())
		{
			if (!visited.insert(offset).second)
			{
				// Reached already-visited code: if in a CALL, pop return address
				if (!call_stack.empty())
				{
					offset = call_stack.top();
					call_stack.pop();
					continue;
				}
				break;
			}

			uint8_t code_byte = bytecode[offset];
			const OpCode& op = registry_.resolve(code_byte);

			size_t len = op.length(bytecode, offset);
			if (len == 0) len = 1;

			// Bounds check: ensure we can read the full instruction
			if (!IsWithinBounds(bytecode, offset, len, actor_number))
			{
				if (!call_stack.empty())
				{
					offset = call_stack.top();
					call_stack.pop();
					continue;
				}
				break;
			}

			op.updateCtx(ctx, bytecode, offset);

			// Only extract dialogues on reachable instructions
			op.extract(out_dialogues, ctx, bytecode, offset);

			// Handle control flow
			if (op.isTerminal())
			{
				// END/terminal opcode: if in a CALL, return to caller
				if (!call_stack.empty())
				{
					offset = call_stack.top();
					call_stack.pop();
					continue;
				}
				break;
			}

			auto targets = op.jumpTargets(bytecode, offset);

			if (code_byte == 0x01) // GOTO unconditional
			{
				if (!targets.empty())
				{
					offset = targets[0];
					continue;
				}
				break;
			}
			else if (code_byte == 0x1A) // CALL
			{
				// Push return address, jump to subroutine
				call_stack.push(offset + len);
				if (!targets.empty())
				{
					offset = targets[0];
					continue;
				}
				break;
			}
			else if (code_byte == 0x1B) // RETURN
			{
				if (!call_stack.empty())
				{
					offset = call_stack.top();
					call_stack.pop();
					continue;
				}
				break;
			}
			else if (code_byte == 0x02) // IF (conditional)
			{
				// Follow fall-through (true branch) first
				// Schedule else branch for later
				if (targets.size() >= 2)
				{
					if (visited.find(targets[0]) == visited.end())
						worklist.push(targets[0]); // else branch
				}
				offset += len; // fall through = true branch
				continue;
			}
			else if (!targets.empty())
			{
				// Other branch instructions: add targets to worklist
				for (auto it = targets.rbegin(); it != targets.rend(); ++it)
				{
					if (visited.find(*it) == visited.end())
						worklist.push(*it);
				}
				break;
			}

			offset += len;
		}
	}
}

std::vector<std::string> BytecodeAnalyzer::Disassemble(
	std::span<const uint8_t> bytecode,
	uint32_t actor_number,
	const std::vector<uint32_t>& imed_data,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map,
	const std::vector<std::u8string>& zone_strings,
	size_t start_offset) const
{
	std::vector<std::string> lines;
	OpCodeContext ctx;
	ctx.current_entity_id = actor_number;
	ctx.imed_data = &imed_data;
	ctx.entity_map = &entity_map;
	ctx.zone_strings = &zone_strings;

	std::set<size_t> visited;
	std::stack<size_t> worklist;
	std::stack<size_t> call_stack;

	if (start_offset < bytecode.size())
		worklist.push(start_offset);

	while (!worklist.empty())
	{
		size_t offset = worklist.top();
		worklist.pop();

		while (offset < bytecode.size())
		{
			if (!visited.insert(offset).second)
			{
				if (!call_stack.empty())
				{
					offset = call_stack.top(); call_stack.pop();
					continue;
				}
				break;
			}

			uint8_t code_byte = bytecode[offset];
			const OpCode& op = registry_.resolve(code_byte);

			size_t len = op.length(bytecode, offset);
			if (len == 0) len = 1;

			if (!IsWithinBounds(bytecode, offset, len, actor_number))
			{
				lines.push_back(std::format("  {:04X}: <OUT OF BOUNDS>", offset));
				if (!call_stack.empty())
				{
					offset = call_stack.top(); call_stack.pop();
					continue;
				}
				break;
			}

			std::string line = std::format("  {:04X}: {:02X} ", offset, code_byte);
			line += op.disasm(bytecode, offset, ctx);

			op.updateCtx(ctx, bytecode, offset);
			lines.push_back(line);

			if (op.isTerminal())
			{
				if (!call_stack.empty())
				{
					offset = call_stack.top(); call_stack.pop();
					continue;
				}
				break;
			}

			auto targets = op.jumpTargets(bytecode, offset);

			if (code_byte == 0x01)
			{
				if (!targets.empty()) { offset = targets[0]; continue; }
				break;
			}
			else if (code_byte == 0x1A)
			{
				call_stack.push(offset + len);
				if (!targets.empty()) { offset = targets[0]; continue; }
				break;
			}
			else if (code_byte == 0x1B)
			{
				if (!call_stack.empty())
				{
					offset = call_stack.top(); call_stack.pop();
					continue;
				}
				break;
			}
			else if (code_byte == 0x02)
			{
				if (targets.size() >= 2 && visited.find(targets[0]) == visited.end())
					worklist.push(targets[0]);
				offset += len;
				continue;
			}
			else if (!targets.empty())
			{
				for (auto it = targets.rbegin(); it != targets.rend(); ++it)
				{
					if (visited.find(*it) == visited.end())
						worklist.push(*it);
				}
				break;
			}

			offset += len;
		}
	}

	return lines;
}
