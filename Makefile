K=kernel
U=user

# .o文件
OBJS = \
	$K/entry.o \

# RISC-V ubuntu下的交叉编译工具
RISC-COMPLIER = riscv64-unknown-elf-

# QEMU相关设置
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=file_system.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

# 编译、汇编、链接指令简写
CC = $(RISC-COMPLIER)gcc
AS = $(RISC-COMPLIER)gas
LD = $(RISC-COMPLIER)ld

LDFLAGS = -z max-page-size = 4096

# 定义kernel二进制文件的依赖关系
$K/kernel: $(OBJS) $/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS)

$U/initcode: $U/initcode.S
	$(CC)

file_system.img: mkfs/mkfs 

qemu: $K/kernel file_system.img
	qemu-system-riscv64 $(QEMUOPTS)