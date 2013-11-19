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
        self.angle = 0.5 * math.pi
        self.finite_rotation = pygplates.FiniteRotation(self.pole, self.angle)
    
    # Attempt to rotate each supported geometry type - an error will be raised if not supported...
    
    def test_rotate_unit_vector_3d(self):
        rotated_unit_vector_3d = self.finite_rotation * pygplates.UnitVector3D(1, 0, 0)
        self.assertTrue(isinstance(rotated_unit_vector_3d, pygplates.UnitVector3D))
    
    def test_rotate_great_circle_arc(self):
        rotated_great_circle_arc = self.finite_rotation * pygplates.GreatCircleArc(
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
    
    def test_get_pole_and_angle(self):
        pole, angle = self.finite_rotation.get_euler_pole_and_angle()
        self.assertTrue(isinstance(pole, pygplates.PointOnSphere))
        self.assertTrue(abs(angle) > 0.5 * math.pi - 0.00001 and abs(angle) < 0.5 * math.pi + 0.00001)
    
    def test_identity(self):
        # Create identity rotation explicitly.
        identity_finite_rotation = pygplates.FiniteRotation.create_identity_rotation()
        self.assertTrue(identity_finite_rotation.represents_identity_rotation())
        self.assertRaises(pygplates.IndeterminateResultError, identity_finite_rotation.get_euler_pole_and_angle)
        # Create identity rotation using zero angle.
        identity_finite_rotation = pygplates.FiniteRotation(self.pole, 0)
        self.assertTrue(identity_finite_rotation.represents_identity_rotation())
        self.assertRaises(pygplates.IndeterminateResultError, identity_finite_rotation.get_euler_pole_and_angle)
    
    def test_equivalent(self):
        reverse_pole = pygplates.PointOnSphere(-self.pole.get_x(), -self.pole.get_y(), -self.pole.get_z())
        reverse_angle = -self.angle
        finite_rotation = pygplates.FiniteRotation(reverse_pole, reverse_angle)
        self.assertTrue(pygplates.represent_equivalent_rotations(self.finite_rotation, finite_rotation))
    
    def test_inverse(self):
        inverse_rotation = self.finite_rotation.get_inverse()
        self.assertTrue(isinstance(inverse_rotation, pygplates.FiniteRotation))
        self.assertTrue((self.finite_rotation * inverse_rotation).represents_identity_rotation())
        self.assertTrue((inverse_rotation * self.finite_rotation).represents_identity_rotation())
    
    def test_compose_rotations(self):
        inverse_rotation = self.finite_rotation.get_inverse()
        # Compose two finite rotations.
        composed_rotation = inverse_rotation * self.finite_rotation
        self.assertTrue(isinstance(composed_rotation, pygplates.FiniteRotation))
        # Another way to compose.
        composed_rotation = pygplates.compose_finite_rotations(inverse_rotation, self.finite_rotation)
        self.assertTrue(isinstance(composed_rotation, pygplates.FiniteRotation))
    
    def test_interpolate(self):
        finite_rotation2 = pygplates.FiniteRotation(pygplates.PointOnSphere(0, 1, 0), 0.25 * math.pi)
        interpolated_rotation = pygplates.interpolate_finite_rotations(
                self.finite_rotation, finite_rotation2,
                10, 20, 15)
        self.assertTrue(isinstance(interpolated_rotation, pygplates.FiniteRotation))


class GreatCircleArcCase(unittest.TestCase):
    def setUp(self):
        self.start_point = pygplates.PointOnSphere(0, 0, 1)
        self.end_point = pygplates.PointOnSphere(0, 1, 0)
        self.gca = pygplates.GreatCircleArc(self.start_point, self.end_point)
    
    def test_compare(self):
        gca = pygplates.GreatCircleArc(self.start_point, self.end_point)
        self.assertTrue(self.gca == gca)
        reverse_gca = pygplates.GreatCircleArc(self.end_point, self.start_point)
        self.assertTrue(self.gca != reverse_gca)
    
    def test_get(self):
        self.assertEquals(self.gca.get_start_point(), self.start_point)
        self.assertEquals(self.gca.get_end_point(), self.end_point)
        arc_length = self.gca.get_arc_length()
        self.assertTrue(arc_length > 0.5 * math.pi - 1e-6 and arc_length < 0.5 * math.pi + 1e-6)
    
    def test_zero_length(self):
        self.assertFalse(self.gca.is_zero_length())
        self.assertFalse(self.gca.get_rotation_axis() is None)
        zero_length_gca = pygplates.GreatCircleArc(self.start_point, self.start_point)
        self.assertTrue(zero_length_gca.is_zero_length())
        self.assertRaises(pygplates.IndeterminateArcRotationAxisError, zero_length_gca.get_rotation_axis)


class LatLonPointCase(unittest.TestCase):
    def setUp(self):
        self.latitude = 50
        self.longitude = 145
        self.lat_lon_point = pygplates.LatLonPoint(self.latitude, self.longitude)

    def test_get(self):
        self.assertTrue(self.lat_lon_point.get_latitude() == self.latitude)
        self.assertTrue(self.lat_lon_point.get_longitude() == self.longitude)

    def test_is_valid(self):
        self.assertTrue(pygplates.LatLonPoint.is_valid_latitude(self.lat_lon_point.get_latitude()))
        self.assertTrue(pygplates.LatLonPoint.is_valid_longitude(self.lat_lon_point.get_longitude()))

    def test_is_invalid(self):
        # Latitude outside range [-90,90].
        self.assertFalse(pygplates.LatLonPoint.is_valid_latitude(100))
        self.assertRaises(pygplates.InvalidLatLonError, pygplates.LatLonPoint, 100, 0)
        self.assertRaises(pygplates.InvalidLatLonError, pygplates.LatLonPoint, -100, 0)
        # Longitude outside range [-360,360].
        self.assertFalse(pygplates.LatLonPoint.is_valid_longitude(370))
        self.assertRaises(pygplates.InvalidLatLonError, pygplates.LatLonPoint, 0, 370)
        self.assertRaises(pygplates.InvalidLatLonError, pygplates.LatLonPoint, 0, -370)

    def test_convert(self):
        point_on_sphere = pygplates.convert_lat_lon_point_to_point_on_sphere(pygplates.LatLonPoint(10, 20))
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        lat_lon_point = pygplates.convert_point_on_sphere_to_lat_lon_point(pygplates.PointOnSphere(1, 0, 0))
        self.assertTrue(isinstance(lat_lon_point, pygplates.LatLonPoint))

def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            FiniteRotationCase,
            GreatCircleArcCase,
            LatLonPointCase
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
