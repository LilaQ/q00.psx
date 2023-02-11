#pragma once
#ifndef MEM_GUARD
#define MEM_GUARD
#include "defs.h"
#include "gpu.h"
#include "cpu.h"
#include "spu.h"
#include "dma.h"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define MASKED_ADDRESS(a) (a & 0x1fff'ffff)
#define SHOW_BIOS_FUNCTIONS false

namespace Memory {

	static const char* A_FUNC_LUT[] = {
	"open(filename,accessmode)",
	"lseek(fd,offset,seektype)",
	"read(fd,dst,length)",
	"write(fd,src,length)",
	"close(fd)",
	"ioctl(fd,cmd,arg)",
	"exit(exitcode)",
	"isatty(fd)",
	"getc(fd)",
	"putc(char,fd)",
	"todigit(char)",
	"atof(src)     ;Does NOT work - uses (ABSENT) cop1 !!!",
	"strtoul(src,src_end,base)",
	"strtol(src,src_end,base)",
	"abs(val)",
	"labs(val)",
	"atoi(src)",
	"atol(src)",
	"atob(src,num_dst)",
	"setjmp(buf)",
	"longjmp(buf,param)",
	"strcat(dst,src)",
	"strncat(dst,src,maxlen)",
	"strcmp(str1,str2)",
	"strncmp(str1,str2,maxlen)",
	"strcpy(dst,src)",
	"strncpy(dst,src,maxlen)",
	"strlen(src)",
	"index(src,char)",
	"rindex(src,char)",
	"strchr(src,char)  ;exactly the same as 'index'",
	"strrchr(src,char) ;exactly the same as 'rindex'",
	"strpbrk(src,list)",
	"strspn(src,list)",
	"strcspn(src,list)",
	"strtok(src,list)  ;use strtok(0,list) in further calls",
	"strstr(str,substr)       ;Bugged",
	"toupper(char)",
	"tolower(char)",
	"bcopy(src,dst,len)",
	"bzero(dst,len)",
	"bcmp(ptr1,ptr2,len)      ;Bugged",
	"memcpy(dst,src,len)",
	"memset(dst,fillbyte,len)",
	"memmove(dst,src,len)     ;Bugged",
	"memcmp(src1,src2,len)    ;Bugged",
	"memchr(src,scanbyte,len)",
	"rand()",
	"srand(seed)",
	"qsort(base,nel,width,callback)",
	"strtod(src,src_end) ;Does NOT work - uses (ABSENT) cop1 !!!",
	"malloc(size)",
	"free(buf)",
	"lsearch(key,base,nel,width,callback)",
	"bsearch(key,base,nel,width,callback)",
	"calloc(sizx,sizy)            ;SLOW!",
	"realloc(old_buf,new_siz)     ;SLOW!",
	"InitHeap(addr,size)",
	"_exit(exitcode)",
	"getchar()",
	"putchar(char)",
	"gets(dst)",
	"puts(src)",
	"printf(txt,param1,param2,etc.)",
	"SystemErrorUnresolvedException()",
	"LoadTest(filename,headerbuf)",
	"Load(filename,headerbuf)",
	"Exec(headerbuf,param1,param2)",
	"FlushCache()",
	"init_a0_b0_c0_vectors",
	"GPU_dw(Xdst,Ydst,Xsiz,Ysiz,src)",
	"gpu_send_dma(Xdst,Ydst,Xsiz,Ysiz,src)",
	"SendGP1Command(gp1cmd)",
	"GPU_cw(gp0cmd)   ;send GP0 command word",
	"GPU_cwp(src,num) ;send GP0 command word and parameter words",
	"send_gpu_linked_list(src)",
	"gpu_abort_dma()",
	"GetGPUStatus()",
	"gpu_sync()",
	"SystemError",
	"SystemError",
	"LoadExec(filename,stackbase,stackoffset)",
	"GetSysSp",
	"SystemError           ;PS2: set_ioabort_handler(src)",
	"_96_init()",
	"_bu_init()",
	"_96_remove()  ;does NOT work due to SysDeqIntRP bug",
	"return 0",
	"return 0",
	"return 0",
	"return 0",
	"dev_tty_init()                                      ;PS2: SystemError",
	"dev_tty_open(fcb,and unused:'path/name',accessmode) ;PS2: SystemError",
	"dev_tty_out(fcb,cmd)                             ;PS2: SystemError",
	"dev_tty_ioctl(fcb,cmd,arg)                          ;PS2: SystemError",
	"dev_cd_open(fcb,'path/name',accessmode)",
	"dev_cd_read(fcb,dst,len)",
	"dev_cd_close(fcb)",
	"dev_cd_firstfile(fcb,'path/name',direntry)",
	"dev_cd_nextfile(fcb,direntry)",
	"dev_cd_chdir(fcb,'path')",
	"dev_card_open(fcb,'path/name',accessmode)",
	"dev_card_read(fcb,dst,len)",
	"dev_card_write(fcb,src,len)",
	"dev_card_close(fcb)",
	"dev_card_firstfile(fcb,'path/name',direntry)",
	"dev_card_nextfile(fcb,direntry)",
	"dev_card_erase(fcb,'path/name')",
	"dev_card_undelete(fcb,'path/name')",
	"dev_card_format(fcb)",
	"dev_card_rename(fcb1,'path/name1',fcb2,'path/name2')",
	"?   ;card ;[r4+18h]=00000000h  ;card_clear_error(fcb) or so",
	"_bu_init()",
	"_96_init()",
	"_96_remove()   ;does NOT work due to SysDeqIntRP bug",
	"return 0",
	"return 0",
	"return 0",
	"return 0",
	"return 0",
	"CdAsyncSeekL(src)",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"CdAsyncGetStatus(dst)",
	"return 0               ;DTL-H: Unknown?",
	"CdAsyncReadSector(count,dst,mode)",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"CdAsyncSetMode(mode)",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?, or reportedly, CdStop (?)",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"return 0               ;DTL-H: Unknown?",
	"CdromIoIrqFunc1()",
	"CdromDmaIrqFunc1()",
	"CdromIoIrqFunc2()",
	"CdromDmaIrqFunc2()",
	"CdromGetInt5errCode(dst1,dst2)",
	"CdInitSubFunc()",
	"AddCDROMDevice()",
	"AddMemCardDevice()     ;DTL-H: SystemError",
	"AddDuartTtyDevice()    ;DTL-H: AddAdconsTtyDevice ;PS2: SystemError",
	"add_nullcon_driver()",
	"SystemError            ;DTL-H: AddMessageWindowDevice",
	"SystemError            ;DTL-H: AddCdromSimDevice",
	"SetConf(num_EvCB,num_TCB,stacktop)",
	"GetConf(num_EvCB_dst,num_TCB_dst,stacktop_dst)",
	"SetCdromIrqAutoAbort(type,flag)",
	"SetMem(megabytes)",
	};
	static const char* B_FUNC_LUT[] = {
		"alloc_kernel_memory(size)",
		"free_kernel_memory(buf)",
		"init_timer(t,reload,flags)",
		"get_timer(t)",
		"enable_timer_irq(t)",
		"disable_timer_irq(t)",
		"restart_timer(t)",
		"DeliverEvent(class, spec)",
		"OpenEvent(class,spec,mode,func)",
		"CloseEvent(event)",
		"WaitEvent(event)",
		"TestEvent(event)",
		"EnableEvent(event)",
		"DisableEvent(event)",
		"OpenTh(reg_PC,reg_SP_FP,reg_GP)",
		"CloseTh(handle)",
		"ChangeTh(handle)",
		"jump_to_00000000h",
		"InitPAD2(buf1,siz1,buf2,siz2)",
		"StartPAD2()",
		"StopPAD2()",
		"PAD_init2(type,button_dest,unused,unused)",
		"PAD_dr()",
		"ReturnFromException()",
		"ResetEntryInt()",
		"HookEntryInt(addr)",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"UnDeliverEvent(class,spec)",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"SystemError  ;PS2: return 0",
		"SystemError  ;PS2: return 0",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"jump_to_00000000h",
		"open(filename,accessmode)",
		"lseek(fd,offset,seektype)",
		"read(fd,dst,length)",
		"write(fd,src,length)",
		"close(fd)",
		"ioctl(fd,cmd,arg)",
		"exit(exitcode)",
		"isatty(fd)",
		"getc(fd)",
		"putc(char,fd)",
		"getchar()",
		"putchar(char)",
		"gets(dst)",
		"puts(src)",
		"cd(name)",
		"format(devicename)",
		"firstfile2(filename,direntry)",
		"nextfile(direntry)",
		"rename(old_filename,new_filename)",
		"erase(filename)",
		"undelete(filename)",
		"AddDrv(device_info)  ;subfunction for AddXxxDevice functions",
		"DelDrv(device_name_lowercase)",
		"PrintInstalledDevices()"
	};
	static const char* C_FUNC_LUT[] = {
		"EnqueueTimerAndVblankIrqs(priority)",
		"EnqueueSyscallHandler(priority)",
		"EnqueueTimerAndVblankIrqs(priority) ;used with prio=1",
		"EnqueueSyscallHandler(priority)     ;used with prio=0",
		"SysEnqIntRP(priority,struc)  ;bugged, use with care",
		"SysDeqIntRP(priority,struc)  ;bugged, use with care",
		"get_free_EvCB_slot()",
		"get_free_TCB_slot()",
		"ExceptionHandler()",
		"InstallExceptionHandlers()  ;destroys/uses k0/k1",
		"SysInitMemory(addr,size)",
		"SysInitKernelVariables()",
		"ChangeClearRCnt(t,flag)",
		"SystemError  ;PS2: return 0",
		"InitDefInt(priority) ;used with prio=3",
		"SetIrqAutoAck(irq,flag)",
		"return 0               ;DTL-H2000: dev_sio_init",
		"return 0               ;DTL-H2000: dev_sio_open",
		"return 0               ;DTL-H2000: dev_sio_out",
		"return 0               ;DTL-H2000: dev_sio_ioctl",
		"InstallDevices(ttyflag)",
		"FlushStdInOutPut()",
		"return 0               ;DTL-H2000: SystemError",
		"_cdevinput(circ,char)",
		"_cdevscan()",
		"_circgetc(circ)    ;uses r5 as garbage txt for _ioabort",
		"_circputc(char,circ)",
		"_ioabort(txt1,txt2)",
		"set_card_find_mode(mode)  ;0=normal, 1=find deleted files",
		"KernelRedirect(ttyflag)   ;PS2: ttyflag=1 causes SystemError",
		"AdjustA0Table()",
		"get_card_find_mode()"
	};

	union I_STAT_MASK {
		struct {
			u32 irq0_vblank : 1;
			u32 irq1_gpu : 1;
			u32 irq2_cdrom : 1;
			u32 irq3_dma : 1;
			u32 irq4_tmr0 : 1;
			u32 irq5_tmr1 : 1;
			u32 irq6_tmr2 : 1;
			u32 irq7_controller_and_memcard_byte_received : 1;
			u32 irq8_sio : 1;
			u32 irq9_spu : 1;
			u32 irq10_controller_lightpen : 1;
			u32 : 21;
		};
		u32 raw;
	};
	static_assert(sizeof(I_STAT_MASK) == sizeof(u32), "Union not at the expected size!");

	extern I_STAT_MASK I_STAT;
	extern I_STAT_MASK I_MASK;
	extern u8* memory;
	extern std::shared_ptr<spdlog::logger> memConsole;

	void init();
	
	template<typename T>
	T readFromMemory(word address) {
		//	byte / u8
		if constexpr (sizeof(T) == sizeof(u8)) {
			return memory[MASKED_ADDRESS(address)];
		}
		//	hword / u16
		else if constexpr (sizeof(T) == sizeof(u16)) {
			address &= 0xffff'fffe;
			hword res = memory[MASKED_ADDRESS(address)];
			res |= memory[MASKED_ADDRESS(address + 1)] << 8;
			return res;
		}
		//	word / u32
		else if constexpr (sizeof(T) == sizeof(u32)) {
			address &= 0xffff'fffc;
			word res = memory[MASKED_ADDRESS(address)];
			res |= memory[MASKED_ADDRESS(address + 1)] << 8;
			res |= memory[MASKED_ADDRESS(address + 2)] << 16;
			res |= memory[MASKED_ADDRESS(address + 3)] << 24;
			return res;
		}
	}

	template<typename T>
	void storeToMemory(word address, T data) {
		if (!R3000A::cop[0].sr.flags.isolate_cache) {
			//	byte / u8
			if constexpr (sizeof(T) == sizeof(u8)) {
				memory[MASKED_ADDRESS(address)] = data;
			}
			//	hword / u16
			else if constexpr (sizeof(T) == sizeof(u16)) {
				address &= 0xffff'fffe;
				memory[MASKED_ADDRESS(address)] = data & 0xff;
				memory[MASKED_ADDRESS(address + 1)] = (data >> 8) & 0xff;
			}
			//	word / u32
			else if constexpr (sizeof(T) == sizeof(u32)) {
				address &= 0xffff'fffc;
				memory[MASKED_ADDRESS(address)] = data & 0xff;
				memory[MASKED_ADDRESS(address + 1)] = (data >> 8) & 0xff;
				memory[MASKED_ADDRESS(address + 2)] = (data >> 16) & 0xff;
				memory[MASKED_ADDRESS(address + 3)] = (data >> 24) & 0xff;
			}
		}
	}


	/*
		KUSEG     KSEG0     KSEG1
		00000000h 80000000h A0000000h  2048K  Main RAM (first 64K reserved for BIOS)
		1F000000h 9F000000h BF000000h  8192K  Expansion Region 1 (ROM/RAM)
		1F800000h 9F800000h    --      1K     Scratchpad (D-Cache used as Fast RAM)
		1F801000h 9F801000h BF801000h  8K     I/O Ports
		1F802000h 9F802000h BF802000h  8K     Expansion Region 2 (I/O Ports)
		1FA00000h 9FA00000h BFA00000h  2048K  Expansion Region 3 (SRAM BIOS region for DTL cards)
		1FC00000h 9FC00000h BFC00000h  512K   BIOS ROM (Kernel) (4096K max)
			FFFE0000h (in KSEG2)     0.5K   Internal CPU control registers (Cache Control)
	*/


	template <typename T>
	T fetch(word address) {
		address = MASKED_ADDRESS(address);

		//	RAM
		if (address < 0x1f00'0000) {

			//	Thunk function calls
			byte thunkFunctionId = R3000A::registers.r[9];
			if ((address == 0xa0 && thunkFunctionId == 0x3c) ||
				(address == 0xb0 && thunkFunctionId == 0x3d)) {
				printf("%c", R3000A::registers.r[4]);
			}
			else if (address == 0xa0 && SHOW_BIOS_FUNCTIONS) {
				memConsole->info("A-Function ({0:x}) - {1:s}", thunkFunctionId, A_FUNC_LUT[thunkFunctionId]);
			}
			else if (address == 0xb0 && SHOW_BIOS_FUNCTIONS) {
				memConsole->info("B-Function ({0:x}) - {1:s}", thunkFunctionId, B_FUNC_LUT[thunkFunctionId]);
			}
			else if (address == 0xc0 && SHOW_BIOS_FUNCTIONS) {
				memConsole->info("C-Function ({0:x}) - {1:s}", thunkFunctionId, C_FUNC_LUT[thunkFunctionId]);
			}

			return readFromMemory<T>(address);
		}


		//	Expansion Region 1
		else if (address < 0x1f80'0000) {
			//memConsole->error("Reading from unknown address on Expansion Region 1 {0:x}", address);
			//exit(1);
			return readFromMemory<T>(address);
		}


		//	Scratchpad
		else if (address < 0x1f80'1000) {
			memConsole->error("Reading from unknown address on Scratchpad {0:x}", address);
			exit(1);
		}


		//	I/O Ports
		else if (address < 0x1f80'2000) {

			//	GPUREAD
			if (address == 0x1f80'1810) {
				return GPU::readGPUREAD();
			}
			
			//	GPUSTAT
			else if (address == 0x1f80'1814) {
				return GPU::readGPUSTAT();
			}
			
			//	I_STAT - Interrupt Status Register
			else if (address == 0x1f80'1070) {
				memConsole->debug("Reading from I_STAT");
				return I_STAT.raw;
			}
			
			//	I_MASK - Interrupt Mask Register
			else if (address == 0x1f80'1074) {
				memConsole->debug("Reading from I_MASK");
				return I_MASK.raw;
			}

			//	DMA Registers
			else if (address >= 0x1f80'1080 && address < 0x1f80'1100) {
				u8 channel = (address % 0x1f80'1084) >> 4;

				//	DMA control register
				if (address == 0x1f80'10f0) {
					return DMA::readDMAControlRegister();
				}

				//	DMA interrupt register
				else if (address == 0x1f80'10f4) {
					return DMA::readDMAInterruptRegister();
				}
					
				//	DMA base address (for channel N)
				else if ((address & 0b1111) == 0) {
					if (channel == 1) {
						spdlog::set_level(spdlog::level::debug);
					}
					return DMA::readDMABaseAddress(channel);
				}

				//	DMA block control
				else if ((address & 0b1111) == 4) {
					memConsole->error("Reading DMA (channel {0:x}) block control [${1:08x}]", channel, address);
					exit(1);
				}

				//	DMA channel control
				else if ((address & 0b1111) == 8) {
					return DMA::readDMAChannelControl(channel);
				}

				else {
					memConsole->error("Invalid DMA read at {0:x}", address);
					exit(1);
				}
			}

			//	SPU
			else if (address >= 0x1f80'1c00 && address < 0x1f80'2000) {

				//	Voice volume
				if (address >= 0x1f80'1c00 && address <= 0x1f80'1d7f) {
					u8 voice = (address % 0x01f801c00) >> 4;
					if ((address & 0b1111) == 0) {
						//SPU::writeVoiceVolumeLeft(data, voice);
					}
					else if ((address & 0b1111) == 2) {
						//SPU::writeVoiceVolumeRight(data, voice);
					}
					else if ((address & 0b1111) == 4) {
						//SPU::writeVoiceADPCMSampleRate(data, voice);
					}
					else if ((address & 0b1111) == 0xc) {
						return SPU::readVoiceCurrentADSRVolume(voice);
					}
				}

				//	SPUSTAT
				else if (address == 0x1f80'1dae) {
					return SPU::readSPUSTAT();
				}

				//	SPUCNT
				else if (address == 0x1f80'1daa) {
					return SPU::readSPUCNT();
				}

				//	Voice Key ON
				else if (address == 0x1f80'1d88) {
					return SPU::readVoiceKeyOn();
				}
				else if (address == 0x1f80'1d8a) {
					return SPU::readVoiceKeyOn(true);
				}

				//	Voice Key OFF
				else if (address == 0x1f80'1d8c) {
					return SPU::readVoiceKeyOff();
				}
				else if (address == 0x1f80'1d8e) {
					return SPU::readVoiceKeyOff(true);
				}

				//	Voice ON/OFF
				else if (address == 0x1f80'1d9c) {
					return SPU::readVoiceOnOff();
				}

				//	Sound RAM data transfer address
				else if (address == 0x1f80'1da6) {
					return SPU::readSoundRAMDataTransferAddress();
				}

				//	Sound RAM data transfer fifo
				else if (address == 0x1f80'1da8) {
					return SPU::readSoundRAMDataTransferFifo();
				}

				//	Sound RAM data transfer control
				else if (address == 0x1f80'1dac) {
					return SPU::readSoundRAMDataTransferControl();
				}

				else {
					memConsole->error("SPU read detected at {0:x}", address);
					//return 0xffffffff;
					exit(1);
				}
			}

			//	Timers
			else if (address >= 0x1f80'1100 && address < 0x1f80'1128) {
				memConsole->error("Implement timers, bitch! {0:x}", address);
				exit(1);
			}

			//	all other reads
			else {
				//memConsole->error("Reading from unknown address on I/O range {0:x}", address);
				//exit(1);
				return readFromMemory<T>(address);
			}
		}


		//	Expansion Region 2
		else if (address < 0x1fa0'0000) {
			memConsole->error("Reading from unknown address on Expansion Region 2 {0:x}", address);
			exit(1);
		}


		//	Expansion Region 3
		else if (address < 0x1fc0'0000) {
			memConsole->error("Reading from unknown address on Expansion Region 2 {0:x}", address);
			exit(1);
		}


		//	BIOS
		else if (address < 0x2000'0000) {
			return readFromMemory<T>(address);
		}
		

		//	Invalid reads
		else {
			memConsole->error("Invalid read at {0:x}", address);
			exit(1);
		}
	}

	template <typename T>
	void store(word address, T data) {
		address = MASKED_ADDRESS(address);

		//	RAM
		if (address < 0x1f00'0000) {
			storeToMemory<T>(address, data);
		}


		//	Expansion Region 1
		else if (address < 0x1f80'0000) {
			storeToMemory<T>(address, data);
		}


		//	Scratchpad
		else if (address < 0x1f80'1000) {
			storeToMemory<T>(address, data);
		}


		//	I/O
		else if (address < 0x1f80'2000) {

			//	Memory Control 1
			if (address >= 0x1f80'1000 && address <= 0x1f80'1020) {
				memConsole->debug("Writing Memory Control 1 (${0:x})", address);
				storeToMemory<T>(address, data);
			}

			//	Memory Control 2 
			else if (address == 0x1f80'1060) {
				memConsole->debug("Writing Memory Control 2 (${0:x})", address);
				storeToMemory<T>(address, data);
			}

			//	GP0 Send Command
			else if (address == 0x1f80'1810) {
				GPU::sendCommandGP0(data);
			}

			//	GP1 Send Command
			else if (address == 0x1f80'1814) {
				GPU::sendCommandGP1(data);
			}

			//	I_STAT - Interrupt Status Register
			else if (address == 0x1f80'1070) {
				I_STAT.raw = data;
				memConsole->info("Writing to I_STAT (${0:x})", data);
			}

			//	I_MASK - Interrupt Mask Register
			else if (address == 0x1f80'1074) {
				I_MASK.raw = data;
				memConsole->info("Writing to I_MASK (${0:x})", data);
			}

			//	DMA Registers
			else if (address >= 0x1f80'1080 && address < 0x1f80'1100) {
				u8 channel = (address % 0x1f80'1080) >> 4;

				//	DMA control register
				if (address == 0x1f80'10f0) {
					DMA::writeDMAControlRegister(data);
				}

				//	DMA interrupt register
				else if (address == 0x1f80'10f4) {
					DMA::writeDMAInterruptRegister(data);
				}

				//	DMA base address (for channel N)
				else if ((address & 0b1111) == 0) {
					u32 addr = data & 0xff'ffff;
					DMA::writeDMABaseAddress(addr, channel);
				}

				//	DMA block control
				else if ((address & 0b1111) == 4) {
					DMA::writeDMABlockControl(data, channel);
				}

				//	DMA channel control
				else if ((address & 0b1111) == 8) {
					DMA::writeDMAChannelControl(data, channel);
				}

				else {
					memConsole->error("Invalid DMA write at {0:x}", address);
					exit(1);
				}
			}

			//	SPU
			else if (address >= 0x1f80'1c00 && address < 0x1f80'2000) {

				//	Voice volume
				if (address >= 0x1f80'1c00 && address <= 0x1f80'1d7f) {
					u8 voice = (address % 0x01f801c00) >> 4;
					if ((address & 0b1111) == 0) {
						SPU::writeVoiceVolumeLeft(data, voice);
					}
					else if ((address & 0b1111) == 2) {
						SPU::writeVoiceVolumeRight(data, voice);
					}
					else if ((address & 0b1111) == 4) {
						SPU::writeVoiceADPCMSampleRate(data, voice);
					}
				}

				//	SPUSTAT
				else if (address == 0x1f80'1dae) {
					SPU::writeSPUSTAT;
				}

				//	SPUCNT
				else if (address == 0x1f80'1daa) {
					SPU::writeSPUCNT(data);
				}

				//	Mainvolume left
				else if (address == 0x1f80'1d80) {
					SPU::writeMainVolumeLeft(data);
				}

				//	Mainvolume right
				else if (address == 0x1f80'1d82) {
					SPU::writeMainVolumeRight(data);
				}

				//	Reverb output volume left
				else if (address == 0x1f80'1d84) {
					SPU::writeReverbOutputVolumeLeft(data);
				}

				//	Reverb output volume right
				else if (address == 0x1f80'1d86) {
					SPU::writeReverbOutputVolumeRight(data);
				}

				//	Voice Key ON
				else if (address == 0x1f80'1d88) {
					SPU::writeVoiceKeyOn(data);
				}
				else if (address == 0x1f80'1d8a) {
					SPU::writeVoiceKeyOn(data, true);
				}

				//	Voice Key OFF
				else if (address == 0x1f80'1d8c) {
					SPU::writeVoiceKeyOff(data);
				}
				else if (address == 0x1f80'1d8e) {
					SPU::writeVoiceKeyOff(data, true);
				}

				//	Pitch modulation enable flags
				else if (address == 0x1f80'1d90) {
					SPU::writePitchModulationEnableFlags(data);
				}
				else if (address == 0x1f80'1d92) {
					SPU::writePitchModulationEnableFlags(data, true);
				}

				//	Voice Noise
				else if (address == 0x1f80'1d94) {
					SPU::writeVoiceNoise(data);
				}
				else if (address == 0x1f80'1d96) {
					SPU::writeVoiceNoise(data, true);
				}

				//	Voice Reverb Mode
				else if (address == 0x1f80'1d98) {
					SPU::writeVoiceReverbMode(data);
				}
				else if (address == 0x1f80'1d9a) {
					SPU::writeVoiceReverbMode(data, true);
				}

				//	Voice ON/OFF
				else if (address == 0x1f80'1d9c) {
					SPU::writeVoiceOnOff(data);
				}

				//	Sound RAM data reverb work area start address
				else if (address == 0x1f80'1da2) {
					SPU::writeSoundRAMDataReverbWorkAreaStartAddress(data);
				}

				//	Sound RAM data transfer address
				else if (address == 0x1f80'1da6) {
					SPU::writeSoundRAMDataTransferAddress(data);
				}

				//	Sound RAM data transfer fifo
				else if (address == 0x1f80'1da8) {
					SPU::writeSoundRAMDataTransferFifo(data);
				}

				//	Sound RAM data transfer control
				else if (address == 0x1f80'1dac) {
					SPU::writeSoundRAMDataTransferControl(data);
				}

				//	CD audio input volume
				else if (address == 0x1f80'1db0) {
					SPU::writeCDAudioInputVolume(data);
				}
				else if (address == 0x1f80'1db2) {
					SPU::writeCDAudioInputVolume(data, true);
				}

				//	External audio input volume
				else if (address == 0x1f80'1db4) {
					SPU::writeExternalAudioInputVolume(data);
				}
				else if (address == 0x1f80'1db6) {
					SPU::writeExternalAudioInputVolume(data, true);
				}

				//	Reverb configuration
				else if (address >= 0x1f80'1dc0 && address < 0x1f80'1e00) {
					SPU::writeReverbConfiguration(data, address - 0x1f80'1dc0);
				}

				else {
					memConsole->error("SPU Write detected to {0:x} at size {1:x}", address, sizeof(T));
					exit(1);
				}
			}

		}


		//	all other writes
		else {
			//memConsole->error("Write to unknown destination {0:x}", address);
			//exit(1);
			storeToMemory<T>(address, data);
		}
	}

	void initEmptyOrderingTable(word base_address, word number_of_words);

	void loadToRAM(word, byte*, word, word);
	void loadBIOS(byte* source, word size);
	void dumpRAM();

}

#endif