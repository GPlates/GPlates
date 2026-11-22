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
velocity domain, the static polygons, 'features_at_50Ma.gpml', the Southeast Indian Ridge in
'ridges.gpml' and the topologies under 'sample-code/') come back with new feature IDs. A later
pyGPlates changes more: every file gets its 'gpml:version' (1.6.0340 where most of these have
1.6.0339), and a property it writes differently changes too (it no longer gives a motion path a
reconstruction method). So restore every file with git checkout unless it was meant to change, or
run a copy of this script (with 'rotations.rot' beside it) in a scratch directory and copy back
only the files that were.

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
    path = os.path.join(OUT, name)
    # A scratch directory has no 'sample-code/' (or the per-script directories under it) to begin with.
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pygplates.FeatureCollection(features).write(path)
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
# one, which leaves half the globe in no polygon. The last point is always at 'lon1', so a box whose
# span is not a multiple of 30 degrees ends with a shorter step rather than short of 'lon1'.
def box(plate_id, lat0, lat1, lon0, lon1, hole=None):
    lons = list(range(lon0, lon1, 30)) + [lon1]
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

# Features to partition at a past time: their geometries are positions at 50 Ma in a reference frame
# with Africa (701) fixed, as if digitised over a reconstruction to that time. With that anchor plate
# the African box (and its hole) is where it is at present day, so these can be laid out against it.
# Each is there for one thing the partitioning sample shows:
# - a polygon around the whole hole: the box cuts a piece with an interior ring from it, the block
#   takes the part of the hole it still covers, and the rest of the hole is in no plate;
# - a profile (a polyline with a scalar value at each point) across the reconstructed eastern edge of
#   the Pacific box, which lies over the African box there: each piece keeps its part of the values;
# - a polygon south of the hole whose northern edge was drawn along the hole's southern edge, but 0.0003
#   degrees (about 30 metres) over it. That strip is a sliver on the block, which a partitioner drops
#   unless told to keep it.
def hole_southern_edge_latitude(lon):
    """The latitude of the hole's southern edge (a great circle arc, so it bows poleward) at 'lon'."""
    mid_lon = 0.5 * (BLOCK_LON0 + BLOCK_LON1)
    return math.degrees(math.atan(
        math.tan(math.radians(BLOCK_LAT0)) / math.cos(math.radians(BLOCK_LON1 - mid_lon)) *
        math.cos(math.radians(lon - mid_lon))))

SLIVER_OVERSHOOT_DEGREES = 0.0003
profile_points = [(30, lon) for lon in range(-45, -14, 5)]
write('features_at_50Ma.gpml', [
    pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature,
        pygplates.PolygonOnSphere([(15, 50), (15, 100), (55, 100), (55, 50)]),
        name='polygon over several plates', valid_time=(55, 45)),
    pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature,
        (pygplates.PolylineOnSphere(profile_points),
         {pygplates.ScalarType.create_gpml('Depth'): [100.0 * index for index in range(len(profile_points))]}),
        name='profile across a plate boundary', valid_time=(55, 45)),
    pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature,
        pygplates.PolygonOnSphere(
            [(11, 65), (11, 85)] +
            [(hole_southern_edge_latitude(lon) + SLIVER_OVERSHOOT_DEGREES, lon) for lon in range(85, 64, -5)]),
        name='polygon digitised along a plate boundary', valid_time=(55, 45))])

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

# Topologies for the one sample that calculates the velocities of the plate boundaries themselves.
# In the topologies above every boundary section but the deforming zone's leading edge is on plate 0,
# so that sample would report a velocity of zero along every plate boundary. Here the trench moves
# with Eurasia (301), the plate that overrides it, and the ridge spreads by half-stage rotation
# between the Pacific (901) and Nazca (911), as trenches and ridges do in real models.
#
# A section that moves no longer ends where its neighbours do, so here the trench and the ridge
# overhang the sides by 10 degrees, and the sides overhang them by 10 degrees: the sections cross
# wherever the moving ones have got to, and each resolved boundary is cut where they cross. (Sections
# that only met end to end, as above, are joined across the gap when one of them moves, and the
# resolved plates come out with pieces of their boundaries missing: Eurasia had a third of its area
# at 5 Ma.) That is enough for 30 My, in which the trench and the ridge each move 4 to 5 degrees.
# It is not a layout for the 140 My that the other samples cover. It is written to that sample's
# own directory, which 'sample_code_test.py' lays over the others for that script alone.
moving_trench = pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.gpml_subduction_zone, across_line(20, half_width=40), name='trench',
    reconstruction_plate_id=301,
    other_properties=[(pygplates.PropertyName.gpml_subduction_polarity,
                       pygplates.Enumeration(
                           pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration'),
                           'Left'))])
# The Pacific is ahead of the ridge, so on its left.
moving_ridge = pygplates.Feature.create_tectonic_section(
    pygplates.FeatureType.gpml_mid_ocean_ridge, across_line(-20, half_width=40), name='East Pacific Rise',
    left_plate=901, right_plate=911, reconstruction_method='HalfStageRotationVersion3')
long_eurasia_sides = band_sides(40, 10, 'Eurasia')
long_pacific_sides = band_sides(30, -30, 'Pacific')
long_nazca_sides = band_sides(-10, -35, 'Nazca')
plate_boundary_velocities_dir = os.path.join('sample-code', 'pygplates_calculate_velocities_of_plate_boundaries')
write(os.path.join(plate_boundary_velocities_dir, 'topologies.gpml'),
      [leading_edge, moving_trench, moving_ridge, trailing_edge] + deforming_zone_edges + deforming_zone_blocks +
      long_eurasia_sides + long_pacific_sides + long_nazca_sides + [
          topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
                   ([leading_edge], moving_trench, long_eurasia_sides), 'Eurasia', 301),
          topology(pygplates.FeatureType.gpml_topological_network, pygplates.GpmlTopologicalNetwork,
                   (deforming_zone_edges, moving_trench, long_eurasia_sides), 'deforming zone', 608,
                   interiors=deforming_zone_blocks),
          topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
                   ([moving_trench], moving_ridge, long_pacific_sides), 'Pacific', 901),
          topology(closed_plate_boundary, pygplates.GpmlTopologicalPolygon,
                   ([moving_ridge], trailing_edge, long_nazca_sides), 'Nazca', 911)])

# Topologies for the one sample that gives a rift an exponential stretching profile. The rift
# parameters need the plate IDs of the two plates that a rift is between, and a network whose
# un-stretched side moves exactly as one of those plates does: that is how the side is recognised
# (the stage rotation of each vertex is compared with those of the two plates).
#
# The deforming zone above can be given rift parameters (plates 0 and 201, which its two sides are
# on), and its triangulation is then refined. But it makes a poor example. Plate 0 is not a plate
# that a reader's rift is between. The zone is one network right across the stretching, where a rift
# is usually a network on each side of an axis. And its leading edge slides sideways as it moves
# ahead, so the profile across it is not a clean one.
#
# This is the rift between South America (201) and Africa (701), which 'rotations.rot' opens slowly
# from 131.7 to 120.4 Ma (before then the two plates are fixed to each other, and after it they
# spread fast). There is a network on each side of the rift axis, as rifts are usually modelled. Each
# is the strip between an un-stretched margin, a line on its plate, and the rift axis, a line that
# moves by half-stage rotation between the two plates and so stays midway between the margins. A
# strip has one row of triangles across it, which is the case the rift parameters are for: without
# them the strain rate is the same right across the strip.
#
# The three lines are laid out where they are at 122 Ma, the time the sample visits, as meridians 3
# degrees apart from 30S to 20S. At that time South America moves relative to Africa about a stage
# pole 36 degrees to the north (at 10N, 8E), so the rift opens nearly at right angles to the lines.
# (The Primer says that the profile works less well for a rift that opens obliquely.) The ends of the
# strips are not sections: each network's boundary is its margin and the axis, drawn in opposite
# directions, and the resolver joins the end of one to the start of the other.
#
# The networks have no rift parameters. Adding them is what the sample does. With them the strain rate
# across each strip at 122 Ma rises from 0.8e-15 to 1.4e-15 per second (it is 1.05e-15 to 1.1e-15
# without), and each strip has 25 to 33 triangles in place of 8. Each strip widens steadily from 131
# to 121 Ma, which is as far as this was checked. The features exist from 132 to 120 Ma.
RIFT_TIME = 122
RIFT_PLATES = 201, 701
RIFT_AXIS_LON = 0
RIFT_HALF_WIDTH = 3
RIFT_LATS = [-30, -27.5, -25, -22.5, -20]
RIFT_VALID_TIME = (132, 120)


def rift_line(lon, lats):
    return pygplates.PolylineOnSphere([(lat, lon) for lat in lats])


def rift_margin(name, lon, plate_id):
    """A line on a plate, drawn south to north where it is at the rift time."""
    return pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_unclassified_feature, rift_line(lon, RIFT_LATS),
        name=name, valid_time=RIFT_VALID_TIME, reconstruction_plate_id=plate_id,
        reverse_reconstruct=(rotation_model, RIFT_TIME))


def rift_network(margin, name, plate_id):
    """The network between an un-stretched margin and the rift axis. It has no rift parameters."""
    feature = pygplates.Feature.create_topological_network_feature(
        pygplates.GpmlTopologicalNetwork(
            [pygplates.GpmlTopologicalSection.create(section, topological_geometry_type=pygplates.GpmlTopologicalNetwork)
             for section in (margin, rift_axis)]),
        name=name, valid_time=RIFT_VALID_TIME)
    feature.set_reconstruction_plate_id(plate_id)
    return feature


south_american_margin = rift_margin('un-stretched margin of South America', RIFT_AXIS_LON - RIFT_HALF_WIDTH, RIFT_PLATES[0])
african_margin = rift_margin('un-stretched margin of Africa', RIFT_AXIS_LON + RIFT_HALF_WIDTH, RIFT_PLATES[1])
# Drawn north to south, so that a margin followed by the axis goes round its network.
rift_axis = pygplates.Feature.create_tectonic_section(
    pygplates.FeatureType.gpml_continental_rift, rift_line(RIFT_AXIS_LON, RIFT_LATS[::-1]),
    name='rift axis', valid_time=RIFT_VALID_TIME, left_plate=RIFT_PLATES[0], right_plate=RIFT_PLATES[1],
    reconstruction_method='HalfStageRotationVersion3',
    reverse_reconstruct=(rotation_model, RIFT_TIME))
rift_dir = os.path.join('sample-code', 'pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift')
write(os.path.join(rift_dir, 'topologies.gpml'),
      [south_american_margin, african_margin, rift_axis,
       rift_network(south_american_margin, 'South American side of the rift', RIFT_PLATES[0]),
       rift_network(african_margin, 'African side of the rift', RIFT_PLATES[1])])

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
