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
#PYGPLATES_PATH = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'bin')

# An example path used for linux/MacOS CMake builds where the source and builds directories differ...
PYGPLATES_PATH = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'python-api-2013-jul-25-build', 'bin')
#PYGPLATES_PATH = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'python-api-2013-jul-25-build-python2', 'bin')
#PYGPLATES_PATH = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'python-api-2013-jul-25-build-python3', 'bin')

# An example path used for Windows CMake builds...
#PYGPLATES_PATH = "C:\\Users\\John\\Subversion\\GPlates\\build\\python-api-2013-jul-25-merge\\bin\\profilegplates"

sys.path.insert(1, PYGPLATES_PATH)
import pygplates

import test_app_logic.test
import test_maths.test
import test_model.test

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), 'fixtures')


class HashableCase(unittest.TestCase):
    def test_hash_value(self):
        # We create clones in order that 'id()' returns a different value (even though they compare equal).
        feature_id1 = pygplates.FeatureId.create_unique_id()
        feature_id2 = pygplates.FeatureId.create_unique_id()
        feature_type = pygplates.FeatureType.create_gpml('FeatureType')
        feature_type_clone = pygplates.FeatureType.create_from_qualified_string(feature_type.to_qualified_string())
        enumeration_type = pygplates.EnumerationType.create_gpml('EnumerationType')
        enumeration_type_clone = pygplates.EnumerationType.create_from_qualified_string(enumeration_type.to_qualified_string())
        property_name = pygplates.PropertyName.create_gpml('PropertyName')
        property_name_clone = pygplates.PropertyName.create_from_qualified_string(property_name.to_qualified_string())

        # Should be able to insert hashable types into dictionary without raising TypeError.
        # And also retrieve them properly (this will fail if __eq__ is defined but __hash__
        # is left to default implementation based on 'id()' - in python 2.7 anyway).
        d = {}
        
        # Insert originals.
        d[feature_id1] = 'test1'
        d[feature_id2] = 'test2'
        d[feature_type] = 'FeatureType value'
        d[enumeration_type] = 'EnumerationType value'
        d[property_name] = 'PropertyName value'
        
        # Test using clones where possible (since clone gives different value for 'id()').
        self.assertTrue(d[feature_id1] == 'test1')
        self.assertTrue(d[feature_id2] == 'test2')
        self.assertTrue(d[feature_type_clone] == 'FeatureType value')
        self.assertTrue(d[enumeration_type_clone] == 'EnumerationType value')
        self.assertTrue(d[property_name_clone] == 'PropertyName value')

    def test_hash_identity(self):
        # Test some classes that hash using object identity (address).
        # Pretty much the majority of pygplates classes will fall in this category
        # (except for the large number of property value types).
        d = {}
        
        feature = pygplates.Feature()
        feature_ref = pygplates.FeatureCollection([feature]).get(feature.get_feature_id())
        d[feature] = 'Feature'
        self.assertTrue(d[feature_ref] == d[feature])

    def test_unhashable(self):
        d = {}
        
        # Make sure the following types are unhashable (ie, __hash__ has been set to None).
        self.assertRaises(TypeError, d.get, pygplates.GeoTimeInstant(0))
        self.assertRaises(TypeError, d.get, pygplates.FiniteRotation((0,0), 0))
        # All property value types are unhashable (due to base PropertyValue defining __hash__ to None)
        # So just test one derived property value type (since the unhashable-ness is done in the base PropertyValue class anyway).
        self.assertRaises(TypeError, d.get, pygplates.XsInteger(0))
        self.assertRaises(TypeError, d.get, pygplates.Property(pygplates.PropertyName.create_gpml('name'), pygplates.XsInteger(0)))
        self.assertRaises(TypeError, d.get, pygplates.GpmlTimeSample(pygplates.XsInteger(0), 10))
        self.assertRaises(TypeError, d.get, pygplates.GpmlTimeWindow(pygplates.XsInteger(0), 20, 10))
        self.assertRaises(TypeError, d.get, pygplates.PointOnSphere((0,0)))
        self.assertRaises(TypeError, d.get, pygplates.MultiPointOnSphere([(0,0), (0,1)]))
        self.assertRaises(TypeError, d.get, pygplates.PolylineOnSphere([(0,0), (0,1)]))
        self.assertRaises(TypeError, d.get, pygplates.PolygonOnSphere([(0,0), (0,1), (1,0)]))
        self.assertRaises(TypeError, d.get, pygplates.GreatCircleArc((0,0), (0,1)))
        self.assertRaises(TypeError, d.get, pygplates.LatLonPoint(0, 0))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            HashableCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
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
