"""
Unit tests for the pygplates geometries on sphere.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GeometryOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.point = pygplates.PointOnSphere(1, 0, 0)
    
    def test_clone(self):
        self.assertEquals(self.point.clone(), self.point)


class PointOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.unit_vector_3d = pygplates.UnitVector3D(1, 0, 0)
    
    def test_construct(self):
        point1 = pygplates.PointOnSphere(self.unit_vector_3d)
        point2 = pygplates.PointOnSphere(
                self.unit_vector_3d.get_x(), self.unit_vector_3d.get_y(), self.unit_vector_3d.get_z())
        self.assertEquals(point1, point2)
        self.assertEquals(point1.get_position_vector(), self.unit_vector_3d)
        self.assertEquals(point2.get_position_vector(), self.unit_vector_3d)

def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GeometryOnSphereCase,
            PointOnSphereCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
