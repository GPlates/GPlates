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
                pygplates.PropertyName.gml_description,
                pygplates.XsString('Feature description'))
        self.feature.add(
                pygplates.PropertyName.gml_name,
                pygplates.XsString('Feature name'))
        self.feature.add(
                pygplates.PropertyName.gpml_reconstruction_plate_id,
                pygplates.GpmlPlateId(201))
        self.feature.add(
                pygplates.PropertyName.gpml_conjugate_plate_id,
                pygplates.GpmlPlateId(2))
        self.feature.add(
                pygplates.PropertyName.gpml_reconstruction_plate_id,
                pygplates.GpmlPlateId(801))
        self.feature.add(
                pygplates.PropertyName.gpml_left_plate,
                pygplates.GpmlPlateId(11))
        self.feature.add(
                pygplates.PropertyName.gpml_right_plate,
                pygplates.GpmlPlateId(12))
        self.feature.add(
                pygplates.PropertyName.gpml_relative_plate,
                pygplates.GpmlPlateId(13))
        self.feature.add(
                pygplates.PropertyName.gpml_times,
                pygplates.GpmlArray([
                        pygplates.GmlTimePeriod(10, 0),
                        pygplates.GmlTimePeriod(20, 10),
                        pygplates.GmlTimePeriod(30, 20),
                        pygplates.GmlTimePeriod(40, 30)]))
        self.feature.add(
                pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'),
                pygplates.XsInteger(300))
        self.feature.add(
                pygplates.PropertyName.gpml_subduction_polarity,
                pygplates.Enumeration(
                        pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration'),
                        'Left'))
        self.feature.add(
                pygplates.PropertyName.create_gpml('shipTrackName'),
                pygplates.XsString('Vessel'))
        self.feature.add(
                pygplates.PropertyName.create_gpml('isActive'),
                pygplates.XsBoolean(True))
        self.feature.add(
                pygplates.PropertyName.create_gpml('subductionZoneDepth'),
                pygplates.XsDouble(10.5))
        self.feature.add(
                pygplates.PropertyName.gml_valid_time,
                pygplates.GmlTimePeriod(50, 0))
        self.feature.add(
                pygplates.PropertyName.gpml_geometry_import_time,
                pygplates.GmlTimeInstant(100))
        # A time-dependent property excluding time 0Ma.
        self.feature.add(
                pygplates.PropertyName.create_gpml('plateId'),
                pygplates.GpmlPiecewiseAggregation([
                    pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(100), pygplates.GeoTimeInstant(10), pygplates.GeoTimeInstant(5)),
                    pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(101), pygplates.GeoTimeInstant(20), pygplates.GeoTimeInstant(10))]),
                # 'gpml:plateId' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.feature.add(
                pygplates.PropertyName.gpml_unclassified_geometry,
                pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(0,1,0))))
        self.feature.add(
                pygplates.PropertyName.gpml_center_line_of,
                pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(1,0,0))))
        # Add topological geometries.
        self.topological_sections = [
            pygplates.GpmlTopologicalLineSection(
                pygplates.GpmlPropertyDelegate(
                    pygplates.FeatureId.create_unique_id(),
                    pygplates.PropertyName.gpml_center_line_of,
                    pygplates.GmlLineString),
                False),
            pygplates.GpmlTopologicalLineSection(
                pygplates.GpmlPropertyDelegate(
                    pygplates.FeatureId.create_unique_id(),
                    pygplates.PropertyName.gpml_unclassified_geometry,
                    pygplates.GmlLineString),
                True)]
        self.feature.add(
                pygplates.PropertyName.gpml_boundary,
                pygplates.GpmlConstantValue(pygplates.GpmlTopologicalPolygon(self.topological_sections)))
        self.feature.add(
                pygplates.PropertyName.gpml_unclassified_geometry,
                pygplates.GpmlConstantValue(pygplates.GpmlTopologicalLine(self.topological_sections)))
        self.topological_interiors = [
            pygplates.GpmlPropertyDelegate(
                pygplates.FeatureId.create_unique_id(),
                pygplates.PropertyName.gpml_position,
                pygplates.GmlPoint),
            pygplates.GpmlPropertyDelegate(
                pygplates.FeatureId.create_unique_id(),
                pygplates.PropertyName.gpml_position,
                pygplates.GmlPoint)]
        self.feature.add(
                pygplates.PropertyName.gpml_network,
                pygplates.GpmlConstantValue(pygplates.GpmlTopologicalNetwork(self.topological_sections, self.topological_interiors)))
    
    def test_get_and_set_geometry(self):
        # The default geometry property name for an unclassified feature type is 'gpml:unclassifiedGeometry'.
        geometry_properties = self.feature.get_geometry(property_return=pygplates.PropertyReturn.all)
        self.assertTrue(geometry_properties == self.feature.get_geometries())
        self.assertTrue(geometry_properties == self.feature.get_geometries(coverage_return=pygplates.CoverageReturn.geometry_only))
        self.assertTrue(geometry_properties ==
                self.feature.get_geometry(property_return=pygplates.PropertyReturn.all, coverage_return=pygplates.CoverageReturn.geometry_only))
        self.assertTrue(len(geometry_properties) == 1)
        self.assertTrue(isinstance(geometry_properties[0], pygplates.PointOnSphere))
        self.assertTrue(isinstance(self.feature.get_geometry(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(self.feature.get_geometry(property_return=pygplates.PropertyReturn.first), pygplates.PointOnSphere))
        self.assertTrue(len(self.feature.get_geometry(lambda property: True, pygplates.PropertyReturn.all)) == 2) # There are two geometries in total.
        self.assertTrue(self.feature.get_geometry(lambda property: True, pygplates.PropertyReturn.all) == self.feature.get_all_geometries())
        self.feature.add(
                pygplates.PropertyName.gpml_position,
                pygplates.GpmlConstantValue(pygplates.GmlPoint(pygplates.PointOnSphere(0,0,1))))
        self.assertTrue(self.feature.get_geometry(pygplates.PropertyName.gpml_position) == pygplates.PointOnSphere(0,0,1))
        # There should now be three geometry properties.
        self.assertTrue(len(self.feature.get_all_geometries()) == 3)
        self.feature.remove(pygplates.PropertyName.gpml_position)
        # There should now be two geometry properties again.
        self.assertTrue(len(self.feature.get_all_geometries()) == 2)
        self.assertFalse(self.feature.get_geometry(lambda property: True)) # Only succeeds if one geometry.
        # Remove the default geometry property.
        self.feature.remove(pygplates.PropertyName.gpml_unclassified_geometry)
        self.assertFalse(self.feature.get_geometry())
        gml_orientable_curve = pygplates.GmlOrientableCurve(pygplates.GmlLineString(
            pygplates.PolylineOnSphere([pygplates.PointOnSphere(1,0,0), pygplates.PointOnSphere(0,1,0)])))
        self.feature.add(
                pygplates.PropertyName.gpml_unclassified_geometry,
                pygplates.GpmlConstantValue(gml_orientable_curve))
        self.assertTrue(isinstance(self.feature.get_geometry(), pygplates.PolylineOnSphere))
        
        point = pygplates.PointOnSphere(1,0,0)
        self.feature.set_geometry(point)
        self.assertTrue(self.feature.get_geometry() == point)
        
        # Test reverse reconstruction of geometry.
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        self.feature.set_geometry(point)
        pygplates.reverse_reconstruct(self.feature, rotation_model, 10.0)
        point1 = self.feature.get_geometry()
        self.feature.set_geometry(point, reverse_reconstruct=(rotation_model, 10.0))
        point2 = self.feature.get_geometry()
        self.assertTrue(point1 == point2)
        self.feature.set_geometry(point, reverse_reconstruct=(rotation_model, 10.0, 0))
        point3 = self.feature.get_geometry()
        self.assertTrue(point1 == point3)
        # Raises error if reverse_reconstruct parameters in wrong order.
        self.assertRaises(TypeError,
                pygplates.Feature.set_geometry, self.feature, point, reverse_reconstruct=(10.0, rotation_model))
        
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
        self.feature.set_geometry(point, pygplates.PropertyName.gpml_position)
        self.assertTrue(self.feature.get_geometry(pygplates.PropertyName.gpml_position) == point)
        hot_spot_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('HotSpot'))
        self.assertTrue(hot_spot_feature.set_geometry(
                point, pygplates.PropertyName.gpml_position).get_value().get_geometry() == point)
        # Cannot set multiple points on 'gpml:position' property on a 'gpml:HotSpot' feature (or any feature but 'gpml:UnclassifiedFeature').
        self.assertRaises(pygplates.InformationModelError,
                hot_spot_feature.set_geometry, (point, point), pygplates.PropertyName.gpml_position)
        # ...unless we don't check the information model.
        self.assertTrue(
                len(hot_spot_feature.set_geometry((point, point), pygplates.PropertyName.gpml_position,
                        verify_information_model=pygplates.VerifyInformationModel.no)) == 2)
        # Setting multiple points on 'gpml:position' property on a 'gpml:UnclassifiedFeature' feature is fine though.
        points = self.feature.set_geometry((point, point), pygplates.PropertyName.gpml_position)
        self.assertTrue(len(points) == 2)
        self.assertTrue(points[0].get_value().get_geometry() == point)
        self.assertTrue(points[1].get_value().get_geometry() == point)
        # Cannot set a multipoint on 'gpml:position'.
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_geometry, multi_point, pygplates.PropertyName.gpml_position)
        # ...unless we don't check the information model.
        self.assertTrue(self.feature.set_geometry(
                multi_point, pygplates.PropertyName.gpml_position, verify_information_model=pygplates.VerifyInformationModel.no))
        
        # Remove all *default* geometries.
        self.feature.set_geometry([])
        # There are now no *default* geometries.
        self.assertTrue(len(self.feature.get_geometry(property_return=pygplates.PropertyReturn.all)) == 0)
        self.assertFalse(self.feature.get_geometry(property_return=pygplates.PropertyReturn.first))
        self.assertFalse(self.feature.get_geometry())
        # However there is still a non-default geometry 'gpml:position' and the original 'gpml:centerLineOf'.
        self.assertTrue(self.feature.get_geometry(pygplates.PropertyName.gpml_position) == multi_point)
        # We don't equality test with the original 'gpml:centerLineOf' since it got reverse reconstructed.
        self.assertTrue(len(self.feature.get_geometries(pygplates.PropertyName.gpml_center_line_of)) == 1)

    def test_get_and_set_coverage(self):
        # The default geometry property name for an unclassified feature type is 'gpml:unclassifiedGeometry'.
        # But there is no coverage range associated with it.
        coverages = self.feature.get_geometry(
                property_return=pygplates.PropertyReturn.all, coverage_return=pygplates.CoverageReturn.geometry_and_scalars)
        self.assertTrue(coverages == self.feature.get_geometries(coverage_return=pygplates.CoverageReturn.geometry_and_scalars))
        self.assertTrue(not coverages)
        
        # Add a coverage range (scalar values) with number of scalars matching points in geometry (ie, one).
        velocity_colat_type = pygplates.ScalarType.create_gpml('VelocityColat')
        velocity_lon_type = pygplates.ScalarType.create_gpml('VelocityLon')
        velocity_colat_values = [1]
        velocity_lon_values = [10]
        velocity_colat_lon_dict = {velocity_colat_type : velocity_colat_values, velocity_lon_type : velocity_lon_values}
        gml_data_block = self.feature.add(
                pygplates.PropertyName.create_gpml('centerLineOfCoverage'),
                pygplates.GmlDataBlock(velocity_colat_lon_dict))
        self.assertTrue(gml_data_block.get_value().get_scalar_values(velocity_lon_type) == velocity_lon_values)
        self.assertTrue(self.feature.get_geometry(
                pygplates.PropertyName.gpml_center_line_of,
                coverage_return=pygplates.CoverageReturn.geometry_and_scalars) ==
                        (pygplates.PointOnSphere(1,0,0), velocity_colat_lon_dict))
        self.assertTrue(self.feature.get_geometry(
                pygplates.PropertyName.gpml_center_line_of,
                pygplates.PropertyReturn.first,
                pygplates.CoverageReturn.geometry_and_scalars) ==
                        (pygplates.PointOnSphere(1,0,0), velocity_colat_lon_dict))
        self.assertTrue(self.feature.get_geometry(
                pygplates.PropertyName.gpml_center_line_of,
                pygplates.PropertyReturn.all,
                pygplates.CoverageReturn.geometry_and_scalars) ==
                        [(pygplates.PointOnSphere(1,0,0), velocity_colat_lon_dict)])
        self.assertTrue(self.feature.get_geometries(
                pygplates.PropertyName.gpml_center_line_of,
                pygplates.CoverageReturn.geometry_and_scalars) ==
                        [(pygplates.PointOnSphere(1,0,0), velocity_colat_lon_dict)])
        self.assertTrue(self.feature.get_all_geometries(
                pygplates.CoverageReturn.geometry_and_scalars) ==
                        [(pygplates.PointOnSphere(1,0,0), velocity_colat_lon_dict)])
        # Remove the domain of the coverage range.
        self.feature.remove(pygplates.PropertyName.gpml_center_line_of)
        # Should not have any coverages.
        self.assertTrue(not self.feature.get_geometries(
                pygplates.PropertyName.gpml_center_line_of,
                pygplates.CoverageReturn.geometry_and_scalars))
        self.assertTrue(not self.feature.get_geometries(
                coverage_return=pygplates.CoverageReturn.geometry_and_scalars))
        self.assertTrue(not self.feature.get_all_geometries(
                pygplates.CoverageReturn.geometry_and_scalars))
        # But still have the coverage range property.
        self.assertTrue(self.feature.get(pygplates.PropertyName.create_gpml('centerLineOfCoverage')))
        # There should now be one geometry property.
        self.assertTrue(len(self.feature.get_all_geometries()) == 1)
        # Setting only the geometry should remove coverage range.
        self.feature.set_geometry(pygplates.PointOnSphere(1,0,0), pygplates.PropertyName.gpml_center_line_of)
        self.assertTrue(len(self.feature.get_all_geometries()) == 2)
        self.assertFalse(self.feature.get_geometry(
                pygplates.PropertyName.gpml_center_line_of,
                coverage_return=pygplates.CoverageReturn.geometry_and_scalars))
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('centerLineOfCoverage')))
        
        velocity_domain = pygplates.PolylineOnSphere([(0,0), (0,10), (10,0)])
        velocity_colat_values = [1, 2, 3]
        velocity_lon_values = [10, 20, 30]
        velocity_colat_lon_dict = {velocity_colat_type : velocity_colat_values, velocity_lon_type : velocity_lon_values}
        # Mismatching number of points/scalars in domain/range.
        self.assertRaises(ValueError, pygplates.Feature.set_geometry, self.feature,
                (pygplates.PolylineOnSphere([(0,0), (10,0)]), velocity_colat_lon_dict))
        self.feature.set_geometry((velocity_domain, velocity_colat_lon_dict))
        self.assertTrue(self.feature.get_geometry(coverage_return=pygplates.CoverageReturn.geometry_and_scalars) ==
                (velocity_domain, velocity_colat_lon_dict))
        
        # Test reverse reconstruction of coverage domain.
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        pygplates.reverse_reconstruct(self.feature, rotation_model, 10.0)
        reversed_velocity_domain, scalars_dict = self.feature.get_geometry(coverage_return=pygplates.CoverageReturn.geometry_and_scalars)
        self.feature.set_geometry((velocity_domain, scalars_dict), reverse_reconstruct=(rotation_model, 10.0))
        self.assertTrue((reversed_velocity_domain, scalars_dict) ==
                self.feature.get_geometry(coverage_return=pygplates.CoverageReturn.geometry_and_scalars))
        # Raises error if reverse_reconstruct parameters in wrong order.
        self.assertRaises(TypeError,
                pygplates.Feature.set_geometry, self.feature, (velocity_domain, scalars_dict), reverse_reconstruct=(10.0, rotation_model))
        
        # Same number of domain points but on a different geometry (domain) property name is OK (but not on same property name).
        spreading_rate_domain = pygplates.MultiPointOnSphere([(5,0), (5,10), (10,5), (10, 0)])
        spreading_rate_type = pygplates.ScalarType.create_gpml('SpreadingRate')
        spreading_rate_values = [1, 2, 3, 4]
        spreading_rate_dict = {spreading_rate_type : spreading_rate_values}
        spreading_rate_domain_property, spreading_rate_range_property = self.feature.set_geometry(
                (spreading_rate_domain, spreading_rate_dict), pygplates.PropertyName.gpml_center_line_of)
        self.assertTrue(spreading_rate_domain_property.get_value().get_geometry() == spreading_rate_domain)
        self.assertTrue(spreading_rate_range_property.get_value().get_scalar_values(spreading_rate_type) == spreading_rate_values)
        self.assertTrue(self.feature.get_geometry(
                pygplates.PropertyName.gpml_center_line_of,
                coverage_return=pygplates.CoverageReturn.geometry_and_scalars) ==
                        (spreading_rate_domain, spreading_rate_dict))
        self.assertTrue(len(self.feature.get_all_geometries(pygplates.CoverageReturn.geometry_and_scalars)) == 2)
        self.feature.set_geometry([])
        self.assertTrue(len(self.feature.get_all_geometries(pygplates.CoverageReturn.geometry_and_scalars)) == 1)
        spreading_rate_property_list = self.feature.set_geometry([], pygplates.PropertyName.gpml_center_line_of)
        self.assertTrue(len(spreading_rate_property_list) == 0)
        self.assertTrue(len(self.feature.get_all_geometries(pygplates.CoverageReturn.geometry_and_scalars)) == 0)
        
        coverage_list = [(spreading_rate_domain, spreading_rate_dict), (velocity_domain, velocity_colat_lon_dict)]
        spreading_rate_property_list = self.feature.set_geometry(coverage_list)
        self.assertTrue(len(spreading_rate_property_list) == 2)
        self.assertTrue(spreading_rate_property_list[0][0].get_value().get_geometry() == spreading_rate_domain)
        self.assertTrue(spreading_rate_property_list[0][1].get_value().get_scalar_values(spreading_rate_type) == spreading_rate_values)
        self.assertTrue(spreading_rate_property_list[1][0].get_value().get_geometry() == velocity_domain)
        self.assertTrue(spreading_rate_property_list[1][1].get_value().get_scalar_values(velocity_colat_type) == velocity_colat_values)
        coverages = self.feature.get_geometries(coverage_return=pygplates.CoverageReturn.geometry_and_scalars)
        self.assertTrue(len(coverages) == 2)
        self.assertTrue(coverages[0] in coverage_list)
        self.assertTrue(coverages[1] in coverage_list)
        
        # Mismatching number of points/scalars in domain/range.
        self.assertRaises(ValueError, pygplates.Feature.set_geometry, self.feature,
                [(pygplates.MultiPointOnSphere([(0,0), (10,0)]), {spreading_rate_type : [1,2]}),
                (pygplates.MultiPointOnSphere([(0,0), (10,0), (0,10)]), {spreading_rate_type : [1,2,3,4]})])
        # Can't have multiple domains with same number of geometry points.
        self.assertRaises(pygplates.AmbiguousGeometryCoverageError, pygplates.Feature.set_geometry, self.feature,
                [(pygplates.MultiPointOnSphere([(0,0), (10,0)]), {spreading_rate_type : [1,2]}),
                (pygplates.MultiPointOnSphere([(0,0), (0,10)]), {spreading_rate_type : [3,4]})])
        
        hot_spot_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('HotSpot'))
        hot_spot_coverage_property = hot_spot_feature.set_geometry(
                (spreading_rate_domain, spreading_rate_dict),
                pygplates.PropertyName.gpml_multi_position)
        self.assertTrue(
                (hot_spot_coverage_property[0].get_value(), hot_spot_coverage_property[1].get_value()) ==
                    (pygplates.GmlMultiPoint(spreading_rate_domain), pygplates.GmlDataBlock(spreading_rate_dict)))
        # Cannot set multiple geometries on 'gpml:multiPosition' property on a 'gpml:HotSpot' feature (or any feature but 'gpml:UnclassifiedFeature').
        self.assertRaises(pygplates.InformationModelError,
                hot_spot_feature.set_geometry,
                ((spreading_rate_domain, spreading_rate_dict), (pygplates.MultiPointOnSphere([(0,0)]), {spreading_rate_type : [0]})),
                pygplates.PropertyName.gpml_multi_position)
        # ...unless we don't check the information model.
        self.assertTrue(
                len(hot_spot_feature.set_geometry(
                    [(spreading_rate_domain, spreading_rate_dict), (pygplates.MultiPointOnSphere([(0,0)]), {spreading_rate_type : [0]})],
                    pygplates.PropertyName.gpml_multi_position,
                        verify_information_model=pygplates.VerifyInformationModel.no)) == 2)
        # Setting multiple points on 'gpml:multiPosition' property on a 'gpml:UnclassifiedFeature' feature is fine though.
        self.assertTrue(len(self.feature.set_geometry(
                ((spreading_rate_domain, spreading_rate_dict), (pygplates.MultiPointOnSphere([(0,0)]), {spreading_rate_type : [0]})),
                pygplates.PropertyName.gpml_multi_position)) == 2)
        # Cannot set a multipoint coverage on 'gpml:position'.
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_geometry,
                (spreading_rate_domain, spreading_rate_dict),
                pygplates.PropertyName.gpml_position)
        # ...even we don't check the information model (because 'gpml:position' does not support coverages).
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_geometry,
                (spreading_rate_domain, spreading_rate_dict),
                pygplates.PropertyName.gpml_position,
                verify_information_model=pygplates.VerifyInformationModel.no)
    
    def test_get_and_set_topological_geometry(self):
        # The topological polygon property is stored in 'gpml:boundary' property.
        topological_boundary_properties = self.feature.get_topological_geometry(pygplates.PropertyName.gpml_boundary, property_return=pygplates.PropertyReturn.all)
        self.assertTrue(topological_boundary_properties == self.feature.get_topological_geometries(pygplates.PropertyName.gpml_boundary))
        self.assertTrue(len(topological_boundary_properties) == 1)
        self.assertTrue(isinstance(topological_boundary_properties[0], pygplates.GpmlTopologicalPolygon))
        self.assertTrue(isinstance(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_boundary), pygplates.GpmlTopologicalPolygon))
        self.assertTrue(isinstance(self.feature.get_topological_geometry(
            pygplates.PropertyName.gpml_boundary, property_return=pygplates.PropertyReturn.first), pygplates.GpmlTopologicalPolygon))
        self.assertTrue(list(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_boundary).get_boundary_sections()) == self.topological_sections)
        self.assertTrue(list(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_boundary).get_exterior_sections()) == self.topological_sections)
        self.assertTrue(len(self.feature.get_topological_geometry(lambda property: True, pygplates.PropertyReturn.all)) == 3) # There are three topological geometries in total.
        self.assertTrue(self.feature.get_topological_geometry(lambda property: True, pygplates.PropertyReturn.all) == self.feature.get_all_topological_geometries())
        self.feature.add(
                pygplates.PropertyName.gpml_unclassified_geometry,
                pygplates.GpmlConstantValue(pygplates.GpmlTopologicalLine(self.topological_sections)))
        # There should now be two default topological geometry properties.
        self.assertTrue(len(self.feature.get_topological_geometries()) == 2)
        # There should now be four topological geometry properties.
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 4)
        self.feature.remove(
                lambda property: property.get_name() == pygplates.PropertyName.gpml_unclassified_geometry and
                     isinstance(property.get_value(), pygplates.GpmlTopologicalLine))
        # There should now be zero default topological geometry properties.
        self.assertTrue(len(self.feature.get_topological_geometries()) == 0)
        # There should now be two topological geometry properties again.
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 2)
        # This shouldn't have removed the non-topological unclassified geometry though.
        self.assertTrue(len(self.feature.get_geometries(pygplates.PropertyName.gpml_unclassified_geometry)) == 1)
        self.assertFalse(self.feature.get_topological_geometry(lambda property: True)) # Only succeeds if one topological geometry.
        self.feature.add(
                pygplates.PropertyName.gpml_unclassified_geometry,
                pygplates.GpmlConstantValue(pygplates.GpmlTopologicalLine(self.topological_sections)))
        # There should now be one default topological geometry property.
        self.assertTrue(len(self.feature.get_topological_geometries()) == 1)
        self.assertTrue(list(self.feature.get_topological_geometry().get_sections()) == self.topological_sections)
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 3)
        
        line_topological_sections = [
            pygplates.GpmlTopologicalLineSection(
                pygplates.GpmlPropertyDelegate(
                    pygplates.FeatureId.create_unique_id(),
                    pygplates.PropertyName.gpml_center_line_of,
                    pygplates.GmlLineString),
                False),
            pygplates.GpmlTopologicalLineSection(
                pygplates.GpmlPropertyDelegate(
                    pygplates.FeatureId.create_unique_id(),
                    pygplates.PropertyName.gpml_center_line_of,
                    pygplates.GmlLineString),
                False)]
        self.feature.set_topological_geometry(pygplates.GpmlTopologicalLine(line_topological_sections))
        # There should still be one default topological geometry property.
        # It got overwritten.
        self.assertTrue(len(self.feature.get_topological_geometries()) == 1)
        self.assertTrue(list(self.feature.get_topological_geometry().get_sections()) == line_topological_sections)
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 3)
        
        self.assertTrue(isinstance(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_network), pygplates.GpmlTopologicalNetwork))
        self.assertTrue(list(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_network).get_boundary_sections()) ==
            self.topological_sections)
        self.assertTrue(list(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_network).get_interiors()) ==
            self.topological_interiors)
        self.assertTrue(len(self.feature.get_topological_geometries(pygplates.PropertyName.gpml_network)) == 1)
        self.feature.set_topological_geometry(
            pygplates.GpmlTopologicalNetwork(line_topological_sections, self.topological_interiors),
            pygplates.PropertyName.gpml_network)
        # There should still be one topological network property.
        # It got overwritten.
        self.assertTrue(len(self.feature.get_topological_geometries(pygplates.PropertyName.gpml_network)) == 1)
        self.assertTrue(list(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_network).get_boundary_sections()) == line_topological_sections)
        self.assertTrue(list(self.feature.get_topological_geometry(pygplates.PropertyName.gpml_network).get_interiors()) == self.topological_interiors)
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 3)
        # Cannot set a topological *network* on a 'gpml:center_line_of' property.
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_topological_geometry,
                pygplates.GpmlTopologicalNetwork(line_topological_sections, self.topological_interiors),
                pygplates.PropertyName.gpml_center_line_of)
        
        # Set multiple topological geometries.
        topological_geometries = self.feature.set_topological_geometry([
                pygplates.GpmlTopologicalLine(line_topological_sections),
                pygplates.GpmlTopologicalPolygon(self.topological_sections)])
        self.assertTrue(len(topological_geometries) == 2)
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 4)
        topological_network_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('TopologicalNetwork'))
        network = pygplates.GpmlTopologicalNetwork(line_topological_sections, self.topological_interiors)
        self.assertTrue(topological_network_feature.set_topological_geometry(network).get_value() == network)
        # Cannot set multiple topological networks on 'gpml:network' property on a 'gpml:TopologicalNetwork' feature (or any feature but 'gpml:UnclassifiedFeature').
        # Note that 'gpml:network' is the default geometry property on a 'gpml:TopologicalNetwork' (so we don't have to specify it).
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_topological_geometry,
                [network, network])
        # ...unless we don't check the information model.
        self.assertTrue(
                len(topological_network_feature.set_topological_geometry((network, network),
                        verify_information_model=pygplates.VerifyInformationModel.no)) == 2)
        # Setting multiple topological networks on 'gpml:network' property on a 'gpml:UnclassifiedFeature' feature is fine though.
        network_properties = self.feature.set_topological_geometry((network, network), pygplates.PropertyName.gpml_network)
        self.assertTrue(len(network_properties) == 2)
        self.assertTrue(network_properties[0].get_value() == network)
        self.assertTrue(network_properties[1].get_value() == network)
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 5)
        
        # Remove all *default* topological geometries.
        self.feature.set_topological_geometry([])
        # There are now no *default* topological geometries.
        self.assertTrue(len(self.feature.get_topological_geometry(property_return=pygplates.PropertyReturn.all)) == 0)
        self.assertFalse(self.feature.get_topological_geometry(property_return=pygplates.PropertyReturn.first))
        self.assertFalse(self.feature.get_topological_geometry())
        self.assertTrue(len(self.feature.get_all_topological_geometries()) == 3)
        # However there are still two non-default topological network geometries 'gpml:network' and a topological polygon 'gpml:boundary'.
        self.assertTrue(len(self.feature.get_topological_geometries(pygplates.PropertyName.gpml_network)) == 2)
        self.assertTrue(len(self.feature.get_topological_geometries(pygplates.PropertyName.gpml_boundary)) == 1)
    
    def test_get_and_set_enumeration(self):
        subduction_polarity = self.feature.get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
        self.assertTrue(subduction_polarity == 'Left')
        self.feature.remove(pygplates.PropertyName.gpml_subduction_polarity)
        # With property removed it should return default.
        subduction_polarity = self.feature.get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
        self.assertTrue(subduction_polarity is None)
        subduction_polarity = self.feature.get_enumeration(pygplates.PropertyName.gpml_subduction_polarity, 'Unknown')
        self.assertTrue(subduction_polarity == 'Unknown')
        
        self.feature.set_enumeration(pygplates.PropertyName.gpml_primary_slip_component, 'DipSlip')
        self.assertTrue(self.feature.get_enumeration(pygplates.PropertyName.gpml_primary_slip_component) == 'DipSlip')
        gpml_primary_slip_component = self.feature.set_enumeration(pygplates.PropertyName.gpml_primary_slip_component, 'StrikeSlip')
        self.assertTrue(gpml_primary_slip_component.get_value().get_content() == 'StrikeSlip')
        self.assertTrue(self.feature.get_enumeration(pygplates.PropertyName.gpml_primary_slip_component) == 'StrikeSlip')
        
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_enumeration, pygplates.PropertyName.create_gpml('invalid'), 'arbitrary')
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_enumeration, pygplates.PropertyName.gpml_primary_slip_component, 'invalid')
        self.feature.set_enumeration(pygplates.PropertyName.gpml_primary_slip_component, 'invalid', pygplates.VerifyInformationModel.no)
        self.assertTrue(self.feature.get_enumeration(pygplates.PropertyName.gpml_primary_slip_component) == 'invalid')
        self.feature.set_enumeration(pygplates.PropertyName.gpml_primary_slip_component, 'StrikeSlip')
        self.assertTrue(self.feature.get_enumeration(pygplates.PropertyName.gpml_primary_slip_component) == 'StrikeSlip')
        self.feature.set_enumeration(pygplates.PropertyName.gpml_subduction_polarity, 'Right')
        self.assertTrue(self.feature.get_enumeration(pygplates.PropertyName.gpml_subduction_polarity) == 'Right')
    
    def test_get_and_set_string(self):
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName')) == 'Vessel')
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName'), [], pygplates.PropertyReturn.all) == ['Vessel'])
        self.feature.remove(pygplates.PropertyName.create_gpml('shipTrackName'))
        # With property removed it should return default.
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName')) == '')
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName'), 'Unknown') == 'Unknown')
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName'), None) is None)
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName'), [], pygplates.PropertyReturn.all) == [])
        
        self.feature.set_string(pygplates.PropertyName.create_gpml('shipTrackName'), 'TestShip')
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName')) == 'TestShip')
        gpml_ship_track_name = self.feature.set_string(pygplates.PropertyName.create_gpml('shipTrackName'), ['TestShip2'])
        self.assertTrue(len(gpml_ship_track_name) == 1)
        self.assertTrue(gpml_ship_track_name[0].get_value().get_string() == 'TestShip2')
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName')) == 'TestShip2')
        
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_string, pygplates.PropertyName.create_gpml('invalid'), 'arbitrary')
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_string, pygplates.PropertyName.create_gpml('reconstructionPlateId'), 'arbitrary')
        self.assertRaises(TypeError,
                self.feature.set_string, pygplates.PropertyName.create_gpml('shipTrackName'), 1)
        # Use 'gpml:MagneticAnomalyIdentification' feature type when checking multiplicity.
        # Because 'gpml:UnclassifiedFeature' feature type is a special case allows any multiplicity.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature(pygplates.FeatureType.gpml_magnetic_anomaly_identification).set_string,
                pygplates.PropertyName.create_gpml('shipTrackName'),
                ['arbitrary1', 'arbitrary2'])
        self.feature.set_string(pygplates.PropertyName.create_gpml('reconstructionPlateId'), 'arbitrary', pygplates.VerifyInformationModel.no)
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('reconstructionPlateId')) == 'arbitrary')
        self.assertTrue(self.feature.get_string(pygplates.PropertyName.create_gpml('shipTrackName')) == 'TestShip2')
    
    def test_get_and_set_boolean(self):
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive')) == True)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive'), [], pygplates.PropertyReturn.all) == [True])
        self.feature.remove(pygplates.PropertyName.create_gpml('isActive'))
        # With property removed it should return default.
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive')) == False)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive'), None) is None)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive'), [], pygplates.PropertyReturn.all) == [])
        
        self.feature.set_boolean(pygplates.PropertyName.create_gpml('isActive'), False)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive'), None) == False)
        gpml_is_active = self.feature.set_boolean(pygplates.PropertyName.create_gpml('isActive'), [True])
        self.assertTrue(len(gpml_is_active) == 1)
        self.assertTrue(gpml_is_active[0].get_value().get_boolean() == True)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive')) == True)
        
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_boolean, pygplates.PropertyName.create_gpml('invalid'), True)
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_boolean, pygplates.PropertyName.create_gpml('reconstructionPlateId'), True)
        self.assertRaises(TypeError,
                self.feature.set_boolean, pygplates.PropertyName.create_gpml('isActive'), 'True')
        self.feature.set_boolean(pygplates.PropertyName.create_gpml('reconstructionPlateId'), True, pygplates.VerifyInformationModel.no)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('reconstructionPlateId')) == True)
        self.assertTrue(self.feature.get_boolean(pygplates.PropertyName.create_gpml('isActive')) == True)
    
    def test_get_and_set_integer(self):
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder')) == 300)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), [], pygplates.PropertyReturn.all) == [300])
        self.feature.remove(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'))
        # With property removed it should return default.
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder')) == 0)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), None) is None)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), [], pygplates.PropertyReturn.all) == [])
        
        self.feature.set_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), 20)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), None) == 20)
        gpml_subduction_zone_system_order = self.feature.set_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), [30])
        self.assertTrue(len(gpml_subduction_zone_system_order) == 1)
        self.assertTrue(gpml_subduction_zone_system_order[0].get_value().get_integer() == 30)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder')) == 30)
        
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_integer, pygplates.PropertyName.create_gpml('invalid'), 5)
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_integer, pygplates.PropertyName.create_gpml('reconstructionPlateId'), 5)
        self.assertRaises(TypeError,
                self.feature.set_integer, pygplates.PropertyName.create_gpml('subductionZoneSystemOrder'), 'True')
        self.feature.set_integer(pygplates.PropertyName.create_gpml('reconstructionPlateId'), 5, pygplates.VerifyInformationModel.no)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('reconstructionPlateId')) == 5)
        self.assertTrue(self.feature.get_integer(pygplates.PropertyName.create_gpml('subductionZoneSystemOrder')) == 30)
    
    def test_get_and_set_double(self):
        self.assertAlmostEqual(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth')), 10.5)
        self.feature.remove(pygplates.PropertyName.create_gpml('subductionZoneDepth'))
        # With property removed it should return default.
        self.assertAlmostEqual(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth')), 0)
        self.assertTrue(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth'), None) is None)
        self.assertTrue(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth'), [], pygplates.PropertyReturn.all) == [])
        
        self.feature.set_double(pygplates.PropertyName.create_gpml('subductionZoneDepth'), 20.4)
        self.assertAlmostEqual(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth'), None), 20.4)
        gpml_subduction_zone_depth = self.feature.set_double(pygplates.PropertyName.create_gpml('subductionZoneDepth'), [30.6])
        self.assertTrue(len(gpml_subduction_zone_depth) == 1)
        self.assertAlmostEqual(gpml_subduction_zone_depth[0].get_value().get_double(), 30.6)
        self.assertAlmostEqual(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth')), 30.6)
        
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_double, pygplates.PropertyName.create_gpml('invalid'), 5.5)
        self.assertRaises(pygplates.InformationModelError,
                self.feature.set_double, pygplates.PropertyName.create_gpml('reconstructionPlateId'), 5.5)
        self.assertRaises(TypeError,
                self.feature.set_double, pygplates.PropertyName.create_gpml('subductionZoneDepth'), 'True')
        self.feature.set_double(pygplates.PropertyName.create_gpml('reconstructionPlateId'), 5.6, pygplates.VerifyInformationModel.no)
        self.assertAlmostEqual(self.feature.get_double(pygplates.PropertyName.create_gpml('reconstructionPlateId')), 5.6)
        self.assertAlmostEqual(self.feature.get_double(pygplates.PropertyName.create_gpml('subductionZoneDepth')), 30.6)
    
    def test_get_and_set_description(self):
        description = self.feature.get_description()
        self.assertTrue(description == 'Feature description')
        self.feature.remove(pygplates.PropertyName.gml_description)
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
        self.feature.remove(pygplates.PropertyName.gml_name)
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
        self.feature.remove(pygplates.PropertyName.gml_name)
        # With property removed it should return default of an empty list.
        self.assertTrue(self.feature.get_name([], pygplates.PropertyReturn.all) == [])
        self.assertFalse(self.feature.get_name([], pygplates.PropertyReturn.all))
        self.assertFalse(self.feature.get_name(None, pygplates.PropertyReturn.all))
        self.feature.add(pygplates.PropertyName.gml_name, pygplates.XsString('Feature name'))
        self.feature.add(pygplates.PropertyName.gml_name, pygplates.XsString(''))
        # The name with the empty string will still get returned.
        names = self.feature.get_name([], pygplates.PropertyReturn.all)
        self.assertTrue('Feature name' in names)
        self.assertTrue('' in names)
        # If we ask for exactly one name then we'll get none (even though one of the two names is an empty string).
        self.assertFalse(self.feature.get_name())
        self.feature.remove(pygplates.PropertyName.gml_name)
        self.feature.add(pygplates.PropertyName.gml_name, pygplates.XsString(''))
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
        self.feature.remove(pygplates.PropertyName.gml_valid_time)
        # With property removed it should return default of all time.
        begin_time, end_time = self.feature.get_valid_time()
        self.assertTrue(pygplates.GeoTimeInstant(begin_time).is_distant_past())
        self.assertTrue(pygplates.GeoTimeInstant(end_time).is_distant_future())
        self.feature.set_valid_time(begin_time, end_time)
        self.assertTrue(self.feature.get_valid_time(None))
        begin_time, end_time = self.feature.get_valid_time()
        self.assertTrue(pygplates.GeoTimeInstant(begin_time).is_distant_past())
        self.assertTrue(pygplates.GeoTimeInstant(end_time).is_distant_future())
        self.feature.remove(pygplates.PropertyName.gml_valid_time)
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_valid_time(None))
        
        self.feature.set_valid_time(pygplates.GeoTimeInstant.create_distant_past(), 0)
        self.assertTrue(self.feature.get_valid_time() == (pygplates.GeoTimeInstant.create_distant_past(), 0))
        gml_valid_time = self.feature.set_valid_time(100, 0)
        self.assertTrue(gml_valid_time.get_value().get_begin_time() == 100)
        self.assertTrue(gml_valid_time.get_value().get_end_time() == 0)
        self.assertTrue(self.feature.get_valid_time() == (100, 0))
    
    def test_is_valid_at_time(self):
        begin_time, end_time = self.feature.get_valid_time()
        self.assertTrue(self.feature.is_valid_at_time(begin_time))
        self.assertTrue(self.feature.is_valid_at_time(end_time))
        self.assertTrue(self.feature.is_valid_at_time(10))
        self.assertTrue(self.feature.is_valid_at_time(pygplates.GeoTimeInstant(10)))
        self.assertFalse(self.feature.is_valid_at_time(60))
        self.assertFalse(self.feature.is_valid_at_time(pygplates.GeoTimeInstant(60)))
        self.feature.remove(pygplates.PropertyName.gml_valid_time)
        # With property removed it should return default of all time.
        self.assertTrue(self.feature.is_valid_at_time(60))
        self.assertTrue(self.feature.is_valid_at_time(pygplates.GeoTimeInstant(60)))
        self.assertTrue(self.feature.is_valid_at_time(pygplates.GeoTimeInstant.create_distant_past()))
        self.assertTrue(self.feature.is_valid_at_time(pygplates.GeoTimeInstant.create_distant_future()))
        self.assertTrue(self.feature.is_valid_at_time(float('inf')))
        self.assertTrue(self.feature.is_valid_at_time(float('-inf')))
    
    def test_get_and_set_geometry_import_time(self):
        geometry_import_time = self.feature.get_geometry_import_time()
        self.assertTrue(pygplates.GeoTimeInstant(geometry_import_time) == 100)
        self.feature.remove(pygplates.PropertyName.gpml_geometry_import_time)
        # With property removed it should return default of zero.
        geometry_import_time = self.feature.get_geometry_import_time()
        self.assertTrue(pygplates.GeoTimeInstant(geometry_import_time) == 0)
        self.feature.set_geometry_import_time(geometry_import_time)
        self.assertTrue(self.feature.get_geometry_import_time(None) is not None)
        geometry_import_time = self.feature.get_geometry_import_time()
        self.assertTrue(pygplates.GeoTimeInstant(geometry_import_time) == 0)
        self.feature.remove(pygplates.PropertyName.gpml_geometry_import_time)
        # With property removed it should return our default (None).
        self.assertTrue(self.feature.get_geometry_import_time(None) is None)
        
        self.feature.set_geometry_import_time(pygplates.GeoTimeInstant(200))
        self.assertTrue(self.feature.get_geometry_import_time() == pygplates.GeoTimeInstant(200))
        gpml_geometry_import_time = self.feature.set_geometry_import_time(300)
        self.assertTrue(gpml_geometry_import_time.get_value().get_time() == 300)
        self.assertTrue(self.feature.get_geometry_import_time() == 300)
    
    def test_get_and_set_left_plate(self):
        self.assertTrue(self.feature.get_left_plate() == 11)
        self.feature.remove(pygplates.PropertyName.gpml_left_plate)
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
        self.feature.remove(pygplates.PropertyName.gpml_right_plate)
        # With property removed it should return default of zero.
        self.assertTrue(self.feature.get_right_plate() == 0)
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_right_plate(None))
        
        self.feature.set_right_plate(201)
        self.assertTrue(self.feature.get_right_plate() == 201)
        gpml_right_plate = self.feature.set_right_plate(701)
        self.assertTrue(gpml_right_plate.get_value().get_plate_id() == self.feature.get_right_plate())
        self.assertTrue(self.feature.get_right_plate() == 701)
    
    def test_get_and_set_reconstruction_method(self):
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
                pygplates.Enumeration(pygplates.EnumerationType.create_gpml('UnknownEnumeration'), 'HalfStageRotationVersion2', pygplates.VerifyInformationModel.no),
                pygplates.VerifyInformationModel.no)
        self.assertFalse(self.feature.get_reconstruction_method(None))
    
    def test_get_and_set_reconstruction_plate_id(self):
        # There are two reconstruction plate IDs so this should return zero (default when not exactly one property).
        self.assertTrue(self.feature.get_reconstruction_plate_id() == 0)
        self.assertFalse(self.feature.get_reconstruction_plate_id(None))
        # Remove both reconstruction plate IDs.
        self.feature.remove(pygplates.PropertyName.gpml_reconstruction_plate_id)
        self.feature.add(pygplates.PropertyName.gpml_reconstruction_plate_id, pygplates.GpmlPlateId(10))
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
        self.feature.remove(pygplates.PropertyName.gpml_conjugate_plate_id)
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
        self.feature.remove(pygplates.PropertyName.gpml_conjugate_plate_id)
        # With property removed it should return default of an empty list.
        self.assertTrue(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all) == [])
        self.assertFalse(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all))
        self.feature.add(pygplates.PropertyName.gpml_conjugate_plate_id, pygplates.GpmlPlateId(22))
        self.feature.add(pygplates.PropertyName.gpml_conjugate_plate_id, pygplates.GpmlPlateId(0))
        # The plate ID zero should still get returned.
        conjugate_plate_ids = self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all)
        self.assertTrue(0 in conjugate_plate_ids)
        self.assertTrue(22 in conjugate_plate_ids)
        # If we ask for exactly one plate ID then we'll get none (even though one of the two plate IDs is zero).
        self.assertFalse(self.feature.get_conjugate_plate_id())
        self.feature.remove(pygplates.PropertyName.gpml_conjugate_plate_id)
        self.feature.add(pygplates.PropertyName.gpml_conjugate_plate_id, pygplates.GpmlPlateId(0))
        # With only a single plate ID of zero it should still return a non-empty list.
        self.assertTrue(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all) == [0])
        self.assertFalse(any(self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all)))
        
        self.feature.set_conjugate_plate_id([903, 904])
        self.assertTrue(903 in self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all))
        self.assertTrue(904 in self.feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all))
        gpml_conjugate_plate_id = self.feature.set_conjugate_plate_id((905, 906))
        self.assertTrue(len(gpml_conjugate_plate_id) == 2)
    
    def test_get_and_set_relative_plate(self):
        self.assertTrue(self.feature.get_relative_plate() == 13)
        self.feature.remove(pygplates.PropertyName.gpml_relative_plate)
        # With property removed it should return default of zero.
        self.assertTrue(self.feature.get_relative_plate() == 0)
        # With property removed it should return our default (None).
        self.assertFalse(self.feature.get_relative_plate(None))
        
        self.feature.set_relative_plate(201)
        self.assertTrue(self.feature.get_relative_plate() == 201)
        gpml_relative_plate = self.feature.set_relative_plate(701)
        self.assertTrue(gpml_relative_plate.get_value().get_plate_id() == self.feature.get_relative_plate())
        self.assertTrue(self.feature.get_relative_plate() == 701)
    
    def test_get_and_set_times(self):
        self.assertTrue(self.feature.get_times() == [0, 10, 20, pygplates.GeoTimeInstant(30), 40])
        self.feature.remove(pygplates.PropertyName.gpml_times)
        # With property removed it should return default of None.
        self.assertFalse(self.feature.get_times())
        
        self.feature.set_times([5, 15, 35])
        self.assertTrue(self.feature.get_times() == [5, 15, 35])
        gpml_times = self.feature.set_times([1, 2, 3])
        # Property stores time periods (not time instants).
        self.assertTrue(gpml_times.get_value()[:] == [pygplates.GmlTimePeriod(2, 1), pygplates.GmlTimePeriod(3, 2)])
        self.assertTrue(self.feature.get_times() == [1, 2, 3])
        # Raises error if less than two time values.
        self.assertRaises(ValueError, self.feature.set_times, [5])
        # Raises error if time sequence is not monotonically increasing.
        self.assertRaises(ValueError, self.feature.set_times, [5, 4])
        self.assertRaises(ValueError, self.feature.set_times, [5, 4, 6, 7])
        self.assertRaises(ValueError, self.feature.set_times, [5, 6, 8, 7])
        self.assertRaises(ValueError, self.feature.set_times, [5, 6, 8, 7, 9])
    
    def test_get_and_set_shapefile_attribute(self):
        # Start off with feature that has no 'gpml:shapefileAttributes' property.
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes'), pygplates.PropertyReturn.all))
        self.assertFalse(self.feature.get_shapefile_attribute('non_existent_key'))
        self.assertTrue(self.feature.get_shapefile_attribute('non_existent_key', 'default_value') == 'default_value')
        # Set shapefile attribute on feature that has no 'gpml:shapefileAttributes' property.
        shapefile_attribute_property = self.feature.set_shapefile_attribute('test_key', 'test_value')
        self.assertTrue(shapefile_attribute_property.get('test_key') == 'test_value')
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
    
    def test_get_and_set_shapefile_attributes(self):
        # Start off with feature that has no 'gpml:shapefileAttributes' property.
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes'), pygplates.PropertyReturn.all))
        self.assertTrue(self.feature.get_shapefile_attributes() is None)
        self.assertTrue(self.feature.get_shapefile_attributes({'key' : 100})['key'] == 100)
        self.assertTrue(self.feature.get_shapefile_attributes({}) == {})
        # Set shapefile attributes on feature that has no 'gpml:shapefileAttributes' property.
        self.feature.set_shapefile_attributes()
        # Should still get an attribute dict but it will be empty.
        self.assertTrue(len(self.feature.get_shapefile_attributes()) == 0)
        self.assertTrue(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes'), pygplates.PropertyReturn.all))
        self.assertTrue(len(self.feature.get(pygplates.PropertyName.create_gpml('shapefileAttributes')).get_value()) == 0)
        shapefile_attribute_property = self.feature.set_shapefile_attributes({'test_key' : 100, 'test_key2' : -100})
        self.assertTrue(len(shapefile_attribute_property.get_value()) == 2)
        self.assertTrue(self.feature.get_shapefile_attributes() == {'test_key' : 100, 'test_key2' : -100})
        self.feature.set_shapefile_attributes([('test_key', 200), ('test_key3', -200)])
        self.assertTrue(self.feature.get_shapefile_attributes() == {'test_key' : 200, 'test_key3' : -200})
        self.feature.set_shapefile_attributes()
        self.assertTrue(self.feature.get_shapefile_attributes() == {})
    
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
                pygplates.PropertyName.gpml_reconstruction_plate_id,
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 2)
        self.assertTrue(properties[0].get_plate_id() in (201,801))
        self.assertTrue(properties[1].get_plate_id() in (201,801))
        property = self.feature.get_value(
                pygplates.PropertyName.gpml_reconstruction_plate_id,
                property_return=pygplates.PropertyReturn.first)
        self.assertTrue(property.get_plate_id() == 201)
        properties = self.feature.get(
                pygplates.PropertyName.gpml_conjugate_plate_id,
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
                lambda property: property.get_name() == pygplates.PropertyName.gpml_reconstruction_plate_id,
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 2)
        self.assertTrue(properties[0].get_plate_id() in (201,801))
        self.assertTrue(properties[1].get_plate_id() in (201,801))
        properties = self.feature.get_value(
                lambda property: property.get_name() == pygplates.PropertyName.gpml_reconstruction_plate_id and
                    property.get_value().get_plate_id() == 201,
                property_return=pygplates.PropertyReturn.all)
        self.assertTrue(len(properties) == 1)
        self.assertTrue(properties[0].get_plate_id() == 201)
        property = self.feature.get_value(
                lambda property: property.get_name() == pygplates.PropertyName.gpml_reconstruction_plate_id,
                property_return=pygplates.PropertyReturn.first)
        self.assertTrue(property.get_plate_id() == 201)
        properties = self.feature.get(
                lambda property: property.get_name() == pygplates.PropertyName.gpml_conjugate_plate_id,
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
                pygplates.GpmlTimeSample(pygplates.XsDouble(0), pygplates.GeoTimeInstant(0), 'sample0', False),
                pygplates.GpmlTimeSample(pygplates.XsDouble(1), pygplates.GeoTimeInstant(1), 'sample1'),
                pygplates.GpmlTimeSample(pygplates.XsDouble(2), 2, 'sample2', False),
                pygplates.GpmlTimeSample(pygplates.XsDouble(3), 3, 'sample3')])

    def test_enabled(self):
        enabled_samples = self.gpml_irregular_sampling.get_enabled_time_samples()
        self.assertTrue(len(enabled_samples) == 2)
        self.assertAlmostEqual(enabled_samples[0].get_value().get_double(), 1)
        self.assertAlmostEqual(enabled_samples[1].get_value().get_double(), 3)

    def test_bounding(self):
        bounding_enabled_samples = self.gpml_irregular_sampling.get_time_samples_bounding_time(pygplates.GeoTimeInstant(1.5))
        self.assertTrue(bounding_enabled_samples)
        first_bounding_enabled_sample, second_bounding_enabled_sample = bounding_enabled_samples
        self.assertAlmostEqual(first_bounding_enabled_sample.get_value().get_double(), 3)
        self.assertAlmostEqual(second_bounding_enabled_sample.get_value().get_double(), 1)
        # Time is outside time samples range.
        self.assertFalse(self.gpml_irregular_sampling.get_time_samples_bounding_time(0.5))
        
        bounding_all_samples = self.gpml_irregular_sampling.get_time_samples_bounding_time(1.5, True)
        self.assertTrue(bounding_all_samples)
        first_bounding_sample, second_bounding_sample = bounding_all_samples
        self.assertAlmostEqual(first_bounding_sample.get_value().get_double(), 2)
        self.assertAlmostEqual(second_bounding_sample.get_value().get_double(), 1)
        # Time is now inside time samples range.
        self.assertTrue(self.gpml_irregular_sampling.get_time_samples_bounding_time(0.5, True))
        # Time is outside time samples range.
        self.assertFalse(self.gpml_irregular_sampling.get_time_samples_bounding_time(3.5, True))

    def test_set(self):
        self.assertRaises(ValueError,
                pygplates.GpmlIrregularSampling.set_value, self.gpml_irregular_sampling,
                pygplates.XsDouble(2.6), pygplates.GeoTimeInstant.create_distant_past())
        self.assertRaises(ValueError,
                pygplates.GpmlIrregularSampling.set_value, self.gpml_irregular_sampling,
                pygplates.XsDouble(2.6), pygplates.GeoTimeInstant.create_distant_future())
        self.assertTrue(len(self.gpml_irregular_sampling.get_time_samples()) == 4)
        # Insert a new time sample.
        gpml_time_sample = self.gpml_irregular_sampling.set_value(pygplates.XsDouble(2.6), 2.5, 'sample2.5', False)
        self.assertTrue(len(self.gpml_irregular_sampling.get_time_samples()) == 5)
        self.assertAlmostEqual(gpml_time_sample.get_value().get_double(), 2.6)
        self.assertAlmostEqual(gpml_time_sample.get_time(), 2.5)
        self.assertTrue(gpml_time_sample.get_description() == 'sample2.5')
        self.assertTrue(gpml_time_sample.is_disabled())
        # New sample is disabled so shouldn't get used in interpolated 'get_value()' here.
        self.assertAlmostEqual(self.gpml_irregular_sampling.get_value(2.5).get_double(), 2.5)
        # Modify existing time sample.
        self.gpml_irregular_sampling.set_value(pygplates.XsDouble(2.6), 2.5)
        self.assertTrue(len(self.gpml_irregular_sampling.get_time_samples()) == 5)
        self.assertAlmostEqual(self.gpml_irregular_sampling.get_value(2.5).get_double(), 2.6)
        # Append sample to end of sequence.
        gpml_time_sample = self.gpml_irregular_sampling.set_value(pygplates.XsDouble(4), 4, 'sample4')
        self.assertTrue(len(self.gpml_irregular_sampling.get_time_samples()) == 6)
        self.assertAlmostEqual(gpml_time_sample.get_value().get_double(), 4)
        self.assertAlmostEqual(gpml_time_sample.get_time(), 4)
        self.assertTrue(gpml_time_sample.get_description() == 'sample4')
        self.assertTrue(gpml_time_sample.is_enabled())
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(ts.get_time()) for ts in self.gpml_irregular_sampling.get_time_samples()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 1, 2, 2.5, 3, 4)])


class GetTimeWindowsCase(unittest.TestCase):
    def setUp(self):
        self.gpml_piecewise_aggregation = pygplates.GpmlPiecewiseAggregation([
                pygplates.GpmlTimeWindow(pygplates.XsInteger(0), pygplates.GeoTimeInstant(1), pygplates.GeoTimeInstant(0)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(10), pygplates.GeoTimeInstant(2), 1),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(20), 3, pygplates.GeoTimeInstant(2)),
                pygplates.GpmlTimeWindow(pygplates.XsInteger(30), 4, 3)])

    def test_get(self):
        time_window = self.gpml_piecewise_aggregation.get_time_window_containing_time(1.5)
        self.assertTrue(time_window)
        self.assertTrue(time_window.get_value().get_integer() == 10)
        # Should be able to specify time using 'GeoTimeInstant' also.
        time_window = self.gpml_piecewise_aggregation.get_time_window_containing_time(pygplates.GeoTimeInstant(2.5))
        self.assertTrue(time_window)
        self.assertTrue(time_window.get_value().get_integer() == 20)

    def test_set(self):
        self.assertRaises(pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError,
                pygplates.GpmlPiecewiseAggregation.set_value, self.gpml_piecewise_aggregation,
                pygplates.XsInteger(10), 1.3, 1.4)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 4)
        
        # Insert a new time window (completely within an existing time window).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(23), 2.7, 2.3)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 6)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (1, 2, 2.3, 2.7, 3, 4)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 1, 2, 2.3, 2.7, 3)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (0, 10, 20, 23, 20, 30)])
        
        # Insert a new time window (completely overlapping two existing time windows).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(25), 2.8, 1.7)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 5)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (1, 1.7, 2.8, 3, 4)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 1, 1.7, 2.8, 3)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (0, 10, 25, 20, 30)])
        
        # Insert a new time window (clipping two existing time windows).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(9), 1.1, 0.9)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 6)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0.9, 1.1, 1.7, 2.8, 3, 4)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 0.9, 1.1, 1.7, 2.8, 3)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (0, 9, 10, 25, 20, 30)])
        
        # Insert a new time window (clipping first time window).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(5), 0.5, 0)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 7)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0.5, 0.9, 1.1, 1.7, 2.8, 3, 4)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 0.5, 0.9, 1.1, 1.7, 2.8, 3)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (5, 0, 9, 10, 25, 20, 30)])
        
        # Insert a new time window (clipping last time window).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(35), 4.2, 3.9)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 8)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0.5, 0.9, 1.1, 1.7, 2.8, 3, 3.9, 4.2)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 0.5, 0.9, 1.1, 1.7, 2.8, 3, 3.9)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (5, 0, 9, 10, 25, 20, 30, 35)])
        
        # Insert a new time window (not overlapping any time windows).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(45), 5.0, 4.2)
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 9)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0.5, 0.9, 1.1, 1.7, 2.8, 3, 3.9, 4.2, 5.0)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (0, 0.5, 0.9, 1.1, 1.7, 2.8, 3, 3.9, 4.2)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (5, 0, 9, 10, 25, 20, 30, 35, 45)])
        
        # Insert a new time window (replacing all time windows).
        gpml_time_window = self.gpml_piecewise_aggregation.set_value(pygplates.XsInteger(100),
                pygplates.GeoTimeInstant.create_distant_past(), pygplates.GeoTimeInstant.create_distant_future())
        self.assertTrue(len(self.gpml_piecewise_aggregation.get_time_windows()) == 1)
        # Make sure the times are in the correct order.
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_begin_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (pygplates.GeoTimeInstant.create_distant_past(),)])
        self.assertTrue([pygplates.GeoTimeInstant(tw.get_end_time()) for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.GeoTimeInstant(t) for t in (pygplates.GeoTimeInstant.create_distant_future(),)])
        # Make sure the values are correct.
        self.assertTrue([tw.get_value() for tw in self.gpml_piecewise_aggregation.get_time_windows()] ==
                [pygplates.XsInteger(i) for i in (100,)])


class PropertyValueVisitorCase(unittest.TestCase):
    def test_visit(self):
        class GetPlateIdVisitor(pygplates.PropertyValueVisitor):
            def __init__(self):
                # NOTE: You must call base class '__init__' otherwise you will
                # get a 'Boost.Python.ArgumentError' exception.
                super(GetPlateIdVisitor, self).__init__()
                self.plate_id = None
            
            # Returns the plate id from the visited GpmlPlateId property value,
            # or None if property value was a different type.
            def get_plate_id(self):
                return self.plate_id
            
            def visit_gpml_constant_value(self, gpml_constant_value):
                # Visit the GpmlConstantValue's nested property value.
                gpml_constant_value.get_value().accept_visitor(self)
            
            def visit_gpml_plate_id(self, gpml_plate_id):
                self.plate_id = gpml_plate_id.get_plate_id()
        
        # Visitor can extract plate id from this...
        property_value1 = pygplates.GpmlPlateId(701)
        plate_id_visitor = GetPlateIdVisitor()
        property_value1.accept_visitor(plate_id_visitor)
        self.assertTrue(plate_id_visitor.get_plate_id() == 701)

        # Visitor cannot extract plate id from this...
        property_value2 = pygplates.XsInteger(701)
        plate_id_visitor = GetPlateIdVisitor()
        property_value2.accept_visitor(plate_id_visitor)
        self.assertTrue(plate_id_visitor.get_plate_id() is None)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GetFeaturePropertiesCase,
            GetGeometryFromPropertyValueCase,
            GetPropertyValueCase,
            GetTimeSamplesCase,
            GetTimeWindowsCase,
            PropertyValueVisitorCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite