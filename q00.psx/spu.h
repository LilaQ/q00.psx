#pragma once
#ifndef SPU_GUARD
#define SPU_GUARD
#include "defs.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include "include/spdlog/pattern_formatter.h"

namespace SPU {

	extern std::shared_ptr<spdlog::logger> console;

	union Volume {
		struct {
			i16 left : 16;
			i16 right : 16;
		} channels;
		u32 raw;
	};

	union MainVolume {
		struct {
			i16 voice_volume : 15;
			i16 : 1;
		} fixed_volume_mode;
		struct {
			u16 sweep_step : 2;
			u16 sweep_shift : 5;
			u16 : 5;
			u16 sweep_phase : 1;
			u16 sweep_direction : 1;
			u16 sweep_mode : 1;
			u16 must_be_set : 1;
		} sweep_volume_mode;
		u16 raw;
	};

	extern u32 voice_key_on;
	extern u32 voice_key_off;
	extern u32 voice_on_off;
	extern u32 pitch_modulation_enable_flags;
	extern u32 voice_noise;
	extern u32 voice_reverb_mode;
	extern Volume cd_audio_input_volume;
	extern Volume external_audio_input_volume;

	void init();

	void write32bRegister(u32* reg, u16 data, bool upperHWord = false);
	u16 read32bRegister(u32* reg, bool upperHWord = false);


	//	Writes
	//:::::::::

	//	Voice registers
	void writeVoiceVolumeLeft(u16 data, u8 voice);
	void writeVoiceVolumeRight(u16 data, u8 voice);
	void writeVoiceADPCMSampleRate(u16 data, u8 voice);

	//	Control registers
	void writeSPUSTAT(u16 data);
	void writeSPUCNT(u16 data);
	void writeMainVolumeLeft(u16 data);
	void writeMainVolumeRight(u16 data);
	void writeReverbOutputVolumeLeft(u16 data);
	void writeReverbOutputVolumeRight(u16 data);
	void writeVoiceKeyOn(u16 data, bool upperHWord = false);
	void writeVoiceKeyOff(u16 data, bool upperHWord = false);
	void writeVoiceOnOff(u16 data, bool upperHWord = false);
	void writePitchModulationEnableFlags(u16 data, bool upperHWord = false);
	void writeVoiceNoise(u16 data, bool upperHWord = false);
	void writeVoiceReverbMode(u16 data, bool upperHWord = false);
	void writeSoundRAMDataReverbWorkAreaStartAddress(u16 data);
	void writeSoundRAMDataTransferAddress(u16 data);
	void writeSoundRAMDataTransferFifo(u16 data);
	void writeSoundRAMDataTransferControl(u16 data);
	void writeCDAudioInputVolume(u16 data, bool upperHWord = false);
	void writeExternalAudioInputVolume(u16 data, bool upperHWord = false);

	//	Reverb configuration
	void writeReverbConfiguration(u16 data, u16 offset);
	

	//	Reads
	//:::::::::

	//	Voice registers
	u16 readVoiceCurrentADSRVolume(u8 voice);

	//	Control registers
	u16 readSPUSTAT();
	u16 readSPUCNT();
	u32 readVoiceKeyOn(bool upperHWord = false);
	u32 readVoiceKeyOff(bool upperHWord = false);
	u32 readVoiceOnOff(bool upperHWord = false);
	u16 readSoundRAMDataTransferAddress();
	u16 readSoundRAMDataTransferFifo();
	u16 readSoundRAMDataTransferControl();

	

}

#endif