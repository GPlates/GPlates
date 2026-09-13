"""
Generate the small fixtures that the documentation's sample scripts read.

The sample pages under 'doc-python-api/sample-code/' name their input files ('coastlines.gpml',
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
