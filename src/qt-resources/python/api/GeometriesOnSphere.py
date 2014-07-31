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
    return polyline_on_sphere.get_points_view()

# Add the module function as a class method.
PolylineOnSphere._get_points = polyline_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del polyline_on_sphere_get_points


def polygon_on_sphere_get_points(polygon_on_sphere):
    return polygon_on_sphere.get_points_view()

# Add the module function as a class method.
PolygonOnSphere._get_points = polygon_on_sphere_get_points
# Delete the module reference to the function - we only keep the class method.
del polygon_on_sphere_get_points
