# IFJ25 Wren Compiler

A comprehensive compiler implementation for the IFJ25 programming language (subset of Wren) developed as part of the VUT FIT Formal Languages and Compilers (IFJ) course project.

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Team](#team)
- [Technical Architecture](#technical-architecture)
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Development](#development)
- [Error Codes](#error-codes)
- [Example](#example)
- [License](#license)

## 🎯 Project Overview

This project implements a complete compiler for the IFJ25 language, a simplified subset of the Wren programming language. The compiler translates IFJ25 source code into a three-address code (TAC) intermediate representation, applies optimizations, and generates executable code for the IFJ25 interpreter.

**Course:** Formal Languages and Compilers (IFJ)  
**Institution:** Brno University of Technology, Faculty of Information Technology (VUT FIT)  
**Academic Year:** 2024/2025  
**Team ID:** 253038

## 👥 Team

| Name | Student ID | Login | Contribution |
|------|-----------|-------|--------------|
| **Serhij Čepil** | 253038 | x253038 | Team Lead, Lexer, Symtable, Optimizer, TAC Generation |
| **Dmytro Kravchenko** | 273125 | xkravcd00 | Lexer, Code Generation |
| **Veronika Turbaievska** | 273123 | xturbav00 | Lexer, Syntax Analysis, Grammar, Symbol Tables |
| **Mykhailo Tarnavskyi** | 272479 | xtarnam00 | Lexer, Semantic Analysis, AST, Type Checking |

## 🏗️ Technical Architecture

### Compiler Pipeline

The compiler follows a multi-stage pipeline architecture:

```
IFJ25 Source Code
    ↓
[1. Lexical Analysis (Lexer)]
    ↓
    Tokens
    ↓
[2. Syntax Analysis (Parser)]
    ↓
    Abstract Syntax Tree (AST)
    ↓
[3. Semantic Analysis]
    ↓
    Annotated AST + Symbol Tables
    ↓
[4. TAC Generation]
    ↓
    Three-Address Code (TAC)
    ↓
[5. Optimization]
    ↓
    Optimized TAC
    ↓
[6. Code Generation]
    ↓
    IFJcode25 Output
```

### Project Structure

```
Wren-Compiler-IFJ-2025/
├── prj/                    # Main project directory
│   ├── src/                # Source files
│   │   ├── main.c          # Compiler entry point
│   │   ├── lexer.c         # Lexical analyzer
│   │   ├── parser.c        # Syntax parser
│   │   ├── semantics.c     # Semantic analyzer
│   │   ├── tac.c           # TAC generation
│   │   ├── optimizer.c     # TAC optimization
│   │   ├── codegen.c       # Code generation
│   │   ├── symtable.c      # Symbol table management
│   │   ├── expression.c    # Expression parsing
│   │   ├── ast.c           # AST node operations
│   │   └── ...
│   ├── include/            # Header files
│   │   ├── lexer.h
│   │   ├── parser.h
│   │   ├── semantics.h
│   │   ├── tac.h
│   │   ├── optimizer.h
│   │   ├── codegen.h
│   │   ├── symtable.h
│   │   ├── error_codes.h
│   │   └── ...
│   ├── Makefile            # Build configuration
│   ├── example.IFJ25       # Sample IFJ25 program
│   ├── ic25int-linux-x86_64 # IFJ25 interpreter
│   └── tests/              # Test suite
├── docs/                   # Documentation
└── README.md               # This file
```

### Key Components

- **Lexer** (`lexer.c/h`): Tokenizes source code, handles keywords, identifiers, literals, and operators
- **Parser** (`parser.c/h`): Builds Abstract Syntax Tree using recursive descent parsing
- **Expression Parser** (`expression.c/h`, `precedence.c/h`): Handles operator precedence for expressions
- **Semantic Analyzer** (`semantics.c/h`): Type checking, variable declaration validation, scope management
- **Symbol Table** (`symtable.c/h`): Manages variable and function definitions across scopes
- **TAC Generator** (`tac.c/h`): Converts AST to three-address code intermediate representation
- **Optimizer** (`optimizer.c/h`): Applies optimization passes on TAC
- **Code Generator** (`codegen.c/h`): Generates final IFJcode25 output from optimized TAC

## ✨ Features

### Lexical Analysis
- Complete tokenization of IFJ25 syntax
- String and numeric literal handling
- Comment processing (single-line and multi-line)
- Comprehensive error detection and reporting

### Syntax and Semantic Analysis
- Class and method declarations
- Static and instance methods
- Getters and setters
- Variable declarations with type inference
- Control flow statements (if/else, while)
- Expression evaluation with proper precedence
- Type checking and compatibility verification
- Scope resolution and validation

### Optimization Techniques
The optimizer implements several advanced optimization passes:

1. **Constant Folding**: Evaluates constant expressions at compile time
   - Arithmetic operations: `2 + 3` → `5`
   - Logical operations: `true && false` → `false`
   - Comparison operations: `5 > 3` → `true`

2. **Constant Propagation**: Replaces variables with known constant values
   ```
   x = 5        x = 5
   y = x + 2    y = 5 + 2  →  y = 7
   z = y * 3    z = 7 * 3  →  z = 21
   ```

3. **Dead Code Elimination**: Removes unreachable or unused code
   - Eliminates unused variable assignments
   - Removes code after unconditional jumps/returns
   - Tracks temporary variable usage

### Code Generation
- Generates IFJcode25 assembly-like instructions
- Proper register allocation and management
- Support for built-in functions from the `ifj25` module
- Stack frame management for function calls

## 📦 Prerequisites

- **Operating System**: Linux (tested on x86_64)
- **Compiler**: GCC with C99 support
- **Build Tool**: GNU Make
- **Optional**: Valgrind (for memory leak detection)

## 🔨 Build Instructions

### Clone the Repository
```bash
git clone https://github.com/Sipxi/Wren-Compiler-IFJ-2025.git
cd Wren-Compiler-IFJ-2025/prj
```

### Build the Compiler
```bash
make
```

This creates two executables:
- `ifj_compiler` - The main compiler
- `test_playground` - Testing executable

### Build Options
```bash
make all              # Build both compiler and test executable
make build            # Build only the compiler
make clean            # Remove all build artifacts
make compile-test     # Build only the test executable
```

## 🚀 Usage

### Compile an IFJ25 Source File

```bash
./ifj_compiler < input.IFJ25 > output.code
```

The compiler reads IFJ25 source code from standard input and writes the generated code to standard output.

### Run Compiled Code

After compilation, execute the generated code using the included IFJ25 interpreter:

```bash
./ic25int-linux-x86_64 output.code
```

If your program requires input:
```bash
./ic25int-linux-x86_64 output.code < input.txt
```

### Quick Test

A convenient make target compiles and runs the example program:

```bash
make run
```

This executes:
1. Compiles `example.IFJ25` to `code.out`
2. Runs `code.out` with `example_input.txt` as input

## 🛠️ Development

### Running Tests

```bash
make test-pg           # Run test executable
make test-pg-valgrind  # Run tests with memory leak detection
```

### Memory Leak Testing

Use Valgrind to check for memory leaks:

```bash
make run-valgrind      # Run compiler with Valgrind
```

Or manually:
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  ./ifj_compiler < example.IFJ25
```

### Debugging

The compiler is built with debug symbols (`-g` flag). Use GDB for debugging:

```bash
gdb ./ifj_compiler
(gdb) run < example.IFJ25
```

## ⚠️ Error Codes

The compiler uses the following error codes as specified by the IFJ project requirements:

| Code | Description |
|------|-------------|
| 1 | **Lexical Error**: Invalid token or character sequence |
| 2 | **Syntax Error**: Invalid program structure |
| 3 | **Semantic Error - Undefined**: Reference to undefined variable or function |
| 4 | **Semantic Error - Redeclaration**: Variable or function redeclared |
| 5 | **Semantic Error - Arguments**: Wrong number or type of function arguments |
| 6 | **Semantic Error - Type Mismatch**: Incompatible types in operation or assignment |
| 10 | **Semantic Error - Other**: Other semantic errors |
| 99 | **Internal Error**: Compiler internal error (memory allocation failure, etc.) |

Runtime errors (codes 25-26) are detected by the interpreter, not the compiler.

## 📝 Example

### Sample IFJ25 Program

```wren
// example.IFJ25
import "ifj25" for Ifj

class Program {
    static main() {
        value = 42
        __a = fun(value, value)
    }
    
    static value {
        if (__stored) {
            return __stored
        } else {
            return 0
        }
    }
    
    static value=(v) {
        __stored = v
    }

    static fun(a, b) {
        a = Ifj.write(a)
        b = Ifj.write(b)
    }
}
```

### Compilation and Execution

```bash
# Compile the program
./ifj_compiler < example.IFJ25 > code.out

# Run the compiled code
./ic25int-linux-x86_64 code.out < example_input.txt

# Or use the convenient shortcut
make run
```

### Expected Output
```
42
42
```

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

**Note**: This project was developed as part of the IFJ course at VUT FIT. It is intended for educational purposes.


