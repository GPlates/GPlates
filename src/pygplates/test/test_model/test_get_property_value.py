"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GetTimeSamplesCase(unittest.TestCase):
    def setUp(self):
        self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling([
                pygplates.GpmlTimeSample(pygplates.XsInteger(0), pygplates.GeoTimeInstant(0), 'sample0', True),
                pygplates.GpmlTimeSample(pygplates.XsInteger(1), pygplates.GeoTimeInstant(1), 'sample1'),
                pygplates.GpmlTimeSample(pygplates.XsInteger(2), pygplates.GeoTimeInstant(2), 'sample2', True),
                pygplates.GpmlTimeSample(pygplates.XsInteger(3), pygplates.GeoTimeInstant(3), 'sample3')])

    def test_enabled(self):
        enabled_samples = pygplates.get_enabled_time_samples(self.gpml_irregular_sampling)
        self.assertTrue(len(enabled_samples) == 2)
        self.assertTrue(enabled_samples[0].get_value().get_integer() == 1)
        self.assertTrue(enabled_samples[1].get_value().get_integer() == 3)

    def test_bounding(self):
        bounding_enabled_samples = pygplates.get_time_samples_bounding_time(self.gpml_irregular_sampling, 1.5)
        self.assertTrue(bounding_enabled_samples)
        first_bounding_enabled_sample, second_bounding_enabled_sample = bounding_enabled_samples
        self.assertTrue(first_bounding_enabled_sample.get_value().get_integer() == 3)
        self.assertTrue(second_bounding_enabled_sample.get_value().get_integer() == 1)
        # Time is outside time samples range.
        self.assertFalse(pygplates.get_time_samples_bounding_time(self.gpml_irregular_sampling, 0.5))
        
        bounding_all_samples = pygplates.get_time_samples_bounding_time(self.gpml_irregular_sampling, 1.5, True)
        self.assertTrue(bounding_all_samples)
        first_bounding_sample, second_bounding_sample = bounding_all_samples
        self.assertTrue(first_bounding_sample.get_value().get_integer() == 2)
        self.assertTrue(second_bounding_sample.get_value().get_integer() == 1)
        # Time is now inside time samples range.
        self.assertTrue(pygplates.get_time_samples_bounding_time(self.gpml_irregular_sampling, 0.5, True))
        # Time is outside time samples range.
        self.assertFalse(pygplates.get_time_samples_bounding_time(self.gpml_irregular_sampling, 3.5, True))


class GetTimeWindowsCase(unittest.TestCase):
    def setUp(self):
        self.gpml_piecewise_aggregation = pygplates.GpmlPiecewiseAggregation([
                pygplates.GpmlTimeWindow(pygplates.XsInteger(0), pygplates.GeoTimeInstant(0), pygplates.GeoTimeInstant(1)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(1), pygplates.GeoTimeInstant(1), pygplates.GeoTimeInstant(2)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(2), pygplates.GeoTimeInstant(2), pygplates.GeoTimeInstant(3))])

    def test_get(self):
        time_window = pygplates.get_time_window_containing_time(self.gpml_piecewise_aggregation, 1.5)
        self.assertTrue(time_window)
        self.assertTrue(time_window.get_value().get_integer() == 1)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GetTimeSamplesCase,
            GetTimeWindowsCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
