# cFep — GNU Make，Windows(MinGW) / Linux / macOS
# 用法：在仓库根目录执行  make  或  make clean
# Windows 建议使用 MSYS2 / Git Bash 自带的 make + gcc，以便支持 mkdir -p

MAKEFILE_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
SRCDIR       := $(MAKEFILE_DIR)src
BUILD_DIR    := $(MAKEFILE_DIR)build

# ---------- 平台 ----------
ifeq ($(OS),Windows_NT)
  PLATFORM   := windows
  EXEEXT     := .exe
  # socket.c 使用 __WIN32 分支；MinGW 通常只预定义 _WIN32
  CPPFLAGS   += -D__WIN32
  LDLIBS     += -lws2_32
  WIN_STUB   := win_wupdate_stub.c
else
  UNAME_S    := $(shell uname -s 2>/dev/null || echo Unknown)
  ifeq ($(UNAME_S),Darwin)
    PLATFORM := darwin
  else
    PLATFORM := linux
  endif
  EXEEXT     :=
  LDLIBS     += -pthread
  WIN_STUB   :=
endif

TARGET   := $(MAKEFILE_DIR)cFep$(EXEEXT)
OBJDIR   := $(BUILD_DIR)/$(PLATFORM)

# VPATH：仅用「文件名」编译，对象统一进 $(OBJDIR)
VPATH := $(SRCDIR):$(SRCDIR)/lib:$(SRCDIR)/lib/zip

SRCS := \
	cFep.c \
	dictionary.c \
	ini.c \
	iniparser.c \
	lib.c \
	listLib.c \
	log.c \
	ptcl_62056-47.c \
	ptcl_698.c \
	ptcl_gw.c \
	ptcl_jl.c \
	ptcl_nw.c \
	ptcl_zj.c \
	socket.c \
	taskLib.c \
	ttynet.c \
	port.c \
	CrypFun.c \
	cceman.c \
	compressfun.c \
	compressfunnew.c \
	$(WIN_STUB)

OBJS := $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS := $(OBJS:.o=.d)

# Make 内置 CC=cc；多数环境需显式 gcc（尤其 Windows MinGW）
ifeq ($(origin CC),default)
  CC := gcc
endif
CFLAGS  ?= -O2 -Wall -Wextra -Werror -std=gnu11 -fsigned-char -ffunction-sections
CPPFLAGS += -MMD -MP -I$(SRCDIR) -I$(SRCDIR)/lib/zip
# macOS 默认链接器对 --gc-sections 支持方式不同，此处省略
ifeq ($(PLATFORM),darwin)
  LDFLAGS ?=
else
  LDFLAGS ?= -Wl,--gc-sections
endif

.PHONY: all clean dirs

all: $(TARGET)

$(TARGET): $(OBJS) | $(OBJDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo "Built: $@"

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# compressfun*.c 为历史实现，length 与循环变量混用 int/size_t，单独放宽 sign-compare
$(OBJDIR)/compressfun.o: compressfun.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-sign-compare -c -o $@ $<

$(OBJDIR)/compressfunnew.o: compressfunnew.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-sign-compare -c -o $@ $<

$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	$(RM) -rf $(BUILD_DIR) "$(TARGET)" "$(MAKEFILE_DIR)cFep" "$(MAKEFILE_DIR)cFep.exe"

-include $(DEPS)
