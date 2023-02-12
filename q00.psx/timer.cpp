#include "timer.h"

static auto console = spdlog::stdout_color_mt("Timer");

namespace Timer {
	u32 current_counter[3];
	u32 counter_target[3];

	enum class SYNC_ENABLE : u32 { free_run = 0, use_sync_modes = 1 };
	enum class SYNC_MODE_0 : u32 { pause_counter_during_hblank = 0, reset_counter_to_0000_at_hblank = 1, reset_counter_to_0000_at_hblank_and_pause_outside_of_hblank = 2, pause_until_hblank_occurs_once_then_freerun = 3 };
	enum class SYNC_MODE_1 : u32 { pause_counter_during_vblank = 0, reset_counter_to_0000_at_vblank = 1, reset_counter_to_0000_at_vblank_and_pause_outside_of_vblank = 2, pause_until_vblank_occurs_once_then_freerun = 3 };
	enum class SYNC_MODE_2 : u32 { stop_counter_at_current_value_forever_0 = 0, free_run_1 = 1, stop_counter_at_current_value_forever_2 = 2, free_run_3 = 3 };
	enum class RESET_COUNTER : u32 { after_counter_equals_ffff = 0, after_counter_equals_target = 1 };
	enum class DISABLE_ENABLE : u32 { disable = 0, enable = 1 };
	enum class ONCE_REPEAT : u32 { once = 0, repeatedly = 1 };
	enum class PULSE_TOGGLE : u32 { pulse = 0, toggle = 1 };
	enum class CLOCK_SOURCE_0 : u32 { system_clock_0 = 0, dotclock_1 = 1, system_clock_2 = 2, dotclock_3 = 3 };
	enum class CLOCK_SOURCE_1 : u32 { system_clock_0 = 0, hblank_1 = 1, system_clock_2 = 2, hblank_3 = 3 };
	enum class CLOCK_SOURCE_2 : u32 { system_clock_0 = 0, system_clock_1 = 1, system_clock_div8_2 = 2, system_clock_div8_3 = 3 };
	enum class INTERRUPT_REQUEST : u32 { yes = 0, no = 1 };
	enum class REACHED_TARGET : u32 { no = 0, yes = 1 };

	union Counter_Mode {
		struct {
			SYNC_ENABLE sync_enable : 1;
			SYNC_MODE_0 sync_mode : 2;
			RESET_COUNTER reset_counter : 1;
			DISABLE_ENABLE irq_when_counter_reaches_target : 1;
			DISABLE_ENABLE irq_when_counter_reaches_ffff : 1;
			ONCE_REPEAT irq_once_or_repeat_mode : 1;
			PULSE_TOGGLE irq_pulse_or_toggle_mode : 1;
			CLOCK_SOURCE_0 clock_source : 2;
			INTERRUPT_REQUEST interrupt_request : 1;
			REACHED_TARGET reached_target : 1;
			REACHED_TARGET reached_ffff : 1;
			u32 : 19;
		} counter_0;
		struct {
			SYNC_ENABLE sync_enable : 1;
			SYNC_MODE_1 sync_mode : 2;
			RESET_COUNTER reset_counter : 1;
			DISABLE_ENABLE irq_when_counter_reaches_target : 1;
			DISABLE_ENABLE irq_when_counter_reaches_ffff : 1;
			ONCE_REPEAT irq_once_or_repeat_mode : 1;
			PULSE_TOGGLE irq_pulse_or_toggle_mode : 1;
			CLOCK_SOURCE_1 clock_source : 2;
			INTERRUPT_REQUEST interrupt_request : 1;
			REACHED_TARGET reached_target : 1;
			REACHED_TARGET reached_ffff : 1;
			u32 : 19;
		} counter_1;
		struct {
			SYNC_ENABLE sync_enable : 1;
			SYNC_MODE_2 sync_mode : 2;
			RESET_COUNTER reset_counter : 1;
			DISABLE_ENABLE irq_when_counter_reaches_target : 1;
			DISABLE_ENABLE irq_when_counter_reaches_ffff : 1;
			ONCE_REPEAT irq_once_or_repeat_mode : 1;
			PULSE_TOGGLE irq_pulse_or_toggle_mode : 1;
			CLOCK_SOURCE_2 clock_source : 2;
			INTERRUPT_REQUEST interrupt_request : 1;
			REACHED_TARGET reached_target : 1;
			REACHED_TARGET reached_ffff : 1;
			u32 : 19;
		} counter_2;
		u32 raw;
	} counter_mode;
	static_assert(sizeof(Counter_Mode) == sizeof(u32), "Union not at the expected size!");
}

void Timer::writeCurrentCounter(u8 timer, u32 data) {
	console->info("Counter {0:x} setting current ${1:x}", timer, data);
	current_counter[timer] = data & 0xffff;
}

void Timer::writeCounterMode(u8 timer, u32 data) {
	counter_mode.raw = data;
	counter_mode.counter_0.interrupt_request = INTERRUPT_REQUEST::no;
}

void Timer::writeCounterTarget(u8 timer, u32 data) {
	console->info("Counter {0:x} setting target ${1:x}", timer, data);
	counter_target[timer] = data & 0xffff;
}

u32 Timer::readCurrentCounter(u8 timer) {
	return current_counter[timer];
}

u32 Timer::readCounterMode(u8 timer) {
	u32 res = counter_mode.raw;
	counter_mode.counter_0.reached_target = REACHED_TARGET::no;
	counter_mode.counter_0.reached_ffff = REACHED_TARGET::no;
	return res;
}

u32 Timer::readCounterTarget(u8 timer) {
	return counter_target[timer];
}

void Timer::tick() {
	current_counter[0]++;
	current_counter[1]++;
	current_counter[2]++;
	//	TODO
	//	handle all the resets, wraps, irqs
}