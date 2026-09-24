CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -Iemulator -Isocket -pthread
AR ?= ar

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib

DRIVER_SRCS = driver/dhfdrv.c
EMU_SRCS = emulator/dhf_host_fs.c emulator/dhf_emu_device.c socket/dhf_socket_client.c socket/dhf_socket_server.c
DAEMON_SRCS = socket/dhf_net_daemon.c emulator/dhf_host_fs.c socket/dhf_socket_server.c

DRIVER_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(DRIVER_SRCS))
EMU_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(EMU_SRCS))
DAEMON_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(DAEMON_SRCS))

TARGET_DRIVER_LIB = $(LIB_DIR)/libdhfdrv.a
TARGET_EMU_LIB = $(LIB_DIR)/libdhf_emu.a
TARGET_DAEMON = $(BIN_DIR)/dhf-net-daemon
TARGET_TEST_LOCAL = $(BIN_DIR)/test_dhfdrv
TARGET_TEST_SOCKET = $(BIN_DIR)/test_dhf_socket

all: $(TARGET_DRIVER_LIB) $(TARGET_EMU_LIB) $(TARGET_DAEMON) $(TARGET_TEST_LOCAL) $(TARGET_TEST_SOCKET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_DRIVER_LIB): $(DRIVER_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(TARGET_EMU_LIB): $(EMU_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(TARGET_DAEMON): $(DAEMON_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_TEST_LOCAL): tests/test_dhfdrv.c $(TARGET_DRIVER_LIB) $(TARGET_EMU_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -ldhfdrv -ldhf_emu -o $@

$(TARGET_TEST_SOCKET): tests/test_dhf_socket.c $(TARGET_DRIVER_LIB) $(TARGET_EMU_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -ldhfdrv -ldhf_emu -o $@

test: $(TARGET_TEST_LOCAL) $(TARGET_TEST_SOCKET)
	@echo "=== Running Local Driver and Emulator Test ==="
	@$(TARGET_TEST_LOCAL)
	@echo "=== Running Socket Forwarding Test ==="
	@$(TARGET_TEST_SOCKET)
	@echo "=== All Tests Passed! ==="

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test clean
