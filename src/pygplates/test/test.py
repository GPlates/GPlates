"""
Unit tests for the pygplates native library.
"""

import os
import sys
import types
import unittest

# Import the built pygplates library (not any other one installed on the 
# system)
sys.path.insert(1, os.path.join(os.path.dirname(__file__), 
    '..', '..', '..', 'bin'))
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), 'fixtures')


class FileHelper:
    def __init__(self):
        self.file_format_registry = pygplates.FeatureCollectionFileFormatRegistry()

    def read(self, filename):
        return self.file_format_registry.read(filename)


class FeatureCase(unittest.TestCase):

    def setUp(self):
        self.property_count = 4
        self.feature = iter(FileHelper().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))).next()

    def test_feature(self):
        self.assertTrue(self.feature)

    def test_feature_id(self):
        feature_id = self.feature.get_feature_id()
        self.assertTrue(isinstance(feature_id, pygplates.FeatureId))

    def test_feature_type(self):
        feature_type = self.feature.get_feature_type()
        self.assertTrue(isinstance(feature_type, pygplates.FeatureType))

    def test_revision_id(self):
        self.assertEquals('GPlates-d172eaab-931f-484e-a8b6-0605e2dacd18',
                self.feature.get_revision_id())

    def test_is_iterable(self):
        """
        Features provide iteration over the properties they contain.
        """
        try:
            i = iter(self.feature)
            self.assertTrue(i)
        except TypeError, e:
            self.assertTrue(False, 'Feature is not iterable')

    def test_get_features(self):
        counter = 0
        for property in iter(self.feature):
            self.assertTrue(isinstance(property, pygplates.Property))
            counter += 1
        self.assertTrue(counter == self.property_count, 
                "Expected " + str(self.property_count) + " properties, actual " + 
                str(counter) + " properties")


class FeatureCollectionCase(unittest.TestCase):

    def setUp(self):
        self.volcanoes_filename = os.path.join(FIXTURES, 'volcanoes.gpml')
        # The volcanoes file contains 4 volcanoes
        self.feature_count = 4
        self.feature_collection = FileHelper().read(self.volcanoes_filename)

    def test_is_iterable(self):
        """
        Feature collections provide iteration over the features they contain.
        """
        try:
            i = iter(self.feature_collection)
            self.assertTrue(i)
        except TypeError, e:
            self.assertTrue(False, 'Feature collection is not iterable')

    def test_get_features(self):
        counter = 0
        for feature in iter(self.feature_collection):
            self.assertTrue(isinstance(feature, pygplates.Feature))
            counter += 1
        self.assertTrue(counter == self.feature_count, 
                "Expected " + str(self.feature_count) + " features, actual " + 
                str(counter) + " features")


class FeatureCollectionFileFormatRegistryCase(unittest.TestCase):

    def setUp(self):
        self.volcanoes_filename = os.path.join(FIXTURES, 'volcanoes.gpml')
        self.rotations_filename = os.path.join(FIXTURES, 'rotations.rot')
        self.file_helper = FileHelper()

    def test_load_feature_collection(self):
        volcanoes = self.file_helper.read(self.volcanoes_filename)
        self.assertTrue(volcanoes)


    def test_load_rotation_model(self):
        rotations = self.file_helper.read(self.rotations_filename)
        self.assertTrue(rotations)

    def test_load_invalid_name(self):
        for name in [ "an_invalid_file_name", None ]:
            try:
                self.file_helper.read(None)
                self.assertTrue(False, "Loading invalid file name should fail")
            except Exception, e:
                self.assertEquals(e.__class__.__name__, 'ArgumentError')
        

class GeoTimeInstantCase(unittest.TestCase):

    def test_create(self):
        age = 10.0
        instant = pygplates.GeoTimeInstant(age)
        self.assertEquals(instant.get_value(), age)

    def test_is_distant_future(self):
        instant = pygplates.GeoTimeInstant.create_distant_future()
        self.assertTrue(instant.is_distant_future())

    def test_is_distant_past(self):
        instant = pygplates.GeoTimeInstant.create_distant_past()
        self.assertTrue(instant.is_distant_past())


class PropertyNameCase(unittest.TestCase):

    def setUp(self):
        feature = iter(FileHelper().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))).next()
        i = iter(feature)
        # From the first volcano: its name and valid time (name is blank)
        self.feature_name = i.next()
        self.feature_valid_time = i.next()

    def test_get_name(self):
        # Feature property: gml:name
        self.assertTrue(isinstance(self.feature_name.get_name(), 
            pygplates.PropertyName))
        self.assertEquals(self.feature_name.get_name().to_qualified_string(),
                'gml:name')
        # Feature property: gml:validTime
        self.assertTrue(isinstance(self.feature_valid_time.get_name(), 
            pygplates.PropertyName))
        self.assertEquals(self.feature_valid_time.get_name().to_qualified_string(),
                'gml:validTime')


class PropertyValueCase(unittest.TestCase):

    def setUp(self):
        feature = iter(FileHelper().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))).next()
        i = iter(feature)
        # From the first volcano: its name and valid time (name is blank)
        self.feature_name = i.next()
        self.feature_valid_time = i.next()

    def test_get_value(self):
        # The volcano's name is blank so we expect an empty string
        self.assertTrue(isinstance(self.feature_name.get_value(), 
            pygplates.XsString))
        # The volcano has a valid time, so we expect a value for this
        valid_time = self.feature_valid_time.get_value()
        self.assertTrue(isinstance(valid_time, pygplates.PropertyValue))
        # Specifically, this should be a GmlTimePeriod value
        self.assertTrue(isinstance(valid_time, pygplates.GmlTimePeriod))
        begin_time = valid_time.get_begin()
        self.assertTrue(isinstance(begin_time, pygplates.GmlTimeInstant))
        end_time = valid_time.get_end()
        self.assertTrue(isinstance(end_time, pygplates.GmlTimeInstant))
        # Test the actual values of the time period
        self.assertEquals(begin_time.get_time_position().get_value(), 40.0)
        self.assertEquals(end_time.get_time_position().get_value(), 0.0)


class ReconstructionTreeCase(unittest.TestCase):

    def setUp(self):
        self.rotations = FileHelper().read(os.path.join(FIXTURES, 'rotations.rot'))

    def test_create(self):
        reconstruction_tree = pygplates.ReconstructionTree.create([ self.rotations ], 10.0)
        self.assertTrue(isinstance(reconstruction_tree, pygplates.ReconstructionTree))

    def test_get_equivalent_total_rotation(self):
        reconstruction_tree = pygplates.ReconstructionTree.create([ self.rotations ], 10.0)
        finite_rotation = reconstruction_tree.get_equivalent_total_rotation(801)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))


if __name__ == "__main__":
    unittest.main()
