# Bakelite

Code generator for reliable embedded communication protocols. Generates serialization, framing, and CRC code from protocol definitions.

## Purpose

Bakelite makes it easier to build reliable communication protocols for embedded systems. While desktop/mobile systems have HTTP/REST, gRPC, etc., these aren't suitable for embedded systems with constrained memory and CPU. Bakelite targets tiny systems without a heap or memory allocator.

**Wire format philosophy**: Predictable from the definition—a developer should be able to guess the wire format after reading the spec once. Bakelite is unopinionated beyond message encoding and framing. No concept of client/server—works for mesh networks, DMA, IPC, etc.

**Versioning** (planned): Will be minimal and developer-controlled.

## Target Platforms

**Embedded** (no heap, static allocation):
- `cpptiny` - C++
- `ctiny` - C99

**Application**:
- `python` - Full runtime

**Planned**: SystemVerilog, MicroPython, embedded Rust, C++ with stdlib, C#, Java, Rust, TypeScript. New features should consider future language implementations.

## Project Structure

- `bakelite/generator/` - Code generation (parser, templates, CLI)
  - `protodef.lark` - Lark grammar for `.bakelite` IDL
  - `templates/` - Jinja2 templates (one per language)
  - `runtimes/` - Static runtime support files (`common/`, `cpptiny/`, `ctiny/`)
- `bakelite/proto/` - Python runtime (serialization, COBS framing, CRC)
- `bakelite/tests/` - Test suite
- `examples/` - Arduino (Python↔embedded) and TCP chat (C/C++)
- `docs/` - mkdocs documentation

## Architecture

- **Generator**: Parses `.bakelite` IDL → generates language-specific code via Jinja2 templates
- **Runtime**: Static helper code (framing, CRC) that doesn't change per-protocol
- **Zero-copy design**: Embedded targets use single-buffer approach, no dynamic allocation
- **Template mapping**: Each language has `{lang}.py` generator + `{lang}*.j2` templates

## Design Constraints

When adding features:
- Embedded targets: No heap allocation, static buffers only
- Wire format must remain predictable from protocol definition
- Features must be implementable across all planned target languages
- Keep protocol-level opinions minimal (except versioning)

## Development

Uses **Pixi** for dependency management and task running.

```bash
pixi run test      # Run Python tests
pixi run test-cpp  # Run C++ tests
pixi run test-all  # Run all tests
pixi run lint      # Run ruff and mypy
pixi run format    # Auto-format with ruff
pixi run build     # Build package
pixi run docs      # Build documentation
```

## Key Technologies

- **Lark** - Parser generator for protocol DSL (`protodef.lark`)
- **Jinja2** - Template engine for code generation
- **Ruff** - Linting and formatting
- **mypy** - Static type checking
- **pytest** - Testing with pytest-describe for BDD-style tests
- **hatchling** - Build backend for packaging

## CLI

```bash
bakelite gen -l python -i proto.bakelite -o output.py
bakelite gen -l cpptiny -i proto.bakelite -o output.h
bakelite gen -l ctiny -i proto.bakelite -o output.h
bakelite runtime -l cpptiny -o bakelite.h
```

## Code Style

- Python 3.13+
- Type hints enforced via mypy (strict mode)
- 4-space indentation, 100 char line length
- Modern type syntax (`X | None`, `list[X]`, etc.)
- Priorities: devex, ergonomic API (even for generated code), small memory footprint, correctness, performance
- Prefer improving type hints over `# type: ignore` comments
- The bakelite tool is invoked with `bakelite` command. User docs should assume bakelite is in the path.
- PRs, commits and documentation should be to the point. Only be verbose when the details matter.
- Don't leave comments about what you've changed unless it's a commit/PR or changelog.
- Make sure lint is clean and all tests pass after each change.
- All target languages must have tests with at least 80% code coverage.
