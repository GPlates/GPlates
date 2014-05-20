"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GetFeaturePropertiesCase(unittest.TestCase):
    def setUp(self):
        self.feature = pygplates.Feature()
        self.property1 = pygplates.Property(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                pygplates.GpmlPlateId(1))
        self.feature.add(self.property1)
        self.property2 = pygplates.Property(
                pygplates.PropertyName.create_gpml('conjugatePlateId'),
                pygplates.GpmlPlateId(2))
        self.feature.add(self.property2)
        self.property3 = pygplates.Property(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                pygplates.GpmlPlateId(3))
        self.feature.add(self.property3)
        self.property4 = pygplates.Property(
                pygplates.PropertyName.create_gpml('test_integer'),
                pygplates.XsInteger(300))
        self.feature.add(self.property4)
        # A time-dependent property excluding time 0Ma.
        self.property5 =  pygplates.Property(
                pygplates.PropertyName.create_gpml('plateId'),
                pygplates.GpmlPiecewiseAggregation([
                    pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(100), pygplates.GeoTimeInstant(10), pygplates.GeoTimeInstant(5)),
                    pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(101), pygplates.GeoTimeInstant(20), pygplates.GeoTimeInstant(10))]))
        self.feature.add(self.property5)
        self.property6 = pygplates.Property(
                pygplates.PropertyName.create_gpml('position'),
                pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(0,1,0))))
        self.feature.add(self.property6)
    
    def test_get_geometry(self):
        geometry_properties = pygplates.get_feature_geometry_properties(self.feature, pygplates.PointOnSphere)
        self.assertTrue(len(geometry_properties) == 1)
        self.assertTrue(isinstance(geometry_properties[0][1], pygplates.PointOnSphere))
        geometry_properties = pygplates.get_feature_geometry_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('position'), pygplates.PointOnSphere)
        self.assertTrue(len(geometry_properties) == 1)
        self.assertTrue(isinstance(geometry_properties[0][1], pygplates.PointOnSphere))
        self.assertFalse(pygplates.get_feature_geometry_properties(self.feature, pygplates.MultiPointOnSphere))
        self.assertFalse(pygplates.get_feature_geometry_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('position2'), pygplates.PointOnSphere))
    
    def test_get_by_name(self):
        properties = pygplates.get_feature_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('test_integer'))
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0][1].get_integer() == 300)
        properties = pygplates.get_feature_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('reconstructionPlateId'))
        self.assertTrue(len(properties) == 2)
        self.assertTrue(properties[0][1].get_plate_id() in (1,3))
        self.assertTrue(properties[1][1].get_plate_id() in (1,3))
        properties = pygplates.get_feature_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('conjugatePlateId'))
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0][1].get_plate_id() == 2)
        properties = pygplates.get_feature_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('plateId'))
        self.assertFalse(properties)
        properties = pygplates.get_feature_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('plateId'), 5)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0][1].get_plate_id() == 100)
        properties = pygplates.get_feature_properties_by_name(
                self.feature, pygplates.PropertyName.create_gpml('plateId'), 15)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0][1].get_plate_id() == 101)
            # No property name existing in feature.
        self.assertFalse(pygplates.get_feature_properties_by_name(self.feature, pygplates.PropertyName.create_gpml('not_exists')))


class GetGeometryFromPropertyValueCase(unittest.TestCase):
    def setUp(self):
        self.gml_point = pygplates.GmlPoint(pygplates.PointOnSphere(1,0,0))
        self.gpml_constant_value_point = pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(0,1,0)))
        self.gml_multi_point = pygplates.GmlMultiPoint(
            pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)]))
        self.gml_line_string = pygplates.GmlLineString(
            pygplates.PolylineOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)]))
        self.gml_orientable_curve = pygplates.GmlOrientableCurve(pygplates.GmlLineString(
            pygplates.PolylineOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)])))
        self.gpml_constant_value_orientable_curve = pygplates.GpmlConstantValue(pygplates.GmlOrientableCurve(pygplates.GmlLineString(
            pygplates.PolylineOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)]))))
        self.gml_polygon = pygplates.GmlPolygon(
            pygplates.PolygonOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0), pygplates.PointOnSphere(0,0,1)]))
        self.not_geometry = pygplates.GpmlPlateId(101)

    def test_get(self):
        point_on_sphere = pygplates.get_geometry_from_property_value(self.gml_point)
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        point_on_sphere = pygplates.get_geometry_from_property_value(self.gml_point, pygplates.PointOnSphere)
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        point_on_sphere = pygplates.get_geometry_from_property_value(self.gpml_constant_value_point)
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        multi_point_on_sphere = pygplates.get_geometry_from_property_value(self.gml_multi_point)
        self.assertTrue(isinstance(multi_point_on_sphere, pygplates.MultiPointOnSphere))
        multi_point_on_sphere = pygplates.get_geometry_from_property_value(self.gml_multi_point, pygplates.MultiPointOnSphere)
        self.assertTrue(isinstance(multi_point_on_sphere, pygplates.MultiPointOnSphere))
        polyline_on_sphere = pygplates.get_geometry_from_property_value(self.gml_line_string)
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = pygplates.get_geometry_from_property_value(self.gml_line_string, pygplates.PolylineOnSphere)
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = pygplates.get_geometry_from_property_value(self.gml_orientable_curve)
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = pygplates.get_geometry_from_property_value(self.gml_orientable_curve, pygplates.PolylineOnSphere)
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = pygplates.get_geometry_from_property_value(self.gpml_constant_value_orientable_curve)
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = pygplates.get_geometry_from_property_value(self.gpml_constant_value_orientable_curve, pygplates.PolylineOnSphere)
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polygon_on_sphere = pygplates.get_geometry_from_property_value(self.gml_polygon)
        self.assertTrue(isinstance(polygon_on_sphere, pygplates.PolygonOnSphere))
        polygon_on_sphere = pygplates.get_geometry_from_property_value(self.gml_polygon, pygplates.PolygonOnSphere)
        self.assertTrue(isinstance(polygon_on_sphere, pygplates.PolygonOnSphere))
        # Not a geometry.
        self.assertFalse(pygplates.get_geometry_from_property_value(self.not_geometry))
        # A geometry but wrong type.
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gml_point, pygplates.MultiPointOnSphere))
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gpml_constant_value_point, pygplates.MultiPointOnSphere))
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gml_multi_point, pygplates.PointOnSphere))
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gml_line_string, pygplates.PolygonOnSphere))
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gml_orientable_curve, pygplates.PointOnSphere))
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gpml_constant_value_orientable_curve, pygplates.MultiPointOnSphere))
        self.assertFalse(pygplates.get_geometry_from_property_value(self.gml_polygon, pygplates.PolylineOnSphere))


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
        interpolated_gpml_finite_rotation = self.gpml_irregular_sampling.get_value(5)
        self.assertTrue(interpolated_gpml_finite_rotation)
        interpolated_pole, interpolated_angle = interpolated_gpml_finite_rotation.get_finite_rotation().get_euler_pole_and_angle()
        self.assertTrue(abs(interpolated_angle) > 0.322 and abs(interpolated_angle) < 0.323)
        # XsDouble can be interpolated.
        interpolated_double = self.gpml_irregular_sampling2.get_value(pygplates.GeoTimeInstant(7))
        self.assertTrue(interpolated_double)
        self.assertTrue(abs(interpolated_double.get_double() - 140) < 1e-10)
        self.assertTrue(self.gpml_plate_id1.get_value() == self.gpml_plate_id1)
        self.assertTrue(self.gpml_constant_value.get_value() == self.gpml_plate_id1)
        self.assertTrue(self.gpml_piecewise_aggregation.get_value(pygplates.GeoTimeInstant(1.5)) == self.gpml_plate_id2)
        # Outside time range.
        self.assertFalse(self.gpml_irregular_sampling.get_value(20))


class GetTimeSamplesCase(unittest.TestCase):
    def setUp(self):
        self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling([
                pygplates.GpmlTimeSample(pygplates.XsInteger(0), pygplates.GeoTimeInstant(0), 'sample0', False),
                pygplates.GpmlTimeSample(pygplates.XsInteger(1), pygplates.GeoTimeInstant(1), 'sample1'),
                pygplates.GpmlTimeSample(pygplates.XsInteger(2), 2, 'sample2', False),
                pygplates.GpmlTimeSample(pygplates.XsInteger(3), 3, 'sample3')])

    def test_enabled(self):
        enabled_samples = self.gpml_irregular_sampling.get_enabled_time_samples()
        self.assertTrue(len(enabled_samples) == 2)
        self.assertTrue(enabled_samples[0].get_value().get_integer() == 1)
        self.assertTrue(enabled_samples[1].get_value().get_integer() == 3)

    def test_bounding(self):
        bounding_enabled_samples = self.gpml_irregular_sampling.get_time_samples_bounding_time(pygplates.GeoTimeInstant(1.5))
        self.assertTrue(bounding_enabled_samples)
        first_bounding_enabled_sample, second_bounding_enabled_sample = bounding_enabled_samples
        self.assertTrue(first_bounding_enabled_sample.get_value().get_integer() == 3)
        self.assertTrue(second_bounding_enabled_sample.get_value().get_integer() == 1)
        # Time is outside time samples range.
        self.assertFalse(self.gpml_irregular_sampling.get_time_samples_bounding_time(0.5))
        
        bounding_all_samples = self.gpml_irregular_sampling.get_time_samples_bounding_time(1.5, True)
        self.assertTrue(bounding_all_samples)
        first_bounding_sample, second_bounding_sample = bounding_all_samples
        self.assertTrue(first_bounding_sample.get_value().get_integer() == 2)
        self.assertTrue(second_bounding_sample.get_value().get_integer() == 1)
        # Time is now inside time samples range.
        self.assertTrue(self.gpml_irregular_sampling.get_time_samples_bounding_time(0.5, True))
        # Time is outside time samples range.
        self.assertFalse(self.gpml_irregular_sampling.get_time_samples_bounding_time(3.5, True))


class GetTimeWindowsCase(unittest.TestCase):
    def setUp(self):
        self.gpml_piecewise_aggregation = pygplates.GpmlPiecewiseAggregation([
                pygplates.GpmlTimeWindow(pygplates.XsInteger(0), pygplates.GeoTimeInstant(1), pygplates.GeoTimeInstant(0)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(1), pygplates.GeoTimeInstant(2), 1),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(2), 3, pygplates.GeoTimeInstant(2)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(3), 4, 3)])

    def test_get(self):
        time_window = self.gpml_piecewise_aggregation.get_time_window_containing_time(1.5)
        self.assertTrue(time_window)
        self.assertTrue(time_window.get_value().get_integer() == 1)
        # Should be able to specify time using 'GeoTimeInstant' also.
        time_window = self.gpml_piecewise_aggregation.get_time_window_containing_time(pygplates.GeoTimeInstant(2.5))
        self.assertTrue(time_window)
        self.assertTrue(time_window.get_value().get_integer() == 2)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GetFeaturePropertiesCase,
            GetGeometryFromPropertyValueCase,
            GetPropertyValueCase,
            GetTimeSamplesCase,
            GetTimeWindowsCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
