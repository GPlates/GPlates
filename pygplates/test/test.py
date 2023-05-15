#!/usr/bin/env python

"""
Unit tests for the pygplates native library.
"""

import os
import sys
import pickle
import platform
import types
import unittest

# If on Windows and Python >= 3.8 then we need to specify the paths to all dependency DLLs of the
# pygplates '.pyd' since Python 3.8 no longer searches for these using the PATH environment variable.
if platform.system() == 'Windows':
    if sys.version_info >= (3, 8):
        # If we're in a conda environment then don't add the paths (conda already ensures dependencies are loaded correctly).
        if not os.environ.get('CONDA_PREFIX'):
            # Just add all the paths in the PATH environment variable.
            env_path = os.environ.get('PATH')
            if env_path:
                # Note that we reverse the order of PATH since 'os.add_dll_directory()'
                # appears to insert to the front of the DLL search order.
                for dll_path in reversed(env_path.split(os.pathsep)):
                    if dll_path and os.path.exists(dll_path):
                        os.add_dll_directory(dll_path)

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


class VersionCase(unittest.TestCase):
    def test_version(self):
        self.assertTrue(pygplates.Version(0, 1) < pygplates.Version.get_imported_version())
        self.assertTrue(pygplates.Version(0, 21) > pygplates.Version(0, 20))
        self.assertTrue(pygplates.Version(0, 34, prerelease_suffix='.dev1') < pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version(0, 34, prerelease_suffix='.dev1') > pygplates.Version(0, 33))
        self.assertTrue(pygplates.Version('0.34.dev1') == pygplates.Version(0, 34, 0, '.dev1'))
        self.assertTrue(pygplates.Version('0.34.dev1') < pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version('0.34.1.dev1') > pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version('0.34.1.dev1') < pygplates.Version(0, 34, 1))
        self.assertTrue(pygplates.Version('0.34.dev1') < pygplates.Version('0.34a1'))
        self.assertTrue(pygplates.Version('0.34a1') < pygplates.Version('0.34a2'))
        self.assertTrue(pygplates.Version('0.34a2') < pygplates.Version('0.34b1'))
        self.assertTrue(pygplates.Version('0.34b1') < pygplates.Version('0.34rc1'))
        self.assertTrue(pygplates.Version('0.34rc1') < pygplates.Version('0.34'))
        
        self.assertTrue(pygplates.Version('0.33').get_major() == 0)
        self.assertTrue(pygplates.Version('0.33').get_minor() == 33)
        self.assertTrue(pygplates.Version('1.33').get_major() == 1)
        self.assertTrue(pygplates.Version('0.33.1').get_patch() == 1)
        self.assertTrue(pygplates.Version('0.33.1').get_prerelease_suffix() is None)
        self.assertTrue(pygplates.Version('1.33.2.dev1').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2.dev1').get_prerelease_suffix() == ".dev1")
        self.assertTrue(pygplates.Version('1.33.dev1').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33.dev1').get_prerelease_suffix() == ".dev1")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2dev1')  # Should be "1.33.2.dev1"
        self.assertTrue(pygplates.Version('1.33.2a1').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2a1').get_prerelease_suffix() == "a1")
        self.assertTrue(pygplates.Version('1.33a1').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33a1').get_prerelease_suffix() == "a1")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2.a1')  # Should be "1.33.2a1"
        self.assertTrue(pygplates.Version('1.33.2b2').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2b2').get_prerelease_suffix() == "b2")
        self.assertTrue(pygplates.Version('1.33b2').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33b2').get_prerelease_suffix() == "b2")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2.b2')  # Should be "1.33.2b2"
        self.assertTrue(pygplates.Version('1.33.2rc3').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2rc3').get_prerelease_suffix() == "rc3")
        self.assertTrue(pygplates.Version('1.33rc3').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33rc3').get_prerelease_suffix() == "rc3")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2.rc3')  # Should be "1.33.2rc3"
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.rc3')  # Should be "1.33rc3"
        
        # Test deprecated functions
        self.assertTrue(pygplates.Version(20).get_revision() == 20)  # 'get_revision()' is deprecated
        self.assertTrue(pygplates.Version(20).get_major() == 0)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20).get_minor() == 20)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20).get_patch() == 0)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20).get_prerelease_suffix() is None)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20) < pygplates.Version(21))  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20) < pygplates.Version(0, 21))  # 'Version(revision)' is deprecated

    def test_pickle(self):
        for version in (pygplates.Version(0, 1), pygplates.Version('0.34.dev1'), pygplates.Version('0.34a2'), pygplates.Version('0.34b1'), pygplates.Version('0.34rc1')):
            self.assertTrue(version == pickle.loads(pickle.dumps(version)))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            HashableCase,
            VersionCase
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
