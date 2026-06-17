#include "OpCode.h"

// Forward: registration helpers from OpcodesImpl.cpp
void RegisterOpcodes00_20(OpCodeRegistry& r);

OpCodeRegistry::OpCodeRegistry() : unknown_(0xFF)
{
	RegisterOpcodes00_20(*this);
}

void OpCodeRegistry::reg(std::unique_ptr<OpCode> op)
{
	table_[op->code()] = std::move(op);
}
