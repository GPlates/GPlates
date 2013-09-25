"""
Unit tests for the pygplates application logic API.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class InterpolateTotalReconstructionPolesTest(unittest.TestCase):
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
        self.assertEquals(trp[0], 901) # Fixed plate id
        self.assertEquals(trp[1], 2)   # Moving plate id
        angle = trp[2].get_unit_quaternion().get_rotation_parameters().angle
        self.assertTrue(abs(angle) > 0.1785 and abs(angle) < 0.179)
        # TODO: Compare axis.


class ReconstructionTreeCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))

    def test_create(self):
        reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], 10.0)
        self.assertTrue(isinstance(reconstruction_tree, pygplates.ReconstructionTree))

    def test_get_equivalent_total_rotation(self):
        reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], 10.0)
        finite_rotation = reconstruction_tree.get_equivalent_total_rotation(801)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            InterpolateTotalReconstructionPolesTest,
            ReconstructionTreeCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
