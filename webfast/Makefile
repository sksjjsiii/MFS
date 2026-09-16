# WebFast Compiler Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include -O2
LDFLAGS = -lpthread

# Source files
SRC_DIR = src
RUNTIME_DIR = runtime
INCLUDE_DIR = include

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/codegen.c
OBJECTS = $(SOURCES:.c=.o)

# Runtime
RUNTIME_SRC = $(RUNTIME_DIR)/webfast_runtime.c
RUNTIME_OBJ = $(RUNTIME_DIR)/webfast_runtime.o

# Compiler executable
COMPILER = wf

# Targets
.PHONY: all clean install examples help

all: $(COMPILER)
	@echo ""
	@echo "╔════════════════════════════════════════╗"
	@echo "║  WebFast Compiler built successfully!║"
	@echo "╚════════════════════════════════════════╝"
	@echo ""
	@echo "Usage: ./$(COMPILER) <input.wf> -o <output.c>"
	@echo ""

$(COMPILER): $(OBJECTS)
	@echo "Linking compiler..."
	$(CC) $(OBJECTS) -o $(COMPILER) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/webfast.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

runtime: $(RUNTIME_OBJ)
	@echo "Runtime library built"

$(RUNTIME_DIR)/%.o: $(RUNTIME_DIR)/%.c $(INCLUDE_DIR)/webfast_runtime.h
	@echo "Compiling runtime..."
	$(CC) $(CFLAGS) -c $< -o $@

examples: $(COMPILER)
	@echo "Compiling examples..."
	./$(COMPILER) examples/blog.wf -o examples/blog.c
	@echo "Compiling blog application..."
	$(CC) examples/blog.c $(RUNTIME_SRC) -o examples/blog $(LDFLAGS)
	@echo "Example compiled: ./examples/blog"

clean:
	@echo "Cleaning..."
	rm -f $(OBJECTS) $(RUNTIME_OBJ) $(COMPILER)
	rm -f examples/*.c examples/blog
	@echo "Clean complete"

install: $(COMPILER)
	@echo "Installing WebFast compiler..."
	sudo cp $(COMPILER) /usr/local/bin/
	sudo mkdir -p /usr/local/include/webfast
	sudo cp $(INCLUDE_DIR)/*.h /usr/local/include/webfast/
	sudo cp $(RUNTIME_SRC) /usr/local/lib/
	@echo "Installation complete"

help:
	@echo "WebFast Compiler Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the compiler (default)"
	@echo "  runtime   - Build runtime library only"
	@echo "  examples  - Compile example applications"
	@echo "  clean     - Remove all build artifacts"
	@echo "  install   - Install compiler to system"
	@echo "  help      - Show this help message"
	@echo ""
