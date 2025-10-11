# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g
INCLUDES = -Iinclude

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Source files (excluding main.c, test.c, test_runner.c - they have their own main functions)
LIB_SOURCES = $(filter-out $(SRCDIR)/main.c $(SRCDIR)/test.c $(SRCDIR)/test_lexer.c, $(wildcard $(SRCDIR)/*.c))
LIB_OBJECTS = $(LIB_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Main program files
MAIN_SOURCE = $(SRCDIR)/main.c
MAIN_OBJECT = $(OBJDIR)/main.o

# Test program files
TEST_SOURCE = $(SRCDIR)/test.c
TEST_OBJECT = $(OBJDIR)/test.o

# Test runner files
TEST_RUNNER_SOURCE = $(SRCDIR)/test_lexer.c
TEST_RUNNER_OBJECT = $(OBJDIR)/test_lexer.o

# Target executables
MAIN_TARGET = ifj_compiler
TEST_TARGET = test_playground
TEST_RUNNER_TARGET = test_lexer

# Default target - compile everything
all: $(MAIN_TARGET) $(TEST_TARGET)

# Create main executable
$(MAIN_TARGET): $(LIB_OBJECTS) $(MAIN_OBJECT)
	$(CC) $^ -o $@

# Create test executable
$(TEST_TARGET): $(LIB_OBJECTS) $(TEST_OBJECT)
	$(CC) $^ -o $@

# Create test runner executable
$(TEST_RUNNER_TARGET): $(LIB_OBJECTS) $(TEST_RUNNER_OBJECT)
	$(CC) $^ -o $@

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean build files
clean:
	rm -rf $(OBJDIR) $(MAIN_TARGET) $(TEST_TARGET) $(TEST_RUNNER_TARGET) tests/actuals/

# Run the main program
run: $(MAIN_TARGET)
	./$(MAIN_TARGET)

# Run the test playground with example.wren
test-pg: $(TEST_TARGET)
	./$(TEST_TARGET) < example.wren

# Run lexer tests
test-lexer: $(TEST_RUNNER_TARGET)
	./$(TEST_RUNNER_TARGET)

# Run automated tests
test: $(TEST_TARGET)
	./tests/run_tests.sh

# Clean only test outputs
clean-tests:
	rm -rf tests/actuals/

# Build test runner only
build-test: $(TEST_RUNNER_TARGET)

# Phony targets
.PHONY: all clean run test-pg test-lexer test build-test clean-tests