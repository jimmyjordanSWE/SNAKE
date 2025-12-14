# Running tests

This project uses small, assert-based C test programs (no external test framework required). Build and run the full test suite with:

    make test

Individual module tests are available via `make test-<module>` targets, e.g. `make test-utils`, `make test-persist`, `make test-net`, `make test-tty`.

Notes:
- Tests are deterministic and quick; many use temporary files or injected test helpers.
- The project is ready for an external test framework (e.g. Criterion) if desired, but the current harness is intentionally lightweight.
