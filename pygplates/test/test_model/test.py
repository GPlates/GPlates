"""
Unit tests for the pygplates model API.
"""

import os
import unittest
import pygplates

import test_model.test_get_property_value
import test_model.test_ids
import test_model.test_property_values
import test_model.test_revisioned_vector

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class CreateFeatureCase(unittest.TestCase):

    def test_create_reconstructable_feature(self):
        geometry = pygplates.PolylineOnSphere([
                        pygplates.PointOnSphere(1, 0, 0),
                        pygplates.PointOnSphere(0, 1, 0),
                        pygplates.PointOnSphere(0, 0, 1)])
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                name='East Antarctica',
                description='a coastline',
                valid_time=(600, pygplates.GeoTimeInstant.create_distant_future()),
                reconstruction_plate_id=802)
        self.assertTrue(len(feature) == 5)
        self.assertTrue(feature.get_feature_type() == pygplates.FeatureType.create_gpml('Coastline'))
        self.assertTrue(feature.get_geometry() == geometry)
        self.assertTrue(feature.get_name() == 'East Antarctica')
        self.assertTrue(feature.get_description() == 'a coastline')
        self.assertTrue(feature.get_valid_time() == (600, pygplates.GeoTimeInstant.create_distant_future()))
        self.assertTrue(feature.get_reconstruction_plate_id() == 802)
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                name=['East Antarctica1', 'East Antarctica2'],
                other_properties=[
                        (pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.GpmlPlateId(802)),
                        (pygplates.PropertyName.create_gml('description'), pygplates.XsString('a coastline'))])
        self.assertTrue(len(feature) == 5)
        self.assertTrue(feature.get_feature_type() == pygplates.FeatureType.create_gpml('Coastline'))
        self.assertTrue(feature.get_geometry() == geometry)
        self.assertTrue(feature.get_name([], pygplates.PropertyReturn.all) == ['East Antarctica1', 'East Antarctica2'])
        self.assertTrue(feature.get_description() == 'a coastline')
        self.assertTrue(feature.get_reconstruction_plate_id() == 802)
        # Should raise error if feature type is not reconstructable.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_reconstructable_feature,
                pygplates.FeatureType.create_gpml('TotalReconstructionSequence'),
                geometry)
        self.assertTrue(isinstance(
                pygplates.Feature.create_reconstructable_feature(pygplates.FeatureType.create_gpml('Coastline'), geometry),
                pygplates.Feature))
        # Should raise error if feature type is reconstructable but does not support conjugate plate ids.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_reconstructable_feature,
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                conjugate_plate_id=201)
        # Isochron is a feature type that is reconstructable and supports conjugate plate ids.
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Isochron'),
                geometry,
                conjugate_plate_id=201)
        self.assertTrue(feature.get_conjugate_plate_id() == 201)

    def test_create_topological_feature(self):
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
        topological_line = pygplates.GpmlTopologicalLine(line_topological_sections)
        subduction_zone_feature = pygplates.Feature.create_topological_feature(
                pygplates.FeatureType.gpml_subduction_zone,
                topological_line,
                name='Andes Subduction Zone',
                description='a subduction zone',
                valid_time=(10, pygplates.GeoTimeInstant.create_distant_future()))
        self.assertTrue(len(subduction_zone_feature) == 4)
        self.assertTrue(subduction_zone_feature.get_feature_type() == pygplates.FeatureType.gpml_subduction_zone)
        self.assertTrue(subduction_zone_feature.get_topological_geometry() == topological_line)
        self.assertTrue(subduction_zone_feature.get_name() == 'Andes Subduction Zone')
        self.assertTrue(subduction_zone_feature.get_description() == 'a subduction zone')
        self.assertTrue(subduction_zone_feature.get_valid_time() == (10, pygplates.GeoTimeInstant.create_distant_future()))
        self.assertTrue(subduction_zone_feature.get_reconstruction_plate_id(None) is None)
        
        polygon_topological_sections = [
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
        topological_polygon = pygplates.GpmlTopologicalPolygon(polygon_topological_sections)
        plate_feature = pygplates.Feature.create_topological_feature(
                pygplates.FeatureType.gpml_topological_closed_plate_boundary,
                topological_polygon,
                name='SAM',
                description='South America rigid plate',
                valid_time=(10, pygplates.GeoTimeInstant.create_distant_future()))
        plate_feature.set_reconstruction_plate_id(201)
        self.assertTrue(len(plate_feature) == 5)
        self.assertTrue(plate_feature.get_feature_type() == pygplates.FeatureType.gpml_topological_closed_plate_boundary)
        self.assertTrue(plate_feature.get_topological_geometry() == topological_polygon)
        self.assertTrue(plate_feature.get_name() == 'SAM')
        self.assertTrue(plate_feature.get_description() == 'South America rigid plate')
        self.assertTrue(plate_feature.get_valid_time() == (10, pygplates.GeoTimeInstant.create_distant_future()))
        self.assertTrue(plate_feature.get_reconstruction_plate_id() == 201)
 
    def test_create_tectonic_section(self):
        geometry = pygplates.PolylineOnSphere([
                        pygplates.PointOnSphere(1, 0, 0),
                        pygplates.PointOnSphere(0, 1, 0),
                        pygplates.PointOnSphere(0, 0, 1)])
        feature = pygplates.Feature.create_tectonic_section(
                pygplates.FeatureType.create_gpml('MidOceanRidge'),
                geometry,
                name=['ridge1', 'ridge2'],
                description='a ridge',
                valid_time=(600, pygplates.GeoTimeInstant.create_distant_future()),
                conjugate_plate_id=801,
                left_plate=201,
                right_plate=701,
                reconstruction_method='HalfStageRotationVersion2')
        self.assertTrue(len(feature) == 9)
        self.assertTrue(feature.get_feature_type() == pygplates.FeatureType.create_gpml('MidOceanRidge'))
        self.assertTrue(feature.get_geometry() == geometry)
        self.assertTrue(feature.get_name([], pygplates.PropertyReturn.all) == ['ridge1', 'ridge2'])
        self.assertTrue(feature.get_description() == 'a ridge')
        self.assertTrue(feature.get_valid_time() == (600, pygplates.GeoTimeInstant.create_distant_future()))
        self.assertTrue(feature.get_conjugate_plate_id() == 801)
        self.assertTrue(feature.get_left_plate() == 201)
        self.assertTrue(feature.get_right_plate() == 701)
        self.assertTrue(feature.get_value(pygplates.PropertyName.create_gpml('reconstructionMethod')).get_content() == 'HalfStageRotationVersion2')
        # Should raise error if feature type is not a tectonic section.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_tectonic_section,
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry)
        self.assertTrue(isinstance(
                pygplates.Feature.create_tectonic_section(pygplates.FeatureType.create_gpml('MidOceanRidge'), geometry),
                pygplates.Feature))
        # Invalid 'gpml:reconstructionMethod' value.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_tectonic_section,
                pygplates.FeatureType.create_gpml('MidOceanRidge'),
                geometry,
                reconstruction_method='UnknownReconstructionMethod')

    def test_create_flowline(self):
        seed_point = pygplates.PointOnSphere((0, 90))
        times = [0, 10, 20, 30]
        feature = pygplates.Feature.create_flowline(
                seed_point,
                times,
                name=['Test flowline1', 'Test flowline2'],
                description='a flowline feature',
                valid_time=(30, 0),
                left_plate=201,
                right_plate=701)
        self.assertTrue(len(feature) == 8+1) # Extra property is the 'gpml:reconstructionMethod'.
        self.assertTrue(feature.get_feature_type() == pygplates.FeatureType.create_gpml('Flowline'))
        self.assertTrue(feature.get_geometry() == seed_point)
        self.assertTrue(feature.get_times() == times)
        self.assertTrue(feature.get_name([], pygplates.PropertyReturn.all) == ['Test flowline1', 'Test flowline2'])
        self.assertTrue(feature.get_description() == 'a flowline feature')
        self.assertTrue(feature.get_valid_time() == (30, 0))
        self.assertTrue(feature.get_reconstruction_method() == 'HalfStageRotationVersion3')
        self.assertTrue(feature.get_left_plate() == 201)
        self.assertTrue(feature.get_right_plate() == 701)
        
        seed_multi_point = pygplates.MultiPointOnSphere([(0, 90), (0, 100)])
        self.assertTrue(pygplates.Feature.create_flowline(seed_multi_point, times).get_geometry() == seed_multi_point)
        # Cannot create from a polyline or polygon.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_flowline, pygplates.PolylineOnSphere([(0, 90), (0, 100)]), times)
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_flowline, pygplates.PolygonOnSphere([(0, 90), (0, 100), (0, 110)]), times)

    def test_create_motion_path(self):
        seed_point = pygplates.PointOnSphere((0, 90))
        times = [0, 10, 20, 30]
        feature = pygplates.Feature.create_motion_path(
                seed_point,
                times,
                name=['Test motion path1', 'Test motion path2'],
                description='a motion path feature',
                valid_time=(30, 0),
                relative_plate=201,
                reconstruction_plate_id=701)
        self.assertTrue(len(feature) == 8+1) # Extra property is the 'gpml:reconstructionMethod'.
        self.assertTrue(feature.get_feature_type() == pygplates.FeatureType.create_gpml('MotionPath'))
        self.assertTrue(feature.get_geometry() == seed_point)
        self.assertTrue(feature.get_times() == times)
        self.assertTrue(feature.get_name([], pygplates.PropertyReturn.all) == ['Test motion path1', 'Test motion path2'])
        self.assertTrue(feature.get_description() == 'a motion path feature')
        self.assertTrue(feature.get_valid_time() == (30, 0))
        self.assertTrue(feature.get_reconstruction_method() == 'ByPlateId')
        self.assertTrue(feature.get_relative_plate() == 201)
        self.assertTrue(feature.get_reconstruction_plate_id() == 701)
        
        seed_multi_point = pygplates.MultiPointOnSphere([(0, 90), (0, 100)])
        self.assertTrue(pygplates.Feature.create_motion_path(seed_multi_point, times).get_geometry() == seed_multi_point)
        # Cannot create from a polyline or polygon.
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_motion_path, pygplates.PolylineOnSphere([(0, 90), (0, 100)]), times)
        self.assertRaises(pygplates.InformationModelError,
                pygplates.Feature.create_motion_path, pygplates.PolygonOnSphere([(0, 90), (0, 100), (0, 110)]), times)

    def test_create_total_reconstruction_sequence(self):
        rotations = [
                pygplates.FiniteRotation((90,0), 1.0),
                pygplates.FiniteRotation((0,90), 1.0)]
        gpml_irregular_sampling = pygplates.GpmlIrregularSampling(
                [pygplates.GpmlTimeSample(pygplates.GpmlFiniteRotation(rotations[index]), index)
                    for index in range(len(rotations))])
        feature = pygplates.Feature.create_total_reconstruction_sequence(
                name=['Test sequence1', 'Test sequence2'],
                description='a rotation feature',
                fixed_plate_id=550,
                moving_plate_id=801,
                total_reconstruction_pole=gpml_irregular_sampling)
        self.assertTrue(len(feature) == 6)
        self.assertTrue(feature.get_feature_type() == pygplates.FeatureType.create_gpml('TotalReconstructionSequence'))
        self.assertTrue(feature.get_name([], pygplates.PropertyReturn.all) == ['Test sequence1', 'Test sequence2'])
        self.assertTrue(feature.get_description() == 'a rotation feature')
        self.assertTrue(feature.get_total_reconstruction_pole() == (550, 801, gpml_irregular_sampling))


class FeatureCase(unittest.TestCase):

    def setUp(self):
        self.property_count = 4
        self.feature = next(iter(pygplates.FeatureCollectionFileFormatRegistry().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))))
    
    def test_clone(self):
        # Modify original and make sure clone is not affected.
        feature_clone = self.feature.clone()
        self.assertTrue(len(feature_clone) == len(self.feature))
        self.assertTrue(feature_clone.get_reconstruction_plate_id() == self.feature.get_reconstruction_plate_id())
        self.feature.set_reconstruction_plate_id(999)
        self.assertTrue(feature_clone.get_reconstruction_plate_id() != self.feature.get_reconstruction_plate_id())
        
        feature_clone = self.feature.clone()
        self.assertTrue(len(feature_clone) == len(self.feature))
        self.feature.remove(pygplates.PropertyName.create_gpml('reconstructionPlateId'))
        self.assertTrue(len(feature_clone) != len(self.feature))

    def test_len(self):
        self.assertEqual(len(self.feature), self.property_count)
    
    def test_feature(self):
        self.assertTrue(self.feature)

    def test_feature_id(self):
        feature_id = self.feature.get_feature_id()
        self.assertTrue(isinstance(feature_id, pygplates.FeatureId))
        self.assertEqual('GPlates-d13bba7f-57f5-419d-bbd4-105a8e72e177', feature_id.get_string())

    def test_feature_type(self):
        feature_type = self.feature.get_feature_type()
        self.assertTrue(isinstance(feature_type, pygplates.FeatureType))
        self.assertEqual(feature_type, pygplates.FeatureType.create_gpml('Volcano'))
        # Since 'gpml:NotAValidFeatureType' is not a (GPGIM) recognised type it should raise an error by default.
        self.assertRaises(pygplates.InformationModelError, pygplates.Feature,
                pygplates.FeatureType.create_gpml('NotAValidFeatureType'))

# Not including RevisionId yet since it is not really needed in the python API user (we can add it later though)...
#    def test_revision_id(self):
#        revision_id = self.feature.get_revision_id()
#        self.assertTrue(isinstance(revision_id, pygplates.RevisionId))
#        self.assertEqual('GPlates-d172eaab-931f-484e-a8b6-0605e2dacd18', revision_id.get_string())

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
    
    def test_add(self):
        # Should not be able to add a second 'gpml:reconstructionPlateId'.
        self.assertRaises(pygplates.InformationModelError, self.feature.add,
                pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.GpmlPlateId(101))
        self.feature.remove(pygplates.PropertyName.create_gpml('reconstructionPlateId'))
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('reconstructionPlateId')))
        # Should not be able to add a valid property name without an unexpected property value *type*.
        self.assertRaises(pygplates.InformationModelError, self.feature.add,
                pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.XsInteger(101))
        self.assertRaises(pygplates.InformationModelError, self.feature.add,
                [(pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.XsInteger(101)),
                (pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.XsInteger(102))])
        # Add back a reconstruction plate id property to keep original property count.
        self.feature.add(pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.GpmlPlateId(101))
        
        integer_property = self.feature.add(
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(100),
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 1)
        found_integer_property = None
        for property in self.feature:
            if property == integer_property:
                found_integer_property = property
                break
        self.assertTrue(found_integer_property)
        # Since 'gpml:integer' is not a (GPGIM) recognised property name it should raise an error by default.
        self.assertRaises(pygplates.InformationModelError, self.feature.add,
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(100))
        
        # Since 'gpml:integer1', etc, is not a (GPGIM) recognised property name it should raise an error by default.
        self.assertRaises(pygplates.InformationModelError, self.feature.add,
                [(pygplates.PropertyName.create_gpml('integer1'), pygplates.XsInteger(101)),
                (pygplates.PropertyName.create_gpml('integer2'), pygplates.XsInteger(102))])
        # Add as a list of property name/value pairs.
        properties_added = self.feature.add([
                (pygplates.PropertyName.create_gpml('integer1'), pygplates.XsInteger(101)),
                (pygplates.PropertyName.create_gpml('integer2'), pygplates.XsInteger(102))],
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 3 and len(properties_added) == 2)
        # Add as a property name and list of property values.
        properties_added = self.feature.add(
                pygplates.PropertyName.create_gpml('integer'),
                [pygplates.XsInteger(103), pygplates.XsInteger(104)],
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 5 and len(properties_added) == 2)
        # Add as a list of property name/values pairs.
        properties_added = self.feature.add([
                (pygplates.PropertyName.create_gpml('integer3'), [pygplates.XsInteger(105), pygplates.XsInteger(106)]),
                [pygplates.PropertyName.create_gpml('integer4'), (pygplates.XsInteger(107),)],
                (pygplates.PropertyName.create_gpml('integer4'), pygplates.XsInteger(108))],
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 9 and len(properties_added) == 4)
    
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
        # Should now raise ValueError.
        self.assertRaises(ValueError, self.feature.remove, name_property)
        
        property_to_remove1 = self.feature.add(
                pygplates.PropertyName.create_gml('name'),
                pygplates.XsString("Name1"))
        property_to_remove2 = self.feature.add(
                pygplates.PropertyName.create_gml('name'),
                pygplates.XsString("Name2"))
        self.assertTrue(len(self.feature) == self.property_count + 1)
        # Remove a sequence of properties (and testing that specifying duplicates is allowed).
        self.feature.remove([property_to_remove1, property_to_remove2, property_to_remove2])
        self.assertTrue(len(self.feature) == self.property_count - 1)
    
    def test_remove_properties_by_name(self):
        self.feature.add(
                pygplates.PropertyName.create_gml('name'),
                pygplates.XsString("property's name"))
        self.assertTrue(len(self.feature) == self.property_count + 1)
        # Should remove newly added and previously added 'name' properties.
        self.feature.remove(pygplates.PropertyName.create_gml('name'))
        self.assertTrue(len(self.feature) == self.property_count - 1)
        # Should not be able to find either now.
        missing_name_property = None
        for property in self.feature:
            if property.get_name() == pygplates.PropertyName.create_gml('name'):
                missing_name_property = property
                break
        self.assertFalse(missing_name_property)
        
        # Remove multiple property names.
        self.feature.remove([
                pygplates.PropertyName.create_gml('validTime'),
                pygplates.PropertyName.create_gpml('reconstructionPlateId')])
        self.assertTrue(len(self.feature) == self.property_count - 3)
    
    def test_remove_properties_by_predicate(self):
        self.feature.add(
                pygplates.PropertyName.create_gml('name'),
                pygplates.XsString("property's name"))
        self.assertTrue(len(self.feature) == self.property_count + 1)
        # Should remove newly added and previously added 'name' properties.
        self.feature.remove(lambda property: property.get_name() == pygplates.PropertyName.create_gml('name'))
        self.assertTrue(len(self.feature) == self.property_count - 1)
        # Should not be able to find either now.
        missing_name_property = None
        for property in self.feature:
            if property.get_name() == pygplates.PropertyName.create_gml('name'):
                missing_name_property = property
                break
        self.assertFalse(missing_name_property)
        
        # Remove using multiple property queries.
        self.feature.remove([
                lambda property: property.get_name() == pygplates.PropertyName.create_gml('validTime'),
                pygplates.PropertyName.create_gpml('reconstructionPlateId')])
        self.assertTrue(len(self.feature) == self.property_count - 3)

    def test_set(self):
        # Should not be able to set two 'gpml:reconstructionPlateId' properties.
        self.assertRaises(pygplates.InformationModelError, self.feature.set,
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                [pygplates.GpmlPlateId(101), pygplates.GpmlPlateId(102)])
        self.feature.remove(pygplates.PropertyName.create_gpml('reconstructionPlateId'))
        self.assertFalse(self.feature.get(pygplates.PropertyName.create_gpml('reconstructionPlateId')))
        # Should not be able to set a valid property name without an unexpected property value *type*.
        self.assertRaises(pygplates.InformationModelError, self.feature.set,
                pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.XsInteger(101))
        
        self.feature.set(pygplates.PropertyName.create_gpml('reconstructionPlateId'), pygplates.GpmlPlateId(10001))
        self.assertTrue(self.feature.get_reconstruction_plate_id() == 10001)
        # Number of properties should remain unchanged since just changing a property.
        self.assertTrue(len(self.feature) == self.property_count)
        
        integer_property = self.feature.set(
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(100),
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 1)
        self.assertTrue(self.feature.get_value(pygplates.PropertyName.create_gpml('integer')).get_integer() == 100)
        # Since 'gpml:integer' is not a (GPGIM) recognised property name it should raise an error by default.
        self.assertRaises(pygplates.InformationModelError, self.feature.set,
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(101))
        
        # Since 'gpml:integer', etc, is not a (GPGIM) recognised property name it should raise an error by default.
        self.assertRaises(pygplates.InformationModelError, self.feature.set,
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(102))
        self.assertTrue(len(self.feature) == self.property_count + 1)
        self.assertTrue(self.feature.get_value(pygplates.PropertyName.create_gpml('integer')).get_integer() == 100)
        integer_property = self.feature.set(
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(102),
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 1)
        self.assertTrue(self.feature.get_value(pygplates.PropertyName.create_gpml('integer')).get_integer() == 102)
        # Set as a property name and list of property values.
        properties_set = self.feature.set(
                pygplates.PropertyName.create_gpml('integer'),
                [pygplates.XsInteger(103), pygplates.XsInteger(104)],
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 2 and len(properties_set) == 2)
        # Value 100 should have been removed by latest 'set()'.
        self.assertTrue(pygplates.XsInteger(100) not in
                self.feature.get_value(pygplates.PropertyName.create_gpml('integer'), property_return=pygplates.PropertyReturn.all))
        self.assertTrue(pygplates.XsInteger(103) in
                self.feature.get_value(pygplates.PropertyName.create_gpml('integer'), property_return=pygplates.PropertyReturn.all))
        self.assertTrue(pygplates.XsInteger(104) in
                self.feature.get_value(pygplates.PropertyName.create_gpml('integer'), property_return=pygplates.PropertyReturn.all))
        # Value 100 should have been removed by latest 'set()'.
        self.assertTrue(100 not in [property.get_value().get_integer() for property in properties_set])
        self.assertTrue(103 in [property.get_value().get_integer() for property in properties_set])
        self.assertTrue(104 in [property.get_value().get_integer() for property in properties_set])
        integer_properties = self.feature.set(
                pygplates.PropertyName.create_gpml('integer'),
                [pygplates.XsInteger(105)],
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
        self.assertTrue(len(self.feature) == self.property_count + 1)
        self.assertTrue(len(integer_properties) == 1)
        self.assertTrue(integer_properties[0].get_value().get_integer() == 105)
        self.assertTrue(self.feature.get_value(pygplates.PropertyName.create_gpml('integer')).get_integer() == 105)
        
        # Can add more than one conjugate plate ID (according to the GPGIM).
        new_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('Isochron'))
        conjugate_plate_ids = new_feature.set(pygplates.PropertyName.create_gpml('conjugatePlateId'), (pygplates.GpmlPlateId(10002), pygplates.GpmlPlateId(10003)))
        self.assertTrue(len(conjugate_plate_ids) == 2)
        conjugate_plate_ids = new_feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all)
        self.assertTrue(len(conjugate_plate_ids) == 2)
        self.assertTrue(len(new_feature) == 2)
        self.assertTrue(10002 in conjugate_plate_ids)
        self.assertTrue(10003 in conjugate_plate_ids)
        conjugate_plate_ids = new_feature.set(pygplates.PropertyName.create_gpml('conjugatePlateId'), (pygplates.GpmlPlateId(10004), ))
        self.assertTrue(len(new_feature) == 1)
        self.assertTrue(len(conjugate_plate_ids) == 1)
        self.assertTrue(new_feature.get_conjugate_plate_id() == 10004)
        conjugate_plate_id = new_feature.set(pygplates.PropertyName.create_gpml('conjugatePlateId'), pygplates.GpmlPlateId(10005))
        self.assertTrue(conjugate_plate_id.get_value().get_plate_id() == 10005)
        self.assertTrue(len(new_feature) == 1)
        self.assertTrue(new_feature.get_conjugate_plate_id() == 10005)
    

class FeatureCollectionCase(unittest.TestCase):

    def setUp(self):
        self.volcanoes_filename = os.path.join(FIXTURES, 'volcanoes.gpml')
        # The volcanoes file contains 4 volcanoes
        self.feature_count = 4
        self.feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(self.volcanoes_filename)

    def test_read_write(self):
        feature_collection = pygplates.FeatureCollection(self.volcanoes_filename) 
        self.assertTrue(len(feature_collection) == 4)
        
        def _internal_test_read_write(test_case, feature_collection, tmp_filename):
            tmp_filename = os.path.join(FIXTURES, tmp_filename)
            feature_collection.write(tmp_filename)
            self.assertTrue(os.path.isfile(tmp_filename))
            feature_collection = pygplates.FeatureCollection(tmp_filename) 
            self.assertTrue(len(feature_collection) == 4)
            feature_collections = pygplates.FeatureCollection.read([tmp_filename, tmp_filename])
            self.assertTrue(len(feature_collections) == 2)
            self.assertTrue(len(feature_collections[0]) == 4 and len(feature_collections[1]) == 4)
            os.remove(tmp_filename)

            # In case an OGR format file (which also has shapefile mapping XML file).
            if os.path.isfile(tmp_filename + '.gplates.xml'):
                os.remove(tmp_filename + '.gplates.xml')
            
            # For Shapefile.
            if tmp_filename.endswith('.shp'):
                tmp_base_filename = tmp_filename[:-len('.shp')]
                if os.path.isfile(tmp_base_filename + '.dbf'):
                    os.remove(tmp_base_filename + '.dbf')
                if os.path.isfile(tmp_base_filename + '.prj'):
                    os.remove(tmp_base_filename + '.prj')
                if os.path.isfile(tmp_base_filename + '.shx'):
                    os.remove(tmp_base_filename + '.shx')
        
        # Test read/write to different format (eg, GPML, Shapefile, GeoJSON, etc).
        _internal_test_read_write(self, feature_collection, 'tmp.gpml')  # GPML
        _internal_test_read_write(self, feature_collection, 'tmp.gpmlz')  # GPMLZ
        _internal_test_read_write(self, feature_collection, 'tmp.dat')  # PLATES4
        _internal_test_read_write(self, feature_collection, 'tmp.shp')  # Shapefile
        _internal_test_read_write(self, feature_collection, 'tmp.gmt')  # OGRGMT
        _internal_test_read_write(self, feature_collection, 'tmp.geojson')  # GeoJSON
        _internal_test_read_write(self, feature_collection, 'tmp.json')  # GeoJSON
        _internal_test_read_write(self, feature_collection, 'tmp.gpkg')  # GeoPackage

    def test_construct(self):
        # Create new empty feature collection.
        new_feature_collection = pygplates.FeatureCollection()
        self.assertEqual(len(new_feature_collection), 0)
        # Create new feature collection from existing features.
        new_feature_collection = pygplates.FeatureCollection([feature for feature in self.feature_collection])
        self.assertEqual(len(new_feature_collection), len(self.feature_collection))
        # A feature collection is also an iterable over features.
        new_feature_collection = pygplates.FeatureCollection(self.feature_collection)
        self.assertEqual(len(new_feature_collection), len(self.feature_collection))
        # Modifications to feature list (not features themselves) should not affect original
        # since 'new_feature_collection' is a shallow copy.
        new_feature_collection.remove(lambda feature: True)
        self.assertTrue(len(new_feature_collection) != len(self.feature_collection))

        feature_collection_from_file = pygplates.FeatureCollection(self.volcanoes_filename)
        self.assertTrue(len(feature_collection_from_file) == self.feature_count)

    def test_clone(self):
        # Modify original and make sure clone is not affected.
        feature_collection_clone = self.feature_collection.clone()
        self.assertTrue(len(feature_collection_clone) == len(self.feature_collection))
        feature_collection_clone.remove(pygplates.FeatureType.create_gpml('Volcano'))
        self.assertTrue(len(feature_collection_clone) != len(self.feature_collection))

    def test_len(self):
        self.assertEqual(len(self.feature_collection), self.feature_count)

    def test_get_item(self):
        for feature_index, feature in enumerate(self.feature_collection):
            self.assertTrue(feature == self.feature_collection[feature_index])

	# Temporarily comment out until we merge the python-model-revisions branch into this (python-api) branch because
	# currently '*feature_iter = feature' does not do anything (since '*feature_iter' just returns a non-null pointer).
    if (False):
        def test_set_item(self):
            shallow_copy_feature_collection = pygplates.FeatureCollection(self.feature_collection)

            self.assertTrue(len(shallow_copy_feature_collection) == len(self.feature_collection))
            for feature_index in range(len(shallow_copy_feature_collection)):
                shallow_copy_feature_collection[feature_index] = self.feature_collection[feature_index]
            self.assertTrue(len(shallow_copy_feature_collection) == len(self.feature_collection))
            for feature_index in range(len(shallow_copy_feature_collection)):
                self.assertTrue(shallow_copy_feature_collection[feature_index] == self.feature_collection[feature_index])
                self.assertTrue(shallow_copy_feature_collection[feature_index].get_feature_id() == self.feature_collection[feature_index].get_feature_id())

            self.assertTrue(len(shallow_copy_feature_collection) == len(self.feature_collection))
            for feature_index in range(len(shallow_copy_feature_collection)):
                shallow_copy_feature_collection[feature_index] = self.feature_collection[feature_index].clone()  # Cloned feature
            self.assertTrue(len(shallow_copy_feature_collection) == len(self.feature_collection))
            for feature_index in range(len(shallow_copy_feature_collection)):
                self.assertTrue(shallow_copy_feature_collection[feature_index] != self.feature_collection[feature_index])
                self.assertTrue(shallow_copy_feature_collection[feature_index].get_feature_id() != self.feature_collection[feature_index].get_feature_id())

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
    
    def test_add(self):
        # Create a feature with a new unique feature ID.
        feature_with_integer_property = pygplates.Feature()
        feature_with_integer_property.add(
                pygplates.PropertyName.create_gpml('integer'),
                pygplates.XsInteger(100),
                # 'gpml:integer' is not a (GPGIM) recognised property name...
                pygplates.VerifyInformationModel.no)
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
        self.assertTrue(next(iter(found_feature_with_integer_property)).get_value().get_integer() == 100)
        
        # Remove and add again as a list.
        self.feature_collection.remove(feature_with_integer_property)
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        self.feature_collection.add([feature_with_integer_property])
        self.assertTrue(len(self.feature_collection) == self.feature_count + 1)
    
    def test_remove(self):
        # Get the first and second features.
        feature_iter = iter(self.feature_collection)
        feature_to_remove1 = next(feature_iter)
        feature_to_remove2 = next(feature_iter)
        
        # Should not raise ValueError.
        self.feature_collection.remove(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == self.feature_count - 1)
        # Should not be able to find it now.
        missing_feature = None
        for feature in self.feature_collection:
            if feature.get_feature_id() == feature_to_remove2.get_feature_id():
                missing_feature = feature
                break
        self.assertFalse(missing_feature)
        # Should now raise ValueError.
        self.assertRaises(ValueError, self.feature_collection.remove, feature_to_remove2)
        
        # Add and remove again as a list.
        self.feature_collection.add(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        # Duplicates list entries will be removed.
        self.feature_collection.remove([feature_to_remove2, feature_to_remove2])
        self.assertTrue(len(self.feature_collection) == self.feature_count - 1)
        self.feature_collection.add(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        self.feature_collection.remove([feature_to_remove1, feature_to_remove2])
        self.assertTrue(len(self.feature_collection) == self.feature_count - 2)
    
    def test_remove_by_feature_type(self):
        # Get the first and second features.
        feature_iter = iter(self.feature_collection)
        feature_to_remove1 = next(feature_iter)
        feature_to_remove2 = next(feature_iter)
        
        self.assertTrue(len(self.feature_collection) > 0)
        self.feature_collection.remove(feature_to_remove2.get_feature_type())
        self.assertTrue(len(self.feature_collection) == 0)
        
        # Add and remove again as a list.
        self.feature_collection.add(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == 1)
        # Duplicates list entries will be removed.
        self.feature_collection.remove([feature_to_remove2.get_feature_type(), feature_to_remove2.get_feature_type()])
        self.assertTrue(len(self.feature_collection) == 0)
        self.feature_collection.add(feature_to_remove1)
        self.assertTrue(len(self.feature_collection) == 1)
        self.feature_collection.remove([feature_to_remove1.get_feature_type(), feature_to_remove2.get_feature_type()])
        self.assertTrue(len(self.feature_collection) == 0)
    
    def test_remove_by_feature_id(self):
        # Get the first and second features.
        feature_iter = iter(self.feature_collection)
        feature_to_remove1 = next(feature_iter)
        feature_to_remove2 = next(feature_iter)
        
        self.assertTrue(len(self.feature_collection) > 0)
        self.feature_collection.remove(feature_to_remove2.get_feature_id())
        self.assertTrue(len(self.feature_collection) == self.feature_count - 1)
        
        # Add and remove again as a list.
        self.feature_collection.add(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        # Duplicates list entries will be removed.
        self.feature_collection.remove([feature_to_remove2.get_feature_id(), feature_to_remove2.get_feature_id()])
        self.assertTrue(len(self.feature_collection) == self.feature_count - 1)
        self.feature_collection.add(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        self.feature_collection.remove([feature_to_remove1.get_feature_id(), feature_to_remove2.get_feature_id()])
        self.assertTrue(len(self.feature_collection) == self.feature_count - 2)
    
    def test_remove_by_predicate(self):
        # Get the first and second features.
        feature_iter = iter(self.feature_collection)
        feature_to_remove1 = next(feature_iter)
        feature_to_remove2 = next(feature_iter)
        
        self.assertTrue(len(self.feature_collection) > 0)
        self.feature_collection.remove(
                lambda feature: feature.get_feature_id() == feature_to_remove2.get_feature_id())
        self.assertTrue(len(self.feature_collection) == self.feature_count - 1)
        
        # Add and remove again as a list.
        self.feature_collection.add(feature_to_remove2)
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        self.feature_collection.remove([
                lambda feature: feature.get_feature_id() == feature_to_remove1.get_feature_id(),
                lambda feature: feature.get_feature_id() == feature_to_remove2.get_feature_id()])
        self.assertTrue(len(self.feature_collection) == self.feature_count - 2)
        
        # Remove using mixed feature queries.
        self.feature_collection.add([feature_to_remove1, feature_to_remove2])
        self.assertTrue(len(self.feature_collection) == self.feature_count)
        self.feature_collection.remove([
                lambda feature: feature.get_feature_id() == feature_to_remove1.get_feature_id(),
                feature_to_remove2.get_feature_id()])
        self.assertTrue(len(self.feature_collection) == self.feature_count - 2)
    
    def test_get_by_feature_type(self):
        features = self.feature_collection.get(
                pygplates.FeatureType.create_gpml('Volcano'),
                pygplates.FeatureReturn.all)
        self.assertTrue(len(features) == 4)
        # We won't get exactly one.
        self.assertFalse(self.feature_collection.get(pygplates.FeatureType.create_gpml('Volcano')))
        # There are no isochrons.
        self.assertFalse(self.feature_collection.get(
                pygplates.FeatureType.create_gpml('Isochron'),
                pygplates.FeatureReturn.all))
    
    def test_get_by_feature_id(self):
        # Get the first and second features.
        feature_iter = iter(self.feature_collection)
        feature1 = next(feature_iter)
        feature2 = next(feature_iter)
        
        feature = self.feature_collection.get(feature1.get_feature_id())
        self.assertTrue(feature.get_feature_id() == feature1.get_feature_id())
        self.assertTrue(len(self.feature_collection.get(feature1.get_feature_id(), pygplates.FeatureReturn.all)) == 1)
        feature = self.feature_collection.get(feature2.get_feature_id())
        self.assertTrue(feature.get_feature_id() == feature2.get_feature_id())
        self.assertFalse(self.feature_collection.get(pygplates.FeatureId.create_unique_id(), pygplates.FeatureReturn.all))
    
    def test_get_by_predicate(self):
        # Get the first and second features.
        feature_iter = iter(self.feature_collection)
        feature1 = next(feature_iter)
        feature2 = next(feature_iter)
        
        feature = self.feature_collection.get(
                lambda feature: feature.get_feature_id() == feature1.get_feature_id())
        self.assertTrue(feature.get_feature_id() == feature1.get_feature_id())
        feature = self.feature_collection.get(
                lambda feature: feature.get_feature_id() == feature2.get_feature_id())
        self.assertTrue(feature.get_feature_id() == feature2.get_feature_id())
        self.assertFalse(self.feature_collection.get(
                lambda feature: feature.get_feature_id() == pygplates.FeatureId.create_unique_id(),
                pygplates.FeatureReturn.all))
        # Of the four volcano features only two have reconstruction plate ids less than 800.
        features = self.feature_collection.get(
                lambda feature: feature.get_feature_type() == pygplates.FeatureType.create_gpml('Volcano') and
                                feature.get_reconstruction_plate_id() < 800,
                pygplates.FeatureReturn.all)
        self.assertTrue(len(features) == 2)
        # There are no isochrons.
        self.assertFalse(self.feature_collection.get(
                lambda feature: feature.get_feature_type() == pygplates.FeatureType.create_gpml('Isochron'),
                pygplates.FeatureReturn.all))


class FeatureCollectionFileFormatRegistryCase(unittest.TestCase):

    def setUp(self):
        self.volcanoes_filename = os.path.join(FIXTURES, 'volcanoes.gpml')
        self.rotations_filename = os.path.join(FIXTURES, 'rotations.rot')
        self.file_format_registry = pygplates.FeatureCollectionFileFormatRegistry()

    def test_read_feature_collection(self):
        volcanoes = self.file_format_registry.read(self.volcanoes_filename)
        self.assertTrue(len(volcanoes) == 4)
        volcanoes_and_rotations = self.file_format_registry.read([self.volcanoes_filename, self.rotations_filename])
        self.assertTrue(len(volcanoes_and_rotations) == 2)

    def test_write_feature_collection(self):
        volcanoes = self.file_format_registry.read(self.volcanoes_filename)
        self.assertTrue(len(volcanoes) == 4)
        tmp_filename = os.path.join(FIXTURES, 'tmp.gpml')
        volcanoes.write(tmp_filename)
        self.assertTrue(os.path.isfile(tmp_filename))
        volcanoes = pygplates.FeatureCollection.read(tmp_filename) 
        self.assertTrue(len(volcanoes) == 4)
        os.remove(tmp_filename)

    def test_read_rotation_model(self):
        rotations = self.file_format_registry.read(self.rotations_filename)
        self.assertTrue(rotations)

    def test_read_invalid_name(self):
        try:
            self.file_format_registry.read(None)
            self.assertTrue(False, "Loading invalid file name should fail")
        except Exception as e:
            self.assertEqual(e.__class__.__name__, 'TypeError')

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


class PropertyCase(unittest.TestCase):
    def setUp(self):
        self.property1 = pygplates.Property(
                pygplates.PropertyName.create_gpml('reconstructionPlateId'),
                pygplates.GpmlPlateId(1))
        self.property2 = pygplates.Property(
                pygplates.PropertyName.create_gpml('test_integer'),
                pygplates.XsInteger(300))
        self.property3 = pygplates.Property(
                pygplates.PropertyName.create_gpml('plateId'),
                pygplates.GpmlPiecewiseAggregation([pygplates.GpmlTimeWindow(pygplates.GpmlPlateId(100), 10, 0)]))
        self.property4 = pygplates.Property(
                pygplates.PropertyName.create_gpml('double'),
                pygplates.GpmlIrregularSampling([
                        pygplates.GpmlTimeSample(pygplates.XsDouble(100), 0),
                        pygplates.GpmlTimeSample(pygplates.XsDouble(200), 10)]))
        self.property5 = pygplates.Property(
                pygplates.PropertyName.create_gpml('plateId'),
                pygplates.GpmlConstantValue(pygplates.GpmlPlateId(300)))
    
    def test_clone(self):
        property1_clone = self.property1.clone()
        self.assertTrue(property1_clone == self.property1)
        # Modify original and make sure clone is not affected.
        self.property1.get_value().set_plate_id(2)
        self.assertTrue(property1_clone != self.property1)

    def test_get_time_dependent_container(self):
        self.assertFalse(self.property1.get_time_dependent_container())
        self.assertFalse(self.property2.get_time_dependent_container())
        self.assertTrue(isinstance(self.property3.get_time_dependent_container(), pygplates.GpmlPiecewiseAggregation))
        self.assertTrue(isinstance(self.property4.get_time_dependent_container(), pygplates.GpmlIrregularSampling))
        self.assertTrue(isinstance(self.property5.get_time_dependent_container(), pygplates.GpmlConstantValue))


class PropertyNameCase(unittest.TestCase):

    def setUp(self):
        feature = next(iter(pygplates.FeatureCollectionFileFormatRegistry().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))))
        i = iter(feature)
        # From the first volcano: its name and valid time (name is blank)
        self.feature_name = next(i)
        self.feature_valid_time = next(i)

    def test_get_name(self):
        # Feature property: gml:name
        self.assertTrue(isinstance(self.feature_name.get_name(), 
            pygplates.PropertyName))
        self.assertEqual(self.feature_name.get_name().to_qualified_string(),
                'gml:name')
        # Feature property: gml:validTime
        self.assertTrue(isinstance(self.feature_valid_time.get_name(), 
            pygplates.PropertyName))
        self.assertEqual(self.feature_valid_time.get_name().to_qualified_string(),
                'gml:validTime')


class PropertyValueCase(unittest.TestCase):

    def setUp(self):
        feature = next(iter(pygplates.FeatureCollectionFileFormatRegistry().read(
            os.path.join(FIXTURES, 'volcanoes.gpml'))))
        i = iter(feature)
        # From the first volcano: its name and valid time (name is blank)
        self.feature_name = next(i)
        self.feature_valid_time = next(i)

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
        self.assertTrue(isinstance(begin_time, float))
        end_time = valid_time.get_end_time()
        self.assertTrue(isinstance(end_time, float))
        # Test the actual values of the time period
        self.assertAlmostEqual(begin_time, 40.0)
        self.assertAlmostEqual(end_time, 0.0)


class TopologyFeatureCase(unittest.TestCase):

    def setUp(self):
        self.topology_features = pygplates.FeatureCollection(
            os.path.join(FIXTURES, 'topologies.gpml'))

    def test_property_delegates(self):
        topologies_tested = 0
        for feature in self.topology_features:
            if feature.get_feature_id().get_string() == 'GPlates-93c09f1d-80b3-44e8-9c52-f50b4deffdaa':
                topologies_tested += 1
                topological_line = feature.get_value(pygplates.PropertyName.gpml_unclassified_geometry)
                sections = topological_line.get_sections()
                property_delegate0 = sections[0].get_property_delegate()
                self.assertTrue(property_delegate0.get_feature_id().get_string() == 'GPlates-84be6d41-6c32-4184-9c44-c38e399090a0')
                self.assertTrue(property_delegate0.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate0.get_property_type() == pygplates.GmlPoint)
                property_delegate1 = sections[1].get_property_delegate()
                self.assertTrue(property_delegate1.get_feature_id().get_string() == 'GPlates-3df7a9df-aefc-403e-a16c-faf203776fd1')
                self.assertTrue(property_delegate1.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate1.get_property_type() == pygplates.GmlPoint)
                property_delegate2 = sections[2].get_property_delegate()
                self.assertTrue(property_delegate2.get_feature_id().get_string() == 'GPlates-56f22e61-ddd5-4c2f-ae41-54e5f66f47ec')
                self.assertTrue(property_delegate2.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate2.get_property_type() == pygplates.GmlPoint)
            elif feature.get_feature_id().get_string() == 'GPlates-a6054d82-6e6d-4f59-9d24-4ab255ece477':
                topologies_tested += 1
                topological_polygon = feature.get_value(pygplates.PropertyName.gpml_boundary)
                exterior_sections = topological_polygon.get_exterior_sections()
                property_delegate0 = exterior_sections[0].get_property_delegate()
                self.assertTrue(property_delegate0.get_feature_id().get_string() == 'GPlates-aa1d0d5a-0445-4380-a516-d2bc66e477a7')
                self.assertTrue(property_delegate0.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate0.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(exterior_sections[0].get_reverse_orientation() == False)
                property_delegate1 = exterior_sections[1].get_property_delegate()
                self.assertTrue(property_delegate1.get_feature_id().get_string() == 'GPlates-0bfc4f2b-c672-47e1-a29e-7214bea0521e')
                self.assertTrue(property_delegate1.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate1.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(exterior_sections[1].get_reverse_orientation() == False)
                property_delegate2 = exterior_sections[2].get_property_delegate()
                self.assertTrue(property_delegate2.get_feature_id().get_string() == 'GPlates-48bd0e0f-e7c8-4dea-9e0a-4bc0e1403db6')
                self.assertTrue(property_delegate2.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate2.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(exterior_sections[2].get_reverse_orientation() == False)
                property_delegate3 = exterior_sections[3].get_property_delegate()
                self.assertTrue(property_delegate3.get_feature_id().get_string() == 'GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2')
                self.assertTrue(property_delegate3.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate3.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(exterior_sections[3].get_reverse_orientation() == False)
                property_delegate4 = exterior_sections[4].get_property_delegate()
                self.assertTrue(property_delegate4.get_feature_id().get_string() == 'GPlates-e184b54d-abb0-465b-8820-c73c543d2562')
                self.assertTrue(property_delegate4.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate4.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(exterior_sections[4].get_reverse_orientation() == True)
            elif feature.get_feature_id().get_string() == 'GPlates-4fe56a89-d041-4494-ab07-3abead642b8e':
                topologies_tested += 1
                topological_network = feature.get_value(pygplates.PropertyName.gpml_network)
                boundary_sections = topological_network.get_boundary_sections()
                property_delegate0 = boundary_sections[0].get_property_delegate()
                self.assertTrue(property_delegate0.get_feature_id().get_string() == 'GPlates-aa1d0d5a-0445-4380-a516-d2bc66e477a7')
                self.assertTrue(property_delegate0.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate0.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(boundary_sections[0].get_reverse_orientation() == False)
                property_delegate1 = boundary_sections[1].get_property_delegate()
                self.assertTrue(property_delegate1.get_feature_id().get_string() == 'GPlates-e184b54d-abb0-465b-8820-c73c543d2562')
                self.assertTrue(property_delegate1.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate1.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(boundary_sections[1].get_reverse_orientation() == False)
                property_delegate2 = boundary_sections[2].get_property_delegate()
                self.assertTrue(property_delegate2.get_feature_id().get_string() == 'GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2')
                self.assertTrue(property_delegate2.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate2.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(boundary_sections[2].get_reverse_orientation() == False)
                property_delegate3 = boundary_sections[3].get_property_delegate()
                self.assertTrue(property_delegate3.get_feature_id().get_string() == 'GPlates-56f3c23d-1ee5-47a9-a46e-006d2aa463c3')
                self.assertTrue(property_delegate3.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate3.get_property_type() == pygplates.GmlPoint)
                self.assertTrue(boundary_sections[3].get_reverse_orientation() == False)
                property_delegate4 = boundary_sections[4].get_property_delegate()
                self.assertTrue(property_delegate4.get_feature_id().get_string() == 'GPlates-0ba4c93d-474e-4d9b-8f1b-618cb21024de')
                self.assertTrue(property_delegate4.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate4.get_property_type() == pygplates.GmlPoint)
                self.assertTrue(boundary_sections[4].get_reverse_orientation() == False)
                property_delegate5 = boundary_sections[5].get_property_delegate()
                self.assertTrue(property_delegate5.get_feature_id().get_string() == 'GPlates-93c09f1d-80b3-44e8-9c52-f50b4deffdaa')
                self.assertTrue(property_delegate5.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate5.get_property_type() == pygplates.GpmlTopologicalLine)
                self.assertTrue(boundary_sections[5].get_reverse_orientation() == False)
                property_delegate6 = boundary_sections[6].get_property_delegate()
                self.assertTrue(property_delegate6.get_feature_id().get_string() == 'GPlates-cc5b9027-d227-4e97-bb06-df26786fd1ec')
                self.assertTrue(property_delegate6.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate6.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(boundary_sections[6].get_reverse_orientation() == True)
                property_delegate7 = boundary_sections[7].get_property_delegate()
                self.assertTrue(property_delegate7.get_feature_id().get_string() == 'GPlates-63b81b91-b7a0-4ad7-908d-16db3c70e6ed')
                self.assertTrue(property_delegate7.get_property_name() == pygplates.PropertyName.gpml_unclassified_geometry)
                self.assertTrue(property_delegate7.get_property_type() == pygplates.GmlLineString)
                self.assertTrue(boundary_sections[7].get_reverse_orientation() == True)
        self.assertTrue(topologies_tested == 3)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            CreateFeatureCase,
            FeatureCase,
            FeatureCollectionCase,
            FeatureCollectionFileFormatRegistryCase,
            GeoTimeInstantCase,
            PropertyCase,
            PropertyNameCase,
            PropertyValueCase,
            TopologyFeatureCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    # Add test suites from sibling modules.
    test_modules = [
            test_model.test_get_property_value,
            test_model.test_ids,
            test_model.test_property_values,
            test_model.test_revisioned_vector
            ]

    for test_module in test_modules:
        suite.addTest(test_module.suite())
    
    return suite
