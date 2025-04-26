"""Pytest config file"""

from pathlib import Path

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--lib-path",
        action="store",
        default="NO_LIB_SELECTED",
        help="Path to library implementing malloc interface",
    )
    parser.addoption(
        "--bin-dir",
        action="store",
        default="./build/test/test_e2e",
        help="Directory with test binaries",
    )
    parser.addoption(
        "--progs-dir",
        action="store",
        default="./bin",
        help="Directory with programs for real program tests",
    )
    parser.addoption(
        "--out-dir",
        action="store",
        default="./test_output",
        help="Output directory of the tests",
    )


@pytest.fixture(scope="session")
def bin_dir(request):
    return Path(request.config.getoption("--bin-dir")).resolve()


@pytest.fixture(scope="session")
def lib_path(request):
    path = request.config.getoption("--lib-path")
    if path == "NO_LIB_SELECTED":
        return None
    return Path(request.config.getoption("--lib-path")).resolve()


@pytest.fixture(scope="session")
def progs_dir(request):
    return Path(request.config.getoption("--progs-dir")).resolve()


@pytest.fixture(scope="session")
def out_dir(request):
    return Path(request.config.getoption("--out-dir")).resolve()


@pytest.fixture(scope="session")
def output_dir_e2e(out_dir):
    path = out_dir / "e2e"
    path.mkdir(exist_ok=True, parents=True)
    return path


@pytest.fixture(scope="session")
def output_dir_performance(out_dir):
    path = out_dir / "performance"
    path.mkdir(exist_ok=True, parents=True)
    return path
