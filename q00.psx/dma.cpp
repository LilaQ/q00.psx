#include "dma.h"
#include "mmu.h"

namespace DMA {

	std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("DMA");

	u32 dma2_target_clks = 0;	//	approx value of clk cycles necessary for the inited dma
	u32 dma2_clk_count = 0;

	u32 dma6_target_clks = 0;	//	approx value of clk cycles necessary for the inited dma
	u32 dma6_clk_count = 0;

	union DMA_Control_Register {
		struct {
			u32 dma0_mdecin_priority : 3;
			u32 dma0_mdecin_master_enable : 1;
			u32 dma1_mdecout_priority : 3;
			u32 dma1_mdecout_master_enable : 1;
			u32 dma2_gpu_priority : 3;
			u32 dma2_gpu_master_enable : 1;
			u32 dma3_cdrom_priority : 3;
			u32 dma3_cdrom_master_enable : 1;
			u32 dma4_spu_priority : 3;
			u32 dma4_spu_master_enable : 1;
			u32 dma5_pio_priority : 3;
			u32 dma5_pio_master_enable : 1;
			u32 dma6_otc_priority : 3;
			u32 dma6_otc_master_enable : 1;
			u32 : 4;
		};
		u32 raw = 0x0765'4321;
	} dma_control_register;
	static_assert(sizeof(DMA_Control_Register) == sizeof(u32), "Union not at the expected size!");

	union DMA_Interrupt_Register {
		struct {
			u32 : 15;
			u32 force_irq : 1;
			u32 irq_enable_setting_bit24_to_30_dma0 : 1;
			u32 irq_enable_setting_bit24_to_30_dma1 : 1;
			u32 irq_enable_setting_bit24_to_30_dma2 : 1;
			u32 irq_enable_setting_bit24_to_30_dma3 : 1;
			u32 irq_enable_setting_bit24_to_30_dma4 : 1;
			u32 irq_enable_setting_bit24_to_30_dma5 : 1;
			u32 irq_enable_setting_bit24_to_30_dma6 : 1;
			u32 irq_enable_setting_bit31_when_bit24_to_30_nonzero : 1;
			u32 irq_flags_for_dma0 : 1;
			u32 irq_flags_for_dma1 : 1;
			u32 irq_flags_for_dma2 : 1;
			u32 irq_flags_for_dma3 : 1;
			u32 irq_flags_for_dma4 : 1;
			u32 irq_flags_for_dma5 : 1;
			u32 irq_flags_for_dma6 : 1;
			u32 irq_signal : 1;
		};
		u32 raw;
	} dma_interrupt_register;
	static_assert(sizeof(DMA_Interrupt_Register) == sizeof(u32), "Union not at the expected size!");

	union DMA_Block_Control {
		struct {
			u32 number_of_word : 16;
			u32 : 16;
		} syncmode_0;
		struct {
			u32 blocksize : 16;
			u32 amount_of_blocks : 16;
		} syncmode_1;
		struct {
			u32 not_used : 32;
		} syncmode_2;
		u32 raw;
	};
	static_assert(sizeof(DMA_Block_Control) == sizeof(u32), "Union not at the expected size!");


	//	DMA channel control
	enum class TRANSFER_DIRECTION : u32 { to_main_ram = 0, from_main_ram = 1 };
	enum class MEMORY_ADDRESS_STEP : u32 { forward_plus_4 = 0, backward_minus_4 = 1 };
	enum class CHOPPING : u32 { normal = 0, chopping = 1 };
	enum class SYNC_MODE : u32 { start_immediately_transfer_all_at_once = 0, sync_blocks_to_dma_requests = 1, linked_list_mode = 2, reserved = 3 };
	enum class START_BUSY : u32 { stopped_completed = 0, busy = 1 };

	union DMA_Channel_Control {
		struct {
			TRANSFER_DIRECTION transfer_direction : 1;
			MEMORY_ADDRESS_STEP memory_address_step : 1;
			u32 : 6;
			CHOPPING chopping_enable : 1;
			SYNC_MODE sync_mode : 2;
			u32 : 5;
			u32 chopping_dma_window_size : 3;
			u32 : 1;
			u32 chopping_cpu_window_size : 3;
			u32 : 1;
			START_BUSY start_busy : 1;
			u32 : 3;
			u32 start_trigger : 1;
			u32 : 3;
		} flags;
		u32 raw;
	};
	static_assert(sizeof(DMA_Channel_Control) == sizeof(u32), "Union not at the expected size!");

	u32 dma_base_address[7];
	DMA_Block_Control dma_block_control[7];
	DMA_Channel_Control dma_channel_control[7];
	
	void DMA::writeDMABaseAddress(u32 data, u8 channel) {
		dma_base_address[channel] = data;
		console->info("Setting DMA (channel {0:x}) base address 0x{1:x}", channel, data);
	}

	void DMA::writeDMABlockControl(u32 data, u8 channel) {
		console->info("Setting DMA (channel {0:x}) block control 0x{1:x}", channel, data);
		dma_block_control[channel].raw = data;
	}

	void DMA::writeDMAChannelControl(u32 data, u8 channel) {
		console->info("Setting DMA (channel {0:x}) channel control 0x{1:x}", channel, data);

		if (channel == 6) {
			dma_channel_control[channel].raw = (data & 0x5100'0000) | 0b10;
		}
		else {
			dma_channel_control[channel].raw = data;
		}

		//	dma started
		if (dma_channel_control[channel].flags.start_trigger == 1 || dma_channel_control[channel].flags.start_busy == START_BUSY::busy) {
			console->info("DMA started on channel {0:x}", channel);

			switch (channel) {

				//	DMA2 - GPU
			case 2:
				//	control bits 0x0100'0200 (VramRead), 0x0100'0201 (VramWrite), 0x0100'0401 (List)
				if (dma_channel_control[2].raw == 0x0100'0200 || dma_channel_control[2].raw == 0x0100'0201 || dma_channel_control[2].raw == 0x0100'0401) {
					dma2_clk_count = 0;
					dma2_target_clks = dma_block_control[2].syncmode_1.blocksize * dma_block_control[2].syncmode_1.amount_of_blocks / 100 * 110;
				}

				//	DMA6 - OTC
				case 6:
					//	control bits
					if (dma_channel_control[6].raw == 0x1100'0002) {
						dma6_clk_count = 0;
						dma6_target_clks = dma_block_control[6].syncmode_0.number_of_word / 100 * 110;
					}
				break;
			}

			dma_channel_control[channel].flags.start_trigger = 0;
		}

	}

	void DMA::writeDMAControlRegister(u32 data) {
		console->info("Writing DMA control register [{0:x}]", data);
		dma_control_register.raw = data;
	}

	void writeDMAInterruptRegister(u32 data) {
		console->info("Writing DMA interrupt register [{0:x}]", data);
		dma_interrupt_register.raw = data;
	}



	//	
	//	Reads

	u32 DMA::readDMAControlRegister() {
		console->info("Reading DMA control register");
		return dma_control_register.raw;
	}

	u32 DMA::readDMAInterruptRegister() {
		console->info("Reading DMA interrupt register");
		return dma_interrupt_register.raw;
	}

	u32 DMA::readDMABaseAddress(u8 channel) {
		console->info("Reading DMA (channel {0:x}) base address", channel);
		return dma_base_address[channel];
	}

	u32 DMA::readDMAChannelControl(u8 channel) {
		//console->info("Reading DMA (channel {0:x}) channel control", channel);
		return dma_channel_control[channel].raw;
	}
}


//	
//	Tick
void DMA::tick() {

	//	DMA2 Send OTC to GPU/CPU
	if (dma_channel_control[2].flags.start_busy == START_BUSY::busy) {

		//	linked list
		if (dma_channel_control[2].flags.sync_mode == SYNC_MODE::linked_list_mode) {
			word val = Memory::readFromMemory<word>(dma_base_address[2]);
			u8 wordCount = val >> 24;

			const bool forward = dma_channel_control[2].flags.memory_address_step == MEMORY_ADDRESS_STEP::backward_minus_4;
			for (int i = 1; i <= wordCount; i++) {
				word command = 0;
				if (forward) {
					command = Memory::readFromMemory<word>(dma_base_address[2] - i * 0x4);
				}
				else {
					command = Memory::readFromMemory<word>(dma_base_address[2] + i * 0x4);
				}
				console->info("Sending GP0 command {0:x}", command);
				GPU::sendCommandGP0(command);
			}
			dma_base_address[2] = val & 0xff'ffff;
			
			//	end marker
			if (val & 0x80'0000) {
				dma_channel_control[2].flags.start_busy = START_BUSY::stopped_completed;
				console->info("Completed DMA2 linked list");
			}
		}

		//	vram
		else if (dma_channel_control[2].flags.sync_mode == SYNC_MODE::sync_blocks_to_dma_requests) {

			//	to main ram
			if (dma_channel_control[2].flags.transfer_direction == TRANSFER_DIRECTION::to_main_ram) {
				console->error("Missing DMA2 to_main_ram");
				exit(1);
				//	TODO	
				//	This should read from GPUREAD and store to Memory at MADR

				/*if (dma2_clk_count < dma2_target_clks) {
					const bool forward = dma_channel_control[2].flags.memory_address_step == MEMORY_ADDRESS_STEP::backward_minus_4;
					Memory::storeToMemory(dma_base_address[2] - dma2_clk_count * 0x4, (dma_base_address[2] - (dma2_clk_count + 1) * 0x4));
					dma2_clk_count++;
				}
				else {
					dma_channel_control[2].flags.start_busy = START_BUSY::stopped_completed;
					console->info("Completed DMA2 vram");
				}*/
			}

			//	from main ram
			else if (dma_channel_control[2].flags.transfer_direction == TRANSFER_DIRECTION::from_main_ram) {
				const bool forward = dma_channel_control[2].flags.memory_address_step == MEMORY_ADDRESS_STEP::backward_minus_4;
				word wordCount = dma_block_control[2].syncmode_1.blocksize * dma_block_control[2].syncmode_1.amount_of_blocks;
				for (int i = 0; i < wordCount; i++) {
					word command = 0;
					if (forward) {
						command = Memory::readFromMemory<word>(dma_base_address[2] - i * 4);
					}
					else {
						command = Memory::readFromMemory<word>(dma_base_address[2] + i * 4);
					}
					console->info("DMA2 - Syncmode 1 - Sending command to GPU {0:x}", command);
					GPU::sendCommandGP0(command);
				}
				dma_channel_control[2].flags.start_busy = START_BUSY::stopped_completed;
				console->info("Completed DMA2 Syncmode 1");
			}
		}
	}

	//	DMA6 OTC
	if (dma_channel_control[6].flags.start_busy == START_BUSY::busy) {
		//	debug, delete
		dma_channel_control[6].flags.start_busy = START_BUSY::stopped_completed;
		if (dma6_clk_count == (dma6_target_clks - 1)) {
			Memory::storeToMemory(dma_base_address[6] - dma6_clk_count * 0x4, 0x00ff'ffff);
			dma_channel_control[6].flags.start_busy = START_BUSY::stopped_completed;
			console->info("Completed DMA6");
			spdlog::set_level(spdlog::level::debug);
		}
		else {
			Memory::storeToMemory(dma_base_address[6] - dma6_clk_count * 0x4, (dma_base_address[6] - (dma6_clk_count + 1) * 0x4) & 0xff'ffff);
		}
		dma6_clk_count++;
	}
}