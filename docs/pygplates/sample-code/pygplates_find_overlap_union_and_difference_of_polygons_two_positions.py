import pygplates


# [fragment: reconstruct-snapshot]
# Reconstruct the static polygons to 50Ma.
reconstruction_time = 50
reconstruct_snapshot = pygplates.ReconstructSnapshot('static_polygons.gpml', 'rotations.rot', reconstruction_time)
# [end: reconstruct-snapshot]


# [fragment: area-function]
def get_area_in_square_kms(polygons):
    """The total area of a list of polygons that do not overlap each other."""
    return sum(polygon.get_area() for polygon in polygons) * pygplates.Earth.mean_radius_in_kms**2
# [end: area-function]


# [fragment: two-polygons]
for reconstructed_geometry in reconstruct_snapshot.get_reconstructed_geometries():

    # The two polygons to compare: where the static polygon is at present day, and where it was at 50Ma.
    present_day_polygon = reconstructed_geometry.get_present_day_geometry()
    reconstructed_polygon = reconstructed_geometry.get_reconstructed_geometry()
    if not isinstance(reconstructed_polygon, pygplates.PolygonOnSphere):
        continue
    # [end: two-polygons]

    # [fragment: intersection]
    # The region inside both polygons.
    polygons_in_both = reconstructed_polygon.intersection(present_day_polygon)
    # [end: intersection]

    # [fragment: difference]
    # The region inside the reconstructed polygon but outside the present day polygon.
    polygons_in_reconstructed_only = reconstructed_polygon.difference(present_day_polygon)
    # [end: difference]

    # [fragment: symmetric-difference]
    # The region inside exactly one of the two polygons.
    polygons_in_one_only = reconstructed_polygon.symmetric_difference(present_day_polygon)
    # [end: symmetric-difference]

    # [fragment: union]
    # The region inside either polygon (or both).
    polygons_in_either = reconstructed_polygon.union(present_day_polygon)
    # [end: union]

    # [fragment: print-areas]
    plate_id = reconstructed_geometry.get_feature().get_reconstruction_plate_id()
    print(f'Polygon of plate {plate_id} at {reconstruction_time}Ma and at present day:')
    print(f'  {get_area_in_square_kms(polygons_in_both):.0f} square kms are inside both')
    print(f'  {get_area_in_square_kms(polygons_in_reconstructed_only):.0f} square kms are inside the reconstructed polygon only')
    print(f'  {get_area_in_square_kms(polygons_in_one_only):.0f} square kms are inside one only')
    print(f'  {get_area_in_square_kms(polygons_in_either):.0f} square kms are inside either')
    # [end: print-areas]

    # [fragment: print-interior-rings]
    # The polygons returned have interior rings where the region has holes.
    num_interior_rings = sum(polygon.get_number_of_interior_rings() for polygon in polygons_in_either)
    print(f'  the region inside either is {len(polygons_in_either)} polygons with {num_interior_rings} interior rings')
    # [end: print-interior-rings]
