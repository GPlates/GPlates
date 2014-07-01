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
        self.feature.add(
                pygplates.PropertyName.create_gml('description'),
                pygplates.XsString('Feature description'))
        self.feature.add(
                pygplates.PropertyName.create_gml('name'),
                pygplates.XsString('Feature name'))
        self.feature.add(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                pygplates.GpmlPlateId(1))
        self.feature.add(
                pygplates.PropertyName.create_gpml('conjugatePlateId'),
                pygplates.GpmlPlateId(2))
        self.feature.add(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                pygplates.GpmlPlateId(3))
        self.feature.add(
                pygplates.PropertyName.create_gpml('leftPlate'),
                pygplates.GpmlPlateId(11))
        self.feature.add(
                pygplates.PropertyName.create_gpml('rightPlate'),
                pygplates.GpmlPlateId(12))
        self.feature.add(
                pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'),
                pygplates.XsInteger(300))
        self.feature.add(
                pygplates.PropertyName.create_gml('validTime'),
                pygplates.GmlTimePeriod(50, 0))
        # A time-dependent property excluding time 0Ma.
        self.feature.add(
                pygplates.PropertyName.create_gpml('plateId'),
                pygplates.GpmlPiecewiseAggregation([
                    pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(100), pygplates.GeoTimeInstant(10), pygplates.GeoTimeInstant(5)),
                    pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(101), pygplates.GeoTimeInstant(20), pygplates.GeoTimeInstant(10))]),
                # 'gpml:plateId' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.feature.add(
                pygplates.PropertyName.create_gpml('unclassifiedGeometry'),
                pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(0,1,0))))
    
    def test_get_and_set_geometry(self):
        # The default geometry property name for an unclassified feature type is 'gpml:unclassifiedGeometry'.
        geometry_properties = self.feature.get_geometry(property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(geometry_properties) == 1)
        self.assertTrue(isinstance(geometry_properties[0], pygplates.PointOnSphere))
        self.assertTrue(isinstance(self.feature.get_geometry(), pygplates.PointOnSphere))
        self.feature.add(
                pygplates.PropertyName.create_gpml('position'),
                pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(0,0,1))))
        # There should now be two geometry properties.
        self.assertTrue(len(self.feature.get_geometry(lambda property: True, pygplates.PropertyReturn.all)) == 2)
        # Remove the default geometry property.
        self.feature.remove(pygplates.PropertyName.create_gpml('unclassifiedGeometry'))
        self.assertFalse(self.feature.get_geometry())
        self.assertTrue(isinstance(
                self.feature.get_geometry(pygplates.PropertyName.create_gpml('position')), pygplates.PointOnSphere))
        gml_orientable_curve = pygplates.GmlOrientableCurve(pygplates.GmlLineString(
            pygplates.PolylineOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)])))
        self.feature.add(
                pygplates.PropertyName.create_gpml('unclassifiedGeometry'),
                pygplates.GpmlConstantValue(gml_orientable_curve))
        self.assertTrue(isinstance(self.feature.get_geometry(), pygplates.PolylineOnSphere))
        
        point = pygplates.PointOnSphere(1,0,0)
        self.feature.set_geometry(point)
        self.assertTrue(self.feature.get_geometry() == point)
        multi_point = pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)])
        self.feature.set_geometry(multi_point)
        self.assertTrue(self.feature.get_geometry() == multi_point)
        # Set a different multipoint.
        multi_point = pygplates.MultiPointOnSphere([pygplates.PointOnSphere(0,1,0), pygplates.PointOnSphere(0,1,0)])
        self.feature.set_geometry(multi_point)
        self.assertTrue(self.feature.get_geometry() == multi_point)
        polyline = pygplates.PolylineOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)])
        self.feature.set_geometry(polyline)
        self.assertTrue(self.feature.get_geometry() == polyline)
        polygon = pygplates.PolygonOnSphere(
                [pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0), pygplates.PointOnSphere(0,0,1)])
        self.feature.set_geometry(polygon)
        self.assertTrue(self.feature.get_geometry() == polygon)
        # Set multiple geometries.
        geometries = self.feature.set_geometry([point, multi_point, polyline, polygon])
        self.assertTrue(len(geometries) == 4)
        self.assertTrue(len(self.feature.get_geometry(property_return=pygplates.PropertyReturn.all)) == 4)
        self.feature.set_geometry(point, pygplates.PropertyName.create_gpml('position'))
        self.assertTrue(self.feature.get_geometry(pygplates.PropertyName.create_gpml('position')) == point)
        hot_spot_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('HotSpot'))
        self.assertTrue(hot_spot_feature.set_geometry(
                point, pygplates.PropertyName.create_gpml('position')).get_value().get_geometry() == point)
        # Cannot set multiple points on 'gpml:position' property on a 'gpml:HotSpot' feature (or any feature but 'gpml:UnclassifiedFeature').
        self.assertRaises(pygplates.InformationModelError,
                hot_spot_feature.set_geometry, (point, point), pygplates.PropertyName.create_gpml('position'))
        # ...unless we don't check the information model.
        self.assertTrue(
                len(hot_spot_feature.set_geometry((point, point), pygplates.PropertyName.create_gpml('position'),
                        pygplates.VerifyInformationModel.no)) == 2)
        # Setting multiple points on 'gpml:position' property on a 'gpml:UnclassifiedFeature' feature is fine though.
        points = self.feature.set_geometry((point, point), pygplates.PropertyName.create_gpml('position'))
        self.assertTrue(len(points) == 2)
        self.assertTrue(points[0].get_value().get_geometry() == point)
        self.assertTrue(points[1].get_value().get_geometry() == point)
        # Cannot set a multipoint on 'gpml:position'.
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_geometry, multi_point, pygplates.PropertyName.create_gpml('position'))
        # ...unless we don't check the information model.
        self.assertTrue(self.feature.set_geometry(
                multi_point, pygplates.PropertyName.create_gpml('position'), pygplates.VerifyInformationModel.no))
    
    def test_get_and_set_description(self):
        description = self.feature.get_description()
        self.assertTrue(description == 'Feature description')
        self.feature.remove(pygplates.PropertyName.create_gml('description'))
        # With property removed it should return default of an empty string.
        description = self.feature.get_description()
        self.assertTrue(description == '')
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_description(None))
        
        self.feature.set_description('Another description')
        self.assertTrue(self.feature.get_description() == 'Another description')
        gml_description = self.feature.set_description('Yet another description')
        self.assertTrue(gml_description.get_value().get_string() == self.feature.get_description())
        self.assertTrue(self.feature.get_description() == 'Yet another description')
    
    def test_get_and_set_name(self):
        self.assertTrue(self.feature.get_name() == 'Feature name')
        self.assertTrue(self.feature.get_name('', pygplates.PropertyReturn.first) == 'Feature name')
        self.feature.remove(pygplates.PropertyName.create_gml('name'))
        # With property removed it should return default of an empty string.
        self.assertTrue(self.feature.get_name() == '')
        self.assertFalse(self.feature.get_name())
        # With property removed it should return default of an empty string.
        self.assertTrue(self.feature.get_name('', pygplates.PropertyReturn.first) == '')
        
        self.feature.set_name('Another name')
        self.assertTrue(self.feature.get_name() == 'Another name')
        gml_name = self.feature.set_name('Yet another name')
        self.assertTrue(gml_name.get_value().get_string() == self.feature.get_name())
        self.assertTrue(self.feature.get_name() == 'Yet another name')
    
    def test_get_and_set_names(self):
        self.assertTrue(self.feature.get_name([], pygplates.PropertyReturn.all) == ['Feature name'])
        self.feature.remove(pygplates.PropertyName.create_gml('name'))
        # With property removed it should return default of an empty list.
        self.assertTrue(self.feature.get_name([], pygplates.PropertyReturn.all) == [])
        self.assertFalse(self.feature.get_name([], pygplates.PropertyReturn.all))
        self.assertFalse(self.feature.get_name(None, pygplates.PropertyReturn.all))
        self.feature.add(pygplates.PropertyName.create_gml('name'), pygplates.XsString('Feature name'))
        self.feature.add(pygplates.PropertyName.create_gml('name'), pygplates.XsString(''))
        # The name with the empty string will still get returned.
        names = self.feature.get_name([], pygplates.PropertyReturn.all)
        self.assertTrue('Feature name' in names)
        self.assertTrue('' in names)
        # If we ask for exactly one name then we'll get none (even though one of the two names is an empty string).
        self.assertFalse(self.feature.get_name())
        self.feature.remove(pygplates.PropertyName.create_gml('name'))
        self.feature.add(pygplates.PropertyName.create_gml('name'), pygplates.XsString(''))
        # With only a single name with empty string it should still return a non-empty list.
        self.assertTrue(self.feature.get_name([], pygplates.PropertyReturn.all) == [''])
        self.assertFalse(any(self.feature.get_name([], pygplates.PropertyReturn.all)))
        
        self.feature.set_name(['Another name1', 'Another name2'])
        self.assertTrue('Another name1' in self.feature.get_name([], pygplates.PropertyReturn.all))
        self.assertTrue('Another name2' in self.feature.get_name([], pygplates.PropertyReturn.all))
        gml_name = self.feature.set_name(('Yet another name1', 'Yet another name2'))
        self.assertTrue(len(gml_name) == 2)
    
    def test_get_and_set_valid_time(self):
        begin_time, end_time = self.feature.get_valid_time()
        self.assertTrue(pygplates.GeoTimeInstant(begin_time) == 50)
        self.assertTrue(pygplates.GeoTimeInstant(end_time) == 0)
        self.feature.remove(pygplates.PropertyName.create_gml('validTime'))
        # With property removed it should return default of all time.
        begin_time, end_time = self.feature.get_valid_time()
        self.assertTrue(pygplates.GeoTimeInstant(begin_time).is_distant_past())
        self.assertTrue(pygplates.GeoTimeInstant(end_time).is_distant_future())
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_valid_time(None))
        
        self.feature.set_valid_time(pygplates.GeoTimeInstant.create_distant_past(), 0)
        self.assertTrue(self.feature.get_valid_time() == (pygplates.GeoTimeInstant.create_distant_past(), 0))
        gml_valid_time = self.feature.set_valid_time(100, 0)
        self.assertTrue(gml_valid_time.get_value().get_begin_time() == 100)
        self.assertTrue(gml_valid_time.get_value().get_end_time() == 0)
        self.assertTrue(self.feature.get_valid_time() == (100, 0))
    
    def test_get_and_set_left_plate(self):
        self.assertTrue(self.feature.get_left_plate() == 11)
        self.feature.remove(pygplates.PropertyName.create_gpml('leftPlate'))
        # With property removed it should return default of zero.
        self.assertTrue(self.feature.get_left_plate() == 0)
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_left_plate(None))
        
        self.feature.set_left_plate(201)
        self.assertTrue(self.feature.get_left_plate() == 201)
        gpml_left_plate = self.feature.set_left_plate(701)
        self.assertTrue(gpml_left_plate.get_value().get_plate_id() == self.feature.get_left_plate())
        self.assertTrue(self.feature.get_left_plate() == 701)
    
    def test_get_and_set_right_plate(self):
        self.assertTrue(self.feature.get_right_plate() == 12)
        self.feature.remove(pygplates.PropertyName.create_gpml('rightPlate'))
        # With property removed it should return default of zero.
        self.assertTrue(self.feature.get_right_plate() == 0)
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_right_plate(None))
        
        self.feature.set_right_plate(201)
        self.assertTrue(self.feature.get_right_plate() == 201)
        gpml_right_plate = self.feature.set_right_plate(701)
        self.assertTrue(gpml_right_plate.get_value().get_plate_id() == self.feature.get_right_plate())
        self.assertTrue(self.feature.get_right_plate() == 701)
    
    def test_get_and_get_reconstruction_method(self):
        # There's no reconstruction method so should return default.
        self.assertTrue(self.feature.get_reconstruction_method() == 'ByPlateId')
        self.assertFalse(self.feature.get_reconstruction_method(None))
        self.feature.set_reconstruction_method('HalfStageRotation')
        self.assertTrue(self.feature.get_reconstruction_method() == 'HalfStageRotation')
        gpml_reconstruction_method = self.feature.set_reconstruction_method('HalfStageRotationVersion2')
        self.assertTrue(self.feature.get_reconstruction_method() == 'HalfStageRotationVersion2')
        self.assertTrue(gpml_reconstruction_method.get_value().get_type() == pygplates.EnumerationType.create_gpml('ReconstructionMethodEnumeration'))
        self.assertTrue(gpml_reconstruction_method.get_value().get_content() == 'HalfStageRotationVersion2')
        self.assertRaises(pygplates.InformationModelError, self.feature.set_reconstruction_method, 'UnknownContent')
        self.feature.set_reconstruction_method('UnknownContent', pygplates.VerifyInformationModel.no)
        self.assertTrue(self.feature.get_reconstruction_method() == 'UnknownContent')
        self.feature.remove(pygplates.PropertyName.create_gpml('reconstructionMethod'))
        self.assertFalse(self.feature.get_reconstruction_method(None))
        # Add a reconstruction method with an invalid enumeration type.
        self.feature.add(
                pygplates.PropertyName.create_gpml('reconstructionMethod'),
                pygplates.Enumeration(pygplates.EnumerationType.create_gpml('UnknownEnumeration'), 'HalfStageRotationVersion2', pygplates.VerifyInformationModel.no))
        self.assertFalse(self.feature.get_reconstruction_method(None))
    
    def test_get_and_get_reconstruction_plate_id(self):
        # There are two reconstruction plate IDs so this should return zero (default when not exactly one property).
        self.assertTrue(self.feature.get_reconstruction_plate_id() == 0)
        self.assertFalse(self.feature.get_reconstruction_plate_id(None))
        # Remove both reconstruction plate IDs.
        self.feature.remove(pygplates.PropertyName.create_gpml('reconstructionPlateId'))
        self.feature.add(pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.GpmlPlateId(10))
        reconstruction_plate_id = self.feature.get_reconstruction_plate_id()
        self.assertTrue(reconstruction_plate_id == 10)
        
        self.feature.set_reconstruction_plate_id(201)
        self.assertTrue(self.feature.get_reconstruction_plate_id() == 201)
        gpml_reconstruction_plate_id = self.feature.set_reconstruction_plate_id(701)
        self.assertTrue(gpml_reconstruction_plate_id.get_value().get_plate_id() == self.feature.get_reconstruction_plate_id())
        self.assertTrue(self.feature.get_reconstruction_plate_id() == 701)
    
    def test_get_and_set_conjugate_plate_id(self):
        self.assertTrue(self.feature.get_conjugate_plate_id() == 2)
        self.assertTrue(self.feature.get_conjugate_plate_id(0, pygplates.PropertyReturn.first) == 2)
        self.feature.remove(pygplates.PropertyName.create_gpml('conjugatePlateId'))
        # With property removed it should return default of zero.
        self.assertTrue(self.feature.get_conjugate_plate_id() == 0)
        self.assertFalse(self.feature.get_conjugate_plate_id())
        # With property removed it should return default of zero.
        self.assertTrue(self.feature.get_conjugate_plate_id(0, pygplates.PropertyReturn.first) == 0)
        
        self.feature.set_conjugate_plate_id(201)
        self.assertTrue(self.feature.get_conjugate_plate_id() == 201)
        gpml_conjugate_plate_id = self.feature.set_conjugate_plate_id(701)
        self.assertTrue(gpml_conjugate_plate_id.get_value().get_plate_id() == self.feature.get_conjugate_plate_id())
        self.assertTrue(self.feature.get_conjugate_plate_id() == 701)
    
    def test_get_and_set_conjugate_plate_ids(self):
        self.assertTrue(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all) == [2])
        self.feature.remove(pygplates.PropertyName.create_gpml('conjugatePlateId'))
        # With property removed it should return default of an empty list.
        self.assertTrue(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all) == [])
        self.assertFalse(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all))
        self.feature.add(pygplates.PropertyName.create_gpml('conjugatePlateId'), pygplates.GpmlPlateId(22))
        self.feature.add(pygplates.PropertyName.create_gpml('conjugatePlateId'), pygplates.GpmlPlateId(0))
        # The plate ID zero should still get returned.
        conjugate_plate_ids = self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all)
        self.assertTrue(0 in conjugate_plate_ids)
        self.assertTrue(22 in conjugate_plate_ids)
        # If we ask for exactly one plate ID then we'll get none (even though one of the two plate IDs is zero).
        self.assertFalse(self.feature.get_conjugate_plate_id())
        self.feature.remove(pygplates.PropertyName.create_gpml('conjugatePlateId'))
        self.feature.add(pygplates.PropertyName.create_gpml('conjugatePlateId'), pygplates.GpmlPlateId(0))
        # With only a single plate ID of zero it should still return a non-empty list.
        self.assertTrue(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all) == [0])
        self.assertFalse(any(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all)))
        
        self.feature.set_conjugate_plate_id([903, 904])
        self.assertTrue(903 in self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all))
        self.assertTrue(904 in self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all))
        gpml_conjugate_plate_id = self.feature.set_conjugate_plate_id((905, 906))
        self.assertTrue(len(gpml_conjugate_plate_id) == 2)
    
    def test_get_and_get_shapefile_attribute(self):
        # Start off with feature that has no 'gpml:shapefileAttributes' property.
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes'), pygplates.PropertyReturn.all))
        self.assertFalse(self.feature.get_shapefile_attribute('non_existent_key'))
        self.assertTrue(self.feature.get_shapefile_attribute('non_existent_key', 'default_value') == 'default_value')
        # Set shapefile attribute on feature that has no 'gpml:shapefileAttributes' property.
        self.feature.set_shapefile_attribute('test_key', 'test_value')
        self.assertTrue(self.feature.get_shapefile_attribute('test_key') == 'test_value')
        self.assertTrue(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes'), pygplates.PropertyReturn.all))
        self.assertTrue(len(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes')).get_value()) == 1)
        self.feature.set_shapefile_attribute('test_key', 100)
        self.assertTrue(self.feature.get_shapefile_attribute('test_key') == 100)
        self.assertTrue(len(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes')).get_value()) == 1)
        self.feature.set_shapefile_attribute('test_key', 101.0)
        self.assertTrue(self.feature.get_shapefile_attribute('test_key') == 101.0)
        self.assertTrue(len(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes')).get_value()) == 1)
        self.feature.set_shapefile_attribute('test_key2', -100)
        self.assertTrue(self.feature.get_shapefile_attribute('test_key2') == -100)
        # Added a new key so should have one more element in the dictionary.
        self.assertTrue(len(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes')).get_value()) == 2)
        self.assertFalse(self.feature.get_shapefile_attribute('non_existent_key'))
        self.assertTrue(self.feature.get_shapefile_attribute('non_existent_key', 'default_value') == 'default_value')
    
    # NOTE: Testing of Feature.set_total_reconstruction_pole() and Feature.get_total_reconstruction_pole()
    # is done in 'test_app_logic/test.py".
    
    def test_get_by_name(self):
        properties = self.feature.get(
                pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'),
                pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0].get_value().get_integer() == 300)
        property_values = self.feature.get_value(
                pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'),
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(property_values) == 1)
        self.assertTrue(property_values[0].get_integer() == 300)
        self.assertTrue(
                self.feature.get_value(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder')).get_integer() == 300)
        properties = self.feature.get_value(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 2)
        self.assertTrue(properties[0].get_plate_id() in (1,3))
        self.assertTrue(properties[1].get_plate_id() in (1,3))
        property = self.feature.get_value(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                property_return=pygplates.PropertyReturn.first)
        self.assertTrue(property.get_plate_id() == 1)
        properties = self.feature.get(
                pygplates.PropertyName.create_gpml('conjugatePlateId'),
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0].get_value().get_plate_id() == 2)
        property = self.feature.get(pygplates.PropertyName.create_gpml('plateId'))
        self.assertTrue(property)
        property_value = self.feature.get_value(pygplates.PropertyName.create_gpml('plateId'))
        self.assertFalse(property_value)
        property_value = self.feature.get_value(pygplates.PropertyName.create_gpml('plateId'), 5)
        self.assertTrue(property_value)
        self.assertTrue(property_value.get_plate_id() == 100)
        self.assertTrue(property.get_value(5).get_plate_id() == 100)
        property_value = self.feature.get_value(pygplates.PropertyName.create_gpml('plateId'), 15)
        self.assertTrue(property_value)
        self.assertTrue(property_value.get_plate_id() == 101)
        # No property name existing in feature.
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('not_exists')))
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('not_exists'), pygplates.PropertyReturn.all))
        self.assertFalse(self.feature.get_value(pygplates.PropertyName.create_gpml('not_exists')))
    
    def test_get_by_predicate(self):
        properties = self.feature.get(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'),
                pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0].get_value().get_integer() == 300)
        property_values = self.feature.get_value(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'),
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(property_values) == 1)
        self.assertTrue(property_values[0].get_integer() == 300)
        self.assertTrue(self.feature.get_value(lambda property: property.get_name() ==
                pygplates.PropertyName.create_gpml('subductionZoneSystemOrder')).get_integer() == 300)
        properties = self.feature.get_value(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 2)
        self.assertTrue(properties[0].get_plate_id() in (1,3))
        self.assertTrue(properties[1].get_plate_id() in (1,3))
        properties = self.feature.get_value(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('reconstructionPlateId') and
                    property.get_value().get_plate_id() < 3,
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0].get_plate_id() == 1)
        property = self.feature.get_value(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                property_return=pygplates.PropertyReturn.first)
        self.assertTrue(property.get_plate_id() == 1)
        properties = self.feature.get(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('conjugatePlateId'),
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0].get_value().get_plate_id() == 2)
        property = self.feature.get(lambda property: property.get_name() == pygplates.PropertyName.create_gpml('plateId'))
        self.assertTrue(property)
        property_value = self.feature.get_value(lambda property: property.get_name() == pygplates.PropertyName.create_gpml('plateId'))
        self.assertFalse(property_value)
        property_value = self.feature.get_value(lambda property: property.get_name() == pygplates.PropertyName.create_gpml('plateId'), 5)
        self.assertTrue(property_value)
        self.assertTrue(property_value.get_plate_id() == 100)
        self.assertTrue(property.get_value(5).get_plate_id() == 100)
        property_value = self.feature.get_value(lambda property: property.get_name() == pygplates.PropertyName.create_gpml('plateId'), 15)
        self.assertTrue(property_value)
        self.assertTrue(property_value.get_plate_id() == 101)
        # No property name existing in feature.
        self.assertFalse(self.feature.get(lambda property: property.get_name() == pygplates.PropertyName.create_gpml('not_exists')))
        self.assertFalse(self.feature.get(
                lambda property: property.get_name() == pygplates.PropertyName.create_gpml('not_exists'),
                pygplates.PropertyReturn.all))
        self.assertFalse(self.feature.get_value(lambda property: property.get_name() == pygplates.PropertyName.create_gpml('not_exists')))


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

    def test_get(self):
        point_on_sphere = self.gml_point.get_geometry()
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        point_on_sphere = self.gpml_constant_value_point.get_geometry()
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        multi_point_on_sphere = self.gml_multi_point.get_geometry()
        self.assertTrue(isinstance(multi_point_on_sphere, pygplates.MultiPointOnSphere))
        polyline_on_sphere = self.gml_line_string.get_geometry()
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = self.gml_orientable_curve.get_geometry()
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polyline_on_sphere = self.gpml_constant_value_orientable_curve.get_geometry()
        self.assertTrue(isinstance(polyline_on_sphere, pygplates.PolylineOnSphere))
        polygon_on_sphere = self.gml_polygon.get_geometry()
        self.assertTrue(isinstance(polygon_on_sphere, pygplates.PolygonOnSphere))


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
