import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-snapshot]
# Reconstruct the static polygons to 50Ma.
reconstruction_time = 50
reconstruct_snapshot = pygplates.ReconstructSnapshot('static_polygons.gpml', rotation_model, reconstruction_time)
# [end: reconstruct-snapshot]

# [fragment: region]
# The region of interest: a polygon that stays where it is (it is not reconstructed).
region = pygplates.PolygonOnSphere([(5, 45), (5, 90), (50, 90), (50, 45)])
# [end: region]

# [fragment: uncovered-region]
# The polygons that make up the part of the region that no reconstructed polygon covers.
# To begin with that is the whole region.
uncovered_polygons = [region]
# [end: uncovered-region]

# [fragment: iterate-reconstructed-polygons]
for reconstructed_geometry in reconstruct_snapshot.get_reconstructed_geometries():

    # Only polygons can overlap the region.
    reconstructed_polygon = reconstructed_geometry.get_reconstructed_geometry()
    if not isinstance(reconstructed_polygon, pygplates.PolygonOnSphere):
        continue
    # [end: iterate-reconstructed-polygons]

    # [fragment: intersection]
    # The part of the reconstructed polygon that is inside the region, as a list of polygons.
    # The list is empty if the two do not overlap.
    polygons_inside_region = reconstructed_polygon.intersection(region)
    # [end: intersection]

    # [fragment: print-intersection]
    if polygons_inside_region:
        area_inside_region = sum(polygon.get_area() for polygon in polygons_inside_region)
        area_inside_region_in_square_kms = area_inside_region * pygplates.Earth.mean_radius_in_kms**2
        num_interior_rings = sum(polygon.get_number_of_interior_rings() for polygon in polygons_inside_region)

        plate_id = reconstructed_geometry.get_feature().get_reconstruction_plate_id()
        print(f'Plate {plate_id} covers {area_inside_region_in_square_kms:.0f} square kms of the region, '
              f'in {len(polygons_inside_region)} polygons with {num_interior_rings} interior rings')
    # [end: print-intersection]

    # [fragment: difference]
    # Remove the reconstructed polygon from what is left of the region.
    remaining_uncovered_polygons = []
    for uncovered_polygon in uncovered_polygons:
        remaining_uncovered_polygons.extend(uncovered_polygon.difference(reconstructed_polygon))
    uncovered_polygons = remaining_uncovered_polygons
    # [end: difference]

# [fragment: print-difference]
uncovered_area = sum(polygon.get_area() for polygon in uncovered_polygons)
uncovered_area_in_square_kms = uncovered_area * pygplates.Earth.mean_radius_in_kms**2
region_area_in_square_kms = region.get_area() * pygplates.Earth.mean_radius_in_kms**2
print(f'No plate covers {uncovered_area_in_square_kms:.0f} of the {region_area_in_square_kms:.0f} square kms of the region, '
      f'in {len(uncovered_polygons)} polygons')
# [end: print-difference]
