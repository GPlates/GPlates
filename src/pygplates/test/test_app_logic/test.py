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

    def test_foo(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        feature_iter.next();
        feature_iter.next();
        feature = feature_iter.next()
        
        f, m, fr = pygplates.interpolate_total_reconstruction_poles(feature, pygplates.GeoTimeInstant(10))
        print "f=", f, " m=", m, " fr=", fr
        for r in self.rotations:
            for p in r:
                if p.get_name() == pygplates.PropertyName.create_gpml('movingReferenceFrame'):
                    print p.get_value().get_plate_id()
                    return


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
