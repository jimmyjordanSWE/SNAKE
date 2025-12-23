# Project Overview

This is a C-based snake game. The project is built with `make` and uses the SDL2 library for graphics. The code is well-structured, with a clear separation of concerns between modules. It follows a set of coding standards that emphasize modularity, memory safety, and testability.

# Building and Running

## Building

*   **Debug build (default):**
    ```bash
    make
    ```
*   **Release build:**
    ```bash
    make release
    ```

## Running

*   **Run the game:**
    ```bash
    ./snakegame
    ```

## Testing

The project has a suite of tests. While there isn't a single command to run all tests, you can run individual test targets from the `Makefile`. For example:

```bash
make test-3d
```

# Development Conventions

## Coding Style

The project follows a strict set of C coding standards, which are documented in `C coding standards.md`. Key conventions include:

*   **Opaque Pointers:** Hiding implementation details using opaque pointers.
*   **Error Handling:** Using an "error-out" pattern for cleanup.
*   **Dependency Injection:** Using capability structs to pass function pointers for dependencies.
*   **Memory Safety:** Strict rules for memory allocation and deallocation.

## Formatting

The project uses `clang-format` for code formatting. There are two formatting styles defined:

*   **`format-llm`:** A style optimized for LLM token usage.
*   **`format-human`:** A more human-readable style.

You can format the code using the following commands:

```bash
make format-llm
make format-human
```

# Project Context & AST

To provide better context for LLMs, this project uses `tree-sitter` to generate a summarized Abstract Syntax Tree (AST) of the codebase, highlighting key functions, structs, and interfaces.

## Updating the AST Context

If you add new modules or change major interfaces, update the context file by running:

```bash
make context
```

The resulting `PROJECT_CONTEXT.txt` will be used by the Gemini CLI to understand the project structure more deeply.
