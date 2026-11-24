import pygplates


# [fragment: load-inputs]
# Create a reconstruct model from the static polygon features and rotation file(s).
reconstruct_model = pygplates.ReconstructModel('static_polygons.gpml', 'rotations.rot')

# Load the features that contain the static points.
# These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_9_18.gpml')

# The static points.
domain_points = []
for velocity_domain_feature in velocity_domain_features:

    # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
    # Iterate over them all.
    for velocity_domain_geometry in velocity_domain_feature.get_geometries():

        domain_points.extend(velocity_domain_geometry.get_points())
# [end: load-inputs]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    # Get a snapshot of our static polygons reconstructed to the current 'time'.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)

    # [fragment: point-locations]
    # Find the reconstructed static polygon that contains each domain point (None if no polygon does).
    # Where reconstructed polygons overlap, this finds the one with the highest plate ID.
    domain_point_locations = reconstruct_snapshot.get_point_locations(domain_points)
    # [end: point-locations]

    # [fragment: point-locations-by-area]
    # Do the same but, where reconstructed polygons overlap, find the one with the largest area.
    domain_point_locations_by_area = reconstruct_snapshot.get_point_locations(
            domain_points,
            sort_reconstructed_static_polygons=pygplates.SortReconstructedStaticPolygons.by_plate_area)
    # [end: point-locations-by-area]

    # [fragment: reconstructed-polygons]
    # The reconstructed static polygons themselves (of the static polygon features that exist at 'time').
    reconstructed_static_polygons = reconstruct_snapshot.get_reconstructed_geometries()
    # [end: reconstructed-polygons]

    # [fragment: count-points]
    # Count the points that no polygon contains, the points that more than one polygon contains, and of
    # those, the points for which the two searches find different polygons.
    num_points_outside_polygons = 0
    num_points_in_overlapping_polygons = 0
    num_points_depending_on_search_order = 0
    for domain_point, reconstructed_static_polygon, reconstructed_static_polygon_by_area in zip(
            domain_points, domain_point_locations, domain_point_locations_by_area):

        if reconstructed_static_polygon is None:
            num_points_outside_polygons += 1
            continue

        # The number of reconstructed polygons that contain the point.
        num_containing_polygons = 0
        for reconstructed_polygon in reconstructed_static_polygons:
            polygon = reconstructed_polygon.get_reconstructed_geometry()
            if isinstance(polygon, pygplates.PolygonOnSphere) and polygon.is_point_in_polygon(domain_point):
                num_containing_polygons += 1

        if num_containing_polygons > 1:
            num_points_in_overlapping_polygons += 1
            if reconstructed_static_polygon != reconstructed_static_polygon_by_area:
                num_points_depending_on_search_order += 1
    # [end: count-points]

    print(f'Time {time}: of {len(domain_points)} points, {num_points_outside_polygons} are outside all polygons '
          f'and {num_points_in_overlapping_polygons} are inside more than one '
          f'({num_points_depending_on_search_order} of them in a different polygon when searched by area)')
