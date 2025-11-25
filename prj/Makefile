# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g -Wno-unused-function
INCLUDES = -Iinclude

# Directories
SRCDIR = src
OBJDIR = obj

# Detect all .c files
ALL_SOURCES = $(wildcard $(SRCDIR)/*.c)

# Exclude these (each has its own main)
MAIN_SOURCE = $(SRCDIR)/main.c
TEST_SOURCE = $(SRCDIR)/test.c

LIB_SOURCES = $(filter-out $(MAIN_SOURCE) $(TEST_SOURCE), $(ALL_SOURCES))

# Object files
LIB_OBJECTS  = $(LIB_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
MAIN_OBJECT  = $(MAIN_SOURCE:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TEST_OBJECT  = $(TEST_SOURCE:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Executables
MAIN_TARGET = ifj_compiler
TEST_TARGET = test_playground

# Default target
all: $(MAIN_TARGET) $(TEST_TARGET)

# Main binary
$(MAIN_TARGET): $(LIB_OBJECTS) $(MAIN_OBJECT)
	$(CC) $^ -o $@

# Test binary
$(TEST_TARGET): $(LIB_OBJECTS) $(TEST_OBJECT)
	$(CC) $^ -o $@

# Generic rule for object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create object dir
$(OBJDIR):
	mkdir -p $(OBJDIR)

# --- Additional target you wanted ---
compile-test: $(LIB_OBJECTS) $(TEST_OBJECT)
	$(CC) $(LIB_OBJECTS) $(TEST_OBJECT) -o test_playground

# Clean
clean:
	rm -rf $(OBJDIR) $(MAIN_TARGET) $(TEST_TARGET) test_playground

# Run commands
run: $(MAIN_TARGET)
	@./$(MAIN_TARGET) < example.IFJ25 > code.out && ./inter code.out

run-valgrind: $(MAIN_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(MAIN_TARGET) < example.IFJ25

test-pg: $(TEST_TARGET)
	./$(TEST_TARGET)

test-pg-valgrind: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_TARGET)

# Rebuild compiler
build: $(MAIN_TARGET)

.PHONY: all clean run test-pg test-pg-valgrind build compile-test
