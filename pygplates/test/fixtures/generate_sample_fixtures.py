"""
Generate the small fixtures that the documentation's sample scripts read.

The sample pages under 'docs/pygplates/sample-code/' name their input files ('coastlines.gpml',
'isochrons.gpml', 'static_polygons.gpml', ...), and 'pygplates-sample-code-test' runs every sample
script in a directory seeded with this directory. These files are stand-ins that let the scripts run:
a few dozen small real features where a real source exists, synthetic geometry where one does not.
They are not the models the pages' "Output" blocks were captured from.

The real features are cut from the sample data installed with GPlates 2.5 (the EarthByte present-day
coastlines, the Seton et al. 2020 isochrons, the Zahirovic et al. 2022 plate-boundary geometries, the
Atlantic flowlines and the GPMDB virtual geomagnetic poles), keeping only features with few points, on a
handful of plates that 'rotations.rot' knows. Re-run this only to regenerate them; the outputs are
committed.

Usage: generate_sample_fixtures.py [<GeoData/FeatureCollections directory>]
"""

import math
import os
import sys

import pygplates

GEODATA = sys.argv[1] if len(sys.argv) > 1 else r'C:\Program Files\GPlates\GPlates 2.5.0\GeoData\FeatureCollections'
OUT = os.path.dirname(os.path.abspath(__file__))


def num_points(feature):
    return sum(len(g.get_points()) if hasattr(g, 'get_points') else 1 for g in feature.get_all_geometries())


def subset(path, plates, feature_type=None, max_points=20, per_plate=3, key=lambda f: f.get_reconstruction_plate_id(),
           accept=lambda f: True):
    """The first 'per_plate' features of each plate in 'plates' that are small and match."""
    taken = {plate: 0 for plate in plates}
    out = []
    for feature in pygplates.FeatureCollection(os.path.join(GEODATA, path)):
        plate = key(feature)
        if plate not in taken or taken[plate] >= per_plate:
            continue
        if feature_type and feature.get_feature_type() != feature_type:
            continue
        if num_points(feature) > max_points or not accept(feature):
            continue
        taken[plate] += 1
        # The shapefile attributes only repeat the properties beside them, at several times the size.
        feature.remove(pygplates.PropertyName.gpml_shapefile_attributes)
        out.append(feature)
    return out


def write(name, features):
    pygplates.FeatureCollection(features).write(os.path.join(OUT, name))
    print('%-40s %3d features' % (name, len(features)))


def finite_begin_time(feature):
    begin, end = feature.get_valid_time()
    return 0 < begin < 200


# Reconstructable features with plate IDs (coastlines are ClosedContinentalBoundary/IslandArc/Basin).
COASTLINES = r'Coastlines\Global_EarthByte_GPlates_PresentDay_Coastlines.gpmlz'
write('coastlines.gpml', subset(COASTLINES, [801, 802, 701, 501, 901, 201, 101]))
write('features.gpml', subset(COASTLINES, [801, 802, 901, 301, 302, 511], per_plate=2))

# Isochrons with conjugate plate IDs, on the Australia-Antarctica pair.
write('isochrons.gpml', subset(r'Isochrons\Seton_etal_2020_Isochrons.gpmlz', [801, 802],
                               feature_type=pygplates.FeatureType.gpml_isochron, max_points=30, per_plate=4,
                               accept=lambda f: f.get_conjugate_plate_id() in (801, 802)))

# Mid-ocean ridges (with left/right plates and a finite time of appearance) and subduction zones.
BOUNDARIES = r'DynamicPolygons\Zahirovic_etal_2022_Feature_Geometries.gpml'
# Ridges carry their plates as left/right rather than a reconstruction plate ID, so select on the left plate.
write('ridges.gpml', subset(BOUNDARIES, [901, 802, 701, 501], feature_type=pygplates.FeatureType.gpml_mid_ocean_ridge,
                            per_plate=3, key=lambda f: f.get_left_plate(), accept=finite_begin_time))
write('subduction_zones.gpml', subset(BOUNDARIES, [801, 901, 911, 301, 501],
                                      feature_type=pygplates.FeatureType.gpml_subduction_zone, per_plate=2))

# Virtual geomagnetic poles.
write('virtual_geomagnetic_poles.gpml', subset(r'Palaeomagnetism\vgp_features_gplates_default.gpmlz',
                                               [801, 701, 101], per_plate=2))

# Flowlines: the two smallest of the Atlantic flowlines (a flowline's time list makes even a
# three-seed feature sizeable in GPML). Two pages name the file differently.
flowlines = sorted(pygplates.FeatureCollection(os.path.join(GEODATA, r'Flowlines\AtlanticFlowlines.gpml')),
                   key=num_points)[:2]
write('flowlines.gpml', flowlines)
write('flowline_features.gpml', flowlines)

# Motion paths: synthetic, seeded on Australia moving relative to Antarctica.
motion_path = pygplates.Feature.create_motion_path(
    pygplates.MultiPointOnSphere([(-30, 130), (-25, 120), (-35, 145)]),
    list(range(0, 101, 10)),
    valid_time=(100, 0),
    relative_plate=802,
    reconstruction_plate_id=801,
    name='Australian motion paths relative to Antarctica')
write('motion_paths.gpml', [motion_path])
write('motion_path_features.gpml', [motion_path])

# A coarse velocity domain: a 10-degree lat/lon grid of points (9 x 18 = 162 points).
grid = [(lat, lon) for lat in range(-80, 81, 20) for lon in range(-170, 180, 20)]
write('lat_lon_velocity_domain_9_18.gpml', [pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.create_gpml('MeshNode'), pygplates.MultiPointOnSphere(grid),
    name='lat/lon velocity domain 9x18')])

# Static polygons: four synthetic plates tiling the sphere between +-80 degrees latitude, so that
# the imported points and polylines below all get a plate ID.
def box(plate_id, lat0, lat1, lon0, lon1):
    polygon = pygplates.PolygonOnSphere(
        [(lat0, lon0), (lat0, lon1), (lat1, lon1), (lat1, lon0)])
    return pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature, polygon,
        name='static polygon %d' % plate_id, valid_time=(200, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=plate_id)

write('static_polygons.gpml', [
    box(801, -80, 0, 90, 180),      # Australia-ish: south-east
    box(802, -80, 0, -180, 90),     # Antarctica-ish: the rest of the south
    box(901, 0, 80, -180, -60),     # Pacific-ish: north-west
    box(701, 0, 80, -60, 180)])     # Africa/Eurasia-ish: north-east

# Topologies for the samples that resolve them. The shared 'topologies.gpml' beside this script belongs
# to the unit tests: its sections are all unclassified and its plates do not move, so a sample that
# looks for ridges or subduction zones would find none. This one is written to 'sample-code/', which
# 'sample_code_test.py' lays over the shared fixtures for every sample.
#
# Four bands in the western Pacific, in the direction the Pacific moves: rigid Nazca (911); the East
# Pacific Rise; rigid Pacific (901); a trench where the Pacific subducts beneath a deforming zone that
# moves with the Philippine Sea plate (608); and rigid Eurasia (301). The plates and the orientation
# were chosen by searching 'rotations.rot' for a trench that converges and a ridge that diverges at
# every time the samples visit (0 to 140 Ma). With plates that do not move consistently the samples
# would report ridges converging and trenches diverging, which is what they look for as anomalies.
#
# Every boundary section is on plate 0, so the geometry stays put and the bands always close. That is
# enough for the samples, because what they measure comes from the topologies, not their sections: a
# rigid plate's velocity (and so the convergence across a boundary and the net rotation) comes from
# the plate ID of its topology, and a network's from its vertices. Sections on the real plates would
# drift apart by thousands of kilometres over the 140 My the samples cover, and the resolved
# boundaries would be stitched across the gaps. The deforming zone deforms because its interior
# points move with the Philippine Sea plate while its boundary stays still. Those points sit near the
# zone's leading edge: nearer the trench, the Philippine Sea plate's motion reaches the trench and makes
# parts of it diverge (with them 8 degrees from the trench, it converges everywhere, at every time).
#
# The bands are laid out in a local frame whose "north" is the Pacific's direction of motion, then
# rotated so that frame is centred on the Pacific at the equator and the date line, facing azimuth 285
# (west-north-west).
TOPOLOGY_CENTRE_LAT, TOPOLOGY_CENTRE_LON = 0, 180
TOPOLOGY_AZIMUTH = 285
TOPOLOGY_HALF_WIDTH = 30


def to_model(local_points):
    """Map (lat, lon) in the local frame (lat along the Pacific's motion) to points on the globe."""
    lat, lon = math.radians(TOPOLOGY_CENTRE_LAT), math.radians(TOPOLOGY_CENTRE_LON)
    azimuth = math.radians(TOPOLOGY_AZIMUTH)
    centre = pygplates.PointOnSphere(TOPOLOGY_CENTRE_LAT, TOPOLOGY_CENTRE_LON).to_xyz()
    north = (-math.sin(lat) * math.cos(lon), -math.sin(lat) * math.sin(lon), math.cos(lat))
    east = (-math.sin(lon), math.cos(lon), 0.0)
    forward = tuple(math.cos(azimuth) * n + math.sin(azimuth) * e for n, e in zip(north, east))
    # To the right of 'forward', as east is to the right of north.
    right = (forward[1] * centre[2] - forward[2] * centre[1],
             forward[2] * centre[0] - forward[0] * centre[2],
             forward[0] * centre[1] - forward[1] * centre[0])
    points = []
    for local_lat, local_lon in local_points:
        u, v, w = pygplates.PointOnSphere(local_lat, local_lon).to_xyz()
        points.append(tuple(u * c + v * r + w * f for c, r, f in zip(centre, right, forward)))
    return points


def across_line(lat):
    """A line across the Pacific's motion, drawn so that the band ahead of it is on its left."""
    return pygplates.PolylineOnSphere(to_model(
        [(lat, lon) for lon in range(-TOPOLOGY_HALF_WIDTH, TOPOLOGY_HALF_WIDTH + 1, 10)]))


def section(feature_type, geometry, name, **properties):
    return pygplates.Feature.create_reconstructable_feature(
        feature_type, geometry, name=name, reconstruction_plate_id=0, **properties)


def band_sides(lat_ahead, lat_behind, band_name):
    """The two edges of a band that run along the Pacific's motion, as transforms."""
    return [section(pygplates.FeatureType.gpml_transform,
                    pygplates.PolylineOnSphere(to_model([(lat_ahead, lon), (lat_behind, lon)])),
                    '%s %s edge' % (band_name, side))
            for side, lon in (('left', -TOPOLOGY_HALF_WIDTH), ('right', TOPOLOGY_HALF_WIDTH))]


leading_edge = section(pygplates.FeatureType.gpml_unclassified_feature, across_line(40),
                       'leading edge of the model')
deforming_zone_edge = section(pygplates.FeatureType.gpml_unclassified_feature, across_line(30),
                              'leading edge of the deforming zone')
# The deforming zone (the overriding plate) is ahead of the trench, so on its left.
trench = section(pygplates.FeatureType.gpml_subduction_zone, across_line(20), 'trench',
                 other_properties=[(pygplates.PropertyName.gpml_subduction_polarity,
                                    pygplates.Enumeration(
                                        pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration'),
                                        'Left'))])
ridge = section(pygplates.FeatureType.gpml_mid_ocean_ridge, across_line(-20), 'East Pacific Rise')
trailing_edge = section(pygplates.FeatureType.gpml_unclassified_feature, across_line(-35),
                        'trailing edge of the model')
deforming_zone_interior = pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.gpml_unclassified_feature,
    pygplates.MultiPointOnSphere(to_model([(28, lon) for lon in range(-25, 26, 10)])),
    name='deforming zone interior', reconstruction_plate_id=608)

eurasia_sides = band_sides(40, 30, 'Eurasia')
deforming_zone_sides = band_sides(30, 20, 'deforming zone')
pacific_sides = band_sides(20, -20, 'Pacific')
nazca_sides = band_sides(-20, -35, 'Nazca')


def topology(feature_type, topological_geometry_type, boundary, name, plate_id, interiors=()):
    """A topology whose boundary goes leading edge, right edge, trailing edge, left edge."""
    ahead, behind, (left, right) = boundary
    sections = [pygplates.GpmlTopologicalSection.create(feature, topological_geometry_type=topological_geometry_type)
                for feature in (ahead, right, behind, left)]
    if interiors:
        topological_geometry = topological_geometry_type(
            sections, [pygplates.GpmlTopologicalSection.create_network_interior(feature) for feature in interiors])
    else:
        topological_geometry = topological_geometry_type(sections)
    feature = pygplates.Feature.create_topological_feature(feature_type, topological_geometry, name=name)
    feature.set_reconstruction_plate_id(plate_id)
    return feature


closed_plate_boundary = pygplates.FeatureType.gpml_topological_closed_plate_boundary
sample_topologies = [
    topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
             (leading_edge, deforming_zone_edge, eurasia_sides), 'Eurasia', 301),
    topology(pygplates.FeatureType.gpml_topological_network, pygplates.GpmlTopologicalNetwork,
             (deforming_zone_edge, trench, deforming_zone_sides), 'deforming zone', 608,
             interiors=[deforming_zone_interior]),
    topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
             (trench, ridge, pacific_sides), 'Pacific', 901),
    topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
             (ridge, trailing_edge, nazca_sides), 'Nazca', 911)]
write(os.path.join('sample-code', 'topologies.gpml'),
      [leading_edge, deforming_zone_edge, trench, ridge, trailing_edge, deforming_zone_interior] +
      eurasia_sides + deforming_zone_sides + pacific_sides + nazca_sides + sample_topologies)

# Input text files for the import page: 'lon lat' per line (GMT order); polylines separated by '>'.
# The points file has no '>' headers in either form: pyGPlates' GMT reader takes a header followed
# by a single point as an (empty) polyline, whereas headerless lines load as one point feature each.
points = [(130, -30), (150, -25), (-120, 40), (20, 50), (-60, -40), (0, 0)]
for name in ('input_points.txt', 'input_points.gmt'):
    with open(os.path.join(OUT, name), 'w', newline='\n') as f:
        for lon, lat in points:
            f.write('%g %g\n' % (lon, lat))
polylines = [[(120, -35), (130, -30), (140, -28)], [(-130, 30), (-120, 40), (-110, 45)], [(10, 45), (20, 50)]]
for name in ('input_polylines.txt', 'input_polylines.gmt'):
    with open(os.path.join(OUT, name), 'w', newline='\n') as f:
        for polyline in polylines:
            f.write('>\n')
            for lon, lat in polyline:
                f.write('%g %g\n' % (lon, lat))
print('input_points.txt / .gmt, input_polylines.txt / .gmt')
