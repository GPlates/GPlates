"""
Unit tests for the pygplates geometries on sphere.
"""

import math
import os
import random
import unittest
import pickle
from fractions import Fraction
import pygplates
# Test using numpy if it's available...
try:
    import numpy
    imported_numpy = True
except ImportError:
    imported_numpy = False

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


#
# Brute-force distances, independent of pyGPlates' distance code, for 'test_distance_against_brute_force'.
#

def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])

def _normalise(a):
    length = math.sqrt(_dot(a, a))
    return (a[0] / length, a[1] / length, a[2] / length)

def _angle(a, b):
    # More accurate than acos for small (and near pi) angles.
    return math.atan2(math.sqrt(_dot(_cross(a, b), _cross(a, b))), _dot(a, b))

def _point_to_arc(p, a, b):
    if _angle(a, b) < 1e-5:
        # A short arc's plane is too inaccurate (its normal is a tiny cross product), so find the closest
        # point on its chord instead, and project that onto the sphere (onto the arc).
        chord = tuple(b[i] - a[i] for i in range(3))
        chord_length_squared = _dot(chord, chord)
        f = 0.0 if chord_length_squared == 0 else max(0.0, min(1.0, _dot(tuple(p[i] - a[i] for i in range(3)), chord) / chord_length_squared))
        return _angle(p, _normalise(tuple(a[i] + f * chord[i] for i in range(3))))
    normal = _normalise(_cross(a, b))
    projected = tuple(p[i] - _dot(p, normal) * normal[i] for i in range(3))
    if _dot(projected, projected) > 1e-24:
        q = _normalise(projected)
        if _dot(_cross(a, q), normal) >= 0 and _dot(_cross(q, b), normal) >= 0:
            return _angle(p, q)
    return min(_angle(p, a), _angle(p, b))

def _arcs_intersect(a1, b1, a2, b2):
    line = _cross(_cross(a1, b1), _cross(a2, b2))
    if _dot(line, line) < 1e-24:
        return False  # on the same great circle (or zero length) - overlap is found by the end points
    line = _normalise(line)
    for c in (line, (-line[0], -line[1], -line[2])):
        if all(_dot(_cross(a, c), _cross(a, b)) >= 0 and _dot(_cross(c, b), _cross(a, b)) >= 0
               for a, b in ((a1, b1), (a2, b2))):
            return True
    return False

def _arcs(geometry):
    if isinstance(geometry, (pygplates.PointOnSphere, pygplates.MultiPointOnSphere)):
        return [(point.to_xyz(), point.to_xyz()) for point in geometry.get_points()]
    # Includes a polygon's interior rings.
    return [(segment.get_start_point().to_xyz(), segment.get_end_point().to_xyz())
            for segment in geometry.get_segments()]

def _brute_force_distance(geometry1, geometry2, solid1, solid2):
    # Inside a solid polygon is zero distance. If the outlines do not intersect (found below) then each
    # ring (or polyline) is entirely inside or outside, so testing one vertex of each is enough.
    # Inside is decided by the brute-force point-in-polygon, without the point-on-outline threshold (as the
    # distance code does). A vertex within the band of the outline (either answer) is not inside: its distance
    # to the outline (below) is within the band anyway.
    rng = random.Random(0)
    for polygon, solid, other in ((geometry1, solid1, geometry2), (geometry2, solid2, geometry1)):
        if not (solid and isinstance(polygon, pygplates.PolygonOnSphere)):
            continue
        if isinstance(other, pygplates.PolygonOnSphere):
            points = [other.get_exterior_ring_points()[0]] + [
                other.get_interior_ring_points(ring)[0] for ring in range(other.get_number_of_interior_rings())]
        elif isinstance(other, pygplates.PolylineOnSphere):
            points = [other.get_points()[0]]
        else:
            points = list(other.get_points())
        if any(_brute_force_point_in_polygon(polygon, point, rng, use_point_on_polygon_threshold=False)
               for point in points):
            return 0.0
    distance = math.pi
    for a1, b1 in _arcs(geometry1):
        for a2, b2 in _arcs(geometry2):
            if _arcs_intersect(a1, b1, a2, b2):
                return 0.0
            distance = min(distance,
                _point_to_arc(a1, a2, b2), _point_to_arc(b1, a2, b2),
                _point_to_arc(a2, a1, b1), _point_to_arc(b2, a1, b1))
    return distance

def _distance_to_indexed(geometry, index, p):
    # Distance from 'p' to the point (or segment) of 'geometry' at 'index'.
    if isinstance(geometry, pygplates.PointOnSphere):
        return _angle(p, geometry.to_xyz()) if index == 0 else math.pi
    if isinstance(geometry, pygplates.MultiPointOnSphere):
        return _angle(p, geometry[index].to_xyz())
    segment = geometry.get_segments()[index]
    return _point_to_arc(p, segment.get_start_point().to_xyz(), segment.get_end_point().to_xyz())

def _random_unit_vector(rng):
    return _normalise((rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1)))

def _random_point(rng, near, spread):
    return pygplates.PointOnSphere(_normalise(tuple(near[i] + rng.gauss(0, spread) for i in range(3))))

def _random_geometry(rng, centre, scale=1.0, duplicates=False):
    # Clustered around 'centre' (within about 20 degrees, times 'scale').
    # With 'duplicates', polylines and polygons get duplicate consecutive points (zero-length segments).
    geometry_type = rng.choice(('point', 'multipoint', 'polyline', 'polyline', 'polygon', 'polygon'))
    if geometry_type == 'point':
        return _random_point(rng, centre, 0.2 * scale)
    if geometry_type == 'multipoint':
        return pygplates.MultiPointOnSphere(
            [_random_point(rng, centre, 0.2 * scale) for _ in range(rng.randint(1, 30))])
    # A random walk, so that segments are short and the bounding tree has spatially coherent nodes.
    points = [_random_point(rng, centre, 0.15 * scale)]
    for _ in range(rng.randint(10, 30) - 1):
        points.append(_random_point(rng, points[-1].to_xyz(), 0.04 * scale))
        if duplicates and rng.random() < 0.2:
            points.append(points[-1])
    if geometry_type == 'polyline':
        return pygplates.PolylineOnSphere(points)
    return pygplates.PolygonOnSphere(points)

def _random_ring(rng, centre, radius, num_points):
    # A roughly circular ring of points about 'radius' radians from 'centre'.
    u = _normalise(_cross(centre, (0.3, 0.5, 0.8) if abs(centre[2]) < 0.9 else (1.0, 0.0, 0.0)))
    v = _cross(centre, u)
    points = []
    for k in range(num_points):
        angle = 2 * math.pi * k / num_points
        r = radius * rng.uniform(0.8, 1.2)
        points.append(pygplates.PointOnSphere(_normalise(tuple(
            math.cos(r) * centre[i] + math.sin(r) * (math.cos(angle) * u[i] + math.sin(angle) * v[i])
            for i in range(3)))))
    return points

def _random_polygon_with_interior_rings(rng, centre):
    interior_rings = [
        _random_ring(rng, _random_point(rng, centre, 0.05).to_xyz(), rng.uniform(0.02, 0.08), rng.randint(5, 20))
        for _ in range(rng.randint(1, 3))]
    return pygplates.PolygonOnSphere(_random_ring(rng, centre, 0.3, rng.randint(20, 40)), interior_rings)

def _random_global_geometry(rng):
    # Spanning more than a hemisphere.
    geometry_type = rng.choice(('multipoint', 'polyline', 'polygon', 'ring'))
    if geometry_type == 'ring':
        return pygplates.PolygonOnSphere(
            _random_ring(rng, _random_unit_vector(rng), rng.uniform(1.7, 2.8), rng.randint(10, 60)))
    points = [pygplates.PointOnSphere(_random_unit_vector(rng)) for _ in range(rng.randint(3, 12))]
    if geometry_type == 'multipoint':
        return pygplates.MultiPointOnSphere(points)
    # Leave out a point (nearly) antipodal to the one before it, which cannot form a segment.
    kept = [points[0]]
    for point in points[1:]:
        if _dot(point.to_xyz(), kept[-1].to_xyz()) > -0.999:
            kept.append(point)
    if len(kept) < 2:
        kept.append(_random_point(rng, kept[0].to_xyz(), 0.1))
    if geometry_type == 'polyline' or len(kept) < 3:
        return pygplates.PolylineOnSphere(kept)
    return pygplates.PolygonOnSphere(kept)


#
# Brute-force point-in-polygon, independent of pyGPlates' point-in-polygon code, for
# 'test_point_in_polygon_against_brute_force'.
#
# The same definition (crossing parity over all rings, from the antipode R of the boundary centroid),
# but with exact orientation predicates, and a path from R to the point through a random waypoint (so
# that neither arc is nearly antipodal or nearly zero length).
#

# The point-on-outline threshold (about 1.414e-6 radians): a point within this distance of the outline is
# inside, beyond the band either answer is accepted, and beyond the band parity decides.
_ON_OUTLINE_DISTANCE = 1.3e-6
_ON_OUTLINE_BAND = 1.5e-6

def _orient(a, b, c):
    # Exact sign of det[a, b, c] for the double inputs: a float filter, then exact rationals.
    d = _dot(a, _cross(b, c))
    if d > 1e-14:
        return 1
    if d < -1e-14:
        return -1
    fa, fb, fc = [Fraction(x) for x in a], [Fraction(x) for x in b], [Fraction(x) for x in c]
    d = (fa[0] * (fb[1] * fc[2] - fb[2] * fc[1]) + fa[1] * (fb[2] * fc[0] - fb[0] * fc[2]) +
         fa[2] * (fb[0] * fc[1] - fb[1] * fc[0]))
    return (d > 0) - (d < 0)

class _Reroute(Exception):
    pass

def _path_arc_crosses_edge(a, b, c, d):
    # Whether arc ab crosses edge cd (both shorter than pi), by S2's CrossingSign rule. A zero orientation
    # (the path touches a vertex or an edge's great circle) reroutes the path. The vertices are perturbed (see
    # below), so a zero can only involve the path's end points, R and the test point, which are off the outline.
    acb = -_orient(a, b, c)
    bda = _orient(a, b, d)
    cbd = -_orient(c, d, b)
    dac = _orient(c, d, a)
    if 0 in (acb, bda, cbd, dac):
        raise _Reroute()
    return acb == bda == cbd == dac

def _brute_force_point_in_polygon(polygon, point, rng, use_point_on_polygon_threshold=True):
    # Returns True or False, or None if within the band of the outline (where either answer is accepted).
    # Without the point-on-outline threshold the band extends to the outline.
    rings = [[p.to_xyz() for p in polygon.get_exterior_ring_points()]] + [
        [p.to_xyz() for p in polygon.get_interior_ring_points(ring)]
        for ring in range(polygon.get_number_of_interior_rings())]
    edges = [(ring[i], ring[(i + 1) % len(ring)]) for ring in rings for i in range(len(ring))]
    p = point.to_xyz()
    distance = min(min(_point_to_arc(p, a, b) for a, b in edges), min(_angle(p, v) for ring in rings for v in ring))
    if use_point_on_polygon_threshold and distance < _ON_OUTLINE_DISTANCE:
        return True
    if distance < _ON_OUTLINE_BAND:
        return None
    # Each vertex is moved by about 1e-12 (the same move wherever it appears), so no orientation involving a
    # vertex is exactly zero (eg, a vertex antipodal to the test point lies in the plane of every path arc ending
    # there). R and the test point are further than the band from the outline, so parity cannot change.
    jitter_rng = random.Random(1)
    moves = {}
    for v in (v for ring in rings for v in ring):
        if v not in moves:
            moves[v] = tuple(jitter_rng.uniform(-1e-12, 1e-12) for _ in range(3))
    jittered = [[tuple(v[k] + moves[v][k] for k in range(3)) for v in ring] for ring in rings]
    jittered_edges = [(ring[i], ring[(i + 1) % len(ring)]) for ring in jittered for i in range(len(ring))]
    r = tuple(-x for x in polygon.get_boundary_centroid().to_xyz())
    while True:
        w = _random_unit_vector(rng)
        if not (0.05 < _angle(r, w) < math.pi - 0.2 and 0.05 < _angle(w, p) < math.pi - 0.2):
            continue
        try:
            crossings = sum(_path_arc_crosses_edge(r, w, c, d) + _path_arc_crosses_edge(w, p, c, d)
                            for c, d in jittered_edges if c != d)
            return crossings % 2 == 1
        except _Reroute:
            continue

def _is_point_in_polygon_at_each_speed(polygon, point):
    # pyGPlates' answer from a freshly created copy of 'polygon' (no set-up), a copy queried 4 times first
    # (bounds) and one queried 200 times first (bounds and, for more than 16 segments, a tree of lunes).
    def copy():
        return pygplates.PolygonOnSphere(polygon.get_exterior_ring_points(), [
            polygon.get_interior_ring_points(ring) for ring in range(polygon.get_number_of_interior_rings())])
    answers = []
    for num_previous_queries in (0, 4, 200):
        polygon_copy = copy()
        for _ in range(num_previous_queries):
            polygon_copy.is_point_in_polygon(pygplates.PointOnSphere.north_pole)
        answers.append(polygon_copy.is_point_in_polygon(point))
    return answers


class GeometryOnSphereCase(unittest.TestCase):
    def test_clone(self):
        point = pygplates.PointOnSphere(1, 0, 0)
        self.assertEqual(point.clone(), point)
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
        # Ensure exception not raised when distance threshold outside range [0, PI].
        try:
            self.assertTrue(pygplates.GeometryOnSphere.distance(polyline1, polyline2, -0.1) is None)  # threshold clamped to zero
            self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(polyline1, polyline2, math.pi + 0.1), math.pi / 19)  # threshold clamped to PI
        except:
            self.fail("Unexpected exception when distance threshold outside range [0, PI]")
        
        # Read some test data.
        # The distances were determined using the Measure canvas tool in GPlates.
        features = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'geometry_distance.gpml'))
        features = list(features)
        self.assertTrue(len(features) == 3)
        
        # 'features[0]' is a polygon.
        distance, closest_point1, closest_point2 = pygplates.GeometryOnSphere.distance(
                features[0].get_geometry(), features[1].get_geometry(),
                return_closest_positions=True, geometry_is_solid=True)
        # One of the points of the multi-point is inside the (solid) polygon.
        self.assertAlmostEqual(distance, 0)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(closest_point1, closest_point2), 0.03407617)
        
        distance, closest_point1, closest_point2 = pygplates.GeometryOnSphere.distance(
                features[0].get_geometry(), features[2].get_geometry(),
                return_closest_positions=True, geometry_is_solid=True)
        # The polyline intersects the polygon.
        self.assertAlmostEqual(distance, 0)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(closest_point1, closest_point2), 0)

        # Test two polylines that intersect each other *twice*.
        (
            distance,
            polyline1_intersection,
            polyline2_intersection,
            polyline1_segment_index,
            polyline2_segment_index,
        ) = pygplates.GeometryOnSphere.distance(
            pygplates.PolylineOnSphere([(5,-5), (5,5), (-5,5)]),
            pygplates.PolylineOnSphere([(10,0), (0,0), (0,10)]),
            return_closest_positions=True,
            return_closest_indices=True)
        self.assertTrue(distance == 0.0)  # can now compare directly with zero
        self.assertTrue(polyline1_intersection == polyline2_intersection)  # should be same intersection position
        # The returned intersection will be one of the two (arbitrarily).
        # Ensure that the intersection point and the segment indices are consistent
        # (there was a bug in pyGPlates 1.0 where the segment indices were for the first intersection but the intersection point was the second intersection).
        self.assertTrue(
            (pygplates.GeometryOnSphere.distance(polyline1_intersection, pygplates.PointOnSphere(5,0)) < math.radians(0.5) and polyline1_segment_index == 0 and polyline2_segment_index == 0) or
            (pygplates.GeometryOnSphere.distance(polyline1_intersection, pygplates.PointOnSphere(0,5)) < math.radians(0.5) and polyline1_segment_index == 1 and polyline2_segment_index == 1))

    def test_distance_call_forms(self):
        point = pygplates.PointOnSphere(0, 0)
        polygon = pygplates.PolygonOnSphere([(-5, 5), (5, 5), (5, 10), (-5, 10)])
        distance = math.radians(5)
        # Called on a geometry, and (as when it was a static method) on the class, including a derived class.
        self.assertAlmostEqual(point.distance(polygon), distance)
        self.assertAlmostEqual(polygon.distance(point), distance)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(point, polygon), distance)
        self.assertAlmostEqual(pygplates.PointOnSphere.distance(point, polygon), distance)
        # Keyword arguments, including for the geometry that it's called on (an undocumented keyword,
        # but the binding must name it: see 'export_geometry_on_sphere()').
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(geometry=point, other_geometry=polygon), distance)
        self.assertAlmostEqual(point.distance(other_geometry=polygon), distance)
        inside_point = pygplates.PointOnSphere(0, 7)
        self.assertTrue(polygon.distance(inside_point) > 0)
        self.assertEqual(polygon.distance(inside_point, geometry_is_solid=True), 0.0)
        self.assertEqual(inside_point.distance(polygon, other_geometry_is_solid=True), 0.0)
        # The argument names it had as a static method still work...
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(geometry1=point, geometry2=polygon), distance)
        self.assertAlmostEqual(point.distance(geometry2=polygon), distance)
        self.assertEqual(pygplates.GeometryOnSphere.distance(polygon, inside_point, geometry1_is_solid=True), 0.0)
        self.assertEqual(pygplates.GeometryOnSphere.distance(inside_point, polygon, geometry2_is_solid=True), 0.0)
        # ...but cannot be mixed with the new names.
        self.assertRaises(
                TypeError,  # Boost.Python.ArgumentError
                pygplates.GeometryOnSphere.distance, polygon, inside_point, geometry_is_solid=True, geometry2_is_solid=True)

    def test_distance_antipodal(self):
        # Geometries exactly pi apart: each geometry type collapsed onto one of two antipodal points
        # (only such degenerate geometries can be exactly pi apart).
        # There was a bug in pyGPlates 1.0 where a threshold of pi counted as exceeded, and where the
        # closest indices were never set (uninitialised) for multi-points and polylines.
        north = pygplates.PointOnSphere(0, 0, 1)
        south = pygplates.PointOnSphere(0, 0, -1)
        def geometries_at(point):
            return (
                point,
                pygplates.MultiPointOnSphere([point, point]),
                pygplates.PolylineOnSphere([point, point]),
                pygplates.PolygonOnSphere([point, point, point]))
        for geometry1 in geometries_at(north):
            for geometry2 in geometries_at(south):
                for solid in (False, True):
                    for g1, g2, p1, p2 in ((geometry1, geometry2, north, south), (geometry2, geometry1, south, north)):
                        msg = '{} to {}, solid={}'.format(type(g1).__name__, type(g2).__name__, solid)
                        def distance(*args, **kwargs):
                            return pygplates.GeometryOnSphere.distance(
                                g1, g2, *args, geometry_is_solid=solid, other_geometry_is_solid=solid, **kwargs)
                        self.assertAlmostEqual(distance(), math.pi, msg=msg)
                        self.assertAlmostEqual(distance(math.pi), math.pi, msg=msg)
                        self.assertTrue(distance(math.pi - 1e-6) is None, msg=msg)
                        for threshold in (None, math.pi):
                            distance_, closest_point1, closest_point2, closest_index1, closest_index2 = distance(
                                threshold, return_closest_positions=True, return_closest_indices=True)
                            self.assertAlmostEqual(distance_, math.pi, msg=msg)
                            self.assertTrue(closest_point1 == p1, msg=msg)
                            self.assertTrue(closest_point2 == p2, msg=msg)
                            self.assertTrue(closest_index1 == 0, msg=msg)
                            self.assertTrue(closest_index2 == 0, msg=msg)
                            distance_, closest_point1, closest_point2 = distance(threshold, return_closest_positions=True)
                            self.assertTrue(closest_point1 == p1, msg=msg)
                            self.assertTrue(closest_point2 == p2, msg=msg)
                            distance_, closest_index1, closest_index2 = distance(threshold, return_closest_indices=True)
                            self.assertTrue(closest_index1 == 0 and closest_index2 == 0, msg=msg)

    def test_distance_threshold_zero(self):
        # Intersecting geometries are zero distance apart, which does not exceed a threshold of zero.
        # There was a bug in pyGPlates 1.0 where a threshold of zero (or within epsilon of zero)
        # ignored the intersection of two polylines/polygons and returned None.
        polyline = pygplates.PolylineOnSphere([(0, -10), (0, 10)])
        polygon = pygplates.PolygonOnSphere([(-10, 0), (10, 0), (10, 5)])
        for geometry1, geometry2 in ((polyline, polygon), (polygon, polyline), (polygon, polygon)):
            for threshold in (0.0, 1e-9):
                self.assertTrue(pygplates.GeometryOnSphere.distance(geometry1, geometry2, threshold) == 0.0)
        self.assertTrue(pygplates.GeometryOnSphere.distance(polyline, pygplates.PointOnSphere(0, 0), 0.0) == 0.0)

    def test_distance_closest_positions_match_indices(self):
        # Two candidates equally close: the closest positions and indices must come from the same one.
        # There was a bug in pyGPlates 1.0 where, for a multi-point and a polyline, they could come
        # from different candidates.
        multipoint = pygplates.MultiPointOnSphere([(10, 0), (-10, 0)])
        polyline = pygplates.PolylineOnSphere([(10, -5), (10, 5), (20, 5), (-20, 5), (-10, 5), (-10, -5)])
        distance, closest_point1, closest_point2, closest_index1, closest_index2 = pygplates.GeometryOnSphere.distance(
            multipoint, polyline, return_closest_positions=True, return_closest_indices=True)
        self.assertTrue(closest_point1 == multipoint[closest_index1])
        closest_segment = polyline.get_segments()[closest_index2]
        self.assertAlmostEqual(
            pygplates.GeometryOnSphere.distance(
                closest_point2,
                pygplates.PolylineOnSphere([closest_segment.get_start_point(), closest_segment.get_end_point()])),
            0)
        self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(closest_point1, closest_point2), distance)

        # An exact tie (mirror images in x) between two multi-points.
        multipoint1 = pygplates.MultiPointOnSphere([(1, 0, 0), (-1, 0, 0)])
        multipoint2 = pygplates.MultiPointOnSphere([(0.6, 0.8, 0), (-0.6, 0.8, 0)])
        for mp1, mp2 in ((multipoint1, multipoint2), (multipoint2, multipoint1)):
            distance, closest_point1, closest_point2, closest_index1, closest_index2 = pygplates.GeometryOnSphere.distance(
                mp1, mp2, return_closest_positions=True, return_closest_indices=True)
            self.assertTrue(closest_point1 == mp1[closest_index1])
            self.assertTrue(closest_point2 == mp2[closest_index2])
            self.assertAlmostEqual(pygplates.GeometryOnSphere.distance(closest_point1, closest_point2), distance)

    def test_distance_threshold_is_exact(self):
        # A distance is None only if the distance that would be returned is greater than the threshold.
        # There was a bug in pyGPlates 1.0 where a distance passed back as the threshold could be
        # excluded by itself (the threshold in radians and the distance had different cosines).
        D = pygplates.GeometryOnSphere.distance
        point = pygplates.PointOnSphere(0, 0)
        geometries = (
            pygplates.PointOnSphere(30, 60),
            pygplates.MultiPointOnSphere([(30, 60), (-20, 70)]),
            pygplates.PolylineOnSphere([(30, 60), (-20, 70), (10, 100)]),
            pygplates.PolygonOnSphere([(30, 60), (-20, 70), (10, 100)]))
        for geometry in geometries:
            for g1, g2 in ((point, geometry), (geometry, point)):
                distance = D(g1, g2)
                self.assertTrue(D(g1, g2, distance) == distance)
                self.assertTrue(D(g1, g2, math.nextafter(distance, 0)) is None)
        # Perpendicular points.
        self.assertTrue(D(pygplates.PointOnSphere(1, 0, 0), pygplates.PointOnSphere(0, 1, 0), math.pi / 2) == math.pi / 2)
        # A distance within epsilon of zero is returned as zero, so it does not exceed a threshold of zero.
        point1 = pygplates.PointOnSphere(0, 0)
        point2 = pygplates.PointOnSphere(0, 1e-6)
        self.assertTrue(D(point1, point2) == 0.0)
        self.assertTrue(D(point1, point2, 0.0) == 0.0)
        # A NaN threshold raises ValueError (it was ignored in pyGPlates 1.0).
        self.assertRaises(ValueError, D, point1, point2, float('nan'))

    def test_distance_to_zero_length_segment(self):
        # A segment shorter than about 1e-6 radians counts as zero length. Its closer end point is
        # measured: there was a bug in pyGPlates 1.0 where only its start point was, so the end point of
        # a polyline's last segment was never measured.
        D = pygplates.GeometryOnSphere.distance
        start = pygplates.PointOnSphere(0, 10)
        end = pygplates.PointOnSphere(0, 10 + 1e-5)  # 1.7e-7 radians further east
        point = pygplates.PointOnSphere(0, 11)
        polyline = pygplates.PolylineOnSphere([pygplates.PointOnSphere(0, 9), start, end])
        distance, _, closest_point, _, closest_index = D(
            point, polyline, return_closest_positions=True, return_closest_indices=True)
        self.assertAlmostEqual(distance, D(point, end), places=12)
        self.assertTrue(closest_point == end)
        self.assertTrue(closest_index == 1)

    def test_distance_independent_of_returned_positions(self):
        # Requesting the closest positions must not change the distance.
        # There was a bug in pyGPlates 1.0 where it could differ in the last bits.
        D = pygplates.GeometryOnSphere.distance
        point = pygplates.PointOnSphere(1.0, 1.234)
        polyline = pygplates.PolylineOnSphere([(0, -10), (0, 10)])
        self.assertTrue(D(point, polyline) == D(point, polyline, return_closest_positions=True)[0])

    def test_distance_against_brute_force(self):
        # Compare with a brute-force calculation (every pair of segments) for random geometries large
        # enough to exercise the bounding-tree search (a leaf bounds up to 8 segments), clustered so that
        # many segments are nearly as close as the closest, with random solid flags and thresholds.
        # Some cases are special: tiny geometries near each other's antipode (distances near PI),
        # geometries spanning more than a hemisphere, polygons with interior rings, and duplicate
        # consecutive points (zero-length segments).
        rng = random.Random(1234)
        D = pygplates.GeometryOnSphere.distance
        for case in range(200):
            centre = _random_unit_vector(rng)
            kind = case % 5
            if kind == 1:
                scale = rng.choice((1e-3, 1e-5))
                geometry1 = _random_geometry(rng, centre, scale)
                geometry2 = _random_geometry(rng, tuple(-c for c in centre), scale)
            elif kind == 2:
                geometry1 = _random_global_geometry(rng)
                geometry2 = _random_global_geometry(rng) if rng.random() < 0.5 else _random_geometry(rng, centre)
            elif kind == 3:
                geometry1 = _random_polygon_with_interior_rings(rng, centre)
                geometry2 = _random_geometry(rng, centre, 0.5)
            elif kind == 4:
                geometry1 = _random_geometry(rng, centre, duplicates=True)
                geometry2 = _random_geometry(rng, centre, duplicates=True)
            else:
                geometry1 = _random_geometry(rng, centre)
                geometry2 = _random_geometry(rng, centre)
            if rng.random() < 0.5:
                geometry1, geometry2 = geometry2, geometry1
            solid1 = rng.random() < 0.5
            solid2 = rng.random() < 0.5
            msg = 'case {}: {} to {}, solid {} {}'.format(
                case, type(geometry1).__name__, type(geometry2).__name__, solid1, solid2)
            expected = _brute_force_distance(geometry1, geometry2, solid1, solid2)

            distance, closest_point1, closest_point2, closest_index1, closest_index2 = D(
                geometry1, geometry2, return_closest_positions=True, return_closest_indices=True,
                geometry_is_solid=solid1, other_geometry_is_solid=solid2)
            # Distances within epsilon of zero are returned as zero, and near PI resolve only to ~1.5e-8.
            places = 7 if expected > math.pi - 1e-3 else 9
            if not (distance == 0.0 and expected < 1.5e-6):
                self.assertAlmostEqual(distance, expected, places=places, msg=msg)
            # The closest positions lie on their geometries' closest point/segment, and their
            # distance apart is the distance (unless inside a solid polygon, which is zero distance).
            for geometry, closest_point, closest_index in (
                    (geometry1, closest_point1, closest_index1),
                    (geometry2, closest_point2, closest_index2)):
                self.assertAlmostEqual(
                    _distance_to_indexed(geometry, closest_index, closest_point.to_xyz()), 0, places=9, msg=msg)
            if distance > 0:
                self.assertAlmostEqual(D(closest_point1, closest_point2), distance, places=places, msg=msg)

            # None only if the distance returned (without a threshold) exceeds the threshold.
            threshold = rng.uniform(0, 2 * distance) if distance > 0 else rng.choice((0.0, 0.01))
            result = D(geometry1, geometry2, threshold, geometry_is_solid=solid1, other_geometry_is_solid=solid2)
            if distance > threshold:
                self.assertTrue(result is None, msg=msg)
            else:
                self.assertTrue(result == distance, msg=msg)
            self.assertTrue(D(geometry1, geometry2, distance, geometry_is_solid=solid1, other_geometry_is_solid=solid2) == distance,
                            msg=msg)

    def test_get_centroid(self):
        # Test all geometry types have a 'get_centroid()' method.
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
        self.assertEqual(point.to_xyz(), self.xyz)
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
        self.assertEqual(
                list(pygplates.MultiPointOnSphere(pygplates.MultiPointOnSphere(self.points)).get_points()),
                self.points)
        self.assertEqual(
                list(pygplates.MultiPointOnSphere(pygplates.PolylineOnSphere(self.points)).get_points()),
                self.points)
        self.assertEqual(
                list(pygplates.MultiPointOnSphere(pygplates.PolygonOnSphere(self.points)).get_points()),
                self.points)
    
    def test_convert(self):
        # Convert to (lat,lon) tuple list and back.
        lat_lon_list = self.multi_point.to_lat_lon_list()
        self.assertEqual(len(lat_lon_list), len(self.multi_point))
        multi_point = pygplates.MultiPointOnSphere(lat_lon_list)
        self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
        # Convert to (x,y,z) tuple list and back.
        xyz_list = self.multi_point.to_xyz_list()
        self.assertEqual(len(xyz_list), len(self.multi_point))
        multi_point = pygplates.MultiPointOnSphere(xyz_list)
        self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
        if imported_numpy:
            # Convert to (lat,lon) numpy array and back.
            lat_lon_array = self.multi_point.to_lat_lon_array()
            self.assertEqual(len(list(lat_lon_array)), len(self.multi_point))
            multi_point = pygplates.MultiPointOnSphere(lat_lon_array)
            self.assertTrue(isinstance(multi_point, pygplates.MultiPointOnSphere))
            # Convert to (x,y,z) numpy array and back.
            xyz_array = self.multi_point.to_xyz_array()
            self.assertEqual(len(list(xyz_array)), len(self.multi_point))
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
        self.assertEqual(self.multi_point, pygplates.MultiPointOnSphere(point_sequence))
    
    def test_compare(self):
        self.assertEqual(self.multi_point, pygplates.MultiPointOnSphere(self.points))
    
    def test_iter(self):
        iter(self.multi_point)
        points = [point for point in self.multi_point]
        self.assertEqual(self.points, points)
        self.assertEqual(self.points, list(self.multi_point))
    
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
    
    def test_pickle(self):
        pickled_multi_point = pickle.loads(pickle.dumps(self.multi_point))
        self.assertTrue(pickled_multi_point == self.multi_point)


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
        self.assertEqual(
                list(pygplates.PolylineOnSphere(pygplates.MultiPointOnSphere(self.points)).get_points()),
                self.points)
        self.assertEqual(
                list(pygplates.PolylineOnSphere(pygplates.PolylineOnSphere(self.points)).get_points()),
                self.points)
        # When a polygon ring gets converted to a polyline the last/first ring vertex is added to create the last polyline arc.
        # Hence the polyline has an extra vertex.
        self.assertEqual(
                list(pygplates.PolylineOnSphere(pygplates.PolygonOnSphere(self.points)).get_points())[:-1],
                self.points)
        self.assertEqual(
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
        self.assertEqual(self.polyline, pygplates.PolylineOnSphere(point_sequence))
    
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
        self.assertEqual(self.polyline, pygplates.PolylineOnSphere(self.points))
    
    def test_iter(self):
        iter(self.polyline)
        points = [point for point in self.polyline]
        self.assertEqual(self.points, points)
        self.assertEqual(self.points, list(self.polyline))
        self.assertEqual(list(reversed(self.points)), list(reversed(self.polyline)))
    
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
        self.assertEqual(self.points, points)
        self.assertEqual(self.points, list(self.polyline.get_points()))
    
    def test_arcs_iter(self):
        iter(self.polyline.get_segments())
        arcs = [arc for arc in self.polyline.get_segments()]
        self.assertEqual(len(self.polyline.get_segments()) + 1, len(self.polyline.get_points()))
     
    def test_convert_polyline_to_polygon(self):
        polygon = pygplates.PolygonOnSphere(self.polyline.get_points())
        self.assertEqual(list(polygon.get_points()), list(self.polyline.get_points()))
    
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
    
    def test_uniform_points(self):
        uniform_points = self.polyline.to_uniform_points(math.radians(20), first_point_spacing_radians=math.radians(5))
        self.assertTrue(len(uniform_points) == 14)
        self.assertTrue(uniform_points[0] == pygplates.PointOnSphere(0, 5))
        self.assertTrue(uniform_points[1] == pygplates.PointOnSphere(0, 25))
        self.assertTrue(uniform_points[2] == pygplates.PointOnSphere(0, 45))
        self.assertTrue(uniform_points[3] == pygplates.PointOnSphere(0, 65))
        self.assertTrue(uniform_points[4] == pygplates.PointOnSphere(0, 85))
        self.assertTrue(uniform_points[5] == pygplates.PointOnSphere(15, 90))
        self.assertTrue(uniform_points[6] == pygplates.PointOnSphere(35, 90))
        self.assertTrue(uniform_points[7] == pygplates.PointOnSphere(55, 90))
        self.assertTrue(uniform_points[8] == pygplates.PointOnSphere(75, 90))
        self.assertTrue(uniform_points[9] == pygplates.PointOnSphere(85, -90))
        self.assertTrue(uniform_points[10] == pygplates.PointOnSphere(65, -90))
        self.assertTrue(uniform_points[11] == pygplates.PointOnSphere(45, -90))
        self.assertTrue(uniform_points[12] == pygplates.PointOnSphere(25, -90))
        self.assertTrue(uniform_points[13] == pygplates.PointOnSphere(5, -90))

        # Last point very near last vertex of polyline should get included.
        uniform_points = self.polyline.to_uniform_points(math.radians(30 - 1e-6))
        self.assertTrue(len(uniform_points) == 10)

        # Check segment informations.
        uniform_points, segment_informations = self.polyline.to_uniform_points(
            math.radians(70), first_point_spacing_radians=math.radians(5), return_segment_informations=True)
        self.assertTrue(len(uniform_points) == 4)
        self.assertTrue(len(segment_informations) == 4)
        self.assertTrue(segment_informations[0][0] == 0); self.assertAlmostEqual(segment_informations[0][1], (0 + 5) / 90.0)
        self.assertTrue(segment_informations[1][0] == 0); self.assertAlmostEqual(segment_informations[1][1], (70 + 5) / 90.0)
        self.assertTrue(segment_informations[2][0] == 1); self.assertAlmostEqual(segment_informations[2][1], (2*70 + 5 - 90) / 90.0)
        self.assertTrue(segment_informations[3][0] == 2); self.assertAlmostEqual(segment_informations[3][1], (3*70 + 5 - 180) / 90.0)
    
    def test_pickle(self):
        self.assertTrue(self.polyline == pickle.loads(pickle.dumps(self.polyline)))
        self.assertTrue(self.polyline.get_points() == pickle.loads(pickle.dumps(self.polyline.get_points())))
        self.assertTrue(self.polyline.get_segments() == pickle.loads(pickle.dumps(self.polyline.get_segments())))


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
        self.assertEqual(
                list(pygplates.PolygonOnSphere(pygplates.MultiPointOnSphere(self.points)).get_points()),
                self.points)
        self.assertEqual(
                list(pygplates.PolygonOnSphere(pygplates.PolylineOnSphere(self.points)).get_points()),
                self.points)
        self.assertEqual(
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
        self.assertEqual(self.polygon, pygplates.PolygonOnSphere(point_sequence))
        exterior_ring_point_sequence = self.polygon.get_exterior_ring_points()
        self.assertEqual(self.polygon, pygplates.PolygonOnSphere(exterior_ring_point_sequence))

        exterior_ring = self.polygon_with_interior.get_exterior_ring_points()
        interior_rings = [list(self.polygon_with_interior.get_interior_ring_points(interior_ring_index))
                for interior_ring_index in range(self.polygon_with_interior.get_number_of_interior_rings())]
        self.assertEqual(self.polygon_with_interior, pygplates.PolygonOnSphere(exterior_ring, interior_rings))
    
    def test_get_segments(self):
        point_sequence = self.polygon.get_points()
        segment_sequence = self.polygon.get_segments()
        self.assertEqual(len(point_sequence), len(segment_sequence))
        for index in range(len(point_sequence)):
            self.assertEqual(point_sequence[index], segment_sequence[index].get_start_point())
        
        exterior_ring_point_sequence = self.polygon_with_interior.get_exterior_ring_points()
        exterior_ring_segment_sequence = self.polygon_with_interior.get_exterior_ring_segments()
        self.assertEqual(len(exterior_ring_point_sequence), len(exterior_ring_segment_sequence))
        for index in range(len(exterior_ring_point_sequence)):
            self.assertEqual(exterior_ring_point_sequence[index], exterior_ring_segment_sequence[index].get_start_point())
        
        self.assertTrue(self.polygon_with_interior.get_number_of_interior_rings() == 2)
        for interior_ring_index in range(self.polygon_with_interior.get_number_of_interior_rings()):
            interior_ring_point_sequence = self.polygon_with_interior.get_interior_ring_points(interior_ring_index)
            interior_ring_segment_sequence = self.polygon_with_interior.get_interior_ring_segments(interior_ring_index)
            self.assertEqual(len(interior_ring_point_sequence), len(interior_ring_segment_sequence))
            for index in range(len(interior_ring_point_sequence)):
                self.assertEqual(interior_ring_point_sequence[index], interior_ring_segment_sequence[index].get_start_point())

    def test_compare(self):
        self.assertEqual(self.polygon, pygplates.PolygonOnSphere(self.points))
    
    def test_iter(self):
        iter(self.polygon)
        points = [point for point in self.polygon]
        self.assertEqual(self.points, points)
        self.assertEqual(self.points, list(self.polygon))
        self.assertEqual(list(reversed(self.points)), list(reversed(self.polygon)))

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
        self.assertEqual(self.points, points)
        self.assertEqual(self.points, list(self.polygon.get_points()))
     
    def test_convert_polygon_to_polyline(self):
        polyline1 = pygplates.PolylineOnSphere(self.polygon.get_points())
        polyline2 = pygplates.PolylineOnSphere(self.polygon)
		# Note that conversion from polygon to polyline gains an extra vertex (to generate last->first arc).
        self.assertEqual(list(polyline1.get_points())[:-1], list(self.polygon.get_points()))
        self.assertEqual(list(polyline2.get_points())[:-1], list(self.polygon.get_points()))
    
    def test_arcs_iter(self):
        iter(self.polygon.get_segments())
        arcs = [arc for arc in self.polygon.get_segments()]
        self.assertEqual(len(self.polygon.get_segments()), len(self.polygon.get_points()))

        iter(self.polygon.get_exterior_ring_segments())
        arcs = [arc for arc in self.polygon.get_exterior_ring_segments()]
        self.assertEqual(len(self.polygon.get_exterior_ring_segments()), len(self.polygon.get_exterior_ring_points()))

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
        # A concave (C-shaped) polygon, whatever its first vertex, and either way round. Orientation agrees with
        # the sign of the signed area (each on a newly created polygon, since one is calculated from the other
        # if that's cached). There was a bug in pyGPlates 1.0 where the edge from the last vertex to the first was
        # left out of the orientation, so it could be wrong for a concave polygon, depending on the first vertex.
        c_shape = [(-10, -10), (-10, 10), (-9.6, 10), (-9.6, -9.6), (9.6, -9.6), (9.6, 10), (10, 10), (10, -10)]
        for points in (c_shape, list(reversed(c_shape))):
            for first in range(len(points)):
                rotated = points[first:] + points[:first]
                expected = (pygplates.PolygonOnSphere.Orientation.counter_clockwise
                            if pygplates.PolygonOnSphere(rotated).get_signed_area() > 0
                            else pygplates.PolygonOnSphere.Orientation.clockwise)
                self.assertEqual(pygplates.PolygonOnSphere(rotated).get_orientation(), expected,
                                 msg='first vertex {}'.format(first))

    def test_point_in_polygon(self):
        inv_sqrt_two = 1.0 / math.sqrt(2.0)
        self.assertTrue(self.polygon.is_point_in_polygon(
                pygplates.PointOnSphere(inv_sqrt_two, 0, inv_sqrt_two)))
        self.assertFalse(self.polygon.is_point_in_polygon(
                pygplates.PointOnSphere(-inv_sqrt_two, 0, inv_sqrt_two)))
    
    def test_point_in_polygon_on_outline(self):
        # A point within about 1.4e-6 radians of the outline (an edge or a vertex) is inside, whatever its
        # parity, and at every speed (the first queries of a polygon use a different method than later ones).
        def rotated_away_from(point, centre, angle):
            # 'point' moved 'angle' radians further from 'centre', along the great circle through both.
            axis = _normalise(_cross(centre, point))
            tangent = _cross(axis, point)
            return pygplates.PointOnSphere(_normalise(tuple(
                math.cos(angle) * point[i] + math.sin(angle) * tangent[i] for i in range(3))))
        square = pygplates.PolygonOnSphere([(10, -10), (10, 10), (-10, 10), (-10, -10)])
        # 24 segments, so that later queries use a tree of lunes.
        ring = pygplates.PolygonOnSphere(_random_ring(random.Random(3), (1.0, 0.0, 0.0), 0.2, 24))
        for polygon in (square, ring):
            centroid = polygon.get_boundary_centroid().to_xyz()
            vertex = polygon.get_exterior_ring_points()[0].to_xyz()
            next_vertex = polygon.get_exterior_ring_points()[1].to_xyz()
            # Outside a corner.
            # There was a bug in pyGPlates 1.0 where a vertex itself could be outside, and points 1e-6 to 1.4e-6
            # radians outside a corner (up to 1e-5 beyond a sharp one) were inside only for the first queries.
            for distance, expected in ((0.0, True), (1e-6, True), (1.3e-6, True), (2e-6, False), (1e-5, False)):
                point = rotated_away_from(vertex, centroid, distance)
                self.assertEqual(_is_point_in_polygon_at_each_speed(polygon, point), [expected] * 3,
                                 msg='{} outside a corner'.format(distance))
            # Outside the middle of an edge.
            midpoint = _normalise(tuple(vertex[i] + next_vertex[i] for i in range(3)))
            for distance, expected in ((1e-7, True), (1.3e-6, True), (2e-6, False)):
                point = rotated_away_from(midpoint, centroid, distance)
                self.assertEqual(_is_point_in_polygon_at_each_speed(polygon, point), [expected] * 3,
                                 msg='{} outside an edge'.format(distance))
        # A point 1e-5 radians beyond a vertex, near the great circle of the next edge (1.2e-6 from it).
        # There was a bug in pyGPlates 1.0 where it was inside: the distance to an edge's great circle was
        # measured, not the distance to the edge, up to about 1e-4 radians away (640 metres).
        polygon = pygplates.PolygonOnSphere([
            (0.0811477206807658, 0.8312559099243764, -0.549935141306785),
            (-0.758639761126243, 0.4280677056928801, 0.49114534730683423),
            (-0.08121580393597666, -0.8312109862042011, 0.5499929905048542)])
        point = pygplates.PointOnSphere(-0.08120819577687013, -0.8312150679124771, 0.5499879451532749)
        self.assertEqual(_is_point_in_polygon_at_each_speed(polygon, point), [False] * 3)
        # A tiny polygon (about 1e-5 radians) and a point near its antipode, which is outside.
        # There was a bug in pyGPlates 1.0 where it was inside for the first three queries.
        polygon = pygplates.PolygonOnSphere([
            (79.73238827940156, -155.8187780084114),
            (79.73238827940156, -155.81820505061629),
            (79.73296123719669, -155.8187780084114)])
        point = pygplates.PointOnSphere(-79.73255743831264, 24.181608291766533)
        self.assertEqual(_is_point_in_polygon_at_each_speed(polygon, point), [False] * 3)

    def test_point_in_polygon_near_antipodal_centroid(self):
        # A point within about 1e-6 radians of the antipodal centroid (or the centroid), where the crossing arc
        # from the antipodal centroid is too short (or too nearly antipodal) for its plane to be defined.
        # There was a bug in pyGPlates 1.0 where these points were inside for the first three queries.
        for exterior_ring, point in (
                # A bow-tie (self-intersecting), at the antipodal centroid. Its edges' planes pass through the centroid.
                ([(0.31632634991861924, -0.742914659942909, 0.5899283417408213),
                  (0.9654266112134291, -0.05465835848510381, 0.2548802114889822),
                  (0.9600187829640893, -0.2743411893103646, 0.05568526020342974),
                  (0.3217341781679591, -0.5232318291176482, 0.7891232930263739)],
                 (-0.7409205817415255, 0.4610391258804732, -0.4883437477424776)),
                # A tiny polygon (about 1e-6 radians), at the antipode of a vertex.
                ([(0.46331008067020724, 0.5714775076315026, -0.6773161945654718),
                  (0.4633102924993963, 0.5714770455155942, -0.6773164395708191),
                  (0.46331017983531914, 0.5714770689359648, -0.6773164968766986),
                  (0.46331018144512975, 0.5714778365850115, -0.677315848080787)],
                 (-0.4633102924993963, -0.5714770455155942, 0.6773164395708191)),
                # A sliver (1e-6 radians wide), at the antipode of a vertex.
                ([(-0.5170680360101083, -0.2608430351399412, -0.815230983927643),
                  (-0.4540345429794015, -0.5160869754809512, -0.7262966800973347),
                  (-0.3560274787820443, -0.73157574133981, -0.5814132515132491)],
                 (0.4540345429794015, 0.5160869754809512, 0.7262966800973347))):
            polygon = pygplates.PolygonOnSphere(exterior_ring)
            self.assertEqual(_is_point_in_polygon_at_each_speed(polygon, pygplates.PointOnSphere(point)), [False] * 3)

    def test_point_in_polygon_against_brute_force(self):
        # Compare with a brute-force point-in-polygon (exact predicates, independent of pyGPlates' point-in-polygon)
        # at every speed, for random polygons (tiny, ordinary, larger than a hemisphere, with interior rings,
        # slivers, self-intersecting, with duplicate points, and enough segments for the tree of lunes) and points
        # (anywhere, near the polygon, on and near its vertices and edges, beyond a vertex along an edge's great
        # circle, and on the great circle through the antipodal centroid and a vertex).
        rng = random.Random(1234)
        for case in range(60):
            centre = _random_unit_vector(rng)
            kind = case % 6
            if kind == 0:
                polygon = pygplates.PolygonOnSphere(
                    [_random_point(rng, centre, rng.choice((1e-3, 1e-5))) for _ in range(rng.randint(3, 6))])
            elif kind == 1:
                polygon = _random_geometry(rng, centre, duplicates=rng.random() < 0.5)
                if not isinstance(polygon, pygplates.PolygonOnSphere):
                    polygon = pygplates.PolygonOnSphere(_random_ring(rng, centre, 0.2, rng.randint(4, 40)))
            elif kind == 2:
                polygon = pygplates.PolygonOnSphere(
                    _random_ring(rng, centre, rng.uniform(0.05, 2.8), rng.randint(17, 300)))
            elif kind == 3:
                polygon = _random_polygon_with_interior_rings(rng, centre)
            elif kind == 4:
                # A sliver.
                u = _normalise(_cross(centre, (0.3, 0.5, 0.8)))
                width = rng.choice((1e-4, 1e-6))
                polygon = pygplates.PolygonOnSphere([
                    _normalise(tuple(centre[i] + 0.3 * u[i] for i in range(3))),
                    _normalise(tuple(centre[i] + width * _cross(centre, u)[i] for i in range(3))),
                    _normalise(tuple(centre[i] - 0.3 * u[i] for i in range(3)))])
            else:
                # Self-intersecting (a bow-tie).
                ring = _random_ring(rng, centre, 0.3, 4)
                polygon = pygplates.PolygonOnSphere([ring[0], ring[2], ring[1], ring[3]])
            vertices = [v.to_xyz() for v in polygon.get_exterior_ring_points()]
            r = tuple(-x for x in polygon.get_boundary_centroid().to_xyz())
            scale = max(_angle(polygon.get_boundary_centroid().to_xyz(), v) for v in vertices)
            points = [_random_unit_vector(rng) for _ in range(4)]
            points += [_random_point(rng, rng.choice(vertices), scale).to_xyz() for _ in range(6)]
            points += [rng.choice(vertices)]
            for distance in (1e-7, 1.2e-6, 2e-6, 1e-5):
                # Either side of an edge.
                index = rng.randrange(len(vertices))
                a, b = vertices[index], vertices[(index + 1) % len(vertices)]
                if _angle(a, b) < 1e-5:
                    continue
                normal = _normalise(_cross(a, b))
                midpoint = _normalise(tuple(a[i] + b[i] for i in range(3)))
                sign = rng.choice((1, -1))
                points.append(_normalise(tuple(midpoint[i] + sign * distance * normal[i] for i in range(3))))
                # Beyond a vertex, near the edge's great circle.
                tangent = _cross(normal, b)
                points.append(_normalise(tuple(b[i] + distance * 10 * tangent[i] + sign * 1e-6 * normal[i] for i in range(3))))
            vertex = rng.choice(vertices)
            if 1e-3 < _angle(r, vertex) < math.pi - 1e-3:
                # On the great circle through the antipodal centroid and a vertex, beyond the vertex.
                tangent = _cross(_normalise(_cross(r, vertex)), vertex)
                points.append(_normalise(tuple(vertex[i] + rng.choice((1e-5, 0.1 * scale)) * tangent[i] for i in range(3))))
            # At, and within 1e-7 and 1e-5 radians of, the antipodal centroid R and the centroid (where pyGPlates'
            # crossing arc, from R, is too short or nearly antipodal for its plane to be defined).
            # There was a bug in pyGPlates 1.0 where a point there could be inside a tiny polygon (or a sliver or
            # a self-intersecting one) for the first three queries.
            for centre in (r, tuple(-x for x in r)):
                points.append(centre)
                points += [_random_point(rng, centre, spread).to_xyz() for spread in (1e-7, 1e-5)]
            # The antipode of a vertex (near the antipodal centroid of a tiny polygon or a sliver).
            points.append(tuple(-x for x in rng.choice(vertices)))
            for p in points:
                point = pygplates.PointOnSphere(p)
                expected = _brute_force_point_in_polygon(polygon, point, rng)
                answers = _is_point_in_polygon_at_each_speed(polygon, point)
                msg = 'case {}: kind {}, point {}'.format(case, kind, p)
                if expected is None:
                    # Within the band of the outline, either answer is accepted, but the same at every speed.
                    self.assertTrue(answers[0] == answers[1] == answers[2], msg=msg)
                else:
                    self.assertEqual(answers, [expected] * 3, msg=msg)

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
        self.assertEqual(self.polygon.get_centroid(), self.polygon.get_interior_centroid())
    
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
    
    def test_uniform_points(self):
        uniform_points = self.polygon.to_uniform_points(math.radians(20), first_point_spacing_radians=math.radians(5))
        self.assertTrue(len(uniform_points) == 18)
        self.assertTrue(uniform_points[0] == pygplates.PointOnSphere(0, 5))
        self.assertTrue(uniform_points[1] == pygplates.PointOnSphere(0, 25))
        self.assertTrue(uniform_points[2] == pygplates.PointOnSphere(0, 45))
        self.assertTrue(uniform_points[3] == pygplates.PointOnSphere(0, 65))
        self.assertTrue(uniform_points[4] == pygplates.PointOnSphere(0, 85))
        self.assertTrue(uniform_points[5] == pygplates.PointOnSphere(15, 90))
        self.assertTrue(uniform_points[6] == pygplates.PointOnSphere(35, 90))
        self.assertTrue(uniform_points[7] == pygplates.PointOnSphere(55, 90))
        self.assertTrue(uniform_points[8] == pygplates.PointOnSphere(75, 90))
        self.assertTrue(uniform_points[9] == pygplates.PointOnSphere(85, -90))
        self.assertTrue(uniform_points[10] == pygplates.PointOnSphere(65, -90))
        self.assertTrue(uniform_points[11] == pygplates.PointOnSphere(45, -90))
        self.assertTrue(uniform_points[12] == pygplates.PointOnSphere(25, -90))
        self.assertTrue(uniform_points[13] == pygplates.PointOnSphere(5, -90))
        self.assertTrue(uniform_points[14] == pygplates.PointOnSphere(0, -75))
        self.assertTrue(uniform_points[15] == pygplates.PointOnSphere(0, -55))
        self.assertTrue(uniform_points[16] == pygplates.PointOnSphere(0, -35))
        self.assertTrue(uniform_points[17] == pygplates.PointOnSphere(0, -15))

        # Last point very near first vertex of polygon should NOT get included (it's a duplicate ring vertex).
        #
        # Note: A negative epsilon ensures last point not clipped but should be small enough that last point
        #       compares equal to the first point (ie, don't want them to compare as separate points) so that
        #       it then gets removed from the uniform points (as a duplicate).
        uniform_points = self.polygon.to_uniform_points(math.radians(30 - 1e-12))
        self.assertTrue(len(uniform_points) == 12)  # it would be 13 if last point was included
    
        polygon_with_interior = pygplates.PolygonOnSphere(
                [(0, 0), (0, 90), (90, 0), (0, -90)],  # exterior ring
                [[(0, 0), (0, 90), (90, 0), (0, -90)], [(0, 0), (0, 90), (90, 0), (0, -90)]])  # 2 interior rings each same as exterior ring
        uniform_points = polygon_with_interior.to_uniform_points(math.radians(20), first_point_spacing_radians=math.radians(5))
        self.assertTrue(len(uniform_points) == 3 * 18)  # each ring has 18 uniform points

        # Check segment informations.
        uniform_points, segment_informations = self.polygon.to_uniform_points(
            math.radians(70), first_point_spacing_radians=math.radians(5), return_segment_informations=True)
        self.assertTrue(len(uniform_points) == 6)
        self.assertTrue(len(segment_informations) == 6)
        self.assertTrue(segment_informations[0][0] == 0); self.assertAlmostEqual(segment_informations[0][1], (0 + 5) / 90.0)
        self.assertTrue(segment_informations[1][0] == 0); self.assertAlmostEqual(segment_informations[1][1], (70 + 5) / 90.0)
        self.assertTrue(segment_informations[2][0] == 1); self.assertAlmostEqual(segment_informations[2][1], (2*70 + 5 - 90) / 90.0)
        self.assertTrue(segment_informations[3][0] == 2); self.assertAlmostEqual(segment_informations[3][1], (3*70 + 5 - 180) / 90.0)
        self.assertTrue(segment_informations[4][0] == 3); self.assertAlmostEqual(segment_informations[4][1], (4*70 + 5 - 270) / 90.0)
        self.assertTrue(segment_informations[5][0] == 3); self.assertAlmostEqual(segment_informations[5][1], (5*70 + 5 - 270) / 90.0)

        uniform_points, segment_informations = polygon_with_interior.to_uniform_points(
            math.radians(20), first_point_spacing_radians=math.radians(5), return_segment_informations=True)
        self.assertTrue(len(uniform_points) == 3 * 18)        # each ring has 18 uniform points
        self.assertTrue(len(segment_informations) == 3 * 18)  # each ring has 18 uniform points

    def test_pickle(self):
        self.assertTrue(self.polygon == pickle.loads(pickle.dumps(self.polygon)))
        self.assertTrue(self.polygon.get_points() == pickle.loads(pickle.dumps(self.polygon.get_points())))
        self.assertTrue(self.polygon.get_segments() == pickle.loads(pickle.dumps(self.polygon.get_segments())))
        self.assertTrue(self.polygon.get_exterior_ring_points() == pickle.loads(pickle.dumps(self.polygon.get_exterior_ring_points())))
        self.assertTrue(self.polygon.get_exterior_ring_segments() == pickle.loads(pickle.dumps(self.polygon.get_exterior_ring_segments())))

        self.assertTrue(self.polygon_with_interior == pickle.loads(pickle.dumps(self.polygon_with_interior)))
        self.assertTrue(self.polygon_with_interior.get_points() == pickle.loads(pickle.dumps(self.polygon_with_interior.get_points())))
        self.assertTrue(self.polygon_with_interior.get_segments() == pickle.loads(pickle.dumps(self.polygon_with_interior.get_segments())))
        self.assertTrue(self.polygon_with_interior.get_exterior_ring_points() == pickle.loads(pickle.dumps(self.polygon_with_interior.get_exterior_ring_points())))
        self.assertTrue(self.polygon_with_interior.get_exterior_ring_segments() == pickle.loads(pickle.dumps(self.polygon_with_interior.get_exterior_ring_segments())))
        self.assertTrue(self.polygon_with_interior.get_interior_ring_points(0) == pickle.loads(pickle.dumps(self.polygon_with_interior.get_interior_ring_points(0))))
        self.assertTrue(self.polygon_with_interior.get_interior_ring_segments(0) == pickle.loads(pickle.dumps(self.polygon_with_interior.get_interior_ring_segments(0))))

        # Test boost-python enum types nested in PolygonOnSphere ('PolygonOnSphere.Orientation' and 'PolygonOnSphere.PartitionResult').
        # These are also defined at the module level so that pickle can find them.
        # This is why we're only testing "nested" enums (regular boost-python enums defined at the module level are pickled fine).
        self.assertTrue(self.polygon.get_orientation() == pickle.loads(pickle.dumps(self.polygon.get_orientation())))
        self.assertTrue(self.polygon.partition(pygplates.PointOnSphere(0, 0)) == pickle.loads(pickle.dumps(self.polygon.partition(pygplates.PointOnSphere(0, 0)))))


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
