CC          := ppu-lv2-gcc
CXX         := ppu-lv2-g++
LD          := ppu-lv2-ld 
OBJCOPY     := ppu-lv2-objcopy
OBJDUMP     := ppu-lv2-objdump
AR          := ppu-lv2-ar
STRIP       := ppu-lv2-strip

TARGET      = $(shell basename $(CURDIR))

INCLUDE     = 

LIBSDIR     = $(SCE_PS3_ROOT)/target/ppu/lib
LIBS        =
#-lgcm_cmd -lgcm_sys_stub -lsysmodule_stub -lresc_stub -lsysutil_stub -lcgb -lcgc


CFLAGS = -m64 -fno-builtin -fno-exceptions -Wall -g \
         $(foreach dir,$(INCLUDE),-I$(dir))
CFLAGS += -Os

LDFLAGS = -mbig-endian -g -Os -Wall \
          -L. $(foreach dir,$(LIBSDIR),-L$(dir)) $(LIBS)

OBJS = ppumain.o

all: $(TARGET).self asm

asm: $(TARGET).lst

%.lst: %.elf
	$(OBJDUMP) -S -D $< > $@

$(TARGET).self: $(TARGET).elf
	make_fself $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.S Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f *.o *.elf *.self *.lst