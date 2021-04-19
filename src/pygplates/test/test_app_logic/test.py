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
            resolved_topological_sections,
            # Make sure can pass in optional ResolveTopologyParameters...
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())

        # Make sure can specify ResolveTopologyParameters with the topological features.
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            (topological_features, pygplates.ResolveTopologyParameters()),
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            # Make sure can pass in optional ResolveTopologyParameters...
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        topological_features = list(topological_features)
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            [
                (topological_features[0], pygplates.ResolveTopologyParameters()),  # single feature with ResolveTopologyParameters
                topological_features[1],  # single feature without ResolveTopologyParameters
                (topological_features[2:4], pygplates.ResolveTopologyParameters()),  # multiple features with ResolveTopologyParameters
                topological_features[4:],  # multiple features without ResolveTopologyParameters
            ],
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections)
        
        self.assertTrue(len(resolved_topologies) == 7)
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
        self.assertTrue(len(resolved_topological_sections) == 17)
        resolved_topological_sections_dict = dict(zip(
                (rts.get_topological_section_feature().get_name() for rts in resolved_topological_sections),
                (rts for rts in resolved_topological_sections)))
        for rts in resolved_topological_sections:
            self.assertTrue(rts.get_topological_section_feature().get_name() in (
                'section1', 'section2', 'section3', 'section4', 'section5', 'section6', 'section7', 'section8', 'section9', 'section10', 'section14',
                'section15', 'section16', 'section17', 'section18', 'section19', 'section20'))
        
        section1_shared_sub_segments = resolved_topological_sections_dict['section1'].get_shared_sub_segments()
        self.assertTrue(len(section1_shared_sub_segments) == 1)
        for sss in section1_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']))
            self.assertTrue(sss.get_resolved_feature().get_geometry() == sss.get_resolved_geometry())
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section2_shared_sub_segments = resolved_topological_sections_dict['section2'].get_shared_sub_segments()
        self.assertTrue(len(section2_shared_sub_segments) == 2)
        for sss in section2_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']) or sharing_topologies == set(['topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section3_shared_sub_segments = resolved_topological_sections_dict['section3'].get_shared_sub_segments()
        self.assertTrue(len(section3_shared_sub_segments) == 1)
        for sss in section3_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section4_shared_sub_segments = resolved_topological_sections_dict['section4'].get_shared_sub_segments()
        self.assertTrue(len(section4_shared_sub_segments) == 2)
        for sss in section4_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']) or sharing_topologies == set(['topology2']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section5_shared_sub_segments = resolved_topological_sections_dict['section5'].get_shared_sub_segments()
        self.assertTrue(len(section5_shared_sub_segments) == 3)
        for sss in section5_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology4']) or
                            sharing_topologies == set(['topology2', 'topology5']) or
                            sharing_topologies == set(['topology5']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            if sharing_topologies == set(['topology5']):
                self.assertFalse(sss.get_overriding_and_subducting_plates()) # Only one adjacent plate.
            else:
                self.assertTrue(sss.get_overriding_and_subducting_plates()) # Two adjacent plates.
            subducting_plate = sss.get_subducting_plate(False)
            # Can always find just the subducting plate though.
            self.assertTrue(subducting_plate.get_feature().get_name() == 'topology4' or
                            subducting_plate.get_feature().get_name() == 'topology5')
        
        section6_shared_sub_segments = resolved_topological_sections_dict['section6'].get_shared_sub_segments()
        self.assertTrue(len(section6_shared_sub_segments) == 1)
        for sss in section6_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section7_shared_sub_segments = resolved_topological_sections_dict['section7'].get_shared_sub_segments()
        self.assertTrue(len(section7_shared_sub_segments) == 2)
        for sss in section7_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1', 'topology2']) or sharing_topologies == set(['topology2', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
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
            subducting_plate = sss.get_subducting_plate()
            subducting_plate, subduction_polarity = sss.get_subducting_plate(True)
            self.assertTrue(subducting_plate.get_feature().get_reconstruction_plate_id() == 0)
            self.assertTrue(subduction_polarity == 'Left')
        
        # 'section9' is a single point.
        section9_shared_sub_segments = resolved_topological_sections_dict['section9'].get_shared_sub_segments()
        self.assertTrue(len(section9_shared_sub_segments) == 1)
        for sss in section9_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology3']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            self.assertTrue(len(sharing_topology_reversal_flags) == 2)
            # One sub-segment should be reversed and the other not.
            self.assertTrue(sharing_topology_reversal_flags['topology2'] != sharing_topology_reversal_flags['topology3'])
            resolved_sub_segment_geom = sss.get_resolved_geometry()
            self.assertTrue(len(resolved_sub_segment_geom) == 3)  # A polyline with 3 points (one section point and two rubber band points).
            # 'topology2' is clockwise and 'section9' is on its left side so start rubber point should be more Southern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology2']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
            # 'topology3' is clockwise and 'section9' is on its right side so start rubber point should be more Northern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology3']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        # 'section10' is a single point.
        section10_shared_sub_segments = resolved_topological_sections_dict['section10'].get_shared_sub_segments()
        self.assertTrue(len(section10_shared_sub_segments) == 1)
        for sss in section10_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology3']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            self.assertTrue(len(sharing_topology_reversal_flags) == 2)
            # One sub-segment should be reversed and the other not.
            self.assertTrue(sharing_topology_reversal_flags['topology2'] != sharing_topology_reversal_flags['topology3'])
            resolved_sub_segment_geom = sss.get_resolved_geometry()
            self.assertTrue(len(resolved_sub_segment_geom) == 3)  # A polyline with 3 points (one section point and two rubber band points).
            # 'topology2' is clockwise and 'section10' is on its left side so start rubber point should be more Southern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology2']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
            # 'topology3' is clockwise and 'section10' is on its right side so start rubber point should be more Northern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology3']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        # Sections 11, 12, 13 are not resolved topological sections since they're only used in a resolved topological line (not in boundaries/networks).
        self.assertTrue('section11' not in resolved_topological_sections_dict)
        self.assertTrue('section12' not in resolved_topological_sections_dict)
        self.assertTrue('section13' not in resolved_topological_sections_dict)
        
        section14_shared_sub_segments = resolved_topological_sections_dict['section14'].get_shared_sub_segments()
        self.assertTrue(len(section14_shared_sub_segments) == 4)
        for sss in section14_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology7']) or
                            sharing_topologies == set(['topology7', 'topology3']) or
                            sharing_topologies == set(['topology4', 'topology3']) or
                            sharing_topologies == set(['topology2', 'topology3']))
            sub_sub_segments = set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in sss.get_sub_segments())
            if sharing_topologies == set(['topology7']):
                self.assertTrue(sub_sub_segments == set(['section13']))
                # The one shared sub-segment happens to have 3 vertices (two from resolved line and one rubber band).
                self.assertTrue(len(sss.get_sub_segments()) == 1)
                self.assertTrue(len(sss.get_sub_segments()[0].get_resolved_geometry()) == 3)
                self.assertTrue(len(sss.get_resolved_geometry()) == 3)
                self.assertFalse(sss.get_overriding_and_subducting_plates()) # Don't have two sharing plates (only one).
            elif sharing_topologies == set(['topology7', 'topology3']):
                self.assertTrue(sub_sub_segments == set(['section13']))
                # The one shared sub-segment happens to have 2 vertices (from resolved line).
                self.assertTrue(len(sss.get_sub_segments()) == 1)
                self.assertTrue(len(sss.get_sub_segments()[0].get_resolved_geometry()) == 2)
                self.assertTrue(len(sss.get_resolved_geometry()) == 2)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology3')
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology7')
                self.assertTrue(subduction_polarity == 'Right')
            elif sharing_topologies == set(['topology4', 'topology3']):
                self.assertTrue(sub_sub_segments == set(['section12', 'section13']))
                # All sub-sub-segments in this shared sub-segment happen to have 2 vertices.
                for sub_sub_segment in sss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 2)
                # Note that sub-sub-segment rubber band points don't contribute to resolved topo line (otherwise there'd be 3 points)...
                self.assertTrue(len(sss.get_resolved_geometry()) == 2)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology3')
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology4')
                self.assertTrue(subduction_polarity == 'Right')
            elif sharing_topologies == set(['topology2', 'topology3']):
                self.assertTrue(sub_sub_segments == set(['section11', 'section12']))
                # All sub-sub-segments in this shared sub-segment happen to have 3 vertices.
                for sub_sub_segment in sss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
                # Note that sub-sub-segment rubber band points don't contribute to resolved topo line (which means 3 instead of 4)
                # but there's a rubber band point on the resolved topo line itself which brings total to 4...
                self.assertTrue(len(sss.get_resolved_geometry()) == 4)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology3')
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology2')
                self.assertTrue(subduction_polarity == 'Right')
        
        # 'section15' is a single point.
        section15_shared_sub_segments = resolved_topological_sections_dict['section15'].get_shared_sub_segments()
        
        # Make a function for testing 'section15' since we're re-use it later below.
        def _internal_test_section15_shared_sub_segments(test_case, section15_shared_sub_segments):
            test_case.assertTrue(len(section15_shared_sub_segments) == 4)
            for sss in section15_shared_sub_segments:
                sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
                test_case.assertTrue(sharing_topologies == set(['topology4', 'topology5']) or
                                sharing_topologies == set(['topology5', 'topology6']) or
                                sharing_topologies == set(['topology4', 'topology7']) or
                                sharing_topologies == set(['topology6', 'topology7']))
                # Dict of topology names to reversal flags.
                sharing_topology_reversal_flags = dict(zip(
                        (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                        sss.get_sharing_resolved_topology_geometry_reversal_flags()))
                if sharing_topologies == set(['topology4', 'topology5']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology4' is clockwise and sub-segment is on its right side so should go North to South (unless reversed).
                    if sharing_topology_reversal_flags['topology4']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                    # 'topology5' is clockwise and sub-segment is on its left side so should go South to North (unless reversed).
                    if sharing_topology_reversal_flags['topology5']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                elif sharing_topologies == set(['topology5', 'topology6']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology5' is clockwise and sub-segment is on its lower side so should go East to West (unless reversed).
                    if sharing_topology_reversal_flags['topology5']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                    # 'topology6' is counter-clockwise and sub-segment is on its upper side so should go East to West (unless reversed).
                    if sharing_topology_reversal_flags['topology6']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                elif sharing_topologies == set(['topology4', 'topology7']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology4' is clockwise and sub-segment is on its lower side so should go East to West (unless reversed).
                    if sharing_topology_reversal_flags['topology4']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                    # 'topology7' is clockwise and sub-segment is on its upper side so should go West to East (unless reversed).
                    if sharing_topology_reversal_flags['topology7']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                elif sharing_topologies == set(['topology6', 'topology7']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology6' is counter-clockwise and sub-segment is on its left side so should go North to South (unless reversed).
                    if sharing_topology_reversal_flags['topology6']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                    # 'topology7' is clockwise and sub-segment is on its right side so should go North to South (unless reversed).
                    if sharing_topology_reversal_flags['topology7']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
        
        _internal_test_section15_shared_sub_segments(self, section15_shared_sub_segments)
        
        # 'section16' is a single point.
        section16_shared_sub_segments = resolved_topological_sections_dict['section16'].get_shared_sub_segments()
        self.assertTrue(len(section16_shared_sub_segments) == 3)
        for sss in section16_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology6', 'topology7']) or
                            sharing_topologies == set(['topology6']) or
                            sharing_topologies == set(['topology7']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            if sharing_topologies == set(['topology6', 'topology7']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 2)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Both sub-segments not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                self.assertTrue(not sharing_topology_reversal_flags['topology7'])
                # 'topology7' is clockwise and 'topology6' is counter-clockwise so sub-segment goes North to South.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
            elif sharing_topologies == set(['topology6']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                # 'topology6' is counter-clockwise so sub-segment goes West to East.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
            elif sharing_topologies == set(['topology7']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology7'])
                # 'topology7' is clockwise so sub-segment goes East to West.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
        
        # 'section17' is a single point.
        section17_shared_sub_segments = resolved_topological_sections_dict['section17'].get_shared_sub_segments()
        self.assertTrue(len(section17_shared_sub_segments) == 3)
        for sss in section17_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology5', 'topology6']) or
                            sharing_topologies == set(['topology5']) or
                            sharing_topologies == set(['topology6']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            if sharing_topologies == set(['topology5', 'topology6']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 2)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Both sub-segments not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology5'])
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                # 'topology5' is clockwise and 'topology6' is counter-clockwise so sub-segment goes East to West.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
            elif sharing_topologies == set(['topology5']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology5'])
                # 'topology5' is clockwise so sub-segment goes North to South.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
            elif sharing_topologies == set(['topology6']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                # 'topology6' is counter-clockwise so sub-segment goes South to North.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
        
        # 'section18' is a single point.
        section18_shared_sub_segments = resolved_topological_sections_dict['section18'].get_shared_sub_segments()
        self.assertTrue(len(section18_shared_sub_segments) == 1)
        for sss in section18_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology6']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            self.assertTrue(len(sharing_topology_reversal_flags) == 1)
            # One sub-segment and should not be reversed because it's shared by only a single topology
            # (and point sections can only get reversed if another topology shares it).
            self.assertTrue(not sharing_topology_reversal_flags['topology6'])
            resolved_sub_segment_geom = sss.get_resolved_geometry()
            self.assertTrue(len(resolved_sub_segment_geom) == 3)  # A polyline with 3 points (one section point and two rubber band points).
            # 'topology6' is counter-clockwise and 'section18' is on its bottom-right corner so start rubber point should be more Southern than end rubber point (unless reversed).
            self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        # Test 'section15' still gives correct result when changing order of adding topologies
        # (from order 4->5->6->7 to order 4->6->5->7) which changes the shared sub-segment reversals.
        # We still get 4 shared sub-segments rubber banding to the single point of 'section15' and
        # each shared sub-segment connects 2 topologies as usual. It's just some reversals change
        # and we're testing a different path through the pyGPlates implementation.
        def _test_reordered_topological_features(
                test_case,
                topological_features,
                rotation_features,
                topologies_4_5_6_7_order_keys):
            
            reordered_topological_features = sorted(topological_features,
                key = lambda f:
                # Assign keys to get order 4->6->5->7...
                topologies_4_5_6_7_order_keys[0] if f.get_name() == 'topology4' else
                topologies_4_5_6_7_order_keys[1] if f.get_name() == 'topology5' else
                topologies_4_5_6_7_order_keys[2] if f.get_name() == 'topology6' else
                topologies_4_5_6_7_order_keys[3] if f.get_name() == 'topology7' else
                # All other features get same key and so retain same order...
                0)
            resolved_topologies = []
            resolved_topological_sections = []
            pygplates.resolve_topologies(
                reordered_topological_features,
                rotation_features,
                resolved_topologies,
                10,
                resolved_topological_sections)
            resolved_topological_section15 = next(filter(lambda rts: rts.get_feature().get_name() == 'section15', resolved_topological_sections))
            section15_shared_sub_segments = resolved_topological_section15.get_shared_sub_segments()
            # Re-use function defined above for testing the shared sub-segments of 'section15'.
            _internal_test_section15_shared_sub_segments(test_case, section15_shared_sub_segments)
        
        # Test with different ordering of topologies 4, 5, 6 and 7.
        #_test_reordered_topological_features(self, topological_features, rotation_features, [1, 2, 3, 4])  # The normal order already tested above.
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 2, 4, 3])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 3, 2, 4])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 3, 4, 2])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 4, 2, 3])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 4, 3, 2])

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
        self.assertTrue(len(resolved_topologies) == 7)
        self.assertTrue(len(resolved_topological_sections) == 15)

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
        self.assertTrue(len(resolved_topologies) == 6)
        self.assertTrue(len(resolved_topological_sections) == 17)

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
        self.assertTrue(len(resolved_topologies) == 6)
        self.assertTrue(len(resolved_topological_sections) == 15)


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
        self.topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model)

    def test_create(self):
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.TopologicalModel,
                'non_existant_topology_file.gpml', self.rotations)

        self.assertTrue(self.topological_model.get_anchor_plate_id() == 0)

        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, anchor_plate_id=1)
        self.assertTrue(topological_model.get_anchor_plate_id() == 1)

        # Make sure can pass in optional ResolveTopologyParameters.
        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        self.assertTrue(topological_model.get_anchor_plate_id() == 0)
        # Make sure can specify ResolveTopologyParameters with the topological features.
        topological_model = pygplates.TopologicalModel(
                (self.topologies, pygplates.ResolveTopologyParameters()),
                self.rotation_model,
                default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        topologies_list = list(self.topologies)
        topological_model = pygplates.TopologicalModel(
                [
                    (topologies_list[0], pygplates.ResolveTopologyParameters()),  # single feature with ResolveTopologyParameters
                    topologies_list[1],  # single feature without ResolveTopologyParameters
                    (topologies_list[2:4], pygplates.ResolveTopologyParameters()),  # multiple features with ResolveTopologyParameters
                    topologies_list[4:],  # multiple features without ResolveTopologyParameters
                ],
                self.rotation_model)

    def test_get_topological_snapshot(self):
        topological_snapshot = self.topological_model.topological_snapshot(10.0)
        self.assertTrue(topological_snapshot.get_anchor_plate_id() == self.topological_model.get_anchor_plate_id())
        self.assertTrue(topological_snapshot.get_rotation_model() == self.topological_model.get_rotation_model())

    def test_get_rotation_model(self):
        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, anchor_plate_id=2)
        self.assertTrue(topological_model.get_rotation_model().get_rotation(1.0, 802) == self.rotation_model.get_rotation(1.0, 802, anchor_plate_id=2))
        self.assertTrue(topological_model.get_rotation_model().get_default_anchor_plate_id() == 2)

        rotation_model_anchor_2 = pygplates.RotationModel(self.rotations, default_anchor_plate_id=2)
        topological_model = pygplates.TopologicalModel(self.topologies, rotation_model_anchor_2)
        self.assertTrue(topological_model.get_anchor_plate_id() == 2)
        self.assertTrue(topological_model.get_rotation_model().get_default_anchor_plate_id() == 2)

    def test_reconstruct_geometry(self):
        # Create from a multipoint.
        multipoint =  pygplates.MultiPointOnSphere([(0,0), (10,10)])
        reconstructed_multipoint_time_span = self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})
        # Create from a point.
        reconstructed_point_time_span = self.topological_model.reconstruct_geometry(
                pygplates.PointOnSphere(0, 0),
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0]})
        # Create from a sequence of points.
        reconstructed_points_time_span = self.topological_model.reconstruct_geometry(
                [(0, 0), (5, 5)],
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})

        # Number of scalars must match number of points.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                pygplates.PointOnSphere(0, 0),
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})
        # 'oldest_time - youngest_time' not an integer multiple of time_increment
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=5,
                time_increment=2)
        # oldest_time later (or same as) youngest_time
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=5)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=4)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,  # initial_time and oldest_time
                youngest_time=101.0)
        # Oldest/youngest times cannot be distant-past or distant-future.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=pygplates.GeoTimeInstant.create_distant_past())
        # Oldest/youngest times and time increment must have integral values.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4.01)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=1.99)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=1,
                time_increment=0.99)
        # Time increment must be positive.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                time_increment=-1)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                time_increment=0)

    def test_get_reconstructed_data(self):
        multipoint =  pygplates.MultiPointOnSphere([(0,0), (0,-30), (0,-60)])
        
        # Try with default deactivate points.
        self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0, 1.0]},
                deactivate_points=pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints())
        
        # Try with our own Python derived class that delegates to the default deactivate points.
        class DelegateDeactivatePoints(pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints):
            def __init__(self):
                super(DelegateDeactivatePoints, self).__init__()
                # Delegate to the default internal algorithm but changes some of its parameters.
                self.default_deactivate_points = pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints(
                        threshold_velocity_delta=0.9,
                        threshold_distance_to_boundary= 15,
                        deactivate_points_that_fall_outside_a_network=True)
                self.last_deactivate_args = None
            def deactivate(self, prev_point, prev_location, prev_time, current_point, current_location, current_time):
                self.last_deactivate_args = prev_point, prev_location, prev_time, current_point, current_location, current_time
                return self.default_deactivate_points.deactivate(prev_point, prev_location, prev_time, current_point, current_location, current_time)
        delegate_deactivate_points = DelegateDeactivatePoints()
        self.assertFalse(delegate_deactivate_points.last_deactivate_args)
        self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0, 1.0]},
                deactivate_points=delegate_deactivate_points)
        self.assertTrue(delegate_deactivate_points.last_deactivate_args)
        # Test the arguments last passed to DelegateDeactivatePoints.deactivate() have the type we expect.
        prev_point, prev_location, prev_time, current_point, current_location, current_time = delegate_deactivate_points.last_deactivate_args
        self.assertTrue(isinstance(prev_point, pygplates.PointOnSphere))
        self.assertTrue(isinstance(prev_location, pygplates.TopologyPointLocation))
        self.assertTrue(isinstance(prev_time, float))
        self.assertTrue(isinstance(current_point, pygplates.PointOnSphere))
        self.assertTrue(isinstance(current_location, pygplates.TopologyPointLocation))
        self.assertTrue(isinstance(current_time, float))
        
        reconstructed_multipoint_time_span = self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0, 1.0]})
        
        # Points.
        reconstructed_points = reconstructed_multipoint_time_span.get_geometry_points(20)
        self.assertTrue(len(reconstructed_points) == 3)
        # Reconstructed points same as initial points (since topologies are currently static and initial points not assigned a plate ID).
        self.assertTrue(reconstructed_points == list(multipoint))
        reconstructed_points = reconstructed_multipoint_time_span.get_geometry_points(20, return_inactive_points=True)
        self.assertTrue(len(reconstructed_points) == 3)
        
        # Topology point locations.
        topology_point_locations = reconstructed_multipoint_time_span.get_topology_point_locations(20)
        self.assertTrue(len(topology_point_locations) == 3)
        self.assertTrue(topology_point_locations[0].not_located_in_resolved_topology())
        self.assertTrue(topology_point_locations[1].located_in_resolved_boundary())
        self.assertTrue(topology_point_locations[2].located_in_resolved_network())
        self.assertTrue(topology_point_locations[2].located_in_resolved_network_deforming_region())
        self.assertFalse(topology_point_locations[2].located_in_resolved_network_rigid_block())
        topology_point_locations = reconstructed_multipoint_time_span.get_topology_point_locations(20, return_inactive_points=True)
        self.assertTrue(len(topology_point_locations) == 3)
        
        # Scalars.
        scalars_dict = reconstructed_multipoint_time_span.get_scalar_values(20)
        # Should be at least the 2 scalar types we supplied initial values for.
        # There will be more since other *evolved* scalar types are reconstructed (such as crustal thinning factor) even if we did not provide initial values.
        self.assertTrue(len(scalars_dict) >= 2)
        self.assertTrue(scalars_dict[pygplates.ScalarType.gpml_crustal_thickness] == [10.0, 10.0, 10.0])
        self.assertTrue(scalars_dict[pygplates.ScalarType.gpml_crustal_stretching_factor] == [1.0, 1.0, 1.0])
        self.assertTrue(reconstructed_multipoint_time_span.get_scalar_values(20, pygplates.ScalarType.gpml_crustal_thickness) == [10.0, 10.0, 10.0])
        self.assertTrue(reconstructed_multipoint_time_span.get_scalar_values(20, pygplates.ScalarType.gpml_crustal_stretching_factor) == [1.0, 1.0, 1.0])
        scalars_dict = reconstructed_multipoint_time_span.get_scalar_values(20, return_inactive_points=True)
        self.assertTrue(len(scalars_dict) >= 2)


class TopologicalSnapshotCase(unittest.TestCase):
    def test(self):
        #
        # Class pygplates.TopologicalSnapshot is used internally by pygplates.resolved_topologies()
        # so most of its testing is already done by testing pygplates.resolved_topologies().
        #
        # Here we're just making sure we can access the pygplates.TopologicalSnapshot methods.
        #
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        
        self.assertTrue(snapshot.get_anchor_plate_id() == 0)
        self.assertTrue(snapshot.get_rotation_model())
        
        resolved_topologies = snapshot.get_resolved_topologies()
        self.assertTrue(len(resolved_topologies) == 7)  # See ResolvedTopologiesTestCase
        
        snapshot.export_resolved_topologies(os.path.join(FIXTURES, 'resolved_topologies.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topologies.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topologies.gmt'))
        
        resolved_topological_sections = snapshot.get_resolved_topological_sections()
        self.assertTrue(len(resolved_topological_sections) == 17)  # See ResolvedTopologiesTestCase
        
        snapshot.export_resolved_topological_sections(os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topological_sections.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))

        # Make sure can pass in optional ResolveTopologyParameters.
        rotations = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        topologies = list(pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml')))
        snapshot = pygplates.TopologicalSnapshot(
            topologies,
            rotations,
            pygplates.GeoTimeInstant(10),
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        # Make sure can specify ResolveTopologyParameters with the topological features.
        snapshot = pygplates.TopologicalSnapshot(
            (topologies, pygplates.ResolveTopologyParameters()),
            rotations,
            pygplates.GeoTimeInstant(10),
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        snapshot = pygplates.TopologicalSnapshot(
            [
                (topologies[0], pygplates.ResolveTopologyParameters()),  # single feature with ResolveTopologyParameters
                topologies[1],  # single feature without ResolveTopologyParameters
                (topologies[2:4], pygplates.ResolveTopologyParameters()),  # multiple features with ResolveTopologyParameters
                topologies[4:],  # multiple features without ResolveTopologyParameters
            ],
            rotations,
            pygplates.GeoTimeInstant(10))


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
            TopologicalModelCase,
            TopologicalSnapshotCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
