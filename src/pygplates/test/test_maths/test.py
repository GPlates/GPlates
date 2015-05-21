"""
Unit tests for the pygplates maths API.
"""

import math
import os
import unittest
import pygplates
# Test using numpy if it's available...
try:
    import numpy
    imported_numpy = True
except ImportError:
    imported_numpy = False

import test_geometries_on_sphere

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class DateLineWrapperCase(unittest.TestCase):
    def test_wrap(self):
        date_line_wrapper = pygplates.DateLineWrapper()
        date_line_wrapper_90 = pygplates.DateLineWrapper(90)
        
        # Wrap point.
        point = pygplates.PointOnSphere(0, -120)
        wrapped_point = date_line_wrapper.wrap(point)
        self.assertAlmostEqual(wrapped_point.get_latitude(), 0)
        self.assertAlmostEqual(wrapped_point.get_longitude(), -120)
        # Longitude should now be in range [-90, 270].
        wrapped_point = date_line_wrapper_90.wrap(point)
        self.assertAlmostEqual(wrapped_point.get_longitude(), -120 + 360)
        
        # Wrap mult-point.
        multi_point = pygplates.MultiPointOnSphere([(10, 170), (0, -170), (-10, 170)])
        wrapped_multi_point = date_line_wrapper.wrap(multi_point)
        self.assertEquals(len(wrapped_multi_point.get_points()), 3)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[0].get_latitude(), 10)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[0].get_longitude(), 170)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[1].get_latitude(), 0)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[1].get_longitude(), -170)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[2].get_latitude(), -10)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[2].get_longitude(), 170)
        # Longitude should now be in range [-90, 270].
        wrapped_multi_point = date_line_wrapper_90.wrap(multi_point)
        self.assertTrue(isinstance(wrapped_multi_point, pygplates.DateLineWrapper.LatLonMultiPoint))
        self.assertAlmostEqual(wrapped_multi_point.get_points()[0].get_longitude(), 170)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[1].get_longitude(), -170 + 360)
        self.assertAlmostEqual(wrapped_multi_point.get_points()[2].get_longitude(), 170)

        # Polygon should get split into two across dateline (but not when central meridian is 90).
        polygon = pygplates.PolygonOnSphere([(10, 170), (0, -170), (-10, 170)])
        wrapped_polygon = date_line_wrapper.wrap(polygon)
        self.assertEquals(len(wrapped_polygon), 2)
        self.assertTrue(isinstance(wrapped_polygon[0].get_exterior_points()[0], pygplates.LatLonPoint))
        wrapped_polygon = date_line_wrapper_90.wrap(polygon)
        self.assertEquals(len(wrapped_polygon), 1)
        self.assertTrue(isinstance(wrapped_polygon[0], pygplates.DateLineWrapper.LatLonPolygon))

        # Polyline should get split into three across dateline (but not when central meridian is 90).
        polyline = pygplates.PolylineOnSphere([(10, 170), (0, -170), (-10, 170)])
        wrapped_polyline = date_line_wrapper.wrap(polyline)
        self.assertEquals(len(wrapped_polyline), 3)
        self.assertTrue(isinstance(wrapped_polyline[0].get_points()[0], pygplates.LatLonPoint))
        wrapped_polyline = date_line_wrapper_90.wrap(polyline)
        self.assertEquals(len(wrapped_polyline), 1)
        self.assertTrue(isinstance(wrapped_polyline[0], pygplates.DateLineWrapper.LatLonPolyline))

class FiniteRotationCase(unittest.TestCase):
    def setUp(self):
        self.pole = pygplates.PointOnSphere(0, 0, 1)
        self.angle = 0.5 * math.pi
        self.finite_rotation = pygplates.FiniteRotation(self.pole, self.angle)
    
    def test_construct_from_pole_and_angle(self):
        # Can construct from PointOnSphere pole.
        finite_rotation = pygplates.FiniteRotation(self.pole, self.angle)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from (x,y,z) tuple pole.
        xyz_tuple = self.pole.to_xyz()
        finite_rotation = pygplates.FiniteRotation(xyz_tuple, self.angle)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from (x,y,z) list pole.
        finite_rotation = pygplates.FiniteRotation(list(xyz_tuple), self.angle)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from (x,y,z) numpy array pole.
        if imported_numpy:
            finite_rotation = pygplates.FiniteRotation(numpy.array(xyz_tuple), self.angle)
            self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from LatLonPoint pole.
        finite_rotation = pygplates.FiniteRotation(self.pole.to_lat_lon_point(), self.angle)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from (lat,lon) tuple pole.
        lat_lon_tuple = self.pole.to_lat_lon()
        finite_rotation = pygplates.FiniteRotation(lat_lon_tuple, self.angle)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from (lat,lon) list pole.
        finite_rotation = pygplates.FiniteRotation(list(lat_lon_tuple), self.angle)
        self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
        # Can construct from (lat,lon) numpy array pole.
        if imported_numpy:
            finite_rotation = pygplates.FiniteRotation(numpy.array(lat_lon_tuple), self.angle)
            self.assertTrue(isinstance(finite_rotation, pygplates.FiniteRotation))
    
    def test_construct_rotation_between_two_points(self):
        self.assertTrue(pygplates.FiniteRotation((10,10), (55,132)) * pygplates.PointOnSphere(10,10)
                == pygplates.PointOnSphere(55,132))
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation((10,10), (10,10))))
        self.assertAlmostEqual(pygplates.FiniteRotation((10,10), (-10,-170))
                .get_lat_lon_euler_pole_and_angle_degrees()[2], 180, 4)
        lat, lon, angle = pygplates.FiniteRotation((10,10), (-30,10)).get_lat_lon_euler_pole_and_angle_degrees()
        self.assertAlmostEqual(lat, 0)
        self.assertAlmostEqual(lon, 100)
        self.assertAlmostEqual(angle, 40)
    
    # Attempt to rotate each supported geometry type - an error will be raised if not supported...
    
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
        pole_lat, pole_long, angle_degrees = self.finite_rotation.get_lat_lon_euler_pole_and_angle_degrees()
        self.assertTrue(abs(math.radians(angle_degrees)) > 0.5 * math.pi - 0.00001 and
            abs(math.radians(angle_degrees)) < 0.5 * math.pi + 0.00001)
    
    def test_get_rotation_distance(self):
        self.assertAlmostEqual(self.finite_rotation.get_rotation_distance((90,0)), 0)
        self.assertAlmostEqual(self.finite_rotation.get_rotation_distance((-90,0)), 0)
        # Rotating along great circle arc.
        self.assertAlmostEqual(self.finite_rotation.get_rotation_distance((0,0)), 0.5 * math.pi)
        # Rotating along small circle arc.
        self.assertAlmostEqual(self.finite_rotation.get_rotation_distance((45,0)), math.sin(math.radians(45)) * 0.5 * math.pi)
        self.assertAlmostEqual(self.finite_rotation.get_rotation_distance((-45,0)), math.sin(math.radians(45)) * 0.5 * math.pi)
    
    def test_identity(self):
        # Create identity rotation explicitly.
        identity_finite_rotation = pygplates.FiniteRotation.create_identity_rotation()
        self.assertTrue(identity_finite_rotation.represents_identity_rotation())
        identity_finite_rotation2 = pygplates.FiniteRotation()
        self.assertTrue(identity_finite_rotation2.represents_identity_rotation())
        self.assertRaises(pygplates.IndeterminateResultError, identity_finite_rotation.get_euler_pole_and_angle, False)
        self.assertRaises(pygplates.IndeterminateResultError, identity_finite_rotation.get_lat_lon_euler_pole_and_angle_degrees, False)
        self.assertTrue(identity_finite_rotation.get_euler_pole_and_angle() == (pygplates.PointOnSphere(90,0), 0))
        self.assertTrue(identity_finite_rotation.get_lat_lon_euler_pole_and_angle_degrees() == (90,0,0))
        # Create identity rotation using zero angle.
        identity_finite_rotation = pygplates.FiniteRotation(self.pole, 0)
        self.assertTrue(identity_finite_rotation.represents_identity_rotation())
        self.assertRaises(pygplates.IndeterminateResultError, identity_finite_rotation.get_euler_pole_and_angle, False)
        self.assertRaises(pygplates.IndeterminateResultError, identity_finite_rotation.get_lat_lon_euler_pole_and_angle_degrees, False)
    
    def test_equality(self):
        # Reversing pole and angle creates the same exact rotation.
        reverse_pole = pygplates.PointOnSphere(-self.pole.get_x(), -self.pole.get_y(), -self.pole.get_z())
        reverse_angle = -self.angle
        finite_rotation = pygplates.FiniteRotation(reverse_pole, reverse_angle)
        self.assertTrue(self.finite_rotation == finite_rotation)
        self.assertTrue(pygplates.FiniteRotation.are_equal(self.finite_rotation, finite_rotation))
        self.assertTrue(pygplates.FiniteRotation.are_equal(
                pygplates.FiniteRotation((10,20), math.radians(30)),
                pygplates.FiniteRotation((10.5,20.4), math.radians(29.6)),
                0.6))
        self.assertFalse(pygplates.FiniteRotation.are_equal(
                pygplates.FiniteRotation((10,20), math.radians(30)),
                pygplates.FiniteRotation((10.5,20.4), math.radians(29.6)),
                0.45))
        self.assertFalse(pygplates.FiniteRotation.are_equal(
                pygplates.FiniteRotation((10,20), math.radians(30)),
                pygplates.FiniteRotation((10.5,20.4), math.radians(29.6))))
    
    def test_equivalent(self):
        pole = pygplates.PointOnSphere(-self.pole.get_x(), -self.pole.get_y(), -self.pole.get_z())
        angle = -self.angle
        finite_rotation = pygplates.FiniteRotation(pole, angle)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(self.finite_rotation, finite_rotation))
        self.assertTrue(self.finite_rotation == finite_rotation)
        
        pole = pygplates.PointOnSphere(-self.pole.get_x(), -self.pole.get_y(), -self.pole.get_z())
        angle = math.radians(360 - math.degrees(self.angle))
        finite_rotation = pygplates.FiniteRotation(pole, angle)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(self.finite_rotation, finite_rotation))
        self.assertTrue(self.finite_rotation != finite_rotation)
        
        pole = self.pole
        angle = math.radians(math.degrees(self.angle) - 360)
        finite_rotation = pygplates.FiniteRotation(pole, angle)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(self.finite_rotation, finite_rotation))
        self.assertTrue(self.finite_rotation != finite_rotation)
    
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
        composed_rotation = pygplates.FiniteRotation.compose(inverse_rotation, self.finite_rotation)
        self.assertTrue(isinstance(composed_rotation, pygplates.FiniteRotation))
    
    def test_interpolate(self):
        finite_rotation2 = pygplates.FiniteRotation(pygplates.PointOnSphere(0, 1, 0), 0.25 * math.pi)
        interpolated_rotation = pygplates.FiniteRotation.interpolate(
                self.finite_rotation, finite_rotation2,
                10, pygplates.GeoTimeInstant(20), 15)
        self.assertTrue(isinstance(interpolated_rotation, pygplates.FiniteRotation))
        # Cannot use distant past/future for any of the time values.
        self.assertRaises(pygplates.InterpolationError,
                pygplates.FiniteRotation.interpolate,
                self.finite_rotation, finite_rotation2,
                pygplates.GeoTimeInstant.create_distant_past(), 20, 15)
        self.assertRaises(pygplates.InterpolationError,
                pygplates.FiniteRotation.interpolate,
                self.finite_rotation, finite_rotation2,
                10, pygplates.GeoTimeInstant.create_distant_future(), 15)
        self.assertRaises(pygplates.InterpolationError,
                pygplates.FiniteRotation.interpolate,
                self.finite_rotation, finite_rotation2,
                10, 20, pygplates.GeoTimeInstant.create_distant_past())


class GreatCircleArcCase(unittest.TestCase):
    def setUp(self):
        self.start_point = pygplates.PointOnSphere(0, 1, 0)
        self.end_point = pygplates.PointOnSphere(0, 0, 1)
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
        self.assertTrue(self.gca.get_rotation_axis() == (1, 0, 0))
        self.assertTrue(self.gca.get_rotation_axis_lat_lon() == (0, 0))
    
    def test_zero_length(self):
        self.assertFalse(self.gca.is_zero_length())
        zero_length_gca = pygplates.GreatCircleArc(self.start_point, self.start_point)
        self.assertTrue(zero_length_gca.is_zero_length())
        self.assertRaises(pygplates.IndeterminateArcRotationAxisError, zero_length_gca.get_rotation_axis)
        self.assertRaises(pygplates.IndeterminateArcRotationAxisError, zero_length_gca.get_rotation_axis_lat_lon)


class LatLonPointCase(unittest.TestCase):
    def setUp(self):
        self.latitude = 50
        self.longitude = 145
        self.lat_lon_point = pygplates.LatLonPoint(self.latitude, self.longitude)
    
    def test_static_data_members(self):
        self.assertAlmostEqual(pygplates.LatLonPoint.north_pole.get_latitude(), 90)
        self.assertAlmostEqual(pygplates.LatLonPoint.south_pole.get_latitude(), -90)

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
    
    def test_to_lat_lon(self):
        lat_lon_point = pygplates.LatLonPoint(0, 90)
        self.assertAlmostEqual(lat_lon_point.get_latitude(), 0)
        self.assertAlmostEqual(lat_lon_point.get_longitude(), 90)
        lat, lon = lat_lon_point.to_lat_lon()
        self.assertAlmostEqual(lat, 0)
        self.assertAlmostEqual(lon, 90)
    
    def test_to_xyz(self):
        lat_lon_point = pygplates.LatLonPoint(0, 90)
        point = lat_lon_point.to_point_on_sphere()
        self.assertAlmostEqual(point.get_x(), 0)
        self.assertAlmostEqual(point.get_y(), 1)
        self.assertAlmostEqual(point.get_z(), 0)
        x, y, z = lat_lon_point.to_xyz()
        self.assertAlmostEqual(x, 0)
        self.assertAlmostEqual(y, 1)
        self.assertAlmostEqual(z, 0)


class Vector3DCase(unittest.TestCase):
    def setUp(self):
        self.xyz = (1.03, -2.232, 34.232)
        self.vector = pygplates.Vector3D(self.xyz)
    
    def test_compare(self):
        self.assertTrue(self.vector == pygplates.Vector3D(self.xyz))
        x, y, z = self.xyz
        self.assertTrue(self.vector == pygplates.Vector3D(x, y, z))

    def test_get(self):
        self.assertTrue(self.vector.get_x() == self.xyz[0])
        self.assertTrue(self.vector.get_y() == self.xyz[1])
        self.assertTrue(self.vector.get_z() == self.xyz[2])
    
    def test_to_xyz(self):
        self.assertTrue(self.vector.to_xyz() == self.xyz)
    
    def test_dot_product(self):
        self.assertAlmostEqual(pygplates.Vector3D.dot((1,1,1), (2,2,2)), 6)
        self.assertAlmostEqual(pygplates.Vector3D.dot(pygplates.Vector3D(1,0,0), (0,1,0)), 0)
        self.assertAlmostEqual(pygplates.Vector3D.dot([1,-1,0.5], pygplates.Vector3D(0,1,-2)), -2)
    
    def test_cross_product(self):
        self.assertTrue(pygplates.Vector3D.cross(pygplates.Vector3D(2,0,0), (1,0,0)) == pygplates.Vector3D(0,0,0))
        self.assertTrue(pygplates.Vector3D.cross(pygplates.Vector3D(2,0,0), (0,1,0)) == pygplates.Vector3D(0,0,2))
        self.assertTrue(pygplates.Vector3D.cross(pygplates.Vector3D(0,1,0), (2,0,0)) == pygplates.Vector3D(0,0,-2))

def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            DateLineWrapperCase,
            FiniteRotationCase,
            GreatCircleArcCase,
            LatLonPointCase,
            Vector3DCase
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
