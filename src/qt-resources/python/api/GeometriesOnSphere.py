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
