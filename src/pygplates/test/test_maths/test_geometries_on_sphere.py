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


class MultiPointOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(-1, 0, 0))
        self.multi_point = pygplates.MultiPointOnSphere.create(self.points)
    
    def test_compare(self):
        self.assertEquals(self.multi_point, pygplates.MultiPointOnSphere.create(self.points))
    
    def test_iter(self):
        points = [point for point in self.multi_point]
        self.assertEquals(self.points, points)
    
    def test_contains(self):
        self.assertTrue(self.points[0] in self.multi_point)
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.multi_point)
        self.assertTrue(pygplates.PointOnSphere(0, -1, 0) not in self.multi_point)

    def test_get_item(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.multi_point[i] == self.points[i])
        self.assertTrue(self.multi_point[-1] == self.points[-1])
        for i, point in enumerate(self.multi_point):
            self.assertTrue(point == self.multi_point[i])
        def get_point1():
            self.multi_point[len(self.multi_point)]
        self.assertRaises(IndexError, get_point1)

    def test_get_slice(self):
        slice = self.multi_point[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_extended_slice(self):
        slice = self.multi_point[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])


class PolylineOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(-1, 0, 0))
        self.polyline = pygplates.PolylineOnSphere.create(self.points)
    
    def test_compare(self):
        self.assertEquals(self.polyline, pygplates.PolylineOnSphere.create(self.points))
    
    def test_iter(self):
        points = [point for point in self.polyline.get_points_view()]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polyline.get_points_view()))
    
    def test_contains(self):
        self.assertTrue(self.points[0] in self.polyline.get_points_view())
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polyline.get_points_view())
        self.assertTrue(pygplates.PointOnSphere(0, -1, 0) not in self.polyline.get_points_view())

    def test_get_item(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polyline.get_points_view()[i] == self.points[i])
        self.assertTrue(self.polyline.get_points_view()[-1] == self.points[-1])
        for i, point in enumerate(self.polyline.get_points_view()):
            self.assertTrue(point == self.polyline.get_points_view()[i])
        def get_point1():
            self.polyline.get_points_view()[len(self.polyline.get_points_view())]
        self.assertRaises(IndexError, get_point1)

    def test_get_slice(self):
        slice = self.polyline.get_points_view()[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_extended_slice(self):
        slice = self.polyline.get_points_view()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GeometryOnSphereCase,
            MultiPointOnSphereCase,
            PointOnSphereCase,
            PolylineOnSphereCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
