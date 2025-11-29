# Bakelite

Code generator for embedded systems communication protocols. Generates serialization, framing, and CRC code from protocol definition files targeting Python and C++.

## Project Structure

- `bakelite/generator/` - Code generation (parser, Jinja2 templates, CLI)
- `bakelite/proto/` - Python runtime (serialization, COBS framing, CRC)
- `bakelite/proto/types.py` - Runtime type descriptors (no external deps)
- `bakelite/tests/` - Test suite
- `docs/` - Documentation (mkdocs)

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
bakelite gen -l python -i proto.def -o output.py
bakelite runtime -l cpptiny -o bakelite.h
```

## Code Style

- Python 3.13+
- Type hints enforced via mypy (strict mode)
- 4-space indentation, 100 char line length
- Modern type syntax (`X | None`, `list[X]`, etc.)
- Priorities for this project are devex and an ergonomic api (even for generated code), small memory footpring for embedded platforms, corectness, performance.
- Prefer improvig type hints over # type-ignore comments
- The bakelite tool is just with with the bakelite command. No need for pixi run -- bakelite unless the pixi environment isn't active. User docs should assume bakelite is in the path.
- PRs, commits and documentation should be to the point. Only be verbose when the details matter to the person reading the docs. When in doubt, ask me what the docs intended purpose is.
- Don't leave comments about what you've changed unless it's a commit/pr or changelog. It makes docs and code comments cofusing.
- Don't leave comments about what you've changed unless it's a commit/pr or changelog. It makes docs and code comments cofusing.
- make sure lint is clean and all tests pass after each change