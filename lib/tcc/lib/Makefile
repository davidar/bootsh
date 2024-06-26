#
# Tiny C Compiler Makefile for libtcc1.a
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(TOPSRC)/lib $(TOPSRC)/win32/lib
T = $(or $(CROSS_TARGET),$(NATIVE_TARGET),unknown)
X = $(if $(CROSS_TARGET),$(CROSS_TARGET)-)

XTCC ?= $(TOP)/$(X)tcc$(EXESUF)
XCC = $(XTCC)
# XAR = $(XTCC) -ar
XAR = $(AR)
XFLAGS-unx = -B$(TOPSRC)
XFLAGS-win = -B$(TOPSRC)/win32 -I$(TOPSRC)/include
XFLAGS = $(XFLAGS$(XCFG)) -I$(TOP) -D__SOFT_FP__
BFLAGS = -bt
XCFG = $(or $(findstring -win,$T),-unx)
S = $(if $(findstring yes,$(SILENT)),@$(info * $@))

# in order to use gcc, type: make <target>-libtcc1-usegcc=yes
arm-libtcc1-usegcc ?= no

# This makes bounds checking 40%..60% faster.
x86_64-libtcc1-usegcc=yes
# i386-libtcc1-usegcc=yes

ifeq "$($(T)-libtcc1-usegcc)" "yes"
 XCC = $(CC)
 XAR = $(AR)
 XFLAGS = $(CFLAGS) -fPIC -fno-omit-frame-pointer -Wno-unused-function -Wno-unused-variable
 BFLAGS = -g
endif

ifneq ($(CONFIG_backtrace),no)
# only for native compiler
ifneq ($(CONFIG_bcheck),no)
$(X)BCHECK_O = bcheck.o
endif
$(X)BT_O = bt-exe.o bt-log.o
$(X)B_O = $(BCHECK_O) bt-exe.o bt-log.o bt-dll.o
endif
# $(X)BT_O += runmain.o tcov.o

DSO_O = dsohandle.o

RT_C := $(wildcard rt/*.c)
RT_O := $(patsubst rt/%.c,rt/%.o,$(RT_C))

I386_O = $(RT_O) syscall-i386.o alloca.o alloca-bt.o stdatomic.o atomic.o builtin.o $(BT_O)
X86_64_O = $(RT_O) syscall-x86_64.o lib-x86_64.o alloca.o alloca-bt.o stdatomic.o atomic.o builtin.o $(BT_O)
ARM_O = $(RT_O) armeabi.o alloca.o armflush.o stdatomic.o atomic.o builtin.o $(BT_O)
ARM64_O = lib-arm64.o stdatomic.o atomic.o builtin.o $(BT_O)
RISCV64_O = lib-arm64.o stdatomic.o atomic.o builtin.o $(BT_O)
WIN_O = crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o

OBJ-i386 = $(I386_O) $(BCHECK_O) $(DSO_O)
OBJ-x86_64 = $(X86_64_O) va_list.o $(BCHECK_O) $(DSO_O)
OBJ-x86_64-osx = $(X86_64_O) va_list.o $(BCHECK_O)
OBJ-i386-win32 = $(I386_O) chkstk.o $(B_O) $(WIN_O)
OBJ-x86_64-win32 = $(X86_64_O) chkstk.o $(B_O) $(WIN_O)
OBJ-arm64 = $(ARM64_O) $(BCHECK_O) $(DSO_O)
OBJ-arm64-osx = $(ARM64_O) $(BCHECK_O)
OBJ-arm = $(ARM_O) $(BCHECK_O) $(DSO_O)
OBJ-arm-fpa = $(ARM_O) $(DSO_O)
OBJ-arm-fpa-ld = $(ARM_O) $(DSO_O)
OBJ-arm-vfp = $(ARM_O) $(DSO_O)
OBJ-arm-eabi = $(ARM_O) $(DSO_O)
OBJ-arm-eabihf = $(ARM_O) $(DSO_O)
OBJ-arm-wince = $(ARM_O) $(WIN_O)
OBJ-riscv64 = $(RISCV64_O) $(BCHECK_O) $(DSO_O)

OBJ-extra = $(filter $(B_O) runmain.o,$(OBJ-$T))
OBJ-libtcc1 = $(addprefix $(X),$(filter-out $(OBJ-extra),$(OBJ-$T)))

ALL = $(addprefix $(TOP)/,$(X)libtcc1.a $(OBJ-extra))

all: $(ALL)

$(TOP)/$(X)libtcc1.a : $(OBJ-libtcc1)
	$S$(XAR) rcs $@ $^

$(X)%.o : %.c
	$S$(XCC) -c $< -o $@ $(XFLAGS)

$(X)%.o : %.S
	$S$(XCC) -c $< -o $@ $(XFLAGS)

$(TOP)/%.o : %.c
	$S$(XCC) -c $< -o $@ $(XFLAGS)

$(TOP)/bcheck.o : XFLAGS += $(BFLAGS) $(if $(CONFIG_musl),-DTCC_MUSL)
$(TOP)/bt-exe.o : $(TOP)/tccrun.c

$(X)crt1w.o : crt1.c
$(X)wincrt1w.o : wincrt1.c

clean :
	rm -f *.a *.o rt/*.o $(ALL)
