OBJS = \
  $K/entry.o \
  $K/kalloc.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o

OBJS_KCSAN = \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/spinlock.o

ifdef KCSAN
OBJS_KCSAN += \
	$K/kcsan.o
endif

ifeq ($(LAB),lock)
OBJS += \
	$K/stats.o\
	$K/sprintf.o
endif


ifeq ($(LAB),net)
OBJS += \
	$K/e1000.o \
	$K/net.o \
	$K/pci.o
endif



$K/kernel: $(OBJS) $(OBJS_KCSAN) $K/kernel.ld $U/initcode
	@$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(OBJS_KCSAN)
	@echo $(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel -OBJS -OBJS_KCSAN
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$(OBJS): EXTRAFLAG := $(KCSANFLAG)
$K/%.o: $K/%.c
	@$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<
	@echo $(CC) -CFLAGS -EXTRAFLAG -c -o $@ $<

$U/initcode: $U/initcode.S
	@$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	@echo $(CC) -CFLAGS -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm



