"""
Unit tests for the pygplates application logic API.
"""

import math
import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class InterpolateTotalReconstructionPoleTest(unittest.TestCase):
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
                                        pygplates.PointOnSphere(1,0,0),
                                        0.5)),
                        pygplates.GeoTimeInstant(10))])

    def test_interpolate(self):
        interpolated_finite_rotation = pygplates.interpolate_total_reconstruction_pole(self.gpml_irregular_sampling, 5)
        self.assertTrue(interpolated_finite_rotation)
        self.assertTrue(isinstance(interpolated_finite_rotation, pygplates.FiniteRotation))
        # Time outside range of time samples.
        self.assertFalse(pygplates.interpolate_total_reconstruction_pole(self.gpml_irregular_sampling, 15))


class InterpolateTotalReconstructionSequenceTest(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))

    def test_interpolate(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        feature_iter.next();
        feature_iter.next();
        feature = feature_iter.next()
        
        trp = pygplates.interpolate_total_reconstruction_sequence(feature, 12.2)
        self.assertTrue(trp)
        fixed_plate_id, moving_plate_id, interpolated_finite_rotation = trp
        self.assertEquals(fixed_plate_id, 901)
        self.assertEquals(moving_plate_id, 2)
        pole, angle = interpolated_finite_rotation.get_euler_pole_and_angle()
        self.assertTrue(abs(angle) > 0.1785 and abs(angle) < 0.179)
        # TODO: Compare pole.


class ReconstructionTreeCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        self.reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], 10.0)

    def test_create(self):
        self.assertTrue(isinstance(self.reconstruction_tree, pygplates.ReconstructionTree))
        self.assertEqual(self.reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(self.reconstruction_tree.get_reconstruction_time() > 9.9999 and
                self.reconstruction_tree.get_reconstruction_time() < 10.00001)

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
                self.reconstruction_tree.get_relative_total_rotation(291, 802),
                pygplates.FiniteRotation))
        self.assertTrue(pygplates.represent_equivalent_rotations(
                self.reconstruction_tree.get_equivalent_total_rotation(802),
                self.reconstruction_tree.get_relative_total_rotation(0, 802)))
        self.assertTrue(pygplates.represent_equivalent_rotations(
                self.reconstruction_tree.get_edge(802).get_relative_total_rotation(),
                self.reconstruction_tree.get_relative_total_rotation(
                        self.reconstruction_tree.get_edge(802).get_fixed_plate_id(),
                        802)))
        # Should return identity rotation.
        self.assertTrue(self.reconstruction_tree.get_equivalent_total_rotation(
                self.reconstruction_tree.get_anchor_plate_id()).represents_identity_rotation())
        self.assertTrue(self.reconstruction_tree.get_relative_total_rotation(
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id()).represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(self.reconstruction_tree.get_equivalent_total_rotation(10000))
        # Should return None for an unknown relative plate id.
        self.assertFalse(self.reconstruction_tree.get_relative_total_rotation(10000, 802))
    
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
        self.assertFalse(pygplates.get_equivalent_stage_rotation(from_reconstruction_tree, self.reconstruction_tree, 10000))
        # Should raise error for differing anchor plate ids (0 versus 291).
        self.assertRaises(
                pygplates.DifferentAnchoredPlatesInReconstructionTreesError,
                pygplates.get_equivalent_stage_rotation,
                pygplates.ReconstructionTree([self.rotations], 11, 291),
                self.reconstruction_tree,
                802)
        
        relative_stage_rotation = pygplates.get_relative_stage_rotation(from_reconstruction_tree, self.reconstruction_tree, 291, 802)
        self.assertTrue(isinstance(relative_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.get_relative_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.get_relative_stage_rotation(from_reconstruction_tree, self.reconstruction_tree, 10000, 802))
        # Should return None for an unknown relative plate id.
        self.assertFalse(pygplates.get_relative_stage_rotation(from_reconstruction_tree, self.reconstruction_tree, 291, 10000))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            InterpolateTotalReconstructionPoleTest,
            InterpolateTotalReconstructionSequenceTest,
            ReconstructionTreeCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
