"""Unit tests configuration file."""


def pytest_configure(config):
    """Disable verbose output when running tests."""
    terminal = config.pluginmanager.getplugin("terminal")
    if terminal:
        terminal.TerminalReporter.showfspath = False
