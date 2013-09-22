"""
Unit tests for the pygplates model API.
"""

import os
import unittest
import pygplates

import test_ids
import test_property_values
import test_revisioned_vector

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class FeatureCase(unittest.TestCase):

    def setUp(self):
        self.property_count = 4
        self.feature = iter(pygplates.FeatureCollectionFileFormatRegistry().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))).next()

    def test_len(self):
        self.assertEquals(len(self.feature), self.property_count)
    
    def test_feature(self):
        self.assertTrue(self.feature)

    def test_feature_id(self):
        feature_id = self.feature.get_feature_id()
        self.assertTrue(isinstance(feature_id, pygplates.FeatureId))
        self.assertEquals('GPlates-d13bba7f-57f5-419d-bbd4-105a8e72e177', feature_id.get_string())

    def test_feature_type(self):
        feature_type = self.feature.get_feature_type()
        self.assertTrue(isinstance(feature_type, pygplates.FeatureType))
        self.assertEquals(feature_type, pygplates.FeatureType.create_gpml('Volcano'))

    def test_revision_id(self):
        revision_id = self.feature.get_revision_id()
        self.assertTrue(isinstance(revision_id, pygplates.RevisionId))
        self.assertEquals('GPlates-d172eaab-931f-484e-a8b6-0605e2dacd18', revision_id.get_string())

    def test_is_iterable(self):
        """
        Features provide iteration over the properties they contain.
        """
        iter(self.feature) # Raises TypeError if not iterable

    def test_get_features(self):
        counter = 0
        for property in self.feature:
            self.assertTrue(isinstance(property, pygplates.Property))
            counter += 1
        self.assertTrue(counter == self.property_count, 
                "Expected " + str(self.property_count) + " properties, actual " + 
                str(counter) + " properties")
    
    def test_remove(self):
        # Find the 'gml:name' property.
        name_property = None
        for property in self.feature:
            if property.get_name() == pygplates.PropertyName.create_gml('name'):
                name_property = property
                break
        self.assertTrue(name_property)
        
        # Should not raise ValueError.
        self.feature.remove(name_property)
        self.assertTrue(len(self.feature) == self.property_count - 1)
        # Should not be able to find it now.
        missing_name_property = None
        for property in self.feature:
            if (property.get_name() == pygplates.PropertyName.create_gml('name') or
                property == name_property):
                missing_name_property = property
                break
        self.assertFalse(missing_name_property)
    
    def test_add(self):
        integer_property = pygplates.Property(
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(100))
        self.feature.add(integer_property)
        self.assertTrue(len(self.feature) == self.property_count + 1)
        found_integer_property = None
        for property in self.feature:
            if property == integer_property:
                found_integer_property = property
                break
        self.assertTrue(found_integer_property)


class FeatureCollectionCase(unittest.TestCase):

    def setUp(self):
        self.volcanoes_filename = os.path.join(FIXTURES, 'volcanoes.gpml')
        # The volcanoes file contains 4 volcanoes
        self.feature_count = 4
        self.feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(self.volcanoes_filename)

    def test_len(self):
        self.assertEquals(len(self.feature_collection), self.feature_count)

    def test_is_iterable(self):
        """
        Feature collections provide iteration over the features they contain.
        """
        iter(self.feature_collection) # Raises TypeError if not iterable

    def test_get_features(self):
        counter = 0
        for feature in self.feature_collection:
            self.assertTrue(isinstance(feature, pygplates.Feature))
            counter += 1
        self.assertTrue(counter == self.feature_count, 
                "Expected " + str(self.feature_count) + " features, actual " + 
                str(counter) + " features")
    
    def test_remove(self):
        # Get the second feature.
        feature_iter = iter(self.feature_collection)
        feature_iter.next(); # Skip first feature.
        feature_to_remove = feature_iter.next()
        
        # Should not raise ValueError.
        self.feature_collection.remove(feature_to_remove)
        self.assertTrue(len(self.feature_collection) == self.feature_count - 1)
        # Should not be able to find it now.
        missing_feature = None
        for feature in self.feature_collection:
            if feature.get_feature_id() == feature_to_remove.get_feature_id():
                missing_feature = feature
                break
        self.assertFalse(missing_feature)
    
    def test_add(self):
        integer_property = pygplates.Property(
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(100))
        # Create a feature with a new unique feature ID.
        feature_with_integer_property = pygplates.Feature()
        feature_with_integer_property.add(integer_property)
        self.feature_collection.add(feature_with_integer_property)
        self.assertTrue(len(self.feature_collection) == self.feature_count + 1)
        # Should be able to find it in the collection.
        found_feature_with_integer_property = None
        for feature in self.feature_collection:
            if feature.get_feature_id() == feature_with_integer_property.get_feature_id():
                found_feature_with_integer_property = feature
                break
        self.assertTrue(found_feature_with_integer_property)
        # Added feature should have one property.
        self.assertTrue(len(found_feature_with_integer_property) == 1)
        self.assertTrue(iter(found_feature_with_integer_property).next().get_value().get_integer() == 100)


class FeatureCollectionFileFormatRegistryCase(unittest.TestCase):

    def setUp(self):
        self.volcanoes_filename = os.path.join(FIXTURES, 'volcanoes.gpml')
        self.rotations_filename = os.path.join(FIXTURES, 'rotations.rot')
        self.file_format_registry = pygplates.FeatureCollectionFileFormatRegistry()

    def test_load_feature_collection(self):
        volcanoes = self.file_format_registry.read(self.volcanoes_filename)
        self.assertTrue(volcanoes)


    def test_load_rotation_model(self):
        rotations = self.file_format_registry.read(self.rotations_filename)
        self.assertTrue(rotations)

    def test_load_invalid_name(self):
        try:
            self.file_format_registry.read(None)
            self.assertTrue(False, "Loading invalid file name should fail")
        except Exception, e:
            self.assertEquals(e.__class__.__name__, 'ArgumentError')

    def test_unsupported_file_format(self):
        # Unsupported file format exception.
        self.assertRaises(
                pygplates.FileFormatNotSupportedError,
                self.file_format_registry.read,
                "this_file_format_is.unknown")

    def test_unable_to_open_for_read(self):
        # Unable to open file for reading exception (using a supported file format).
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                self.file_format_registry.read,
                "this_file_does_not_exist.gpml")


class GeoTimeInstantCase(unittest.TestCase):

    def test_create(self):
        age = 10.0
        instant = pygplates.GeoTimeInstant(age)
        self.assertAlmostEqual(instant.get_value(), age)

    def test_is_distant_future(self):
        instant = pygplates.GeoTimeInstant.create_distant_future()
        self.assertTrue(instant.is_distant_future())

    def test_is_distant_past(self):
        instant = pygplates.GeoTimeInstant.create_distant_past()
        self.assertTrue(instant.is_distant_past())


class PropertyNameCase(unittest.TestCase):

    def setUp(self):
        feature = iter(pygplates.FeatureCollectionFileFormatRegistry().read(
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
        feature = iter(pygplates.FeatureCollectionFileFormatRegistry().read(
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
        begin_time = valid_time.get_begin_time()
        self.assertTrue(isinstance(begin_time, pygplates.GeoTimeInstant))
        end_time = valid_time.get_end_time()
        self.assertTrue(isinstance(end_time, pygplates.GeoTimeInstant))
        # Test the actual values of the time period
        self.assertAlmostEqual(begin_time.get_value(), 40.0)
        self.assertAlmostEqual(end_time.get_value(), 0.0)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            FeatureCase,
            FeatureCollectionCase,
            FeatureCollectionFileFormatRegistryCase,
            GeoTimeInstantCase,
            PropertyNameCase,
            PropertyValueCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    # Add test suites from sibling modules.
    test_modules = [
            test_ids,
            test_property_values,
            test_revisioned_vector
            ]

    for test_module in test_modules:
        suite.addTest(test_module.suite())
    
    return suite
