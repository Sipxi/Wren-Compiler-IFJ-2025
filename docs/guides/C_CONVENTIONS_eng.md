# C Coding Style Guide

This document defines the coding style for our project. 
Following these conventions ensures that our codebase stays **readable, consistent, and maintainable**.

---

## 🔤 Naming Conventions

* **Variables**: `snake_case`

  ```c
  int buffer_size;
  char user_name[50];
  ```
* **Functions**: `snake_case`, verb + noun form

  ```c
  void print_token();
  int parse_expression();
  ```
* **Constants & Macros**: `ALL_CAPS` with underscores

  ```c
  #define MAX_BUFFER 1024
  const int DEFAULT_PORT = 8080;
  ```
* **Types (struct, enum, typedef)**: `PascalCase` or `_t` suffix

  ```c
  typedef struct Token {
      int type;
      char *value;
  } Token;

  enum LogLevel { LOG_INFO, LOG_WARNING, LOG_ERROR };
  ```
* **Global Variables**: prefix with `g_`

  ```c
  int g_token_count;
  ```

---

## 📏 Formatting

* **Indentation**: 4 spaces, no tabs.
* **Line length**: max 100 characters.
* **Braces**: K\&R style

  ```c
  if (x > 0) {
      do_something();
  } else {
      do_other();
  }
  ```
* **Spacing**:

  * Space after keywords: `if (x > 0)`
  * Space around operators: `a + b`, not `a+b`
  * Pointers: star sticks to variable, not type

    ```c
    int *ptr;   // correct
    int* ptr;   // avoid
    ```

---

## 🗂️ Files & Modules

* One `.c` + `.h` pair per module.
* File names: `snake_case.c`, `snake_case.h`
* **Header guards** (or `#pragma once`):

  ```c
  #ifndef LEXER_H
  #define LEXER_H

  void tokenize(const char *source);

  #endif // LEXER_H
  ```
* **Include order**:

  1. Own header
  2. System headers
  3. Third-party headers

  ```c
  #include "lexer.h"
  #include <stdio.h>
  #include <stdlib.h>
  ```

---

## 📚 Functions & Code Practices

* **One purpose per function** (keep <50 lines).
* **Early returns** over deep nesting:

  ```c
  if (!user) return;
  ```
* Avoid **magic numbers** → use constants/macros.
* **Error handling**:

  * Return codes (`0` = success, `-1` = error)
  * Use enums for detailed errors

---

## 📝 Comments & Documentation

* `//` for inline comments, `/* */` for block.
* Document each public function:

  ```c
  /**
   * Parses an expression from tokens.
   * @param tokens List of tokens.
   * @return AST node if success, NULL if error.
   */
  ASTNode *parse_expression(TokenList *tokens);
  ```
* Write comments for **why**, not obvious **what**.

---

## 🛠️ Tools

* Use **clang-format** for consistent formatting.
* Run format before pushing code.
* Compiler warnings: build with `-Wall -Wextra -Werror`.

