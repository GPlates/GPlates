"""
Unit tests for the pygplates application logic API.
"""

import math
import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class InterpolateTotalReconstructionSequenceTest(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))

    def test_get(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        feature_iter.next();
        feature_iter.next();
        feature = feature_iter.next()
        
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
        feature_iter.next();
        feature_iter.next();
        feature = feature_iter.next()
        
        total_reconstruction_pole = feature.get_total_reconstruction_pole()
        self.assertTrue(total_reconstruction_pole)
        fixed_plate_id, moving_plate_id, total_reconstruction_pole_rotations = total_reconstruction_pole
        interpolated_finite_rotation = total_reconstruction_pole_rotations.get_value(12.2).get_finite_rotation()
        self.assertEquals(fixed_plate_id, 901)
        self.assertEquals(moving_plate_id, 2)
        pole, angle = interpolated_finite_rotation.get_euler_pole_and_angle()
        self.assertTrue(abs(angle) > 0.1785 and abs(angle) < 0.179)
        # TODO: Compare pole.


class ReconstructTest(unittest.TestCase):
    def test_reconstruct(self):
        pygplates.reconstruct(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            'test.xy',
            pygplates.GeoTimeInstant(10))
        
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
        
    def test_deprecated_reconstruct(self):
        # We continue to support the deprecated version of 'reconstruct()' since
        # it was one of the few python API functions that's been around since
        # the dawn of time and is currently used in some web applications.
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml')],
            [os.path.join(FIXTURES, 'rotations.rot')],
            10,
            0,
            'test.xy')

    def test_reverse_reconstruct(self):
        # Test modifying the feature collection file.
        pygplates.reverse_reconstruct(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            [os.path.join(FIXTURES, 'rotations.rot')],
            pygplates.GeoTimeInstant(10))
        
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
        pygplates.reverse_reconstruct([
                os.path.join(FIXTURES, 'volcanoes.gpml'),
                reconstructable_feature_collection,
                [feature for feature in reconstructable_feature_collection],
                next(iter(reconstructable_feature_collection))],
            rotation_features,
            10,
            0)
        
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
        self.assertTrue(pygplates.represent_equivalent_rotations(
                self.reconstruction_tree.get_equivalent_total_rotation(802),
                self.reconstruction_tree.get_relative_total_rotation(802, 0)))
        self.assertTrue(pygplates.represent_equivalent_rotations(
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
        
        equivalent_stage_rotation = pygplates.get_equivalent_stage_rotation(from_reconstruction_tree, self.reconstruction_tree, 802)
        self.assertTrue(isinstance(equivalent_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.get_equivalent_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.get_equivalent_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 10000, use_identity_for_missing_plate_ids=False))
        # Should raise error for differing anchor plate ids (0 versus 291).
        self.assertRaises(
                pygplates.DifferentAnchoredPlatesInReconstructionTreesError,
                pygplates.get_equivalent_stage_rotation,
                pygplates.ReconstructionTree([self.rotations], 11, 291),
                self.reconstruction_tree,
                802)
        
        relative_stage_rotation = pygplates.get_relative_stage_rotation(from_reconstruction_tree, self.reconstruction_tree, 802, 291)
        self.assertTrue(isinstance(relative_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.get_relative_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802, 10000, use_identity_for_missing_plate_ids=False))
        # Should return None for an unknown relative plate id.
        self.assertFalse(pygplates.get_relative_stage_rotation(
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
        # Create using feature collections instead of filenames.
        rotation_model = pygplates.RotationModel([self.rotations])
        # Create using a single feature collection.
        rotation_model = pygplates.RotationModel(self.rotations)
        # Create using a list of features.
        rotation_model = pygplates.RotationModel([rotation for rotation in self.rotations])
        # Create using a single feature.
        rotation_model = pygplates.RotationModel(next(iter(self.rotations)))
        # Create using a mixture of the above.
        rotation_model = pygplates.RotationModel([
                os.path.join(FIXTURES, 'rotations.rot'),
                self.rotations,
                [rotation for rotation in self.rotations],
                next(iter(self.rotations))])
    
    def test_get_reconstruction_tree(self):
        to_reconstruction_tree = self.rotation_model.get_reconstruction_tree(self.to_time)
        self.assertTrue(isinstance(to_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertTrue(to_reconstruction_tree.get_reconstruction_time() > self.to_time.get_value() - 1e-6 and
                to_reconstruction_tree.get_reconstruction_time() < self.to_time.get_value() + 1e-6)
        self.assertRaises(pygplates.InterpolationError,
                pygplates.RotationModel.get_reconstruction_tree, self.rotation_model, pygplates.GeoTimeInstant.create_distant_past())
    
    def test_get_rotation(self):
        equivalent_total_rotation = self.rotation_model.get_rotation(self.to_time, 802)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                equivalent_total_rotation,
                self.to_reconstruction_tree.get_equivalent_total_rotation(802)))
        self.assertRaises(pygplates.InterpolationError,
                pygplates.RotationModel.get_rotation, self.rotation_model, pygplates.GeoTimeInstant.create_distant_past(), 802)
        
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        # Fixed plate id defaults to anchored plate id.
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=291)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        # Shouldn't really matter what the anchor plate id is (as long as there's a plate circuit
        # path from anchor plate to both fixed and moving plates.
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291, anchor_plate_id=802)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        
        equivalent_stage_rotation = self.rotation_model.get_rotation(self.to_time, 802, self.from_time)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                equivalent_stage_rotation,
                pygplates.get_equivalent_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802)))
        
        relative_stage_rotation = self.rotation_model.get_rotation(self.to_time, 802, self.from_time, 291)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                relative_stage_rotation,
                pygplates.get_relative_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802,
                        291)))
        # Fixed plate id defaults to anchored plate id.
        relative_stage_rotation = self.rotation_model.get_rotation(
                self.to_time, 802, pygplates.GeoTimeInstant(self.from_time), anchor_plate_id=291)
        self.assertTrue(pygplates.represent_equivalent_rotations(
                relative_stage_rotation,
                pygplates.get_relative_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802,
                        291)))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            InterpolateTotalReconstructionSequenceTest,
            ReconstructTest,
            ReconstructionTreeCase,
            RotationModelCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite