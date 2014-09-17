# Copyright (C) 2014 The University of Sydney, Australia
# 
# This file is part of GPlates.
# 
# GPlates is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
# 
# GPlates is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


import itertools
# Import numpy if it's available...
try:
    import numpy
except ImportError:
    pass


def geometry_on_sphere_get_points(geometry):
    """get_points() -> sequence
    Returns the sequence of :class:`points<PointOnSphere>` in this geometry.
    
    The following operations for accessing the points in the returned sequence are supported:
    
    ============================ ==========================================================
    Operation                    Result
    ============================ ==========================================================
    ``len(seq)``                 length of *seq*
    ``for p in seq``             iterates over the points *p* of *seq*
    ``p in seq``                 ``True`` if *p* is equal to a point in *seq*
    ``p not in seq``             ``False`` if *p* is equal to a point in *seq*
    ``seq[i]``                   the point of *seq* at index *i*
    ``seq[i:j]``                 slice of *seq* from *i* to *j*
    ``seq[i:j:k]``               slice of *seq* from *i* to *j* with step *k*
    ============================ ==========================================================
    
    If this geometry is a :class:`PointOnSphere` then the returned sequence has length one.
    For other geometry types (:class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and
    :class:`PolygonOnSphere`) the length will equal the number of :class:`points<PointOnSphere>`
    contained within.
    
    The following example demonstrates some uses of the above operations:
    ::
    
      points = geometry.get_points()
      for point in points:
          print point
      first_point = points[0]
      last_point = points[-1]
    
    Note that since a *GeometryOnSphere* is immutable it contains no operations or
    methods that modify its state (such as adding or removing points).
    """
    
    # Use private method in derived classes.
    return geometry._get_points()

# Add the module function as a class method.
GeometryOnSphere.get_points = geometry_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_get_points


def geometry_on_sphere_to_lat_lon_list(geometry):
    """to_lat_lon_list() -> list
    Returns the sequence of points, in this geometry, as (latitude,longitude) tuples (in degrees).
    
    :rtype: list of (float,float) tuples
    :returns: a list of (latitude,longitude) tuples (in degrees)
    
    If this geometry is a :class:`PointOnSphere` then the returned sequence has length one.
    For other geometry types (:class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and
    :class:`PolygonOnSphere`) the length will equal the number of :class:`points<PointOnSphere>`
    contained within.
    
    If you want the latitude/longitude order swapped in the returned tuples then the following is one way to achieve this:
    ::
    
      # Convert (latitude,longitude) to (longitude,latitude).
      [(lon,lat) for lat, lon in geometry.to_lat_lon_list()]
    """
    
    return [point.to_lat_lon() for point in geometry.get_points()]

# Add the module function as a class method.
GeometryOnSphere.to_lat_lon_list = geometry_on_sphere_to_lat_lon_list
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_to_lat_lon_list


def geometry_on_sphere_to_lat_lon_array(geometry):
    """to_lat_lon_array() -> array
    Returns the sequence of points, in this geometry, as a numpy array of (latitude,longitude) pairs (in degrees).
    
    :rtype: 2D numpy array with number of points as outer dimension and an inner dimension of two
    :returns: an array of (latitude,longitude) pairs (in degrees)
    
    **NOTE** this method should only be called if the ``numpy`` module is available.
    
    If this geometry is a :class:`PointOnSphere` then the returned sequence has length one.
    For other geometry types (:class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and
    :class:`PolygonOnSphere`) the length will equal the number of :class:`points<PointOnSphere>`
    contained within.
    
    If you want the latitude/longitude order swapped in the returned tuples then the following is one way to achieve this:
    ::
    
      # Convert (latitude,longitude) to (longitude,latitude).
      geometry.to_lat_lon_array()[:, (1,0)]
    
    If you need a flat 1D numpy array then you can do something like:
    ::
    
      geometry.to_lat_lon_array().flatten()
    """
    
    return numpy.array(geometry.to_lat_lon_list())

# Add the module function as a class method.
GeometryOnSphere.to_lat_lon_array = geometry_on_sphere_to_lat_lon_array
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_to_lat_lon_array


def geometry_on_sphere_to_lat_lon_point_list(geometry):
    """to_lat_lon_point_list() -> list
    Returns the sequence of points, in this geometry, as :class:`lat lon points<LatLonPoint>`.
    
    :rtype: list of :class:`LatLonPoint`
    
    If this geometry is a :class:`PointOnSphere` then the returned sequence has length one.
    For other geometry types (:class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and
    :class:`PolygonOnSphere`) the length will equal the number of :class:`points<PointOnSphere>`
    contained within.
    """
    
    return [point.to_lat_lon_point() for point in geometry.get_points()]

# Add the module function as a class method.
GeometryOnSphere.to_lat_lon_point_list = geometry_on_sphere_to_lat_lon_point_list
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_to_lat_lon_point_list


def geometry_on_sphere_to_xyz_list(geometry):
    """to_xyz_list() -> list
    Returns the sequence of points, in this geometry, as (x,y,z) cartesian coordinate tuples.
    
    :rtype: list of (float,float,float) tuples
    :returns: a list of (x,y,z) tuples
    
    If this geometry is a :class:`PointOnSphere` then the returned sequence has length one.
    For other geometry types (:class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and
    :class:`PolygonOnSphere`) the length will equal the number of :class:`points<PointOnSphere>`
    contained within.
    """
    
    return [point.to_xyz() for point in geometry.get_points()]

# Add the module function as a class method.
GeometryOnSphere.to_xyz_list = geometry_on_sphere_to_xyz_list
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_to_xyz_list


def geometry_on_sphere_to_xyz_array(geometry):
    """to_xyz_array() -> array
    Returns the sequence of points, in this geometry, as a numpy array of (x,y,z) triplets.
    
    :rtype: 2D numpy array with number of points as outer dimension and an inner dimension of three
    :returns: an array of (x,y,z) triplets
    
    **NOTE** this method should only be called if the ``numpy`` module is available.
    
    If you need a flat 1D numpy array then you can do something like:
    ::
    
      geometry.to_xyz_array().flatten()
    
    If this geometry is a :class:`PointOnSphere` then the returned sequence has length one.
    For other geometry types (:class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and
    :class:`PolygonOnSphere`) the length will equal the number of :class:`points<PointOnSphere>`
    contained within.
    """
    
    return numpy.array(geometry.to_xyz_list())

# Add the module function as a class method.
GeometryOnSphere.to_xyz_array = geometry_on_sphere_to_xyz_array
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_to_xyz_array


def point_on_sphere_get_points(point_on_sphere):
    # Just return the single point in a list.
    return [point_on_sphere]

# Add the module function as a class method.
PointOnSphere._get_points = point_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del point_on_sphere_get_points


def multi_point_on_sphere_get_points(multi_point_on_sphere):
    # Just return the multipoint itself - it acts like a list.
    return multi_point_on_sphere

# Add the module function as a class method.
MultiPointOnSphere._get_points = multi_point_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del multi_point_on_sphere_get_points


def polyline_on_sphere_get_points(polyline_on_sphere):
    return polyline_on_sphere

# Add the module function as a class method.
PolylineOnSphere._get_points = polyline_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del polyline_on_sphere_get_points


def polyline_on_sphere_join(polylines, distance_threshold_radians):
    """join(polylines, distance_threshold_radians) -> list
    Joins polylines that have end points closer than a distance threshold.
    
    :param polylines: the polylines to join
    :type polyline: sequence (eg, ``list`` or ``tuple``) of :class:`PolylineOnSphere` - though any \
    :class:`GeometryOnSphere` will work since :meth:`GeometryOnSphere.get_points` is used internally to query end points
    :param distance_threshold_radians: closeness distance threshold in radians for joining to occur
    :type distance_threshold_radians: float
    :returns: a list of joined polylines - the list will contain all polylines in *polylines* if none were joined
    :rtype: list of :class:`PolylineOnSphere`
    
    All pairs of polylines are tested for joining and only those closer than *distance_threshold_radians*
    radians are joined. Each joined polyline is further joined if possible until there are no more
    possibilities for joining (or there is a single joined polyline that is a concatenation of all
    polylines in *polylines*).
    
    When determining if two polylines A and B can be joined the closest pair of end points
    (one from A and one from B) decides which end of each polyline can be joined, provided their
    distance is less than *distance_threshold_radians* radians. If a third polyline C also has an end
    point close enough to A then the closest of B and C is joined to A.
    
    Two polylines A and B are joined by prepending or appending a (possibly reversed) copy of the
    points in polyline B to a copy of the points in polyline A. Hence the joined polyline will always
    have points ordered in the same direction as polyline A (only the points from polyline B are
    reversed if necessary). So polylines earlier in the *polylines* sequence determine the direction
    of joined polylines.
    
    Join three polylines if their end points are within 3 degrees of another:
    ::
    
      # If all three polylines join then the returned list will have one joined polyline.
      # If only two polylines join then the returned list will have two polylines (one original and one joined).
      # If no polylines join then the returned list will have the three original polylines.
      joined_polylines = pygplates.PolylineOnSphere.join((polyline1, polyline2, polyline3), math.radians(3))
    """
    
    # Start with all original polylines and then reduce number in list as polylines are joined.
    joined_polylines = list(polylines)
    
    #
    # Do N^2 search over pairs of polylines to test for joining.
    #
    
    # Iterate over all polylines except last one.
    # Using len() since some polylines are removed during iteration.
    # Note: If there are fewer than two polylines then the loop is not entered.
    polyline1_index = 0
    while polyline1_index < len(joined_polylines) - 1:
        # Make sure works with any GeometryOnSphere type by calling 'get_points()'.
        polyline1 = joined_polylines[polyline1_index].get_points()
        
        min_dist = distance_threshold_radians
        join_polylines = None
        
        # Iterate over the remaining polylines (after 'polyline1') and find the closest
        # polyline (if any less than threshold).
        for polyline2_index in range(polyline1_index + 1, len(joined_polylines)):
            # Make sure works with any GeometryOnSphere type by calling 'get_points()'.
            polyline2 = joined_polylines[polyline2_index].get_points()
            
            # Calculate distance between four combinations of polyline1/polyline2 end points.
            dist00 = GeometryOnSphere.distance(polyline1[0], polyline2[0])
            dist01 = GeometryOnSphere.distance(polyline1[0], polyline2[-1])
            dist10 = GeometryOnSphere.distance(polyline1[-1], polyline2[0])
            dist11 = GeometryOnSphere.distance(polyline1[-1], polyline2[-1])

            dist = min(dist00, dist01, dist10, dist11)
            
            # See if the minimum distance was below the theshold (and is the shortest distance so far).
            if dist <= min_dist:
                min_dist = dist
                # Record the iterator that joins 'polyline1' and 'polyline2' and also record the
                # index of 'polyline2' (so it can be removed later if it's the closest polyline).
                if dist == dist00:
                    join_polylines = (itertools.chain(reversed(polyline2), polyline1), polyline2_index)
                elif dist == dist01:
                    join_polylines = (itertools.chain(polyline2, polyline1), polyline2_index)
                elif dist == dist10:
                    join_polylines = (itertools.chain(polyline1, polyline2), polyline2_index)
                else:
                    join_polylines = (itertools.chain(polyline1, reversed(polyline2)), polyline2_index)
            
        if join_polylines:
            # Replace 'polyline1' with the joined polyline.
            joined_polylines[polyline1_index] = PolylineOnSphere(join_polylines[0])
            # Remove the 'polyline2' that was joined with 'polyline1'.
            del joined_polylines[join_polylines[1]]
            # Note that we don't increment 'polyline1_index' because we want to test the
            # newly joined polyline with subsequent polyline2.
        else:
            polyline1_index += 1
    
    return joined_polylines

# Add the module function as a class static-method.
PolylineOnSphere.join = staticmethod(polyline_on_sphere_join)
# Delete the module reference to the function - we only keep the class method.
del polyline_on_sphere_join


def polygon_on_sphere_get_points(polygon_on_sphere):
    return polygon_on_sphere

# Add the module function as a class method.
PolygonOnSphere._get_points = polygon_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del polygon_on_sphere_get_points
