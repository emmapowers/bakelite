"""Bakelite - Protocol code generator for embedded systems."""

from importlib.metadata import PackageNotFoundError, version

try:
    __version__ = version("bakelite")
except PackageNotFoundError:
    __version__ = "(local)"
