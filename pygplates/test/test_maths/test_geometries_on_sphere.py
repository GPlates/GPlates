"""
Unit tests for the pygplates geometries on sphere.
"""

import math
import os
import unittest
import pickle
import pygplates
# Test using numpy if it's available...
try:
    import numpy
    imported_numpy = True
except ImportError:
    imported_numpy = False

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GeometryOnSphereCase(unittest.TestCase):
    def test_clone(self):
        point = pygplates.PointOnSphere(1, 0, 0)
        self.assertEquals(point.clone(), point)
        self.assertTrue(isinstance(point.clone(), pygplates.PointOnSphere))
        # Call a method that only PointOnSphere has.
        self.assertTrue(point.clone().get_x() == point.get_x())
        self.assertTrue(list(point.clone().get_points()) == list(point.get_points()))
        polyline = pygplates.PolylineOnSphere([(0,0), (10,0), (10,10)])
        self.assertTrue(list(polyline.clone().get_points()) == list(polyline.get_points()))
    
    def test_distance(self):
        point1 = pygplates.PointOnSphere((0, 0))
        point2 = pygplates.PointOnSphere((0, math.degrees(math.pi / 3)))
        polyline1 = pygplates.PolylineOnSphere([
                (0, math.degrees(math.pi / 13)),
                (0, math.degrees(math.pi / 6)),
                (0, math.degrees(math.pi / 2)),
                (0, math.degrees(math.pi / 1))])
        polyline2 = pygplates.PolylineOnSphere([
                (math.degrees(math.pi / 18), math.degrees(math.pi / 13.1)),
                (math.degrees(math.pi / 17), math.degrees(math.pi / 6)),
                (math.degrees(math.pi / 19), math.degrees(math.pi / 4)),
                (math.degrees(math.pi / 17), math.degrees(math.pi / 2)),
                (math.degrees(math.pi / 18), math.degrees(math.pi / 1))])
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(point1, point2), math.pi / 3)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(point1, polyline1), math.pi / 13)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(polyline1, polyline2), math.pi / 19)
        distance, closest_point1, closest_point2, closest_index1, closest_index2 = pygplates.GeometryOnSphere.distance(
                polyline1, polyline2, return_closest_positions=True, return_closest_indices=True)
        self.assertAlmostEqual(distance, math.pi / 19)
        self.assertTrue(closest_point1 == pygplates.PointOnSphere((0, math.degrees(math.pi / 4))))
        self.assertTrue(closest_point2 == pygplates.PointOnSphere((math.degrees(math.pi / 19), math.degrees(math.pi / 4))))
        self.assertTrue(closest_index1 == 1)
        # Both segments 1 and 2 of 'polyline2' contain closest point so could be either...
        self.assertTrue(closest_index2 == 1 or closest_index2 == 2)
        # Test using distance thresholds.
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(polyline1, polyline2, math.pi / 18), math.pi / 19)
        self.assertTrue(pygplates.GeometryOnSphere.distance(polyline1, polyline2, math.pi / 20) is None)
        
        # Read some test data.
        # The distances were determined using the Measure canvas tool in GPlates.
        features = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'geometry_distance.gpml'))
        features = list(features)
        self.assertTrue(len(features) == 3)
        
        # 'features[0]' is a polygon.
        distance, closest_point1, closest_point2 = pygplates.GeometryOnSphere.distance(
                features[0].get_geometry(), features[1].get_geometry(),
                return_closest_positions=True, geometry1_is_solid=True)
        # One of the points of the multi-point is inside the (solid) polygon.
        self.assertAlmostEqual(distance, 0)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(closest_point1, closest_point2), 0.03407617)
        
        distance, closest_point1, closest_point2 = pygplates.GeometryOnSphere.distance(
                features[0].get_geometry(), features[2].get_geometry(),
                return_closest_positions=True, geometry1_is_solid=True)
        # The polyline intersects the polygon.
        self.assertAlmostEqual(distance, 0)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(closest_point1, closest_point2), 0)
    
    def test_get_centroid(self):
        # Test all geometry types have a 'get_controid()' method.
        self.assertTrue(isinstance(pygplates.PointOnSphere(0, 1, 0).get_centroid(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(pygplates.MultiPointOnSphere([(0, 0), (10, 10)]).get_centroid(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(pygplates.PolylineOnSphere([(0, 0), (10, 10)]).get_centroid(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(pygplates.PolygonOnSphere([(0, 0), (10, 10), (20, 20)]).get_centroid(), pygplates.PointOnSphere))


class PointOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.xyz = (1, 0, 0)
    
    def test_static_data_members(self):
        self.assertTrue(pygplates.PointOnSphere.north_pole == pygplates.PointOnSphere(0,0,1))
        self.assertTrue(pygplates.PointOnSphere.south_pole == pygplates.PointOnSphere(0,0,-1))
    
    def test_construct(self):
        point = pygplates.PointOnSphere(self.xyz[0], self.xyz[1], self.xyz[2])
        self.assertEquals(point.to_xyz(), self.xyz)
        # A non-unit length vector raises error.
        self.assertRaises(pygplates.ViolatedUnitVectorInvariantError, pygplates.PointOnSphere, 1, 1, 1)
        # A non-unit length vector is fine if normalisation requested.
        self.assertTrue(pygplates.PointOnSphere(2, 0, 0, True) == pygplates.PointOnSphere(1, 0, 0))
        self.assertTrue(pygplates.PointOnSphere(2, 2, 2, True) == pygplates.PointOnSphere(1, 1, 1, True))
        # A zero-length vector raises error if normalisation requested.
        self.assertRaises(pygplates.UnableToNormaliseZeroVectorError, pygplates.PointOnSphere, 0, 0, 0, True)
        
        lat, lon = 45, 60
        point = pygplates.PointOnSphere(lat, lon)
        self.assertAlmostEqual(point.to_lat_lon()[0], lat)
        self.assertAlmostEqual(point.to_lat_lon()[1], lon)
        self.assertRaises(pygplates.InvalidLatLonError, pygplates.PointOnSphere, 95, 60)
    
    def test_convert(self):
        point_on_sphere = pygplates.PointOnSphere(self.xyz)
        # Can construct from PointOnSphere.
        point_on_sphere = pygplates.PointOnSphere(point_on_sphere)
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from (x,y,z) tuple.
        xyz_tuple = point_on_sphere.to_xyz()
        point_on_sphere = pygplates.PointOnSphere(xyz_tuple)
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from (x,y,z) list.
        point_on_sphere = pygplates.PointOnSphere(list(xyz_tuple))
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from (x,y,z) numpy array.
        if imported_numpy:
            point_on_sphere = pygplates.PointOnSphere(numpy.array(xyz_tuple))
            self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from LatLonPoint.
        point_on_sphere = pygplates.PointOnSphere(point_on_sphere.to_lat_lon_point())
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from (lat,lon) tuple.
        lat_lon_tuple = point_on_sphere.to_lat_lon()
        point_on_sphere = pygplates.PointOnSphere(lat_lon_tuple)
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from (lat,lon) list.
        point_on_sphere = pygplates.PointOnSphere(list(lat_lon_tuple))
        self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
        # Can construct from (lat,lon) numpy array.
        if imported_numpy:
            point_on_sphere = pygplates.PointOnSphere(numpy.array(lat_lon_tuple))
            self.assertTrue(isinstance(point_on_sphere, pygplates.PointOnSphere))
    
    def test_get_points(self):
        point = pygplates.PointOnSphere(1, 0, 0)
        points = point.get_points()
        self.assertTrue(len(points) == 1 and point == points[0])
    
    def test_to_lat_lon(self):
        point = pygplates.PointOnSphere(0, 1, 0)
        lat, lon = point.to_lat_lon()
        self.assertAlmostEqual(lat, 0)
        self.assertAlmostEqual(lon, 90)
        lat_lon_point = point.to_lat_lon_point()
        self.assertAlmostEqual(lat_lon_point.get_latitude(), 0)
        self.assertAlmostEqual(lat_lon_point.get_longitude(), 90)
    
    def test_to_xyz(self):
        point = pygplates.PointOnSphere(0, 1, 0)
        self.assertAlmostEqual(point.get_x(), 0)
        self.assertAlmostEqual(point.get_y(), 1)
        self.assertAlmostEqual(point.get_z(), 0)
        x, y, z = point.to_xyz()
        self.assertAlmostEqual(x, 0)
        self.assertAlmostEqual(y, 1)
        self.assertAlmostEqual(z, 0)
    
    def test_get_centroid(self):
        point = pygplates.PointOnSphere(0, 1, 0)
        self.assertEqual(point.get_centroid(), point)
    
    def test_pickle(self):
        point_on_sphere = pygplates.PointOnSphere(self.xyz)
        pickled_point_on_sphere = pickle.loads(pickle.dumps(point_on_sphere))
        self.assertTrue(pickled_point_on_sphere == point_on_sphere)


class MultiPointOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(0, -1, 0))
        self.multi_point = pygplates.MultiPointOnSphere(self.points)

    def test_construct(self):
        # Need at least one point.
        self.assertRaises(
                pygplates.InsufficientPointsForMultiPointConstructionError,
                pygplates.MultiPointOnSphere,
                [])
        lat_lon_tuple_list = [point.to_lat_lon() for point in self.points]
        multi_point = pygplates.MultiPointOnSphere(lat_lon_tuple_list)
        self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
        xyz_list_list = [list(point.to_xyz()) for point in self.points]
        multi_point = pygplates.MultiPointOnSphere(xyz_list_list)
        self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
        # Can construct directly from other GeometryOnSphere types.
        self.assertEquals(
                list(pygplates.MultiPointOnSphere(pygplates.MultiPointOnSphere(self.points)).get_points()),
                self.points)
        self.assertEquals(
                list(pygplates.MultiPointOnSphere(pygplates.PolylineOnSphere(self.points)).get_points()),
                self.points)
        self.assertEquals(
                list(pygplates.MultiPointOnSphere(pygplates.PolygonOnSphere(self.points)).get_points()),
                self.points)
    
    def test_convert(self):
        # Convert to (lat,lon) tuple list and back.
        lat_lon_list = self.multi_point.to_lat_lon_list()
        self.assertEquals(len(lat_lon_list), len(self.multi_point))
        multi_point = pygplates.MultiPointOnSphere(lat_lon_list)
        self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
        # Convert to (x,y,z) tuple list and back.
        xyz_list = self.multi_point.to_xyz_list()
        self.assertEquals(len(xyz_list), len(self.multi_point))
        multi_point = pygplates.MultiPointOnSphere(xyz_list)
        self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
        if imported_numpy:
            # Convert to (lat,lon) numpy array and back.
            lat_lon_array = self.multi_point.to_lat_lon_array()
            self.assertEquals(len(list(lat_lon_array)), len(self.multi_point))
            multi_point = pygplates.MultiPointOnSphere(lat_lon_array)
            self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
            # Convert to (x,y,z) numpy array and back.
            xyz_array = self.multi_point.to_xyz_array()
            self.assertEquals(len(list(xyz_array)), len(self.multi_point))
            multi_point = pygplates.MultiPointOnSphere(xyz_array)
            self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
    
    def test_convert_geometry(self):
        # Convert point to multipoint.
        self.assertTrue(pygplates.MultiPointOnSphere(pygplates.PointOnSphere(1, 0, 0))
                == pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1, 0, 0)]))
        # Convert multipoint to multipoint.
        self.assertTrue(
                pygplates.MultiPointOnSphere(pygplates.MultiPointOnSphere(pygplates.PointOnSphere(1, 0, 0)))
                == pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1, 0, 0)]))
        # Convert polyline to multipoint.
        self.assertTrue(
                pygplates.MultiPointOnSphere(pygplates.PolylineOnSphere([(0,0), (10,0)]))
                == pygplates.MultiPointOnSphere([(0,0), (10,0)]))
        # Convert polygon to multipoint.
        self.assertTrue(
                pygplates.MultiPointOnSphere(pygplates.PolygonOnSphere([(0,0), (10,0), (20,0)]))
                == pygplates.MultiPointOnSphere([(0,0), (10,0), (20,0)]))
    
    def test_get_points(self):
        point_sequence = self.multi_point.get_points()
        self.assertEquals(self.multi_point, pygplates.MultiPointOnSphere(point_sequence))
    
    def test_compare(self):
        self.assertEquals(self.multi_point, pygplates.MultiPointOnSphere(self.points))
    
    def test_iter(self):
        iter(self.multi_point)
        points = [point for point in self.multi_point]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.multi_point))
    
    def test_len(self):
        self.assertTrue(len(self.multi_point) == len(self.points))
    
    def test_contains(self):
        self.assertTrue(self.points[0] in self.multi_point)
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.multi_point)
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.multi_point)

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
    
    def test_centroid(self):
        self.assertTrue(isinstance(self.multi_point.get_centroid(), pygplates.PointOnSphere))


class PolylineOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(0, -1, 0))
        self.polyline = pygplates.PolylineOnSphere(self.points)

    def test_construct(self):
        # Need at least two points.
        self.assertRaises(
                pygplates.InvalidPointsForPolylineConstructionError,
                pygplates.PolylineOnSphere,
                [pygplates.PointOnSphere(1, 0, 0)])
        lat_lon_tuple_list = [point.to_lat_lon() for point in self.points]
        polyline = pygplates.PolylineOnSphere(lat_lon_tuple_list)
        self.assertTrue(isinstance(polyline, pygplates.PolylineOnSphere))
        xyz_list_list = [list(point.to_xyz()) for point in self.points]
        polyline = pygplates.PolylineOnSphere(xyz_list_list)
        self.assertTrue(isinstance(polyline, pygplates.PolylineOnSphere))
        # Can construct directly from other GeometryOnSphere types.
        self.assertEquals(
                list(pygplates.PolylineOnSphere(pygplates.MultiPointOnSphere(self.points)).get_points()),
                self.points)
        self.assertEquals(
                list(pygplates.PolylineOnSphere(pygplates.PolylineOnSphere(self.points)).get_points()),
                self.points)
        # When a polygon ring gets converted to a polyline the last/first ring vertex is added to create the last polyline arc.
        # Hence the polyline has an extra vertex.
        self.assertEquals(
                list(pygplates.PolylineOnSphere(pygplates.PolygonOnSphere(self.points)).get_points())[:-1],
                self.points)
        self.assertEquals(
                pygplates.PolylineOnSphere(pygplates.PolygonOnSphere(self.points))[-1],
                self.points[0])
    
    def test_convert_geometry(self):
        # Need at least two points.
        self.assertRaises(
                pygplates.InvalidPointsForPolylineConstructionError,
                pygplates.PolylineOnSphere,
                pygplates.PointOnSphere(1, 0, 0),
                False)
        self.assertRaises(
                pygplates.InvalidPointsForPolylineConstructionError,
                pygplates.PolylineOnSphere,
                pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1, 0, 0)]),
                False)
        # Unless allow one point - gets duplicated.
        self.assertTrue(pygplates.PolylineOnSphere(pygplates.PointOnSphere(1, 0, 0))
                == pygplates.PolylineOnSphere([pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(1, 0, 0)]))
        self.assertTrue(
                pygplates.PolylineOnSphere(pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1, 0, 0)]))
                == pygplates.PolylineOnSphere([pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(1, 0, 0)]))
        
        # Convert polyline to polyline.
        self.assertTrue(
                pygplates.PolylineOnSphere(pygplates.PointOnSphere(1, 0, 0))
                == pygplates.PolylineOnSphere(
                    pygplates.PolylineOnSphere(pygplates.PointOnSphere(1, 0, 0))))
        # Convert polygon to polyline.
        # Note that conversion from polygon to polyline gains an extra vertex (to generate last->first arc).
        self.assertTrue(
                pygplates.PolylineOnSphere(
                        [pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(1, 0, 0)])
                == pygplates.PolylineOnSphere(
                    pygplates.PolygonOnSphere(pygplates.PointOnSphere(1, 0, 0))))
        # Interior rings of polygon are ignored when converting to polyline.
        self.assertTrue(
                pygplates.PolylineOnSphere([(40, 100), (-40, 100), (-40, -100), (40, -100), (40, 100)]) # First and last vertex the same
                == pygplates.PolylineOnSphere(
                    pygplates.PolygonOnSphere(
                            [(40, 100), (-40, 100), (-40, -100), (40, -100)],  # exterior ring
                            [[(30, 120), (30, 150), (-30, 150), (-30, 120)], [(30, 165), (-30, 165), (-30, -165), (30, -165)]])))  # 2 interior rings get ignored
    
    def test_get_points(self):
        point_sequence = self.polyline.get_points()
        self.assertEquals(self.polyline, pygplates.PolylineOnSphere(point_sequence))
    
    def test_join(self):
        # Should work on any GeometryOnSphere type.
        joined_polylines = pygplates.PolylineOnSphere.join(
                [pygplates.PointOnSphere((0,10)),
                    # Note: When converted to polyline, the first and last vertices will be (0,8)...
                    pygplates.PolygonOnSphere([(0,8), (20,8), (10,8)])],
                math.radians(2.1),
                pygplates.PolylineConversion.convert_to_polyline)
        self.assertTrue(len(joined_polylines) == 1)
        self.assertTrue(isinstance(joined_polylines[0], pygplates.PolylineOnSphere))
        joined_polylines = pygplates.PolylineOnSphere.join(
                [pygplates.MultiPointOnSphere([(0,0), (0,10)]),
                    # Note: When converted to polyline, the first and last vertices will be (0,8)...
                    pygplates.PolygonOnSphere([(0,8), (20,8), (10,8)])],
                math.radians(2.1),
                pygplates.PolylineConversion.convert_to_polyline)
        self.assertTrue(len(joined_polylines) == 1)
        self.assertTrue(isinstance(joined_polylines[0], pygplates.PolylineOnSphere))
        joined_polylines = pygplates.PolylineOnSphere.join(
                [pygplates.MultiPointOnSphere([(0,0), (0,10)]),
                    # Note: When converted to polyline, the first and last vertices will be (0,8)...
                    pygplates.PolygonOnSphere([(0,8), (20,8), (10,8)])],
                math.radians(0.1),
                pygplates.PolylineConversion.convert_to_polyline)
        self.assertTrue(len(joined_polylines) == 2)
        # Anything not joined should get converted to PolylineOnSphere.
        self.assertTrue(isinstance(joined_polylines[0], pygplates.PolylineOnSphere))
        self.assertTrue(isinstance(joined_polylines[1], pygplates.PolylineOnSphere))
        
        joined_polylines = pygplates.PolylineOnSphere.join(
                [pygplates.MultiPointOnSphere([(0,0), (0,10)]),
                    pygplates.PolylineOnSphere([(20,8), (10,8), (0,8)])],
                math.radians(1.9),
                pygplates.PolylineConversion.convert_to_polyline)
        self.assertTrue(len(joined_polylines) == 2)
        joined_polylines = pygplates.PolylineOnSphere.join(
                [pygplates.MultiPointOnSphere([(0,0), (0,10)]),
                    pygplates.PolylineOnSphere([(20,8), (10,8), (0,8)])],
                math.radians(1.9),
                pygplates.PolylineConversion.ignore_non_polyline)
        self.assertTrue(len(joined_polylines) == 1)
        self.assertRaises(pygplates.GeometryTypeError,
                pygplates.PolylineOnSphere.join,
                [pygplates.MultiPointOnSphere([(0,0), (0,10)]),
                    pygplates.PolylineOnSphere([(20,8), (10,8), (0,8)])],
                math.radians(1.9),
                pygplates.PolylineConversion.raise_if_non_polyline)
        joined_polylines = pygplates.PolylineOnSphere.join(
                [pygplates.PolylineOnSphere([(0,0), (0,10)]),
                    pygplates.PolylineOnSphere([(20,8), (10,8), (0,8)])],
                math.radians(1.9),
                pygplates.PolylineConversion.raise_if_non_polyline)
        self.assertTrue(len(joined_polylines) == 2)
        
        polyline1 = pygplates.PolylineOnSphere([(0,0), (0,10)])
        polyline2 = pygplates.PolylineOnSphere([(20,8), (0,8)])
        # Too far apart.
        self.assertTrue(len(pygplates.PolylineOnSphere.join((polyline1, polyline2), math.radians(1))) == 2)
        # polyline1 + reversed(polyline2)
        self.assertTrue(
                pygplates.PolylineOnSphere.join((polyline1, polyline2), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(0,0), (0,10), (0,8), (20,8)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,1), (0,10)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(0,0), (0,10), (0,1)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,1), (0,10)]))) ==
                [pygplates.PolylineOnSphere([(0,0), (0,10), (0,1)])])
        # polyline1 + polyline2
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,12), (0,15)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(0,0), (0,10), (0,12), (0,15)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,10), (0,1)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(0,0), (0,10), (0,1)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,10), (0,1)]))) ==
                [pygplates.PolylineOnSphere([(0,0), (0,10), (0,1)])])
        # polyline2 + polyline1
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(10,1), (0,1)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(10,1), (0,1), (0,0), (0,10)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,1), (0,0)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(0,1), (0,0), (0,10)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,1), (0,0)]))) ==
                [pygplates.PolylineOnSphere([(0,1), (0,0), (0,10)])])
        # reversed(polyline2) + polyline1
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,1), (10,1)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(10,1), (0,1), (0,0), (0,10)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,0), (0,1)])), math.radians(2.1)) ==
                [pygplates.PolylineOnSphere([(0,1), (0,0), (0,10)])])
        self.assertTrue(
                pygplates.PolylineOnSphere.join(
                        (polyline1, pygplates.PolylineOnSphere([(0,0), (0,1)]))) ==
                [pygplates.PolylineOnSphere([(0,1), (0,0), (0,10)])])
        
        self.assertTrue(len(pygplates.PolylineOnSphere.join((polyline1, polyline2))) == 2)
        # End points still not close enough (since no threshold specified)...
        self.assertTrue(len(pygplates.PolylineOnSphere.join((
                pygplates.PolylineOnSphere([(0,0), (0,10)]),
                pygplates.PolylineOnSphere([(0,0.001), (0,10.001)])))) == 2)
   
    def test_rotation_interpolate(self):
        # Two polylines whose latitude ranges do not overlap.
        self.assertFalse(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(30,10), (20,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1)))
        self.assertTrue(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1)))
        self.assertFalse(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1),
                0,
                0,
                math.radians(9),
                pygplates.FlattenLongitudeOverlaps.no))
        self.assertTrue(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1),
                0,
                0,
                math.radians(11),
                pygplates.FlattenLongitudeOverlaps.no))
        
        # Using a list of interpolate ratios.
        self.assertTrue(len(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                [0.1, 0.5, 0.8])) == 3)
        self.assertTrue(len(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (20,0)]),
                pygplates.PolylineOnSphere([(5,10), (15,10)]),
                pygplates.PointOnSphere(90,0),
                [0])) == 1)
        # An empty list means just test if interpolation is possible.
        self.assertTrue(len(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (20,0)]),
                pygplates.PolylineOnSphere([(5,10), (15,10)]),
                pygplates.PointOnSphere(90,0),
                [])) == 0)
        # No overlap...
        self.assertTrue(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.PolylineOnSphere([(10,0), (20,0)]),
                pygplates.PolylineOnSphere([(25,10), (35,10)]),
                pygplates.PointOnSphere(90,0),
                []) is None)
        
        self.assertRaises(pygplates.GeometryTypeError,
                pygplates.PolylineOnSphere.rotation_interpolate,
                pygplates.MultiPointOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1),
                polyline_conversion=pygplates.PolylineConversion.raise_if_non_polyline)
        self.assertFalse(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.MultiPointOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1),
                polyline_conversion=pygplates.PolylineConversion.ignore_non_polyline))
        self.assertTrue(pygplates.PolylineOnSphere.rotation_interpolate(
                pygplates.MultiPointOnSphere([(10,0), (-10,0)]),
                pygplates.PolylineOnSphere([(20,10), (0,10)]),
                pygplates.PointOnSphere(90,0),
                math.radians(1),
                polyline_conversion=pygplates.PolylineConversion.convert_to_polyline))

    def test_compare(self):
        self.assertEquals(self.polyline, pygplates.PolylineOnSphere(self.points))
    
    def test_iter(self):
        iter(self.polyline)
        points = [point for point in self.polyline]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polyline))
        self.assertEquals(list(reversed(self.points)), list(reversed(self.polyline)))
    
    def test_len(self):
        self.assertTrue(len(self.polyline) == len(self.points))
    
    def test_contains(self):
        self.assertTrue(self.points[0] in self.polyline)
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polyline)
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.polyline)

    def test_get_item(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polyline[i] == self.points[i])
        self.assertTrue(self.polyline[-1] == self.points[-1])
        for i, point in enumerate(self.polyline):
            self.assertTrue(point == self.polyline[i])
        def get_point1():
            self.polyline[len(self.polyline)]
        self.assertRaises(IndexError, get_point1)

    def test_get_slice(self):
        slice = self.polyline[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_extended_slice(self):
        slice = self.polyline[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])
    
    def test_points_iter(self):
        iter(self.polyline.get_points())
        points = [point for point in self.polyline.get_points()]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polyline.get_points()))
    
    def test_arcs_iter(self):
        iter(self.polyline.get_segments())
        arcs = [arc for arc in self.polyline.get_segments()]
        self.assertEquals(len(self.polyline.get_segments()) + 1, len(self.polyline.get_points()))
     
    def test_convert_polyline_to_polygon(self):
        polygon = pygplates.PolygonOnSphere(self.polyline.get_points())
        self.assertEquals(list(polygon.get_points()), list(self.polyline.get_points()))
    
    def test_contains_point(self):
        self.assertTrue(self.points[0] in self.polyline.get_points())
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polyline.get_points())
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.polyline.get_points())
    
    def test_contains_arc(self):
        first_arc = pygplates.GreatCircleArc(self.points[0], self.points[1])
        self.assertTrue(first_arc in self.polyline.get_segments())
        last_arc = pygplates.GreatCircleArc(self.points[-2], self.points[-1])
        self.assertTrue(last_arc in self.polyline.get_segments())

    def test_get_item_point(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polyline.get_points()[i] == self.points[i])
        self.assertTrue(self.polyline.get_points()[-1] == self.points[-1])
        for i, point in enumerate(self.polyline.get_points()):
            self.assertTrue(point == self.polyline.get_points()[i])
        def get_point1():
            self.polyline.get_points()[len(self.polyline.get_points())]
        self.assertRaises(IndexError, get_point1)

    def test_get_item_arc(self):
        for i in range(0, len(self.points)-1):
            arc = pygplates.GreatCircleArc(self.points[i], self.points[i+1])
            self.assertTrue(self.polyline.get_segments()[i] == arc)
        last_arc = pygplates.GreatCircleArc(self.points[-2], self.points[-1])
        self.assertTrue(self.polyline.get_segments()[-1] == last_arc)
        def get_arc1():
            self.polyline.get_segments()[len(self.polyline.get_segments())]
        self.assertRaises(IndexError, get_arc1)

    def test_get_slice_point(self):
        slice = self.polyline.get_points()[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_slice_arc(self):
        slice = self.polyline.get_segments()[0:2]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            arc = pygplates.GreatCircleArc(self.points[i], self.points[i+1])
            self.assertTrue(slice[i] == arc)

    def test_get_extended_slice_point(self):
        slice = self.polyline.get_points()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])

    def test_get_extended_slice_arc(self):
        slice = self.polyline.get_segments()[::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == pygplates.GreatCircleArc(self.points[0], self.points[1]))
        self.assertTrue(slice[1] == pygplates.GreatCircleArc(self.points[2], self.points[3]))
    
    def test_arc_length(self):
        self.assertTrue(isinstance(self.polyline.get_arc_length(), float))
    
    def test_centroid(self):
        self.assertTrue(isinstance(self.polyline.get_centroid(), pygplates.PointOnSphere))
    
    def test_tessellate(self):
        tessellated = self.polyline.to_tessellated(math.radians(91))
        self.assertTrue(tessellated == self.polyline)
        
        tessellated = self.polyline.to_tessellated(math.radians(46))
        self.assertTrue(len(tessellated) == 7)
        self.assertTrue(tessellated[0] == self.points[0])
        self.assertTrue(tessellated[2] == self.points[1])
        self.assertTrue(tessellated[4] == self.points[2])
        self.assertTrue(tessellated[6] == self.points[3])
        self.assertTrue(tessellated[1] == pygplates.FiniteRotation((0,0,1), math.pi / 4) * self.points[0])
        self.assertTrue(tessellated[3] == pygplates.FiniteRotation((1,0,0), math.pi / 4) * self.points[1])
        self.assertTrue(tessellated[5] == pygplates.FiniteRotation((1,0,0), math.pi / 4) * self.points[2])
        
        tessellated = self.polyline.to_tessellated(math.radians(44))
        self.assertTrue(len(tessellated) == 10)
        self.assertTrue(tessellated[0] == self.points[0])
        self.assertTrue(tessellated[3] == self.points[1])
        self.assertTrue(tessellated[6] == self.points[2])
        self.assertTrue(tessellated[9] == self.points[3])
        self.assertTrue(tessellated[1] == pygplates.FiniteRotation((0,0,1), math.pi / 6) * self.points[0])
        self.assertTrue(tessellated[2] == pygplates.FiniteRotation((0,0,1), 2 * math.pi / 6) * self.points[0])
        self.assertTrue(tessellated[4] == pygplates.FiniteRotation((1,0,0), math.pi / 6) * self.points[1])
        self.assertTrue(tessellated[5] == pygplates.FiniteRotation((1,0,0), 2 * math.pi / 6) * self.points[1])
        self.assertTrue(tessellated[7] == pygplates.FiniteRotation((1,0,0), math.pi / 6) * self.points[2])
        self.assertTrue(tessellated[8] == pygplates.FiniteRotation((1,0,0), 2 * math.pi / 6) * self.points[2])


class PolygonOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(0, -1, 0))
        self.polygon = pygplates.PolygonOnSphere(self.points)
        self.polygon_with_interior = pygplates.PolygonOnSphere(
                [(40, 100), (-40, 100), (-40, -100), (40, -100)],  # exterior ring
                [[(30, 120), (30, 150), (-30, 150), (-30, 120)], [(30, 165), (-30, 165), (-30, -165), (30, -165)]])  # 2 interior rings

    def test_construct(self):
        # Need at least three points.
        self.assertRaises(
                pygplates.InvalidPointsForPolygonConstructionError,
                pygplates.PolygonOnSphere,
                [pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(0, 1, 0)])
        lat_lon_tuple_list = [point.to_lat_lon() for point in self.points]
        polygon = pygplates.PolygonOnSphere(lat_lon_tuple_list)
        self.assertTrue(isinstance(polygon, pygplates.PolygonOnSphere))
        xyz_list_list = [list(point.to_xyz()) for point in self.points]
        polygon = pygplates.PolygonOnSphere(xyz_list_list)
        self.assertTrue(isinstance(polygon, pygplates.PolygonOnSphere))
        # Can construct directly from other GeometryOnSphere types.
        self.assertEquals(
                list(pygplates.PolygonOnSphere(pygplates.MultiPointOnSphere(self.points)).get_points()),
                self.points)
        self.assertEquals(
                list(pygplates.PolygonOnSphere(pygplates.PolylineOnSphere(self.points)).get_points()),
                self.points)
        self.assertEquals(
                list(pygplates.PolygonOnSphere(pygplates.PolygonOnSphere(self.points)).get_points()),
                self.points)
    
    def test_convert_geometry(self):
        # Need at least three points.
        self.assertRaises(
                pygplates.InvalidPointsForPolygonConstructionError,
                pygplates.PolygonOnSphere,
                pygplates.PointOnSphere(1, 0, 0),
                False)
        self.assertRaises(
                pygplates.InvalidPointsForPolygonConstructionError,
                pygplates.PolygonOnSphere,
                pygplates.MultiPointOnSphere([pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(0, 1, 0)]),
                False)
        self.assertRaises(
                pygplates.InvalidPointsForPolygonConstructionError,
                pygplates.PolygonOnSphere,
                pygplates.PolylineOnSphere([pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(0, 1, 0)]),
                False)
        # Unless allow one or two points - gets duplicated.
        self.assertTrue(
                pygplates.PolygonOnSphere(pygplates.PointOnSphere((0,0)))
                == pygplates.PolygonOnSphere([(0,0), (0,0), (0,0)]))
        self.assertTrue(
                pygplates.PolygonOnSphere(pygplates.MultiPointOnSphere([(0,0)]))
                == pygplates.PolygonOnSphere([(0,0), (0,0), (0,0)]))
        self.assertTrue(
                pygplates.PolygonOnSphere(pygplates.PolylineOnSphere([(0,0), (0,0)]))
                == pygplates.PolygonOnSphere([(0,0), (0,0), (0,0)]))
        
        # Convert polygon to polygon.
        self.assertTrue(
                pygplates.PolygonOnSphere(pygplates.PointOnSphere(1, 0, 0))
                == pygplates.PolygonOnSphere(
                    pygplates.PolygonOnSphere(pygplates.PointOnSphere(1, 0, 0))))
    
    def test_get_points(self):
        point_sequence = self.polygon.get_points()
        self.assertEquals(self.polygon, pygplates.PolygonOnSphere(point_sequence))
        exterior_ring_point_sequence = self.polygon.get_exterior_ring_points()
        self.assertEquals(self.polygon, pygplates.PolygonOnSphere(exterior_ring_point_sequence))

        exterior_ring = self.polygon_with_interior.get_exterior_ring_points()
        interior_rings = [list(self.polygon_with_interior.get_interior_ring_points(interior_ring_index))
                for interior_ring_index in range(self.polygon_with_interior.get_number_of_interior_rings())]
        self.assertEquals(self.polygon_with_interior, pygplates.PolygonOnSphere(exterior_ring, interior_rings))
    
    def test_get_segments(self):
        point_sequence = self.polygon.get_points()
        segment_sequence = self.polygon.get_segments()
        self.assertEquals(len(point_sequence), len(segment_sequence))
        for index in range(len(point_sequence)):
            self.assertEquals(point_sequence[index], segment_sequence[index].get_start_point())
        
        exterior_ring_point_sequence = self.polygon_with_interior.get_exterior_ring_points()
        exterior_ring_segment_sequence = self.polygon_with_interior.get_exterior_ring_segments()
        self.assertEquals(len(exterior_ring_point_sequence), len(exterior_ring_segment_sequence))
        for index in range(len(exterior_ring_point_sequence)):
            self.assertEquals(exterior_ring_point_sequence[index], exterior_ring_segment_sequence[index].get_start_point())
        
        self.assertTrue(self.polygon_with_interior.get_number_of_interior_rings() == 2)
        for interior_ring_index in range(self.polygon_with_interior.get_number_of_interior_rings()):
            interior_ring_point_sequence = self.polygon_with_interior.get_interior_ring_points(interior_ring_index)
            interior_ring_segment_sequence = self.polygon_with_interior.get_interior_ring_segments(interior_ring_index)
            self.assertEquals(len(interior_ring_point_sequence), len(interior_ring_segment_sequence))
            for index in range(len(interior_ring_point_sequence)):
                self.assertEquals(interior_ring_point_sequence[index], interior_ring_segment_sequence[index].get_start_point())

    def test_compare(self):
        self.assertEquals(self.polygon, pygplates.PolygonOnSphere(self.points))
    
    def test_iter(self):
        iter(self.polygon)
        points = [point for point in self.polygon]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polygon))
        self.assertEquals(list(reversed(self.points)), list(reversed(self.polygon)))

    def test_len(self):
        self.assertTrue(len(self.polygon) == len(self.points))
    
    def test_contains(self):
        self.assertTrue(self.points[0] in self.polygon)
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polygon)
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.polygon)

    def test_get_item(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polygon[i] == self.points[i])
        self.assertTrue(self.polygon[-1] == self.points[-1])
        for i, point in enumerate(self.polygon):
            self.assertTrue(point == self.polygon[i])
        def get_point1():
            self.polygon[len(self.polygon)]
        self.assertRaises(IndexError, get_point1)

    def test_get_slice(self):
        slice = self.polygon[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_extended_slice(self):
        slice = self.polygon[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])
    
    def test_points_iter(self):
        iter(self.polygon.get_points())
        points = [point for point in self.polygon.get_points()]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polygon.get_points()))
     
    def test_convert_polygon_to_polyline(self):
        polyline1 = pygplates.PolylineOnSphere(self.polygon.get_points())
        polyline2 = pygplates.PolylineOnSphere(self.polygon)
		# Note that conversion from polygon to polyline gains an extra vertex (to generate last->first arc).
        self.assertEquals(list(polyline1.get_points())[:-1], list(self.polygon.get_points()))
        self.assertEquals(list(polyline2.get_points())[:-1], list(self.polygon.get_points()))
    
    def test_arcs_iter(self):
        iter(self.polygon.get_segments())
        arcs = [arc for arc in self.polygon.get_segments()]
        self.assertEquals(len(self.polygon.get_segments()), len(self.polygon.get_points()))

        iter(self.polygon.get_exterior_ring_segments())
        arcs = [arc for arc in self.polygon.get_exterior_ring_segments()]
        self.assertEquals(len(self.polygon.get_exterior_ring_segments()), len(self.polygon.get_exterior_ring_points()))

        self.assertTrue(self.polygon.get_number_of_interior_rings() == 0)
   
    def test_contains_point(self):
        self.assertTrue(self.points[0] in self.polygon.get_points())
        self.assertTrue(self.points[0] in self.polygon.get_exterior_ring_points())
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polygon.get_points())
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.polygon.get_points())
    
    def test_contains_arc(self):
        first_arc = pygplates.GreatCircleArc(self.points[0], self.points[1])
        self.assertTrue(first_arc in self.polygon.get_segments())
        self.assertTrue(first_arc in self.polygon.get_exterior_ring_segments())
        # Last arc wraps around for a polygon.
        last_arc = pygplates.GreatCircleArc(self.points[-1], self.points[0])
        self.assertTrue(last_arc in self.polygon.get_segments())
        self.assertTrue(last_arc in self.polygon.get_exterior_ring_segments())

    def test_get_item_point(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polygon.get_points()[i] == self.points[i])
        self.assertTrue(self.polygon.get_points()[-1] == self.points[-1])
        for i, point in enumerate(self.polygon.get_points()):
            self.assertTrue(point == self.polygon.get_points()[i])
        def get_point1():
            self.polygon.get_points()[len(self.polygon.get_points())]
        self.assertRaises(IndexError, get_point1)

    def test_get_item_arc(self):
        for i in range(0, len(self.points)-1):
            arc = pygplates.GreatCircleArc(self.points[i], self.points[i+1])
            self.assertTrue(self.polygon.get_segments()[i] == arc)
        last_arc = pygplates.GreatCircleArc(self.points[-1], self.points[0])
        self.assertTrue(self.polygon.get_segments()[-1] == last_arc)
        def get_arc1():
            self.polygon.get_segments()[len(self.polygon.get_segments())]
        self.assertRaises(IndexError, get_arc1)

    def test_get_slice_point(self):
        slice = self.polygon.get_points()[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_slice_arc(self):
        slice = self.polygon.get_segments()[2:]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == pygplates.GreatCircleArc(self.points[2], self.points[3]))
        # Wrap around arc.
        self.assertTrue(slice[1] == pygplates.GreatCircleArc(self.points[3], self.points[0]))

    def test_get_extended_slice_point(self):
        slice = self.polygon.get_points()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])

    def test_get_extended_slice_arc(self):
        slice = self.polygon.get_segments()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == pygplates.GreatCircleArc(self.points[1], self.points[2]))
        # Wrap around arc.
        self.assertTrue(slice[1] == pygplates.GreatCircleArc(self.points[3], self.points[0]))
    
    def test_arc_length(self):
        arc_length = self.polygon.get_arc_length()
        # Polygon boundary is exactly four times 90 degrees.
        self.assertTrue(arc_length > 2 * math.pi - 1e-6 and arc_length < 2 * math.pi + 1e-6)
    
    def test_area(self):
        area = self.polygon.get_area()
        signed_area = self.polygon.get_signed_area()
        # Polygon covers exactly a quarter of the unit sphere.
        self.assertAlmostEqual(area, math.pi)
        # Clockwise polygon.
        cw_polygon = pygplates.PolygonOnSphere(reversed(self.polygon.get_points()))
        cw_area = cw_polygon.get_area()
        cw_signed_area = cw_polygon.get_signed_area()
        self.assertAlmostEqual(area, cw_area)
        self.assertAlmostEqual(signed_area, -cw_signed_area)

        # Test some zero area dateline sliver polygons that previously gave incorrect areas (as multiples of PI).
        self.assertAlmostEqual(0.0, pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 180)]).get_area())  # previously was 2*PI
        self.assertAlmostEqual(0.0, pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 180), (-5, 180)]).get_area())  # previously was 4*PI
        self.assertAlmostEqual(0.0, pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 180), (-5, 180), (-6, 180)]).get_area())  # previously was 2*PI
        self.assertAlmostEqual(0.0, pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 180), (-5, 180), (-6, 180), (-5, 180)]).get_area())  # previously was 8*PI
        self.assertAlmostEqual(0.0, pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 180), (-5, 180), (-6, 180), (-5, 180), (-3, 180)]).get_area())  # previously was 4*PI
        self.assertAlmostEqual(0.0, pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 180), (-5, 180), (-6, 180), (-5, 180), (-3, 180), (-1, 180)]).get_area())  # previously was 2*PI

        # Test the sign of the area of some almost zero area sliver polygons.
        self.assertTrue(pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, -179.99)]).get_signed_area() > 0)  # Counter-clockwise
        self.assertTrue(pygplates.PolygonOnSphere([(0, 180), (-1, 180), (-3, 179.99)]).get_signed_area() < 0)  # Clockwise

        # Test some polygons that cover half the globe (largest polygon possible).
        #
        # Note: We use 'places=4' because even though polygon is exactly along the equator pygplates doesn't calculate exactly 2*PI.
        #       This is because internally it encounters an arc with antipodal end points and has to adjust slightly to avoid this (thus introducing a small error).
        self.assertAlmostEqual(2 * math.pi, pygplates.PolygonOnSphere([(0, 0), (0, 90), (0, 180), (0, -90)]).get_area(), places=4)  # Neither clockwise nor counter-clockwise (right on equator)
        self.assertAlmostEqual(2 * math.pi, pygplates.PolygonOnSphere([(0, 0), (0, 90), (0, 180), (0.001, -90)]).get_signed_area(), places=3)  # Counter-clockwise (slightly above equator)
        self.assertAlmostEqual(-2 * math.pi, pygplates.PolygonOnSphere([(0, 0), (0, 90), (0, 180), (-0.001, -90)]).get_signed_area(), places=3)  # Clockwise (slightly below equator)
    
    def test_orientation(self):
        self.assertTrue(self.polygon.get_orientation() == pygplates.PolygonOnSphere.Orientation.counter_clockwise)
        self.assertTrue(pygplates.PolygonOnSphere(reversed(self.polygon.get_points())).get_orientation() ==
                pygplates.PolygonOnSphere.Orientation.clockwise)
    
    def test_point_in_polygon(self):
        inv_sqrt_two = 1.0 / math.sqrt(2.0)
        self.assertTrue(self.polygon.is_point_in_polygon(
                pygplates.PointOnSphere(inv_sqrt_two, 0, inv_sqrt_two)))
        self.assertFalse(self.polygon.is_point_in_polygon(
                pygplates.PointOnSphere(-inv_sqrt_two, 0, inv_sqrt_two)))
    
    def test_partition(self):
        inv_sqrt_two = 1.0 / math.sqrt(2.0)
        
        point1 = pygplates.PointOnSphere(inv_sqrt_two, 0, inv_sqrt_two)
        self.assertTrue(self.polygon.partition(point1) == pygplates.PolygonOnSphere.PartitionResult.inside)
        inside_geometries = []
        outside_geometries = []
        self.assertTrue(self.polygon.partition(point1, inside_geometries, outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.inside)
        self.assertTrue(inside_geometries == [point1])
        self.assertFalse(outside_geometries)
        
        point2 = pygplates.PointOnSphere(-inv_sqrt_two, 0, inv_sqrt_two)
        self.assertTrue(self.polygon.partition(point2) == pygplates.PolygonOnSphere.PartitionResult.outside)
        inside_geometries = []
        outside_geometries = []
        self.assertTrue(self.polygon.partition(point2, inside_geometries, outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.outside)
        self.assertFalse(inside_geometries)
        self.assertTrue(outside_geometries == [point2])
        
        multipoint = pygplates.MultiPointOnSphere([point1, point2])
        self.assertTrue(self.polygon.partition(multipoint) == pygplates.PolygonOnSphere.PartitionResult.intersecting)
        inside_geometries = []
        outside_geometries = []
        self.assertTrue(self.polygon.partition(multipoint, inside_geometries, outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.intersecting)
        self.assertTrue(inside_geometries == [pygplates.MultiPointOnSphere([point1])])
        self.assertTrue(outside_geometries == [pygplates.MultiPointOnSphere([point2])])
        inside_geometries = []
        self.assertTrue(self.polygon.partition(multipoint, inside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.intersecting)
        self.assertTrue(inside_geometries == [pygplates.MultiPointOnSphere([point1])])
        outside_geometries = []
        self.assertTrue(self.polygon.partition(multipoint, partitioned_geometries_outside=outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.intersecting)
        self.assertTrue(outside_geometries == [pygplates.MultiPointOnSphere([point2])])
        
        polyline = pygplates.PolylineOnSphere([(10, 0), (-10, 0)])
        self.assertTrue(self.polygon.partition(polyline) == pygplates.PolygonOnSphere.PartitionResult.intersecting)
        inside_geometries = []
        outside_geometries = []
        self.assertTrue(self.polygon.partition(polyline, inside_geometries, outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.intersecting)
        self.assertTrue(inside_geometries == [pygplates.PolylineOnSphere([(10, 0), (0, 0)])] or
                        inside_geometries == [pygplates.PolylineOnSphere([(0, 0), (10, 0)])])
        self.assertTrue(outside_geometries == [pygplates.PolylineOnSphere([(0, 0), (-10, 0)])] or
                        outside_geometries == [pygplates.PolylineOnSphere([(-10, 0), (0, 0)])])
        polyline = pygplates.PolylineOnSphere([(10, 0), (5, 0)])
        inside_geometries = []
        self.assertTrue(self.polygon.partition(polyline, inside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.inside)
        self.assertTrue(inside_geometries == [polyline])
        polyline = pygplates.PolylineOnSphere([(-10, 0), (-5, 0)])
        outside_geometries = []
        self.assertTrue(self.polygon.partition(polyline, partitioned_geometries_outside=outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.outside)
        self.assertTrue(outside_geometries == [polyline])
        
        polygon = pygplates.PolygonOnSphere([(10, 0), (-10, 0), (-10, 5), (10,5)])
        self.assertTrue(self.polygon.partition(polygon) == pygplates.PolygonOnSphere.PartitionResult.intersecting)
        inside_geometries = []
        outside_geometries = []
        self.assertTrue(self.polygon.partition(polygon, inside_geometries, outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.intersecting)
        # Note that *polylines* are returned when intersecting (not polygons) - will be fixed in future.
        # Also we end up with 2 polylines inside (instead of 1).
        self.assertTrue(len(inside_geometries) == 2)
        self.assertTrue(inside_geometries[0] == pygplates.PolylineOnSphere([(10, 0), (0, 0)]) or
                        inside_geometries[0] == pygplates.PolylineOnSphere([(0, 0), (10, 0)]) or
                        inside_geometries[0] == pygplates.PolylineOnSphere([(0, 5), (10, 5), (10, 0)]) or
                        inside_geometries[0] == pygplates.PolylineOnSphere([(10, 0), (10, 5), (0, 5)]))
        self.assertTrue(inside_geometries[1] == pygplates.PolylineOnSphere([(10, 0), (0, 0)]) or
                        inside_geometries[1] == pygplates.PolylineOnSphere([(0, 0), (10, 0)]) or
                        inside_geometries[1] == pygplates.PolylineOnSphere([(0, 5), (10, 5), (10, 0)]) or
                        inside_geometries[1] == pygplates.PolylineOnSphere([(10, 0), (10, 5), (0, 5)]))
        self.assertTrue(len(outside_geometries) == 1)
        self.assertTrue(outside_geometries[0] == pygplates.PolylineOnSphere([(0, 0), (-10, 0), (-10, 5), (0, 5)]) or
                        outside_geometries[0] == pygplates.PolylineOnSphere([(0, 5), (-10, 5), (-10, 0), (0, 0)]))
        polygon = pygplates.PolygonOnSphere([(10, 0), (5, 0), (5, 5), (10,5)])
        inside_geometries = []
        self.assertTrue(self.polygon.partition(polygon, inside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.inside)
        self.assertTrue(inside_geometries == [polygon])
        polygon = pygplates.PolygonOnSphere([(-10, 0), (-5, 0), (-5, 5), (-10,5)])
        outside_geometries = []
        self.assertTrue(self.polygon.partition(polygon, partitioned_geometries_outside=outside_geometries) ==
                pygplates.PolygonOnSphere.PartitionResult.outside)
        self.assertTrue(outside_geometries == [polygon])
    
    def test_centroid(self):
        self.assertTrue(isinstance(self.polygon.get_boundary_centroid(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(self.polygon.get_interior_centroid(), pygplates.PointOnSphere))
        self.assertEquals(self.polygon.get_centroid(), self.polygon.get_interior_centroid())
    
    def test_tessellate(self):
        tessellated = self.polygon.to_tessellated(math.radians(91))
        self.assertTrue(tessellated == self.polygon)
        
        tessellated = self.polygon.to_tessellated(math.radians(46))
        self.assertTrue(len(tessellated) == 8)
        self.assertTrue(tessellated[0] == self.points[0])
        self.assertTrue(tessellated[2] == self.points[1])
        self.assertTrue(tessellated[4] == self.points[2])
        self.assertTrue(tessellated[6] == self.points[3])
        self.assertTrue(tessellated[1] == pygplates.FiniteRotation((0,0,1), math.pi / 4) * self.points[0])
        self.assertTrue(tessellated[3] == pygplates.FiniteRotation((1,0,0), math.pi / 4) * self.points[1])
        self.assertTrue(tessellated[5] == pygplates.FiniteRotation((1,0,0), math.pi / 4) * self.points[2])
        self.assertTrue(tessellated[7] == pygplates.FiniteRotation((0,0,1), math.pi / 4) * self.points[3])
        
        tessellated = self.polygon.to_tessellated(math.radians(44))
        self.assertTrue(len(tessellated) == 12)
        self.assertTrue(tessellated[0] == self.points[0])
        self.assertTrue(tessellated[3] == self.points[1])
        self.assertTrue(tessellated[6] == self.points[2])
        self.assertTrue(tessellated[9] == self.points[3])
        self.assertTrue(tessellated[1] == pygplates.FiniteRotation((0,0,1), math.pi / 6) * self.points[0])
        self.assertTrue(tessellated[2] == pygplates.FiniteRotation((0,0,1), 2 * math.pi / 6) * self.points[0])
        self.assertTrue(tessellated[4] == pygplates.FiniteRotation((1,0,0), math.pi / 6) * self.points[1])
        self.assertTrue(tessellated[5] == pygplates.FiniteRotation((1,0,0), 2 * math.pi / 6) * self.points[1])
        self.assertTrue(tessellated[7] == pygplates.FiniteRotation((1,0,0), math.pi / 6) * self.points[2])
        self.assertTrue(tessellated[8] == pygplates.FiniteRotation((1,0,0), 2 * math.pi / 6) * self.points[2])
        self.assertTrue(tessellated[10] == pygplates.FiniteRotation((0,0,1), math.pi / 6) * self.points[3])
        self.assertTrue(tessellated[11] == pygplates.FiniteRotation((0,0,1), 2 * math.pi / 6) * self.points[3])


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GeometryOnSphereCase,
            MultiPointOnSphereCase,
            PointOnSphereCase,
            PolygonOnSphereCase,
            PolylineOnSphereCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
