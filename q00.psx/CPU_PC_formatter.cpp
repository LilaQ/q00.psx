#include "cpu.h"
#include <sstream>
#include <iostream>
#include "spdlog/pattern_formatter.h"
class CPU_PC_flag_formatter : public spdlog::custom_flag_formatter
{
public:
	void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override
	{
		std::stringstream ss;
		ss << std::hex << std::showbase << CPU::registers.pc;
		dest.append(ss.str());
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return spdlog::details::make_unique<CPU_PC_flag_formatter>();
	}
};