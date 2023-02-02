#define _CRT_SECURE_NO_WARNINGS
#include "defs.h"
#include "mmu.h"
#include "gpu.h"
#include "cpu.h"
#include "spu.h"
#include "fileimport.h"
#include <iostream>
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#define MASKED_ADDRESS(a) (a & 0x1fff'ffff)
#define MEMORY_REGION(a) (a & 0xf'ffff)
#define LOCALIZED_ADDRESS(a) MASKED_ADDRESS((a & addressMask))
#define CPU R3000A

static auto console = spdlog::stdout_color_mt("Memory");

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

class Mem {
	protected:
		
		word addressMask;
		std::shared_ptr<spdlog::logger> memConsole;
	public:
		u8* memory;
		//	log
		virtual void readByteLog(word address) {
			memConsole->debug("Reading from 0x{0:x}", address);
		};
		virtual void readHalfwordLog(word address) {
			memConsole->debug("Reading from 0x{0:x}", address);
		};
		virtual void readWordLog(word address) {
			memConsole->debug("Reading from 0x{0:x}", address);
		};
		virtual void storeByteLog(word address, byte data) {
			memConsole->debug("Storing to address 0x{0:x}, data ${1:02x}", address, data);
		};
		virtual void storeWordLog(word address, word data) {
			memConsole->debug("Storing to address 0x{0:x}, data ${1:08x}", address, data);
		};

		virtual byte readByte(word address) {
			readByteLog(LOCALIZED_ADDRESS(address));
			return memory[LOCALIZED_ADDRESS(address)];
		};
		virtual hword readHalfword(word address) {
			readHalfwordLog(LOCALIZED_ADDRESS(address));
			hword res = memory[LOCALIZED_ADDRESS(address)];
			res |= memory[LOCALIZED_ADDRESS(address + 1)] << 8;
			return res;
		};
		virtual word readWordWithoutLog(word address) {
			word res = memory[LOCALIZED_ADDRESS(address)];
			res |= memory[LOCALIZED_ADDRESS(address + 1)] << 8;
			res |= memory[LOCALIZED_ADDRESS(address + 2)] << 16;
			res |= memory[LOCALIZED_ADDRESS(address + 3)] << 24;
			return res;
		}
		virtual word readWord(word address) {
			readWordLog(address);
			return readWordWithoutLog(address);
		};
		virtual void storeByte(word address, byte data) {
			storeByteLog(LOCALIZED_ADDRESS(address), data);
			memory[LOCALIZED_ADDRESS(address)] = data;
		};
		virtual void storeHalfword(word address, hword data) {
			storeWordLog(LOCALIZED_ADDRESS(address), data);
			memory[LOCALIZED_ADDRESS(address)] = data & 0xff;
			memory[LOCALIZED_ADDRESS(address + 1)] = (data >> 8) & 0xff;
		};
		virtual void storeWord(word address, word data) {
			storeWordLog(LOCALIZED_ADDRESS(address), data);
			memory[LOCALIZED_ADDRESS(address)] = data & 0xff;
			memory[LOCALIZED_ADDRESS(address + 1)] = (data >> 8) & 0xff;
			memory[LOCALIZED_ADDRESS(address + 2)] = (data >> 16) & 0xff;
			memory[LOCALIZED_ADDRESS(address + 3)] = (data >> 24) & 0xff;
		};
};

class RAM : public Mem {			//	2048k (first 64k reserved for BIOS) - RAM
	public: 
		void load(word targetAddress, byte* source, word offset, word size) {
			memcpy(&memory[MASKED_ADDRESS(targetAddress)], &source[offset], sizeof(byte) * size);
		}
		RAM() {
			memory = new u8[0x200'000];
			addressMask = 0x1f'ffff;
			memConsole = spdlog::stdout_color_mt("RAM");
		}

		word readWord(word address) {
			address = LOCALIZED_ADDRESS(address);
			byte funcId = R3000A::registers.r[9];
			if ((address == 0xa0 && funcId == 0x3c) || 
				(address == 0xb0 && funcId == 0x3d))  {
				printf("%c", R3000A::registers.r[4]);
			}
			else if (address == 0xa0) {
				memConsole->info("A-Function ({0:x}) - {1:s}", funcId, A_FUNC_LUT[funcId]);
			}
			else if (address == 0xb0) {
				memConsole->info("B-Function ({0:x}) - {1:s}", funcId, B_FUNC_LUT[funcId]);
			}
			else if (address == 0xc0) {
				memConsole->info("C-Function ({0:x}) - {1:s}", funcId, C_FUNC_LUT[funcId]);
			}

			return readWordWithoutLog(address);
		}
} mRam;

class Exp1 : public Mem {			//	8192k - Expansion Region 1
	public:
		Exp1() {
			memory = new u8[0x800'000];
			addressMask = 0x7f'ffff;
			memConsole = spdlog::stdout_color_mt("EXP1");
		}
} mExp1;

class Scratchpad : public Mem {		//	1k - Scratchpad
	public:
		Scratchpad() {
			memory = new u8[0x400];
			addressMask = 0x3ff;
			memConsole = spdlog::stdout_color_mt("SCRATCHPAD");
		}
} mScratchpad;

class IO : public Mem {				//	8k - I/O 

	word I_STAT;
	word I_MASK;

	public:
		IO() {
			memory = new u8[0x2000];
			addressMask = 0x1fff;
			memConsole = spdlog::stdout_color_mt("IO");
		}

		void storeWord(word address, word data) {
			address = LOCALIZED_ADDRESS(address);

			//	Memory Control 1
			if (address >= 0x1000 && address <= 0x1020) {
				memConsole->info("Writing Memory Control 1 (${0:x})", address);
				memory[address] = data & 0xff;
				memory[address + 1] = (data >> 8) & 0xff;
				memory[address + 2] = (data >> 16) & 0xff;
				memory[address + 3] = (data >> 24) & 0xff;
			}

			//	Memory Control 2 
			else if (address == 0x1060) {
				memConsole->info("Writing Memory Control 2 (${0:x})", address);
				memory[address] = data & 0xff;
				memory[address + 1] = (data >> 8) & 0xff;
				memory[address + 2] = (data >> 16) & 0xff;
				memory[address + 3] = (data >> 24) & 0xff;
			}

			//	GP0 Send Command
			else if (address == 0x1810) {
				GPU::sendCommandGP0(data);
			}
			//	GP1 Send Command
			else if (address == 0x1814) {
				GPU::sendCommandGP1(data);
			}

			//	I_STAT - Interrupt Status Register
			else if (address == 0x1070) {
				memConsole->info("Writing to I_STAT (${0:x})", data);
				I_STAT = data;
			}
			//	I_MASK - Interrupt Mask Register
			else if (address == 0x1074) {
				memConsole->info("Writing to I_MASK (${0:x})", data);
				I_MASK = data;
			}

			//	DMA Registers
			else if (address >= 0x1080 && address < 0x1100) {
				memConsole->debug("DMA write at {0:x}", address);
			}

			//	SPU
			else if (address >= 0x1c00 && address < 0x2000) {
				SPU::storeWord(address, data);
			}

			//	all other writes
			else {
				console->error("Write to unknown destination {0:x}", address);
				exit(1);
			}
		}

		word readWord(word address) {
			address = LOCALIZED_ADDRESS(address);

			//	GPUREAD
			if (address == 0x1810) {
				return GPU::readGPUREAD();
			}
			//	GPUSTAT
			else if (address == 0x1814) {
				return GPU::readGPUSTAT();
			}
			//	I_STAT - Interrupt Status Register
			else if (address == 0x1070) {
				memConsole->info("Reading from I_STAT");
				return I_STAT;
			}
			//	I_MASK - Interrupt Mask Register
			else if (address == 0x1074) {
				memConsole->info("Reading from I_MASK");
				return I_MASK;
			}

			//	DMA Registers
			else if (address >= 0x1080 && address < 0x1100) {
				memConsole->debug("DMA read at {0:x}", address);
			}

			//	SPU
			else if (address >= 0x1c00 && address < 0x2000) {
				return SPU::readWord(address);
			}

			//	all other reads
			else {
				console->error("Read word from unknown destination {0:x}", address);
				exit(1);
			}
		}

		byte readByte(word address) {
			address = LOCALIZED_ADDRESS(address);
			
			if (address >= 0x1c00 && address < 0x2000) {
				return SPU::readByte(address);
			}
			else {
				console->error("Read byte from unknown destination {0:x}", address);
				exit(1);
			}
		}

		void writeByte(word address, byte data) {
			address = LOCALIZED_ADDRESS(address);
			if (address >= 0x1c00 && address < 0x2000) {
				return SPU::storeByte(address, data);
			}
			else {
				console->error("Write to unknown destination");
				exit(1);
			}
		}

		hword readHalfword(word address) {
			address = LOCALIZED_ADDRESS(address);
			
			if (address >= 0x1c00 && address < 0x2000) {
				return SPU::readHalfword(address);
			}
			else {
				console->error("Read hword from unknown destination {0:x}", address);
				exit(1);
			}
		}

		void writeHalfword(word address, hword data) {
			address = LOCALIZED_ADDRESS(address);
			if (address >= 0x1c00 && address < 0x2000) {
				return SPU::storeHalfword(address, data);
			}
			else {
				console->error("Write to unknown destination");
				exit(1);
			}
		}

} mIo;

class Exp2 : public Mem {			//	8k - Expansion Region 2
	public:
		Exp2() {
			memory = new u8[0x2000];
			addressMask = 0x1fff;
			memConsole = spdlog::stdout_color_mt("EXP2");
		}
} mExp2;

class Expe : public Mem {			//	2048k - Expansion Region 3
	public:
		Expe() {
			memory = new u8[0x200'000];
			addressMask = 0x1f'ffff;
			memConsole = spdlog::stdout_color_mt("EXPE");
		}
} mExpe;

class BIOS : public Mem {			//	512k - BIOS ROM
	public:
		void load(byte* source, word size) {
			memcpy(&memory[0x0000], &source[0x0000], sizeof(byte) * size);
		}
		BIOS() {
			memory = new u8[0x80'000];
			addressMask = 0x7'ffff;
			memConsole = spdlog::stdout_color_mt("BIOS");
		}
} mBios;

void Memory::init() { 
	console->info("Memory init");
}

constexpr Mem* getMemoryRegion(word address) {
	if (MASKED_ADDRESS(address) < 0x1f00'0000) {
		return &mRam;
	} else if (MASKED_ADDRESS(address) < 0x1f80'0000) {
		return &mExp1;
	} else if (MASKED_ADDRESS(address) < 0x1f80'1000) {
		return &mScratchpad;
	} else if (MASKED_ADDRESS(address) < 0x1f80'2000) {
		return &mIo;
	} else if (MASKED_ADDRESS(address) < 0x1fa0'0000) {
		return &mExp2;
	} else if (MASKED_ADDRESS(address) < 0x1fc0'0000) {
		return &mExpe;
	} else {
		return &mBios;
	}
}

byte Memory::fetchByte(word address) {
	Mem* region = getMemoryRegion(address);
	return region->readByte(address);
}

hword Memory::fetchHalfword(word address) {
	address &= 0xffff'fffe;
	Mem* region = getMemoryRegion(address);
	return region->readHalfword(address);
}

word Memory::fetchWord(word address) {
	address &= 0xffff'fffc;
	Mem* region = getMemoryRegion(address);
	return region->readWord(address);
}



void Memory::storeByte(word address, byte data) {
	if (!CPU::cop[0].sr.flags.isolate_cache) {
		Mem* region = getMemoryRegion(address);
		region->storeByte(address, data);
	}
}

void Memory::storeHalfword(word address, hword data) {
	if (!CPU::cop[0].sr.flags.isolate_cache) {
		address &= 0xffff'fffe;
		Mem* region = getMemoryRegion(address);
		region->storeHalfword(address, data);
	}
}

void Memory::storeWord(word address, word data) {
	if (!CPU::cop[0].sr.flags.isolate_cache) {
		address &= 0xffff'fffc;
		Mem* region = getMemoryRegion(address);
		region->storeWord(address, data);
	}
}



void Memory::loadToRAM(word targetAddress, byte* source, word offset, word size) {
	mRam.load(targetAddress, source, offset, size);
	console->info("Done loading to RAM");
}

void Memory::loadBIOS(byte* source, word size) {
	mBios.load(source, size);
	console->info("Done loading BIOS");
}

void Memory::dumpRAM() {
	FileImport::saveFile("ramDump.txt", mRam.memory, 0x200'000);
}
