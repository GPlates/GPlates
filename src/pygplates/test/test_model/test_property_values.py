"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class PropertyValueCase(unittest.TestCase):
    def test_clone(self):
        plate_id = 701
        gpml_plate_id = pygplates.GpmlPlateId(plate_id)
        self.assertTrue(gpml_plate_id.clone().get_plate_id() == plate_id)
        
        gpml_array = pygplates.GpmlArray([pygplates.XsInteger(1), pygplates.XsInteger(2)])
        gpml_array_clone = gpml_array.clone()
        self.assertTrue(gpml_array_clone == pygplates.GpmlArray([pygplates.XsInteger(1), pygplates.XsInteger(2)]))
        # Modify original and make sure clone is not affected.
        gpml_array[0] = pygplates.XsInteger(100)
        self.assertTrue(gpml_array == pygplates.GpmlArray([pygplates.XsInteger(100), pygplates.XsInteger(2)]))
        self.assertTrue(gpml_array_clone == pygplates.GpmlArray([pygplates.XsInteger(1), pygplates.XsInteger(2)]))
        del gpml_array[0]
        self.assertTrue(gpml_array == pygplates.GpmlArray([pygplates.XsInteger(2)]))
        self.assertTrue(gpml_array_clone == pygplates.GpmlArray([pygplates.XsInteger(1), pygplates.XsInteger(2)]))

    def test_get_geometry(self):
        polyline = pygplates.PolylineOnSphere([(0,0), (10,0), (10,10)])
        gml_line_string = pygplates.GpmlConstantValue(pygplates.GmlLineString(polyline))
        # Using 'clone()' ensures C++ code returns a base PropertyValue
        # (which boost-python converts to derived).
        self.assertTrue(gml_line_string.clone().get_geometry() == polyline)


class EnumerationCase(unittest.TestCase):
    def setUp(self):
        self.enum_type = pygplates.EnumerationType.create_gpml('DipSlipEnumeration')
        self.enum_content = 'Extension'
        self.enumeration = pygplates.Enumeration(self.enum_type, self.enum_content)

    def test_get_type(self):
        self.assertTrue(self.enumeration.get_type() == self.enum_type)

    def test_get_content(self):
        self.assertTrue(self.enumeration.get_content() == self.enum_content)

    def test_set_content(self):
        new_content = 'Compression'
        self.enumeration.set_content(new_content)
        self.assertTrue(self.enumeration.get_content() == new_content)

    def test_information_model(self):
        unknown_enum_type = pygplates.EnumerationType.create_gpml('UnknownEnumType')
        unknown_enum_content = 'UnknownContent'
        self.assertRaises(pygplates.InformationModelError, pygplates.Enumeration, unknown_enum_type, 'Extension')
        new_enumeration = pygplates.Enumeration(unknown_enum_type, unknown_enum_content, pygplates.VerifyInformationModel.no)
        self.assertTrue(new_enumeration.get_type() == unknown_enum_type)
        self.assertTrue(new_enumeration.get_content() == unknown_enum_content)
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Enumeration.set_content, self.enumeration, unknown_enum_content)
        self.enumeration.set_content(unknown_enum_content, pygplates.VerifyInformationModel.no)
        self.assertTrue(self.enumeration.get_content() == unknown_enum_content)


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
        self.assertTrue(pygplates.GeoTimeInstant(float('inf')).is_distant_past())

    def test_distant_future(self):
        self.assertTrue(self.distant_future.is_distant_future())
        self.assertFalse(self.distant_future.is_distant_past())
        self.assertFalse(self.distant_past.is_real())
        self.assertTrue(pygplates.GeoTimeInstant(float('-inf')).is_distant_future())

    def test_real_time(self):
        self.assertFalse(self.real_time1.is_distant_past())
        self.assertFalse(self.real_time1.is_distant_future())
        self.assertTrue(self.real_time1.is_real())

    def test_comparisons(self):
        self.assertTrue(self.distant_past > self.distant_future)
        self.assertTrue(self.distant_past.get_value() > self.distant_future)
        self.assertTrue(self.distant_past > self.distant_future.get_value())
        self.assertTrue(self.distant_past.get_value() > self.distant_future.get_value())
        
        self.assertTrue(self.distant_past >= self.distant_future)
        self.assertTrue(self.distant_past.get_value() >= self.distant_future)
        self.assertTrue(self.distant_past >= self.distant_future.get_value())
        self.assertTrue(self.distant_past.get_value() >= self.distant_future.get_value())
        
        self.assertTrue(self.distant_past != self.distant_future)
        self.assertTrue(self.distant_past.get_value() != self.distant_future)
        self.assertTrue(self.distant_past != self.distant_future.get_value())
        self.assertTrue(self.distant_past.get_value() != self.distant_future.get_value())
        
        self.assertTrue(self.distant_future < self.distant_past)
        self.assertTrue(self.distant_future.get_value() < self.distant_past)
        self.assertTrue(self.distant_future < self.distant_past.get_value())
        self.assertTrue(self.distant_future.get_value() < self.distant_past.get_value())
        
        self.assertTrue(self.distant_future <= self.distant_past)
        self.assertTrue(self.distant_future.get_value() <= self.distant_past)
        self.assertTrue(self.distant_future <= self.distant_past.get_value())
        self.assertTrue(self.distant_future.get_value() <= self.distant_past.get_value())
        
        self.assertTrue(self.distant_future != self.distant_past)
        self.assertTrue(self.distant_future.get_value() != self.distant_past)
        self.assertTrue(self.distant_future != self.distant_past.get_value())
        self.assertTrue(self.distant_future.get_value() != self.distant_past.get_value())
        
        self.assertTrue(self.real_time1 > self.distant_future)
        self.assertTrue(self.real_time1.get_value() > self.distant_future)
        self.assertTrue(self.real_time1 > self.distant_future.get_value())
        self.assertTrue(self.real_time1.get_value() > self.distant_future.get_value())
        
        self.assertTrue(self.real_time1 >= self.distant_future)
        self.assertTrue(self.real_time1.get_value() >= self.distant_future)
        self.assertTrue(self.real_time1 >= self.distant_future.get_value())
        self.assertTrue(self.real_time1.get_value() >= self.distant_future.get_value())

        self.assertTrue(self.real_time1 != self.distant_future)
        self.assertTrue(self.real_time1.get_value() != self.distant_future)
        self.assertTrue(self.real_time1 != self.distant_future.get_value())
        self.assertTrue(self.real_time1.get_value() != self.distant_future.get_value())
        
        self.assertTrue(self.real_time1 < self.distant_past)
        self.assertTrue(self.real_time1.get_value() < self.distant_past)
        self.assertTrue(self.real_time1 < self.distant_past.get_value())
        self.assertTrue(self.real_time1.get_value() < self.distant_past.get_value())
        
        self.assertTrue(self.real_time1 <= self.distant_past)
        self.assertTrue(self.real_time1.get_value() <= self.distant_past)
        self.assertTrue(self.real_time1 <= self.distant_past.get_value())
        self.assertTrue(self.real_time1.get_value() <= self.distant_past.get_value())
        
        self.assertTrue(self.real_time1 != self.distant_past)
        self.assertTrue(self.real_time1.get_value() != self.distant_past)
        self.assertTrue(self.real_time1 != self.distant_past.get_value())
        self.assertTrue(self.real_time1.get_value() != self.distant_past.get_value())
        
        self.assertTrue(self.real_time1 > self.real_time2)
        self.assertTrue(self.real_time1.get_value() > self.real_time2)
        self.assertTrue(self.real_time1 > self.real_time2.get_value())
        self.assertTrue(self.real_time1.get_value() > self.real_time2.get_value())
        
        self.assertTrue(self.real_time1 >= self.real_time2)
        self.assertTrue(self.real_time1.get_value() >= self.real_time2)
        self.assertTrue(self.real_time1 >= self.real_time2.get_value())
        self.assertTrue(self.real_time1.get_value() >= self.real_time2.get_value())
        
        self.assertTrue(self.real_time1 != self.real_time2)
        self.assertTrue(self.real_time1.get_value() != self.real_time2)
        self.assertTrue(self.real_time1 != self.real_time2.get_value())
        self.assertTrue(self.real_time1.get_value() != self.real_time2.get_value())
        
        self.assertTrue(self.real_time1 == self.real_time1)
        self.assertTrue(self.real_time1.get_value() == self.real_time1)
        self.assertTrue(self.real_time1 == self.real_time1.get_value())
        self.assertTrue(self.real_time1.get_value() == self.real_time1.get_value())
        
        self.assertTrue(self.real_time1 <= self.real_time1)
        self.assertTrue(self.real_time1.get_value() <= self.real_time1)
        self.assertTrue(self.real_time1 <= self.real_time1.get_value())
        self.assertTrue(self.real_time1.get_value() <= self.real_time1.get_value())
        
        self.assertTrue(self.real_time1 >= self.real_time1)
        self.assertTrue(self.real_time1.get_value() >= self.real_time1)
        self.assertTrue(self.real_time1 >= self.real_time1.get_value())
        self.assertTrue(self.real_time1.get_value() >= self.real_time1.get_value())


class GmlDataBlockCase(unittest.TestCase):
    def setUp(self):
        self.velocity_colat_type = pygplates.ScalarType.create_gpml('VelocityColat')
        self.velocity_lon_type = pygplates.ScalarType.create_gpml('VelocityLon')
        self.velocity_colat_values = [1,2,3,4]
        self.velocity_lon_values = [10,20,30,40]
        self.gml_data_block = pygplates.GmlDataBlock([
                (self.velocity_colat_type, self.velocity_colat_values),
                (self.velocity_lon_type, self.velocity_lon_values)])

    def test_construct_from_dict(self):
        self.assertRaises(ValueError, pygplates.GmlDataBlock, [])
        self.assertRaises(ValueError, pygplates.GmlDataBlock, {})
        self.assertRaises(ValueError, pygplates.GmlDataBlock,
                {self.velocity_colat_type : [1,2,3], self.velocity_lon_type : [1,2,3,4]})
        gml_data_block = pygplates.GmlDataBlock(
                {self.velocity_colat_type : self.velocity_colat_values, self.velocity_colat_type : self.velocity_colat_values})
        self.assertTrue(len(gml_data_block) == 1)
        gml_data_block = pygplates.GmlDataBlock(dict([
                (self.velocity_colat_type, self.velocity_colat_values),
                (self.velocity_lon_type, self.velocity_lon_values)]))
        self.assertTrue(len(gml_data_block) == 2)
        self.assertTrue(gml_data_block.get_scalar_values(self.velocity_colat_type) == self.velocity_colat_values)
        self.assertTrue(gml_data_block.get_scalar_values(self.velocity_lon_type) == self.velocity_lon_values)

    def test_len(self):
        self.assertTrue(len(self.gml_data_block) == 2)

    def test_contains(self):
        self.assertTrue(self.velocity_colat_type in self.gml_data_block)
        self.assertTrue(self.velocity_lon_type in self.gml_data_block)
        self.assertTrue(pygplates.ScalarType.create_gpml('UnknownType') not in self.gml_data_block)

    def test_iter(self):
        self.assertTrue(len(self.gml_data_block) == 2)
        count = 0
        for scalar_type in self.gml_data_block:
            count += 1
            self.assertTrue(scalar_type in self.gml_data_block)
            self.assertTrue(scalar_type in [self.velocity_colat_type, self.velocity_lon_type])
        self.assertTrue(count == 2)

    def test_get(self):
        self.assertTrue(self.gml_data_block.get_scalar_values(self.velocity_colat_type) == self.velocity_colat_values)
        self.assertTrue(self.gml_data_block.get_scalar_values(self.velocity_lon_type) == self.velocity_lon_values)

    def test_set(self):
        # Override existing value.
        self.assertTrue(len(self.gml_data_block) == 2)
        self.gml_data_block.set(self.velocity_colat_type, [-1,-2,-3,-4])
        self.assertTrue(len(self.gml_data_block) == 2)
        self.assertTrue(self.gml_data_block.get_scalar_values(self.velocity_colat_type) == [-1,-2,-3,-4])
        self.assertTrue(len(self.gml_data_block) == 2)
        self.assertRaises(ValueError, self.gml_data_block.set, self.velocity_colat_type, [-1,-2,-3])
        self.assertTrue(len(self.gml_data_block) == 2)
        self.gml_data_block.set(pygplates.ScalarType.create_gpml('TestType'), [100,200,300,400])
        self.assertTrue(len(self.gml_data_block) == 3)
        self.assertTrue(self.gml_data_block.get_scalar_values(pygplates.ScalarType.create_gpml('TestType')) == [100,200,300,400])

    def test_remove(self):
        self.assertTrue(len(self.gml_data_block) == 2)
        self.assertTrue(self.velocity_colat_type in self.gml_data_block)
        self.gml_data_block.remove(self.velocity_colat_type)
        self.assertTrue(len(self.gml_data_block) == 1)
        self.assertTrue(self.velocity_colat_type not in self.gml_data_block)
        self.assertTrue(self.gml_data_block.get_scalar_values(self.velocity_colat_type) is None)
        # Removing same attribute twice should be fine.
        self.gml_data_block.remove(self.velocity_colat_type)
        self.assertTrue(len(self.gml_data_block) == 1)
        self.assertTrue(self.velocity_colat_type not in self.gml_data_block)
        self.assertTrue(self.gml_data_block.get_scalar_values(self.velocity_colat_type) is None)
        self.assertTrue(self.gml_data_block.get_scalar_values(self.velocity_lon_type) == self.velocity_lon_values)


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

    def test_convert(self):
        self.assertTrue(pygplates.GmlPoint([0, 0]) == self.gml_point)
        self.assertTrue(pygplates.GmlPoint([1, 0, 0]) == self.gml_point)
        self.assertTrue(pygplates.GmlPoint((90, 0)) == pygplates.GmlPoint(pygplates.PointOnSphere(0, 0, 1)))
        self.assertTrue(pygplates.GmlPoint((0, 0, 1)) == pygplates.GmlPoint(pygplates.PointOnSphere(0, 0, 1)))

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
        self.gml_time_instant1 = pygplates.GmlTimeInstant(self.geo_time_instant)
        self.gml_time_instant2 = pygplates.GmlTimeInstant(self.geo_time_instant.get_value())

    def test_get(self):
        self.assertTrue(self.gml_time_instant1.get_time() == self.geo_time_instant)
        self.assertTrue(self.gml_time_instant2.get_time() == self.geo_time_instant)

    def test_set(self):
        new_geo_time_instant = pygplates.GeoTimeInstant(20)
        self.gml_time_instant1.set_time(new_geo_time_instant)
        self.assertTrue(self.gml_time_instant1.get_time() == new_geo_time_instant)


class GmlTimePeriodCase(unittest.TestCase):
    def setUp(self):
        self.begin_time_position = 20
        self.end_geo_time_instant = pygplates.GeoTimeInstant.create_distant_future()
        self.gml_time_period = pygplates.GmlTimePeriod(
                self.begin_time_position, self.end_geo_time_instant)

    def test_get(self):
        self.assertTrue(self.gml_time_period.get_begin_time() == self.begin_time_position)
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
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                pygplates.GmlTimePeriod, 1.0, 2.0)


class GpmlArrayCase(unittest.TestCase):
    """Test GpmlArray.
    
    NOTE: The testing of the array elements list implementation is handled in 'test_revisioned_vector.py'.
    GpmlArray essentially just delegates to the RevisionedVector implementation so we just test a few delegate methods here."""
    
    def setUp(self):
        self.original_elements = []
        for i in range(4):
            element = pygplates.XsInteger(i)
            self.original_elements.append(element)
        
        self.gpml_array = pygplates.GpmlArray(self.original_elements)

    def test_get(self):
        self.assertTrue(list(self.gpml_array) == self.original_elements)
        self.assertTrue([element for element in self.gpml_array] == self.original_elements)

    def test_set(self):
        reversed_elements = list(reversed(self.gpml_array))
        self.gpml_array[:] = reversed_elements
        self.assertTrue(list(self.gpml_array) == reversed_elements)
        self.assertTrue(list(self.gpml_array) == [pygplates.XsInteger(i) for i in range(3,-1,-1)])
        
        self.gpml_array.sort(key = lambda i: i.get_integer())
        self.assertTrue(list(self.gpml_array) == [pygplates.XsInteger(i) for i in range(4)])
        
        # Need at least one time sample to create a GpmlArray.
        self.assertRaises(RuntimeError, pygplates.GpmlArray, [])


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
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                self.gpml_finite_rotation.get_finite_rotation(), self.finite_rotation))

    def test_set(self):
        new_finite_rotation = pygplates.FiniteRotation(pygplates.PointOnSphere(0,1,0), 0.4)
        self.gpml_finite_rotation.set_finite_rotation(new_finite_rotation)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                self.gpml_finite_rotation.get_finite_rotation(), new_finite_rotation))


# Not including interpolation function since it is not really used (yet) in GPlates and hence
# is just extra baggage for the python API user (we can add it later though)...
#class GpmlFiniteRotationSlerpCase(unittest.TestCase):
#    def test_create(self):
#        gpml_finite_rotation_slerp = pygplates.GpmlFiniteRotationSlerp()
#        self.assertTrue(isinstance(gpml_finite_rotation_slerp, pygplates.GpmlFiniteRotationSlerp))
#        self.assertTrue(isinstance(gpml_finite_rotation_slerp, pygplates.GpmlInterpolationFunction))


class GpmlIrregularSamplingCase(unittest.TestCase):
    """Test GpmlIrregularSampling.
    
    NOTE: The testing of the time sample list (GpmlTimeSampleList) is handled in 'test_revisioned_vector.py'."""
    
    def setUp(self):
        self.original_time_samples = []
        for i in range(4):
            ts = pygplates.GpmlTimeSample(pygplates.XsInteger(i), i)
            self.original_time_samples.append(ts)
        
        self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling(
                self.original_time_samples
                # Not including interpolation function since it is not really used (yet) in GPlates and hence
                # is just extra baggage for the python API user (we can add it later though)...
                #, pygplates.GpmlFiniteRotationSlerp()
                )

    def test_get(self):
        # Not including interpolation function since it is not really used (yet) in GPlates and hence
        # is just extra baggage for the python API user (we can add it later though)...
        #self.assertTrue(isinstance(self.gpml_irregular_sampling.get_interpolation_function(), pygplates.GpmlInterpolationFunction))
        
        self.assertTrue(list(self.gpml_irregular_sampling.get_time_samples()) == self.original_time_samples)
        self.assertTrue(list(self.gpml_irregular_sampling) == self.original_time_samples)
        self.assertTrue([time_sample for time_sample in self.gpml_irregular_sampling] == self.original_time_samples)

    def test_set(self):
        gpml_time_sample_list = self.gpml_irregular_sampling.get_time_samples()
        reversed_time_samples = list(reversed(self.original_time_samples))
        gpml_time_sample_list[:] = reversed_time_samples
        self.assertTrue(list(self.gpml_irregular_sampling.get_time_samples()) == reversed_time_samples)
        self.assertTrue(list(self.gpml_irregular_sampling.get_time_samples()) ==
                [pygplates.GpmlTimeSample(pygplates.XsInteger(i), i) for i in range(3,-1,-1)])

        reversed_time_samples = list(reversed(self.gpml_irregular_sampling))
        self.gpml_irregular_sampling[:] = reversed_time_samples
        self.assertTrue(list(self.gpml_irregular_sampling) == reversed_time_samples)
        self.assertTrue(list(self.gpml_irregular_sampling) ==
                [pygplates.GpmlTimeSample(pygplates.XsInteger(i), i) for i in range(4)])
        
        # Should be able to do list operations directly on the GpmlIrregularSampling instance.
        self.gpml_irregular_sampling.sort(key = lambda ts: -ts.get_time())
        self.assertTrue(list(self.gpml_irregular_sampling) ==
                [pygplates.GpmlTimeSample(pygplates.XsInteger(i), i) for i in range(3,-1,-1)])
        
        # Not including interpolation function since it is not really used (yet) in GPlates and hence
        # is just extra baggage for the python API user (we can add it later though)...
        #self.gpml_irregular_sampling.set_interpolation_function()
        #self.assertTrue(self.gpml_irregular_sampling.get_interpolation_function() is None)
        #interpolation_func = pygplates.GpmlFiniteRotationSlerp()
        #self.gpml_irregular_sampling.set_interpolation_function(interpolation_func)
        #self.assertTrue(self.gpml_irregular_sampling.get_interpolation_function() == interpolation_func)
        
        # Need at least one time sample to create a GpmlIrregularSampling.
        self.assertRaises(RuntimeError, pygplates.GpmlIrregularSampling, [])


class GpmlKeyValueDictionaryCase(unittest.TestCase):
    def setUp(self):
        self.gpml_key_value_dictionary = pygplates.GpmlKeyValueDictionary()
        for i in range(4):
            self.gpml_key_value_dictionary.set(str(i), i)

    def test_construct(self):
        gpml_key_value_dictionary = pygplates.GpmlKeyValueDictionary(
                [('name', 'Test')])
        self.assertTrue(len(gpml_key_value_dictionary) == 1)
        
        gpml_key_value_dictionary = pygplates.GpmlKeyValueDictionary(
                {'name' : 'Test'})
        self.assertTrue(len(gpml_key_value_dictionary) == 1)
        
        gpml_key_value_dictionary = pygplates.GpmlKeyValueDictionary(
                [('name', 'Test'), ('id', 23)])
        self.assertTrue(len(gpml_key_value_dictionary) == 2)
        self.assertTrue(gpml_key_value_dictionary.get('name') == 'Test')
        self.assertTrue(gpml_key_value_dictionary.get('id') == 23)
        
        gpml_key_value_dictionary = pygplates.GpmlKeyValueDictionary(
                {'name' : 'Test', 'id' : 23})
        self.assertTrue(len(gpml_key_value_dictionary) == 2)
        self.assertTrue(gpml_key_value_dictionary.get('name') == 'Test')
        self.assertTrue(gpml_key_value_dictionary.get('id') == 23)

    def test_len(self):
        self.assertTrue(len(self.gpml_key_value_dictionary) == 4)
        self.assertTrue(len(pygplates.GpmlKeyValueDictionary()) == 0)

    def test_contains(self):
        for i in range(4):
            self.assertTrue(str(i) in self.gpml_key_value_dictionary)
        self.assertTrue(str(4) not in self.gpml_key_value_dictionary)

    def test_iter(self):
        self.assertTrue(len(self.gpml_key_value_dictionary) == 4)
        count = 0
        for key in self.gpml_key_value_dictionary:
            count += 1
            self.assertTrue(key in self.gpml_key_value_dictionary)
            self.assertTrue(key in [str(i) for i in range(4)])
            self.assertTrue(self.gpml_key_value_dictionary.get(key) in range(4))
        self.assertTrue(count == 4)

    def test_get(self):
        for i in range(4):
            self.assertTrue(self.gpml_key_value_dictionary.get(str(i)) == i)
        self.assertFalse(self.gpml_key_value_dictionary.get(str(4)))
        self.assertTrue(self.gpml_key_value_dictionary.get(str(4), 4) == 4)

    def test_set(self):
        # Override existing value.
        self.assertTrue(len(self.gpml_key_value_dictionary) == 4)
        self.gpml_key_value_dictionary.set(str(1), 10)
        self.assertTrue(len(self.gpml_key_value_dictionary) == 4)
        for i in range(4):
            if i == 1:
                self.assertTrue(self.gpml_key_value_dictionary.get(str(1)) == 10)
                self.assertTrue(isinstance(self.gpml_key_value_dictionary.get(str(1)), int))
            else:
                self.assertTrue(self.gpml_key_value_dictionary.get(str(i)) == i)
        self.gpml_key_value_dictionary.set('test_key', 10.2)
        self.assertTrue(self.gpml_key_value_dictionary.get('test_key') == 10.2)
        self.gpml_key_value_dictionary.set('test_key', 'test_value')
        self.assertTrue(self.gpml_key_value_dictionary.get('test_key') == 'test_value')
        self.gpml_key_value_dictionary.set('test_key2', 'test_value2')
        self.assertTrue(self.gpml_key_value_dictionary.get('test_key') == 'test_value')
        self.assertTrue(self.gpml_key_value_dictionary.get('test_key2') == 'test_value2')

    def test_remove(self):
        self.assertTrue(len(self.gpml_key_value_dictionary) == 4)
        self.gpml_key_value_dictionary.remove(str(1))
        self.assertTrue(len(self.gpml_key_value_dictionary) == 3)
        # Removing same attribute twice should be fine.
        self.gpml_key_value_dictionary.remove(str(1))
        self.assertTrue(len(self.gpml_key_value_dictionary) == 3)
        for i in range(4):
            if i != 1:
                self.assertTrue(self.gpml_key_value_dictionary.get(str(i)) == i)
        self.assertFalse(self.gpml_key_value_dictionary.get(str(1)))
        self.assertTrue(self.gpml_key_value_dictionary.get(str(1), 1) == 1)


class GpmlOldPlatesHeaderCase(unittest.TestCase):
    def setUp(self):
        self.gpml_old_plates_header = pygplates.GpmlOldPlatesHeader(
                region_number=10,
                reference_number=11,
                string_number=12,
                geographic_description='geographic description',
                plate_id_number=201,
                age_of_appearance=20.6,
                age_of_disappearance=10.2,
                data_type_code='RI',
                data_type_code_number=13,
                data_type_code_number_additional='a',
                conjugate_plate_id_number=701,
                colour_code=14,
                number_of_points=15)

    def test_get(self):
        self.assertTrue(self.gpml_old_plates_header.get_region_number() == 10)
        self.assertTrue(self.gpml_old_plates_header.get_reference_number() == 11)
        self.assertTrue(self.gpml_old_plates_header.get_string_number() == 12)
        self.assertTrue(self.gpml_old_plates_header.get_geographic_description() == 'geographic description')
        self.assertTrue(self.gpml_old_plates_header.get_plate_id_number() == 201)
        self.assertAlmostEqual(self.gpml_old_plates_header.get_age_of_appearance(), 20.6)
        self.assertAlmostEqual(self.gpml_old_plates_header.get_age_of_disappearance(), 10.2)
        self.assertTrue(self.gpml_old_plates_header.get_data_type_code() == 'RI')
        self.assertTrue(self.gpml_old_plates_header.get_data_type_code_number() == 13)
        self.assertTrue(self.gpml_old_plates_header.get_data_type_code_number_additional() == 'a')
        self.assertTrue(self.gpml_old_plates_header.get_conjugate_plate_id_number() == 701)
        self.assertTrue(self.gpml_old_plates_header.get_colour_code() == 14)
        self.assertTrue(self.gpml_old_plates_header.get_number_of_points() == 15)

    def test_set(self):
        self.gpml_old_plates_header.set_region_number(20)
        self.assertTrue(self.gpml_old_plates_header.get_region_number() == 20)
        self.gpml_old_plates_header.set_reference_number(21)
        self.assertTrue(self.gpml_old_plates_header.get_reference_number() == 21)
        self.gpml_old_plates_header.set_string_number(22)
        self.assertTrue(self.gpml_old_plates_header.get_string_number() == 22)
        self.gpml_old_plates_header.set_geographic_description('another geographic description')
        self.assertTrue(self.gpml_old_plates_header.get_geographic_description() == 'another geographic description')
        self.gpml_old_plates_header.set_plate_id_number(202)
        self.assertTrue(self.gpml_old_plates_header.get_plate_id_number() == 202)
        self.gpml_old_plates_header.set_age_of_appearance(30.6)
        self.assertAlmostEqual(self.gpml_old_plates_header.get_age_of_appearance(), 30.6)
        self.gpml_old_plates_header.set_age_of_disappearance(20.2)
        self.assertAlmostEqual(self.gpml_old_plates_header.get_age_of_disappearance(), 20.2)
        self.gpml_old_plates_header.set_data_type_code('TF')
        self.assertTrue(self.gpml_old_plates_header.get_data_type_code() == 'TF')
        self.gpml_old_plates_header.set_data_type_code_number(23)
        self.assertTrue(self.gpml_old_plates_header.get_data_type_code_number() == 23)
        self.gpml_old_plates_header.set_data_type_code_number_additional('b')
        self.assertTrue(self.gpml_old_plates_header.get_data_type_code_number_additional() == 'b')
        self.gpml_old_plates_header.set_conjugate_plate_id_number(702)
        self.assertTrue(self.gpml_old_plates_header.get_conjugate_plate_id_number() == 702)
        self.gpml_old_plates_header.set_colour_code(24)
        self.assertTrue(self.gpml_old_plates_header.get_colour_code() == 24)
        self.gpml_old_plates_header.set_number_of_points(25)
        self.assertTrue(self.gpml_old_plates_header.get_number_of_points() == 25)


class GpmlPiecewiseAggregationCase(unittest.TestCase):
    """Test GpmlPiecewiseAggregation.
    
    NOTE: The testing of the time window list (GpmlTimeWindowList) is handled in 'test_revisioned_vector.py'."""
    
    def setUp(self):
        self.original_time_windows = []
        for i in range(4):
            tw = pygplates.GpmlTimeWindow(
                pygplates.XsInteger(i),
                pygplates.GeoTimeInstant(i+1), # begin time - earlier
                i)   # end time - later
            self.original_time_windows.append(tw)
        
        self.gpml_piecewise_aggregation = pygplates.GpmlPiecewiseAggregation(self.original_time_windows)

    def test_get(self):
        self.assertTrue(list(self.gpml_piecewise_aggregation.get_time_windows()) == self.original_time_windows)
        self.assertTrue(list(self.gpml_piecewise_aggregation) == self.original_time_windows)
        self.assertTrue([time_window for time_window in self.gpml_piecewise_aggregation] == self.original_time_windows)

    def test_set(self):
        # Need at least one time sample to create a GpmlPiecewiseAggregation.
        self.assertRaises(RuntimeError, pygplates.GpmlPiecewiseAggregation, [])

        gpml_time_window_list = self.gpml_piecewise_aggregation.get_time_windows()
        reversed_time_windows = list(reversed(self.original_time_windows))
        gpml_time_window_list[:] = reversed_time_windows
        self.assertTrue(list(self.gpml_piecewise_aggregation.get_time_windows()) == reversed_time_windows)
        self.assertTrue(list(self.gpml_piecewise_aggregation.get_time_windows()) ==
                [pygplates.GpmlTimeWindow(pygplates.XsInteger(i), i+1, i) for i in range(3,-1,-1)])

        reversed_time_windows = list(reversed(self.gpml_piecewise_aggregation))
        self.gpml_piecewise_aggregation[:] = reversed_time_windows
        self.assertTrue(list(self.gpml_piecewise_aggregation) == reversed_time_windows)
        self.assertTrue(list(self.gpml_piecewise_aggregation) ==
                [pygplates.GpmlTimeWindow(pygplates.XsInteger(i), i+1, i) for i in range(4)])
        
        # Should be able to do list operations directly on the GpmlPiecewiseAggregation instance.
        self.gpml_piecewise_aggregation.sort(key = lambda tw: -tw.get_begin_time())
        self.assertTrue(list(self.gpml_piecewise_aggregation) ==
                [pygplates.GpmlTimeWindow(pygplates.XsInteger(i), i+1, i) for i in range(3,-1,-1)])
        self.gpml_piecewise_aggregation.sort(key = lambda tw: tw.get_begin_time())
        self.assertTrue(list(self.gpml_piecewise_aggregation) ==
                [pygplates.GpmlTimeWindow(pygplates.XsInteger(i), i+1, i) for i in range(4)])
        
        # Replace the second and third time windows with a new time window that spans both.
        self.gpml_piecewise_aggregation[1:3] = [
                pygplates.GpmlTimeWindow(pygplates.XsInteger(10),
                self.gpml_piecewise_aggregation[2].get_begin_time(),
                self.gpml_piecewise_aggregation[1].get_end_time())]
        self.assertTrue(list(self.gpml_piecewise_aggregation) ==
                [pygplates.GpmlTimeWindow(pygplates.XsInteger(i), bt, et) for i, bt, et in [(0,1,0), (10,3,1), (3,4,3)]])


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


class GpmlPolarityChronIdCase(unittest.TestCase):
    def setUp(self):
        self.era = 'Cenozoic'
        self.major_region = 34
        self.minor_region = 'ad'
        self.gpml_polarity_chron_id = pygplates.GpmlPolarityChronId(
                self.era, self.major_region, self.minor_region)

    def test_get(self):
        self.assertTrue(self.gpml_polarity_chron_id.get_era() == self.era)
        self.assertTrue(self.gpml_polarity_chron_id.get_major_region() == self.major_region)
        self.assertTrue(self.gpml_polarity_chron_id.get_minor_region() == self.minor_region)

    def test_set(self):
        new_era = 'Mesozoic'
        new_major_region = 20
        new_minor_region = 'c'
        self.gpml_polarity_chron_id.set_era(new_era)
        self.assertTrue(self.gpml_polarity_chron_id.get_era() == new_era)
        self.gpml_polarity_chron_id.set_major_region(new_major_region)
        self.assertTrue(self.gpml_polarity_chron_id.get_major_region() == new_major_region)
        self.gpml_polarity_chron_id.set_minor_region(new_minor_region)
        self.assertTrue(self.gpml_polarity_chron_id.get_minor_region() == new_minor_region)
        
        self.assertTrue(pygplates.GpmlPolarityChronId(major_region=34, minor_region='ad').get_era() is None)
        self.assertTrue(pygplates.GpmlPolarityChronId(era='Cenozoic', minor_region='ad').get_major_region() is None)
        self.assertTrue(pygplates.GpmlPolarityChronId(era='Cenozoic', major_region=34).get_minor_region() is None)
        
        # Violate the era class invariant.
        self.assertRaises(pygplates.InformationModelError, pygplates.GpmlPolarityChronId, 'UnknownEra')
        self.assertRaises(pygplates.InformationModelError, pygplates.GpmlPolarityChronId.set_era, self.gpml_polarity_chron_id, 'UnknownEra')


class GpmlTimeSampleCase(unittest.TestCase):
    def setUp(self):
        self.property_value1 = pygplates.GpmlPlateId(701)
        self.time1 = pygplates.GeoTimeInstant(10)
        self.description1 = 'A plate id time sample'
        self.is_enabled1 = False
        self.gpml_time_sample1 = pygplates.GpmlTimeSample(
                self.property_value1, self.time1, self.description1, self.is_enabled1)
        
        self.property_value2 = pygplates.GpmlPlateId(201)
        self.time2 = 20
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
        self.gpml_time_sample1.set_value(new_property_value)
        self.gpml_time_sample1.set_time(new_time)
        self.gpml_time_sample1.set_description(new_description)
        self.gpml_time_sample1.set_disabled()
        self.assertTrue(self.gpml_time_sample1.get_value() == new_property_value)
        self.assertTrue(self.gpml_time_sample1.get_time() == new_time)
        self.assertTrue(self.gpml_time_sample1.get_description() == new_description)
        self.assertTrue(not self.gpml_time_sample1.is_enabled())
        self.assertTrue(self.gpml_time_sample1.is_disabled())
        self.gpml_time_sample1.set_enabled()
        self.assertTrue(self.gpml_time_sample1.is_enabled())
        self.assertTrue(not self.gpml_time_sample1.is_disabled())
        
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
        self.end_time = 10
        self.gpml_time_window = pygplates.GpmlTimeWindow(
                self.property_value, self.begin_time, self.end_time)

    def test_get(self):
        self.assertTrue(self.gpml_time_window.get_value() == self.property_value)
        self.assertTrue(self.gpml_time_window.get_begin_time() == self.begin_time)
        self.assertTrue(self.gpml_time_window.get_end_time() == self.end_time)

    def test_set(self):
        # Violate the begin/end time class invariant.
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                pygplates.GpmlTimeWindow, self.property_value, 1.0, 2.0)
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                self.gpml_time_window.set_begin_time, 1.0)
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                self.gpml_time_window.set_end_time, 30)

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
            PropertyValueCase,
            EnumerationCase,
            GeoTimeInstantCase,
            GmlDataBlockCase,
            GmlLineStringCase,
            GmlMultiPointCase,
            GmlOrientableCurveCase,
            GmlPointCase,
            GmlPolygonCase,
            GmlTimeInstantCase,
            GmlTimePeriodCase,
            GpmlArrayCase,
            GpmlConstantValueCase,
            GpmlFiniteRotationCase,
            # Not including interpolation function since it is not really used (yet) in GPlates and hence
            # is just extra baggage for the python API user (we can add it later though)...
            #GpmlFiniteRotationSlerpCase,
            
            GpmlIrregularSamplingCase,
            GpmlKeyValueDictionaryCase,
            GpmlOldPlatesHeaderCase,
            GpmlPiecewiseAggregationCase,
            GpmlPlateIdCase,
            GpmlPolarityChronIdCase,
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
