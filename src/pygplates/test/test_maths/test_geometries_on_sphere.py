"""
Unit tests for the pygplates geometries on sphere.
"""

import math
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
        # A non-unit length vector raises error.
        self.assertRaises(pygplates.ViolatedUnitVectorInvariantError, pygplates.PointOnSphere, 1, 1, 1)


class MultiPointOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(-1, 0, 0))
        self.multi_point = pygplates.MultiPointOnSphere(self.points)

    def test_construct(self):
        # Need at least one point.
        self.assertRaises(
                pygplates.InsufficientPointsForMultiPointConstructionError,
                pygplates.MultiPointOnSphere,
                [])
    
    def test_compare(self):
        self.assertEquals(self.multi_point, pygplates.MultiPointOnSphere(self.points))
    
    def test_iter(self):
        iter(self.multi_point)
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

    def test_compare(self):
        self.assertEquals(self.polyline, pygplates.PolylineOnSphere(self.points))
    
    def test_points_iter(self):
        iter(self.polyline.get_points_view())
        points = [point for point in self.polyline.get_points_view()]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polyline.get_points_view()))
    
    def test_arcs_iter(self):
        iter(self.polyline.get_great_circle_arcs_view())
        arcs = [arc for arc in self.polyline.get_great_circle_arcs_view()]
        self.assertEquals(len(self.polyline.get_great_circle_arcs_view()) + 1, len(self.polyline.get_points_view()))
     
    def test_convert_polyline_to_polygon(self):
        polygon = pygplates.PolygonOnSphere(self.polyline.get_points_view())
        self.assertEquals(list(polygon.get_points_view()), list(self.polyline.get_points_view()))
    
    def test_contains_point(self):
        self.assertTrue(self.points[0] in self.polyline.get_points_view())
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polyline.get_points_view())
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.polyline.get_points_view())
    
    def test_contains_arc(self):
        first_arc = pygplates.GreatCircleArc(self.points[0], self.points[1])
        self.assertTrue(first_arc in self.polyline.get_great_circle_arcs_view())
        last_arc = pygplates.GreatCircleArc(self.points[-2], self.points[-1])
        self.assertTrue(last_arc in self.polyline.get_great_circle_arcs_view())

    def test_get_item_point(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polyline.get_points_view()[i] == self.points[i])
        self.assertTrue(self.polyline.get_points_view()[-1] == self.points[-1])
        for i, point in enumerate(self.polyline.get_points_view()):
            self.assertTrue(point == self.polyline.get_points_view()[i])
        def get_point1():
            self.polyline.get_points_view()[len(self.polyline.get_points_view())]
        self.assertRaises(IndexError, get_point1)

    def test_get_item_arc(self):
        for i in range(0, len(self.points)-1):
            arc = pygplates.GreatCircleArc(self.points[i], self.points[i+1])
            self.assertTrue(self.polyline.get_great_circle_arcs_view()[i] == arc)
        last_arc = pygplates.GreatCircleArc(self.points[-2], self.points[-1])
        self.assertTrue(self.polyline.get_great_circle_arcs_view()[-1] == last_arc)
        def get_arc1():
            self.polyline.get_great_circle_arcs_view()[len(self.polyline.get_great_circle_arcs_view())]
        self.assertRaises(IndexError, get_arc1)

    def test_get_slice_point(self):
        slice = self.polyline.get_points_view()[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_slice_arc(self):
        slice = self.polyline.get_great_circle_arcs_view()[0:2]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            arc = pygplates.GreatCircleArc(self.points[i], self.points[i+1])
            self.assertTrue(slice[i] == arc)

    def test_get_extended_slice_point(self):
        slice = self.polyline.get_points_view()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])

    def test_get_extended_slice_arc(self):
        slice = self.polyline.get_great_circle_arcs_view()[::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == pygplates.GreatCircleArc(self.points[0], self.points[1]))
        self.assertTrue(slice[1] == pygplates.GreatCircleArc(self.points[2], self.points[3]))
    
    def test_arc_length(self):
        self.assertTrue(isinstance(self.polyline.get_arc_length(), float))
    
    def test_centroid(self):
        self.assertTrue(isinstance(self.polyline.get_centroid(), pygplates.PointOnSphere))


class PolygonOnSphereCase(unittest.TestCase):
    def setUp(self):
        self.points = []
        self.points.append(pygplates.PointOnSphere(1, 0, 0))
        self.points.append(pygplates.PointOnSphere(0, 1, 0))
        self.points.append(pygplates.PointOnSphere(0, 0, 1))
        self.points.append(pygplates.PointOnSphere(0, -1, 0))
        self.polygon = pygplates.PolygonOnSphere(self.points)

    def test_construct(self):
        # Need at least three points.
        self.assertRaises(
                pygplates.InvalidPointsForPolygonConstructionError,
                pygplates.PolygonOnSphere,
                [pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(0, 1, 0)])

    def test_compare(self):
        self.assertEquals(self.polygon, pygplates.PolygonOnSphere(self.points))
    
    def test_points_iter(self):
        iter(self.polygon.get_points_view())
        points = [point for point in self.polygon.get_points_view()]
        self.assertEquals(self.points, points)
        self.assertEquals(self.points, list(self.polygon.get_points_view()))
     
    def test_convert_polygon_to_polyline(self):
        polyline = pygplates.PolylineOnSphere(self.polygon.get_points_view())
        self.assertEquals(list(polyline.get_points_view()), list(self.polygon.get_points_view()))
    
    def test_arcs_iter(self):
        iter(self.polygon.get_great_circle_arcs_view())
        arcs = [arc for arc in self.polygon.get_great_circle_arcs_view()]
        self.assertEquals(len(self.polygon.get_great_circle_arcs_view()), len(self.polygon.get_points_view()))
   
    def test_contains_point(self):
        self.assertTrue(self.points[0] in self.polygon.get_points_view())
        self.assertTrue(pygplates.PointOnSphere(1, 0, 0) in self.polygon.get_points_view())
        self.assertTrue(pygplates.PointOnSphere(0, 0, -1) not in self.polygon.get_points_view())
    
    def test_contains_arc(self):
        first_arc = pygplates.GreatCircleArc(self.points[0], self.points[1])
        self.assertTrue(first_arc in self.polygon.get_great_circle_arcs_view())
        # Last arc wraps around for a polygon.
        last_arc = pygplates.GreatCircleArc(self.points[-1], self.points[0])
        self.assertTrue(last_arc in self.polygon.get_great_circle_arcs_view())

    def test_get_item_point(self):
        for i in range(0, len(self.points)):
            self.assertTrue(self.polygon.get_points_view()[i] == self.points[i])
        self.assertTrue(self.polygon.get_points_view()[-1] == self.points[-1])
        for i, point in enumerate(self.polygon.get_points_view()):
            self.assertTrue(point == self.polygon.get_points_view()[i])
        def get_point1():
            self.polygon.get_points_view()[len(self.polygon.get_points_view())]
        self.assertRaises(IndexError, get_point1)

    def test_get_item_arc(self):
        for i in range(0, len(self.points)-1):
            arc = pygplates.GreatCircleArc(self.points[i], self.points[i+1])
            self.assertTrue(self.polygon.get_great_circle_arcs_view()[i] == arc)
        last_arc = pygplates.GreatCircleArc(self.points[-1], self.points[0])
        self.assertTrue(self.polygon.get_great_circle_arcs_view()[-1] == last_arc)
        def get_arc1():
            self.polygon.get_great_circle_arcs_view()[len(self.polygon.get_great_circle_arcs_view())]
        self.assertRaises(IndexError, get_arc1)

    def test_get_slice_point(self):
        slice = self.polygon.get_points_view()[1:3]
        self.assertTrue(len(slice) == 2)
        for i in range(0, len(slice)):
            self.assertTrue(slice[i] == self.points[i+1])

    def test_get_slice_arc(self):
        slice = self.polygon.get_great_circle_arcs_view()[2:]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == pygplates.GreatCircleArc(self.points[2], self.points[3]))
        # Wrap around arc.
        self.assertTrue(slice[1] == pygplates.GreatCircleArc(self.points[3], self.points[0]))

    def test_get_extended_slice_point(self):
        slice = self.polygon.get_points_view()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.points[1])
        self.assertTrue(slice[1] == self.points[3])

    def test_get_extended_slice_arc(self):
        slice = self.polygon.get_great_circle_arcs_view()[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == pygplates.GreatCircleArc(self.points[1], self.points[2]))
        # Wrap around arc.
        self.assertTrue(slice[1] == pygplates.GreatCircleArc(self.points[3], self.points[0]))
    
    def test_arc_length(self):
        self.assertTrue(isinstance(self.polygon.get_arc_length(), float))
    
    def test_area(self):
        area = self.polygon.get_area()
        signed_area = self.polygon.get_signed_area()
        # Polygon covers exactly a quarter of the unit sphere.
        self.assertTrue(area > math.pi - 1e-6 and area < math.pi + 1e-6)
        # Counter-clockwise polygon.
        ccw_polygon = pygplates.PolygonOnSphere(reversed(self.polygon.get_points_view()))
        ccw_area = ccw_polygon.get_area()
        ccw_signed_area = ccw_polygon.get_signed_area()
        self.assertTrue(area > ccw_area - 1e-6 and area < ccw_area + 1e-6)
        self.assertTrue(signed_area > ccw_signed_area - 1e-6 and signed_area < -ccw_signed_area + 1e-6)
    
    def test_orientation(self):
        self.assertTrue(self.polygon.get_orientation() == pygplates.PolygonOnSphere.Orientation.counter_clockwise)
        self.assertTrue(pygplates.PolygonOnSphere(reversed(self.polygon.get_points_view())).get_orientation() ==
                pygplates.PolygonOnSphere.Orientation.clockwise)
    
    def test_point_in_polygon(self):
        inv_sqrt_two = 1.0 / math.sqrt(2.0)
        self.assertTrue(self.polygon.is_point_in_polygon(
                pygplates.PointOnSphere(inv_sqrt_two, 0, inv_sqrt_two)))
        self.assertFalse(self.polygon.is_point_in_polygon(
                pygplates.PointOnSphere(-inv_sqrt_two, 0, inv_sqrt_two)))
    
    def test_centroid(self):
        self.assertTrue(isinstance(self.polygon.get_boundary_centroid(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(self.polygon.get_interior_centroid(), pygplates.PointOnSphere))


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
