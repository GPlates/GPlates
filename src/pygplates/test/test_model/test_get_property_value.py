"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GetPropertyValueCase(unittest.TestCase):
    def setUp(self):
        self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling(
                [pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                                pygplates.FiniteRotation(
                                        pygplates.PointOnSphere(1,0,0),
                                        0.4)),
                        pygplates.GeoTimeInstant(0)),
                pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                                pygplates.FiniteRotation(
                                        pygplates.PointOnSphere(0,1,0),
                                        0.5)),
                        pygplates.GeoTimeInstant(10))])
        self.gpml_irregular_sampling2 = pygplates.GpmlIrregularSampling(
                [pygplates.GpmlTimeSample(pygplates.XsDouble(100), pygplates.GeoTimeInstant(5)),
                pygplates.GpmlTimeSample( pygplates.XsDouble(200), pygplates.GeoTimeInstant(10))])
        self.gpml_plate_id1 = pygplates.GpmlPlateId(1)
        self.gpml_plate_id2 = pygplates.GpmlPlateId(2)
        self.gpml_constant_value = pygplates.GpmlConstantValue(self.gpml_plate_id1)
        self.gpml_piecewise_aggregation = pygplates.GpmlPiecewiseAggregation([
                pygplates.GpmlTimeWindow(self.gpml_plate_id1, pygplates.GeoTimeInstant(1), pygplates.GeoTimeInstant(0)),
                pygplates.GpmlTimeWindow(self.gpml_plate_id2, pygplates.GeoTimeInstant(2), pygplates.GeoTimeInstant(1))])

    def test_get(self):
        interpolated_gpml_finite_rotation = pygplates.get_property_value(self.gpml_irregular_sampling, 5)
        self.assertTrue(interpolated_gpml_finite_rotation)
        interpolated_pole, interpolated_angle = interpolated_gpml_finite_rotation.get_finite_rotation().get_euler_pole_and_angle()
        self.assertTrue(abs(interpolated_angle) > 0.322 and abs(interpolated_angle) < 0.323)
        # XsDouble can be interpolated.
        interpolated_double = pygplates.get_property_value(self.gpml_irregular_sampling2, 7)
        self.assertTrue(interpolated_double)
        self.assertTrue(abs(interpolated_double.get_double() - 140) < 1e-10)
        self.assertTrue(pygplates.get_property_value(self.gpml_plate_id1) == self.gpml_plate_id1)
        self.assertTrue(pygplates.get_property_value(self.gpml_constant_value) == self.gpml_plate_id1)
        self.assertTrue(pygplates.get_property_value(self.gpml_piecewise_aggregation, 1.5) == self.gpml_plate_id2)
        # Outside time range.
        self.assertFalse(pygplates.get_property_value(self.gpml_irregular_sampling, 20))

    def test_get_by_type(self):
        interpolated_gpml_finite_rotation = pygplates.get_property_value_by_type(
                self.gpml_irregular_sampling, pygplates.GpmlFiniteRotation, 5)
        interpolated_pole, interpolated_angle = interpolated_gpml_finite_rotation.get_finite_rotation().get_euler_pole_and_angle()
        self.assertTrue(abs(interpolated_angle) > 0.322 and abs(interpolated_angle) < 0.323)
        # XsDouble can be interpolated.
        interpolated_double = pygplates.get_property_value_by_type(self.gpml_irregular_sampling2, pygplates.XsDouble, 7)
        self.assertTrue(interpolated_double)
        self.assertTrue(abs(interpolated_double.get_double() - 140) < 1e-10)
        self.assertTrue(pygplates.get_property_value_by_type(self.gpml_plate_id1, pygplates.GpmlPlateId) == self.gpml_plate_id1)
        self.assertTrue(pygplates.get_property_value_by_type(self.gpml_constant_value, pygplates.GpmlPlateId) == self.gpml_plate_id1)
        self.assertTrue(pygplates.get_property_value_by_type(self.gpml_constant_value, pygplates.GpmlConstantValue) == self.gpml_constant_value)
        self.assertTrue(pygplates.get_property_value_by_type(self.gpml_piecewise_aggregation, pygplates.GpmlPlateId))
        self.assertTrue(pygplates.get_property_value_by_type(
                self.gpml_piecewise_aggregation, pygplates.GpmlPlateId, 1.5) == self.gpml_plate_id2)
        self.assertTrue(pygplates.get_property_value_by_type(
                self.gpml_piecewise_aggregation, pygplates.GpmlPlateId, 0.5) == self.gpml_plate_id1)
        self.assertTrue(pygplates.get_property_value_by_type(
                self.gpml_piecewise_aggregation, pygplates.GpmlPiecewiseAggregation) == self.gpml_piecewise_aggregation)
        # Time makes no difference in this case.
        self.assertTrue(pygplates.get_property_value_by_type(
                self.gpml_piecewise_aggregation, pygplates.GpmlPiecewiseAggregation, 0.5) == self.gpml_piecewise_aggregation)
        # Should fail to extract incorrectly specified nested type.
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_irregular_sampling, pygplates.GpmlPlateId))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_irregular_sampling2, pygplates.XsInteger))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_plate_id1, pygplates.GpmlConstantValue))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_plate_id1, pygplates.XsInteger))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_constant_value, pygplates.XsInteger))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_piecewise_aggregation, pygplates.GpmlConstantValue))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_piecewise_aggregation, pygplates.GpmlIrregularSampling))
        self.assertFalse(pygplates.get_property_value_by_type(self.gpml_piecewise_aggregation, pygplates.XsInteger))


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
                pygplates.GpmlTimeWindow(pygplates.XsInteger(0), pygplates.GeoTimeInstant(1), pygplates.GeoTimeInstant(0)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(1), pygplates.GeoTimeInstant(2), pygplates.GeoTimeInstant(1)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(2), pygplates.GeoTimeInstant(3), pygplates.GeoTimeInstant(2))])

    def test_get(self):
        time_window = pygplates.get_time_window_containing_time(self.gpml_piecewise_aggregation, 1.5)
        self.assertTrue(time_window)
        self.assertTrue(time_window.get_value().get_integer() == 1)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GetPropertyValueCase,
            GetTimeSamplesCase,
            GetTimeWindowsCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
