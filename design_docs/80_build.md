# Build & tooling

## Build targets
- `make`: builds the project
- `make clean`: removes build outputs
- `make debug`: builds with `-O0 -g` and starts `gdb`

## Compiler flags
- Use warnings to keep code quality high:
  - `-Wall -Wextra -Wpedantic`

## Suggested future structure
As modules are implemented, update the Makefile to compile `src/**.c` and link into the `snake` binary.
