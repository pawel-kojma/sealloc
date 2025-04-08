"""Pytest config file"""

import pytest
from pathlib import Path


def pytest_addoption(parser):
    parser.addoption(
        "--lib-path",
        action="store",
        default="libsealloc.so",
        help="Path to library implementing malloc interface",
    )
    parser.addoption(
        "--bin-dir",
        action="store",
        default="./build/test/test_e2e",
        help="Directory with test binaries",
    )
