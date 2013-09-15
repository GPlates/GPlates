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


class GmlTimeInstantCase(unittest.TestCase):
	def setUp(self):
		self.geo_time_instant = pygplates.GeoTimeInstant(10)
		self.gml_time_instant = pygplates.GmlTimeInstant.create(self.geo_time_instant)

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
		self.gml_time_period = pygplates.GmlTimePeriod.create(
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


class GpmlConstantValueCase(unittest.TestCase):
	def setUp(self):
		self.property_value1 = pygplates.GpmlPlateId.create(701)
		self.description1 = 'A plate id'
		self.gpml_constant_value1 = pygplates.GpmlConstantValue.create(self.property_value1, self.description1)
		
		self.property_value2 = pygplates.GpmlPlateId.create(201)
		self.gpml_constant_value2 = pygplates.GpmlConstantValue.create(self.property_value2)

	def test_get(self):
		self.assertTrue(self.gpml_constant_value1.get_value() == self.property_value1)
		self.assertTrue(self.gpml_constant_value1.get_description() == self.description1)
		self.assertTrue(self.gpml_constant_value2.get_value() == self.property_value2)
		self.assertTrue(self.gpml_constant_value2.get_description() is None)

	def test_set(self):
		new_property_value = pygplates.GpmlPlateId.create(801)
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


class GpmlFiniteRotationSlerpCase(unittest.TestCase):
	def test_create(self):
		gpml_finite_rotation_slerp = pygplates.GpmlFiniteRotationSlerp.create()
		self.assertTrue(isinstance(gpml_finite_rotation_slerp, pygplates.GpmlFiniteRotationSlerp))
		self.assertTrue(isinstance(gpml_finite_rotation_slerp, pygplates.GpmlInterpolationFunction))


class GpmlIrregularSamplingCase(unittest.TestCase):
	"""Test GpmlIrregularSampling.
	
	NOTE: The testing of the time sample list (GpmlTimeSampleList) is handled in 'test_revisioned_vector.py'."""
	
	def setUp(self):
		self.original_time_samples = []
		for i in range(0,4):
			ts = pygplates.GpmlTimeSample.create(
				pygplates.XsInteger.create(i),
				pygplates.GeoTimeInstant(i))
			self.original_time_samples.append(ts)
		
		self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling.create(
				self.original_time_samples,
				pygplates.GpmlFiniteRotationSlerp.create())

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
		interpolation_func = pygplates.GpmlFiniteRotationSlerp.create()
		self.gpml_irregular_sampling.set_interpolation_function(interpolation_func)
		self.assertTrue(self.gpml_irregular_sampling.get_interpolation_function() == interpolation_func)


class GpmlPlateIdCase(unittest.TestCase):
	def setUp(self):
		self.plate_id = 701
		self.gpml_plate_id = pygplates.GpmlPlateId.create(self.plate_id)

	def test_get(self):
		self.assertTrue(self.gpml_plate_id.get_plate_id() == self.plate_id)

	def test_set(self):
		new_plate_id = 201
		self.gpml_plate_id.set_plate_id(new_plate_id)
		self.assertTrue(self.gpml_plate_id.get_plate_id() == new_plate_id)


class XsBooleanCase(unittest.TestCase):
	def setUp(self):
		self.boolean = True
		self.xs_boolean = pygplates.XsBoolean.create(self.boolean)

	def test_get(self):
		self.assertTrue(self.xs_boolean.get_boolean() == self.boolean)

	def test_set(self):
		new_boolean = False
		self.xs_boolean.set_boolean(new_boolean)
		self.assertTrue(self.xs_boolean.get_boolean() == new_boolean)


class XsDoubleCase(unittest.TestCase):
	def setUp(self):
		self.double = 10.5
		self.xs_double = pygplates.XsDouble.create(self.double)

	def test_get(self):
		self.assertTrue(self.xs_double.get_double() == self.double)

	def test_set(self):
		new_double = -100.2
		self.xs_double.set_double(new_double)
		self.assertTrue(self.xs_double.get_double() == new_double)


class XsIntegerCase(unittest.TestCase):
	def setUp(self):
		self.integer = 10
		self.xs_integer = pygplates.XsInteger.create(self.integer)

	def test_get(self):
		self.assertTrue(self.xs_integer.get_integer() == self.integer)

	def test_set(self):
		new_integer = -100
		self.xs_integer.set_integer(new_integer)
		self.assertTrue(self.xs_integer.get_integer() == new_integer)


class XsStringCase(unittest.TestCase):
	def setUp(self):
		self.string = 'test-string'
		self.xs_string = pygplates.XsString.create(self.string)

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
			GmlTimeInstantCase,
			GmlTimePeriodCase,
			GpmlConstantValueCase,
			GpmlFiniteRotationSlerpCase,
			GpmlIrregularSamplingCase,
			GpmlPlateIdCase,
			XsBooleanCase,
			XsDoubleCase,
			XsIntegerCase,
			XsStringCase
		]

	for test_case in test_cases:
		suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
	
	return suite
