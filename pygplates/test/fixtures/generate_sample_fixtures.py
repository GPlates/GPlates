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
committed. Re-running reproduces the real features, but the synthetic ones (the motion paths, the
velocity domain, the static polygons, the Southeast Indian Ridge in 'ridges.gpml' and
'sample-code/topologies.gpml') come back with new feature IDs. A later pyGPlates changes more: every
file gets its 'gpml:version' (1.6.0340 where most of these have 1.6.0339), and a property it writes
differently changes too (it no longer gives a motion path a reconstruction method). So restore
every file with git checkout unless it was meant to change.

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
# The source's ridges are time slices that all end by 40 Ma, so a sample that creates isochrons from 40 Ma
# to present day would find none alive: a synthetic Southeast Indian Ridge (Australia-Antarctica, drawn
# west to east so Australia is on its left) is added, from its opening at 83 Ma to present day.
southeast_indian_ridge = pygplates.Feature.create_tectonic_section(
    pygplates.FeatureType.gpml_mid_ocean_ridge,
    pygplates.PolylineOnSphere([(-50, 110), (-50.5, 120), (-51.5, 130), (-53, 140)]),
    name='Southeast Indian Ridge (synthetic)',
    valid_time=(83, pygplates.GeoTimeInstant.create_distant_future()),
    left_plate=801,
    right_plate=802,
    reconstruction_method='HalfStageRotationVersion3')
write('ridges.gpml', subset(BOUNDARIES, [901, 802, 701, 501], feature_type=pygplates.FeatureType.gpml_mid_ocean_ridge,
                            per_plate=3, key=lambda f: f.get_left_plate(), accept=finite_begin_time) +
      [southeast_indian_ridge])
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

# Static polygons: five synthetic plates tiling the sphere between +-80 degrees latitude at present
# day, so that the imported points and polylines below all get a plate ID. Four are boxes, and the
# fifth is a small block in a hole in the largest box: a polygon partitioned over the block gets a
# piece with an interior ring from the box around it.
#
# Reconstructed to a past time they move apart and over each other, as real static polygons do, which
# is what the samples that partition, or calculate velocities and net rotation, without topologies
# rely on. For example at 50 Ma, with Africa (701) as the anchor plate, the block has moved south
# across its hole: it covers the south of the hole (and the hole's southern edge as far east as
# longitude 87), lies over the box south of that, and leaves the north of the hole empty.
#
# An edge of a polygon is the shorter great circle arc between its two points, so each box has a
# point every 30 degrees along its northern and southern edges. Without them an edge that should run
# more than 180 degrees round a parallel runs the other way instead: with four points each, the
# Antarctic box is the same region as the Australian one, and the African one the same as the Pacific
# one, which leaves half the globe in no polygon.
def box(plate_id, lat0, lat1, lon0, lon1, hole=None):
    lons = list(range(lon0, lon1 + 1, 30))
    polygon = pygplates.PolygonOnSphere(
        [(lat0, lon) for lon in lons] + [(lat1, lon) for lon in reversed(lons)],
        [hole] if hole else [])
    return pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature, polygon,
        name='static polygon %d' % plate_id, valid_time=(200, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=plate_id)

# The block, and the hole it sits in at present day.
BLOCK_LAT0, BLOCK_LAT1, BLOCK_LON0, BLOCK_LON1 = 20, 50, 60, 90
static_polygons = [
    box(801, -80, 0, 90, 180),      # Australia-ish: south-east
    box(802, -80, 0, -180, 90),     # Antarctica-ish: the rest of the south
    box(901, 0, 80, -180, -60),     # Pacific-ish: north-west
    box(701, 0, 80, -60, 180,       # Africa/Eurasia-ish: north-east, with a hole where the block is
        hole=[(BLOCK_LAT0, BLOCK_LON0), (BLOCK_LAT0, BLOCK_LON1),
              (BLOCK_LAT1, BLOCK_LON1), (BLOCK_LAT1, BLOCK_LON0)]),
    box(501, BLOCK_LAT0, BLOCK_LAT1, BLOCK_LON0, BLOCK_LON1)]   # India-ish: the block in the hole
write('static_polygons.gpml', static_polygons)

# Topologies for the samples that resolve them. The shared 'topologies.gpml' beside this script belongs
# to the unit tests: its sections are all unclassified and its plates do not move, so a sample that
# looks for ridges or subduction zones would find none. This one is written to 'sample-code/', which
# 'sample_code_test.py' lays over the shared fixtures for every sample.
#
# Three rigid plates in bands across the western Pacific, in the direction the Pacific moves: Nazca
# (911); the East Pacific Rise; the Pacific (901); a trench; and Eurasia (301), which the Pacific
# subducts beneath. A deforming zone (a topological network) lies over the part of Eurasia beside the
# trench. The plates and the orientation were chosen by searching 'rotations.rot' for a trench that
# converges and a ridge that diverges at every time the samples visit (0 to 140 Ma). With plates that
# do not move consistently the samples would report ridges converging and trenches diverging, which is
# what they look for as anomalies. Beyond 140 Ma the layout is unchecked, and 'rotations.rot' thins
# out: Eurasia's chain to the anchor (through Africa) ends at 141 Ma, and the Pacific plates' at 240 Ma.
#
# Every boundary section of the plates is on plate 0, so the geometry stays put and the bands always
# close. That is enough for the samples, because what they measure comes from the topologies, not their
# sections: a rigid plate's velocity (and so the convergence across a boundary and the net rotation)
# comes from the plate ID of its topology, and a network's from its vertices. Sections on the real
# plates would drift apart by thousands of kilometres over the 140 My the samples cover, and the
# resolved boundaries would be stitched across the gaps.
#
# The deforming zone overlays Eurasia, as networks overlay plates in real models, where it used to sit
# between Eurasia and the trench. So three topologies share the trench (the Pacific on one side,
# Eurasia and the zone on the other), and the zone alone has its leading edge, which crosses Eurasia's
# interior. Neither is a gap or an overlap, which the gaps-and-overlaps sample has to get right. The
# zone runs the whole length of the trench, and has to: where a network and a plate are on the same
# side of a boundary the network's velocity is the one compared, and at the trench that is zero, which
# is what the search above was done with. With Eurasia's own velocity there, parts of the trench would
# diverge (by 3 cm/yr at 100 Ma). The zone keeps the plate ID of the Philippine Sea plate (608), which
# its interior once moved with. Nothing moves with that plate now: the ID tells the zone from Eurasia
# where a sample prints the plate ID of the overriding plate.
#
# The zone deforms from 30.5 Ma. Until then it is the band from the trench to a leading edge 10 degrees
# ahead, with a rigid block inside it, all on plate 0: nothing in it moves or strains, and a point
# reconstructed through the topologies stays where it is (a point in Eurasia outside the zone does
# not: from 141 Ma it moves with plate 301, and has left the model by 95 Ma). From 30.5 Ma the
# leading edge and the block move ahead with South America (201), at 1 to 2.5 cm/yr mid-zone, while
# the trench stays put. The zone widens to about 16 degrees at its middle, and the crust in it
# stretches: by a factor of 1.4 in mid-zone, and of 1.9 between the trench and the block, which takes
# up none of the stretching itself. That is what the crustal-thickness and strain samples reconstruct
# their points through.
#
# South America is here for its motion, not its geography. A plate's motion relative to the anchor is
# all that 'rotations.rot' offers, and at this place most plates wander, or run thousands of
# kilometres in 140 My. Plate 201 is one of the few that move steadily ahead, at a rift's speed, for
# tens of millions of years. Even it does not do so for long enough: it would carry the leading edge
# back across the trench by 70 Ma. Hence the handover. The leading edge and the block are each a pair
# of features with the same geometry at 30.5 Ma, one on plate 0 that ends then and one on plate 201
# that begins then ('fixed_then_moving()' below). The resolver is made for this: it leaves out a
# section whose feature does not exist at the time, so that another can take its place.
#
# Three things to know before changing the zone:
# - The handover is at 30.5 Ma because the samples visit whole millions of years. At 30.5 Ma itself
#   both features of each pair exist, and the zone resolves with two leading edges and two blocks.
# - The leading edge slides 3 to 5 degrees to the left as it moves ahead, and the zone's sides are
#   fixed. So the leading edge overhangs the sides by 10 degrees, and its points are 20 degrees apart,
#   which keeps them 4 degrees or more from a side at every time. A point that comes within a degree
#   or two of a side makes a sliver triangle with the corner: with points 5 degrees apart, one had 20
#   times the strain rate of its neighbours.
# - The same slide shortens the crust beside the zone's left side instead of stretching it (by present
#   day, within 4 degrees of the side). A sample that wants plain extension keeps away from the sides.
#
# The bands are laid out in a local frame whose "north" is the Pacific's direction of motion, then
# rotated so that frame is centred on the Pacific at the equator and the date line, facing azimuth 285
# (west-north-west).
TOPOLOGY_CENTRE_LAT, TOPOLOGY_CENTRE_LON = 0, 180
TOPOLOGY_AZIMUTH = 285
TOPOLOGY_HALF_WIDTH = 30
# The deforming zone's leading edge and rigid block stay put until this time (Ma), then move with this plate.
DEFORMING_ZONE_HANDOVER_TIME = 30.5
DEFORMING_ZONE_MOVING_PLATE = 201


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


def across_line(lat, half_width=TOPOLOGY_HALF_WIDTH, spacing=10):
    """A line across the Pacific's motion, drawn so that the band ahead of it is on its left."""
    return pygplates.PolylineOnSphere(to_model(
        [(lat, lon) for lon in range(-half_width, half_width + 1, spacing)]))


def section(feature_type, geometry, name, **properties):
    return pygplates.Feature.create_reconstructable_feature(
        feature_type, geometry, name=name, reconstruction_plate_id=0, **properties)


def band_sides(lat_ahead, lat_behind, band_name):
    """The two edges of a band that run along the Pacific's motion, as transforms."""
    return [section(pygplates.FeatureType.gpml_transform,
                    pygplates.PolylineOnSphere(to_model([(lat_ahead, lon), (lat_behind, lon)])),
                    '%s %s edge' % (band_name, side))
            for side, lon in (('left', -TOPOLOGY_HALF_WIDTH), ('right', TOPOLOGY_HALF_WIDTH))]


# The rotations the samples resolve the topologies with, to find where a moving feature is at present day.
rotation_model = pygplates.RotationModel(os.path.join(OUT, 'rotations.rot'))


def fixed_then_moving(geometry, name):
    """
    The two features of a geometry that stays put until the handover, then moves with a plate.

    'geometry' is where it is at the handover, which is the geometry of the fixed feature. The moving
    feature's present-day geometry is that, reverse reconstructed from the handover time.
    """
    fixed = section(pygplates.FeatureType.gpml_unclassified_feature, geometry,
                    '%s (until %g Ma)' % (name, DEFORMING_ZONE_HANDOVER_TIME),
                    valid_time=(pygplates.GeoTimeInstant.create_distant_past(), DEFORMING_ZONE_HANDOVER_TIME))
    moving = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature, geometry,
        name='%s (from %g Ma)' % (name, DEFORMING_ZONE_HANDOVER_TIME),
        valid_time=(DEFORMING_ZONE_HANDOVER_TIME, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=DEFORMING_ZONE_MOVING_PLATE,
        reverse_reconstruct=(rotation_model, DEFORMING_ZONE_HANDOVER_TIME))
    return [fixed, moving]


leading_edge = section(pygplates.FeatureType.gpml_unclassified_feature, across_line(40),
                       'leading edge of the model')
deforming_zone_edges = fixed_then_moving(across_line(30, half_width=40, spacing=20),
                                         'leading edge of the deforming zone')
# The block is to the right of the zone's middle, which leaves the middle to stretch evenly. From 12 Ma
# to present day it is over the velocity domain's point at (20, 150), so that the samples that
# calculate velocities at those points meet a rigid block as well as the zone's deforming region.
deforming_zone_blocks = fixed_then_moving(
    pygplates.PolygonOnSphere(to_model([(26, 6), (26, 20), (28.5, 20), (28.5, 6)])),
    'rigid block in the deforming zone')
# The deforming zone (and Eurasia, the overriding plate) is ahead of the trench, so on its left.
trench = section(pygplates.FeatureType.gpml_subduction_zone, across_line(20), 'trench',
                 other_properties=[(pygplates.PropertyName.gpml_subduction_polarity,
                                    pygplates.Enumeration(
                                        pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration'),
                                        'Left'))])
ridge = section(pygplates.FeatureType.gpml_mid_ocean_ridge, across_line(-20), 'East Pacific Rise')
trailing_edge = section(pygplates.FeatureType.gpml_unclassified_feature, across_line(-35),
                        'trailing edge of the model')

# Eurasia's sides are the deforming zone's too, as far as the zone's leading edge.
eurasia_sides = band_sides(40, 20, 'Eurasia')
pacific_sides = band_sides(20, -20, 'Pacific')
nazca_sides = band_sides(-20, -35, 'Nazca')


def topology(feature_type, topological_geometry_type, boundary, name, plate_id, interiors=()):
    """
    A topology whose boundary goes leading edge, right edge, trailing edge, left edge.

    The leading edge is a list: one feature, or the features that take its place at different times
    (a section whose feature does not exist at a time is left out of the boundary resolved then).
    """
    ahead, behind, (left, right) = boundary
    sections = [pygplates.GpmlTopologicalSection.create(feature, topological_geometry_type=topological_geometry_type)
                for feature in ahead + [right, behind, left]]
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
             ([leading_edge], trench, eurasia_sides), 'Eurasia', 301),
    topology(pygplates.FeatureType.gpml_topological_network, pygplates.GpmlTopologicalNetwork,
             (deforming_zone_edges, trench, eurasia_sides), 'deforming zone', 608,
             interiors=deforming_zone_blocks),
    topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
             ([trench], ridge, pacific_sides), 'Pacific', 901),
    topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
             ([ridge], trailing_edge, nazca_sides), 'Nazca', 911)]
write(os.path.join('sample-code', 'topologies.gpml'),
      [leading_edge, trench, ridge, trailing_edge] + deforming_zone_edges + deforming_zone_blocks +
      eurasia_sides + pacific_sides + nazca_sides + sample_topologies)

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
