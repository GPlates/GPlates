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
    """get_points()
    Returns a **read-only** sequence of :class:`points<PointOnSphere>` in this geometry.
    
    :rtype: a read-only sequence of :class:`PointOnSphere`
    
    The following operations for accessing the points in the returned read-only sequence are supported:
    
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
    
    .. note:: The returned sequence is **read-only** and cannot be modified.
    
    .. note:: If you want a modifiable sequence consider wrapping the returned sequence in a ``list``
       using something like ``points = list(geometry.get_points())`` **but** note that modifying
       the ``list`` (eg, inserting a new point) will **not** modify the original geometry.
    
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
    
    | However if you know you have a :class:`MultiPointOnSphere`, :class:`PolylineOnSphere` or :class:`PolygonOnSphere`
      (ie, not a :class:`PointOnSphere`) it's actually easier to iterate directly over the geometry itself.
    | For example with a :class:`PolylineOnSphere`:
    
    ::
    
      for point in polyline:
          print point
      first_point = polyline[0]
      last_point = polyline[-1]
    
    .. note:: There are also methods that return the sequence of points as (latitude,longitude)
       values and (x,y,z) values contained in lists and numpy arrays
       (:meth:`to_lat_lon_list`, :meth:`to_lat_lon_array`, :meth:`to_xyz_list` and :meth:`to_xyz_array`).
    """

    # Use private method in derived classes.
    return geometry._get_points()

# Add the module function as a class method.
GeometryOnSphere.get_points = geometry_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del geometry_on_sphere_get_points


def geometry_on_sphere_to_lat_lon_list(geometry):
    """to_lat_lon_list()
    Returns the sequence of points, in this geometry, as (latitude,longitude) tuples (in degrees).
    
    :returns: a list of (latitude,longitude) tuples (in degrees)
    :rtype: list of (float,float) tuples
    
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
    """to_lat_lon_array()
    Returns the sequence of points, in this geometry, as a numpy array of (latitude,longitude) pairs (in degrees).
    
    :returns: an array of (latitude,longitude) pairs (in degrees)
    :rtype: 2D numpy array with number of points as outer dimension and an inner dimension of two
    
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
    """to_lat_lon_point_list()
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
    """to_xyz_list()
    Returns the sequence of points, in this geometry, as (x,y,z) cartesian coordinate tuples.
    
    :returns: a list of (x,y,z) tuples
    :rtype: list of (float,float,float) tuples
    
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
    """to_xyz_array()
    Returns the sequence of points, in this geometry, as a numpy array of (x,y,z) triplets.
    
    :returns: an array of (x,y,z) triplets
    :rtype: 2D numpy array with number of points as outer dimension and an inner dimension of three
    
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


def polygon_on_sphere_get_points(polygon_on_sphere):
    return polygon_on_sphere

# Add the module function as a class method.
PolygonOnSphere._get_points = polygon_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del polygon_on_sphere_get_points


def polyline_on_sphere_join(geometries, distance_threshold_radians, polyline_conversion=PolylineConversion.ignore_non_polyline):
    """join(geometries, distance_threshold_radians, polyline_conversion=PolylineConversion.ignore_non_polyline)
    Joins geometries that have end points closer than a distance threshold.
    
    :param geometries: the geometries to join
    :type geometries: sequence (eg, ``list`` or ``tuple``) of :class:`GeometryOnSphere`
    :param distance_threshold_radians: closeness distance threshold in radians for joining to occur
    :type distance_threshold_radians: float
    :param polyline_conversion: whether to raise error, convert to :class:`PolylineOnSphere` or ignore \
    those geometries in *geometries* that are not :class:`PolylineOnSphere` - defaults to \
    *PolylineConversion.ignore_non_polyline*
    :type polyline_conversion: *PolylineConversion.convert_to_polyline*, *PolylineConversion.ignore_non_polyline* \
    or *PolylineConversion.raise_if_non_polyline*
    :returns: a list of joined polylines
    :rtype: list of :class:`PolylineOnSphere`
    :raises: GeometryTypeError if *polyline_conversion* is *PolylineConversion.raise_if_non_polyline* and \
    any geometry in *geometries* is not a :class:`PolylineOnSphere`
    
    All pairs of geometries are tested for joining and only those closer than *distance_threshold_radians*
    radians are joined. Each joined polyline is further joined if possible until there are no more
    possibilities for joining (or there is a single joined polyline that is a concatenation of all
    geometries in *geometries* - depending on *polyline_conversion*).
    
    When determining if two geometries A and B can be joined the closest pair of end points
    (one from A and one from B) decides which end of each geometry can be joined, provided their
    distance is less than *distance_threshold_radians* radians. If a third geometries C also has an
    end point close enough to A then the closest of B and C is joined to A.
    
    Two geometries A and B are joined by prepending or appending a (possibly reversed) copy of the
    points in geometry B to a copy of the points in geometry A. Hence the joined polyline will
    always have points ordered in the same direction as geometry A (only the points from geometry B are
    reversed if necessary). So geometries earlier in the *geometries* sequence determine the direction
    of joined polylines.
    
    Join three polylines if their end points are within 3 degrees of another:
    ::
    
      # If all three polylines join then the returned list will have one joined polyline.
      # If only two polylines join then the returned list will have two polylines (one original and one joined).
      # If no polylines join then the returned list will have the three original polylines.
      joined_polylines = pygplates.PolylineOnSphere.join((polyline1, polyline2, polyline3), math.radians(3))
    
    Other geometries besides :class:`PolylineOnSphere` can be joined if *polyline_conversion* is
    *PolylineConversion.convert_to_polyline*. This is useful for joining nearby points into polylines for example:
    ::
    
      # If all points are close enough then the returned list will have one joined polyline,
      # otherwise there will be multiple polylines each representing a subset of the points.
      # If none of the points are close to each other then the returned list will have degenerate
      # polylines that each look like a point (each polyline has two identical points).
      joined_polylines = pygplates.PolylineOnSphere.join(
              points, math.radians(3), pygplates.PolylineConversion.convert_to_polyline)
      """
    
    # Start with all original geometries in 'joined_polylines' and
    # then reduce number in list as geometries are joined.
    if polyline_conversion == PolylineConversion.convert_to_polyline:
        # Convert all geometries to PolylineOnSphere's if necessary.
        joined_polylines = [PolylineOnSphere(geometry, allow_one_point=True) for geometry in geometries]
    elif polyline_conversion == PolylineConversion.ignore_non_polyline:
        # Only add PolylineOnSphere geometries.
        joined_polylines = []
        for geometry in geometries:
            if isinstance(geometry, PolylineOnSphere):
                joined_polylines.append(geometry)
    else: # polyline_conversion == PolylineConversion.raise_if_non_polyline
        # Raise error if any geometry is not a PolylineOnSphere.
        for geometry in geometries:
            if not isinstance(geometry, PolylineOnSphere):
                raise GeometryTypeError('Expected PolylineOnSphere geometries')
        joined_polylines = list(geometries)
    
    #
    # Do N^2 search over pairs of geometries to test for joining.
    #
    
    # Iterate over all polylines except last one.
    # Using len() since some polylines are removed during iteration.
    # Note: If there are fewer than two polylines then the loop is not entered.
    polyline1_index = 0
    while polyline1_index < len(joined_polylines) - 1:
        polyline1 = joined_polylines[polyline1_index]
        
        min_dist = distance_threshold_radians
        join_polylines = None
        
        # Iterate over the remaining polylines (after 'polyline1') and find the closest
        # polyline (if any less than threshold).
        for polyline2_index in range(polyline1_index + 1, len(joined_polylines)):
            polyline2 = joined_polylines[polyline2_index]
            
            # Calculate distance between four combinations of polyline1/polyline2 end points.
            dist00 = GeometryOnSphere.distance(polyline1[0], polyline2[0])
            dist01 = GeometryOnSphere.distance(polyline1[0], polyline2[-1])
            dist10 = GeometryOnSphere.distance(polyline1[-1], polyline2[0])
            dist11 = GeometryOnSphere.distance(polyline1[-1], polyline2[-1])

            dist = min(dist00, dist01, dist10, dist11)
            
            # See if the minimum distance was below the threshold (and is the shortest distance so far).
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
            # newly joined polyline with subsequent polyline2's.
        else:
            # We're keeping the original geometry.
            polyline1_index += 1
    
    return joined_polylines

# Add the module function as a class static-method.
PolylineOnSphere.join = staticmethod(polyline_on_sphere_join)
# Delete the module reference to the function - we only keep the class method.
del polyline_on_sphere_join
