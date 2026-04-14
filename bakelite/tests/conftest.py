"""Unit tests configuration file."""


def pytest_configure(config):
    """Disable verbose output when running tests."""
    reporter = config.pluginmanager.getplugin("terminalreporter")
    if reporter is not None:
        reporter.showfspath = False
