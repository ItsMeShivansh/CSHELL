# Compiler and flags based on project requirements
CC=gcc
CFLAGS=-std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-gnu-keywords -fno-asm

# Directories for source, include, and build files
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Automatically find all .c files in the source directory
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Generate corresponding .o file paths in the build directory
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# The final executable file name
TARGET = shell.out

# Phony targets are not files; 'all' and 'clean' are common examples
.PHONY: all clean

# The default goal: build the final executable
all: $(TARGET)

# Rule to link all object files into the final executable
# '$^' is a special variable that means "all prerequisites" (all the .o files)
# '$@' is a special variable that means "the target" (shell.out)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile a single .c file into a .o file
# '$<' is a special variable that means "the first prerequisite" (the .c file)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<

# Rule to clean up the build directory and the executable
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
