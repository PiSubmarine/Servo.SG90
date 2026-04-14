# PiSubmarine AGENTS.md

This is a module of PiSubmarine project - a Raspberry Pi-based underwater FPV drone.

PiSubmarine is a distributed system consisting of multiple cooperating components:

- Embedded real-time controller (STM32)
- Linux-based control unit (Raspberry Pi)
- Remote clients

Design decisions must account for:

- Network boundaries
- Latency
- Partial failures

## Project structure

### Build System

PiSubmarine consists of multiple git repositories, each containing at least one module. All internal repositories can be
found in https://github.com/PiSubmarine organization (all public).

PiSubmarine uses a custom FetchContent-based Build System, implemented
in https://github.com/PiSubmarine/Build.CMake/blob/main/PiSubmarine.Build.CMake.cmake

Internal dependencies are declared with function PiSubmarineAddDependency(URL TAG) and are downloaded as source code to
the build directory.

It is NOT safe to modify dependencies inside the build directory. FetchContent creates separate copies per build
configuration. Any local changes may be lost or duplicated.

Always modify the original repository and update the TAG (if present) in PiSubmarineAddDependency.
Usually, there is a local copy of the dependency in the directory above the source root.
For example, if the current project is in C:\Projects\PiSubmarine\Drv8908, and it depends
on https://github.com/PiSubmarine/Spi.Linux then a local authoritative copy of Spi.Linux is likely located in
C:\Projects\PiSubmarine\Spi.Linux. If not found, ask for the correct local location.

Each module may depend on other PiSubmarine modules, as well as external libraries. Vcpkg manages external libraries.

Modules for all platforms (except STM32) use https://github.com/PiSubmarine/ModuleTemplate as a template.

### Module Structure

Modules have a strict directory structure:

- **src**: main portion of module code. Compiled as a library. All reusable code goes here.
  Can be linked against by other modules.
- **public**: contains .h files associated with src. Can be linked against by other modules.
- **mock**: contains gmocks for interfaces, defined in **public**. Can be linked against by other modules.
- **test**: contains unit tests for src. Cannot be linked against.
- **app**: contains an executable (or multiple executables), closely associated with **src**. Cannot be linked against.

Code is placed in subdirectories of the above directories, copying the namespace path. For example, class PiSubmarine::
Drv8908::Driver is placed in public/PiSubmarine/Drv8908/Driver.h and src/PiSubmarine/Drv8908/Driver.cpp.
Test for this class is placed in test/PiSubmarine/Drv8908/DriverTest.cpp. This rule is not enforced, but encouraged.

### Module Responsibilities

- Each module must have a single, well-defined responsibility.
- Avoid "utility" or "misc" modules. If you see a common functionality that should be extracted, ask what to do.
- If a module grows beyond a single responsibility, split it.

### Build and CMake Usage

CMake must always be invoked using presets (consult CMakePresets.json). For example, for Windows host the correct preset
is "windows-x64-debug". Do not call CMake manually with custom parameters.

## Target platforms

The main targets are:

- x64-Windows or x64-Linux for development, testing and operator control.
- Raspberry Pi Zero 2 W for the on-board computer.
- STM32 for on-board chipset (manages power and initial boot of Raspberry Pi)

## Naming conventions

- Modules (repositories) are named after the main class they contain or their main functionality. For example, the
  module that implements DRV8908 PWM driver is named "PiSubmarine::Drv8908". "PiSubmarine" is omitted in the repository
  name, because it is already part of the GitHub URL (as organization).
- Modules (repositories) containing platform-specific code are (usually) marked with the platform name suffix in their
  name and namespace. For example, the module that implements the SPI driver for Linux is named "PiSubmarine.Spi.Linux".
- Modules (repositories) containing only interfaces and type definitions are marked with ".Api" suffix in their name and
  namespace. For example, the module that defines SPI driver interface is named "PiSubmarine.Spi.Api".
- Namespaces should be treated as part of class names; do not repeat words in full names (e.g., use "class PiSubmarine::
  Drv8908::Driver" instead of "class PiSubmarine::Drv8908::Drv8908Driver").
- Interfaces should be prefixed with "I".

## Cross-cutting concepts

- spdlog is used for logging on all platforms, except STM32 (no logging on STM32).
- gtest is used for unit tests on all platforms, except STM32 (no unit tests for platform-specific STM32 code).
- If code may fail, use std::expected<Value, ErrorEnum> for return values, even if Value is void.
- Use fail-fast error handling for errors that are not recoverable. Usage of exceptions is allowed on all platforms,
  except STM32.
- All platforms support C++23, all modules are compiled with C++23.
- Units (Volts, Amperes, etc.) are defined in SI units and are located in PiSubmarine.Units repository in PiSubmarine
  namespace.

## Software Architecture

Clarity and maintainability are more important than brevity. Adhere to SOLID principles. Do not shorten the code if this
violates SOLID or makes it less readable or maintainable. If something can be exported as an interface, ask. Strive for
loose coupling.

- High-level modules must not depend on low-level modules. Depend on abstractions.
- Platform-specific code must never leak into platform-independent modules.
- Avoid cyclic dependencies between modules at all cost.
- Prefer composition over inheritance unless polymorphism is explicitly required.
- Business logic must be testable without hardware.
- Implementation modules must depend on Api modules, never the other way around.
- Do not define interfaces inside implementation modules unless strictly private.
- If multiple modules need a contract, extract it into a dedicated *.Api module.

## Concurrency and Thread Safety

The project uses a tick-based approach (like in game engines). Objects that require time-based updates must
implement https://github.com/PiSubmarine/Time.Tickable.Api/blob/main/public/PiSubmarine/Time/ITickable.h interface. All
ticks are processed in a single thread.

In case that multi-threading is required, adhere to the following rules:

- Thread safety must be explicit: document whether a class is thread-safe or not.
- Do not hide synchronization inside low-level classes unless necessary.
- Prefer message-passing or queues over shared mutable state.
- Avoid blocking calls in control loops or time-critical paths.
- Use RAII for synchronization primitives.
- The tick loop is single-threaded and must remain deterministic.
- Background threads may not directly modify ticked objects.
- Cross-thread communication must happen via queues or message passing.
- Shared state between threads must be minimized and explicitly synchronized.

## Determinism

- Time-critical parts of the system must behave deterministically.
- Avoid non-deterministic operations (e.g., blocking I/O, dynamic allocation) in control paths.
- Execution time of tick handlers should be predictable and bounded.

## Error Handling

- Use std::expected for recoverable errors.
- Use exceptions only for unrecoverable or initialization-time failures (except STM32).
- Never mix exceptions and std::expected in the same API boundary.
- Error enums must be strongly typed and domain-specific.
- Do not return generic error codes.

## Hardware Abstraction

- All hardware access must go through interfaces.
- No direct register or driver access in high-level modules.
- Hardware drivers must be isolated in platform-specific modules.
- Mock implementations must be provided for all hardware interfaces.
- Hardware interfaces must not expose platform-specific types or concepts.

## Testing Strategy

- All business logic must have unit tests.
- Mock external dependencies using gmock.
- Do not test implementation details. Test behavior instead.
- Platform-specific code may skip tests only if impossible to mock.
- If some non-trivial code is constexpr, test that it can actually be evaluated at compile time. Use static_assert.

## Logging

- See the logging cross-cutting concept.
- Do not use logging in STM32, except for debugging. Production STM32 code must not log anything.
- Do not log in tight loops unless necessary.
- Logging must not affect timing-critical code.
- Loggers should be injected or retrieved for each module. Do not use global loggers.
- Logging must include sufficient context (module, operation, identifiers).

## Code Review Expectations

- Prefer clarity over cleverness.
- Question architecture, not only syntax.
- Reject code that introduces hidden coupling.
- Highlight violations of SOLID or layering.

## Memory Management

- Avoid dynamic allocations in time-critical or embedded code paths.
- Prefer static or pre-allocated memory where possible.
- Allocation patterns must be explicit and predictable.

## API Design

- APIs must be minimal and explicit.
- Avoid "fat" interfaces with unrelated methods.
- Prefer multiple small interfaces over one large interface.
- All API methods must clearly define ownership and lifetime of data.
- Avoid implicit behavior or hidden side effects.

## When in Doubt

- If a change affects architecture, module boundaries or interfaces — ask before implementing.
- If multiple design options exist, present alternatives with their trade-offs.
- Do not make assumptions about hardware behavior or requirements.

## Do's

- Use modern C++23 features.
- Use modern CMake.
- Use constexpr.
- Use namespaces.
- Use the standard library features, unless not supported by the platform. If not sure, ask.
- Define and use explicit types instead of int and double. If a chip reports millivolts, define a type for it, do not
  return simple int. Many types are already defined in PiSubmarine.Units.
- Point out code smells, wrong architecture decisions, and other issues that may be found in the code.
- Write self-explanatory code. Comments should be used sparingly only where behavior is not obvious.

## Don'ts

- Do not use macros unless absolutely necessary.
- Do not use C-style code unless absolutely necessary.
- Do not use unmanaged memory. Use smart pointers for owned memory, references for not-owned memory. Use raw pointers
  sparingly. Prefer static allocation.
- Do not use std::cout, unless absolutely necessary. See logging cross-cutting concept.
- Never use std::cin (or anything that introduces tight coupling) in src code.