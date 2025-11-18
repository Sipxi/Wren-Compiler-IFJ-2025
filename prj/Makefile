# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g -Wno-unused-function
INCLUDES = -Iinclude

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Source files (excluding main.c and test.c - they have their own main functions)
LIB_SOURCES = $(filter-out $(SRCDIR)/main.c $(SRCDIR)/test.c, $(wildcard $(SRCDIR)/*.c))
LIB_OBJECTS = $(LIB_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Main program files
MAIN_SOURCE = $(SRCDIR)/main.c
MAIN_OBJECT = $(OBJDIR)/main.o

# Test program files
TEST_SOURCE = $(SRCDIR)/test.c
TEST_OBJECT = $(OBJDIR)/test.o

# Target executables
MAIN_TARGET = ifj_compiler
TEST_TARGET = test_playground

# Default target - compile everything
all: $(MAIN_TARGET) $(TEST_TARGET)

# Create main executable
$(MAIN_TARGET): $(LIB_OBJECTS) $(MAIN_OBJECT)
	$(CC) $^ -o $@

# Create test executable
$(TEST_TARGET): $(LIB_OBJECTS) $(TEST_OBJECT)
	$(CC) $^ -o $@

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean build files
clean:
	rm -rf $(OBJDIR) $(MAIN_TARGET) $(TEST_TARGET)

# Run the main program
run: $(MAIN_TARGET)
	@./$(MAIN_TARGET) < example.IFJ25 > code.out

run-valgrind: $(MAIN_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(MAIN_TARGET) < example.IFJ25

# Run the test playground
test-pg: $(TEST_TARGET)
	./$(TEST_TARGET)

# Run the test playground with valgrind
test-pg-valgrind: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_TARGET)

# Build the compiler binary
build: $(LIB_OBJECTS) $(MAIN_OBJECT)
	$(CC) $^ -o ifj_compiler


# Phony targets
.PHONY: all clean run test-pg test-pg-valgrind