"""
Unit tests for the pygplates native library.
"""

import os
import sys
import types
import unittest

# Import the built pygplates library (not any other one installed on the system).
sys.path.insert(1, os.path.join(os.path.dirname(__file__), '..', '..', '..', 'bin'))
import pygplates

import test_app_logic.test
import test_model.test

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), 'fixtures')


def suite():
    suite = unittest.TestSuite()
    
    test_modules = [
            test_app_logic,
            test_model
        ]

    for test_module in test_modules:
        suite.addTest(test_module.test.suite())
    
    return suite

if __name__ == "__main__":
    unittest.TextTestRunner().run(suite())
