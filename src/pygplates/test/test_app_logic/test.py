"""
Unit tests for the pygplates application logic API.
"""

import math
import os
import shutil
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class CalculateVelocitiesTestCase(unittest.TestCase):
    def test_calculate_velocities(self):
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 10
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
                os.path.join(FIXTURES, 'volcanoes.gpml'),
                rotation_model,
                reconstructed_feature_geometries,
                reconstruction_time)
        for reconstructed_feature_geometry in reconstructed_feature_geometries:
            equivalent_stage_rotation = rotation_model.get_rotation(
                reconstruction_time,
                reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id(),
                reconstruction_time + 1)
            reconstructed_points = reconstructed_feature_geometry.get_reconstructed_geometry().get_points()
            velocities = pygplates.calculate_velocities(
                reconstructed_points,
                equivalent_stage_rotation,
                1,
                pygplates.VelocityUnits.cms_per_yr)
            self.assertTrue(len(velocities) == len(reconstructed_points))
            for index in range(len(velocities)):
                # The velocity direction should be orthogonal to the point direction (from globe origin).
                self.assertAlmostEqual(
                    pygplates.Vector3D.dot(velocities[index], reconstructed_points[index].to_xyz()), 0)


class CrossoverTestCase(unittest.TestCase):
    def test_find_crossovers(self):
        crossovers = pygplates.find_crossovers(os.path.join(FIXTURES, 'rotations.rot'))
        self.assertTrue(len(crossovers) == 133)
        
        # TODO: Add more tests.

    def test_synchronise_crossovers(self):
        # This writes back to the rotation file.
        #pygplates.synchronise_crossovers(os.path.join(FIXTURES, 'rotations.rot'))
        
        # This does not write back to the rotation file.
        rotation_feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        crossover_results = []
        pygplates.synchronise_crossovers(
                rotation_feature_collection,
                lambda crossover: crossover.time < 600,
                0.01, # 2 decimal places
                pygplates.CrossoverType.synch_old_crossover_and_stages,
                crossover_results)
        # Due to filtering of crossover times less than 600Ma we have 123 instead of 134 crossovers.
        self.assertTrue(len(crossover_results) == 123)
        
        # TODO: Add more tests.


class InterpolateTotalReconstructionSequenceTestCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))

    def test_get(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        next(feature_iter);
        next(feature_iter);
        feature = next(feature_iter)
        
        total_reconstruction_pole = feature.get_total_reconstruction_pole()
        self.assertTrue(total_reconstruction_pole)
        fixed_plate_id, moving_plate_id, total_reconstruction_pole_rotations = total_reconstruction_pole
        self.assertTrue(isinstance(total_reconstruction_pole_rotations, pygplates.GpmlIrregularSampling))
        self.assertEquals(fixed_plate_id, 901)
        self.assertEquals(moving_plate_id, 2)

    def test_set(self):
        gpml_irregular_sampling = pygplates.GpmlIrregularSampling(
                [pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                                pygplates.FiniteRotation(
                                        pygplates.PointOnSphere(1,0,0),
                                        0.4)),
                        0),
                pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                                pygplates.FiniteRotation(
                                        pygplates.PointOnSphere(0,1,0),
                                        0.5)),
                        10)])
        feature = pygplates.Feature(pygplates.FeatureType.create_gpml('TotalReconstructionSequence'))
        fixed_plate_property, moving_plate_property, total_pole_property = \
                feature.set_total_reconstruction_pole(901, 2, gpml_irregular_sampling)
        # Should have added three properties.
        self.assertTrue(len(feature) == 3)
        self.assertTrue(fixed_plate_property.get_value().get_plate_id() == 901)
        self.assertTrue(moving_plate_property.get_value().get_plate_id() == 2)
        interpolated_pole, interpolated_angle = total_pole_property.get_value(5).get_finite_rotation().get_euler_pole_and_angle()
        self.assertTrue(abs(interpolated_angle) > 0.322 and abs(interpolated_angle) < 0.323)

    def test_interpolate(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        next(feature_iter);
        next(feature_iter);
        feature = next(feature_iter)
        
        total_reconstruction_pole = feature.get_total_reconstruction_pole()
        self.assertTrue(total_reconstruction_pole)
        fixed_plate_id, moving_plate_id, total_reconstruction_pole_rotations = total_reconstruction_pole
        interpolated_finite_rotation = total_reconstruction_pole_rotations.get_value(12.2).get_finite_rotation()
        self.assertEquals(fixed_plate_id, 901)
        self.assertEquals(moving_plate_id, 2)
        pole, angle = interpolated_finite_rotation.get_euler_pole_and_angle()
        self.assertTrue(abs(angle) > 0.1785 and abs(angle) < 0.179)
        # TODO: Compare pole.


class ReconstructTestCase(unittest.TestCase):
    def test_reconstruct(self):
        pygplates.reconstruct(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            os.path.join(FIXTURES, 'test.xy'),
            pygplates.GeoTimeInstant(10))
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))
        os.remove(os.path.join(FIXTURES, 'test.xy'))
        
        feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'volcanoes.gpml'))
        self.assertEqual(len(feature_collection), 4)
        rotation_features = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml'), os.path.join(FIXTURES, 'volcanoes.gpml')],
            rotation_features,
            reconstructed_feature_geometries,
            0)
        # We've doubled up on the number of RFG's compared to number of features.
        self.assertEqual(len(reconstructed_feature_geometries), 2 * len(feature_collection))
        # The order of RFG's should match the order of features.
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
            # We've doubled up on the number of RFG's compared to number of features.
            self.assertTrue(reconstructed_feature_geometries[index + len(feature_collection)].get_feature().get_feature_id() == feature.get_feature_id())
        # Test queries on ReconstructedFeatureGeometry.
        rfg1 = reconstructed_feature_geometries[0]
        self.assertTrue(rfg1.get_feature())
        self.assertTrue(rfg1.get_property())
        self.assertTrue(isinstance(rfg1.get_present_day_geometry(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(rfg1.get_reconstructed_geometry(), pygplates.PointOnSphere))
        
        # Reconstruct a feature collection.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            feature_collection,
            rotation_features,
            reconstructed_feature_geometries,
            0)
        self.assertEqual(len(reconstructed_feature_geometries), len(feature_collection))
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
        
        # Reconstruct a list of features.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [feature for feature in feature_collection],
            rotation_features,
            reconstructed_feature_geometries,
            0)
        self.assertEqual(len(reconstructed_feature_geometries), len(feature_collection))
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
        
        # Reconstruct individual features.
        for feature in feature_collection:
            reconstructed_feature_geometries = []
            pygplates.reconstruct(
                feature,
                rotation_features,
                reconstructed_feature_geometries,
                0)
            self.assertEqual(len(reconstructed_feature_geometries), 1)
            self.assertTrue(reconstructed_feature_geometries[0].get_feature().get_feature_id() == feature.get_feature_id())
        
        # Reconstruct a list that is a mixture of feature collection, filename, list of features and a feature.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [
                pygplates.FeatureCollectionFileFormatRegistry().read(os.path.join(FIXTURES, 'volcanoes.gpml')), # feature collection
                os.path.join(FIXTURES, 'volcanoes.gpml'), # filename
                [feature for feature in pygplates.FeatureCollectionFileFormatRegistry().read(os.path.join(FIXTURES, 'volcanoes.gpml'))], # list of features
                next(iter(pygplates.FeatureCollectionFileFormatRegistry().read(os.path.join(FIXTURES, 'volcanoes.gpml')))) # single feature
            ],
            rotation_features,
            reconstructed_feature_geometries,
            0)
        self.assertEqual(len(reconstructed_feature_geometries), 3 * len(feature_collection) + 1)
        # The order of RFG's should match the order of features (provided we don't duplicate the features across the lists - which
        # is why we loaded the features from scratch above instead of specifying derivatives of 'feature_collection').
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
            self.assertTrue(reconstructed_feature_geometries[index + len(feature_collection)].get_feature().get_feature_id() == feature.get_feature_id())
            self.assertTrue(reconstructed_feature_geometries[index + 2 * len(feature_collection)].get_feature().get_feature_id() == feature.get_feature_id())
        self.assertTrue(reconstructed_feature_geometries[3 * len(feature_collection)].get_feature().get_feature_id() == next(iter(feature_collection)).get_feature_id())
        
        # Reconstruct to 15Ma.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml')],
            [os.path.join(FIXTURES, 'rotations.rot')],
            reconstructed_feature_geometries,
            15)
        # One volcano does not exist at 15Ma.
        self.assertEqual(len(reconstructed_feature_geometries), 3)
        
    def test_reconstruct_feature_geometry(self):
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 15
        geometry = pygplates.PolylineOnSphere([(0,0), (10, 10)])
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                valid_time=(30, 0),
                reconstruction_plate_id=801)
        reconstructed_feature_geometries = []
        pygplates.reconstruct(feature, rotation_model, reconstructed_feature_geometries, reconstruction_time)
        self.assertEqual(len(reconstructed_feature_geometries), 1)
        self.assertTrue(reconstructed_feature_geometries[0].get_feature().get_feature_id() == feature.get_feature_id())
        self.assertTrue(geometry == reconstructed_feature_geometries[0].get_present_day_geometry())
        # Test grouping with feature.
        grouped_reconstructed_feature_geometries = []
        pygplates.reconstruct(feature, rotation_model, grouped_reconstructed_feature_geometries, reconstruction_time, group_with_feature=True)
        self.assertEqual(len(grouped_reconstructed_feature_geometries), 1)
        grouped_feature, reconstructed_feature_geometries = grouped_reconstructed_feature_geometries[0]
        self.assertTrue(grouped_feature.get_feature_id() == feature.get_feature_id())
        self.assertEqual(len(reconstructed_feature_geometries), 1)
        self.assertTrue(geometry == reconstructed_feature_geometries[0].get_present_day_geometry())
        
        # Test reverse reconstruction.
        geometry_at_reconstruction_time = geometry
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                valid_time=(30, 0),
                reconstruction_plate_id=801,
                reverse_reconstruct=(rotation_model, reconstruction_time))
        geometry_at_present_day = feature.get_geometry()
        reconstructed_feature_geometries = []
        pygplates.reconstruct(feature, rotation_model, reconstructed_feature_geometries, reconstruction_time)
        self.assertEqual(len(reconstructed_feature_geometries), 1)
        self.assertTrue(reconstructed_feature_geometries[0].get_feature().get_feature_id() == feature.get_feature_id())
        self.assertTrue(geometry_at_present_day == reconstructed_feature_geometries[0].get_present_day_geometry())
        self.assertTrue(geometry_at_reconstruction_time == reconstructed_feature_geometries[0].get_reconstructed_geometry())
        
    def test_reconstruct_flowline(self):
        pygplates.reconstruct(
            os.path.join(FIXTURES, 'flowline.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            os.path.join(FIXTURES, 'test.xy'),
            pygplates.GeoTimeInstant(10),
            reconstruct_type=pygplates.ReconstructType.flowline)
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))
        os.remove(os.path.join(FIXTURES, 'test.xy'))
        
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 15
        seed_points = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        flowline_feature = pygplates.Feature.create_flowline(
                seed_points,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                left_plate=201,
                right_plate=801)
        reconstructed_flowlines = []
        # First without specifying flowlines.
        pygplates.reconstruct(flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time)
        self.assertEqual(len(reconstructed_flowlines), 0)
        # Now specify flowlines.
        pygplates.reconstruct(
                flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.flowline)
        self.assertEqual(len(reconstructed_flowlines), 2)
        for index, reconstructed_flowline in enumerate(reconstructed_flowlines):
            self.assertTrue(reconstructed_flowline.get_feature().get_feature_id() == flowline_feature.get_feature_id())
            self.assertTrue(seed_points[index] == reconstructed_flowline.get_present_day_seed_point())
        
        # Test reverse reconstruction.
        seed_points_at_reconstruction_time = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        flowline_feature = pygplates.Feature.create_flowline(
                seed_points_at_reconstruction_time,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                left_plate=201,
                right_plate=801,
                reverse_reconstruct=(rotation_model, reconstruction_time))
        seed_points_at_present_day = flowline_feature.get_geometry()
        reconstructed_flowlines = []
        pygplates.reconstruct(
                flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.flowline)
        self.assertEqual(len(reconstructed_flowlines), 2)
        for index, reconstructed_flowline in enumerate(reconstructed_flowlines):
            self.assertTrue(reconstructed_flowline.get_feature().get_feature_id() == flowline_feature.get_feature_id())
            self.assertTrue(seed_points_at_present_day[index] == reconstructed_flowline.get_present_day_seed_point())
            self.assertTrue(seed_points_at_reconstruction_time[index] == reconstructed_flowline.get_reconstructed_seed_point())
            # At 15Ma there should be four points (15, 20, 30, 40).
            self.assertTrue(len(reconstructed_flowline.get_left_flowline()) == 4)
            self.assertTrue(len(reconstructed_flowline.get_right_flowline()) == 4)
            # First point in left/right flowline is reconstructed seed point.
            self.assertTrue(reconstructed_flowline.get_left_flowline()[0] == reconstructed_flowline.get_reconstructed_seed_point())
            self.assertTrue(reconstructed_flowline.get_right_flowline()[0] == reconstructed_flowline.get_reconstructed_seed_point())
        
    def test_reconstruct_motion_path(self):
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 15
        seed_points = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        motion_path_feature = pygplates.Feature.create_motion_path(
                seed_points,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                relative_plate=201,
                reconstruction_plate_id=801)
        reconstructed_motion_paths = []
        # First without specifying motion paths.
        pygplates.reconstruct(motion_path_feature, rotation_model, reconstructed_motion_paths, reconstruction_time)
        self.assertEqual(len(reconstructed_motion_paths), 0)
        # Now specify motion paths.
        pygplates.reconstruct(
                motion_path_feature, rotation_model, reconstructed_motion_paths, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.motion_path)
        self.assertEqual(len(reconstructed_motion_paths), 2)
        for index, reconstructed_motion_path in enumerate(reconstructed_motion_paths):
            self.assertTrue(reconstructed_motion_path.get_feature().get_feature_id() == motion_path_feature.get_feature_id())
            self.assertTrue(seed_points[index] == reconstructed_motion_path.get_present_day_seed_point())
        
        # Test reverse reconstruction.
        seed_points_at_reconstruction_time = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        motion_path_feature = pygplates.Feature.create_motion_path(
                seed_points_at_reconstruction_time,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                relative_plate=201,
                reconstruction_plate_id=801,
                reverse_reconstruct=(rotation_model, reconstruction_time))
        seed_points_at_present_day = motion_path_feature.get_geometry()
        reconstructed_motion_paths = []
        pygplates.reconstruct(
                motion_path_feature, rotation_model, reconstructed_motion_paths, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.motion_path)
        self.assertEqual(len(reconstructed_motion_paths), 2)
        for index, reconstructed_motion_path in enumerate(reconstructed_motion_paths):
            self.assertTrue(reconstructed_motion_path.get_feature().get_feature_id() == motion_path_feature.get_feature_id())
            self.assertTrue(seed_points_at_present_day[index] == reconstructed_motion_path.get_present_day_seed_point())
            self.assertTrue(seed_points_at_reconstruction_time[index] == reconstructed_motion_path.get_reconstructed_seed_point())
            # At 15Ma there should be four points (15, 20, 30, 40).
            self.assertTrue(len(reconstructed_motion_path.get_motion_path()) == 4)
            # Last point in motion path is reconstructed seed point.
            self.assertTrue(reconstructed_motion_path.get_motion_path()[-1] == reconstructed_motion_path.get_reconstructed_seed_point())
        
    def test_deprecated_reconstruct(self):
        # We continue to support the deprecated version of 'reconstruct()' since
        # it was one of the few python API functions that's been around since
        # the dawn of time and is currently used in some web applications.
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml')],
            [os.path.join(FIXTURES, 'rotations.rot')],
            10,
            0,
            os.path.join(FIXTURES, 'test.xy'))
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))
        os.remove(os.path.join(FIXTURES, 'test.xy'))

    def test_reverse_reconstruct(self):
        # Test modifying the feature collection file.
        # Modify a copy of the file.
        shutil.copyfile(os.path.join(FIXTURES, 'volcanoes.gpml'), os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))
        pygplates.reverse_reconstruct(
            os.path.join(FIXTURES, 'volcanoes_tmp.gpml'),
            [os.path.join(FIXTURES, 'rotations.rot')],
            pygplates.GeoTimeInstant(10))
        # Remove modify copy of the file.
        os.remove(os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))
        
        rotation_features = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        
        # Test modifying the feature collection only (not the file it was read from).
        reconstructable_feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'volcanoes.gpml'))
        pygplates.reverse_reconstruct(
            reconstructable_feature_collection,
            [rotation_features],
            10,
            0)
        
        # Test modifying a list of features.
        pygplates.reverse_reconstruct(
            [feature for feature in reconstructable_feature_collection],
            rotation_features,
            10,
            0)
        
        # Test modifying a single feature only.
        pygplates.reverse_reconstruct(
            next(iter(reconstructable_feature_collection)),
            rotation_features,
            10,
            0)
            
        # Test modifying a mixture of the above.
        # Modify a copy of the file.
        shutil.copyfile(os.path.join(FIXTURES, 'volcanoes.gpml'), os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))
        pygplates.reverse_reconstruct([
                os.path.join(FIXTURES, 'volcanoes_tmp.gpml'),
                reconstructable_feature_collection,
                [feature for feature in reconstructable_feature_collection],
                next(iter(reconstructable_feature_collection))],
            rotation_features,
            10,
            0)
        # Remove modify copy of the file.
        os.remove(os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))


class PlatePartitionerTestCase(unittest.TestCase):
    def setUp(self):
        self.topological_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        self.rotation_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))

    def test_partition_exceptions(self):
        rotation_model = pygplates.RotationModel(self.rotation_features)
        resolved_topologies = []
        pygplates.resolve_topologies(
            self.topological_features,
            rotation_model,
            resolved_topologies,
            0)
        
        # Can have no reconstruction geometries.
        plate_partitioner = pygplates.PlatePartitioner([], rotation_model)
        self.assertFalse(plate_partitioner.partition_point(pygplates.PointOnSphere(0, 0)))
        
        # All reconstruction times must be the same.
        resolved_topologies_10 = []
        pygplates.resolve_topologies(
            self.topological_features,
            self.rotation_features,
            resolved_topologies_10,
            10)
        self.assertRaises(pygplates.DifferentTimesInPartitioningPlatesError,
                pygplates.PlatePartitioner, resolved_topologies + resolved_topologies_10, rotation_model)

    def test_sort(self):
        rotation_model = pygplates.RotationModel(self.rotation_features)
        resolved_topologies = []
        pygplates.resolve_topologies(
            self.topological_features,
            rotation_model,
            resolved_topologies,
            0)
        
        # Pick a polyline that intersects all three topology regions.
        # We'll use this to verify the sort order of partitioning geometries.
        polyline = pygplates.PolylineOnSphere([(0,0), (0,-30), (30,-30), (30,-90)])
        
        # Unsorted.
        plate_partitioner = pygplates.PlatePartitioner(self.topological_features, rotation_model, sort_partitioning_plates=None)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Should be in original order.
        for recon_geom_index, (recon_geom, inside_geoms) in enumerate(partitioned_inside_geometries):
            self.assertTrue(recon_geom.get_feature().get_feature_id() ==
                    resolved_topologies[recon_geom_index].get_feature().get_feature_id())
        
        # Unsorted.
        plate_partitioner = pygplates.PlatePartitioner(resolved_topologies, rotation_model, sort_partitioning_plates=None)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Should be in original order.
        for recon_geom_index, (recon_geom, inside_geoms) in enumerate(partitioned_inside_geometries):
            self.assertTrue(recon_geom.get_feature().get_feature_id() ==
                    resolved_topologies[recon_geom_index].get_feature().get_feature_id())
        
        # Sorted by partition type.
        resolved_topologies_copy = resolved_topologies[:]
        if isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork):
            # Move to end of list.
            resolved_topologies_copy.append(resolved_topologies_copy[0])
            del resolved_topologies_copy[0]
        self.assertFalse(isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork))
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Topological network always comes first.
        self.assertTrue(isinstance(partitioned_inside_geometries[0][0], pygplates.ResolvedTopologicalNetwork))
        
        # Sorted by partition type then plate ID.
        #
        # Sort in opposite plate ID order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_feature().get_reconstruction_plate_id())
        if isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork):
            # Move to end of list.
            resolved_topologies_copy.append(resolved_topologies_copy[0])
            del resolved_topologies_copy[0]
        self.assertFalse(isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork))
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_id)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Topological network always comes first.
        self.assertTrue(isinstance(partitioned_inside_geometries[0][0], pygplates.ResolvedTopologicalNetwork))
        # Then the two resolved boundaries are sorted by plate ID.
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 2)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 1)
        
        # Sorted by partition type then plate area.
        #
        # Sort in opposite plate area order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_resolved_boundary().get_area())
        if isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork):
            # Move to end of list.
            resolved_topologies_copy.append(resolved_topologies_copy[0])
            del resolved_topologies_copy[0]
        self.assertFalse(isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork))
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_area)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Topological network always comes first.
        self.assertTrue(isinstance(partitioned_inside_geometries[0][0], pygplates.ResolvedTopologicalNetwork))
        # Then the two resolved boundaries are sorted by plate area (not plate ID).
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 2)
        
        # Sorted by plate ID.
        #
        # Sort in opposite plate ID order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_feature().get_reconstruction_plate_id())
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_plate_id)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        self.assertTrue(partitioned_inside_geometries[0][0].get_feature().get_reconstruction_plate_id() == 2)
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 0)
        
        # Sorted by plate area.
        #
        # Sort in opposite plate area order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_resolved_boundary().get_area())
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_plate_area)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        self.assertTrue(partitioned_inside_geometries[0][0].get_feature().get_reconstruction_plate_id() == 0)
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 2)

    def test_partition_features(self):
        plate_partitioner = pygplates.PlatePartitioner(self.topological_features, self.rotation_features)
        
        # Partition inside point.
        point_feature = pygplates.Feature()
        point_feature.set_geometry(pygplates.PointOnSphere(0, -30))
        partitioned_features = plate_partitioner.partition_features(
            point_feature,
            properties_to_copy = [
                pygplates.PartitionProperty.reconstruction_plate_id,
                pygplates.PartitionProperty.valid_time_end])
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_reconstruction_plate_id() == 1)
        # Only the end time (0Ma) should have changed.
        self.assertTrue(partitioned_features[0].get_valid_time() ==
            (pygplates.GeoTimeInstant.create_distant_past(), 0))
        
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            point_feature,
            properties_to_copy = [
                pygplates.PartitionProperty.reconstruction_plate_id,
                pygplates.PartitionProperty.valid_time_period,
                pygplates.PropertyName.gml_name],
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(unpartitioned_features) == 0)
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_features[0].get_name() == 'topology2')
        # Both begin and end time should have changed.
        self.assertTrue(partitioned_features[0].get_valid_time() == (100,0))
        
        partitioned_groups, unpartitioned_features = plate_partitioner.partition_features(
            point_feature,
            properties_to_copy = [
                pygplates.PartitionProperty.reconstruction_plate_id,
                pygplates.PartitionProperty.valid_time_begin],
            partition_return = pygplates.PartitionReturn.partitioned_groups_and_unpartitioned)
        self.assertTrue(len(unpartitioned_features) == 0)
        self.assertTrue(len(partitioned_groups) == 1)
        self.assertTrue(partitioned_groups[0][0].get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(partitioned_groups[0][1]) == 1)
        self.assertTrue(partitioned_groups[0][1][0].get_reconstruction_plate_id() == 1)
        # Only the begin time (100Ma) should have changed.
        self.assertTrue(partitioned_groups[0][1][0].get_valid_time() == (100, pygplates.GeoTimeInstant.create_distant_future()))
        
        # Partition inside and outside point.
        inside_point_feature = pygplates.Feature()
        inside_point_feature.set_geometry(pygplates.PointOnSphere(0, -30))
        outside_point_feature = pygplates.Feature()
        outside_point_feature.set_geometry(pygplates.PointOnSphere(0, 0))
        def test_set_name(partitioning_feature, feature):
            try:
                feature.set_name(partitioning_feature.get_name())
            except pygplates.InformationModelError:
                pass
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            [inside_point_feature, outside_point_feature],
            properties_to_copy = test_set_name,
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(unpartitioned_features) == 1)
        self.assertTrue(unpartitioned_features[0].get_name() == '')
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_name() == 'topology2')
        
        # Partition VGP feature - average sample site position is inside and pole position is outside.
        vgp_feature = pygplates.Feature(pygplates.FeatureType.gpml_virtual_geomagnetic_pole)
        vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, -30), pygplates.PropertyName.gpml_average_sample_site_position)
        vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, 0), pygplates.PropertyName.gpml_pole_position)
        features = plate_partitioner.partition_features(vgp_feature)
        self.assertTrue(len(features) == 1)
        self.assertTrue(features[0].get_reconstruction_plate_id() == 1)
        # Move average sample site position outside.
        vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, 0), pygplates.PropertyName.gpml_average_sample_site_position)
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            vgp_feature,
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(partitioned_features) == 0)
        self.assertTrue(len(unpartitioned_features) == 1)
        self.assertTrue(unpartitioned_features[0].get_reconstruction_plate_id() == 0)
        # Again but not a VGP feature - should get split into two features.
        non_vgp_feature = pygplates.Feature()
        non_vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, -30), pygplates.PropertyName.gpml_average_sample_site_position)
        non_vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, 0), pygplates.PropertyName.gpml_pole_position)
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            non_vgp_feature,
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_reconstruction_plate_id() == 1)
        self.assertTrue(len(unpartitioned_features) == 1)
        self.assertTrue(unpartitioned_features[0].get_reconstruction_plate_id() == 0)

    def test_partition_geometry(self):
        plate_partitioner = pygplates.PlatePartitioner(self.topological_features, self.rotation_features)
        
        # Test optional arguments.
        point = pygplates.PointOnSphere(0, -30)
        self.assertTrue(plate_partitioner.partition_geometry(point))
        partitioned_inside_geometries = []
        self.assertTrue(plate_partitioner.partition_geometry(point, partitioned_inside_geometries))
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        partitioned_outside_geometries = []
        self.assertTrue(plate_partitioner.partition_geometry(point, partitioned_outside_geometries=partitioned_outside_geometries))
        self.assertFalse(partitioned_outside_geometries)
        
        # Partition inside point.
        point = pygplates.PointOnSphere(0, -30)
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(point, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_points = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_points) == 1)
        self.assertTrue(inside_points[0] == point)
        
        # Partition outside point.
        point = pygplates.PointOnSphere(0, 0)
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(point, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_point = partitioned_outside_geometries[0]
        self.assertTrue(outside_point == point)
        
        # Partition inside and outside point.
        inside_point = pygplates.PointOnSphere(0, -30)
        outside_point = pygplates.PointOnSphere(0, 0)
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry([inside_point, outside_point], partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_points = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_points) == 1)
        self.assertTrue(inside_points[0] == inside_point)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        self.assertTrue(partitioned_outside_geometries[0] == outside_point)
        
        # Partition inside multipoint.
        multipoint = pygplates.MultiPointOnSphere([(15,-30), (0,-30)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == multipoint)
        
        # Partition outside multipoint.
        multipoint = pygplates.MultiPointOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == multipoint)
        
        # Partition intersecting multipoint.
        multipoint = pygplates.MultiPointOnSphere([(0,-30), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == pygplates.MultiPointOnSphere([(0,-30)]))
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == pygplates.MultiPointOnSphere([(0,0)]))
        
        # Partition intersecting multipoint.
        multipoint = pygplates.MultiPointOnSphere([(30,-30), (0,-30), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 2)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        recon_geom1, inside_geoms1 = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom1.get_feature().get_feature_id().get_string() == 'GPlates-a6054d82-6e6d-4f59-9d24-4ab255ece477')
        self.assertTrue(len(inside_geoms1) == 1)
        self.assertTrue(inside_geoms1[0] == pygplates.MultiPointOnSphere([(30,-30)]))
        recon_geom2, inside_geoms2 = partitioned_inside_geometries[1]
        self.assertTrue(recon_geom2.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms2) == 1)
        self.assertTrue(inside_geoms2[0] == pygplates.MultiPointOnSphere([(0,-30)]))
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == pygplates.MultiPointOnSphere([(0,0)]))
        
        # Partition inside polyline.
        polyline = pygplates.PolylineOnSphere([(15,-30), (0,-30)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == polyline)
        
        # Partition outside polyline.
        polyline = pygplates.PolylineOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == polyline)
        
        # Partition inside and outside polyline.
        inside_polyline = pygplates.PolylineOnSphere([(15,-30), (0,-30)])
        outside_polyline = pygplates.PolylineOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry((inside_polyline, outside_polyline), partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == inside_polyline)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        self.assertTrue(partitioned_outside_geometries[0] == outside_polyline)
        
        # Partition outside polyline.
        polyline = pygplates.PolylineOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == polyline)
        
        # Partition intersecting polyline.
        polyline = pygplates.PolylineOnSphere([(0,0), (0,-30), (30,-30), (30,-90)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        self.assertTrue(len(partitioned_outside_geometries) == 2)
        
        # Partition inside polygon.
        polygon = pygplates.PolygonOnSphere([(15,-30), (0,-30), (0,-15)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polygon, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == polygon)
        
        # Partition outside polygon.
        polygon = pygplates.PolygonOnSphere([(15,0), (0,0), (0,15)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polygon, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == polygon)
        
        # Partition intersecting polygon.
        polygon = pygplates.PolygonOnSphere([(0,0), (0,-30), (30,-30), (30,-90)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polygon, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Note that *polylines* are returned when intersecting (not polygons) - will be fixed in future.
        # Also we end up with 3 polylines outside (instead of 2).
        self.assertTrue(len(partitioned_outside_geometries) == 3)

    def test_partition_point(self):
        rotation_model = pygplates.RotationModel(self.rotation_features)
        resolved_topologies = []
        pygplates.resolve_topologies(
            self.topological_features,
            rotation_model,
            resolved_topologies,
            0)
        
        plate_partitioner = pygplates.PlatePartitioner(resolved_topologies, rotation_model)
        
        # Partition points.
        self.assertFalse(plate_partitioner.partition_point(pygplates.PointOnSphere(0, 0)))
        self.assertTrue(
                plate_partitioner.partition_point(pygplates.PointOnSphere(0, -30)).get_feature().get_feature_id().get_string() ==
                'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(
                plate_partitioner.partition_point(pygplates.PointOnSphere(30, -30)).get_feature().get_feature_id().get_string() ==
                'GPlates-a6054d82-6e6d-4f59-9d24-4ab255ece477')
        self.assertTrue(
                plate_partitioner.partition_point(pygplates.PointOnSphere(0, -60)).get_feature().get_feature_id().get_string() ==
                'GPlates-4fe56a89-d041-4494-ab07-3abead642b8e')


class ResolvedTopologiesTestCase(unittest.TestCase):
    def test_resolve_topologies(self):
        pygplates.resolve_topologies(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            os.path.join(FIXTURES, 'resolved_topologies.gmt'),
            pygplates.GeoTimeInstant(10),
            os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topologies.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topologies.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topological_sections.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))
        
        topological_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        rotation_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections)
        
        self.assertTrue(len(resolved_topologies) == 3)
        for resolved_topology in resolved_topologies:
            self.assertTrue(resolved_topology.get_resolved_feature().get_geometry() == resolved_topology.get_resolved_geometry())
        resolved_topologies_dict = dict(zip(
                (rt.get_feature().get_name() for rt in resolved_topologies),
                (rt for rt in resolved_topologies)))
        for bss in resolved_topologies_dict['topology1'].get_boundary_sub_segments():
            self.assertTrue(bss.get_topological_section_feature().get_name() in ('section2', 'section3', 'section4', 'section7', 'section8'))
            self.assertFalse(bss.get_sub_segments()) # No topological lines in 'topology1'.
        for bss in resolved_topologies_dict['topology2'].get_boundary_sub_segments():
            self.assertTrue(bss.get_topological_section_feature().get_name() in ('section4', 'section5', 'section7', 'section14', 'section9', 'section10'))
            if bss.get_topological_section_feature().get_name() == 'section14':
                self.assertTrue(set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in bss.get_sub_segments()) == set(['section11', 'section12']))
                # All sub-sub-segments in this shared sub-segment happen to have 3 vertices.
                for sub_sub_segment in bss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
            else:
                self.assertFalse(bss.get_sub_segments()) # Not from a topological line.
        for bss in resolved_topologies_dict['topology3'].get_boundary_sub_segments():
            self.assertTrue(bss.get_topological_section_feature().get_name() in ('section1', 'section2', 'section6', 'section7', 'section8', 'section14', 'section9', 'section10'))
            self.assertTrue(bss.get_resolved_feature().get_geometry() == bss.get_resolved_geometry())
            if bss.get_topological_section_feature().get_name() == 'section14':
                # We know 'section14' is a ResolvedTopologicalLine...
                self.assertTrue(bss.get_topological_section().get_resolved_line() == bss.get_topological_section_geometry())
                self.assertTrue(set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in bss.get_sub_segments()) == set(['section11', 'section12', 'section13']))
                for sub_sub_segment in bss.get_sub_segments():
                    if sub_sub_segment.get_feature().get_name() == 'section13':
                        self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 2)
                    else:
                        self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
            else:
                # We know all sections except 'section14' are ReconstructedFeatureGeometry's (not ResolvedTopologicalLine's)...
                self.assertTrue(pygplates.PolylineOnSphere(bss.get_topological_section().get_reconstructed_geometry()) == bss.get_topological_section_geometry())
                self.assertFalse(bss.get_sub_segments()) # Not from a topological line.
        
        # Sections 9 and 10 are points that now are separate sub-segments (each point is a rubber-banded line).
        # Previously they were joined into a single sub-segment (a hack).
        self.assertTrue(len(resolved_topological_sections) == 11)
        resolved_topological_sections_dict = dict(zip(
                (rts.get_topological_section_feature().get_name() for rts in resolved_topological_sections),
                (rts for rts in resolved_topological_sections)))
        for rts in resolved_topological_sections:
            self.assertTrue(rts.get_topological_section_feature().get_name() in ('section1', 'section2', 'section3', 'section4', 'section5', 'section6', 'section7', 'section8', 'section9', 'section10', 'section14'))
        
        section1_shared_sub_segments = resolved_topological_sections_dict['section1'].get_shared_sub_segments()
        self.assertTrue(len(section1_shared_sub_segments) == 1)
        for sss in section1_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']))
            self.assertTrue(sss.get_resolved_feature().get_geometry() == sss.get_resolved_geometry())
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section2_shared_sub_segments = resolved_topological_sections_dict['section2'].get_shared_sub_segments()
        self.assertTrue(len(section2_shared_sub_segments) == 2)
        for sss in section2_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']) or sharing_topologies == set(['topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section3_shared_sub_segments = resolved_topological_sections_dict['section3'].get_shared_sub_segments()
        self.assertTrue(len(section3_shared_sub_segments) == 1)
        for sss in section3_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section4_shared_sub_segments = resolved_topological_sections_dict['section4'].get_shared_sub_segments()
        self.assertTrue(len(section4_shared_sub_segments) == 2)
        for sss in section4_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']) or sharing_topologies == set(['topology2']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section5_shared_sub_segments = resolved_topological_sections_dict['section5'].get_shared_sub_segments()
        self.assertTrue(len(section5_shared_sub_segments) == 1)
        for sss in section5_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section6_shared_sub_segments = resolved_topological_sections_dict['section6'].get_shared_sub_segments()
        self.assertTrue(len(section6_shared_sub_segments) == 1)
        for sss in section6_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section7_shared_sub_segments = resolved_topological_sections_dict['section7'].get_shared_sub_segments()
        self.assertTrue(len(section7_shared_sub_segments) == 2)
        for sss in section7_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1', 'topology2']) or sharing_topologies == set(['topology2', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section8_shared_sub_segments = resolved_topological_sections_dict['section8'].get_shared_sub_segments()
        self.assertTrue(len(section8_shared_sub_segments) == 1)
        for sss in section8_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            overriding_plate, subducting_plate = sss.get_overriding_and_subducting_plates()
            overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
            self.assertTrue(overriding_plate.get_feature().get_reconstruction_plate_id() == 2)
            self.assertTrue(subducting_plate.get_feature().get_reconstruction_plate_id() == 0)
            self.assertTrue(subduction_polarity == 'Left')
        
        section9_shared_sub_segments = resolved_topological_sections_dict['section9'].get_shared_sub_segments()
        self.assertTrue(len(section9_shared_sub_segments) == 1)
        for sss in section9_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section10_shared_sub_segments = resolved_topological_sections_dict['section10'].get_shared_sub_segments()
        self.assertTrue(len(section10_shared_sub_segments) == 1)
        for sss in section10_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
        
        section14_shared_sub_segments = resolved_topological_sections_dict['section14'].get_shared_sub_segments()
        self.assertTrue(len(section14_shared_sub_segments) == 2)
        for sss in section14_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']) or sharing_topologies == set(['topology2', 'topology3']))
            sub_sub_segments = set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in sss.get_sub_segments())
            if sharing_topologies == set(['topology3']):
                self.assertTrue(sub_sub_segments == set(['section12', 'section13']))
                # All sub-sub-segments in this shared sub-segment happen to have 2 vertices.
                for sub_sub_segment in sss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 2)
                self.assertFalse(sss.get_overriding_and_subducting_plates()) # Don't have two sharing plates (only one).
            else:
                self.assertTrue(sub_sub_segments == set(['section11', 'section12']))
                # All sub-sub-segments in this shared sub-segment happen to have 3 vertices.
                for sub_sub_segment in sss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_reconstruction_plate_id() == 0)
                self.assertTrue(subducting_plate.get_feature().get_reconstruction_plate_id() == 1)
                self.assertTrue(subduction_polarity == 'Right')

        # This time exclude networks from the topological sections (but not the topologies).
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            resolve_topological_section_types = pygplates.ResolveTopologyType.boundary)
        self.assertTrue(len(resolved_topologies) == 3)
        self.assertTrue(len(resolved_topological_sections) == 9)

        # This time exclude networks from the topologies (but not the topological sections).
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            resolve_topology_types = pygplates.ResolveTopologyType.boundary,
            resolve_topological_section_types = pygplates.ResolveTopologyType.boundary | pygplates.ResolveTopologyType.network)
        self.assertTrue(len(resolved_topologies) == 2)
        self.assertTrue(len(resolved_topological_sections) == 11)

        # This time exclude networks from both the topologies and the topological sections.
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            resolve_topology_types = pygplates.ResolveTopologyType.boundary)
        self.assertTrue(len(resolved_topologies) == 2)
        self.assertTrue(len(resolved_topological_sections) == 9)


class ReconstructionTreeCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        self.reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], 10.0)

    def test_create(self):
        self.assertTrue(isinstance(self.reconstruction_tree, pygplates.ReconstructionTree))
        # Create again with a GeoTimeInstant instead of float.
        self.reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], pygplates.GeoTimeInstant(10.0))
        self.assertRaises(pygplates.InterpolationError,
                pygplates.ReconstructionTree, [self.rotations], pygplates.GeoTimeInstant.create_distant_past())
        self.assertEqual(self.reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(self.reconstruction_tree.get_reconstruction_time() > 9.9999 and
                self.reconstruction_tree.get_reconstruction_time() < 10.00001)
        
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.ReconstructionTree,
                [ 'non_existent_file.rot' ],
                10.0)
        # Create using feature collections.
        reconstruction_tree = pygplates.ReconstructionTree([self.rotations], 10.0)
        # Create using a single feature collection.
        reconstruction_tree = pygplates.ReconstructionTree(self.rotations, 10.0)
        # Create using a list of features.
        reconstruction_tree = pygplates.ReconstructionTree([rotation for rotation in self.rotations], 10.0)
        # Create using a single feature.
        reconstruction_tree = pygplates.ReconstructionTree(next(iter(self.rotations)), 10.0)
        # Create using a feature collection, list of features and a single feature (dividing the rotation features between them).
        reconstruction_tree = pygplates.ReconstructionTree(
                [ pygplates.FeatureCollection(self.rotations.get(lambda feature: True, pygplates.FeatureReturn.all)[0:2]), # First and second features
                    list(self.rotations)[2:-1], # All but first, second and last features
                    list(self.rotations)[-1]], # Last feature
                10.0)

    def test_build(self):
        def build_tree(builder, rotation_features, reconstruction_time):
            for rotation_feature in rotation_features:
                trp = rotation_feature.get_total_reconstruction_pole()
                if trp:
                    fixed_plate_id, moving_plate_id, total_reconstruction_pole = trp
                    interpolated_rotation = total_reconstruction_pole.get_value(reconstruction_time)
                    if interpolated_rotation:
                        builder.insert_total_reconstruction_pole(
                                fixed_plate_id,
                                moving_plate_id,
                                interpolated_rotation.get_finite_rotation())
        
        # Build using a ReconstructionTreeBuilder.
        builder = pygplates.ReconstructionTreeBuilder()
        reconstruction_time = self.reconstruction_tree.get_reconstruction_time()
        build_tree(builder, self.rotations, reconstruction_time)
        built_reconstruction_tree = builder.build_reconstruction_tree(
                self.reconstruction_tree.get_anchor_plate_id(),
                reconstruction_time)
        self.assertTrue(isinstance(built_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertEqual(built_reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(built_reconstruction_tree.get_reconstruction_time() > 9.9999 and
                built_reconstruction_tree.get_reconstruction_time() < 10.00001)
        self.assertTrue(len(built_reconstruction_tree.get_edges()) == 447)
        # Building again without inserting poles will give an empty reconstruction tree.
        built_reconstruction_tree = builder.build_reconstruction_tree(
                self.reconstruction_tree.get_anchor_plate_id(),
                reconstruction_time)
        self.assertTrue(len(built_reconstruction_tree.get_edges()) == 0)
        # Build again (because ReconstructionTreeBuilder.build_reconstruction_tree clears state).
        build_tree(builder, self.rotations, pygplates.GeoTimeInstant(reconstruction_time))
        # Build using GeoTimeInstant instead of float.
        built_reconstruction_tree = builder.build_reconstruction_tree(
                self.reconstruction_tree.get_anchor_plate_id(),
                pygplates.GeoTimeInstant(reconstruction_time))
        self.assertTrue(isinstance(built_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertEqual(built_reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(built_reconstruction_tree.get_reconstruction_time() > 9.9999 and
                built_reconstruction_tree.get_reconstruction_time() < 10.00001)
        self.assertTrue(len(built_reconstruction_tree.get_edges()) == 447)

    def test_get_edge(self):
        # Should not be able to get an edge for the anchor plate id.
        self.assertFalse(self.reconstruction_tree.get_edge(self.reconstruction_tree.get_anchor_plate_id()))
        edge = self.reconstruction_tree.get_edge(101)
        self.assertTrue(edge.get_moving_plate_id() == 101)
        self.assertTrue(edge.get_fixed_plate_id() == 714)
        self.assertTrue(isinstance(edge.get_equivalent_total_rotation(), pygplates.FiniteRotation))
        self.assertTrue(isinstance(edge.get_relative_total_rotation(), pygplates.FiniteRotation))
        
        child_edges = edge.get_child_edges()
        self.assertTrue(len(child_edges) == 33)
        chld_edge_count = 0
        for child_edge in child_edges:
            self.assertTrue(child_edge.get_fixed_plate_id() == edge.get_moving_plate_id())
            chld_edge_count += 1
        self.assertEquals(chld_edge_count, len(child_edges))
    
    def test_anchor_plate_edges(self):
        anchor_plate_edges = self.reconstruction_tree.get_anchor_plate_edges()
        for i in range(0, len(anchor_plate_edges)):
            self.assertTrue(anchor_plate_edges[i].get_fixed_plate_id() == self.reconstruction_tree.get_anchor_plate_id())
        self.assertTrue(len(anchor_plate_edges) == 2)
        
        edge_count = 0
        for edge in anchor_plate_edges:
            self.assertTrue(edge.get_fixed_plate_id() == self.reconstruction_tree.get_anchor_plate_id())
            edge_count += 1
        self.assertEquals(edge_count, len(anchor_plate_edges))
    
    def test_edges(self):
        edges = self.reconstruction_tree.get_edges()
        for i in range(0, len(edges)):
            self.assertTrue(isinstance(edges[i], pygplates.ReconstructionTreeEdge))
        self.assertTrue(len(edges) == 447)
        
        edge_count = 0
        for edge in edges:
            edge_count += 1
        self.assertEquals(edge_count, len(edges))

    def test_get_parent_traversal(self):
        edge = self.reconstruction_tree.get_edge(907)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 301)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 101)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 714)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 701)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 1)
        self.assertTrue(edge.get_fixed_plate_id() == 0)
        edge = edge.get_parent_edge()
        # Reached anchor plate.
        self.assertFalse(edge)

    def test_total_rotation(self):
        self.assertTrue(isinstance(
                self.reconstruction_tree.get_equivalent_total_rotation(802),
                pygplates.FiniteRotation))
        self.assertTrue(isinstance(
                # Pick plates that are in different sub-trees.
                self.reconstruction_tree.get_relative_total_rotation(802, 291),
                pygplates.FiniteRotation))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                self.reconstruction_tree.get_equivalent_total_rotation(802),
                self.reconstruction_tree.get_relative_total_rotation(802, 0)))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                self.reconstruction_tree.get_edge(802).get_relative_total_rotation(),
                self.reconstruction_tree.get_relative_total_rotation(
                        802,
                        self.reconstruction_tree.get_edge(802).get_fixed_plate_id())))
        # Should return identity rotation.
        self.assertTrue(self.reconstruction_tree.get_equivalent_total_rotation(
                self.reconstruction_tree.get_anchor_plate_id()).represents_identity_rotation())
        self.assertTrue(self.reconstruction_tree.get_relative_total_rotation(
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id()).represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(self.reconstruction_tree.get_equivalent_total_rotation(10000, use_identity_for_missing_plate_ids=False))
        # Should return None for an unknown relative plate id.
        self.assertFalse(self.reconstruction_tree.get_relative_total_rotation(802, 10000, use_identity_for_missing_plate_ids=False))
    
    def test_stage_rotation(self):
        from_reconstruction_tree = pygplates.ReconstructionTree(
                [ self.rotations ],
                # Further in the past...
                self.reconstruction_tree.get_reconstruction_time() + 1)
        
        equivalent_stage_rotation = pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802)
        self.assertTrue(isinstance(equivalent_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 10000, use_identity_for_missing_plate_ids=False))
        # Should raise error for differing anchor plate ids (0 versus 291).
        self.assertRaises(
                pygplates.DifferentAnchoredPlatesInReconstructionTreesError,
                pygplates.ReconstructionTree.get_equivalent_stage_rotation,
                pygplates.ReconstructionTree([self.rotations], 11, 291),
                self.reconstruction_tree,
                802)
        
        relative_stage_rotation = pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802, 291)
        self.assertTrue(isinstance(relative_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802, 10000, use_identity_for_missing_plate_ids=False))
        # Should return None for an unknown relative plate id.
        self.assertFalse(pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 10000, 291, use_identity_for_missing_plate_ids=False))


class RotationModelCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        self.rotation_model = pygplates.RotationModel([ os.path.join(FIXTURES, 'rotations.rot') ])
        self.from_time = 20.0
        self.to_time = pygplates.GeoTimeInstant(10.0)
        self.from_reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], self.from_time)
        self.to_reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], self.to_time)

    def test_create(self):
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.RotationModel,
                [ 'non_existent_file.rot' ])
        #
        # UPDATE: Argument 'clone_rotation_features' is deprecated in revision 25.
        #         And argument 'extend_total_reconstruction_poles_to_distant_past' was added in revision 25.
        #
        # Create using feature collections instead of filenames.
        rotation_model = pygplates.RotationModel([self.rotations], clone_rotation_features=False)
        # Create using a single feature collection.
        rotation_model = pygplates.RotationModel(self.rotations, clone_rotation_features=False)
        # Create using a list of features.
        rotation_model = pygplates.RotationModel([rotation for rotation in self.rotations], clone_rotation_features=False)
        # Create using a single feature.
        rotation_model = pygplates.RotationModel(next(iter(self.rotations)), clone_rotation_features=False)
        # Create using a mixture of the above.
        rotation_model = pygplates.RotationModel(
                [os.path.join(FIXTURES, 'rotations.rot'),
                    self.rotations,
                    [rotation for rotation in self.rotations],
                    next(iter(self.rotations))],
                clone_rotation_features=False)
        # Create a reference to the same (C++) rotation model.
        rotation_model_reference = pygplates.RotationModel(rotation_model)
        self.assertTrue(rotation_model_reference == rotation_model)
        
        # Adapt an existing rotation model to use a different cache size and default anchor plate ID.
        rotation_model_adapted = pygplates.RotationModel(self.rotation_model, default_anchor_plate_id=802)
        self.assertTrue(rotation_model_adapted.get_default_anchor_plate_id() == 802)
        self.assertTrue(rotation_model_adapted != self.rotation_model)  # Should be a different C++ instance.
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_adapted.get_rotation(self.to_time, 802),
                self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=802)))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_adapted.get_rotation(self.to_time, 802, anchor_plate_id=0),
                self.rotation_model.get_rotation(self.to_time, 802)))
        # Make sure newly adapted model using a new cache size but delegating default anchor plate ID actually delegates.
        another_rotation_model_adapted = pygplates.RotationModel(rotation_model_adapted, 32)
        self.assertTrue(another_rotation_model_adapted.get_default_anchor_plate_id() == 802)
        
        # Test using a non-zero default anchor plate ID.
        rotation_model_non_zero_default_anchor = pygplates.RotationModel(self.rotations, default_anchor_plate_id=802)
        self.assertTrue(rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802).represents_identity_rotation())
        self.assertFalse(rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802, anchor_plate_id=0).represents_identity_rotation())
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802),
                self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=802)))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802, anchor_plate_id=0),
                self.rotation_model.get_rotation(self.to_time, 802)))
        
        # Test extending total reconstruction poles to distant past.
        rotation_model_not_extended = pygplates.RotationModel(self.rotations)
        # At 1000Ma there are no rotations (for un-extended model).
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        # Deprecated version (triggered by explicitly specifying 'clone_rotation_features' argument) is also an un-extended model.
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, clone_rotation_features=False)
        # At 1000Ma there are no rotations (for un-extended model).
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, clone_rotation_features=True)
        # At 1000Ma there are no rotations (for un-extended model).
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, extend_total_reconstruction_poles_to_distant_past=False)
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        # This should choose the 'extend_total_reconstruction_poles_to_distant_past' __init__ overload instead of the
        # deprecated (not documented) overload accepting 'clone_rotation_features'.
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, 100, False)
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        
        rotation_model_extended = pygplates.RotationModel(self.rotations, extend_total_reconstruction_poles_to_distant_past=True)
        # But at 1000Ma there are rotations (for extended model).
        self.assertTrue(rotation_model_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        # This should still choose the 'extend_total_reconstruction_poles_to_distant_past' __init__ overload instead of the
        # deprecated (not documented) overload accepting 'clone_rotation_features'.
        rotation_model_extended = pygplates.RotationModel(self.rotations, 100, True)
        self.assertTrue(rotation_model_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
    
    def test_get_reconstruction_tree(self):
        to_reconstruction_tree = self.rotation_model.get_reconstruction_tree(self.to_time)
        self.assertTrue(isinstance(to_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertTrue(to_reconstruction_tree.get_reconstruction_time() > self.to_time.get_value() - 1e-6 and
                to_reconstruction_tree.get_reconstruction_time() < self.to_time.get_value() + 1e-6)
        self.assertRaises(pygplates.InterpolationError,
                pygplates.RotationModel.get_reconstruction_tree, self.rotation_model, pygplates.GeoTimeInstant.create_distant_past())
    
    def test_get_rotation(self):
        equivalent_total_rotation = self.rotation_model.get_rotation(self.to_time, 802)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                equivalent_total_rotation,
                self.to_reconstruction_tree.get_equivalent_total_rotation(802)))
        self.assertRaises(pygplates.InterpolationError,
                pygplates.RotationModel.get_rotation, self.rotation_model, pygplates.GeoTimeInstant.create_distant_past(), 802)
        
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        # Fixed plate id defaults to anchored plate id.
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        # Shouldn't really matter what the anchor plate id is (as long as there's a plate circuit
        # path from anchor plate to both fixed and moving plates.
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291, anchor_plate_id=802)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        
        equivalent_stage_rotation = self.rotation_model.get_rotation(self.to_time, 802, self.from_time)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                equivalent_stage_rotation,
                pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802)))
        
        relative_stage_rotation = self.rotation_model.get_rotation(self.to_time, 802, self.from_time, 291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_stage_rotation,
                pygplates.ReconstructionTree.get_relative_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802,
                        291)))
        # Fixed plate id defaults to anchored plate id.
        relative_stage_rotation = self.rotation_model.get_rotation(
                self.to_time, 802, pygplates.GeoTimeInstant(self.from_time), anchor_plate_id=291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_stage_rotation,
                pygplates.ReconstructionTree.get_relative_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802,
                        291)))
        
        # Ensure that specifying 'from_time' at present day (ie, 0Ma) does not assume zero finite rotation (at present day).
        non_zero_present_day_rotation_model = pygplates.RotationModel(
            pygplates.Feature.create_total_reconstruction_sequence(
                0,
                801,
                pygplates.GpmlIrregularSampling([
                    pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                            pygplates.FiniteRotation((0, 0), 1.57)),
                        0.0), # non-zero finite rotation at present day
                    pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                            pygplates.FiniteRotation.create_identity_rotation()),
                        10.0)
                    ])))
        # Non-zero finite rotation.
        self.assertFalse(non_zero_present_day_rotation_model.get_rotation(0.0, 801).represents_identity_rotation())
        # Just looks at 10Ma.
        self.assertTrue(non_zero_present_day_rotation_model.get_rotation(10.0, 801).represents_identity_rotation())
        # 10Ma relative to non-zero finite rotation at present day.
        #
        #   R(0->time, A->Plate) = R(time, A->Plate) * inverse[R(0, A->Plate)]
        self.assertTrue(
            non_zero_present_day_rotation_model.get_rotation(10.0, 801, 0.0) ==
            non_zero_present_day_rotation_model.get_rotation(10.0, 801) * non_zero_present_day_rotation_model.get_rotation(0.0, 801).get_inverse())


class TopologicalModelCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        self.rotation_model = pygplates.RotationModel(self.rotations)

        self.topologies = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        self.topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, 2)

    def test_create(self):
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.TopologicalModel,
                'non_existant_topology_file.gpml', self.rotations, 2)

        # 'oldest_time - youngest_time' not an integer multiple of time_increment
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 5, time_increment=2)
        # oldest_time later (or same as) youngest_time
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4, 5)
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4, 4)
        # Oldest/youngest times cannot be distant-past or distant-future.
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, pygplates.GeoTimeInstant.create_distant_past())
        # Oldest/youngest times and time increment must have integral values.
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4.01)
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4, 1.99)
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4, 1, 0.99)
        # Time increment must be positive.
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4, time_increment=-1)
        self.assertRaises(
                ValueError,
                pygplates.TopologicalModel,
                self.topologies, self.rotations, 4, time_increment=0)

        topological_model = pygplates.TopologicalModel(self.topologies, self.rotations, 2)
        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, 2, time_increment=2)

        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, 2, anchor_plate_id=1)
        self.assertTrue(topological_model.get_anchor_plate_id() == 1)

    def test_get_rotation_model(self):
        self.assertTrue(self.topological_model.get_rotation_model().get_rotation(1.0, 802) == self.rotation_model.get_rotation(1.0, 802))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            CalculateVelocitiesTestCase,
            CrossoverTestCase,
            InterpolateTotalReconstructionSequenceTestCase,
            PlatePartitionerTestCase,
            ReconstructTestCase,
            ReconstructionTreeCase,
            ResolvedTopologiesTestCase,
            RotationModelCase,
            TopologicalModelCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
