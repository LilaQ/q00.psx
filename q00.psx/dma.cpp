#include "dma.h"

namespace DMA {

	std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("DMA");

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

	union DMA_Channel_Control {
		struct {
			u32 transfer_direction : 1;
			u32 memory_address_step : 1;
			u32 : 6;
			u32 chopping_enable : 1;
			u32 sync_mode : 2;
			u32 : 5;
			u32 chopping_dma_window_size : 3;
			u32 : 1;
			u32 chopping_cpu_window_size : 3;
			u32 : 1;
			u32 start_busy : 1;
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
		console->info("Setting DMA (channel {0:x}) base address 0x{1:x}", channel, data);
		dma_base_address[channel] = data;
	}

	void DMA::writeDMABlockControl(u32 data, u8 channel) {
		console->info("Setting DMA (channel {0:x}) block control 0x{1:x}", channel, data);
		dma_block_control[channel].raw = data;
	}

	void DMA::writeDMAChannelControl(u32 data, u8 channel) {
		console->info("Setting DMA (channel {0:x}) channel control 0x{1:x}", channel, data);
		dma_channel_control[channel].raw = data;
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
}