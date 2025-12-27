# Course Requirement Gap Analysis

This document evaluates the current project implementation against the learning outcomes specified in the course requirements (`kurs_krav.txt`) and planning (`kursplan.txt`).

## Overview

The project demonstrates high-quality C99 engineering, modularity, and memory safety. However, several specific technical requirements from the curriculum remain partially addressed or omitted.

## Identified Gaps

### 1. Networking (TCP/Socket IO)
- **Requirement**: Implementation of a network-based multiplayer mode using TCP and HTTP.
- **Current Status**: Data serialization (packing/unpacking) is implemented in `src/net/net.c`. However, the active networking layer (socket creation, connection management, and data transmission) is currently stubbed.
- **Demonstration Goal**: Demonstrating proficiency in POSIX socket programming and real-time state synchronization.

### 2. Security Integration (mbedtls)
- **Requirement**: Use of the `mbedtls` library for secure communication.
- **Current Status**: The project does not currently integrate `mbedtls` or implement encrypted data streams. Any future network communication would facilitate plaintext transmission.
- **Demonstration Goal**: Implementation of transport layer security (TLS) for sensitive game data and high scores.

### 3. Systematic Testing
- **Requirement**: Clear implementation of test strategies and validation of correct functionality.
- **Current Status**: While build targets for testing exist, the project lacks a comprehensive suite of automated tests for core logic (e.g., collision detection, scoring, and state transitions).
- **Demonstration Goal**: Use of unit testing frameworks or custom test harnesses to ensure functional robustness.

### 4. Security Analysis & Input Validation
- **Requirement**: Execution of security analyses (e.g., pen-testing) and patching of identified vulnerabilities like buffer overflows.
- **Current Status**: The codebase utilizes safe string functions (`snprintf`) and the error-out pattern. However, a formal security audit of external data sources (like network packets or configuration files) is not documented.
- **Demonstration Goal**: Documented hardening against common C-specific exploits and rigorous validation of all untrusted input.

## Conclusion

The project successfully meets the core C99 programming and modularity goals. To fully satisfy the specialized networking and security outcomes of the curriculum, the implementation of a secured networking client using `mbedtls` and a formalised test suite is recommended.
