#!/usr/bin/env python

"""
Unit tests for the pygplates native library.
"""

import os
import sys
import types
import unittest

# Import the built pygplates library (not any other one installed on the system).
# TODO: Get CMake to specify these paths using its 'configure_file' command.

# Path used for linux/MacOS CMake builds that are 'in-place' (same source and build directories)...
PYGPLATES_PATH = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'bin')

# An example path used for linux/MacOS CMake builds where the source and builds directories differ...
#PYGPLATES_PATH = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'python-api-2013-jul-25-build', 'bin')

# An example path used for Windows CMake builds...
#PYGPLATES_PATH = "C:\\Users\\John\\Development\\Usyd\\gplates\\svn\\build\\python-api-2013-jul-25\\bin\\profilegplates"

sys.path.insert(1, PYGPLATES_PATH)
import pygplates

import test_app_logic.test
import test_maths.test
import test_model.test

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), 'fixtures')


def suite():
    suite = unittest.TestSuite()
    
    # Add test suites from sibling modules.
    test_modules = [
            test_app_logic.test,
            test_maths.test,
            test_model.test
        ]

    for test_module in test_modules:
        suite.addTest(test_module.suite())
    
    return suite

if __name__ == "__main__":
    unittest.TextTestRunner().run(suite())
