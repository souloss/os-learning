# ============================================================
# Common Makefile for OS tutorial chapters
# Supports NASM + GCC (i386) + QEMU + Multi-module builds
# ============================================================

# ------------------------------------------------------------
# 工具链定义（可通过环境变量覆盖）
# ------------------------------------------------------------
NASM     ?= nasm
CC       ?= i386-elf-gcc
LD       ?= i386-elf-ld
QEMU_IMG ?= qemu-img
QEMU     ?= qemu-system-i386
DD       ?= dd
HEXDUMP  ?= hexdump
OBJCOPY  ?= i386-elf-objcopy

# ------------------------------------------------------------
# 通用参数配置
# ------------------------------------------------------------
BS           ?= 512
IMG_SIZE_MB  ?= 10
IMG_FORMAT   ?= vpc
BUILD_DIR    ?= ../build/$(CHAPTER)
IMG_FILE     ?= $(BUILD_DIR)/disk.vhd

CFLAGS       ?= -m32 -ffreestanding -nostdlib -nostartfiles -Wall -fno-builtin -fno-stack-protector -no-pie -fno-pic
LDFILE       ?= link.ld
NASMFLAGS    ?= -f elf32
LDFLAGS      ?= -m elf_i386

RUN_QEMU_OPTS ?= -nographic
VNC_DISPLAY    ?= 1
VNC_PORT       := $(shell echo $$((5900 + $(VNC_DISPLAY))))
QEMU_PID_FILE  := $(BUILD_DIR)/.qemu.pid

# ------------------------------------------------------------
# 保证 build 目录存在
# ------------------------------------------------------------
$(shell mkdir -p $(BUILD_DIR))

# ------------------------------------------------------------
# 自动发现
# ------------------------------------------------------------
# 查找指定目录下的所有.c文件
define find_c_files
$(shell find $(1) -name "*.c" -type f)
endef

# 查找指定目录下的所有.S文件
define find_asm_files
$(shell find $(1) -name "*.S" -type f)
endef

# 判断是否是文件
define is_dir
$(if $(wildcard $(1)/.),yes,no)
endef

# 查找指定目录下所有包含头文件的目录
define find_include_dirs
$(shell find $(1) -name "*.h" -type f -exec dirname {} \; | sort -u)
endef

# 将目录列表转换为 -I 参数
define dirs_to_includes
$(foreach dir,$(1),-I$(dir))
endef

# ------------------------------------------------------------
# 模块构建规则
# ------------------------------------------------------------
# 为每个模块生成目标文件
define create_module_rules
# ---- 动态判断 ----
$(if $(filter yes,$(call is_dir,$($(1)_SRC))), \
  $(call create_dir_module_rules,$(1)), \
  $(call create_single_module_rules,$(1)) \
)
endef

define create_dir_module_rules
# 多文件模块
$(1)_C_SRCS := $$(call find_c_files,$($(1)_SRC))
$(1)_ASM_SRCS := $$(call find_asm_files,$($(1)_SRC))

# ✅ 保留原路径并在文件名中加上扩展名，防止 .c/.S 同名冲突
$(1)_OBJS := $$(patsubst %.c,$$(BUILD_DIR)/%.c.o,$$($(1)_C_SRCS))
$(1)_OBJS += $$(patsubst %.S,$$(BUILD_DIR)/%.S.o,$$($(1)_ASM_SRCS))

$(1)_INCLUDE_DIRS := $$(call find_include_dirs,$($(1)_SRC))
$(1)_INCLUDES := $$(call dirs_to_includes,$$($(1)_INCLUDE_DIRS))

# ✅ C 文件编译规则
$$(BUILD_DIR)/%.c.o: %.c
	@echo "  [CC] $$< -> $$@"
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) $$($(1)_INCLUDES) -c $$< -o $$@

# ✅ 汇编文件编译规则
$$(BUILD_DIR)/%.S.o: %.S
	@echo "  [NASM] $$< -> $$@"
	@mkdir -p $$(dir $$@)
	$$(NASM) $$(NASMFLAGS) $$< -o $$@

# ✅ 链接规则
$$(BUILD_DIR)/$(1).bin: $$($(1)_OBJS) $$($(1)_LDFILE)
	@echo "  [LD] linking $$($(1)_OBJS) -> $$@"
	$$(LD) $$(LDFLAGS) -T $$($(1)_LDFILE) $$($(1)_OBJS) -o $$@
endef

define create_single_module_rules
# 单文件模块
ifeq ($(suffix $($(1)_SRC)),.S)
$$(BUILD_DIR)/$(1).bin: $($(1)_SRC)
	@echo "  [NASM] $$< -> $$@"
	@mkdir -p $$(dir $$@)
	$$(NASM) -f bin -o $$@ $$<
else ifeq ($(suffix $($(1)_SRC)),.c)
$$(BUILD_DIR)/$(1).bin: $($(1)_SRC) $$($(1)_LDFILE)
	@echo "  [CC] $$< -> $$(BUILD_DIR)/$(1).o"
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -c $$< -o $$(BUILD_DIR)/$(1).o
	@echo "  [LD] linking $$(BUILD_DIR)/$(1).o -> $$@"
	$$(LD) $$(LDFLAGS) -T $$($(1)_LDFILE) $$(BUILD_DIR)/$(1).o -o $$@
endif
endef
# ------------------------------------------------------------
# Step 1: 模块自动推导
# MODULES = mbr loader kernel
# 每个模块定义 <mod>_SRC, <mod>_OFFSET
# ------------------------------------------------------------
MODULE_BINS := $(foreach m,$(MODULES),$(BUILD_DIR)/$(m).bin)

#$(BUILD_DIR)/%.bin: $($(*)_SRC)
#	@src=$($(*)_SRC); \
#	echo "[BUILD] target: $*  src: $$src"; \
#	case "$$src" in \
#		*.S) \
#			echo "  [NASM] assembling $$src -> $@"; \
#			$(NASM) -f bin -o $@ $$src ;; \
#		*.c) \
#			echo "  [CC] compiling $$src -> $(BUILD_DIR)/$*.o"; \
#			$(CC) $(CFLAGS) -c $$src -o $(BUILD_DIR)/$*.o; \
#			echo "  [LD] linking $(BUILD_DIR)/$*.o -> $@"; \
#			$(LD) $(LDFLAGS) $(BUILD_DIR)/$*.o -o $@ ;; \
#		*) \
#			echo "Error: Unknown source type for $$src"; \
#			exit 1 ;; \
#	esac

# ------------------------------------------------------------
# Step 2: 生成虚拟磁盘镜像并写入模块
# ------------------------------------------------------------
.PHONY: all image clean run view run-vnc stop info help
all: image

image: $(IMG_FILE)

$(IMG_FILE): $(MODULE_BINS)
	@echo "[qemu-img] create fixed-size $(IMG_SIZE_MB)MB image: $@"
	$(QEMU_IMG) create -f $(IMG_FORMAT) -o subformat=fixed $@ $(IMG_SIZE_MB)M
	@echo "[dd] write modules into image:"
	@$(foreach m,$(MODULES), \
		echo "  -> Writing $m ($($m_SRC)) to offset sector $($m_OFFSET)"; \
		$(DD) if=$(BUILD_DIR)/$(m).bin of=$@ bs=$(BS) seek=$($m_OFFSET) conv=notrunc; \
	)

# ------------------------------------------------------------
# Step 3: 运行 / 调试 / 工具目标
# ------------------------------------------------------------
run: $(IMG_FILE)
	@echo "[QEMU] Running..."
	$(QEMU) -drive file=$(IMG_FILE),format=$(IMG_FORMAT) $(RUN_QEMU_OPTS)

run-vnc: $(IMG_FILE)
	@echo "[QEMU] Running in background with VNC :$(VNC_DISPLAY)"
	$(QEMU) \
		-drive file=$(IMG_FILE),format=$(IMG_FORMAT) \
		-vnc :$(VNC_DISPLAY) -m 32M -pidfile $(QEMU_PID_FILE) -daemonize
	@echo "Connect via VNC: localhost:$(VNC_PORT)"

run-vnc.monitor: $(IMG_FILE)
	@echo "[QEMU] Running in background with VNC :$(VNC_DISPLAY)"
	$(QEMU) \
		-drive file=$(IMG_FILE),format=$(IMG_FORMAT) \
		-vnc :$(VNC_DISPLAY) -m 32M -pidfile $(QEMU_PID_FILE) -monitor stdio
	@echo "Connect via VNC: localhost:$(VNC_PORT)"

run-vnc.debug: $(IMG_FILE)
	@echo "[QEMU] Running in background with VNC :$(VNC_DISPLAY)"
	$(QEMU) \
		-drive file=$(IMG_FILE),format=$(IMG_FORMAT) \
		-vnc :$(VNC_DISPLAY) -m 32M -pidfile $(QEMU_PID_FILE) -S -s
	@echo "Connect via VNC: localhost:$(VNC_PORT)"

stop:
	@if [ -f $(QEMU_PID_FILE) ]; then \
		PID=$$(cat $(QEMU_PID_FILE)); \
		echo "Stopping QEMU (PID $$PID)..."; \
		kill $$PID; \
		rm -f $(QEMU_PID_FILE); \
	else \
		echo "No QEMU PID file found, is QEMU running?"; \
	fi

view:
	$(HEXDUMP) -C -n 1024 $(IMG_FILE)

info:
	@echo ">>> Module info:"
	@$(foreach m,$(MODULES), \
		printf "%-10s offset=%-4s src=%s\n" "$(m)" "$($m_OFFSET)" "$($m_SRC)"; \
	)

clean: stop
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all, image    Build image (default)"
	@echo "  run           Run image in QEMU (nographic)"
	@echo "  run-vnc       Run QEMU in background with VNC"
	@echo "  stop          Stop background QEMU"
	@echo "  view          Hexdump start of image"
	@echo "  info          Show module build info"
	@echo "  clean         Remove build outputs"
	@echo ""
	@echo "Variables (can override):"
	@echo "  IMG_SIZE_MB     Image size in MB (default $(IMG_SIZE_MB))"
	@echo "  RUN_QEMU_OPTS   Extra options for QEMU"
	@echo "  VNC_DISPLAY     VNC display number (:$(VNC_DISPLAY))"

$(foreach m,$(MODULES),$(eval $(call create_module_rules,$(m))))