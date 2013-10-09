"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GeoTimeInstantCase(unittest.TestCase):
    def setUp(self):
        self.distant_past = pygplates.GeoTimeInstant.create_distant_past()
        self.distant_future = pygplates.GeoTimeInstant.create_distant_future()
        self.real_time1 = pygplates.GeoTimeInstant(10.5)
        self.real_time2 = pygplates.GeoTimeInstant(5.5)

    def test_distant_past(self):
        self.assertTrue(self.distant_past.is_distant_past())
        self.assertFalse(self.distant_past.is_distant_future())
        self.assertFalse(self.distant_past.is_real())

    def test_distant_future(self):
        self.assertTrue(self.distant_future.is_distant_future())
        self.assertFalse(self.distant_future.is_distant_past())
        self.assertFalse(self.distant_past.is_real())

    def test_real_time(self):
        self.assertFalse(self.real_time1.is_distant_past())
        self.assertFalse(self.real_time1.is_distant_future())
        self.assertTrue(self.real_time1.is_real())

    def test_comparisons(self):
        self.assertTrue(self.distant_past < self.distant_future)
        self.assertTrue(self.distant_past <= self.distant_future)
        self.assertTrue(self.distant_past != self.distant_future)
        
        self.assertTrue(self.distant_future > self.distant_past)
        self.assertTrue(self.distant_future >= self.distant_past)
        self.assertTrue(self.distant_future != self.distant_past)
        
        self.assertTrue(self.real_time1 < self.distant_future)
        self.assertTrue(self.real_time1 <= self.distant_future)
        self.assertTrue(self.real_time1 != self.distant_future)
        
        self.assertTrue(self.real_time1 > self.distant_past)
        self.assertTrue(self.real_time1 >= self.distant_past)
        self.assertTrue(self.real_time1 != self.distant_past)
        
        self.assertTrue(self.real_time1 < self.real_time2)
        self.assertTrue(self.real_time1 <= self.real_time2)
        self.assertTrue(self.real_time1 != self.real_time2)
        
        self.assertTrue(self.real_time1 == self.real_time1)
        self.assertTrue(self.real_time1 <= self.real_time1)
        self.assertTrue(self.real_time1 >= self.real_time1)
        
        # Note that, unlike the above comparisons on GeoTimeInstant objects, these comparisons are
        # on floating-point values (hence the reversal of the comparisons). Also we shouldn't really compare
        # with distant-past and distant-future since they return +Inf and -Inf respectively (and the user
        # should not call 'get_value()' in either case, but we test anyway just to make sure the comparison
        # holds in case the user actually does do this.
        self.assertTrue(self.real_time1.get_value() > self.real_time2.get_value())
        self.assertTrue(self.real_time1.get_value() > self.distant_future.get_value())
        self.assertTrue(self.real_time1.get_value() < self.distant_past.get_value())
        self.assertTrue(self.distant_future.get_value() < self.distant_past.get_value())


class GmlLineStringCase(unittest.TestCase):
    def setUp(self):
        self.polyline = pygplates.PolylineOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        self.gml_line_string = pygplates.GmlLineString(self.polyline)

    def test_get(self):
        self.assertTrue(self.gml_line_string.get_polyline() == self.polyline)

    def test_set(self):
        new_polyline = pygplates.PolylineOnSphere(
                [pygplates.PointOnSphere(-1, 0, 0),
                pygplates.PointOnSphere(0, -1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        self.gml_line_string.set_polyline(new_polyline)
        self.assertTrue(self.gml_line_string.get_polyline() == new_polyline)


class GmlMultiPointCase(unittest.TestCase):
    def setUp(self):
        self.multi_point = pygplates.MultiPointOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        self.gml_multi_point = pygplates.GmlMultiPoint(self.multi_point)

    def test_get(self):
        self.assertTrue(self.gml_multi_point.get_multi_point() == self.multi_point)

    def test_set(self):
        new_multi_point = pygplates.MultiPointOnSphere(
                [pygplates.PointOnSphere(-1, 0, 0), pygplates.PointOnSphere(0, -1, 0)])
        self.gml_multi_point.set_multi_point(new_multi_point)
        self.assertTrue(self.gml_multi_point.get_multi_point() == new_multi_point)


class GmlOrientableCurveCase(unittest.TestCase):
    def setUp(self):
        self.polyline = pygplates.PolylineOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        self.gml_line_string = pygplates.GmlLineString(self.polyline)
        self.gml_orientable_curve = pygplates.GmlOrientableCurve(self.gml_line_string)

    def test_get(self):
        self.assertTrue(self.gml_orientable_curve.get_base_curve() == self.gml_line_string)

    def test_set(self):
        new_polyline = pygplates.PolylineOnSphere(
                [pygplates.PointOnSphere(-1, 0, 0),
                pygplates.PointOnSphere(0, -1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        new_gml_line_string = pygplates.GmlLineString(new_polyline)
        self.gml_orientable_curve.set_base_curve(new_gml_line_string)
        self.assertTrue(self.gml_orientable_curve.get_base_curve() == new_gml_line_string)


class GmlPointCase(unittest.TestCase):
    def setUp(self):
        self.point = pygplates.PointOnSphere(1,0,0)
        self.gml_point = pygplates.GmlPoint(self.point)

    def test_get(self):
        self.assertTrue(self.gml_point.get_point() == self.point)

    def test_set(self):
        new_point = pygplates.PointOnSphere(0,0,1)
        self.gml_point.set_point(new_point)
        self.assertTrue(self.gml_point.get_point() == new_point)


class GmlPolygonCase(unittest.TestCase):
    def setUp(self):
        self.polygon = pygplates.PolygonOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        self.gml_polygon = pygplates.GmlPolygon(self.polygon)

    def test_get(self):
        self.assertTrue(self.gml_polygon.get_polygon() == self.polygon)

    def test_set(self):
        new_polygon = pygplates.PolygonOnSphere(
                [pygplates.PointOnSphere(-1, 0, 0),
                pygplates.PointOnSphere(0, -1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        self.gml_polygon.set_polygon(new_polygon)
        self.assertTrue(self.gml_polygon.get_polygon() == new_polygon)


class GmlTimeInstantCase(unittest.TestCase):
    def setUp(self):
        self.geo_time_instant = pygplates.GeoTimeInstant(10)
        self.gml_time_instant = pygplates.GmlTimeInstant(self.geo_time_instant)

    def test_get(self):
        self.assertTrue(self.gml_time_instant.get_time() == self.geo_time_instant)

    def test_set(self):
        new_geo_time_instant = pygplates.GeoTimeInstant(20)
        self.gml_time_instant.set_time(new_geo_time_instant)
        self.assertTrue(self.gml_time_instant.get_time() == new_geo_time_instant)


class GmlTimePeriodCase(unittest.TestCase):
    def setUp(self):
        self.begin_geo_time_instant = pygplates.GeoTimeInstant(20)
        self.end_geo_time_instant = pygplates.GeoTimeInstant.create_distant_future()
        self.gml_time_period = pygplates.GmlTimePeriod(
                self.begin_geo_time_instant, self.end_geo_time_instant)

    def test_get(self):
        self.assertTrue(self.gml_time_period.get_begin_time() == self.begin_geo_time_instant)
        self.assertTrue(self.gml_time_period.get_end_time() == self.end_geo_time_instant)

    def test_set(self):
        new_begin_geo_time_instant = pygplates.GeoTimeInstant(5)
        new_end_geo_time_instant = pygplates.GeoTimeInstant(1.5)
        self.gml_time_period.set_begin_time(new_begin_geo_time_instant)
        self.assertTrue(self.gml_time_period.get_begin_time() == new_begin_geo_time_instant)
        self.assertTrue(self.gml_time_period.get_end_time() == self.end_geo_time_instant)
        self.gml_time_period.set_end_time(new_end_geo_time_instant)
        self.assertTrue(self.gml_time_period.get_begin_time() == new_begin_geo_time_instant)
        self.assertTrue(self.gml_time_period.get_end_time() == new_end_geo_time_instant)
        
        # Violate the begin/end time class invariant.
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                self.gml_time_period.set_begin_time, pygplates.GeoTimeInstant(1.0))
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                self.gml_time_period.set_end_time, pygplates.GeoTimeInstant(5.5))


class GpmlConstantValueCase(unittest.TestCase):
    def setUp(self):
        self.property_value1 = pygplates.GpmlPlateId(701)
        self.description1 = 'A plate id'
        self.gpml_constant_value1 = pygplates.GpmlConstantValue(self.property_value1, self.description1)
        
        self.property_value2 = pygplates.GpmlPlateId(201)
        self.gpml_constant_value2 = pygplates.GpmlConstantValue(self.property_value2)
        
        # Create a property value in a constant value wrapper but then lose the reference to the wrapper
        # so that only the nested property value exists (this is to test that modifying the property value's
        # state doesn't crash due to a dangling parent reference to the non-existent parent constant value wrapper).
        self.property_value3 = pygplates.GpmlPlateId(201)
        # Note there's no 'self.' on the constant value wrapper.
        gpml_constant_value2 = pygplates.GpmlConstantValue(self.property_value3)
        del gpml_constant_value2 # 'del' doesn't really guarantee anything though.

    def test_get(self):
        self.assertTrue(self.gpml_constant_value1.get_value() == self.property_value1)
        self.assertTrue(self.gpml_constant_value1.get_description() == self.description1)
        self.assertTrue(self.gpml_constant_value2.get_value() == self.property_value2)
        self.assertTrue(self.gpml_constant_value2.get_description() is None)

    def test_set(self):
        new_property_value = pygplates.GpmlPlateId(801)
        self.gpml_constant_value1.set_value(new_property_value)
        self.assertTrue(self.gpml_constant_value1.get_value() == new_property_value)
        new_description = 'Another plate id'
        self.gpml_constant_value1.set_description(new_description)
        self.assertTrue(self.gpml_constant_value1.get_description() == new_description)
        self.gpml_constant_value1.set_description()
        self.assertTrue(self.gpml_constant_value1.get_description() is None)
        new_description2 = 'Yet another plate id'
        self.gpml_constant_value1.set_description(new_description2)
        self.assertTrue(self.gpml_constant_value1.get_description() == new_description2)
        
        # Test that this does not crash due to dangling parent reference to non-existent constant value wrapper.
        self.property_value3.set_plate_id(101)


class GpmlFiniteRotationCase(unittest.TestCase):
    def setUp(self):
        self.finite_rotation = pygplates.FiniteRotation(pygplates.PointOnSphere(1,0,0), 0.3)
        self.gpml_finite_rotation = pygplates.GpmlFiniteRotation(self.finite_rotation)

    def test_get(self):
        self.assertTrue(pygplates.represent_equivalent_rotations(
                self.gpml_finite_rotation.get_finite_rotation(), self.finite_rotation))

    def test_set(self):
        new_finite_rotation = pygplates.FiniteRotation(pygplates.PointOnSphere(0,1,0), 0.4)
        self.gpml_finite_rotation.set_finite_rotation(new_finite_rotation)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                self.gpml_finite_rotation.get_finite_rotation(), new_finite_rotation))


class GpmlFiniteRotationSlerpCase(unittest.TestCase):
    def test_create(self):
        gpml_finite_rotation_slerp = pygplates.GpmlFiniteRotationSlerp()
        self.assertTrue(isinstance(gpml_finite_rotation_slerp, pygplates.GpmlFiniteRotationSlerp))
        self.assertTrue(isinstance(gpml_finite_rotation_slerp, pygplates.GpmlInterpolationFunction))


class GpmlIrregularSamplingCase(unittest.TestCase):
    """Test GpmlIrregularSampling.
    
    NOTE: The testing of the time sample list (GpmlTimeSampleList) is handled in 'test_revisioned_vector.py'."""
    
    def setUp(self):
        self.original_time_samples = []
        for i in range(0,4):
            ts = pygplates.GpmlTimeSample(
                pygplates.XsInteger(i),
                pygplates.GeoTimeInstant(i))
            self.original_time_samples.append(ts)
        
        self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling(
                self.original_time_samples,
                pygplates.GpmlFiniteRotationSlerp())

    def test_get(self):
        self.assertTrue(isinstance(self.gpml_irregular_sampling.get_interpolation_function(), pygplates.GpmlInterpolationFunction))
        self.assertTrue(list(self.gpml_irregular_sampling.get_time_samples()) == self.original_time_samples)

    def test_set(self):
        gpml_time_sample_list = self.gpml_irregular_sampling.get_time_samples()
        reversed_time_samples = list(reversed(self.original_time_samples))
        gpml_time_sample_list[:] = reversed_time_samples
        self.assertTrue(list(self.gpml_irregular_sampling.get_time_samples()) == reversed_time_samples)
        
        self.gpml_irregular_sampling.set_interpolation_function()
        self.assertTrue(self.gpml_irregular_sampling.get_interpolation_function() is None)
        interpolation_func = pygplates.GpmlFiniteRotationSlerp()
        self.gpml_irregular_sampling.set_interpolation_function(interpolation_func)
        self.assertTrue(self.gpml_irregular_sampling.get_interpolation_function() == interpolation_func)
        
        # Need at least one time sample to create a GpmlIrregularSampling.
        self.assertRaises(RuntimeError, pygplates.GpmlIrregularSampling, [])


class GpmlPiecewiseAggregationCase(unittest.TestCase):
    """Test GpmlPiecewiseAggregation.
    
    NOTE: The testing of the time window list (GpmlTimeWindowList) is handled in 'test_revisioned_vector.py'."""
    
    def setUp(self):
        self.original_time_windows = []
        for i in range(0,4):
            tw = pygplates.GpmlTimeWindow(
                pygplates.XsInteger(i),
                pygplates.GeoTimeInstant(i+1), # begin time - earlier
                pygplates.GeoTimeInstant(i))   # end time - later
            self.original_time_windows.append(tw)
        
        self.gpml_piecewise_aggregation = pygplates.GpmlPiecewiseAggregation(self.original_time_windows)

    def test_get(self):
        self.assertTrue(list(self.gpml_piecewise_aggregation.get_time_windows()) == self.original_time_windows)

    def test_set(self):
        gpml_time_window_list = self.gpml_piecewise_aggregation.get_time_windows()
        reversed_time_windows = list(reversed(self.original_time_windows))
        gpml_time_window_list[:] = reversed_time_windows
        self.assertTrue(list(self.gpml_piecewise_aggregation.get_time_windows()) == reversed_time_windows)
        
        # Need at least one time sample to create a GpmlPiecewiseAggregation.
        self.assertRaises(RuntimeError, pygplates.GpmlPiecewiseAggregation, [])


class GpmlPlateIdCase(unittest.TestCase):
    def setUp(self):
        self.plate_id = 701
        self.gpml_plate_id = pygplates.GpmlPlateId(self.plate_id)

    def test_get(self):
        self.assertTrue(self.gpml_plate_id.get_plate_id() == self.plate_id)

    def test_set(self):
        new_plate_id = 201
        self.gpml_plate_id.set_plate_id(new_plate_id)
        self.assertTrue(self.gpml_plate_id.get_plate_id() == new_plate_id)


class GpmlTimeSampleCase(unittest.TestCase):
    def setUp(self):
        self.property_value1 = pygplates.GpmlPlateId(701)
        self.time1 = pygplates.GeoTimeInstant(10)
        self.description1 = 'A plate id time sample'
        self.is_disabled1 = True
        self.gpml_time_sample1 = pygplates.GpmlTimeSample(
                self.property_value1, self.time1, self.description1, self.is_disabled1)
        
        self.property_value2 = pygplates.GpmlPlateId(201)
        self.time2 = pygplates.GeoTimeInstant(20)
        self.gpml_time_sample2 = pygplates.GpmlTimeSample(self.property_value2, self.time2)

    def test_get(self):
        self.assertTrue(self.gpml_time_sample1.get_value() == self.property_value1)
        self.assertTrue(self.gpml_time_sample1.get_time() == self.time1)
        self.assertTrue(self.gpml_time_sample1.get_description() == self.description1)
        self.assertTrue(self.gpml_time_sample1.is_disabled())
        
        self.assertTrue(self.gpml_time_sample2.get_value() == self.property_value2)
        self.assertTrue(self.gpml_time_sample2.get_time() == self.time2)
        self.assertTrue(self.gpml_time_sample2.get_description() is None)
        self.assertTrue(not self.gpml_time_sample2.is_disabled())

    def test_set(self):
        new_property_value = pygplates.GpmlPlateId(801)
        new_time = pygplates.GeoTimeInstant(30)
        new_description = 'Another plate id time sample'
        new_is_disabled = False
        self.gpml_time_sample1.set_value(new_property_value)
        self.gpml_time_sample1.set_time(new_time)
        self.gpml_time_sample1.set_description(new_description)
        self.gpml_time_sample1.set_disabled(new_is_disabled)
        self.assertTrue(self.gpml_time_sample1.get_value() == new_property_value)
        self.assertTrue(self.gpml_time_sample1.get_time() == new_time)
        self.assertTrue(self.gpml_time_sample1.get_description() == new_description)
        self.assertTrue(self.gpml_time_sample1.is_disabled() == new_is_disabled)
        
        self.gpml_time_sample1.set_description()
        self.assertTrue(self.gpml_time_sample1.get_description() is None)
        new_description2 = 'Yet another plate id time sample'
        self.gpml_time_sample1.set_description(new_description2)
        self.assertTrue(self.gpml_time_sample1.get_description() == new_description2)

    def test_comparison(self):
        self.assertTrue(self.gpml_time_sample1 == self.gpml_time_sample1)
        self.assertTrue(self.gpml_time_sample1 != self.gpml_time_sample2)
        
        self.gpml_time_sample1.set_time(self.gpml_time_sample2.get_time())
        self.gpml_time_sample1.set_description(self.gpml_time_sample2.get_description())
        self.gpml_time_sample1.set_disabled(self.gpml_time_sample2.is_disabled())
        # Different property value instance but same value (plate id).
        self.gpml_time_sample1.get_value().set_plate_id(self.gpml_time_sample2.get_value().get_plate_id())
        self.assertTrue(self.gpml_time_sample1 == self.gpml_time_sample2)
        
        # Same property value instance.
        self.gpml_time_sample1.set_value(self.gpml_time_sample2.get_value())
        self.assertTrue(self.gpml_time_sample1 == self.gpml_time_sample2)


class GpmlTimeWindowCase(unittest.TestCase):
    def setUp(self):
        self.property_value = pygplates.GpmlPlateId(701)
        self.begin_time = pygplates.GeoTimeInstant(20)
        self.end_time = pygplates.GeoTimeInstant(10)
        self.gpml_time_window = pygplates.GpmlTimeWindow(
                self.property_value, self.begin_time, self.end_time)

    def test_get(self):
        self.assertTrue(self.gpml_time_window.get_value() == self.property_value)
        self.assertTrue(self.gpml_time_window.get_begin_time() == self.begin_time)
        self.assertTrue(self.gpml_time_window.get_end_time() == self.end_time)

    def test_set(self):
        new_property_value = pygplates.GpmlPlateId(801)
        new_begin_time = pygplates.GeoTimeInstant(40)
        new_end_time = pygplates.GeoTimeInstant(30)
        self.gpml_time_window.set_value(new_property_value)
        self.gpml_time_window.set_begin_time(new_begin_time)
        self.gpml_time_window.set_end_time(new_end_time)
        self.assertTrue(self.gpml_time_window.get_value() == new_property_value)
        self.assertTrue(self.gpml_time_window.get_begin_time() == new_begin_time)
        self.assertTrue(self.gpml_time_window.get_end_time() == new_end_time)

    def test_comparison(self):
        self.assertTrue(self.gpml_time_window == self.gpml_time_window)
        
        self.gpml_time_window2 = pygplates.GpmlTimeWindow(
                pygplates.GpmlPlateId(201), pygplates.GeoTimeInstant(40), pygplates.GeoTimeInstant(30))
        self.assertTrue(self.gpml_time_window != self.gpml_time_window2)
        
        self.gpml_time_window.set_begin_time(self.gpml_time_window2.get_begin_time())
        self.gpml_time_window.set_end_time(self.gpml_time_window2.get_end_time())
        # Different property value instance but same value (plate id).
        self.gpml_time_window.get_value().set_plate_id(self.gpml_time_window2.get_value().get_plate_id())
        self.assertTrue(self.gpml_time_window == self.gpml_time_window2)
        
        # Same property value instance.
        self.gpml_time_window.set_value(self.gpml_time_window2.get_value())
        self.assertTrue(self.gpml_time_window == self.gpml_time_window2)


class XsBooleanCase(unittest.TestCase):
    def setUp(self):
        self.boolean = True
        self.xs_boolean = pygplates.XsBoolean(self.boolean)

    def test_get(self):
        self.assertTrue(self.xs_boolean.get_boolean() == self.boolean)

    def test_set(self):
        new_boolean = False
        self.xs_boolean.set_boolean(new_boolean)
        self.assertTrue(self.xs_boolean.get_boolean() == new_boolean)


class XsDoubleCase(unittest.TestCase):
    def setUp(self):
        self.double = 10.5
        self.xs_double = pygplates.XsDouble(self.double)

    def test_get(self):
        self.assertTrue(self.xs_double.get_double() == self.double)

    def test_set(self):
        new_double = -100.2
        self.xs_double.set_double(new_double)
        self.assertTrue(self.xs_double.get_double() == new_double)


class XsIntegerCase(unittest.TestCase):
    def setUp(self):
        self.integer = 10
        self.xs_integer = pygplates.XsInteger(self.integer)

    def test_get(self):
        self.assertTrue(self.xs_integer.get_integer() == self.integer)

    def test_set(self):
        new_integer = -100
        self.xs_integer.set_integer(new_integer)
        self.assertTrue(self.xs_integer.get_integer() == new_integer)


class XsStringCase(unittest.TestCase):
    def setUp(self):
        self.string = 'test-string'
        self.xs_string = pygplates.XsString(self.string)

    def test_get(self):
        self.assertTrue(self.xs_string.get_string() == self.string)

    def test_set(self):
        new_string = 'another-string-for-testing'
        self.xs_string.set_string(new_string)
        self.assertTrue(self.xs_string.get_string() == new_string)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GeoTimeInstantCase,
            GmlLineStringCase,
            GmlMultiPointCase,
            GmlOrientableCurveCase,
            GmlPointCase,
            GmlPolygonCase,
            GmlTimeInstantCase,
            GmlTimePeriodCase,
            GpmlConstantValueCase,
            GpmlFiniteRotationCase,
            GpmlFiniteRotationSlerpCase,
            GpmlIrregularSamplingCase,
            GpmlPiecewiseAggregationCase,
            GpmlPlateIdCase,
            GpmlTimeSampleCase,
            GpmlTimeWindowCase,
            XsBooleanCase,
            XsDoubleCase,
            XsIntegerCase,
            XsStringCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
