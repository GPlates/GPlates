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

import test_maths.test_geometries_on_sphere

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
        
        # Wrap multi-point.
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
        wrapped_polygons = date_line_wrapper.wrap(polygon)
        self.assertEquals(len(wrapped_polygons), 2)
        self.assertTrue(isinstance(wrapped_polygons[0].get_exterior_points()[0], pygplates.LatLonPoint))
        self.assertTrue(isinstance(wrapped_polygons[0].get_points()[0], pygplates.LatLonPoint))
        wrapped_polygons = date_line_wrapper_90.wrap(polygon)
        self.assertEquals(len(wrapped_polygons), 1)
        self.assertTrue(isinstance(wrapped_polygons[0], pygplates.DateLineWrapper.LatLonPolygon))

        # Polygon with holes should get split into two across dateline (but not when central meridian is 90).
        polygon = pygplates.PolygonOnSphere(
                [(40, 100), (-40, 100), (-40, -100), (40, -100)],  # exterior ring
                [[(30, 120), (30, 150), (-30, 150), (-30, 120)], [(30, 165), (-30, 165), (-30, -165), (30, -165)]])  # 2 interior rings
        wrapped_polygons = date_line_wrapper.wrap(polygon)
        self.assertEquals(len(wrapped_polygons), 2)
        # Total wrapped interior rings is 1 because one interior ring is clipped to dateline (so no longer interior) and the other is not.
        self.assertTrue(wrapped_polygons[0].get_number_of_interior_rings() + wrapped_polygons[1].get_number_of_interior_rings() == 1)
        # Get the sole wrapped interior ring.
        if wrapped_polygons[0].get_number_of_interior_rings() > 0:
            wrapped_polygon_with_interior_index = 0
        else:
            wrapped_polygon_with_interior_index = 1
        wrapped_interior_ring = wrapped_polygons[wrapped_polygon_with_interior_index].get_interior_points(0)
        wrapped_interior_ring_flags = wrapped_polygons[wrapped_polygon_with_interior_index].get_is_original_interior_point_flags(0)
        self.assertEquals(len(wrapped_interior_ring), 4)
        self.assertEquals(len(wrapped_interior_ring_flags), 4)
        self.assertTrue(all(wrapped_interior_ring_flags))  # all points are original points
        # Each point in wrapped interior ring should match the first interior ring we created the original polygon with.
        for llp in wrapped_interior_ring:
            self.assertTrue(llp.to_point_on_sphere() in polygon.get_interior_ring_points(0))
        wrapped_polygons = date_line_wrapper_90.wrap(polygon)
        self.assertEquals(len(wrapped_polygons), 1)
        self.assertTrue(wrapped_polygons[0].get_number_of_interior_rings() == 2)

        # Polyline should get split into three across dateline (but not when central meridian is 90).
        polyline = pygplates.PolylineOnSphere([(10, 170), (0, -170), (-10, 170)])
        wrapped_polylines = date_line_wrapper.wrap(polyline)
        self.assertEquals(len(wrapped_polylines), 3)
        self.assertTrue(isinstance(wrapped_polylines[0].get_points()[0], pygplates.LatLonPoint))
        wrapped_polylines = date_line_wrapper_90.wrap(polyline)
        self.assertEquals(len(wrapped_polylines), 1)
        self.assertTrue(isinstance(wrapped_polylines[0], pygplates.DateLineWrapper.LatLonPolyline))

        # Test polyline tessellation.
        polyline = pygplates.PolylineOnSphere([(0, 170), (0, -170)])
        wrapped_polylines = date_line_wrapper.wrap(polyline)
        self.assertEquals(len(wrapped_polylines), 2)
        self.assertEquals(len(wrapped_polylines[0].get_points()), 2)
        self.assertEquals(len(wrapped_polylines[1].get_points()), 2)
        self.assertEquals(wrapped_polylines[0].get_is_original_point_flags(), [True, False])
        self.assertEquals(wrapped_polylines[1].get_is_original_point_flags(), [False, True])
        wrapped_polylines = date_line_wrapper.wrap(polyline, 4)
        self.assertEquals(len(wrapped_polylines), 2)
        self.assertEquals(len(wrapped_polylines[0].get_points()), 4)
        self.assertEquals(len(wrapped_polylines[1].get_points()), 4)
        self.assertEquals(wrapped_polylines[0].get_is_original_point_flags(), [True, False, False, False])
        self.assertEquals(wrapped_polylines[1].get_is_original_point_flags(), [False, False, False, True])
        wrapped_polylines = date_line_wrapper_90.wrap(polyline)
        self.assertEquals(len(wrapped_polylines), 1)
        self.assertEquals(len(wrapped_polylines[0].get_points()), 2)
        self.assertEquals(wrapped_polylines[0].get_is_original_point_flags(), [True, True])
        wrapped_polylines = date_line_wrapper_90.wrap(polyline, 6)
        self.assertEquals(len(wrapped_polylines), 1)
        self.assertEquals(len(wrapped_polylines[0].get_points()), 5)
        self.assertEquals(wrapped_polylines[0].get_is_original_point_flags(), [True, False, False, False, True])

        # Test polygon tessellation.
        polygon = pygplates.PolygonOnSphere([(1, 170), (1, -170), (-1, -170), (-1, 170)])
        wrapped_polygons = date_line_wrapper.wrap(polygon)
        self.assertEquals(len(wrapped_polygons), 2)
        self.assertEquals(len(wrapped_polygons[0].get_exterior_points()), 4)
        self.assertEquals(len(wrapped_polygons[1].get_exterior_points()), 4)
        self.assertEquals(len(wrapped_polygons[0].get_is_original_exterior_point_flags()), 4)
        self.assertEquals(len(wrapped_polygons[1].get_is_original_exterior_point_flags()), 4)
        self.assertEquals(len(wrapped_polygons[0].get_points()), 4)
        self.assertEquals(len(wrapped_polygons[1].get_points()), 4)
        self.assertEquals(len(wrapped_polygons[0].get_is_original_point_flags()), 4)
        self.assertEquals(len(wrapped_polygons[1].get_is_original_point_flags()), 4)
        wrapped_polygons = date_line_wrapper.wrap(polygon, 4)
        self.assertEquals(len(wrapped_polygons), 2)
        self.assertEquals(len(wrapped_polygons[0].get_exterior_points()), 8)
        self.assertEquals(len(wrapped_polygons[1].get_exterior_points()), 8)
        self.assertEquals(len(wrapped_polygons[0].get_is_original_exterior_point_flags()), 8)
        self.assertEquals(len(wrapped_polygons[1].get_is_original_exterior_point_flags()), 8)
        wrapped_polygons = date_line_wrapper_90.wrap(polygon)
        self.assertEquals(len(wrapped_polygons), 1)
        self.assertEquals(len(wrapped_polygons[0].get_exterior_points()), 4)
        self.assertEquals(len(wrapped_polygons[0].get_is_original_exterior_point_flags()), 4)
        wrapped_polygons = date_line_wrapper_90.wrap(polygon, 6)
        self.assertEquals(len(wrapped_polygons), 1)
        self.assertEquals(len(wrapped_polygons[0].get_exterior_points()), 10)
        self.assertEquals(len(wrapped_polygons[0].get_is_original_exterior_point_flags()), 10)

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
    
    def test_create_great_circle_point_rotation(self):
        self.assertTrue(pygplates.FiniteRotation.create_great_circle_point_rotation((10,10), (55,132)) * pygplates.PointOnSphere(10,10)
                == pygplates.PointOnSphere(55,132))
        self.assertTrue(pygplates.FiniteRotation((10,10), (55,132)) * pygplates.PointOnSphere(10,10)
                == pygplates.PointOnSphere(55,132))

        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation.create_great_circle_point_rotation((10,10), (10,10))))
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation((10,10), (10,10))))
        
        self.assertAlmostEqual(pygplates.FiniteRotation((10,10), (-10,-170))
                .get_lat_lon_euler_pole_and_angle_degrees()[2], 180, 4)
        lat, lon, angle = pygplates.FiniteRotation((10,10), (-30,10)).get_lat_lon_euler_pole_and_angle_degrees()
        self.assertAlmostEqual(lat, 0)
        self.assertAlmostEqual(lon, 100)
        self.assertAlmostEqual(angle, 40)
    
    def test_create_small_circle_point_rotation(self):
        # When pole is North Pole the rotated longitudes much match (but not necessarily latitudes).
        self.assertTrue(pygplates.FiniteRotation.create_small_circle_point_rotation((90,0), (10,10), (20,20)) * pygplates.PointOnSphere(10,10)
                == pygplates.PointOnSphere(10,20))
        self.assertTrue(pygplates.FiniteRotation.create_small_circle_point_rotation((90,0), (20,20), (10,10)) * pygplates.PointOnSphere(20,20)
                == pygplates.PointOnSphere(20,10))
        self.assertTrue(pygplates.FiniteRotation.create_small_circle_point_rotation((0,90), (0,100), (10,90)) * pygplates.PointOnSphere(0,100)
                == pygplates.PointOnSphere(10,90))
        self.assertTrue(pygplates.FiniteRotation.create_small_circle_point_rotation((0,90), (10,90), (0,100)) * pygplates.PointOnSphere(10,90)
                == pygplates.PointOnSphere(0,100))
        # When 'from' or 'to' point coincides with rotation pole then we get identity rotation.
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation.create_small_circle_point_rotation((10,10), (10,10), (20,20))))
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation.create_small_circle_point_rotation((10,10), (20, 20), (10,10))))
    
    def test_create_segment_rotation(self):
        # We know 'from' and 'to' arcs are same length because each is a 90 degree longitude rotation at a 10 degree latitude around their respective poles.
        # So the start point of 'from' arc rotates onto 'to', and similarly for their end points.
        segment_rotation = pygplates.FiniteRotation.create_segment_rotation((80,30), (80,120), (0,100), (10,90))
        self.assertTrue(segment_rotation * pygplates.PointOnSphere(80,30) == pygplates.PointOnSphere(0,100))
        self.assertTrue(segment_rotation * pygplates.PointOnSphere(80,120) == pygplates.PointOnSphere(10,90))
        # The 'from' and 'to' arcs don't have to be the same length, in which case the 'from' start point rotates onto the 'to' start point but
        # the 'from' end point doesn't necessarily rotate onto the 'to' end point, instead the rotated 'from' orientation matches 'to' orientation.
        segment_rotation = pygplates.FiniteRotation.create_segment_rotation((0,0), (10,0), (0,90), (0,110)) # 'from' points North, 'to' points East
        self.assertTrue(segment_rotation * pygplates.PointOnSphere(0,0) == pygplates.PointOnSphere(0,90))  # Rotated start point matches
        self.assertTrue(segment_rotation * pygplates.PointOnSphere(10,0) == pygplates.PointOnSphere(0,100)) # Rotated 'from' end point doesn't match but still East like 'to'
        # When start point of 'from' or 'to' arcs coincide and either arc is zero length then we get identity rotation.
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation.create_segment_rotation((10,10), (10,10), (10,10), (20,20))))
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation.create_segment_rotation((10,10), (20,20), (10,10), (10,10))))
        # When start point of 'from' or 'to' arcs coincide and orientation of both arcs coincide then we get identity rotation.
        self.assertTrue(pygplates.FiniteRotation.represents_identity_rotation(
                pygplates.FiniteRotation.create_segment_rotation((10,0), (20,0), (10,0), (30,0))))
    
    # Attempt to rotate each supported geometry type - an error will be raised if not supported...
    
    def test_rotate_vector(self):
        rotated_vector = self.finite_rotation * pygplates.Vector3D(1, 0, 0)
        self.assertTrue(rotated_vector == pygplates.Vector3D(0, 1, 0))
    
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
        self.assertTrue(len(rotated_polyline.get_points()) == len(polyline.get_points()))
    
    def test_rotate_polygon_on_sphere(self):
        polygon = pygplates.PolygonOnSphere(
                [pygplates.PointOnSphere(1, 0, 0),
                pygplates.PointOnSphere(0, 1, 0),
                pygplates.PointOnSphere(0, 0, 1)])
        rotated_polygon = self.finite_rotation * polygon
        self.assertTrue(isinstance(rotated_polygon, pygplates.PolygonOnSphere))
        self.assertTrue(len(rotated_polygon.get_points()) == len(polygon.get_points()))
    
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
        self.assertTrue(self.gca.get_great_circle_normal() == (1, 0, 0))
        self.assertTrue(self.gca.get_rotation_axis_lat_lon() == (0, 0))
        
        self.assertTrue(self.gca.get_arc_point(0) == self.gca.get_start_point())
        self.assertTrue(self.gca.get_arc_point(1) == self.gca.get_end_point())
        self.assertTrue(self.gca.get_arc_direction(0) == pygplates.Vector3D(0, 0, 1))
        self.assertTrue(self.gca.get_arc_direction(1) == pygplates.Vector3D(0, -1, 0))
        arc_midpoint = (pygplates.Vector3D(self.start_point.to_xyz()) + pygplates.Vector3D(self.end_point.to_xyz())).to_normalised()
        self.assertTrue(self.gca.get_arc_point(0.5) == pygplates.PointOnSphere(arc_midpoint.to_xyz()))
        self.assertTrue(self.gca.get_arc_direction(0.5) == pygplates.Vector3D.cross(self.gca.get_great_circle_normal(), arc_midpoint))
        arc_quarter_point = (pygplates.Vector3D(self.start_point.to_xyz()) + arc_midpoint).to_normalised()
        self.assertTrue(self.gca.get_arc_point(0.25) == pygplates.PointOnSphere(arc_quarter_point.to_xyz()))
        self.assertTrue(self.gca.get_arc_direction(0.25) == pygplates.Vector3D.cross(self.gca.get_great_circle_normal(), arc_quarter_point))
        self.assertRaises(ValueError, self.gca.get_arc_point, -0.01)
        self.assertRaises(ValueError, self.gca.get_arc_point, 1.01)
        self.assertRaises(ValueError, self.gca.get_arc_direction, -0.01)
        self.assertRaises(ValueError, self.gca.get_arc_direction, 1.01)
    
    def test_zero_length(self):
        self.assertFalse(self.gca.is_zero_length())
        zero_length_gca = pygplates.GreatCircleArc(self.start_point, self.start_point)
        self.assertTrue(zero_length_gca.is_zero_length())
        self.assertRaises(pygplates.IndeterminateArcRotationAxisError, zero_length_gca.get_rotation_axis)
        self.assertRaises(pygplates.IndeterminateArcRotationAxisError, zero_length_gca.get_rotation_axis_lat_lon)
        self.assertRaises(pygplates.IndeterminateGreatCircleArcNormalError, zero_length_gca.get_great_circle_normal)
        self.assertRaises(pygplates.IndeterminateGreatCircleArcDirectionError, zero_length_gca.get_arc_direction, 0.5)
    
    def test_tessellate(self):
        tessellation_points = self.gca.to_tessellated(math.radians(91))
        self.assertTrue(len(tessellation_points) == 2)
        self.assertTrue(tessellation_points[0] == self.gca.get_start_point())
        self.assertTrue(tessellation_points[1] == self.gca.get_end_point())
        
        tessellation_points = self.gca.to_tessellated(math.radians(46))
        self.assertTrue(len(tessellation_points) == 3)
        self.assertTrue(tessellation_points[0] == self.gca.get_start_point())
        self.assertTrue(tessellation_points[1] == pygplates.FiniteRotation((1,0,0), math.pi / 4) * self.gca.get_start_point())
        self.assertTrue(tessellation_points[2] == self.gca.get_end_point())
        
        tessellation_points = self.gca.to_tessellated(math.radians(44))
        self.assertTrue(len(tessellation_points) == 4)
        self.assertTrue(tessellation_points[0] == self.gca.get_start_point())
        self.assertTrue(tessellation_points[1] == pygplates.FiniteRotation((1,0,0), math.pi / 6) * self.gca.get_start_point())
        self.assertTrue(tessellation_points[2] == pygplates.FiniteRotation((1,0,0), 2 * math.pi / 6) * self.gca.get_start_point())
        self.assertTrue(tessellation_points[3] == self.gca.get_end_point())


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

def assert_almost_equal_tuple(unit_test_case, tuple1, tuple2):
    unit_test_case.assertTrue(len(tuple1) == len(tuple2))
    for index in range(len(tuple1)):
        unit_test_case.assertAlmostEqual(tuple1[index], tuple2[index])

class LocalCartesianCase(unittest.TestCase):
    def setUp(self):
        self.point = pygplates.PointOnSphere(1, 0, 1, True)
        self.local_cartesian = pygplates.LocalCartesian(self.point)
    
    def test_get_north_east_down(self):
        self.assertTrue(self.local_cartesian.get_north() == pygplates.Vector3D.create_normalised(-1, 0, 1))
        self.assertTrue(self.local_cartesian.get_east() == pygplates.Vector3D.create_normalised(0, 1, 0))
        self.assertTrue(self.local_cartesian.get_down() == pygplates.Vector3D.create_normalised(-1, 0, -1))
    
    def test_convert_from_geocentric_to_north_east_down(self):
        self.assertTrue(self.local_cartesian.from_geocentric_to_north_east_down(self.local_cartesian.get_north()) == pygplates.Vector3D(1, 0, 0))
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(self.point, self.local_cartesian.get_north()) == pygplates.Vector3D(1, 0, 0))
        self.assertTrue(self.local_cartesian.from_geocentric_to_north_east_down(self.local_cartesian.get_east()) == pygplates.Vector3D(0, 1, 0))
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(self.point, self.local_cartesian.get_east()) == pygplates.Vector3D(0, 1, 0))
        self.assertTrue(self.local_cartesian.from_geocentric_to_north_east_down(self.local_cartesian.get_down()) == pygplates.Vector3D(0, 0, 1))
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(self.point, self.local_cartesian.get_down()) == pygplates.Vector3D(0, 0, 1))
        
        self.assertTrue(self.local_cartesian.from_geocentric_to_north_east_down(1,0,1) == pygplates.Vector3D(0, 0, -math.sqrt(2)))
        self.assertTrue(self.local_cartesian.from_geocentric_to_north_east_down((1,0,1)) == pygplates.Vector3D(0, 0, -math.sqrt(2)))
        self.assertTrue(self.local_cartesian.from_geocentric_to_north_east_down(pygplates.Vector3D(1,0,1)) == pygplates.Vector3D(0, 0, -math.sqrt(2)))
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(self.point, 1,0,1) == pygplates.Vector3D(0, 0, -math.sqrt(2)))
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(self.point, (1,0,1)) == pygplates.Vector3D(0, 0, -math.sqrt(2)))
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(self.point, pygplates.Vector3D(1,0,1)) == pygplates.Vector3D(0, 0, -math.sqrt(2)))
        
        local_origins = ((0,0), (0,0,1))
        vectors = ((1,0,0), (0,0,1))
        ned_vectors = [pygplates.Vector3D(0,0,-1), pygplates.Vector3D(0,0,-1)]
        self.assertTrue(pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(local_origins, vectors) == ned_vectors)
    
    def test_convert_from_north_east_down_to_geocentric(self):
        self.assertTrue(self.local_cartesian.from_north_east_down_to_geocentric(pygplates.Vector3D(1, 0, 0)) == self.local_cartesian.get_north())
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(self.point, pygplates.Vector3D(1, 0, 0)) == self.local_cartesian.get_north())
        self.assertTrue(self.local_cartesian.from_north_east_down_to_geocentric(pygplates.Vector3D(0, 1, 0)) == self.local_cartesian.get_east())
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(self.point, pygplates.Vector3D(0, 1, 0)) == self.local_cartesian.get_east())
        self.assertTrue(self.local_cartesian.from_north_east_down_to_geocentric(pygplates.Vector3D(0, 0, 1)) == self.local_cartesian.get_down())
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(self.point, pygplates.Vector3D(0, 0, 1)) == self.local_cartesian.get_down())
        
        self.assertTrue(self.local_cartesian.from_north_east_down_to_geocentric(0, 0, -math.sqrt(2)) == pygplates.Vector3D(1,0,1))
        self.assertTrue(self.local_cartesian.from_north_east_down_to_geocentric((0, 0, -math.sqrt(2))) == pygplates.Vector3D(1,0,1))
        self.assertTrue(self.local_cartesian.from_north_east_down_to_geocentric(pygplates.Vector3D(0, 0, -math.sqrt(2))) == pygplates.Vector3D(1,0,1))
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(self.point, 0, 0, -math.sqrt(2)) == pygplates.Vector3D(1,0,1))
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(self.point, (0, 0, -math.sqrt(2))) == pygplates.Vector3D(1,0,1))
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(self.point, pygplates.Vector3D(0, 0, -math.sqrt(2))) == pygplates.Vector3D(1,0,1))
        
        local_origins = ((0,0), (0,0,1))
        vectors = ((0,0,-1), (0,0,-1))
        geocentric_vectors = [pygplates.Vector3D(1,0,0), pygplates.Vector3D(0,0,1)]
        self.assertTrue(pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(local_origins, vectors) == geocentric_vectors)
    
    def test_convert_from_geocentric_to_magnitude_azimuth_inclination(self):
        assert_almost_equal_tuple(self, self.local_cartesian.from_geocentric_to_magnitude_azimuth_inclination(self.local_cartesian.get_north()), (1, 0, 0))
        assert_almost_equal_tuple(self, pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(self.point, self.local_cartesian.get_north()), (1, 0, 0))
        assert_almost_equal_tuple(self, self.local_cartesian.from_geocentric_to_magnitude_azimuth_inclination(self.local_cartesian.get_east()), (1, math.pi / 2, 0))
        assert_almost_equal_tuple(self, pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(self.point, self.local_cartesian.get_east()), (1, math.pi / 2, 0))
        assert_almost_equal_tuple(self, self.local_cartesian.from_geocentric_to_magnitude_azimuth_inclination(self.local_cartesian.get_down()), (1, 0, math.pi / 2))
        assert_almost_equal_tuple(self, pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(self.point, self.local_cartesian.get_down()), (1, 0, math.pi / 2))
        
        assert_almost_equal_tuple(self, self.local_cartesian.from_geocentric_to_magnitude_azimuth_inclination(1,0,1), (math.sqrt(2), 0, -math.pi / 2))
        assert_almost_equal_tuple(self, self.local_cartesian.from_geocentric_to_magnitude_azimuth_inclination((1,0,1)), (math.sqrt(2), 0, -math.pi / 2))
        assert_almost_equal_tuple(self, self.local_cartesian.from_geocentric_to_magnitude_azimuth_inclination(pygplates.Vector3D(1,0,1)), (math.sqrt(2), 0, -math.pi / 2))
        assert_almost_equal_tuple(self, pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(self.point, 1,0,1), (math.sqrt(2), 0, -math.pi / 2))
        assert_almost_equal_tuple(self, pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(self.point, (1,0,1)), (math.sqrt(2), 0, -math.pi / 2))
        assert_almost_equal_tuple(self, pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(self.point, pygplates.Vector3D(1,0,1)), (math.sqrt(2), 0, -math.pi / 2))
        
        local_origins = ((0,0), (0,0,1))
        vectors = ((1,0,0), (0,0,1))
        mai_tuples = [(1,0,-math.pi / 2), (1,0,-math.pi / 2)]
        converted_mai_tuples = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(local_origins, vectors)
        self.assertTrue(len(converted_mai_tuples) == len(mai_tuples))
        for index in range(len(mai_tuples)):
            assert_almost_equal_tuple(self, converted_mai_tuples[index], mai_tuples[index])
    
    def test_convert_from_magnitude_azimuth_inclination_to_geocentric(self):
        self.assertTrue(self.local_cartesian.from_magnitude_azimuth_inclination_to_geocentric((1, 0, 0)) == self.local_cartesian.get_north())
        self.assertTrue(pygplates.LocalCartesian.convert_from_magnitude_azimuth_inclination_to_geocentric(self.point, (1, 0, 0)) == self.local_cartesian.get_north())
        self.assertTrue(self.local_cartesian.from_magnitude_azimuth_inclination_to_geocentric((1, math.pi / 2, 0)) == self.local_cartesian.get_east())
        self.assertTrue(pygplates.LocalCartesian.convert_from_magnitude_azimuth_inclination_to_geocentric(self.point, (1, math.pi / 2, 0)) == self.local_cartesian.get_east())
        self.assertTrue(self.local_cartesian.from_magnitude_azimuth_inclination_to_geocentric((1, 0, math.pi / 2)) == self.local_cartesian.get_down())
        self.assertTrue(pygplates.LocalCartesian.convert_from_magnitude_azimuth_inclination_to_geocentric(self.point, (1, 0, math.pi / 2)) == self.local_cartesian.get_down())
        
        self.assertTrue(self.local_cartesian.from_magnitude_azimuth_inclination_to_geocentric(math.sqrt(2), 0, -math.pi / 2) == pygplates.Vector3D(1,0,1))
        self.assertTrue(self.local_cartesian.from_magnitude_azimuth_inclination_to_geocentric((math.sqrt(2), 0, -math.pi / 2)) == pygplates.Vector3D(1,0,1))
        self.assertTrue(pygplates.LocalCartesian.convert_from_magnitude_azimuth_inclination_to_geocentric(self.point, math.sqrt(2), 0, -math.pi / 2) == pygplates.Vector3D(1,0,1))
        self.assertTrue(pygplates.LocalCartesian.convert_from_magnitude_azimuth_inclination_to_geocentric(self.point, (math.sqrt(2), 0, -math.pi / 2)) == pygplates.Vector3D(1,0,1))
        
        local_origins = ((0,0), (0,0,1))
        mai_tuples = ((1,0,-math.pi / 2), (1,0,-math.pi / 2))
        geocentric_vectors = [pygplates.Vector3D(1,0,0), pygplates.Vector3D(0,0,1)]
        self.assertTrue(pygplates.LocalCartesian.convert_from_magnitude_azimuth_inclination_to_geocentric(local_origins, mai_tuples) == geocentric_vectors)


class Vector3DCase(unittest.TestCase):
    def setUp(self):
        self.xyz = (1.03, -2.232, 34.232)
        self.vector = pygplates.Vector3D(self.xyz)
    
    def test_compare(self):
        self.assertTrue(self.vector == pygplates.Vector3D(self.xyz))
        x, y, z = self.xyz
        self.assertTrue(self.vector == pygplates.Vector3D(x, y, z))
    
    def test_constants(self):
        self.assertTrue(pygplates.Vector3D.zero == pygplates.Vector3D(0,0,0))
        self.assertTrue(pygplates.Vector3D.x_axis == pygplates.Vector3D(1,0,0))
        self.assertTrue(pygplates.Vector3D.y_axis == pygplates.Vector3D(0,1,0))
        self.assertTrue(pygplates.Vector3D.z_axis == pygplates.Vector3D(0,0,1))
        self.assertTrue(
                2 * pygplates.Vector3D.x_axis + 3 * pygplates.Vector3D.y_axis + 4 * pygplates.Vector3D.z_axis ==
                pygplates.Vector3D(2,3,4))

    def test_get(self):
        self.assertTrue(self.vector.get_x() == self.xyz[0])
        self.assertTrue(self.vector.get_y() == self.xyz[1])
        self.assertTrue(self.vector.get_z() == self.xyz[2])
    
    def test_to_xyz(self):
        self.assertTrue(self.vector.to_xyz() == self.xyz)
    
    def test_operators(self):
        scale = 3.1
        scaled_xyz = (scale * self.xyz[0], scale * self.xyz[1], scale * self.xyz[2])
        self.assertTrue(scale * self.vector == pygplates.Vector3D(scaled_xyz))
        self.assertTrue(self.vector * scale == pygplates.Vector3D(scaled_xyz))
        
        self.assertTrue(self.vector - self.vector == pygplates.Vector3D(0,0,0))
        self.assertTrue(self.vector + self.vector == 2 * self.vector)
        self.assertTrue(-self.vector == -1 * self.vector)
    
    def test_angle_between(self):
        self.assertAlmostEqual(pygplates.Vector3D.angle_between((1,0,1), (2,0,0)), math.pi / 4)
        self.assertAlmostEqual(pygplates.Vector3D.angle_between(pygplates.Vector3D(-1,0,1), (2,0,0)), 3 * math.pi / 4)
        self.assertAlmostEqual(pygplates.Vector3D.angle_between(pygplates.Vector3D(-1,0,1), (-1,0,1)), 0)
        self.assertAlmostEqual(pygplates.Vector3D.angle_between(pygplates.Vector3D(-1,0,1), (1,0,-1)), math.pi)
        self.assertRaises(pygplates.UnableToNormaliseZeroVectorError, pygplates.Vector3D.angle_between, self.vector, pygplates.Vector3D(0,0,0))
    
    def test_dot_product(self):
        self.assertAlmostEqual(pygplates.Vector3D.dot((1,1,1), (2,2,2)), 6)
        self.assertAlmostEqual(pygplates.Vector3D.dot(pygplates.Vector3D(1,0,0), (0,1,0)), 0)
        self.assertAlmostEqual(pygplates.Vector3D.dot([1,-1,0.5], pygplates.Vector3D(0,1,-2).to_xyz()), -2)
    
    def test_cross_product(self):
        self.assertTrue(pygplates.Vector3D.cross(pygplates.Vector3D(2,0,0), (1,0,0)) == pygplates.Vector3D(0,0,0))
        self.assertTrue(pygplates.Vector3D.cross(pygplates.Vector3D(2,0,0), (0,1,0)) == pygplates.Vector3D(0,0,2))
        self.assertTrue(pygplates.Vector3D.cross(pygplates.Vector3D(0,1,0), (2,0,0)) == pygplates.Vector3D(0,0,-2))
    
    def test_magnitude(self):
        self.assertTrue(pygplates.Vector3D(0,0,0).is_zero_magnitude())
        self.assertTrue(not pygplates.Vector3D(2,0,0).is_zero_magnitude())
        self.assertAlmostEqual(pygplates.Vector3D(2,0,0).get_magnitude(), 2)
        self.assertAlmostEqual(self.vector.get_magnitude(), math.sqrt(pygplates.Vector3D.dot(self.vector, self.vector)))
    
    def test_normalise(self):
        self.assertTrue(pygplates.Vector3D(2,0,0).to_normalised() == pygplates.Vector3D(1,0,0))
        self.assertTrue(pygplates.Vector3D.create_normalised((2,0,0)) == pygplates.Vector3D(1,0,0))
        self.assertTrue(pygplates.Vector3D.create_normalised(2,0,0) == pygplates.Vector3D(1,0,0))
        self.assertTrue(pygplates.Vector3D(2,0,0).to_normalised() == pygplates.Vector3D(1,0,0)) # American spelling.
        self.assertTrue(pygplates.Vector3D.create_normalised((2,0,0)) == pygplates.Vector3D(1,0,0)) # American spelling.
        self.assertTrue(pygplates.Vector3D.create_normalised(2,0,0) == pygplates.Vector3D(1,0,0)) # American spelling.
        vec = pygplates.Vector3D(1,-1,1)
        self.assertTrue(vec.to_normalised() == (1.0 / vec.get_magnitude()) * vec)
        self.assertTrue(pygplates.Vector3D.create_normalised(vec.to_xyz()) == (1.0 / vec.get_magnitude()) * vec) # American spelling.
        self.assertTrue(pygplates.Vector3D.create_normalised(vec.get_x(), vec.get_y(), vec.get_z()) == (1.0 / vec.get_magnitude()) * vec) # American spelling.
        
        self.assertRaises(pygplates.UnableToNormaliseZeroVectorError, pygplates.Vector3D(0,0,0).to_normalised)
        self.assertRaises(pygplates.UnableToNormaliseZeroVectorError, pygplates.Vector3D.create_normalised, 0, 0, 0)


class IntegerFloatCase(unittest.TestCase):
    
    def test_numpy_scalar_to_integer_float(self):
        try:
            import numpy as np;
        except ImportError:
            print('NumPy not installed: skipping NumPy scalar conversion test')
            return
        
        #
        # Just using XsInteger and XsDouble as a way of accepting integer and floating-point numbers.
        #
        # These should all not raise an exception (Boost.Python.ArgumentError).
        #

        pygplates.XsInteger(np.longlong(1000))
        pygplates.XsInteger(np.int64(1000))
        pygplates.XsInteger(np.int32(-1000))
        pygplates.XsInteger(np.int16(-1000))
        pygplates.XsInteger(np.uint8(100))
        pygplates.XsInteger(np.uint(1000))
        self.assertTrue(pygplates.XsInteger(np.uint(1000)).get_integer() == 1000)
        
        f = pygplates.Feature()
        f.set_reconstruction_plate_id(np.uint32(801))
        self.assertTrue(f.get_reconstruction_plate_id() == 801)

        pygplates.XsDouble(np.longlong(-1000))
        pygplates.XsDouble(np.int64(1000))
        pygplates.XsDouble(np.int32(-1000))
        pygplates.XsDouble(np.uint(1000))
        pygplates.XsDouble(np.float64(1000))
        pygplates.XsDouble(np.float32(-1000))
        pygplates.XsDouble(np.float_(1000))
        pygplates.XsDouble(np.double(1000))
        pygplates.XsDouble(np.longdouble(-1000))
        self.assertAlmostEqual(pygplates.XsDouble(np.float64(105.67)).get_double(), 105.67)

        pygplates.GeoTimeInstant(np.float64(140.6))
        self.assertTrue(pygplates.GeoTimeInstant(np.float64(140.6)) == pygplates.GeoTimeInstant(140.6))

        f.set_valid_time(np.longdouble(140.23), np.single(10.54))
        begin, end = f.get_valid_time()
        self.assertAlmostEqual(begin, 140.23)
        self.assertAlmostEqual(end, 10.54)


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            DateLineWrapperCase,
            FiniteRotationCase,
            GreatCircleArcCase,
            LatLonPointCase,
            LocalCartesianCase,
            Vector3DCase,
            IntegerFloatCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    # Add test suites from sibling modules.
    test_modules = [
            test_maths.test_geometries_on_sphere
            ]

    for test_module in test_modules:
        suite.addTest(test_module.suite())
    
    return suite
