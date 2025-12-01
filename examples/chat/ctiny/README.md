# Bakelite Chat Example (C)

A simple TCP chat demonstrating the bakelite ctiny runtime.

## Build

```bash
cmake -B build
cmake --build build
```

Or using the Makefile wrapper:
```bash
make
```

The build automatically generates `proto.h` and `bakelite.h` from `chat.bakelite` via CMake custom commands.

## Usage

Start the server in one terminal:
```bash
./build/server
```

Connect with the client in another:
```bash
./build/client
```

Type messages and press Enter to send. Use `/name <newname>` to change your display name.

## CMake Integration

This example demonstrates integrating bakelite code generation into CMake. The key parts:

```cmake
find_program(BAKELITE_EXECUTABLE bakelite REQUIRED)

add_custom_command(
    OUTPUT proto.h
    COMMAND ${BAKELITE_EXECUTABLE} gen -l ctiny -i ../chat.bakelite -o proto.h
    DEPENDS ../chat.bakelite
)
```

Generated files are rebuilt automatically when `chat.bakelite` changes.

## Interoperability

Uses the same protocol as the cpptiny (C++) version. You can run a C server with a C++ client or vice versa.
