#include "spu.h"

namespace SPU {

	std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("SPU");

	byte* memory = new byte[0x8'0000] { 0x0000 };		//	512k

	union SPUSTAT {
		struct {
			u16 current_spu_mode : 6;
			u16 irq9_flag : 1;
			u16 data_transfer_dma_rw_request : 1;
			u16 data_transfer_dma_write_request : 1;
			u16 data_transfer_dma_read_request : 1;
			u16 data_transfer_busy_flag : 1;
			u16 capture_buffer_target : 1;
			u16 : 4;
		} flags;
		u16 raw;
	} spustat;
	static_assert(sizeof(spustat) == sizeof(u16), "Union not at the expected size!");

	union SPUCNT {
		struct {
			u16 cd_audio_enable : 1;
			u16 external_audio_enable : 1;
			u16 cd_audio_reverb : 1;
			u16 external_audio_reverb : 1;
			u16 sound_ram_transfer_mode : 2;
			u16 irq9_enable : 1;
			u16 reverb_master_enable : 1;
			u16 noise_frequency_step : 2;
			u16 noise_frequency_shift : 4;
			u16 mute_spu : 1;
			u16 spu_enable : 1;
		} flags;
		u16 raw;
	} spucnt;
	static_assert(sizeof(spucnt) == sizeof(u16), "Union not at the expected size!");

	MainVolume main_volume_left;
	MainVolume main_volume_right;
	MainVolume voice_volume_left[24];
	MainVolume voice_volume_right[24];

	i16 reverb_output_volume_left = 0;
	i16 reverb_output_volume_right = 0;

	u16 sound_ram_data_transfer_address = 0;
	u16 sound_ram_data_transfer_fifo = 0;
	u16 sound_ram_data_transfer_type = 0;

	u32 voice_key_on = 0;
	u32 voice_key_off = 0;
	u32 voice_on_off = 0;
	u32 pitch_modulation_enable_flags = 0;
	u32 voice_noise = 0;
	u32 voice_reverb_mode = 0;
	Volume cd_audio_input_volume;
	Volume external_audio_input_volume;
	u16 voice_adpcm_sample_rate[24];
}

void SPU::init() {
	console->info("SPU init");
}

void SPU::write32bRegister(u32* reg, u16 data, bool upperHWord) {
	*reg = upperHWord ? ((*reg & 0xffff) | (data << 16)) : ((*reg & 0xffff'0000) | data);
}

u16 SPU::read32bRegister(u32* reg, bool upperHWord) {
	return upperHWord ? (*reg >> 16) : (*reg & 0xffff);
}

void SPU::writeSPUSTAT(u16 data) {
	spustat.raw = data;
}

void SPU::writeSPUCNT(u16 data) {
	spucnt.raw = data;
}

void SPU::writeMainVolumeLeft(u16 data) {
	main_volume_left.raw = data;
}

void SPU::writeMainVolumeRight(u16 data) {
	main_volume_right.raw = data;
}

void SPU::writeVoiceVolumeLeft(u16 data, u8 voice) {
	voice_volume_left[voice].raw = data;
}

void SPU::writeVoiceVolumeRight(u16 data, u8 voice) {
	voice_volume_right[voice].raw = data;
}

void SPU::writeVoiceADPCMSampleRate(u16 data, u8 voice) {
	voice_adpcm_sample_rate[voice] = data;
}

void SPU::writeReverbOutputVolumeLeft(u16 data) {
	reverb_output_volume_left = data;
}

void SPU::writeReverbOutputVolumeRight(u16 data) {
	reverb_output_volume_right = data;
}

void SPU::writeVoiceKeyOn(u16 data, bool upperHWord) {
	write32bRegister(&voice_key_on, data, upperHWord);
}

void SPU::writeVoiceKeyOff(u16 data, bool upperHWord) {
	write32bRegister(&voice_key_off, data, upperHWord);
}

void SPU::writeVoiceOnOff(u16 data, bool upperHWord) {
	write32bRegister(&voice_on_off, data, upperHWord);
}

void SPU::writePitchModulationEnableFlags(u16 data, bool upperHWord) {
	write32bRegister(&pitch_modulation_enable_flags, data, upperHWord);
}

void SPU::writeVoiceNoise(u16 data, bool upperHWord) {
	write32bRegister(&voice_noise, data, upperHWord);
}

void SPU::writeVoiceReverbMode(u16 data, bool upperHWord) {
	write32bRegister(&voice_reverb_mode, data, upperHWord);
}

void SPU::writeSoundRAMDataTransferAddress(u16 data) {
	sound_ram_data_transfer_address = data;
}

void SPU::writeSoundRAMDataTransferFifo(u16 data) {
	sound_ram_data_transfer_fifo = data;
}

void SPU::writeSoundRAMDataTransferControl(u16 data) {
	sound_ram_data_transfer_type = (data >> 1) & 0b111;
}

void SPU::writeCDAudioInputVolume(u16 data, bool upperHWord) {
	write32bRegister(&cd_audio_input_volume.raw, data, upperHWord);
}

void SPU::writeExternalAudioInputVolume(u16 data, bool upperHWord) {
	write32bRegister(&external_audio_input_volume.raw, data, upperHWord);
}



//	Reads

u16 SPU::readSPUSTAT() {
	return spustat.raw;
}

u16 SPU::readSPUCNT() {
	return spucnt.raw;
}

u32 SPU::readVoiceKeyOn() {
	return voice_key_on;
}

u32 SPU::readVoiceKeyOff() {
	return voice_key_off;
}

u32 SPU::readVoiceOnOff() {
	return voice_on_off;
}

u16 SPU::readSoundRAMDataTransferAddress() {
	return sound_ram_data_transfer_address;
}

u16 SPU::readSoundRAMDataTransferFifo() {
	return sound_ram_data_transfer_fifo;
}

u16 SPU::readSoundRAMDataTransferControl() {
	return sound_ram_data_transfer_type << 1;
}