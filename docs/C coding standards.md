# C Coding Standards

## Purpose
- Provide a lightweight, practical set of rules to keep C code readable, safe, and testable.

## Formatting and Naming
Formatting should be human-readable, but the highest priority is on minimizing LLM token usage.

## Files & Headers
- Each module has a single public header `module.h` and an implementation `module.c`.
- Use `#pragma once` 
- Opaque pattern. Public headers expose only the API needed by callers; types and helpers that are internal should be in a `module_internal.h` or `module.c`.

## Code Organization
- Order declarations: macros, types, static prototypes, static data, public functions, static functions
- Group related functions together
- Keep related declarations in the same compilation unit when possible

## API Design & Ownership
- Document ownership and lifetime of returned pointers and out-parameters in header comments.
- Prefer explicit initialization and teardown functions (`init`, `shutdown`) over implicit static init.
- Avoid hidden global state; prefer passing context structs to functions.

## Common Idioms & Patterns

### Opaque Pointers
Hide implementation details by declaring structs in headers without defining them:
```c
// module.h
typedef struct module_context module_context_t;
module_context_t* module_create(void);

// module.c
struct module_context { /* actual fields */ };
```

### Error-Out Pattern
Single exit point with cleanup:
```c
int func(void) {
    int err = 0;
    resource_t* r = NULL;
    
    r = alloc_resource();
    if (!r) { err = ERR_NOMEM; goto out; }
    
    // ... work ...
    
out:
    free_resource(r);
    return err;
}
```

### Capability Structs
Pass function pointers for dependency injection:
```c
typedef struct {
    int (*read)(void* ctx, void* buf, size_t n);
    int (*write)(void* ctx, const void* buf, size_t n);
    void* context;
} io_ops_t;
```

## Error Handling
- Return explicit error codes (use enums or `int` with named constants). Document expected errors.
- Check function return values and propagate errors upward rather than ignoring them.

## Memory Safety
- Prefer stack allocation for small objects; use `malloc`/`free` with clear ownership.
- Validate all allocations and handle failures gracefully.
- Always free in reverse order of allocation.
- Set pointers to NULL after freeing.
- Use `sizeof(*ptr)` instead of `sizeof(type)` in allocations to prevent type mismatches.

## Safety & Robustness
- Always initialize pointers to NULL and check before dereferencing.
- Zero-initialize structs to avoid uninitialized fields: `struct foo f = {0};`
- Validate array indices and buffer sizes before access.
- Check for integer overflow in size calculations before allocation.

## Const Correctness
- Mark pointer parameters `const` if the function doesn't modify the pointee.
- Use `const` for read-only data structures and lookup tables.
- Helps document intent and enables compiler optimizations.

## String Handling
- Prefer `strncpy`, `snprintf` over unbounded `strcpy`, `sprintf`.
- Always null-terminate after bounded copy operations.
- Consider using counted strings (ptr + length) for binary-safe data.

## Global/Static State
- Minimize globals. If needed, restrict to `static` with well-documented accessors and test hooks.

## Assertions & Defensive Checks
- Use `assert()` for invariants that indicate programming errors.
- Never use `assert()` with side effects (it's compiled out in release builds).
- Use defensive checks for input validation and return errors instead of crashing.

## Testability
- Design functions with clear inputs and outputs so they are easy to unit test.
- Provide hooks or dependency injection for RNG, time, file IO, and platform interactions to support deterministic tests and mocking.

## Miscellaneous
- Prefer fixed-width integer types (`uint32_t`, `int64_t`) for public APIs.
- Limit complex macros; prefer inline static functions where possible.
- Keep functions short and focused (prefer < 100 lines; split complex logic into helpers).

## Documentation
- Document each public function with a short synopsis, parameters, return value, and ownership notes.

## Review
- Conformance to these standards is required before a module proceeds to the test-writing phase. Use the checklist in `impl_012_tests.md` to record completion of the code-review step.