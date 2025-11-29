"""Tests for CLI interface."""

import os
import tempfile

from click.testing import CliRunner

from bakelite.generator.cli import cli

FILE_DIR = os.path.dirname(os.path.realpath(__file__))


def describe_gen_command():
    def generates_python_code(expect):
        runner = CliRunner()
        with tempfile.NamedTemporaryFile(suffix=".py", delete=False) as f:
            output_file = f.name

        try:
            result = runner.invoke(
                cli,
                [
                    "gen",
                    "-l",
                    "python",
                    "-i",
                    f"{FILE_DIR}/struct.bakelite",
                    "-o",
                    output_file,
                ],
            )
            expect(result.exit_code) == 0
            with open(output_file) as f:
                content = f.read()
            expect("class TestStruct" in content) == True
            expect("@dataclass" in content) == True
        finally:
            os.unlink(output_file)

    def generates_cpptiny_code(expect):
        runner = CliRunner()
        with tempfile.NamedTemporaryFile(suffix=".h", delete=False) as f:
            output_file = f.name

        try:
            result = runner.invoke(
                cli,
                [
                    "gen",
                    "-l",
                    "cpptiny",
                    "-i",
                    f"{FILE_DIR}/struct.bakelite",
                    "-o",
                    output_file,
                ],
            )
            expect(result.exit_code) == 0
            with open(output_file) as f:
                content = f.read()
            expect("struct TestStruct" in content) == True
        finally:
            os.unlink(output_file)

    def fails_with_unknown_language(expect):
        runner = CliRunner()
        result = runner.invoke(
            cli,
            [
                "gen",
                "-l",
                "unknown",
                "-i",
                f"{FILE_DIR}/struct.bakelite",
                "-o",
                "/tmp/out.txt",
            ],
        )
        expect(result.exit_code) == 1
        expect("Unknown language" in result.output) == True

    def fails_with_missing_input(expect):
        runner = CliRunner()
        result = runner.invoke(
            cli,
            ["gen", "-l", "python", "-i", "/nonexistent/file.bakelite", "-o", "/tmp/out.py"],
        )
        expect(result.exit_code) != 0

    def requires_all_options(expect):
        runner = CliRunner()
        result = runner.invoke(cli, ["gen", "-l", "python"])
        expect(result.exit_code) != 0
        expect("Missing option" in result.output) == True


def describe_runtime_command():
    def generates_cpptiny_runtime(expect):
        runner = CliRunner()
        with tempfile.NamedTemporaryFile(suffix=".h", delete=False) as f:
            output_file = f.name

        try:
            result = runner.invoke(
                cli,
                ["runtime", "-l", "cpptiny", "-o", output_file],
            )
            expect(result.exit_code) == 0
            with open(output_file) as f:
                content = f.read()
            expect("namespace Bakelite" in content) == True
        finally:
            os.unlink(output_file)

    def generates_python_runtime(expect):
        runner = CliRunner()
        with tempfile.TemporaryDirectory() as tmpdir:
            result = runner.invoke(
                cli,
                ["runtime", "-l", "python", "-o", tmpdir],
            )
            expect(result.exit_code) == 0
            # Check that the runtime folder was created with expected files
            runtime_dir = os.path.join(tmpdir, "bakelite_runtime")
            expect(os.path.isdir(runtime_dir)) == True
            expect(os.path.isfile(os.path.join(runtime_dir, "__init__.py"))) == True
            expect(os.path.isfile(os.path.join(runtime_dir, "serialization.py"))) == True

    def fails_with_unknown_language(expect):
        runner = CliRunner()
        result = runner.invoke(
            cli,
            ["runtime", "-l", "unknown", "-o", "/tmp/"],
        )
        expect(result.exit_code) == 1
        expect("Unknown language" in result.output) == True


def describe_main_group():
    def shows_help(expect):
        runner = CliRunner()
        result = runner.invoke(cli, ["--help"])
        expect(result.exit_code) == 0
        expect("gen" in result.output) == True
        expect("runtime" in result.output) == True
