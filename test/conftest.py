"""Pytest config file"""

import pytest
from pathlib import Path


def pytest_addoption(parser):
    parser.addoption(
        "--lib-path",
        action="store",
        default="./build/src/libsealloc.so",
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


@pytest.fixture(autouse=True)
def bin_dir(request):
    return Path(request.config.getoption("--bin-dir")).resolve()


@pytest.fixture(autouse=True)
def lib_path(request):
    return Path(request.config.getoption("--lib-path")).resolve()


@pytest.fixture(autouse=True)
def progs_dir(request):
    return Path(request.config.getoption("--progs-dir")).resolve()


@pytest.fixture(scope="session")
def output_dir_e2e():
    path = Path("./test_output/e2e")
    path.mkdir(exist_ok=True, parents=True)
    return path


@pytest.fixture(scope="session")
def output_dir_performance():
    path = Path("./test_output/performance")
    path.mkdir(exist_ok=True, parents=True)
    return path
