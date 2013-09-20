"""
Unit tests for the pygplates maths API.
"""

import math
import os
import unittest
import pygplates

import test_geometries_on_sphere

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class FiniteRotationCase(unittest.TestCase):
    def setUp(self):
        self.pole = pygplates.PointOnSphere(0, 0, 1)
        self.angle = math.pi
        self.finite_rotation = pygplates.FiniteRotation(self.pole, self.angle)
    
    # Attempt to rotate each supported geometry type - an error will be raised if not supported...
    
    def test_rotate_unit_vector_3d(self):
        rotated_unit_vector_3d = self.finite_rotation * pygplates.UnitVector3D(1, 0, 0)
        self.assertTrue(isinstance(rotated_unit_vector_3d, pygplates.UnitVector3D))
    
    def test_rotate_great_circle_arc(self):
        rotated_great_circle_arc = self.finite_rotation * pygplates.GreatCircleArc.create(
                pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0))
        self.assertTrue(isinstance(rotated_great_circle_arc, pygplates.GreatCircleArc))
    
    def test_rotate_point_on_sphere(self):
        rotated_point = self.finite_rotation * pygplates.PointOnSphere(0, 1, 0)
        self.assertTrue(isinstance(rotated_point, pygplates.PointOnSphere))
    
    def test_rotate_multi_point_on_sphere(self):
        multi_point = pygplates.MultiPointOnSphere(
                [pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(0, 1, 0)])
        rotated_multi_point = self.finite_rotation * multi_point
        self.assertTrue(isinstance(rotated_multi_point, pygplates.MultiPointOnSphere))
        self.assertTrue(len(rotated_multi_point) == len(multi_point))
    
    def test_rotate_polyline_on_sphere(self):
        polyline = pygplates.PolylineOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        rotated_polyline = self.finite_rotation * polyline
        self.assertTrue(isinstance(rotated_polyline, pygplates.PolylineOnSphere))
        self.assertTrue(len(rotated_polyline.get_points_view()) == len(polyline.get_points_view()))
    
    def test_rotate_polygon_on_sphere(self):
        polygon = pygplates.PolygonOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        rotated_polygon = self.finite_rotation * polygon
        self.assertTrue(isinstance(rotated_polygon, pygplates.PolygonOnSphere))
        self.assertTrue(len(rotated_polygon.get_points_view()) == len(polygon.get_points_view()))


class GreatCircleArcCase(unittest.TestCase):
    def setUp(self):
        self.start_point = pygplates.PointOnSphere(0, 0, 1)
        self.end_point = pygplates.PointOnSphere(0, 1, 0)
        self.gca = pygplates.GreatCircleArc.create(self.start_point, self.end_point)
    
    def test_compare(self):
        gca = pygplates.GreatCircleArc.create(self.start_point, self.end_point)
        self.assertTrue(self.gca == gca)
        reverse_gca = pygplates.GreatCircleArc.create(self.end_point, self.start_point)
        self.assertTrue(self.gca != reverse_gca)
    
    def test_get(self):
        self.assertEquals(self.gca.get_start_point(), self.start_point)
        self.assertEquals(self.gca.get_end_point(), self.end_point)
    
    def test_zero_length(self):
        self.assertFalse(self.gca.is_zero_length())
        self.assertFalse(self.gca.get_rotation_axis() is None)
        zero_length_gca = pygplates.GreatCircleArc.create(self.start_point, self.start_point)
        self.assertTrue(zero_length_gca.is_zero_length())
        self.assertTrue(zero_length_gca.get_rotation_axis() is None)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            FiniteRotationCase,
            GreatCircleArcCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    # Add test suites from sibling modules.
    test_modules = [
            test_geometries_on_sphere
            ]

    for test_module in test_modules:
        suite.addTest(test_module.suite())
    
    return suite
