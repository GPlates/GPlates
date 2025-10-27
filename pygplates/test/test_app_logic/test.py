"""
Unit tests for the pygplates application logic API.
"""

import math
import os
import sys
import pickle
import shutil
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class CalculateVelocitiesTestCase(unittest.TestCase):
    def test_calculate_velocities(self):
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 10
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
                os.path.join(FIXTURES, 'volcanoes.gpml'),
                rotation_model,
                reconstructed_feature_geometries,
                reconstruction_time)
        for reconstructed_feature_geometry in reconstructed_feature_geometries:
            equivalent_stage_rotation = rotation_model.get_rotation(
                reconstruction_time,
                reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id(),
                reconstruction_time + 1)
            reconstructed_points = reconstructed_feature_geometry.get_reconstructed_geometry().get_points()
            velocities = pygplates.calculate_velocities(
                reconstructed_points,
                equivalent_stage_rotation,
                1,
                pygplates.VelocityUnits.cms_per_yr)
            self.assertTrue(len(velocities) == len(reconstructed_points))
            for index in range(len(velocities)):
                # The velocity direction should be orthogonal to the point direction (from globe origin).
                self.assertAlmostEqual(
                    pygplates.Vector3D.dot(velocities[index], reconstructed_points[index].to_xyz()), 0)


class CrossoverTestCase(unittest.TestCase):
    def test_find_crossovers(self):
        crossovers = pygplates.find_crossovers(os.path.join(FIXTURES, 'rotations.rot'))
        self.assertTrue(len(crossovers) == 133)

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            crossovers = pygplates.find_crossovers(FIXTURES / Path('rotations.rot'))
            self.assertTrue(len(crossovers) == 133)
        
        # TODO: Add more tests.

    def test_synchronise_crossovers(self):
        # This writes back to the rotation file.
        #pygplates.synchronise_crossovers(os.path.join(FIXTURES, 'rotations.rot'))
        
        # This does not write back to the rotation file.
        rotation_feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        crossover_results = []
        pygplates.synchronise_crossovers(
                rotation_feature_collection,
                lambda crossover: crossover.time < 600,
                0.01, # 2 decimal places
                pygplates.CrossoverType.synch_old_crossover_and_stages,
                crossover_results)
        # Due to filtering of crossover times less than 600Ma we have 123 instead of 134 crossovers.
        self.assertTrue(len(crossover_results) == 123)

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            import shutil
            # Copy 'rotations.rot' to 'tmp.rot'.
            tmp_rot_filename = FIXTURES / Path('tmp.rot')
            shutil.copyfile(FIXTURES / Path('rotations.rot'), tmp_rot_filename)
            # Modify 'tmp.rot' in place.
            crossover_results = []
            pygplates.synchronise_crossovers(
                    tmp_rot_filename,
                    lambda crossover: crossover.time < 600,
                    0.01, # 2 decimal places
                    pygplates.CrossoverType.synch_old_crossover_and_stages,
                    crossover_results)
            os.remove(tmp_rot_filename)  # remove 'tmp.rot'
            # Due to filtering of crossover times less than 600Ma we have 123 instead of 134 crossovers.
            self.assertTrue(len(crossover_results) == 123)
        
        # TODO: Add more tests.


class InterpolateTotalReconstructionSequenceTestCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))

    def test_get(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        next(feature_iter);
        next(feature_iter);
        feature = next(feature_iter)
        
        total_reconstruction_pole = feature.get_total_reconstruction_pole()
        self.assertTrue(total_reconstruction_pole)
        fixed_plate_id, moving_plate_id, total_reconstruction_pole_rotations = total_reconstruction_pole
        self.assertTrue(isinstance(total_reconstruction_pole_rotations, pygplates.GpmlIrregularSampling))
        self.assertEqual(fixed_plate_id, 901)
        self.assertEqual(moving_plate_id, 2)

    def test_set(self):
        gpml_irregular_sampling = pygplates.GpmlIrregularSampling(
                [pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                                pygplates.FiniteRotation(
                                        pygplates.PointOnSphere(1,0,0),
                                        0.4)),
                        0),
                pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                                pygplates.FiniteRotation(
                                        pygplates.PointOnSphere(0,1,0),
                                        0.5)),
                        10)])
        feature = pygplates.Feature(pygplates.FeatureType.create_gpml('TotalReconstructionSequence'))
        fixed_plate_property, moving_plate_property, total_pole_property = \
                feature.set_total_reconstruction_pole(901, 2, gpml_irregular_sampling)
        # Should have added three properties.
        self.assertTrue(len(feature) == 3)
        self.assertTrue(fixed_plate_property.get_value().get_plate_id() == 901)
        self.assertTrue(moving_plate_property.get_value().get_plate_id() == 2)
        interpolated_pole, interpolated_angle = total_pole_property.get_value(5).get_finite_rotation().get_euler_pole_and_angle()
        self.assertTrue(abs(interpolated_angle) > 0.322 and abs(interpolated_angle) < 0.323)

    def test_interpolate(self):
        # Get the third rotation feature (contains more interesting poles).
        feature_iter = iter(self.rotations)
        next(feature_iter);
        next(feature_iter);
        feature = next(feature_iter)
        
        total_reconstruction_pole = feature.get_total_reconstruction_pole()
        self.assertTrue(total_reconstruction_pole)
        fixed_plate_id, moving_plate_id, total_reconstruction_pole_rotations = total_reconstruction_pole
        interpolated_finite_rotation = total_reconstruction_pole_rotations.get_value(12.2).get_finite_rotation()
        self.assertEqual(fixed_plate_id, 901)
        self.assertEqual(moving_plate_id, 2)
        pole, angle = interpolated_finite_rotation.get_euler_pole_and_angle()
        self.assertTrue(abs(angle) > 0.1785 and abs(angle) < 0.179)
        # TODO: Compare pole.


class ReconstructModelTestCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        self.rotation_model = pygplates.RotationModel(self.rotations)

        self.reconstructable_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'volcanoes.gpml'))
        self.reconstruct_model = pygplates.ReconstructModel(self.reconstructable_features, self.rotation_model)

    def test_create(self):
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.ReconstructModel,
                'non_existant_reconstructable_file.gpml', self.rotations)

        self.assertTrue(self.reconstruct_model.get_anchor_plate_id() == 0)

        reconstruct_model = pygplates.ReconstructModel(self.reconstructable_features, self.rotation_model, anchor_plate_id=1)
        self.assertTrue(reconstruct_model.get_anchor_plate_id() == 1)

        # Make sure can specify a reconstruct snapshot cache size.
        reconstruct_model = pygplates.ReconstructModel(self.reconstructable_features, self.rotation_model, reconstruct_snapshot_cache_size=2)

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            reconstruct_model = pygplates.ReconstructModel(
                FIXTURES / Path('volcanoes.gpml'),
                FIXTURES / Path('rotations.rot'))

    def test_get_reconstruct_snapshot(self):
        reconstruct_snapshot = self.reconstruct_model.reconstruct_snapshot(10.5)  # note: it should allow a non-integral time
        self.assertTrue(reconstruct_snapshot.get_anchor_plate_id() == self.reconstruct_model.get_anchor_plate_id())
        self.assertTrue(reconstruct_snapshot.get_rotation_model() == self.reconstruct_model.get_rotation_model())

    def test_get_rotation_model(self):
        reconstruct_model = pygplates.ReconstructModel(self.reconstructable_features, self.rotation_model, anchor_plate_id=2)
        self.assertTrue(reconstruct_model.get_rotation_model().get_rotation(1.0, 802) == self.rotation_model.get_rotation(1.0, 802, anchor_plate_id=2))
        self.assertTrue(reconstruct_model.get_rotation_model().get_default_anchor_plate_id() == 2)

        rotation_model_anchor_2 = pygplates.RotationModel(self.rotations, default_anchor_plate_id=2)
        reconstruct_model = pygplates.ReconstructModel(self.reconstructable_features, rotation_model_anchor_2)
        self.assertTrue(reconstruct_model.get_anchor_plate_id() == 2)
        self.assertTrue(reconstruct_model.get_rotation_model().get_default_anchor_plate_id() == 2)
    
    def test_pickle(self):
        # Pickle a ReconstructModel.
        pickled_reconstruct_model = pickle.loads(pickle.dumps(self.reconstruct_model))
        self.assertTrue(pickled_reconstruct_model.get_rotation_model().get_rotation(100, 802) ==
                        self.reconstruct_model.get_rotation_model().get_rotation(100, 802))
        # Check snapshots of the original and pickled reconstruct models.
        reconstructed_geometries = self.reconstruct_model.reconstruct_snapshot(10.0).get_reconstructed_geometries(same_order_as_reconstructable_features=True)
        pickled_reconstructed_geometries = pickled_reconstruct_model.reconstruct_snapshot(10.0).get_reconstructed_geometries(same_order_as_reconstructable_features=True)
        self.assertTrue(len(pickled_reconstructed_geometries) == len(reconstructed_geometries))
        for index in range(len(pickled_reconstructed_geometries)):
            self.assertTrue(pickled_reconstructed_geometries[index].get_reconstructed_geometry() == reconstructed_geometries[index].get_reconstructed_geometry())


class ReconstructSnapshotTestCase(unittest.TestCase):
    def test(self):
        #
        # Class pygplates.ReconstructSnapshot is used internally by pygplates.reconstruct()
        # so most of its testing is already done by testing pygplates.reconstruct().
        #
        # Here we're just making sure we can access the pygplates.ReconstructSnapshot methods.
        #
        snapshot = pygplates.ReconstructSnapshot(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10),
            anchor_plate_id=1)
        self.assertTrue(snapshot.get_anchor_plate_id() == 1)
        self.assertTrue(snapshot.get_rotation_model())
        
        snapshot = pygplates.ReconstructSnapshot(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        
        self.assertTrue(snapshot.get_anchor_plate_id() == 0)
        self.assertTrue(snapshot.get_rotation_model())

        reconstructed_geometries = snapshot.get_reconstructed_geometries()
        self.assertTrue(len(reconstructed_geometries) == 4)  # See ReconstructTestCase
        
        snapshot.export_reconstructed_geometries(os.path.join(FIXTURES, 'reconstructed_geometries.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'reconstructed_geometries.gmt')))
        os.remove(os.path.join(FIXTURES, 'reconstructed_geometries.gmt'))

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            snapshot = pygplates.ReconstructSnapshot(
                FIXTURES / Path('volcanoes.gpml'),
                FIXTURES / Path('rotations.rot'),
                pygplates.GeoTimeInstant(10))
            
            self.assertTrue(snapshot.get_anchor_plate_id() == 0)
            self.assertTrue(snapshot.get_rotation_model())
    
    def test_get_reconstructed_features(self):
        # This example matches use of 'group_with_feature' in ReconstructTestCase.
        reconstruction_time = 15
        geometry = pygplates.PolylineOnSphere([(0,0), (10, 10)])
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                valid_time=(30, 0),
                reconstruction_plate_id=801)
        snapshot = pygplates.ReconstructSnapshot(
                feature,
                os.path.join(FIXTURES, 'rotations.rot'),
                reconstruction_time)
        
        grouped_reconstructed_feature_geometries = snapshot.get_reconstructed_features()
        self.assertTrue(len(grouped_reconstructed_feature_geometries) == 1)
        grouped_feature, reconstructed_feature_geometries = grouped_reconstructed_feature_geometries[0]
        self.assertTrue(grouped_feature.get_feature_id() == feature.get_feature_id())
        self.assertTrue(len(reconstructed_feature_geometries) == 1)
        self.assertTrue(geometry == reconstructed_feature_geometries[0].get_present_day_geometry())
    
    def test_point_locations_velocities(self):
        # We'll reconstruct a polyline and two polygons.
        # The polyline will get ignored (since it cannot contain points).
        polyline_1 = pygplates.PolylineOnSphere([(0,0), (10, 10)])
        polyline_1_feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                polyline_1, name='polyline_1', reconstruction_plate_id=1)
        polygon_1 = pygplates.PolygonOnSphere([(1,-32), (1,-28), (-1,-28), (-1,-32)])
        polygon_1_feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.gpml_unclassified_feature,
                polygon_1, name='polygon_1', reconstruction_plate_id=1)
        polygon_2 = pygplates.PolygonOnSphere([(1,31), (1,29), (-1,29), (-1,31)])  # smaller area than polygon_1
        polygon_2_feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.gpml_unclassified_feature,
                polygon_2, name='polygon_2', reconstruction_plate_id=2)
        reconstructable_features = [polyline_1_feature, polygon_1_feature, polygon_2_feature]

        # Create our own rotation model (for plate IDs 1 and 2 relative to 0).
        #
        # Both rotations have same velocity *magnitude* of 1 degree per Myr (just in different directions).
        velocity_magnitude_kms_per_my = math.radians(1) * pygplates.Earth.mean_radius_in_kms
        #
        # Plate ID 1 rotates *anti-clockwise* around North pole at 1 degree per Myr (going backward in time).
        # Note: This is *clockwise* going *forward* in time (used for velocities).
        rotation_time_samples_1 = [
                pygplates.GpmlTimeSample(
                    pygplates.GpmlFiniteRotation(pygplates.FiniteRotation((lat, lon), math.radians(angle))),
                    time)
                for time, lat, lon, angle in [(0, 90, 0, 0), (100, 90, 0, 100)]]
        rotation_feature_1 = pygplates.Feature.create_total_reconstruction_sequence(
            0, 1, pygplates.GpmlIrregularSampling(rotation_time_samples_1))
        # Plate ID 2 rotates *clockwise* around North pole at 1 degree per Myr (going backward in time).
        # Note: This is *anti-clockwise* going *forward* in time.
        rotation_time_samples_2 = [
                pygplates.GpmlTimeSample(
                    pygplates.GpmlFiniteRotation(pygplates.FiniteRotation((lat, lon), math.radians(angle))),
                    time)
                for time, lat, lon, angle in [(0, 90, 0, 0), (100, 90, 0, -100)]]
        rotation_feature_2 = pygplates.Feature.create_total_reconstruction_sequence(
            0, 2, pygplates.GpmlIrregularSampling(rotation_time_samples_2))
        rotation_model = pygplates.RotationModel([rotation_feature_1, rotation_feature_2])
        
        # Points to test.
        points = [
                pygplates.PointOnSphere(0, -30),
                (0, 0),
                pygplates.LatLonPoint(0, 30).to_xyz(),
        ]

        #
        # Reconstruct to 0 Ma.
        # Polygon 1 should contain 1st point.
        # Polygon 2 should contain 2nd point.
        #
        snapshot = pygplates.ReconstructSnapshot(
                reconstructable_features,
                rotation_model,
                reconstruction_time=0)
        point_locations = snapshot.get_point_locations(points)
        point_velocities, point_locations_from_vel = snapshot.get_point_velocities(points,
                                                                            # Also test velocity arguments get accepted...
                                                                            velocity_delta_time=1.0,
                                                                            velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t,
                                                                            velocity_units=pygplates.VelocityUnits.kms_per_my,
                                                                            earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms,
                                                                            return_point_locations=True)
        self.assertTrue(len(point_velocities) == len(point_locations_from_vel) == len(points))
        self.assertTrue(point_locations == point_locations_from_vel)
        # Polygon 1 contains 1st point.
        self.assertTrue(point_locations[0].get_feature().get_name() == 'polygon_1')
        self.assertTrue(point_locations[0].get_reconstructed_geometry() == polygon_1)
        # Compare velocities to only 4 decimal places.
        for vel_comp_calc, vel_comp_actual in zip(
                point_velocities[0].to_xyz(),
                # Velocity is *clockwise* (going forward in time)...
                (pygplates.FiniteRotation((90, 0), math.radians(-30)) * pygplates.Vector3D(0, -velocity_magnitude_kms_per_my, 0)).to_xyz()):
            self.assertAlmostEqual(vel_comp_calc, vel_comp_actual, places=4)
        # No polygon contains 2nd point.
        self.assertTrue(point_locations[1] is None)
        self.assertTrue(point_velocities[1] is None)
        # Polygon 2 contains 3rd point.
        self.assertTrue(point_locations[2].get_feature().get_name() == 'polygon_2')
        self.assertTrue(point_locations[2].get_reconstructed_geometry() == polygon_2)
        # Compare velocities to only 4 decimal places.
        for vel_comp_calc, vel_comp_actual in zip(
                point_velocities[2].to_xyz(),
                # Velocity is *anti-clockwise* (going forward in time)...
                (pygplates.FiniteRotation((90, 0), math.radians(30)) * pygplates.Vector3D(0, velocity_magnitude_kms_per_my, 0)).to_xyz()):
            self.assertAlmostEqual(vel_comp_calc, vel_comp_actual, places=4)

        #
        # Reconstruct to 30 Ma.
        # Polygon 1 should contain 3rd point.
        # Polygon 2 should contain 3rd point.
        #
        snapshot = pygplates.ReconstructSnapshot(
                reconstructable_features,
                rotation_model,
                reconstruction_time=30)
        
        #
        # Use original order of polygon features (polygon_1 then polygon_2).
        #
        sort_reconstructed_static_polygons = None
        point_locations = snapshot.get_point_locations(points,
                                                       sort_reconstructed_static_polygons=sort_reconstructed_static_polygons)
        point_velocities, point_locations_from_vel = snapshot.get_point_velocities(points,
                                                                                   sort_reconstructed_static_polygons=sort_reconstructed_static_polygons,
                                                                                   return_point_locations=True)
        self.assertTrue(len(point_velocities) == len(point_locations_from_vel) == len(points))
        self.assertTrue(point_locations == point_locations_from_vel)
        # No polygon contains 1st point.
        self.assertTrue(point_locations[0] is None)
        self.assertTrue(point_velocities[0] is None)
        # Both polygon 1 and 2 contain 2nd point (but polygon 1 wins since it's the first reconstructable polygon when snapshot created).
        self.assertTrue(point_locations[1].get_feature().get_name() == 'polygon_1')
        self.assertTrue(point_locations[1].get_reconstructed_geometry() == pygplates.FiniteRotation((90, 0), math.radians(30)) * polygon_1)
        # Compare velocities to only 4 decimal places.
        for vel_comp_calc, vel_comp_actual in zip(
                point_velocities[1].to_xyz(),
                # Velocity is *clockwise* (going forward in time)...
                pygplates.Vector3D(0, -velocity_magnitude_kms_per_my, 0).to_xyz()):
            self.assertAlmostEqual(vel_comp_calc, vel_comp_actual, places=4)
        # No polygon contains 3rd point.
        self.assertTrue(point_locations[2] is None)
        self.assertTrue(point_velocities[2] is None)
        
        #
        # Sort polygon features by plate ID (polygon_2 then polygon_1).
        #
        sort_reconstructed_static_polygons = pygplates.SortReconstructedStaticPolygons.by_plate_id
        point_locations = snapshot.get_point_locations(points,
                                                       sort_reconstructed_static_polygons=sort_reconstructed_static_polygons)
        point_velocities, point_locations_from_vel = snapshot.get_point_velocities(points,
                                                                                   sort_reconstructed_static_polygons=sort_reconstructed_static_polygons,
                                                                                   return_point_locations=True)
        self.assertTrue(len(point_velocities) == len(point_locations_from_vel) == len(points))
        self.assertTrue(point_locations == point_locations_from_vel)
        # No polygon contains 1st point.
        self.assertTrue(point_locations[0] is None)
        self.assertTrue(point_velocities[0] is None)
        # Both polygon 1 and 2 contain 2nd point (but polygon 2 wins since it has a higher plate ID).
        self.assertTrue(point_locations[1].get_feature().get_name() == 'polygon_2')
        self.assertTrue(point_locations[1].get_reconstructed_geometry() == pygplates.FiniteRotation((90, 0), math.radians(-30)) * polygon_2)
        # Compare velocities to only 4 decimal places.
        for vel_comp_calc, vel_comp_actual in zip(
                point_velocities[1].to_xyz(),
                # Velocity is *anti-clockwise* (going forward in time)...
                pygplates.Vector3D(0, velocity_magnitude_kms_per_my, 0).to_xyz()):
            self.assertAlmostEqual(vel_comp_calc, vel_comp_actual, places=4)
        # No polygon contains 3rd point.
        self.assertTrue(point_locations[2] is None)
        self.assertTrue(point_velocities[2] is None)
        
        #
        # Sort polygon features by plate area (polygon_1 then polygon_2).
        #
        sort_reconstructed_static_polygons = pygplates.SortReconstructedStaticPolygons.by_plate_area
        point_locations = snapshot.get_point_locations(points,
                                                       sort_reconstructed_static_polygons=sort_reconstructed_static_polygons)
        point_velocities, point_locations_from_vel = snapshot.get_point_velocities(points,
                                                                                   sort_reconstructed_static_polygons=sort_reconstructed_static_polygons,
                                                                                   return_point_locations=True)
        self.assertTrue(len(point_velocities) == len(point_locations_from_vel) == len(points))
        self.assertTrue(point_locations == point_locations_from_vel)
        # No polygon contains 1st point.
        self.assertTrue(point_locations[0] is None)
        self.assertTrue(point_velocities[0] is None)
        # Both polygon 1 and 2 contain 2nd point (but polygon 1 wins since it has a larger plate area).
        self.assertTrue(point_locations[1].get_feature().get_name() == 'polygon_1')
        self.assertTrue(point_locations[1].get_reconstructed_geometry() == pygplates.FiniteRotation((90, 0), math.radians(30)) * polygon_1)
        # Compare velocities to only 4 decimal places.
        for vel_comp_calc, vel_comp_actual in zip(
                point_velocities[1].to_xyz(),
                # Velocity is *clockwise* (going forward in time)...
                pygplates.Vector3D(0, -velocity_magnitude_kms_per_my, 0).to_xyz()):
            self.assertAlmostEqual(vel_comp_calc, vel_comp_actual, places=4)
        # No polygon contains 3rd point.
        self.assertTrue(point_locations[2] is None)
        self.assertTrue(point_velocities[2] is None)

    def test_reconstructed_export_files(self):
        reconstructable_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'volcanoes.gpml')) 
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        snapshot = pygplates.ReconstructSnapshot(
            reconstructable_features,
            rotation_model,
            pygplates.GeoTimeInstant(10))
        
        def _internal_test_export_files(
                test_case,
                snapshot,
                tmp_export_reconstructed_geometries_filename):
            
            def _remove_export(tmp_export_filename):
                os.remove(tmp_export_filename)

                # In case an OGR format file (which also has shapefile mapping XML file).
                if os.path.isfile(tmp_export_filename + '.gplates.xml'):
                    os.remove(tmp_export_filename + '.gplates.xml')
                
                # For Shapefile.
                if tmp_export_filename.endswith('.shp'):
                    tmp_export_base_filename = tmp_export_filename[:-len('.shp')]
                    if os.path.isfile(tmp_export_base_filename + '.dbf'):
                        os.remove(tmp_export_base_filename + '.dbf')
                    if os.path.isfile(tmp_export_base_filename + '.prj'):
                        os.remove(tmp_export_base_filename + '.prj')
                    if os.path.isfile(tmp_export_base_filename + '.shx'):
                        os.remove(tmp_export_base_filename + '.shx')
            
            tmp_export_reconstructed_geometries_filename = os.path.join(FIXTURES, tmp_export_reconstructed_geometries_filename)
            snapshot.export_reconstructed_geometries(tmp_export_reconstructed_geometries_filename)
            test_case.assertTrue(os.path.isfile(tmp_export_reconstructed_geometries_filename))

            # Read back in the exported file to make sure correct number of reconstructed geometries (except cannot read '.xy' files).
            if not tmp_export_reconstructed_geometries_filename.endswith('.xy'):
                reconstructed_features = pygplates.FeatureCollection(tmp_export_reconstructed_geometries_filename) 
                test_case.assertTrue(len(reconstructed_features) == len(snapshot.get_reconstructed_geometries()))
            
            _remove_export(tmp_export_reconstructed_geometries_filename)
        
        # Test reconstructed export to different format (eg, GMT, OGRGMT, Shapefile, etc).
        _internal_test_export_files(self, snapshot, 'tmp.xy')  # GMT
        _internal_test_export_files(self, snapshot, 'tmp.shp')  # Shapefile
        _internal_test_export_files(self, snapshot, 'tmp.gmt')  # OGRGMT
        _internal_test_export_files(self, snapshot, 'tmp.geojson')  # GeoJSON
        _internal_test_export_files(self, snapshot, 'tmp.json')  # GeoJSON

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            tmp_export_reconstructed_geometries_filename = FIXTURES / Path('tmp_export_reconstructed_geometries.gmt')
            snapshot.export_reconstructed_geometries(tmp_export_reconstructed_geometries_filename)
            self.assertTrue(tmp_export_reconstructed_geometries_filename.exists())
            tmp_export_reconstructed_geometries_filename.unlink()
    
    def test_pickle(self):
        snapshot = pygplates.ReconstructSnapshot(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        
        # Pickle the ReconstructSnapshot.
        pickled_snapshot = pickle.loads(pickle.dumps(snapshot))
        self.assertTrue(pickled_snapshot.get_rotation_model().get_rotation(100, 802) == snapshot.get_rotation_model().get_rotation(100, 802))
        # Check the original and pickled reconstruct snapshots.
        reconstructed_geometries = snapshot.get_reconstructed_geometries(same_order_as_reconstructable_features=True)
        pickled_reconstructed_geometries = pickled_snapshot.get_reconstructed_geometries(same_order_as_reconstructable_features=True)
        self.assertTrue(len(pickled_reconstructed_geometries) == len(reconstructed_geometries))
        for index in range(len(pickled_reconstructed_geometries)):
            self.assertTrue(pickled_reconstructed_geometries[index].get_reconstructed_geometry() == reconstructed_geometries[index].get_reconstructed_geometry())


class ReconstructTestCase(unittest.TestCase):
    def test_reconstruct(self):
        pygplates.reconstruct(
            os.path.join(FIXTURES, 'volcanoes.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            os.path.join(FIXTURES, 'test.xy'),
            pygplates.GeoTimeInstant(10))
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))
        os.remove(os.path.join(FIXTURES, 'test.xy'))
        self.assertFalse(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            output_path = FIXTURES / Path('test.xy')
            pygplates.reconstruct(
                FIXTURES / Path('volcanoes.gpml'),
                FIXTURES / Path('rotations.rot'),
                output_path,
                pygplates.GeoTimeInstant(10))
            
            self.assertTrue(output_path.exists())
            output_path.unlink()
            self.assertFalse(output_path.exists())
        
        feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'volcanoes.gpml'))
        self.assertEqual(len(feature_collection), 4)
        rotation_features = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml'), os.path.join(FIXTURES, 'volcanoes.gpml')],
            rotation_features,
            reconstructed_feature_geometries,
            0)
        # We've doubled up on the number of RFG's compared to number of features.
        self.assertEqual(len(reconstructed_feature_geometries), 2 * len(feature_collection))
        # The order of RFG's should match the order of features.
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
            # We've doubled up on the number of RFG's compared to number of features.
            self.assertTrue(reconstructed_feature_geometries[index + len(feature_collection)].get_feature().get_feature_id() == feature.get_feature_id())
        # Test queries on ReconstructedFeatureGeometry.
        rfg1 = reconstructed_feature_geometries[0]
        self.assertTrue(rfg1.get_feature())
        self.assertTrue(rfg1.get_property())
        self.assertTrue(isinstance(rfg1.get_present_day_geometry(), pygplates.PointOnSphere))
        self.assertTrue(isinstance(rfg1.get_reconstructed_geometry(), pygplates.PointOnSphere))
        
        # Reconstruct a feature collection.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            feature_collection,
            rotation_features,
            reconstructed_feature_geometries,
            0)
        self.assertEqual(len(reconstructed_feature_geometries), len(feature_collection))
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
        
        # Reconstruct a list of features.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [feature for feature in feature_collection],
            rotation_features,
            reconstructed_feature_geometries,
            0)
        self.assertEqual(len(reconstructed_feature_geometries), len(feature_collection))
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
        
        # Reconstruct individual features.
        for feature in feature_collection:
            reconstructed_feature_geometries = []
            pygplates.reconstruct(
                feature,
                rotation_features,
                reconstructed_feature_geometries,
                0)
            self.assertEqual(len(reconstructed_feature_geometries), 1)
            self.assertTrue(reconstructed_feature_geometries[0].get_feature().get_feature_id() == feature.get_feature_id())
        
        # Reconstruct a list that is a mixture of feature collection, filename, list of features and a feature.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [
                pygplates.FeatureCollectionFileFormatRegistry().read(os.path.join(FIXTURES, 'volcanoes.gpml')), # feature collection
                os.path.join(FIXTURES, 'volcanoes.gpml'), # filename
                [feature for feature in pygplates.FeatureCollectionFileFormatRegistry().read(os.path.join(FIXTURES, 'volcanoes.gpml'))], # list of features
                next(iter(pygplates.FeatureCollectionFileFormatRegistry().read(os.path.join(FIXTURES, 'volcanoes.gpml')))) # single feature
            ],
            rotation_features,
            reconstructed_feature_geometries,
            0)
        self.assertEqual(len(reconstructed_feature_geometries), 3 * len(feature_collection) + 1)
        # The order of RFG's should match the order of features (provided we don't duplicate the features across the lists - which
        # is why we loaded the features from scratch above instead of specifying derivatives of 'feature_collection').
        for index, feature in enumerate(feature_collection):
            self.assertTrue(reconstructed_feature_geometries[index].get_feature().get_feature_id() == feature.get_feature_id())
            self.assertTrue(reconstructed_feature_geometries[index + len(feature_collection)].get_feature().get_feature_id() == feature.get_feature_id())
            self.assertTrue(reconstructed_feature_geometries[index + 2 * len(feature_collection)].get_feature().get_feature_id() == feature.get_feature_id())
        self.assertTrue(reconstructed_feature_geometries[3 * len(feature_collection)].get_feature().get_feature_id() == next(iter(feature_collection)).get_feature_id())
        
        # Reconstruct to 15Ma.
        reconstructed_feature_geometries = []
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml')],
            [os.path.join(FIXTURES, 'rotations.rot')],
            reconstructed_feature_geometries,
            15)
        # One volcano does not exist at 15Ma.
        self.assertEqual(len(reconstructed_feature_geometries), 3)
        
    def test_reconstruct_feature_geometry(self):
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 15
        reconstruction_plate_id = 801
        geometry = pygplates.PolylineOnSphere([(0,0), (10, 10)])
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                valid_time=(30, 0),
                reconstruction_plate_id=reconstruction_plate_id)
        reconstructed_feature_geometries = []
        pygplates.reconstruct(feature, rotation_model, reconstructed_feature_geometries, reconstruction_time)
        self.assertEqual(len(reconstructed_feature_geometries), 1)
        reconstructed_feature_geometry = reconstructed_feature_geometries[0]
        self.assertTrue(reconstructed_feature_geometry.get_feature().get_feature_id() == feature.get_feature_id())
        self.assertTrue(geometry == reconstructed_feature_geometry.get_present_day_geometry())
        reconstructed_geometry = rotation_model.get_rotation(
            reconstructed_feature_geometry.get_reconstruction_time(),
            reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id()) * geometry
        self.assertTrue(reconstructed_geometry == reconstructed_feature_geometry.get_reconstructed_geometry())
        # Test reconstructed points and their velocities.
        self.assertTrue(reconstructed_geometry == pygplates.PolylineOnSphere(reconstructed_feature_geometry.get_reconstructed_geometry_points()))
        velocity_stage_rotation = rotation_model.get_rotation(
            reconstructed_feature_geometry.get_reconstruction_time(),
            reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id(),
            reconstructed_feature_geometry.get_reconstruction_time() + 1)
        velocities = pygplates.calculate_velocities(
            reconstructed_feature_geometry.get_reconstructed_geometry_points(),
            velocity_stage_rotation,
            1.0)
        self.assertTrue(velocities == reconstructed_feature_geometry.get_reconstructed_geometry_point_velocities())
        # Test grouping with feature.
        grouped_reconstructed_feature_geometries = []
        pygplates.reconstruct(feature, rotation_model, grouped_reconstructed_feature_geometries, reconstruction_time, group_with_feature=True)
        self.assertEqual(len(grouped_reconstructed_feature_geometries), 1)
        grouped_feature, reconstructed_feature_geometries = grouped_reconstructed_feature_geometries[0]
        self.assertTrue(grouped_feature.get_feature_id() == feature.get_feature_id())
        self.assertEqual(len(reconstructed_feature_geometries), 1)
        self.assertTrue(geometry == reconstructed_feature_geometries[0].get_present_day_geometry())
        
        # Test reverse reconstruction.
        geometry_at_reconstruction_time = geometry
        feature = pygplates.Feature.create_reconstructable_feature(
                pygplates.FeatureType.create_gpml('Coastline'),
                geometry,
                valid_time=(30, 0),
                reconstruction_plate_id=reconstruction_plate_id,
                reverse_reconstruct=(rotation_model, reconstruction_time))
        geometry_at_present_day = feature.get_geometry()
        reconstructed_feature_geometries = []
        pygplates.reconstruct(feature, rotation_model, reconstructed_feature_geometries, reconstruction_time)
        self.assertEqual(len(reconstructed_feature_geometries), 1)
        self.assertTrue(reconstructed_feature_geometries[0].get_feature().get_feature_id() == feature.get_feature_id())
        self.assertTrue(geometry_at_present_day == reconstructed_feature_geometries[0].get_present_day_geometry())
        self.assertTrue(geometry_at_reconstruction_time == reconstructed_feature_geometries[0].get_reconstructed_geometry())
        
    def test_reconstruct_flowline(self):
        pygplates.reconstruct(
            os.path.join(FIXTURES, 'flowline.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            os.path.join(FIXTURES, 'test.xy'),
            pygplates.GeoTimeInstant(10),
            reconstruct_type=pygplates.ReconstructType.flowline)
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))
        os.remove(os.path.join(FIXTURES, 'test.xy'))
        
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 15
        seed_points = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        flowline_feature = pygplates.Feature.create_flowline(
                seed_points,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                left_plate=201,
                right_plate=801)
        reconstructed_flowlines = []
        # First without specifying flowlines.
        pygplates.reconstruct(flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time)
        self.assertEqual(len(reconstructed_flowlines), 0)
        # Now specify flowlines.
        pygplates.reconstruct(
                flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.flowline)
        self.assertEqual(len(reconstructed_flowlines), 2)
        for index, reconstructed_flowline in enumerate(reconstructed_flowlines):
            self.assertTrue(reconstructed_flowline.get_feature().get_feature_id() == flowline_feature.get_feature_id())
            self.assertTrue(seed_points[index] == reconstructed_flowline.get_present_day_seed_point())
            # First point in left/right flowline is reconstructed seed point.
            self.assertTrue(reconstructed_flowline.get_left_flowline()[0] == reconstructed_flowline.get_reconstructed_seed_point())
            self.assertTrue(reconstructed_flowline.get_right_flowline()[0] == reconstructed_flowline.get_reconstructed_seed_point())
            # Should have non-zero velocity at reconstructed seed point
            self.assertTrue(reconstructed_flowline.get_reconstructed_seed_point_velocity() != pygplates.Vector3D.zero)
        
        # Test reverse reconstruction.
        seed_points_at_reconstruction_time = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        flowline_feature = pygplates.Feature.create_flowline(
                seed_points_at_reconstruction_time,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                left_plate=201,
                right_plate=801,
                reverse_reconstruct=(rotation_model, reconstruction_time))
        seed_points_at_present_day = flowline_feature.get_geometry()
        reconstructed_flowlines = []
        pygplates.reconstruct(
                flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.flowline)
        self.assertEqual(len(reconstructed_flowlines), 2)
        for index, reconstructed_flowline in enumerate(reconstructed_flowlines):
            self.assertTrue(reconstructed_flowline.get_feature().get_feature_id() == flowline_feature.get_feature_id())
            self.assertTrue(seed_points_at_present_day[index] == reconstructed_flowline.get_present_day_seed_point())
            self.assertTrue(seed_points_at_reconstruction_time[index] == reconstructed_flowline.get_reconstructed_seed_point())
            # At 15Ma there should be four points (15, 20, 30, 40).
            self.assertTrue(len(reconstructed_flowline.get_left_flowline()) == 4)
            self.assertTrue(len(reconstructed_flowline.get_right_flowline()) == 4)
            # First point in left/right flowline is reconstructed seed point.
            self.assertTrue(reconstructed_flowline.get_left_flowline()[0] == reconstructed_flowline.get_reconstructed_seed_point())
            self.assertTrue(reconstructed_flowline.get_right_flowline()[0] == reconstructed_flowline.get_reconstructed_seed_point())
        
    def test_reconstruct_motion_path(self):
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        reconstruction_time = 15
        seed_points = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        motion_path_feature = pygplates.Feature.create_motion_path(
                seed_points,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                relative_plate=201,
                reconstruction_plate_id=801)
        # First without specifying motion paths.
        reconstructed_motion_paths = []
        pygplates.reconstruct(motion_path_feature, rotation_model, reconstructed_motion_paths, reconstruction_time)
        self.assertEqual(len(reconstructed_motion_paths), 0)
        # Now specify motion paths.
        reconstructed_motion_paths = []
        pygplates.reconstruct(
                motion_path_feature, rotation_model, reconstructed_motion_paths, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.motion_path)
        self.assertEqual(len(reconstructed_motion_paths), 2)
        for index, reconstructed_motion_path in enumerate(reconstructed_motion_paths):
            self.assertTrue(reconstructed_motion_path.get_feature().get_feature_id() == motion_path_feature.get_feature_id())
            self.assertTrue(seed_points[index] == reconstructed_motion_path.get_present_day_seed_point())
            # Last point in motion path is reconstructed seed point.
            self.assertTrue(reconstructed_motion_path.get_motion_path()[-1] == reconstructed_motion_path.get_reconstructed_seed_point())
            # Should have non-zero velocity at reconstructed seed point
            self.assertTrue(reconstructed_motion_path.get_reconstructed_seed_point_velocity() != pygplates.Vector3D.zero)
        
        # Test reverse reconstruction.
        seed_points_at_reconstruction_time = pygplates.MultiPointOnSphere([(0,0), (0,90)])
        motion_path_feature = pygplates.Feature.create_motion_path(
                seed_points_at_reconstruction_time,
                [0, 10, 20, 30, 40],
                valid_time=(30, 0),
                relative_plate=201,
                reconstruction_plate_id=801,
                reverse_reconstruct=(rotation_model, reconstruction_time))
        seed_points_at_present_day = motion_path_feature.get_geometry()
        reconstructed_motion_paths = []
        pygplates.reconstruct(
                motion_path_feature, rotation_model, reconstructed_motion_paths, reconstruction_time,
                reconstruct_type=pygplates.ReconstructType.motion_path)
        self.assertEqual(len(reconstructed_motion_paths), 2)
        for index, reconstructed_motion_path in enumerate(reconstructed_motion_paths):
            self.assertTrue(reconstructed_motion_path.get_feature().get_feature_id() == motion_path_feature.get_feature_id())
            self.assertTrue(seed_points_at_present_day[index] == reconstructed_motion_path.get_present_day_seed_point())
            self.assertTrue(seed_points_at_reconstruction_time[index] == reconstructed_motion_path.get_reconstructed_seed_point())
            # At 15Ma there should be four points (15, 20, 30, 40).
            self.assertTrue(len(reconstructed_motion_path.get_motion_path()) == 4)
            # Last point in motion path is reconstructed seed point.
            self.assertTrue(reconstructed_motion_path.get_motion_path()[-1] == reconstructed_motion_path.get_reconstructed_seed_point())

    def test_reconstruct_export_files(self):
        reconstruct_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'volcanoes.gpml')) 
        self.assertTrue(len(reconstruct_features) == 4)
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        
        def _internal_test_export_files(test_case, reconstruct_features, rotation_model, tmp_export_filename):
            tmp_export_filename = os.path.join(FIXTURES, tmp_export_filename)
            
            pygplates.reconstruct(
                reconstruct_features,
                rotation_model,
                tmp_export_filename,
                pygplates.GeoTimeInstant(10))
            
            test_case.assertTrue(os.path.isfile(tmp_export_filename))
            
            # Read back in the exported file to make sure correct number of features (except cannot read '.xy' files).
            if not tmp_export_filename.endswith('.xy'):
                reconstruct_features = pygplates.FeatureCollection(tmp_export_filename) 
                test_case.assertTrue(len(reconstruct_features) == 4)
            
            os.remove(tmp_export_filename)

            # In case an OGR format file (which also has shapefile mapping XML file).
            if os.path.isfile(tmp_export_filename + '.gplates.xml'):
                os.remove(tmp_export_filename + '.gplates.xml')
            
            # For Shapefile.
            if tmp_export_filename.endswith('.shp'):
                tmp_export_base_filename = tmp_export_filename[:-len('.shp')]
                if os.path.isfile(tmp_export_base_filename + '.dbf'):
                    os.remove(tmp_export_base_filename + '.dbf')
                if os.path.isfile(tmp_export_base_filename + '.prj'):
                    os.remove(tmp_export_base_filename + '.prj')
                if os.path.isfile(tmp_export_base_filename + '.shx'):
                    os.remove(tmp_export_base_filename + '.shx')
        
        # Test reconstruct export to different format (eg, GMT, OGRGMT, Shapefile, etc).
        _internal_test_export_files(self, reconstruct_features, rotation_model, 'tmp.xy')  # GMT
        _internal_test_export_files(self, reconstruct_features, rotation_model, 'tmp.shp')  # Shapefile
        _internal_test_export_files(self, reconstruct_features, rotation_model, 'tmp.gmt')  # OGRGMT
        _internal_test_export_files(self, reconstruct_features, rotation_model, 'tmp.geojson')  # GeoJSON
        _internal_test_export_files(self, reconstruct_features, rotation_model, 'tmp.json')  # GeoJSON
        
    def test_deprecated_reconstruct(self):
        # We continue to support the deprecated version of 'reconstruct()' since
        # it was one of the few python API functions that's been around since
        # the dawn of time and is currently used in some web applications.
        pygplates.reconstruct(
            [os.path.join(FIXTURES, 'volcanoes.gpml')],
            [os.path.join(FIXTURES, 'rotations.rot')],
            10,
            0,
            os.path.join(FIXTURES, 'test.xy'))
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'test.xy')))
        os.remove(os.path.join(FIXTURES, 'test.xy'))

    def test_reverse_reconstruct(self):
        # Test modifying the feature collection file.
        # Modify a copy of the file.
        shutil.copyfile(os.path.join(FIXTURES, 'volcanoes.gpml'), os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))
        pygplates.reverse_reconstruct(
            os.path.join(FIXTURES, 'volcanoes_tmp.gpml'),
            [os.path.join(FIXTURES, 'rotations.rot')],
            pygplates.GeoTimeInstant(10))
        # Remove modify copy of the file.
        os.remove(os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))
        
        rotation_features = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        
        # Test modifying the feature collection only (not the file it was read from).
        reconstructable_feature_collection = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'volcanoes.gpml'))
        pygplates.reverse_reconstruct(
            reconstructable_feature_collection,
            [rotation_features],
            10,
            0)
        
        # Test modifying a list of features.
        pygplates.reverse_reconstruct(
            [feature for feature in reconstructable_feature_collection],
            rotation_features,
            10,
            0)
        
        # Test modifying a single feature only.
        pygplates.reverse_reconstruct(
            next(iter(reconstructable_feature_collection)),
            rotation_features,
            10,
            0)
            
        # Test modifying a mixture of the above.
        # Modify a copy of the file.
        shutil.copyfile(os.path.join(FIXTURES, 'volcanoes.gpml'), os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))
        pygplates.reverse_reconstruct([
                os.path.join(FIXTURES, 'volcanoes_tmp.gpml'),
                reconstructable_feature_collection,
                [feature for feature in reconstructable_feature_collection],
                next(iter(reconstructable_feature_collection))],
            rotation_features,
            10,
            0)
        # Remove modify copy of the file.
        os.remove(os.path.join(FIXTURES, 'volcanoes_tmp.gpml'))


class NetRotationTestCase(unittest.TestCase):
    def setUp(self):
        rotation_model = pygplates.RotationModel(pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot')))
        topologies = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        self.topological_model = pygplates.TopologicalModel(topologies, rotation_model)
        self.net_rotation_model = pygplates.NetRotationModel(self.topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t)
    
    def test_pickle(self):
        # Pickle a NetRotationModel.
        pickled_net_rotation_model = pickle.loads(pickle.dumps(self.net_rotation_model))
        self.assertTrue(pickled_net_rotation_model.get_topological_model().get_rotation_model().get_rotation(100, 802) ==
                        self.net_rotation_model.get_topological_model().get_rotation_model().get_rotation(100, 802))
        self.assertTrue(pickled_net_rotation_model.net_rotation_snapshot(10).get_total_net_rotation().get_finite_rotation() ==
                        self.net_rotation_model.net_rotation_snapshot(10).get_total_net_rotation().get_finite_rotation())
        # Pickle a NetRotationSnapshot.
        net_rotation_snapshot = self.net_rotation_model.net_rotation_snapshot(10)
        pickled_net_rotation_snapshot = pickle.loads(pickle.dumps(net_rotation_snapshot))
        self.assertTrue(pickled_net_rotation_snapshot.get_topological_snapshot().get_reconstruction_time() ==
                        net_rotation_snapshot.get_topological_snapshot().get_reconstruction_time())
        self.assertTrue(pickled_net_rotation_snapshot.get_total_net_rotation().get_finite_rotation() == net_rotation_snapshot.get_total_net_rotation().get_finite_rotation())
        # Pickle a NetRotation.
        total_net_rotation = net_rotation_snapshot.get_total_net_rotation()
        pickled_total_net_rotation = pickle.loads(pickle.dumps(total_net_rotation))
        self.assertTrue(pickled_total_net_rotation.get_finite_rotation() == total_net_rotation.get_finite_rotation())
        self.assertTrue(pickled_total_net_rotation.get_rotation_rate_vector() == total_net_rotation.get_rotation_rate_vector())
    
    def test_net_rotation(self):
        # Use the default 'num_samples_along_meridian' (180).
        net_rotation_snapshot = self.net_rotation_model.net_rotation_snapshot(0)
        total_net_rotation = net_rotation_snapshot.get_total_net_rotation()
        total_pole_latitude, total_pole_longitude, total_angle_degrees  = total_net_rotation.get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
        # These values were obtained from the GPlates net rotation export.
        self.assertAlmostEqual(total_pole_latitude, 15.4673, places=4)
        self.assertAlmostEqual(total_pole_longitude, -113.761, places=3)
        self.assertAlmostEqual(total_angle_degrees, 0.014637, places=6)
        # Test the individual net rotations of resolved topological boundaries and networks.
        total_net_rotation_accumulator = pygplates.NetRotation()  # zero net rotation
        for resolved_topology_net_rotation in net_rotation_snapshot.get_net_rotation().values():
            total_net_rotation_accumulator += resolved_topology_net_rotation
        self.assertTrue(total_net_rotation.get_finite_rotation() == total_net_rotation_accumulator.get_finite_rotation())
        # Test again but extracting each resolved topology's net rotation individually.
        total_net_rotation_accumulator = pygplates.NetRotation()  # zero net rotation
        for resolved_topology in net_rotation_snapshot.get_topological_snapshot().get_resolved_topologies():
            resolved_topology_net_rotation = net_rotation_snapshot.get_net_rotation(resolved_topology)
            # Not all resolved topologies in our topological snapshot will necessarily contribute net rotation.
            if resolved_topology_net_rotation:
                total_net_rotation_accumulator += resolved_topology_net_rotation
        self.assertTrue(total_net_rotation.get_finite_rotation() == total_net_rotation_accumulator.get_finite_rotation())
    
    def test_net_rotation_samples(self):
        # Create equal net rotation samples from a finite rotation (over 1Myr) and from an equivalent rotation rate vector.
        net_rotation_sample_from_finite_rotation = pygplates.NetRotation.create_sample_from_finite_rotation((10, 10), 0.01, pygplates.FiniteRotation((15, 15), 0.01))
        net_rotation_sample_from_rotation_rate = pygplates.NetRotation.create_sample_from_rotation_rate((10, 10), 0.01, 0.01 * pygplates.Vector3D(pygplates.LatLonPoint(15, 15).to_xyz()))
        self.assertTrue(net_rotation_sample_from_finite_rotation.get_finite_rotation() == net_rotation_sample_from_rotation_rate.get_finite_rotation())
        # Test addition of net rotation samples - both samples have the same net rotation so adding them results in an unchanged final net rotation.
        self.assertTrue(net_rotation_sample_from_finite_rotation.get_finite_rotation() ==
                        (net_rotation_sample_from_finite_rotation + net_rotation_sample_from_rotation_rate).get_finite_rotation())
        net_rotation_accumulator = pygplates.NetRotation()  # zero net rotation
        net_rotation_accumulator += net_rotation_sample_from_finite_rotation
        net_rotation_accumulator += net_rotation_sample_from_rotation_rate
        self.assertTrue(net_rotation_accumulator.get_finite_rotation() == net_rotation_sample_from_finite_rotation.get_finite_rotation())
        self.assertTrue(net_rotation_accumulator.get_finite_rotation() == net_rotation_sample_from_rotation_rate.get_finite_rotation())
        # While the net rotation is unchanged the area is doubled.
        self.assertTrue(net_rotation_accumulator.get_area() == 2 * net_rotation_sample_from_finite_rotation.get_area())
        self.assertTrue(net_rotation_accumulator.get_area() == 2 * net_rotation_sample_from_rotation_rate.get_area())
        # Use the default 'num_samples_along_meridian' (180).
        total_net_rotation = self.net_rotation_model.net_rotation_snapshot(0).get_total_net_rotation()
        total_net_rotation_clone = self.net_rotation_model.net_rotation_snapshot(0).get_total_net_rotation()
        # Ensure modifying a net rotation changes the original object (ie, doesn't create a new one via addition).
        total_net_rotation_before_modification = total_net_rotation
        total_net_rotation += net_rotation_sample_from_finite_rotation
        self.assertTrue(total_net_rotation_before_modification.get_finite_rotation() == total_net_rotation.get_finite_rotation())
        self.assertTrue(total_net_rotation.get_finite_rotation() != total_net_rotation_clone.get_finite_rotation())  # make sure net rotation actually changed
    
    def test_net_rotation_conversion(self):
        total_net_rotation = self.net_rotation_model.net_rotation_snapshot(0).get_total_net_rotation()
        total_net_finite_rotation = total_net_rotation.get_finite_rotation()
        total_net_rotation_rate_vector = total_net_rotation.get_rotation_rate_vector()
        # Compare the pole and angle from rotation rate vector with the finite rotation.
        total_net_finite_rotation_pole, total_net_finite_rotation_angle = total_net_finite_rotation.get_euler_pole_and_angle()
        total_net_rotation_rate_vector_pole = pygplates.PointOnSphere(total_net_rotation_rate_vector.to_normalized().to_xyz())
        total_net_rotation_rate_vector_angle = total_net_rotation_rate_vector.get_magnitude()
        self.assertTrue(total_net_finite_rotation_pole == total_net_rotation_rate_vector_pole)
        self.assertAlmostEqual(total_net_finite_rotation_angle, total_net_rotation_rate_vector_angle)
        # Test conversion between rotation rate vector and finite rotation.
        self.assertTrue(total_net_finite_rotation == pygplates.NetRotation.convert_rotation_rate_vector_to_finite_rotation(total_net_rotation_rate_vector))
        self.assertTrue(total_net_rotation_rate_vector == pygplates.NetRotation.convert_finite_rotation_to_rotation_rate_vector(total_net_finite_rotation))
        total_net_finite_rotation_over_10myr = pygplates.FiniteRotation(total_net_finite_rotation_pole, 10 * total_net_finite_rotation_angle)
        self.assertTrue(total_net_finite_rotation_over_10myr == pygplates.NetRotation.convert_rotation_rate_vector_to_finite_rotation(total_net_rotation_rate_vector, 10))
        self.assertTrue(total_net_rotation_rate_vector == pygplates.NetRotation.convert_finite_rotation_to_rotation_rate_vector(total_net_finite_rotation_over_10myr, 10))
    
    def test_arbitrary_point_distribution(self):
        # Test an arbitrary point distribution.
        # We actually use the same uniform lat-lon distribution used internally when explicitly specifying 'num_samples_along_meridian'.
        # In which case we should get the same total net rotation result.
        point_distribution = []
        num_samples_along_meridian = 180  # same as the default 'num_samples_along_meridian' (if 'point_distribution' were not to be specified below)
        delta_in_degrees = 180.0 / num_samples_along_meridian
        delta_in_radians = math.radians(delta_in_degrees)
        for lat_index in range(num_samples_along_meridian):
            lat = -90.0 + (lat_index + 0.5) * delta_in_degrees
            sample_area_radians = math.cos(math.radians(lat)) * delta_in_radians * delta_in_radians
            for lon_index in range(2*num_samples_along_meridian):
                lon = -180.0 + (lon_index + 0.5) * delta_in_degrees
                point_distribution.append(((lat, lon), sample_area_radians))
        
        net_rotation_model = pygplates.NetRotationModel(self.topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t, point_distribution=point_distribution)
        total_net_rotation = net_rotation_model.net_rotation_snapshot(0).get_total_net_rotation()
        total_pole_latitude, total_pole_longitude, total_angle_degrees  = total_net_rotation.get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
        # These values were obtained from the GPlates net rotation export.
        self.assertAlmostEqual(total_pole_latitude, 15.4673, places=4)
        self.assertAlmostEqual(total_pole_longitude, -113.761, places=3)
        self.assertAlmostEqual(total_angle_degrees, 0.014637, places=6)


class PlatePartitionerTestCase(unittest.TestCase):
    def setUp(self):
        self.topological_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        self.rotation_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))

    def test_partition_exceptions(self):
        rotation_model = pygplates.RotationModel(self.rotation_features)
        resolved_topologies = []
        pygplates.resolve_topologies(
            self.topological_features,
            rotation_model,
            resolved_topologies,
            0)
        
        # Can have no reconstruction geometries.
        plate_partitioner = pygplates.PlatePartitioner([], rotation_model)
        self.assertFalse(plate_partitioner.partition_point(pygplates.PointOnSphere(0, 0)))
        
        # All reconstruction times must be the same.
        resolved_topologies_10 = []
        pygplates.resolve_topologies(
            self.topological_features,
            self.rotation_features,
            resolved_topologies_10,
            10)
        self.assertRaises(pygplates.DifferentTimesInPartitioningPlatesError,
                pygplates.PlatePartitioner, resolved_topologies + resolved_topologies_10, rotation_model)

    def test_sort(self):
        rotation_model = pygplates.RotationModel(self.rotation_features)
        resolved_topologies = []
        pygplates.resolve_topologies(
            self.topological_features,
            rotation_model,
            resolved_topologies,
            0)
        
        # Pick a polyline that intersects all three topology regions.
        # We'll use this to verify the sort order of partitioning geometries.
        polyline = pygplates.PolylineOnSphere([(0,0), (0,-30), (30,-30), (30,-90)])
        
        # Unsorted.
        plate_partitioner = pygplates.PlatePartitioner(self.topological_features, rotation_model, sort_partitioning_plates=None)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Should be in original order.
        for recon_geom_index, (recon_geom, inside_geoms) in enumerate(partitioned_inside_geometries):
            self.assertTrue(recon_geom.get_feature().get_feature_id() ==
                    resolved_topologies[recon_geom_index].get_feature().get_feature_id())
        
        # Unsorted.
        plate_partitioner = pygplates.PlatePartitioner(resolved_topologies, rotation_model, sort_partitioning_plates=None)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Should be in original order.
        for recon_geom_index, (recon_geom, inside_geoms) in enumerate(partitioned_inside_geometries):
            self.assertTrue(recon_geom.get_feature().get_feature_id() ==
                    resolved_topologies[recon_geom_index].get_feature().get_feature_id())
        
        # Sorted by partition type.
        resolved_topologies_copy = resolved_topologies[:]
        if isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork):
            # Move to end of list.
            resolved_topologies_copy.append(resolved_topologies_copy[0])
            del resolved_topologies_copy[0]
        self.assertFalse(isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork))
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Topological network always comes first.
        self.assertTrue(isinstance(partitioned_inside_geometries[0][0], pygplates.ResolvedTopologicalNetwork))
        
        # Sorted by partition type then plate ID.
        #
        # Sort in opposite plate ID order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_feature().get_reconstruction_plate_id())
        if isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork):
            # Move to end of list.
            resolved_topologies_copy.append(resolved_topologies_copy[0])
            del resolved_topologies_copy[0]
        self.assertFalse(isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork))
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_id)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Topological network always comes first.
        self.assertTrue(isinstance(partitioned_inside_geometries[0][0], pygplates.ResolvedTopologicalNetwork))
        # Then the two resolved boundaries are sorted by plate ID.
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 2)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 1)
        
        # Sorted by partition type then plate area.
        #
        # Sort in opposite plate area order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_resolved_boundary().get_area())
        if isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork):
            # Move to end of list.
            resolved_topologies_copy.append(resolved_topologies_copy[0])
            del resolved_topologies_copy[0]
        self.assertFalse(isinstance(resolved_topologies_copy[0], pygplates.ResolvedTopologicalNetwork))
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_area)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Topological network always comes first.
        self.assertTrue(isinstance(partitioned_inside_geometries[0][0], pygplates.ResolvedTopologicalNetwork))
        # Then the two resolved boundaries are sorted by plate area (not plate ID).
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 2)
        
        # Sorted by plate ID.
        #
        # Sort in opposite plate ID order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_feature().get_reconstruction_plate_id())
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_plate_id)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        self.assertTrue(partitioned_inside_geometries[0][0].get_feature().get_reconstruction_plate_id() == 2)
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 0)
        
        # Sorted by plate area.
        #
        # Sort in opposite plate area order.
        resolved_topologies_copy = sorted(resolved_topologies, key = lambda rg: rg.get_resolved_boundary().get_area())
        plate_partitioner = pygplates.PlatePartitioner(
                resolved_topologies_copy, rotation_model, sort_partitioning_plates=pygplates.SortPartitioningPlates.by_plate_area)
        partitioned_inside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        self.assertTrue(partitioned_inside_geometries[0][0].get_feature().get_reconstruction_plate_id() == 0)
        self.assertTrue(partitioned_inside_geometries[1][0].get_feature().get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_inside_geometries[2][0].get_feature().get_reconstruction_plate_id() == 2)

    def test_partition_features(self):
        plate_partitioner = pygplates.PlatePartitioner(self.topological_features, self.rotation_features)
        
        # Partition inside point.
        point_feature = pygplates.Feature()
        point_feature.set_geometry(pygplates.PointOnSphere(0, -30))
        partitioned_features = plate_partitioner.partition_features(
            point_feature,
            properties_to_copy = [
                pygplates.PartitionProperty.reconstruction_plate_id,
                pygplates.PartitionProperty.valid_time_end])
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_reconstruction_plate_id() == 1)
        # Only the end time (0Ma) should have changed.
        self.assertTrue(partitioned_features[0].get_valid_time() ==
            (pygplates.GeoTimeInstant.create_distant_past(), 0))
        
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            point_feature,
            properties_to_copy = [
                pygplates.PartitionProperty.reconstruction_plate_id,
                pygplates.PartitionProperty.valid_time_period,
                pygplates.PropertyName.gml_name],
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(unpartitioned_features) == 0)
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_reconstruction_plate_id() == 1)
        self.assertTrue(partitioned_features[0].get_name() == 'topology2')
        # Both begin and end time should have changed.
        self.assertTrue(partitioned_features[0].get_valid_time() == (100,0))
        
        partitioned_groups, unpartitioned_features = plate_partitioner.partition_features(
            point_feature,
            properties_to_copy = [
                pygplates.PartitionProperty.reconstruction_plate_id,
                pygplates.PartitionProperty.valid_time_begin],
            partition_return = pygplates.PartitionReturn.partitioned_groups_and_unpartitioned)
        self.assertTrue(len(unpartitioned_features) == 0)
        self.assertTrue(len(partitioned_groups) == 1)
        self.assertTrue(partitioned_groups[0][0].get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(partitioned_groups[0][1]) == 1)
        self.assertTrue(partitioned_groups[0][1][0].get_reconstruction_plate_id() == 1)
        # Only the begin time (100Ma) should have changed.
        self.assertTrue(partitioned_groups[0][1][0].get_valid_time() == (100, pygplates.GeoTimeInstant.create_distant_future()))
        
        # Partition inside and outside point.
        inside_point_feature = pygplates.Feature()
        inside_point_feature.set_geometry(pygplates.PointOnSphere(0, -30))
        outside_point_feature = pygplates.Feature()
        outside_point_feature.set_geometry(pygplates.PointOnSphere(0, 0))
        def test_set_name(partitioning_feature, feature):
            try:
                feature.set_name(partitioning_feature.get_name())
            except pygplates.InformationModelError:
                pass
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            [inside_point_feature, outside_point_feature],
            properties_to_copy = test_set_name,
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(unpartitioned_features) == 1)
        self.assertTrue(unpartitioned_features[0].get_name() == '')
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_name() == 'topology2')
        
        # Partition VGP feature - average sample site position is inside and pole position is outside.
        vgp_feature = pygplates.Feature(pygplates.FeatureType.gpml_virtual_geomagnetic_pole)
        vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, -30), pygplates.PropertyName.gpml_average_sample_site_position)
        vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, 0), pygplates.PropertyName.gpml_pole_position)
        features = plate_partitioner.partition_features(vgp_feature)
        self.assertTrue(len(features) == 1)
        self.assertTrue(features[0].get_reconstruction_plate_id() == 1)
        # Move average sample site position outside.
        vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, 0), pygplates.PropertyName.gpml_average_sample_site_position)
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            vgp_feature,
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(partitioned_features) == 0)
        self.assertTrue(len(unpartitioned_features) == 1)
        self.assertTrue(unpartitioned_features[0].get_reconstruction_plate_id() == 0)
        # Again but not a VGP feature - should get split into two features.
        non_vgp_feature = pygplates.Feature()
        non_vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, -30), pygplates.PropertyName.gpml_average_sample_site_position)
        non_vgp_feature.set_geometry(
            pygplates.PointOnSphere(0, 0), pygplates.PropertyName.gpml_pole_position)
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
            non_vgp_feature,
            partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        self.assertTrue(len(partitioned_features) == 1)
        self.assertTrue(partitioned_features[0].get_reconstruction_plate_id() == 1)
        self.assertTrue(len(unpartitioned_features) == 1)
        self.assertTrue(unpartitioned_features[0].get_reconstruction_plate_id() == 0)

    def test_partition_geometry(self):
        plate_partitioner = pygplates.PlatePartitioner(self.topological_features, self.rotation_features)
        
        # Test optional arguments.
        point = pygplates.PointOnSphere(0, -30)
        self.assertTrue(plate_partitioner.partition_geometry(point))
        partitioned_inside_geometries = []
        self.assertTrue(plate_partitioner.partition_geometry(point, partitioned_inside_geometries))
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        partitioned_outside_geometries = []
        self.assertTrue(plate_partitioner.partition_geometry(point, partitioned_outside_geometries=partitioned_outside_geometries))
        self.assertFalse(partitioned_outside_geometries)
        
        # Partition inside point.
        point = pygplates.PointOnSphere(0, -30)
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(point, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_points = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_points) == 1)
        self.assertTrue(inside_points[0] == point)
        
        # Partition outside point.
        point = pygplates.PointOnSphere(0, 0)
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(point, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_point = partitioned_outside_geometries[0]
        self.assertTrue(outside_point == point)
        
        # Partition inside and outside point.
        inside_point = pygplates.PointOnSphere(0, -30)
        outside_point = pygplates.PointOnSphere(0, 0)
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry([inside_point, outside_point], partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_points = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_points) == 1)
        self.assertTrue(inside_points[0] == inside_point)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        self.assertTrue(partitioned_outside_geometries[0] == outside_point)
        
        # Partition inside multipoint.
        multipoint = pygplates.MultiPointOnSphere([(15,-30), (0,-30)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == multipoint)
        
        # Partition outside multipoint.
        multipoint = pygplates.MultiPointOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == multipoint)
        
        # Partition intersecting multipoint.
        multipoint = pygplates.MultiPointOnSphere([(0,-30), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == pygplates.MultiPointOnSphere([(0,-30)]))
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == pygplates.MultiPointOnSphere([(0,0)]))
        
        # Partition intersecting multipoint.
        multipoint = pygplates.MultiPointOnSphere([(30,-30), (0,-30), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(multipoint, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 2)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        recon_geom1, inside_geoms1 = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom1.get_feature().get_feature_id().get_string() == 'GPlates-a6054d82-6e6d-4f59-9d24-4ab255ece477')
        self.assertTrue(len(inside_geoms1) == 1)
        self.assertTrue(inside_geoms1[0] == pygplates.MultiPointOnSphere([(30,-30)]))
        recon_geom2, inside_geoms2 = partitioned_inside_geometries[1]
        self.assertTrue(recon_geom2.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms2) == 1)
        self.assertTrue(inside_geoms2[0] == pygplates.MultiPointOnSphere([(0,-30)]))
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == pygplates.MultiPointOnSphere([(0,0)]))
        
        # Partition inside polyline.
        polyline = pygplates.PolylineOnSphere([(15,-30), (0,-30)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == polyline)
        
        # Partition outside polyline.
        polyline = pygplates.PolylineOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == polyline)
        
        # Partition inside and outside polyline.
        inside_polyline = pygplates.PolylineOnSphere([(15,-30), (0,-30)])
        outside_polyline = pygplates.PolylineOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry((inside_polyline, outside_polyline), partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == inside_polyline)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        self.assertTrue(partitioned_outside_geometries[0] == outside_polyline)
        
        # Partition outside polyline.
        polyline = pygplates.PolylineOnSphere([(15,0), (0,0)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == polyline)
        
        # Partition intersecting polyline.
        polyline = pygplates.PolylineOnSphere([(0,0), (0,-30), (30,-30), (30,-90)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polyline, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        self.assertTrue(len(partitioned_outside_geometries) == 2)
        
        # Partition inside polygon.
        polygon = pygplates.PolygonOnSphere([(15,-30), (0,-30), (0,-15)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polygon, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 1)
        recon_geom, inside_geoms = partitioned_inside_geometries[0]
        self.assertTrue(recon_geom.get_feature().get_feature_id().get_string() == 'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(len(inside_geoms) == 1)
        self.assertTrue(inside_geoms[0] == polygon)
        
        # Partition outside polygon.
        polygon = pygplates.PolygonOnSphere([(15,0), (0,0), (0,15)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polygon, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertFalse(partitioned_inside_geometries)
        self.assertTrue(len(partitioned_outside_geometries) == 1)
        outside_geom = partitioned_outside_geometries[0]
        self.assertTrue(outside_geom == polygon)
        
        # Partition intersecting polygon.
        polygon = pygplates.PolygonOnSphere([(0,0), (0,-30), (30,-30), (30,-90)])
        partitioned_inside_geometries = []
        partitioned_outside_geometries = []
        plate_partitioner.partition_geometry(polygon, partitioned_inside_geometries, partitioned_outside_geometries)
        self.assertTrue(len(partitioned_inside_geometries) == 3)
        # Note that *polylines* are returned when intersecting (not polygons) - will be fixed in future.
        # Also we end up with 3 polylines outside (instead of 2).
        self.assertTrue(len(partitioned_outside_geometries) == 3)

    def test_partition_point(self):
        rotation_model = pygplates.RotationModel(self.rotation_features)
        resolved_topologies = []
        pygplates.resolve_topologies(
            self.topological_features,
            rotation_model,
            resolved_topologies,
            0)
        
        plate_partitioner = pygplates.PlatePartitioner(resolved_topologies, rotation_model)
        
        # Partition points.
        self.assertFalse(plate_partitioner.partition_point(pygplates.PointOnSphere(0, 0)))
        self.assertTrue(
                plate_partitioner.partition_point(pygplates.PointOnSphere(0, -30)).get_feature().get_feature_id().get_string() ==
                'GPlates-5511af6a-71bb-44b6-9cd2-fea9be3b7e8f')
        self.assertTrue(
                plate_partitioner.partition_point(pygplates.PointOnSphere(30, -30)).get_feature().get_feature_id().get_string() ==
                'GPlates-a6054d82-6e6d-4f59-9d24-4ab255ece477')
        self.assertTrue(
                plate_partitioner.partition_point(pygplates.PointOnSphere(0, -60)).get_feature().get_feature_id().get_string() ==
                'GPlates-4fe56a89-d041-4494-ab07-3abead642b8e')
    
    def test_pathlike(self):

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path

            # Test first PlatePartitioner.__init__ overload.
            rotation_model = pygplates.RotationModel(self.rotation_features)
            resolved_topologies = []
            pygplates.resolve_topologies(self.topological_features, rotation_model, resolved_topologies, 0)
            plate_partitioner = pygplates.PlatePartitioner(resolved_topologies, FIXTURES / Path('rotations.rot'))
            self.assertTrue(plate_partitioner.partition_geometry(pygplates.PointOnSphere(0, -30)))

            # Test second PlatePartitioner.__init__ overload.
            plate_partitioner = pygplates.PlatePartitioner(FIXTURES / Path('topologies.gpml'), FIXTURES / Path('rotations.rot'))
            self.assertTrue(plate_partitioner.partition_geometry(pygplates.PointOnSphere(0, -30)))

            # Temporary file containing features to partition.
            tmp_features_filename = FIXTURES / Path('tmp_features.gpml')
            point_feature = pygplates.Feature()
            point_feature.set_geometry(pygplates.PointOnSphere(0, -30))
            pygplates.FeatureCollection(point_feature).write(tmp_features_filename)

            # Test PlatePartitioner.partition_features().
            partitioned_features = plate_partitioner.partition_features(tmp_features_filename)
            self.assertTrue(len(partitioned_features) == 1)

            # Test partition_into_plates().
            partitioned_features = pygplates.partition_into_plates(
                    FIXTURES / Path('topologies.gpml'),
                    FIXTURES / Path('rotations.rot'),
                    tmp_features_filename)
            self.assertTrue(len(partitioned_features) == 1)

            tmp_features_filename.unlink()


class ResolvedTopologiesTestCase(unittest.TestCase):
    def test_resolve_topologies(self):
        pygplates.resolve_topologies(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            os.path.join(FIXTURES, 'resolved_topologies.gmt'),
            pygplates.GeoTimeInstant(10),
            os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))
        
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topologies.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topologies.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topological_sections.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            resolved_topologies_output_path = FIXTURES / Path('resolved_topologies.gmt')
            resolved_topological_sections_output_path = FIXTURES / Path('resolved_topological_sections.gmt')
            pygplates.resolve_topologies(
                FIXTURES / Path('topologies.gpml'),
                FIXTURES / Path('rotations.rot'),
                resolved_topologies_output_path,
                pygplates.GeoTimeInstant(10),
                resolved_topological_sections_output_path)
            
            self.assertTrue(resolved_topologies_output_path.exists())
            resolved_topologies_output_path.unlink()
            self.assertTrue(resolved_topological_sections_output_path.exists())
            resolved_topological_sections_output_path.unlink()
        
        topological_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        rotation_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            # Make sure can pass in optional ResolveTopologyParameters...
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())

        # Make sure can specify ResolveTopologyParameters with the topological features.
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            (topological_features, pygplates.ResolveTopologyParameters()),
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            # Make sure can pass in optional ResolveTopologyParameters...
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        topological_features = list(topological_features)
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            [
                (topological_features[0], pygplates.ResolveTopologyParameters()),  # single feature with ResolveTopologyParameters
                topological_features[1],  # single feature without ResolveTopologyParameters
                (topological_features[2:4], pygplates.ResolveTopologyParameters()),  # multiple features with ResolveTopologyParameters
                topological_features[4:],  # multiple features without ResolveTopologyParameters
            ],
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections)
        
        self.assertTrue(len(resolved_topologies) == 7)
        for resolved_topology in resolved_topologies:
            self.assertTrue(resolved_topology.get_resolved_feature().get_geometry() == resolved_topology.get_resolved_geometry())
        resolved_topologies_dict = dict(zip(
                (rt.get_feature().get_name() for rt in resolved_topologies),
                (rt for rt in resolved_topologies)))
        for bss in resolved_topologies_dict['topology1'].get_boundary_sub_segments():
            self.assertTrue(bss.get_topological_section_feature().get_name() in ('section2', 'section3', 'section4', 'section7', 'section8'))
            self.assertFalse(bss.get_sub_segments()) # No topological lines in 'topology1'.
        for bss in resolved_topologies_dict['topology2'].get_boundary_sub_segments():
            self.assertTrue(bss.get_topological_section_feature().get_name() in ('section4', 'section5', 'section7', 'section14', 'section9', 'section10'))
            if bss.get_topological_section_feature().get_name() == 'section14':
                self.assertTrue(set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in bss.get_sub_segments()) == set(['section11', 'section12']))
                # All sub-sub-segments in this shared sub-segment happen to have 3 vertices.
                for sub_sub_segment in bss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
            else:
                self.assertFalse(bss.get_sub_segments()) # Not from a topological line.
        for bss in resolved_topologies_dict['topology3'].get_boundary_sub_segments():
            self.assertTrue(bss.get_topological_section_feature().get_name() in ('section1', 'section2', 'section6', 'section7', 'section8', 'section14', 'section9', 'section10'))
            self.assertTrue(bss.get_resolved_feature().get_geometry() == bss.get_resolved_geometry())
            if bss.get_topological_section_feature().get_name() == 'section14':
                # We know 'section14' is a ResolvedTopologicalLine...
                self.assertTrue(bss.get_topological_section().get_resolved_line() == bss.get_topological_section_geometry())
                self.assertTrue(set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in bss.get_sub_segments()) == set(['section11', 'section12', 'section13']))
                for sub_sub_segment in bss.get_sub_segments():
                    if sub_sub_segment.get_feature().get_name() == 'section13':
                        self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 2)
                    else:
                        self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
            else:
                # We know all sections except 'section14' are ReconstructedFeatureGeometry's (not ResolvedTopologicalLine's)...
                self.assertTrue(pygplates.PolylineOnSphere(bss.get_topological_section().get_reconstructed_geometry()) == bss.get_topological_section_geometry())
                self.assertFalse(bss.get_sub_segments()) # Not from a topological line.
        
        # Sections 9 and 10 are points that now are separate sub-segments (each point is a rubber-banded line).
        # Previously they were joined into a single sub-segment (a hack).
        self.assertTrue(len(resolved_topological_sections) == 17)
        resolved_topological_sections_dict = dict(zip(
                (rts.get_topological_section_feature().get_name() for rts in resolved_topological_sections),
                (rts for rts in resolved_topological_sections)))
        for rts in resolved_topological_sections:
            self.assertTrue(rts.get_topological_section_feature().get_name() in (
                'section1', 'section2', 'section3', 'section4', 'section5', 'section6', 'section7', 'section8', 'section9', 'section10', 'section14',
                'section15', 'section16', 'section17', 'section18', 'section19', 'section20'))
        
        section1_shared_sub_segments = resolved_topological_sections_dict['section1'].get_shared_sub_segments()
        self.assertTrue(len(section1_shared_sub_segments) == 1)
        for sss in section1_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']))
            self.assertTrue(sss.get_resolved_feature().get_geometry() == sss.get_resolved_geometry())
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section2_shared_sub_segments = resolved_topological_sections_dict['section2'].get_shared_sub_segments()
        self.assertTrue(len(section2_shared_sub_segments) == 2)
        for sss in section2_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']) or sharing_topologies == set(['topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section3_shared_sub_segments = resolved_topological_sections_dict['section3'].get_shared_sub_segments()
        self.assertTrue(len(section3_shared_sub_segments) == 1)
        for sss in section3_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section4_shared_sub_segments = resolved_topological_sections_dict['section4'].get_shared_sub_segments()
        self.assertTrue(len(section4_shared_sub_segments) == 2)
        for sss in section4_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1']) or sharing_topologies == set(['topology2']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section5_shared_sub_segments = resolved_topological_sections_dict['section5'].get_shared_sub_segments()
        self.assertTrue(len(section5_shared_sub_segments) == 3)
        for sss in section5_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology4']) or
                            sharing_topologies == set(['topology2', 'topology5']) or
                            sharing_topologies == set(['topology5']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            if sharing_topologies == set(['topology5']):
                self.assertFalse(sss.get_overriding_and_subducting_plates()) # Only one adjacent plate (subducting)
                overriding_plate, subducting_plate = sss.get_overriding_and_subducting_plates(enforce_single_plates=False)
                self.assertTrue(overriding_plate is None)
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology5')
                self.assertFalse(sss.get_overriding_plate(return_subduction_polarity=True)) # Only one adjacent plate (subducting)
                overriding_plate, subduction_polarity = sss.get_overriding_plate(return_subduction_polarity=True, enforce_single_plate=False)
                self.assertTrue(overriding_plate is None)
            else:
                self.assertTrue(sss.get_overriding_and_subducting_plates()) # Two adjacent plates.
                overriding_plate = sss.get_overriding_plate()
                self.assertTrue(overriding_plate == sss.get_overriding_plate(enforce_single_plate=False))
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology2')
                subducting_plate = sss.get_subducting_plate()
                self.assertTrue(subducting_plate == sss.get_subducting_plate(enforce_single_plate=False))
            subducting_plate = sss.get_subducting_plate()
            # Can always find just the subducting plate though.
            self.assertTrue(subducting_plate.get_feature().get_name() == 'topology4' or
                            subducting_plate.get_feature().get_name() == 'topology5')
        
        section6_shared_sub_segments = resolved_topological_sections_dict['section6'].get_shared_sub_segments()
        self.assertTrue(len(section6_shared_sub_segments) == 1)
        for sss in section6_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section7_shared_sub_segments = resolved_topological_sections_dict['section7'].get_shared_sub_segments()
        self.assertTrue(len(section7_shared_sub_segments) == 2)
        for sss in section7_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1', 'topology2']) or sharing_topologies == set(['topology2', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        section8_shared_sub_segments = resolved_topological_sections_dict['section8'].get_shared_sub_segments()
        self.assertTrue(len(section8_shared_sub_segments) == 1)
        for sss in section8_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology1', 'topology3']))
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            overriding_plate, subducting_plate = sss.get_overriding_and_subducting_plates()
            overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
            self.assertTrue(overriding_plate.get_feature().get_reconstruction_plate_id() == 2)
            self.assertTrue(subducting_plate.get_feature().get_reconstruction_plate_id() == 0)
            self.assertTrue(subduction_polarity == 'Left')
            subducting_plate = sss.get_subducting_plate()
            subducting_plate, subduction_polarity = sss.get_subducting_plate(True)
            self.assertTrue(subducting_plate.get_feature().get_reconstruction_plate_id() == 0)
            overriding_plate = sss.get_overriding_plate()
            overriding_plate, subduction_polarity = sss.get_overriding_plate(True)
            self.assertTrue(overriding_plate.get_feature().get_reconstruction_plate_id() == 2)
        
        # 'section9' is a single point.
        section9_shared_sub_segments = resolved_topological_sections_dict['section9'].get_shared_sub_segments()
        self.assertTrue(len(section9_shared_sub_segments) == 1)
        for sss in section9_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology3']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            self.assertTrue(len(sharing_topology_reversal_flags) == 2)
            # Dict of topology names to topology-on-left flags.
            sharing_topology_on_left_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_on_left_flags()))
            self.assertTrue(len(sharing_topology_on_left_flags) == 2)
            # One sub-segment should be reversed and the other not.
            self.assertTrue(sharing_topology_reversal_flags['topology2'] != sharing_topology_reversal_flags['topology3'])
            resolved_sub_segment_geom = sss.get_resolved_geometry()
            self.assertTrue(len(resolved_sub_segment_geom) == 3)  # A polyline with 3 points (one section point and two rubber band points).
            # 'topology2' is clockwise and 'section9' is on its left side so start rubber point should be more Southern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology2']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
                self.assertTrue(sharing_topology_on_left_flags['topology2'])  # topology on left of sub-segment
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
                self.assertTrue(not sharing_topology_on_left_flags['topology2'])  # topology on right of sub-segment
            # 'topology3' is clockwise and 'section9' is on its right side so start rubber point should be more Northern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology3']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
                self.assertTrue(sharing_topology_on_left_flags['topology3'])  # topology on left of sub-segment
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
                self.assertTrue(not sharing_topology_on_left_flags['topology3'])  # topology on right of sub-segment
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        # 'section10' is a single point.
        section10_shared_sub_segments = resolved_topological_sections_dict['section10'].get_shared_sub_segments()
        self.assertTrue(len(section10_shared_sub_segments) == 1)
        for sss in section10_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology2', 'topology3']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            self.assertTrue(len(sharing_topology_reversal_flags) == 2)
            # Dict of topology names to topology-on-left flags.
            sharing_topology_on_left_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_on_left_flags()))
            self.assertTrue(len(sharing_topology_on_left_flags) == 2)
            # One sub-segment should be reversed and the other not.
            self.assertTrue(sharing_topology_reversal_flags['topology2'] != sharing_topology_reversal_flags['topology3'])
            resolved_sub_segment_geom = sss.get_resolved_geometry()
            self.assertTrue(len(resolved_sub_segment_geom) == 3)  # A polyline with 3 points (one section point and two rubber band points).
            # 'topology2' is clockwise and 'section10' is on its left side so start rubber point should be more Southern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology2']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
                self.assertTrue(sharing_topology_on_left_flags['topology2'])  # topology on left of sub-segment
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
                self.assertTrue(not sharing_topology_on_left_flags['topology2'])  # topology on right of sub-segment
            # 'topology3' is clockwise and 'section10' is on its right side so start rubber point should be more Northern than end rubber point (unless reversed).
            if sharing_topology_reversal_flags['topology3']:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
                self.assertTrue(sharing_topology_on_left_flags['topology3'])  # topology on left of sub-segment
            else:
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Northern
                self.assertTrue(not sharing_topology_on_left_flags['topology3'])  # topology on right of sub-segment
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        # Sections 11, 12, 13 are not resolved topological sections since they're only used in a resolved topological line (not in boundaries/networks).
        self.assertTrue('section11' not in resolved_topological_sections_dict)
        self.assertTrue('section12' not in resolved_topological_sections_dict)
        self.assertTrue('section13' not in resolved_topological_sections_dict)
        
        section14_shared_sub_segments = resolved_topological_sections_dict['section14'].get_shared_sub_segments()
        self.assertTrue(len(section14_shared_sub_segments) == 4)
        for sss in section14_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology7']) or
                            sharing_topologies == set(['topology7', 'topology3']) or
                            sharing_topologies == set(['topology4', 'topology3']) or
                            sharing_topologies == set(['topology2', 'topology3']))
            sub_sub_segments = set(sub_sub_segment.get_feature().get_name() for sub_sub_segment in sss.get_sub_segments())
            if sharing_topologies == set(['topology7']):
                self.assertTrue(sub_sub_segments == set(['section13']))
                # The one shared sub-segment happens to have 3 vertices (two from resolved line and one rubber band).
                self.assertTrue(len(sss.get_sub_segments()) == 1)
                self.assertTrue(len(sss.get_sub_segments()[0].get_resolved_geometry()) == 3)
                self.assertTrue(len(sss.get_resolved_geometry()) == 3)
                self.assertFalse(sss.get_overriding_and_subducting_plates()) # Don't have two sharing plates (only one).
                overriding_plate, subducting_plate = sss.get_overriding_and_subducting_plates(enforce_single_plates=False)
                self.assertTrue(overriding_plate is None)
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology7')
            elif sharing_topologies == set(['topology7', 'topology3']):
                self.assertTrue(sub_sub_segments == set(['section13']))
                # The one shared sub-segment happens to have 2 vertices (from resolved line).
                self.assertTrue(len(sss.get_sub_segments()) == 1)
                self.assertTrue(len(sss.get_sub_segments()[0].get_resolved_geometry()) == 2)
                self.assertTrue(len(sss.get_resolved_geometry()) == 2)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology3')
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology7')
                self.assertTrue(subduction_polarity == 'Right')
            elif sharing_topologies == set(['topology4', 'topology3']):
                self.assertTrue(sub_sub_segments == set(['section12', 'section13']))
                # All sub-sub-segments in this shared sub-segment happen to have 2 vertices.
                for sub_sub_segment in sss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 2)
                # Note that sub-sub-segment rubber band points don't contribute to resolved topo line (otherwise there'd be 3 points)...
                self.assertTrue(len(sss.get_resolved_geometry()) == 2)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology3')
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology4')
                self.assertTrue(subduction_polarity == 'Right')
            elif sharing_topologies == set(['topology2', 'topology3']):
                self.assertTrue(sub_sub_segments == set(['section11', 'section12']))
                # All sub-sub-segments in this shared sub-segment happen to have 3 vertices.
                for sub_sub_segment in sss.get_sub_segments():
                    self.assertTrue(len(sub_sub_segment.get_resolved_geometry()) == 3)
                # Note that sub-sub-segment rubber band points don't contribute to resolved topo line (which means 3 instead of 4)
                # but there's a rubber band point on the resolved topo line itself which brings total to 4...
                self.assertTrue(len(sss.get_resolved_geometry()) == 4)
                overriding_plate, subducting_plate, subduction_polarity = sss.get_overriding_and_subducting_plates(True)
                self.assertTrue(overriding_plate.get_feature().get_name() == 'topology3')
                self.assertTrue(subducting_plate.get_feature().get_name() == 'topology2')
                self.assertTrue(subduction_polarity == 'Right')
        
        # 'section15' is a single point.
        section15_shared_sub_segments = resolved_topological_sections_dict['section15'].get_shared_sub_segments()
        
        # Make a function for testing 'section15' since we're re-use it later below.
        def _internal_test_section15_shared_sub_segments(test_case, section15_shared_sub_segments):
            test_case.assertTrue(len(section15_shared_sub_segments) == 4)
            for sss in section15_shared_sub_segments:
                sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
                test_case.assertTrue(sharing_topologies == set(['topology4', 'topology5']) or
                                sharing_topologies == set(['topology5', 'topology6']) or
                                sharing_topologies == set(['topology4', 'topology7']) or
                                sharing_topologies == set(['topology6', 'topology7']))
                # Dict of topology names to reversal flags.
                sharing_topology_reversal_flags = dict(zip(
                        (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                        sss.get_sharing_resolved_topology_geometry_reversal_flags()))
                # Dict of topology names to topology-on-left flags.
                sharing_topology_on_left_flags = dict(zip(
                        (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                        sss.get_sharing_resolved_topology_on_left_flags()))
                if sharing_topologies == set(['topology4', 'topology5']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology4' is clockwise and sub-segment is on its right side so should go North to South (unless reversed).
                    if sharing_topology_reversal_flags['topology4']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                        test_case.assertTrue(sharing_topology_on_left_flags['topology4'])  # topology on left of sub-segment
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                        test_case.assertTrue(not sharing_topology_on_left_flags['topology4'])  # topology on right of sub-segment
                    # 'topology5' is clockwise and sub-segment is on its left side so should go South to North (unless reversed).
                    if sharing_topology_reversal_flags['topology5']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                elif sharing_topologies == set(['topology5', 'topology6']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology5' is clockwise and sub-segment is on its lower side so should go East to West (unless reversed).
                    if sharing_topology_reversal_flags['topology5']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                        test_case.assertTrue(sharing_topology_on_left_flags['topology5'])  # topology on left of sub-segment
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                        test_case.assertTrue(not sharing_topology_on_left_flags['topology5'])  # topology on right of sub-segment
                    # 'topology6' is counter-clockwise and sub-segment is on its upper side so should go East to West (unless reversed).
                    if sharing_topology_reversal_flags['topology6']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                elif sharing_topologies == set(['topology4', 'topology7']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology4' is clockwise and sub-segment is on its lower side so should go East to West (unless reversed).
                    if sharing_topology_reversal_flags['topology4']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                        test_case.assertTrue(sharing_topology_on_left_flags['topology4'])  # topology on left of sub-segment
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                        test_case.assertTrue(not sharing_topology_on_left_flags['topology4'])  # topology on right of sub-segment
                    # 'topology7' is clockwise and sub-segment is on its upper side so should go West to East (unless reversed).
                    if sharing_topology_reversal_flags['topology7']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                        test_case.assertTrue(sharing_topology_on_left_flags['topology7'])  # topology on left of sub-segment
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                        test_case.assertTrue(not sharing_topology_on_left_flags['topology7'])  # topology on right of sub-segment
                elif sharing_topologies == set(['topology6', 'topology7']):
                    test_case.assertTrue(len(sharing_topology_reversal_flags) == 2)
                    resolved_sub_segment_geom = sss.get_resolved_geometry()
                    test_case.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                    # 'topology6' is counter-clockwise and sub-segment is on its left side so should go North to South (unless reversed).
                    if sharing_topology_reversal_flags['topology6']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                        test_case.assertTrue(not sharing_topology_on_left_flags['topology6'])  # topology on right of sub-segment
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                        test_case.assertTrue(sharing_topology_on_left_flags['topology6'])  # topology on left of sub-segment
                    # 'topology7' is clockwise and sub-segment is on its right side so should go North to South (unless reversed).
                    if sharing_topology_reversal_flags['topology7']:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                        test_case.assertTrue(sharing_topology_on_left_flags['topology7'])  # topology on left of sub-segment
                    else:
                        test_case.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                        test_case.assertTrue(not sharing_topology_on_left_flags['topology7'])  # topology on right of sub-segment
        
        _internal_test_section15_shared_sub_segments(self, section15_shared_sub_segments)
        
        # 'section16' is a single point.
        section16_shared_sub_segments = resolved_topological_sections_dict['section16'].get_shared_sub_segments()
        self.assertTrue(len(section16_shared_sub_segments) == 3)
        for sss in section16_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology6', 'topology7']) or
                            sharing_topologies == set(['topology6']) or
                            sharing_topologies == set(['topology7']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            # Dict of topology names to topology-on-left flags.
            sharing_topology_on_left_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_on_left_flags()))
            if sharing_topologies == set(['topology6', 'topology7']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 2)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Both sub-segments not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                self.assertTrue(not sharing_topology_reversal_flags['topology7'])
                # 'topology7' is clockwise and 'topology6' is counter-clockwise so sub-segment goes North to South.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                self.assertTrue(sharing_topology_on_left_flags['topology6'])  # topology on left of sub-segment
                self.assertTrue(not sharing_topology_on_left_flags['topology7'])  # topology on right of sub-segment
            elif sharing_topologies == set(['topology6']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                # 'topology6' is counter-clockwise so sub-segment goes West to East.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] < resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Western
                self.assertTrue(sharing_topology_on_left_flags['topology6'])  # topology on left of sub-segment
            elif sharing_topologies == set(['topology7']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology7'])
                # 'topology7' is clockwise so sub-segment goes East to West.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                self.assertTrue(not sharing_topology_on_left_flags['topology7'])  # topology on right of sub-segment
        
        # 'section17' is a single point.
        section17_shared_sub_segments = resolved_topological_sections_dict['section17'].get_shared_sub_segments()
        self.assertTrue(len(section17_shared_sub_segments) == 3)
        for sss in section17_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology5', 'topology6']) or
                            sharing_topologies == set(['topology5']) or
                            sharing_topologies == set(['topology6']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            # Dict of topology names to topology-on-left flags.
            sharing_topology_on_left_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_on_left_flags()))
            if sharing_topologies == set(['topology5', 'topology6']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 2)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Both sub-segments not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology5'])
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                # 'topology5' is clockwise and 'topology6' is counter-clockwise so sub-segment goes East to West.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[1] > resolved_sub_segment_geom[1].to_lat_lon()[1]) # More Eastern
                self.assertTrue(not sharing_topology_on_left_flags['topology5'])  # topology on right of sub-segment
                self.assertTrue(sharing_topology_on_left_flags['topology6'])  # topology on left of sub-segment
            elif sharing_topologies == set(['topology5']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology5'])
                # 'topology5' is clockwise so sub-segment goes North to South.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] > resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Northern
                self.assertTrue(not sharing_topology_on_left_flags['topology5'])  # topology on right of sub-segment
            elif sharing_topologies == set(['topology6']):
                self.assertTrue(len(sharing_topology_reversal_flags) == 1)
                resolved_sub_segment_geom = sss.get_resolved_geometry()
                self.assertTrue(len(resolved_sub_segment_geom) == 2)  # A polyline with 2 points.
                # Sub-segment is not reversed.
                self.assertTrue(not sharing_topology_reversal_flags['topology6'])
                # 'topology6' is counter-clockwise so sub-segment goes South to North.
                self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[1].to_lat_lon()[0]) # More Southern
                self.assertTrue(sharing_topology_on_left_flags['topology6'])  # topology on left of sub-segment
        
        # 'section18' is a single point.
        section18_shared_sub_segments = resolved_topological_sections_dict['section18'].get_shared_sub_segments()
        self.assertTrue(len(section18_shared_sub_segments) == 1)
        for sss in section18_shared_sub_segments:
            sharing_topologies = set(srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies())
            self.assertTrue(sharing_topologies == set(['topology6']))
            # Dict of topology names to reversal flags.
            sharing_topology_reversal_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_geometry_reversal_flags()))
            # Dict of topology names to topology-on-left flags.
            sharing_topology_on_left_flags = dict(zip(
                    (srt.get_feature().get_name() for srt in sss.get_sharing_resolved_topologies()),
                    sss.get_sharing_resolved_topology_on_left_flags()))
            self.assertTrue(len(sharing_topology_reversal_flags) == 1)
            # One sub-segment and should not be reversed because it's shared by only a single topology
            # (and point sections can only get reversed if another topology shares it).
            self.assertTrue(not sharing_topology_reversal_flags['topology6'])
            self.assertTrue(sharing_topology_on_left_flags['topology6'])  # topology on left of sub-segment
            resolved_sub_segment_geom = sss.get_resolved_geometry()
            self.assertTrue(len(resolved_sub_segment_geom) == 3)  # A polyline with 3 points (one section point and two rubber band points).
            # 'topology6' is counter-clockwise and 'section18' is on its bottom-right corner so start rubber point should be more Southern than end rubber point (unless reversed).
            self.assertTrue(resolved_sub_segment_geom[0].to_lat_lon()[0] < resolved_sub_segment_geom[2].to_lat_lon()[0]) # More Southern
            self.assertFalse(sss.get_sub_segments()) # Not from a topological line.
            self.assertFalse(sss.get_overriding_and_subducting_plates()) # Not a subduction zone.
            self.assertFalse(sss.get_overriding_plate()) # Not a subduction zone.
            self.assertFalse(sss.get_subducting_plate()) # Not a subduction zone.
        
        # Test 'section15' still gives correct result when changing order of adding topologies
        # (from order 4->5->6->7 to order 4->6->5->7) which changes the shared sub-segment reversals.
        # We still get 4 shared sub-segments rubber banding to the single point of 'section15' and
        # each shared sub-segment connects 2 topologies as usual. It's just some reversals change
        # and we're testing a different path through the pyGPlates implementation.
        def _test_reordered_topological_features(
                test_case,
                topological_features,
                rotation_features,
                topologies_4_5_6_7_order_keys):
            
            reordered_topological_features = sorted(topological_features,
                key = lambda f:
                # Assign keys to get order 4->6->5->7...
                topologies_4_5_6_7_order_keys[0] if f.get_name() == 'topology4' else
                topologies_4_5_6_7_order_keys[1] if f.get_name() == 'topology5' else
                topologies_4_5_6_7_order_keys[2] if f.get_name() == 'topology6' else
                topologies_4_5_6_7_order_keys[3] if f.get_name() == 'topology7' else
                # All other features get same key and so retain same order...
                0)
            resolved_topologies = []
            resolved_topological_sections = []
            pygplates.resolve_topologies(
                reordered_topological_features,
                rotation_features,
                resolved_topologies,
                10,
                resolved_topological_sections)
            resolved_topological_section15 = next(filter(lambda rts: rts.get_feature().get_name() == 'section15', resolved_topological_sections))
            section15_shared_sub_segments = resolved_topological_section15.get_shared_sub_segments()
            # Re-use function defined above for testing the shared sub-segments of 'section15'.
            _internal_test_section15_shared_sub_segments(test_case, section15_shared_sub_segments)
        
        # Test with different ordering of topologies 4, 5, 6 and 7.
        #_test_reordered_topological_features(self, topological_features, rotation_features, [1, 2, 3, 4])  # The normal order already tested above.
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 2, 4, 3])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 3, 2, 4])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 3, 4, 2])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 4, 2, 3])
        _test_reordered_topological_features(self, topological_features, rotation_features, [1, 4, 3, 2])

        # This time exclude networks from the topological sections (but not the topologies).
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            resolve_topological_section_types = pygplates.ResolveTopologyType.boundary)
        self.assertTrue(len(resolved_topologies) == 7)
        self.assertTrue(len(resolved_topological_sections) == 15)

        # This time exclude networks from the topologies (but not the topological sections).
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            resolve_topology_types = pygplates.ResolveTopologyType.boundary,
            resolve_topological_section_types = pygplates.ResolveTopologyType.boundary | pygplates.ResolveTopologyType.network)
        self.assertTrue(len(resolved_topologies) == 6)
        self.assertTrue(len(resolved_topological_sections) == 17)

        # This time exclude networks from both the topologies and the topological sections.
        resolved_topologies = []
        resolved_topological_sections = []
        pygplates.resolve_topologies(
            topological_features,
            rotation_features,
            resolved_topologies,
            10,
            resolved_topological_sections,
            resolve_topology_types = pygplates.ResolveTopologyType.boundary)
        self.assertTrue(len(resolved_topologies) == 6)
        self.assertTrue(len(resolved_topological_sections) == 15)


class ReconstructionTreeCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        self.reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], 10.0)

    def test_create(self):
        self.assertTrue(isinstance(self.reconstruction_tree, pygplates.ReconstructionTree))
        # Create again with a GeoTimeInstant instead of float.
        self.reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], pygplates.GeoTimeInstant(10.0))
        self.assertRaises(pygplates.InterpolationError,
                pygplates.ReconstructionTree, [self.rotations], pygplates.GeoTimeInstant.create_distant_past())
        self.assertEqual(self.reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(self.reconstruction_tree.get_reconstruction_time() > 9.9999 and
                self.reconstruction_tree.get_reconstruction_time() < 10.00001)
        
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.ReconstructionTree,
                [ 'non_existent_file.rot' ],
                10.0)
        # Create using feature collections.
        reconstruction_tree = pygplates.ReconstructionTree([self.rotations], 10.0)
        # Create using a single feature collection.
        reconstruction_tree = pygplates.ReconstructionTree(self.rotations, 10.0)
        # Create using a list of features.
        reconstruction_tree = pygplates.ReconstructionTree([rotation for rotation in self.rotations], 10.0)
        # Create using a single feature.
        reconstruction_tree = pygplates.ReconstructionTree(next(iter(self.rotations)), 10.0)
        # Create using a feature collection, list of features and a single feature (dividing the rotation features between them).
        reconstruction_tree = pygplates.ReconstructionTree(
                [ pygplates.FeatureCollection(self.rotations.get(lambda feature: True, pygplates.FeatureReturn.all)[0:2]), # First and second features
                    list(self.rotations)[2:-1], # All but first, second and last features
                    list(self.rotations)[-1]], # Last feature
                10.0)

    def test_build(self):
        def build_tree(builder, rotation_features, reconstruction_time):
            for rotation_feature in rotation_features:
                trp = rotation_feature.get_total_reconstruction_pole()
                if trp:
                    fixed_plate_id, moving_plate_id, total_reconstruction_pole = trp
                    interpolated_rotation = total_reconstruction_pole.get_value(reconstruction_time)
                    if interpolated_rotation:
                        builder.insert_total_reconstruction_pole(
                                fixed_plate_id,
                                moving_plate_id,
                                interpolated_rotation.get_finite_rotation())
        
        # Build using a ReconstructionTreeBuilder.
        builder = pygplates.ReconstructionTreeBuilder()
        reconstruction_time = self.reconstruction_tree.get_reconstruction_time()
        build_tree(builder, self.rotations, reconstruction_time)
        built_reconstruction_tree = builder.build_reconstruction_tree(
                self.reconstruction_tree.get_anchor_plate_id(),
                reconstruction_time)
        self.assertTrue(isinstance(built_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertEqual(built_reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(built_reconstruction_tree.get_reconstruction_time() > 9.9999 and
                built_reconstruction_tree.get_reconstruction_time() < 10.00001)
        self.assertTrue(len(built_reconstruction_tree.get_edges()) == 447)
        # Building again without inserting poles will give an empty reconstruction tree.
        built_reconstruction_tree = builder.build_reconstruction_tree(
                self.reconstruction_tree.get_anchor_plate_id(),
                reconstruction_time)
        self.assertTrue(len(built_reconstruction_tree.get_edges()) == 0)
        # Build again (because ReconstructionTreeBuilder.build_reconstruction_tree clears state).
        build_tree(builder, self.rotations, pygplates.GeoTimeInstant(reconstruction_time))
        # Build using GeoTimeInstant instead of float.
        built_reconstruction_tree = builder.build_reconstruction_tree(
                self.reconstruction_tree.get_anchor_plate_id(),
                pygplates.GeoTimeInstant(reconstruction_time))
        self.assertTrue(isinstance(built_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertEqual(built_reconstruction_tree.get_anchor_plate_id(), 0)
        self.assertTrue(built_reconstruction_tree.get_reconstruction_time() > 9.9999 and
                built_reconstruction_tree.get_reconstruction_time() < 10.00001)
        self.assertTrue(len(built_reconstruction_tree.get_edges()) == 447)

    def test_get_edge(self):
        # Should not be able to get an edge for the anchor plate id.
        self.assertFalse(self.reconstruction_tree.get_edge(self.reconstruction_tree.get_anchor_plate_id()))
        edge = self.reconstruction_tree.get_edge(101)
        self.assertTrue(edge.get_moving_plate_id() == 101)
        self.assertTrue(edge.get_fixed_plate_id() == 714)
        self.assertTrue(isinstance(edge.get_equivalent_total_rotation(), pygplates.FiniteRotation))
        self.assertTrue(isinstance(edge.get_relative_total_rotation(), pygplates.FiniteRotation))
        
        child_edges = edge.get_child_edges()
        self.assertTrue(len(child_edges) == 33)
        chld_edge_count = 0
        for child_edge in child_edges:
            self.assertTrue(child_edge.get_fixed_plate_id() == edge.get_moving_plate_id())
            chld_edge_count += 1
        self.assertEqual(chld_edge_count, len(child_edges))
    
    def test_anchor_plate_edges(self):
        anchor_plate_edges = self.reconstruction_tree.get_anchor_plate_edges()
        for i in range(0, len(anchor_plate_edges)):
            self.assertTrue(anchor_plate_edges[i].get_fixed_plate_id() == self.reconstruction_tree.get_anchor_plate_id())
        self.assertTrue(len(anchor_plate_edges) == 2)
        
        edge_count = 0
        for edge in anchor_plate_edges:
            self.assertTrue(edge.get_fixed_plate_id() == self.reconstruction_tree.get_anchor_plate_id())
            edge_count += 1
        self.assertEqual(edge_count, len(anchor_plate_edges))
    
    def test_edges(self):
        edges = self.reconstruction_tree.get_edges()
        for i in range(0, len(edges)):
            self.assertTrue(isinstance(edges[i], pygplates.ReconstructionTreeEdge))
        self.assertTrue(len(edges) == 447)
        
        edge_count = 0
        for edge in edges:
            edge_count += 1
        self.assertEqual(edge_count, len(edges))

    def test_get_parent_traversal(self):
        edge = self.reconstruction_tree.get_edge(907)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 301)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 101)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 714)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 701)
        edge = edge.get_parent_edge()
        self.assertTrue(edge.get_moving_plate_id() == 1)
        self.assertTrue(edge.get_fixed_plate_id() == 0)
        edge = edge.get_parent_edge()
        # Reached anchor plate.
        self.assertFalse(edge)

    def test_total_rotation(self):
        self.assertTrue(isinstance(
                self.reconstruction_tree.get_equivalent_total_rotation(802),
                pygplates.FiniteRotation))
        self.assertTrue(isinstance(
                # Pick plates that are in different sub-trees.
                self.reconstruction_tree.get_relative_total_rotation(802, 291),
                pygplates.FiniteRotation))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                self.reconstruction_tree.get_equivalent_total_rotation(802),
                self.reconstruction_tree.get_relative_total_rotation(802, 0)))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                self.reconstruction_tree.get_edge(802).get_relative_total_rotation(),
                self.reconstruction_tree.get_relative_total_rotation(
                        802,
                        self.reconstruction_tree.get_edge(802).get_fixed_plate_id())))
        # Should return identity rotation.
        self.assertTrue(self.reconstruction_tree.get_equivalent_total_rotation(
                self.reconstruction_tree.get_anchor_plate_id()).represents_identity_rotation())
        self.assertTrue(self.reconstruction_tree.get_relative_total_rotation(
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id()).represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(self.reconstruction_tree.get_equivalent_total_rotation(10000, use_identity_for_missing_plate_ids=False))
        # Should return None for an unknown relative plate id.
        self.assertFalse(self.reconstruction_tree.get_relative_total_rotation(802, 10000, use_identity_for_missing_plate_ids=False))
    
    def test_stage_rotation(self):
        from_reconstruction_tree = pygplates.ReconstructionTree(
                [ self.rotations ],
                # Further in the past...
                self.reconstruction_tree.get_reconstruction_time() + 1)
        
        equivalent_stage_rotation = pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802)
        self.assertTrue(isinstance(equivalent_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 10000, use_identity_for_missing_plate_ids=False))
        # Should raise error for differing anchor plate ids (0 versus 291).
        self.assertRaises(
                pygplates.DifferentAnchoredPlatesInReconstructionTreesError,
                pygplates.ReconstructionTree.get_equivalent_stage_rotation,
                pygplates.ReconstructionTree([self.rotations], 11, 291),
                self.reconstruction_tree,
                802)
        
        relative_stage_rotation = pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802, 291)
        self.assertTrue(isinstance(relative_stage_rotation, pygplates.FiniteRotation))
        # Should return identity rotation.
        self.assertTrue(pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree,
                self.reconstruction_tree,
                self.reconstruction_tree.get_anchor_plate_id(),
                self.reconstruction_tree.get_anchor_plate_id())
                        .represents_identity_rotation())
        # Should return None for an unknown plate id.
        self.assertFalse(pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 802, 10000, use_identity_for_missing_plate_ids=False))
        # Should return None for an unknown relative plate id.
        self.assertFalse(pygplates.ReconstructionTree.get_relative_stage_rotation(
                from_reconstruction_tree, self.reconstruction_tree, 10000, 291, use_identity_for_missing_plate_ids=False))


class RotationModelTestCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollectionFileFormatRegistry().read(
                os.path.join(FIXTURES, 'rotations.rot'))
        self.rotation_model = pygplates.RotationModel([ os.path.join(FIXTURES, 'rotations.rot') ])
        self.from_time = 20.0
        self.to_time = pygplates.GeoTimeInstant(10.0)
        self.from_reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], self.from_time)
        self.to_reconstruction_tree = pygplates.ReconstructionTree([ self.rotations ], self.to_time)

    def test_create(self):
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.RotationModel,
                [ 'non_existent_file.rot' ])
        #
        # UPDATE: Argument 'clone_rotation_features' is deprecated in revision 25.
        #         And argument 'extend_total_reconstruction_poles_to_distant_past' was added in revision 25.
        #
        # Create using feature collections instead of filenames.
        rotation_model = pygplates.RotationModel([self.rotations], clone_rotation_features=False)
        # Create using a single feature collection.
        rotation_model = pygplates.RotationModel(self.rotations, clone_rotation_features=False)
        # Create using a list of features.
        rotation_model = pygplates.RotationModel([rotation for rotation in self.rotations], clone_rotation_features=False)
        # Create using a single feature.
        rotation_model = pygplates.RotationModel(next(iter(self.rotations)), clone_rotation_features=False)
        # Create using a mixture of the above.
        rotation_model = pygplates.RotationModel(
                [os.path.join(FIXTURES, 'rotations.rot'),
                    self.rotations,
                    [rotation for rotation in self.rotations],
                    next(iter(self.rotations))],
                clone_rotation_features=False)
        # Create a reference to the same (C++) rotation model.
        rotation_model_reference = pygplates.RotationModel(rotation_model)
        self.assertTrue(rotation_model_reference == rotation_model)
        
        # Adapt an existing rotation model to use a different cache size and default anchor plate ID.
        rotation_model_adapted = pygplates.RotationModel(self.rotation_model, default_anchor_plate_id=802)
        self.assertTrue(rotation_model_adapted.get_default_anchor_plate_id() == 802)
        self.assertTrue(rotation_model_adapted != self.rotation_model)  # Should be a different C++ instance.
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_adapted.get_rotation(self.to_time, 802),
                self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=802)))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_adapted.get_rotation(self.to_time, 802, anchor_plate_id=0),
                self.rotation_model.get_rotation(self.to_time, 802)))
        # Make sure newly adapted model using a new cache size but delegating default anchor plate ID actually delegates.
        another_rotation_model_adapted = pygplates.RotationModel(rotation_model_adapted, 32)
        self.assertTrue(another_rotation_model_adapted.get_default_anchor_plate_id() == 802)
        
        # Test using a non-zero default anchor plate ID.
        rotation_model_non_zero_default_anchor = pygplates.RotationModel(self.rotations, default_anchor_plate_id=802)
        self.assertTrue(rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802).represents_identity_rotation())
        self.assertFalse(rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802, anchor_plate_id=0).represents_identity_rotation())
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802),
                self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=802)))
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802, anchor_plate_id=0),
                self.rotation_model.get_rotation(self.to_time, 802)))
        
        # Test extending total reconstruction poles to distant past.
        rotation_model_not_extended = pygplates.RotationModel(self.rotations)
        # At 1000Ma there are no rotations (for un-extended model).
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        # Deprecated version (triggered by explicitly specifying 'clone_rotation_features' argument) is also an un-extended model.
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, clone_rotation_features=False)
        # At 1000Ma there are no rotations (for un-extended model).
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, clone_rotation_features=True)
        # At 1000Ma there are no rotations (for un-extended model).
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, extend_total_reconstruction_poles_to_distant_past=False)
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        # This should choose the 'extend_total_reconstruction_poles_to_distant_past' __init__ overload instead of the
        # deprecated (not documented) overload accepting 'clone_rotation_features'.
        rotation_model_not_extended = pygplates.RotationModel(self.rotations, 100, False)
        self.assertFalse(rotation_model_not_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        
        rotation_model_extended = pygplates.RotationModel(self.rotations, extend_total_reconstruction_poles_to_distant_past=True)
        # But at 1000Ma there are rotations (for extended model).
        self.assertTrue(rotation_model_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))
        # This should still choose the 'extend_total_reconstruction_poles_to_distant_past' __init__ overload instead of the
        # deprecated (not documented) overload accepting 'clone_rotation_features'.
        rotation_model_extended = pygplates.RotationModel(self.rotations, 100, True)
        self.assertTrue(rotation_model_extended.get_rotation(1000.0, 801, anchor_plate_id=802, use_identity_for_missing_plate_ids=False))

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            rotation_model_from_path = pygplates.RotationModel(FIXTURES / Path('rotations.rot'))
            self.assertTrue(rotation_model_from_path.get_rotation(0.0, 801, use_identity_for_missing_plate_ids=False))
    
    def test_get_reconstruction_tree(self):
        to_reconstruction_tree = self.rotation_model.get_reconstruction_tree(self.to_time)
        self.assertTrue(isinstance(to_reconstruction_tree, pygplates.ReconstructionTree))
        self.assertTrue(to_reconstruction_tree.get_reconstruction_time() > self.to_time.get_value() - 1e-6 and
                to_reconstruction_tree.get_reconstruction_time() < self.to_time.get_value() + 1e-6)
        self.assertRaises(pygplates.InterpolationError,
                pygplates.RotationModel.get_reconstruction_tree, self.rotation_model, pygplates.GeoTimeInstant.create_distant_past())
    
    def test_get_rotation(self):
        equivalent_total_rotation = self.rotation_model.get_rotation(self.to_time, 802)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                equivalent_total_rotation,
                self.to_reconstruction_tree.get_equivalent_total_rotation(802)))
        self.assertRaises(pygplates.InterpolationError,
                pygplates.RotationModel.get_rotation, self.rotation_model, pygplates.GeoTimeInstant.create_distant_past(), 802)
        
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        # Fixed plate id defaults to anchored plate id.
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, anchor_plate_id=291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        # Shouldn't really matter what the anchor plate id is (as long as there's a plate circuit
        # path from anchor plate to both fixed and moving plates.
        relative_total_rotation = self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291, anchor_plate_id=802)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_total_rotation,
                self.to_reconstruction_tree.get_relative_total_rotation(802, 291)))
        
        equivalent_stage_rotation = self.rotation_model.get_rotation(self.to_time, 802, self.from_time)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                equivalent_stage_rotation,
                pygplates.ReconstructionTree.get_equivalent_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802)))
        
        relative_stage_rotation = self.rotation_model.get_rotation(self.to_time, 802, self.from_time, 291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_stage_rotation,
                pygplates.ReconstructionTree.get_relative_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802,
                        291)))
        # Fixed plate id defaults to anchored plate id.
        relative_stage_rotation = self.rotation_model.get_rotation(
                self.to_time, 802, pygplates.GeoTimeInstant(self.from_time), anchor_plate_id=291)
        self.assertTrue(pygplates.FiniteRotation.are_equivalent(
                relative_stage_rotation,
                pygplates.ReconstructionTree.get_relative_stage_rotation(
                        self.from_reconstruction_tree,
                        self.to_reconstruction_tree,
                        802,
                        291)))
        
        # Ensure that specifying 'from_time' at present day (ie, 0Ma) does not assume zero finite rotation (at present day).
        non_zero_present_day_rotation_model = pygplates.RotationModel(
            pygplates.Feature.create_total_reconstruction_sequence(
                0,
                801,
                pygplates.GpmlIrregularSampling([
                    pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                            pygplates.FiniteRotation((0, 0), 1.57)),
                        0.0), # non-zero finite rotation at present day
                    pygplates.GpmlTimeSample(
                        pygplates.GpmlFiniteRotation(
                            pygplates.FiniteRotation.create_identity_rotation()),
                        10.0)
                    ])))
        # Non-zero finite rotation.
        self.assertFalse(non_zero_present_day_rotation_model.get_rotation(0.0, 801).represents_identity_rotation())
        # Just looks at 10Ma.
        self.assertTrue(non_zero_present_day_rotation_model.get_rotation(10.0, 801).represents_identity_rotation())
        # 10Ma relative to non-zero finite rotation at present day.
        #
        #   R(0->time, A->Plate) = R(time, A->Plate) * inverse[R(0, A->Plate)]
        self.assertTrue(
            non_zero_present_day_rotation_model.get_rotation(10.0, 801, 0.0) ==
            non_zero_present_day_rotation_model.get_rotation(10.0, 801) * non_zero_present_day_rotation_model.get_rotation(0.0, 801).get_inverse())
    
    def test_pickle(self):
        pickled_rotation_model = pickle.loads(pickle.dumps(self.rotation_model))
        self.assertTrue(pickled_rotation_model.get_rotation(self.to_time, 802) ==
                        self.rotation_model.get_rotation(self.to_time, 802))
        self.assertTrue(pickled_rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291, anchor_plate_id=802) ==
                        self.rotation_model.get_rotation(self.to_time, 802, fixed_plate_id=291, anchor_plate_id=802))
        # Test a rotation model that delegates to an existing rotation model.
        rotation_model_delegator = pygplates.RotationModel(self.rotation_model, default_anchor_plate_id=701)
        pickled_rotation_model_delegator = pickle.loads(pickle.dumps(rotation_model_delegator))
        self.assertTrue(pickled_rotation_model_delegator.get_rotation(self.to_time, 802) ==
                        rotation_model_delegator.get_rotation(self.to_time, 802))
        # Test a rotation model with non-zero default anchor plate ID (to ensure default anchor plate ID is transcribed).
        rotation_model_non_zero_default_anchor = pygplates.RotationModel(self.rotations, default_anchor_plate_id=701)
        pickled_rotation_model_non_zero_default_anchor = pickle.loads(pickle.dumps(rotation_model_non_zero_default_anchor))
        self.assertTrue(pickled_rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802) ==
                        rotation_model_non_zero_default_anchor.get_rotation(self.to_time, 802))
        # Test adding an attribute to a rotation model from the Python (note: this should work for all other pyGPlates objects too).
        # This attribute will be unknown to the C++ side (except that it's in __dict__).
        self.rotation_model.reconstruction_identifier = "RotationModel"
        pickled_rotation_model = pickle.loads(pickle.dumps(self.rotation_model))
        self.assertTrue(pickled_rotation_model.reconstruction_identifier == "RotationModel")



class StrainTestCase(unittest.TestCase):

    def test_create(self):
        self.assertTrue(pygplates.StrainRate().get_velocity_spatial_gradient() == (0, 0, 0, 0))
        self.assertTrue(pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16).get_velocity_spatial_gradient() == (1e-15, 1e-16, 2e-15, 5e-16))

        self.assertTrue(pygplates.Strain().get_deformation_gradient() == (1, 0, 0, 1))
        self.assertTrue(pygplates.Strain(1+1e-4, 1e-5, 2e-4, 1+5e-5).get_deformation_gradient() == (1+1e-4, 1e-5, 2e-4, 1+5e-5))
    
    def test_compare(self):
        self.assertTrue(pygplates.StrainRate() == pygplates.StrainRate.zero)
        self.assertTrue(pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16) == pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16))
        self.assertTrue(pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16) != pygplates.StrainRate(1.01e-15, 1e-16, 2e-15, 5e-16))
        self.assertTrue(pygplates.Strain() == pygplates.Strain.identity)
        self.assertTrue(pygplates.Strain(1+1e-4, 1e-5, 2e-4, 1+5e-5) == pygplates.Strain(1+1e-4, 1e-5, 2e-4, 1+5e-5))
        self.assertTrue(pygplates.Strain(1+1e-4, 1e-5, 2e-4, 1+5e-5) != pygplates.Strain(1+1.01e-4, 1e-5, 2e-4, 1+5e-5))
        # Strains are not typically as small as strain *rates*, so really small strains that are slightly different will compare equal.
        self.assertTrue(pygplates.Strain(1+1e-15, 1e-16, 2e-15, 5e-16) == pygplates.Strain(1+1.01e-15, 1e-16, 2e-15, 5e-16))
    
    def test_constants(self):
        self.assertTrue(pygplates.StrainRate.zero == pygplates.StrainRate())
        self.assertTrue(pygplates.Strain.identity == pygplates.Strain())
    
    def test_get_dilatation_rate(self):
        self.assertTrue(pygplates.StrainRate().get_dilatation_rate() == 0)
        self.assertAlmostEqual(pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16).get_dilatation_rate(), 1.5e-15, places=16)
    
    def test_get_total_strain_rate(self):
        self.assertTrue(pygplates.StrainRate().get_total_strain_rate() == 0)
        self.assertAlmostEqual(pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16).get_total_strain_rate(), 1.8587630295441107e-15, places=16)
    
    def test_get_strain_rate_style(self):
        # Strain rate style should be NaN (zero divided by zero).
        self.assertTrue(math.isnan(pygplates.StrainRate().get_strain_rate_style()))
        self.assertAlmostEqual(pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16).get_strain_rate_style(), 0.8199626321480792)
    
    def test_get_rate_of_deformation(self):
        self.assertTrue(pygplates.StrainRate().get_rate_of_deformation() == (0, 0, 0, 0))

        rate_of_deformation = pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16).get_rate_of_deformation()
        self.assertAlmostEqual(rate_of_deformation[0], 1e-15, places=16)
        self.assertAlmostEqual(rate_of_deformation[1], 1.05e-15, places=16)
        self.assertAlmostEqual(rate_of_deformation[2], 1.05e-15, places=16)
        self.assertAlmostEqual(rate_of_deformation[3], 5e-16, places=16)
    
    def test_get_velocity_spatial_gradient(self):
        self.assertTrue(pygplates.StrainRate().get_velocity_spatial_gradient() == (0, 0, 0, 0))

        velocity_spatial_gradient = pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16).get_velocity_spatial_gradient()
        self.assertAlmostEqual(velocity_spatial_gradient[0], 1e-15, places=16)
        self.assertAlmostEqual(velocity_spatial_gradient[1], 1e-16, places=16)
        self.assertAlmostEqual(velocity_spatial_gradient[2], 2e-15, places=16)
        self.assertAlmostEqual(velocity_spatial_gradient[3], 5e-16, places=16)
    
    def test_get_dilatation(self):
        self.assertTrue(pygplates.Strain().get_dilatation() == 0)
        self.assertTrue(pygplates.Strain(1+1e-4, 1e-5, 2e-4, 1+5e-5).get_dilatation(), 0.000150003)
    
    def test_get_principal_strain(self):
        self.assertTrue(pygplates.Strain().get_principal_strain() == (0, 0, 0))

        strain = pygplates.Strain(1+1e-15, 1e-16, 2e-15, 1+5e-16)
        principal_strain = strain.get_principal_strain()
        self.assertAlmostEqual(principal_strain[0], 1.9984014443252818e-15, places=16)
        self.assertAlmostEqual(principal_strain[1], -2.220446049250313e-16, places=16)
        self.assertAlmostEqual(principal_strain[2], 0.6318146893398876)
        principal_strain = strain.get_principal_strain(principal_angle_type=pygplates.PrincipalAngleType.major_east)
        self.assertAlmostEqual(principal_strain[0], 1.9984014443252818e-15, places=16)
        self.assertAlmostEqual(principal_strain[1], -2.220446049250313e-16, places=16)
        self.assertAlmostEqual(principal_strain[2], 0.6318146893398876 - math.pi/2)
        principal_strain = strain.get_principal_strain(principal_angle_type=pygplates.PrincipalAngleType.major_azimuth)
        self.assertAlmostEqual(principal_strain[0], 1.9984014443252818e-15, places=16)
        self.assertAlmostEqual(principal_strain[1], -2.220446049250313e-16, places=16)
        self.assertAlmostEqual(principal_strain[2], math.pi - 0.6318146893398876)
    
    def test_get_deformation_gradient(self):
        self.assertTrue(pygplates.Strain().get_deformation_gradient() == (1, 0, 0, 1))

        deformation_gradient = pygplates.Strain(1+1e-15, 1e-16, 2e-15, 1+5e-16).get_deformation_gradient()
        self.assertAlmostEqual(deformation_gradient[0], 1+1e-15, places=16)
        self.assertAlmostEqual(deformation_gradient[1], 1e-16, places=16)
        self.assertAlmostEqual(deformation_gradient[2], 2e-15, places=16)
        self.assertAlmostEqual(deformation_gradient[3], 1+5e-16, places=16)
    
    def test_accumulate_strain(self):
        strain = pygplates.Strain.accumulate(pygplates.Strain.identity, pygplates.StrainRate.zero, pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16), 100)
        deformation_gradient = strain.get_deformation_gradient()
        self.assertAlmostEqual(deformation_gradient[0], 1.00000000000005, places=16)
        self.assertAlmostEqual(deformation_gradient[1], 5.0000000000003755e-15, places=16)
        self.assertAlmostEqual(deformation_gradient[2], 1.0000000000000751e-13, places=16)
        self.assertAlmostEqual(deformation_gradient[3], 1.000000000000025, places=16)
    
    def test_pickle(self):
        strain_rate = pygplates.StrainRate(1e-15, 1e-16, 2e-15, 5e-16)
        pickled_strain_rate = pickle.loads(pickle.dumps(strain_rate))
        self.assertTrue(pickled_strain_rate == strain_rate)

        strain = pygplates.Strain(1+1e-4, 1e-5, 2e-4, 1+5e-5)
        pickled_strain = pickle.loads(pickle.dumps(strain))
        self.assertTrue(pickled_strain == strain)


class TopologicalModelTestCase(unittest.TestCase):
    def setUp(self):
        self.rotations = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        self.rotation_model = pygplates.RotationModel(self.rotations)

        self.topologies = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml'))
        self.topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model)

    def test_create(self):
        self.assertRaises(
                pygplates.OpenFileForReadingError,
                pygplates.TopologicalModel,
                'non_existant_topology_file.gpml', self.rotations)

        self.assertTrue(self.topological_model.get_anchor_plate_id() == 0)

        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, anchor_plate_id=1)
        self.assertTrue(topological_model.get_anchor_plate_id() == 1)

        # Make sure can pass in optional ResolveTopologyParameters.
        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        self.assertTrue(topological_model.get_anchor_plate_id() == 0)
        # Make sure can specify ResolveTopologyParameters with the topological features.
        topological_model = pygplates.TopologicalModel(
                (self.topologies, pygplates.ResolveTopologyParameters()),
                self.rotation_model,
                default_resolve_topology_parameters=pygplates.ResolveTopologyParameters())
        topologies_list = list(self.topologies)
        topological_model = pygplates.TopologicalModel(
                [
                    (topologies_list[0], pygplates.ResolveTopologyParameters()),  # single feature with ResolveTopologyParameters
                    topologies_list[1],  # single feature without ResolveTopologyParameters
                    (topologies_list[2:4], pygplates.ResolveTopologyParameters()),  # multiple features with ResolveTopologyParameters
                    topologies_list[4:],  # multiple features without ResolveTopologyParameters
                ],
                self.rotation_model)
        # Make sure can specify a topological snapshot cache size.
        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, topological_snapshot_cache_size=2)

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            topological_model = pygplates.TopologicalModel(
                FIXTURES / Path('topologies.gpml'),
                FIXTURES / Path('rotations.rot'))

    def test_get_topological_snapshot(self):
        topological_snapshot = self.topological_model.topological_snapshot(10.5)  # note: it should allow a non-integral time
        self.assertTrue(topological_snapshot.get_anchor_plate_id() == self.topological_model.get_anchor_plate_id())
        self.assertTrue(topological_snapshot.get_rotation_model() == self.topological_model.get_rotation_model())

    def test_get_rotation_model(self):
        topological_model = pygplates.TopologicalModel(self.topologies, self.rotation_model, anchor_plate_id=2)
        self.assertTrue(topological_model.get_rotation_model().get_rotation(1.0, 802) == self.rotation_model.get_rotation(1.0, 802, anchor_plate_id=2))
        self.assertTrue(topological_model.get_rotation_model().get_default_anchor_plate_id() == 2)

        rotation_model_anchor_2 = pygplates.RotationModel(self.rotations, default_anchor_plate_id=2)
        topological_model = pygplates.TopologicalModel(self.topologies, rotation_model_anchor_2)
        self.assertTrue(topological_model.get_anchor_plate_id() == 2)
        self.assertTrue(topological_model.get_rotation_model().get_default_anchor_plate_id() == 2)

    def test_reconstruct_geometry(self):
        # Create from a multipoint.
        multipoint =  pygplates.MultiPointOnSphere([(0,0), (10,10)])
        reconstructed_multipoint_time_span = self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]},
                deformation_uses_natural_neighbour_interpolation=False)
        # Create from a point.
        reconstructed_point_time_span = self.topological_model.reconstruct_geometry(
                pygplates.PointOnSphere(0, 0),
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0]})
        # Create from a sequence of points.
        reconstructed_points_time_span = self.topological_model.reconstruct_geometry(
                [(0, 0), (5, 5)],
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})
        # Create using non-integral initial, oldest, youngest times, and a non-integral time increment.
        reconstructed_points_time_span = self.topological_model.reconstruct_geometry(
                [(0, 0), (5, 5)],
                initial_time=20.5,
                oldest_time=30.5,
                youngest_time=10.5,
                time_increment=0.5,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})

        # Number of scalars must match number of points.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                pygplates.PointOnSphere(0, 0),
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})
        # 'oldest_time - youngest_time' not an integer multiple of time_increment
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=5,
                time_increment=2)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4.01)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=1.99)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=1,
                time_increment=0.99)
        # oldest_time later (or same as) youngest_time
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=5)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                youngest_time=4)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,  # initial_time and oldest_time
                youngest_time=101.0)
        # Oldest/youngest times cannot be distant-past or distant-future.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=pygplates.GeoTimeInstant.create_distant_past())
        # Time increment must be positive.
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                time_increment=-1)
        self.assertRaises(
                ValueError,
                self.topological_model.reconstruct_geometry,
                multipoint,
                100.0,
                oldest_time=4,
                time_increment=0)

    def test_get_reconstructed_data(self):
        multipoint =  pygplates.MultiPointOnSphere([(0,0), (0,-30), (0,-60)])
        
        # Try with default deactivate points.
        self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0, 1.0]},
                deactivate_points=pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints())
        
        # Try with our own Python derived class that delegates to the default deactivate points.
        class DelegateDeactivatePoints(pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints):
            def __init__(self):
                super(DelegateDeactivatePoints, self).__init__()
                # Delegate to the default internal algorithm but changes some of its parameters.
                self.default_deactivate_points = pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints(
                        threshold_velocity_delta=0.9,
                        threshold_distance_to_boundary= 15,
                        deactivate_points_that_fall_outside_a_network=True)
                self.last_deactivate_args = None
            def deactivate(self, prev_point, prev_location, prev_time, current_point, current_location, current_time):
                self.last_deactivate_args = prev_point, prev_location, prev_time, current_point, current_location, current_time
                return self.default_deactivate_points.deactivate(prev_point, prev_location, prev_time, current_point, current_location, current_time)
        delegate_deactivate_points = DelegateDeactivatePoints()
        self.assertFalse(delegate_deactivate_points.last_deactivate_args)
        self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0, 1.0]},
                deactivate_points=delegate_deactivate_points)
        self.assertTrue(delegate_deactivate_points.last_deactivate_args)
        # Test the arguments last passed to DelegateDeactivatePoints.deactivate() have the type we expect.
        prev_point, prev_location, prev_time, current_point, current_location, current_time = delegate_deactivate_points.last_deactivate_args
        self.assertTrue(isinstance(prev_point, pygplates.PointOnSphere))
        self.assertTrue(isinstance(prev_location, pygplates.TopologyPointLocation))
        self.assertTrue(isinstance(prev_time, float))
        self.assertTrue(isinstance(current_point, pygplates.PointOnSphere))
        self.assertTrue(isinstance(current_location, pygplates.TopologyPointLocation))
        self.assertTrue(isinstance(current_time, float))
        
        reconstructed_multipoint_time_span = self.topological_model.reconstruct_geometry(
                multipoint,
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0, 1.0]})
        
        # Time range.
        oldest_time, youngest_time, time_increment, num_time_slots = reconstructed_multipoint_time_span.get_time_span()
        self.assertTrue(oldest_time == 30 and youngest_time == 10 and time_increment == 1 and num_time_slots == 21)
        
        # Points.
        reconstructed_points = reconstructed_multipoint_time_span.get_geometry_points(20)
        self.assertTrue(len(reconstructed_points) == 3)
        # Reconstructed points same as initial points (since topologies are currently static and initial points not assigned a plate ID).
        self.assertTrue(reconstructed_points == list(multipoint))
        reconstructed_points = reconstructed_multipoint_time_span.get_geometry_points(20, return_inactive_points=True)
        self.assertTrue(len(reconstructed_points) == 3)
        
        # Topology point locations.
        topology_point_locations = reconstructed_multipoint_time_span.get_topology_point_locations(20)
        self.assertTrue(len(topology_point_locations) == 3)
        self.assertTrue(topology_point_locations[0].not_located_in_resolved_topology())
        self.assertTrue(topology_point_locations[1].located_in_resolved_boundary())
        self.assertTrue(topology_point_locations[2].located_in_resolved_network())
        self.assertTrue(topology_point_locations[2].located_in_resolved_network_deforming_region())
        self.assertFalse(topology_point_locations[2].located_in_resolved_network_rigid_block())
        topology_point_locations = reconstructed_multipoint_time_span.get_topology_point_locations(20, return_inactive_points=True)
        self.assertTrue(len(topology_point_locations) == 3)
        
        # Strain rates.
        strain_rates = reconstructed_multipoint_time_span.get_strain_rates(20)
        self.assertTrue(len(strain_rates) == 3)
        self.assertTrue(strain_rates[0] == pygplates.StrainRate.zero)
        self.assertTrue(strain_rates[1] == pygplates.StrainRate.zero)
        self.assertTrue(strain_rates[2] == pygplates.StrainRate.zero)
        strain_rates = reconstructed_multipoint_time_span.get_strain_rates(20, return_inactive_points=True)
        self.assertTrue(len(strain_rates) == 3)
        
        # Strains.
        strains = reconstructed_multipoint_time_span.get_strains(20)
        self.assertTrue(len(strains) == 3)
        self.assertTrue(strains[0] == pygplates.Strain.identity)
        self.assertTrue(strains[1] == pygplates.Strain.identity)
        self.assertTrue(strains[2] == pygplates.Strain.identity)
        strains = reconstructed_multipoint_time_span.get_strains(20, return_inactive_points=True)
        self.assertTrue(len(strains) == 3)
        
        # Velocities.
        velocities = reconstructed_multipoint_time_span.get_velocities(20)
        self.assertTrue(len(velocities) == 3)
        self.assertTrue(velocities[0] == pygplates.Vector3D.zero)
        self.assertTrue(velocities[1] == pygplates.Vector3D.zero)
        self.assertTrue(velocities[2] == pygplates.Vector3D.zero)
        velocities = reconstructed_multipoint_time_span.get_velocities(20, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t, pygplates.VelocityUnits.cms_per_yr, return_inactive_points=True)
        self.assertTrue(len(velocities) == 3)
        
        # Scalars.
        scalars_dict = reconstructed_multipoint_time_span.get_scalar_values(20)
        # Although we only supplied initial values for 2 scalar types, there will be more since other *evolved* scalar types are reconstructed
        # (such as crustal thinning factor) that we did not provide initial values for.
        self.assertTrue(len(scalars_dict) == 4)
        self.assertTrue(scalars_dict[pygplates.ScalarType.gpml_crustal_thickness] == [10.0, 10.0, 10.0])
        self.assertTrue(scalars_dict[pygplates.ScalarType.gpml_crustal_stretching_factor] == [1.0, 1.0, 1.0])
        self.assertTrue(scalars_dict[pygplates.ScalarType.gpml_crustal_thinning_factor] == [0.0, 0.0, 0.0])
        self.assertTrue(scalars_dict[pygplates.ScalarType.gpml_tectonic_subsidence] == [0.0, 0.0, 0.0])
        self.assertTrue(reconstructed_multipoint_time_span.get_scalar_values(20, pygplates.ScalarType.gpml_crustal_thickness) == [10.0, 10.0, 10.0])
        self.assertTrue(reconstructed_multipoint_time_span.get_scalar_values(20, pygplates.ScalarType.gpml_crustal_stretching_factor) == [1.0, 1.0, 1.0])
        self.assertTrue(reconstructed_multipoint_time_span.get_scalar_values(20, pygplates.ScalarType.gpml_crustal_thinning_factor) == [0.0, 0.0, 0.0])
        self.assertTrue(reconstructed_multipoint_time_span.get_scalar_values(20, pygplates.ScalarType.gpml_tectonic_subsidence) == [0.0, 0.0, 0.0])
        scalars_dict = reconstructed_multipoint_time_span.get_scalar_values(20, return_inactive_points=True)
        self.assertTrue(len(scalars_dict) == 4)
         
        # Crustal thicknesses.
        crustal_thickness = reconstructed_multipoint_time_span.get_crustal_thicknesses(20)
        self.assertTrue(len(crustal_thickness) == 3)
        self.assertTrue(crustal_thickness[0] == 10.0)
        self.assertTrue(crustal_thickness[1] == 10.0)
        self.assertTrue(crustal_thickness[2] == 10.0)
        crustal_thickness = reconstructed_multipoint_time_span.get_crustal_thicknesses(20, return_inactive_points=True)
        self.assertTrue(len(crustal_thickness) == 3)
         
        # Crustal stretching factors.
        crustal_stretching_factors = reconstructed_multipoint_time_span.get_crustal_stretching_factors(20)
        self.assertTrue(len(crustal_stretching_factors) == 3)
        self.assertTrue(crustal_stretching_factors[0] == 1.0)
        self.assertTrue(crustal_stretching_factors[1] == 1.0)
        self.assertTrue(crustal_stretching_factors[2] == 1.0)
        crustal_stretching_factors = reconstructed_multipoint_time_span.get_crustal_stretching_factors(20, return_inactive_points=True)
        self.assertTrue(len(crustal_stretching_factors) == 3)
         
        # Crustal thinning factors.
        crustal_thinning_factors = reconstructed_multipoint_time_span.get_crustal_thinning_factors(20)
        self.assertTrue(len(crustal_thinning_factors) == 3)
        self.assertTrue(crustal_thinning_factors[0] == 0.0)
        self.assertTrue(crustal_thinning_factors[1] == 0.0)
        self.assertTrue(crustal_thinning_factors[2] == 0.0)
        crustal_thinning_factors = reconstructed_multipoint_time_span.get_crustal_thinning_factors(20, return_inactive_points=True)
        self.assertTrue(len(crustal_thinning_factors) == 3)
         
        # Tectonic subsidence.
        tectonic_subsidences = reconstructed_multipoint_time_span.get_tectonic_subsidences(20)
        self.assertTrue(len(tectonic_subsidences) == 3)
        self.assertTrue(tectonic_subsidences[0] == 0.0)
        self.assertTrue(tectonic_subsidences[1] == 0.0)
        self.assertTrue(tectonic_subsidences[2] == 0.0)
        tectonic_subsidences = reconstructed_multipoint_time_span.get_tectonic_subsidences(20, return_inactive_points=True)
        self.assertTrue(len(tectonic_subsidences) == 3)
   
    def test_pickle(self):
        # Pickle a TopologicalModel.
        pickled_topological_model = pickle.loads(pickle.dumps(self.topological_model))
        self.assertTrue(pickled_topological_model.get_rotation_model().get_rotation(100, 802) ==
                        self.topological_model.get_rotation_model().get_rotation(100, 802))
        # Check snapshots of the original and pickled topological models.
        resolved_topologies = self.topological_model.topological_snapshot(10.0).get_resolved_topologies(same_order_as_topological_features=True)
        pickled_resolved_topologies = pickled_topological_model.topological_snapshot(10.0).get_resolved_topologies(same_order_as_topological_features=True)
        self.assertTrue(len(pickled_resolved_topologies) == len(resolved_topologies))
        for index in range(len(pickled_resolved_topologies)):
            self.assertTrue(pickled_resolved_topologies[index].get_resolved_geometry() == resolved_topologies[index].get_resolved_geometry())
        # Check reconstructed geometry time spans of the original and pickled topological models.
        reconstructed_time_span = self.topological_model.reconstruct_geometry(
                pygplates.MultiPointOnSphere([(0,0), (10,10)]),
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})
        pickled_reconstructed_time_span = pickled_topological_model.reconstruct_geometry(
                pygplates.MultiPointOnSphere([(0,0), (10,10)]),
                initial_time=20.0,
                oldest_time=30.0,
                youngest_time=10.0,
                reconstruction_plate_id=802,
                initial_scalars={pygplates.ScalarType.gpml_crustal_thickness : [10.0, 10.0], pygplates.ScalarType.gpml_crustal_stretching_factor : [1.0, 1.0]})
        self.assertTrue(pickled_reconstructed_time_span.get_geometry_points(10.0) == reconstructed_time_span.get_geometry_points(10.0))
        self.assertTrue(pickled_reconstructed_time_span.get_strains(10.0) == reconstructed_time_span.get_strains(10.0))
        self.assertTrue(pickled_reconstructed_time_span.get_strain_rates(10.0) == reconstructed_time_span.get_strain_rates(10.0))
        self.assertTrue(pickled_reconstructed_time_span.get_scalar_values(10.0) == reconstructed_time_span.get_scalar_values(10.0))
        # Check the topology point locations explicitly (since resolved topologies are not equality comparable).
        pickled_topology_point_locations = pickled_reconstructed_time_span.get_topology_point_locations(10.0)
        topology_point_locations = reconstructed_time_span.get_topology_point_locations(10.0)
        self.assertTrue(len(pickled_topology_point_locations) == len(topology_point_locations))
        for index in range(len(pickled_topology_point_locations)):
            pickled_located_in_resolved_boundary = pickled_topology_point_locations[index].located_in_resolved_boundary()
            located_in_resolved_boundary = topology_point_locations[index].located_in_resolved_boundary()
            self.assertTrue((pickled_located_in_resolved_boundary is None and located_in_resolved_boundary is None) or
                            pickled_located_in_resolved_boundary.get_resolved_geometry() == located_in_resolved_boundary.get_resolved_geometry())
            pickled_located_in_resolved_network = pickled_topology_point_locations[index].located_in_resolved_network()
            located_in_resolved_network = topology_point_locations[index].located_in_resolved_network()
            self.assertTrue((pickled_located_in_resolved_network is None and located_in_resolved_network is None) or
                            pickled_located_in_resolved_network.get_resolved_geometry() == located_in_resolved_network.get_resolved_geometry())


class TopologicalSnapshotTestCase(unittest.TestCase):
    def test(self):
        #
        # Class pygplates.TopologicalSnapshot is used internally by pygplates.resolve_topologies()
        # so most of its testing is already done by testing pygplates.resolve_topologies().
        #
        # Here we're just making sure we can access the pygplates.TopologicalSnapshot methods.
        #
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10),
            anchor_plate_id=1)
        
        self.assertTrue(snapshot.get_anchor_plate_id() == 1)
        self.assertTrue(snapshot.get_rotation_model())
        
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        
        self.assertTrue(snapshot.get_anchor_plate_id() == 0)
        self.assertTrue(snapshot.get_rotation_model())
        
        resolved_topologies = snapshot.get_resolved_topologies()
        self.assertTrue(len(resolved_topologies) == 7)  # See ResolvedTopologiesTestCase
        
        snapshot.export_resolved_topologies(os.path.join(FIXTURES, 'resolved_topologies.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topologies.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topologies.gmt'))
        
        resolved_topological_sections = snapshot.get_resolved_topological_sections()
        self.assertTrue(len(resolved_topological_sections) == 17)  # See ResolvedTopologiesTestCase
        
        snapshot.export_resolved_topological_sections(os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))
        self.assertTrue(os.path.isfile(os.path.join(FIXTURES, 'resolved_topological_sections.gmt')))
        os.remove(os.path.join(FIXTURES, 'resolved_topological_sections.gmt'))

        # Make sure can pass in optional ResolveTopologyParameters.
        rotations = pygplates.FeatureCollection(os.path.join(FIXTURES, 'rotations.rot'))
        topologies = list(pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml')))
        snapshot = pygplates.TopologicalSnapshot(
            topologies,
            rotations,
            pygplates.GeoTimeInstant(10),
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters(
                    enable_strain_rate_clamping=True,
                    strain_rate_smoothing=pygplates.StrainRateSmoothing.barycentric))
        # Make sure can specify ResolveTopologyParameters with the topological features.
        snapshot = pygplates.TopologicalSnapshot(
            (topologies, pygplates.ResolveTopologyParameters()),
            rotations,
            pygplates.GeoTimeInstant(10),
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters(
                    strain_rate_smoothing=pygplates.StrainRateSmoothing.none))
        snapshot = pygplates.TopologicalSnapshot(
            [
                (topologies[0], pygplates.ResolveTopologyParameters()),  # single feature with ResolveTopologyParameters
                topologies[1],  # single feature without ResolveTopologyParameters
                (topologies[2:4], pygplates.ResolveTopologyParameters()),  # multiple features with ResolveTopologyParameters
                topologies[4:],  # multiple features without ResolveTopologyParameters
            ],
            rotations,
            pygplates.GeoTimeInstant(10))

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            snapshot = pygplates.TopologicalSnapshot(
                FIXTURES / Path('topologies.gpml'),
                FIXTURES / Path('rotations.rot'),
                pygplates.GeoTimeInstant(10))
            
            self.assertTrue(snapshot.get_anchor_plate_id() == 0)
            self.assertTrue(snapshot.get_rotation_model())

    def test_resolved_topological_lines(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        resolved_topological_lines = snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.line)
        self.assertTrue(len(resolved_topological_lines) == 1)

        # Test geometry points, velocities and source features.
        resolved_topological_line = resolved_topological_lines[0]
        resolved_geometry_points = resolved_topological_line.get_resolved_geometry_points()
        resolved_geometry_point_velocities = resolved_topological_line.get_resolved_geometry_point_velocities()
        resolved_geometry_point_features = resolved_topological_line.get_resolved_geometry_point_features()
        self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_velocities))
        self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_features))
        self.assertTrue(resolved_topological_line.get_resolved_geometry() == pygplates.PolylineOnSphere(resolved_geometry_points))
        self.assertTrue(resolved_geometry_point_velocities == [pygplates.Vector3D.zero] * len(resolved_geometry_point_velocities))
        for point_feature in resolved_geometry_point_features:
            self.assertTrue(point_feature.get_name().startswith('section'))

    def test_resolved_topological_boundaries(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        resolved_topological_boundaries = snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.boundary)
        self.assertTrue(len(resolved_topological_boundaries) >= 1)

        # Test geometry points, velocities and source features.
        for resolved_topological_boundary in resolved_topological_boundaries:
            resolved_geometry_points = resolved_topological_boundary.get_resolved_geometry_points()
            resolved_geometry_point_velocities = resolved_topological_boundary.get_resolved_geometry_point_velocities()
            resolved_geometry_point_features = resolved_topological_boundary.get_resolved_geometry_point_features()
            self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_velocities))
            self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_features))
            self.assertTrue(resolved_topological_boundary.get_resolved_geometry() == pygplates.PolygonOnSphere(resolved_geometry_points))
            self.assertTrue(resolved_geometry_point_velocities == [pygplates.Vector3D.zero] * len(resolved_geometry_point_velocities))
            for point_feature in resolved_geometry_point_features:
                point_feature_name = point_feature.get_name()
                # Source feature should be a topological section but not a topological line (ie, not 'section14' which is the only topological line).
                self.assertTrue(point_feature_name.startswith('section') and point_feature_name != 'section14')

        # Test point location/velocity/strain-rate and reconstructed point.
        for resolved_topological_boundary in resolved_topological_boundaries:
            point_in_topology2 = pygplates.PointOnSphere(0, -30)  # only 'topology2' contains this point
            point_location = resolved_topological_boundary.get_point_location(point_in_topology2)
            point_velocity = resolved_topological_boundary.get_point_velocity(point_in_topology2)
            point_strain_rate = resolved_topological_boundary.get_point_strain_rate(point_in_topology2)
            reconstructed_point = resolved_topological_boundary.reconstruct_point(
                    point_in_topology2,
                    resolved_topological_boundary.get_reconstruction_time() + 1.0)
            if resolved_topological_boundary.get_feature().get_name() == 'topology2':
                self.assertTrue(point_location.located_in_resolved_boundary() == resolved_topological_boundary)
                self.assertTrue(point_velocity == pygplates.Vector3D.zero)
                self.assertTrue(point_strain_rate == pygplates.StrainRate.zero)
                self.assertTrue(reconstructed_point == point_in_topology2)  # plates don't actually move
            else:
                self.assertTrue(point_location.located_in_resolved_boundary() is None)
                self.assertTrue(point_velocity is None)
                self.assertTrue(point_strain_rate is None)
                self.assertTrue(reconstructed_point is None)
            point_in_network = pygplates.PointOnSphere(0, -60)
            self.assertTrue(resolved_topological_boundary.get_point_location(point_in_network).located_in_resolved_network() is None)  # no networks resolved

    def test_resolved_topological_networks(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        resolved_topological_networks = snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.network)
        self.assertTrue(len(resolved_topological_networks) == 1)
        resolved_topological_network = resolved_topological_networks[0]

        # Test geometry points, velocities and source features.
        for include_rigid_blocks_as_interior_holes in (True, False):
            resolved_geometry_points = resolved_topological_network.get_resolved_geometry_points(include_rigid_blocks_as_interior_holes)
            resolved_geometry_point_velocities = resolved_topological_network.get_resolved_geometry_point_velocities(include_rigid_blocks_as_interior_holes)
            resolved_geometry_point_features = resolved_topological_network.get_resolved_geometry_point_features(include_rigid_blocks_as_interior_holes)
            self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_velocities))
            self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_features))
            self.assertTrue(resolved_geometry_point_velocities == [pygplates.Vector3D.zero] * len(resolved_geometry_point_velocities))
            # Note: The network has NO interior holes. If it did then this would fail.
            self.assertTrue(resolved_topological_network.get_resolved_geometry() == pygplates.PolygonOnSphere(resolved_geometry_points))
            for point_feature in resolved_geometry_point_features:
                point_feature_name = point_feature.get_name()
                # Source feature should be a topological section but not a topological line (ie, not 'section14' which is the only topological line).
                self.assertTrue(point_feature_name.startswith('section') and point_feature_name != 'section14')

        # Test interior rigid blocks.
        boundary_with_holes = resolved_topological_network.get_resolved_boundary(True)
        self.assertTrue(boundary_with_holes == resolved_topological_network.get_resolved_geometry(include_rigid_blocks_as_interior_holes=True))
        interior_rigid_blocks = resolved_topological_network.get_rigid_blocks()
        self.assertTrue(len(interior_rigid_blocks) == 0)

        # Test network triangulation.
        network_triangulation = resolved_topological_network.get_network_triangulation()
        triangles = network_triangulation.get_triangles()
        # There are 22 triangles in the Delaunay triangulation but only 13 in the deforming region.
        num_triangles = 22
        num_deforming_triangles = 13
        num_deforming_triangulation_boundary_edges = 15  # triangle edges bounding the *deforming* triangulation
        self.assertTrue(len(triangles) == num_triangles)
        deforming_triangles = [tri for tri in triangles if tri.is_in_deforming_region]
        self.assertTrue(len(deforming_triangles) == num_deforming_triangles)
        vertices = network_triangulation.get_vertices()
        self.assertTrue(len(vertices) == 15)
        # Can use vertices as keys in a dict.
        vertex_to_triangles_dict = {}  # mapping of each vertex to all triangles referencing it
        for triangle_index, triangle in enumerate(triangles):
            self.assertTrue(triangle == triangles[triangle_index])
            for index in range(3):
                triangle_vertex = triangle.get_vertex(index)
                self.assertTrue(triangle_vertex in vertices)
                vertex_to_triangles_dict.setdefault(triangle_vertex, []).append(triangle)
            self.assertTrue(triangle.strain_rate == pygplates.StrainRate.zero)
        self.assertTrue(len(vertex_to_triangles_dict) == len(vertices))
        self.assertTrue(sum(len(vertex_to_triangles_dict[v]) for v in vertices) == 3 * num_triangles)
        # Can use triangles as keys in a dict.
        deforming_triangle_to_adjacent_deforming_triangles_dict = {}  # mapping of each *deforming* triangle to its adjacent *deforming* triangles
        for deforming_triangle in deforming_triangles:
            deforming_triangle_to_adjacent_deforming_triangles_dict[deforming_triangle] = []
            for index in range(3):
                adjacent_triangle = deforming_triangle.get_adjacent_triangle(index)
                if (adjacent_triangle and                       # if adjacent triangle is not at a triangulation boundary
                    adjacent_triangle.is_in_deforming_region):  # if adjacent triangle is deforming
                    deforming_triangle_to_adjacent_deforming_triangles_dict[deforming_triangle].append(adjacent_triangle)
        self.assertTrue(len(deforming_triangle_to_adjacent_deforming_triangles_dict) == num_deforming_triangles)
        self.assertTrue(sum(len(deforming_triangle_to_adjacent_deforming_triangles_dict[t]) for t in deforming_triangles) ==
                        3 * num_deforming_triangles - num_deforming_triangulation_boundary_edges)  # no adjacent triangles at the boundary
        for vertex_index, vertex in enumerate(vertices):
            self.assertTrue(vertex == vertices[vertex_index])
            vertex.position  # just access
            self.assertTrue(vertex.strain_rate == pygplates.StrainRate.zero)
            self.assertTrue(vertex.get_velocity() == pygplates.Vector3D.zero)
            # Test velocity with parameters.
            self.assertTrue(vertex.get_velocity(
                velocity_delta_time=1.0, velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t,
                velocity_units=pygplates.VelocityUnits.kms_per_my, earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms
            ) == pygplates.Vector3D.zero)
        # Each incident vertex has itself a list of incident vertices which should contain the original vertex.
        for vertex in vertices:
            incident_vertices = vertex.get_incident_vertices()
            self.assertTrue(incident_vertices)
            for incident_vertex in incident_vertices:
                self.assertTrue(vertex in incident_vertex.get_incident_vertices())
        # Each incident vertex has a list of incident triangles of which each triangle should contain the original vertex.
        for vertex in vertices:
            incident_triangles = vertex.get_incident_triangles()
            self.assertTrue(incident_triangles)
            for incident_triangle in incident_triangles:
                triangle_vertices_matching_original_vertex = 0
                # Exactly one vertex of the incident triangle should match the original vertex.
                for index in range(3):
                    if incident_triangle.get_vertex(index) == vertex:
                        triangle_vertices_matching_original_vertex += 1
                self.assertTrue(triangle_vertices_matching_original_vertex == 1)

        # Test point location/velocity/strain-rate and reconstructed point.
        point_inside_network = pygplates.PointOnSphere(0, -60)  # point is inside network
        self.assertTrue(resolved_topological_network.get_point_location(point_inside_network).located_in_resolved_network() == resolved_topological_network)

        # Check that point is in correct triangle of network triangulation.
        self.assertTrue(resolved_topological_network.get_point_location(point_inside_network).located_in_resolved_network_deforming_region() == resolved_topological_network)
        _, network_triangle = resolved_topological_network.get_point_location(point_inside_network).located_in_resolved_network_deforming_region(return_network_triangle=True)
        # Point should be in the triangle with these vertices - so check they match the network triangle.
        network_triangle_vertex_lat_lons = [
            (1.9190404608408473, -46.33105360687644),
            (0.1281258847639748, -89.46803133550394),
            (-25.92674267168927, -67.49384878759929)]
        network_triangle_vertices = [pygplates.PointOnSphere(lat, lon) for lat, lon in network_triangle_vertex_lat_lons]
        for index in range(3):
            self.assertTrue(network_triangle.get_vertex(index).position in network_triangle_vertices)
        self.assertTrue(network_triangle.is_in_deforming_region)
        
        self.assertTrue(resolved_topological_network.get_point_velocity(point_inside_network) == pygplates.Vector3D.zero)
        self.assertTrue(resolved_topological_network.get_point_strain_rate(point_inside_network) == pygplates.StrainRate.zero)
        self.assertTrue(resolved_topological_network.reconstruct_point(
                point_inside_network,
                resolved_topological_network.get_reconstruction_time() + 1.0,
                use_natural_neighbour_interpolation=False)
                        == point_inside_network)  # network doesn't rotate/deform
        point_in_boundary = pygplates.PointOnSphere(0, -30)  # point is outside network
        self.assertTrue(resolved_topological_network.get_point_location(point_in_boundary).located_in_resolved_network() is None)
        self.assertTrue(resolved_topological_network.get_point_location(point_in_boundary).located_in_resolved_boundary() is None)  # no boundaries resolved
        self.assertTrue(resolved_topological_network.get_point_velocity(point_in_boundary) is None)
        self.assertTrue(resolved_topological_network.get_point_strain_rate(point_in_boundary) is None)
        self.assertTrue(resolved_topological_network.reconstruct_point(
                point_in_boundary,
                resolved_topological_network.get_reconstruction_time() + 1.0)
                        is None)

    def test_resolved_topological_sub_segments(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        resolved_topological_boundaries = snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.boundary)
        self.assertTrue(len(resolved_topological_boundaries) >= 1)

        # Test geometry points, velocities and source features.
        for resolved_topological_boundary in resolved_topological_boundaries:
            for boundary_sub_segment in resolved_topological_boundary.get_boundary_sub_segments():
                resolved_geometry_points = boundary_sub_segment.get_resolved_geometry_points()
                resolved_geometry_point_velocities = boundary_sub_segment.get_resolved_geometry_point_velocities()
                resolved_geometry_point_features = boundary_sub_segment.get_resolved_geometry_point_features()
                self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_velocities))
                self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_features))
                self.assertTrue(boundary_sub_segment.get_resolved_geometry() == pygplates.PolylineOnSphere(resolved_geometry_points))
                self.assertTrue(resolved_geometry_point_velocities == [pygplates.Vector3D.zero] * len(resolved_geometry_point_velocities))
                for point_feature in resolved_geometry_point_features:
                    self.assertTrue(point_feature.get_name().startswith('section'))

    def test_resolved_topological_shared_sub_segments(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        resolved_topological_sections = snapshot.get_resolved_topological_sections()
        self.assertTrue(len(resolved_topological_sections) >= 1)

        # Test geometry points, velocities and source features.
        for resolved_topological_section in resolved_topological_sections:
            for shared_sub_segment in resolved_topological_section.get_shared_sub_segments():
                resolved_geometry_points = shared_sub_segment.get_resolved_geometry_points()
                resolved_geometry_point_velocities = shared_sub_segment.get_resolved_geometry_point_velocities()
                resolved_geometry_point_features = shared_sub_segment.get_resolved_geometry_point_features()
                self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_velocities))
                self.assertTrue(len(resolved_geometry_points) == len(resolved_geometry_point_features))
                self.assertTrue(shared_sub_segment.get_resolved_geometry() == pygplates.PolylineOnSphere(resolved_geometry_points))
                self.assertTrue(resolved_geometry_point_velocities == [pygplates.Vector3D.zero] * len(resolved_geometry_point_velocities))
                for point_feature in resolved_geometry_point_features:
                    self.assertTrue(point_feature.get_name().startswith('section'))
    
    def test_point_locations_velocities_strain_rates(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        points = [
                pygplates.PointOnSphere(0, -30),  # only 'topology2' contains this point
                (0, -60),  # only the sole network 'topology3' contains this point
        ]

        # Search only resolved boundaries (not resolved networks).
        point_locations = snapshot.get_point_locations(points,
                                                       resolve_topology_types=pygplates.ResolveTopologyType.boundary)
        point_velocities, point_locations_2 = snapshot.get_point_velocities(points,
                                                                            resolve_topology_types=pygplates.ResolveTopologyType.boundary,
                                                                            # Also test velocity arguments get accepted...
                                                                            velocity_delta_time=1.0,
                                                                            velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t,
                                                                            velocity_units=pygplates.VelocityUnits.kms_per_my,
                                                                            earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms,
                                                                            return_point_locations=True)
        self.assertTrue(len(point_velocities) == len(point_locations_2) == 2)
        point_strain_rates, point_locations_3 = snapshot.get_point_strain_rates(points,
                                                                                resolve_topology_types=pygplates.ResolveTopologyType.boundary,
                                                                                return_point_locations=True)
        self.assertTrue(len(point_strain_rates) == len(point_locations_3) == 2)
        # First point is in resolved boundary 'topology2'.
        self.assertTrue(point_locations == point_locations_2 == point_locations_3)
        resolved_topology2 = point_locations[0].located_in_resolved_boundary()
        self.assertTrue(resolved_topology2 and resolved_topology2.get_feature().get_name() == 'topology2')
        self.assertTrue(point_velocities[0] == pygplates.Vector3D.zero)
        self.assertTrue(point_strain_rates[0] == pygplates.StrainRate.zero)
        # Second point is in resolved network 'topology3', but we only searched resolved boundaries.
        self.assertTrue(point_locations[1].not_located_in_resolved_topology())
        self.assertTrue(point_velocities[1] is None)
        self.assertTrue(point_strain_rates[1] is None)

        # Search again, but include networks this time.
        point_locations = snapshot.get_point_locations(points)
        point_velocities = snapshot.get_point_velocities(points)
        point_strain_rates = snapshot.get_point_strain_rates(points)
        self.assertTrue(len(points) == len(point_locations) == len(point_velocities) == len(point_strain_rates) == 2)
        # First point is in resolved boundary 'topology2'.
        resolved_topology2 = point_locations[0].located_in_resolved_boundary()
        self.assertTrue(resolved_topology2 and resolved_topology2.get_feature().get_name() == 'topology2')
        # Second point is in resolved network 'topology3'.
        resolved_topology3 = point_locations[1].located_in_resolved_network_deforming_region()
        self.assertTrue(resolved_topology3 and resolved_topology3.get_feature().get_name() == 'topology3')
        # Both velocities and strain rates are zero.
        self.assertTrue(point_velocities == [pygplates.Vector3D.zero] * len(points))
        self.assertTrue(point_strain_rates == [pygplates.StrainRate.zero] * len(points))
    
    def test_resolve_topology_parameters(self):
        default_resolve_topology_parameters=pygplates.ResolveTopologyParameters()
        self.assertFalse(default_resolve_topology_parameters.enable_strain_rate_clamping)
        self.assertAlmostEqual(default_resolve_topology_parameters.max_clamped_strain_rate, 5e-15, 19)
        self.assertTrue(default_resolve_topology_parameters.strain_rate_smoothing == pygplates.StrainRateSmoothing.natural_neighbour)
        self.assertAlmostEqual(default_resolve_topology_parameters.rift_exponential_stretching_constant, 1.0)
        self.assertAlmostEqual(default_resolve_topology_parameters.rift_strain_rate_resolution, 5e-17, 19)
        self.assertAlmostEqual(default_resolve_topology_parameters.rift_edge_length_threshold_degrees, 0.1)

        resolve_topology_parameters=pygplates.ResolveTopologyParameters(
                enable_strain_rate_clamping=True,
                max_clamped_strain_rate=1e-14,
                strain_rate_smoothing=pygplates.StrainRateSmoothing.barycentric,
                rift_exponential_stretching_constant=1.5,
                rift_strain_rate_resolution=1e-16,
                rift_edge_length_threshold_degrees=0.2)
        self.assertTrue(resolve_topology_parameters.enable_strain_rate_clamping)
        self.assertAlmostEqual(resolve_topology_parameters.max_clamped_strain_rate, 1e-14, 19)
        self.assertTrue(resolve_topology_parameters.strain_rate_smoothing == pygplates.StrainRateSmoothing.barycentric)
        self.assertAlmostEqual(resolve_topology_parameters.rift_exponential_stretching_constant, 1.5)
        self.assertAlmostEqual(resolve_topology_parameters.rift_strain_rate_resolution, 1e-16, 19)
        self.assertAlmostEqual(resolve_topology_parameters.rift_edge_length_threshold_degrees, 0.2)

    def test_resolved_export_files(self):
        resolve_features = pygplates.FeatureCollection(os.path.join(FIXTURES, 'topologies.gpml')) 
        rotation_model = pygplates.RotationModel(os.path.join(FIXTURES, 'rotations.rot'))
        snapshot = pygplates.TopologicalSnapshot(
            resolve_features,
            rotation_model,
            pygplates.GeoTimeInstant(10))
        
        def _internal_test_export_files(
                test_case,
                snapshot,
                tmp_export_resolved_topologies_filename,
                tmp_export_resolved_topological_sections_filename):
            
            def _remove_export(tmp_export_filename):
                os.remove(tmp_export_filename)

                # In case an OGR format file (which also has shapefile mapping XML file).
                if os.path.isfile(tmp_export_filename + '.gplates.xml'):
                    os.remove(tmp_export_filename + '.gplates.xml')
                
                # For Shapefile.
                if tmp_export_filename.endswith('.shp'):
                    tmp_export_base_filename = tmp_export_filename[:-len('.shp')]
                    if os.path.isfile(tmp_export_base_filename + '.dbf'):
                        os.remove(tmp_export_base_filename + '.dbf')
                    if os.path.isfile(tmp_export_base_filename + '.prj'):
                        os.remove(tmp_export_base_filename + '.prj')
                    if os.path.isfile(tmp_export_base_filename + '.shx'):
                        os.remove(tmp_export_base_filename + '.shx')
            
            tmp_export_resolved_topologies_filename = os.path.join(FIXTURES, tmp_export_resolved_topologies_filename)
            snapshot.export_resolved_topologies(tmp_export_resolved_topologies_filename)
            test_case.assertTrue(os.path.isfile(tmp_export_resolved_topologies_filename))

            # Read back in the exported file to make sure correct number of resolved features (except cannot read '.xy' files).
            if not tmp_export_resolved_topologies_filename.endswith('.xy'):
                resolved_features = pygplates.FeatureCollection(tmp_export_resolved_topologies_filename) 
                test_case.assertTrue(len(resolved_features) == len(snapshot.get_resolved_topologies()))
            
            _remove_export(tmp_export_resolved_topologies_filename)

            tmp_export_resolved_topological_sections_filename = os.path.join(FIXTURES, tmp_export_resolved_topological_sections_filename)
            for export_topological_line_sub_segments in (True, False):
                snapshot.export_resolved_topological_sections(
                        tmp_export_resolved_topological_sections_filename,
                        export_topological_line_sub_segments=export_topological_line_sub_segments)
                test_case.assertTrue(os.path.isfile(tmp_export_resolved_topological_sections_filename))
                
                # Read back in the exported file to make sure correct number of resolved segment features (except cannot read '.xy' files).
                if not tmp_export_resolved_topological_sections_filename.endswith('.xy'):
                    # Find out how many sub-segments were exported.
                    resolved_sub_segment_features = pygplates.FeatureCollection(tmp_export_resolved_topological_sections_filename)
                    num_exported_sub_segments = sum(len(feature.get_geometries()) for feature in resolved_sub_segment_features)
                    
                    # Now find out how many sub-segments there actually are in the snapshot.
                    resolved_topological_sections = snapshot.get_resolved_topological_sections()
                    num_sub_segments = 0
                    for rts in resolved_topological_sections:
                        for sss in rts.get_shared_sub_segments():
                            # If sections were exported as the finest grain sub-segments.
                            if export_topological_line_sub_segments:
                                sub_segments = sss.get_sub_segments()
                                if sub_segments:  # resolved topological line (which has its own internal sub-segments)
                                    num_sub_segments += len(sub_segments)
                                else:
                                    num_sub_segments += 1
                            else:
                                num_sub_segments += 1
                    # Make sure they match.
                    test_case.assertTrue(num_exported_sub_segments == num_sub_segments)
                
                _remove_export(tmp_export_resolved_topological_sections_filename)
        
        # Test resolved export to different format (eg, GMT, OGRGMT, Shapefile, etc).
        _internal_test_export_files(self, snapshot, 'tmp.xy', 'tmp_sections.xy')  # GMT
        _internal_test_export_files(self, snapshot, 'tmp.shp', 'tmp_sections.shp')  # Shapefile
        _internal_test_export_files(self, snapshot, 'tmp.gmt', 'tmp_sections.gmt')  # OGRGMT
        _internal_test_export_files(self, snapshot, 'tmp.geojson', 'tmp_sections.geojson')  # GeoJSON
        _internal_test_export_files(self, snapshot, 'tmp.json', 'tmp_sections.json')  # GeoJSON

        # Test PathLike file paths (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
        # For example, "pathlib.Path" imported with "from pathlib import Path".
        if sys.version_info >= (3, 6):  # os.PathLike new in Python 3.6
            from pathlib import Path
            
            tmp_export_resolved_topologies_filename = FIXTURES / Path('tmp_export_resolved_topologies.gmt')
            snapshot.export_resolved_topologies(tmp_export_resolved_topologies_filename)
            self.assertTrue(tmp_export_resolved_topologies_filename.exists())
            tmp_export_resolved_topologies_filename.unlink()
            
            tmp_export_resolved_topological_sections_filename = FIXTURES / Path('tmp_export_resolved_topological_sections.gmt')
            snapshot.export_resolved_topological_sections(tmp_export_resolved_topological_sections_filename)
            self.assertTrue(tmp_export_resolved_topological_sections_filename.exists())
            tmp_export_resolved_topological_sections_filename.unlink()
    
    def test_calculate_plate_boundary_statistics(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))

        plate_boundary_stats = snapshot.calculate_plate_boundary_statistics(math.radians(10), first_uniform_point_spacing_radians=0.0)
        self.assertTrue(len(plate_boundary_stats) == 46)
        plate_boundary_stats = snapshot.calculate_plate_boundary_statistics(math.radians(10), first_uniform_point_spacing_radians=0.0, include_network_boundaries=True)
        self.assertTrue(len(plate_boundary_stats) == 60)
        plate_boundary_stats = snapshot.calculate_plate_boundary_statistics(math.radians(10))
        self.assertTrue(len(plate_boundary_stats) == 35)
        plate_boundary_stats = snapshot.calculate_plate_boundary_statistics(math.radians(10), include_network_boundaries=True)
        self.assertTrue(len(plate_boundary_stats) == 47)
        # Test the boundary point locations are what we expect.
        plate_boundary_point_lat_lons = [
                (-21.946252475914736, -19.346293200343844),
                (-22.263605990417016, -30.091019489088392),
                (-14.129996291084824, -11.506734532892487),
                (-14.430381668646307, -21.821783377601868),
                (-14.283044764886622, -32.143886500299004),
                (-13.693316402918944, -42.432420981802544),
                (18.326889205210314, -14.998878874263106),
                (18.554579148162833, -25.522685963030952),
                (-22.764772155019195, -39.71273543910178),
                (16.70728652671387, -48.89580498731181),
                (-31.732842469635937, -17.51643377293209),
                (39.50030441441998, -22.582837115784884),
                (33.23157851662342, -12.915515099055257),
                (-31.83922723518894, -29.204913960315768),
                (-30.99540328138162, -44.876248571709745),
                (-32.054485342279214, -34.08499535394857),
                (0.39052799774844704, -46.19246934095273),
                (-9.581073246488833, -45.50253053612027),
                (-21.756972872766777, -6.76622067408889),
                (40.06528806134546, -50.33794799230483),
                (42.35195077531569, -37.38298913948155),
                (10.312560528313567, -47.43561181664631),
                (16.077431034728548, -9.688752704075124),
                (6.098016658202067, -9.074482704326721),
                (-3.8880946684713225, -8.822690073188438),
                (-13.885676744359461, -9.045558108663817),
                (-21.39194690061765, -1.4036247206911152),
                (21.92609237556592, -89.59715591286103),
                (11.926266099667044, -89.53544210807405),
                (1.9264273486903765, -89.47815991774948),
                (-8.073412629307109, -89.42143354345046),
                (-18.073244242729142, -89.36176938774878),
                (29.430057905628797, -84.72043539544897),
                (33.843078259328216, -74.17084500284454),
                (37.26900636827229, -62.61183577799688),
                (-25.926603811980286, -83.74805508707956),
                (-25.827966619343684, -72.6707317363454),
                (-26.05969135338931, -61.55260351765192),
                (-26.04527471565717, -50.43015875739786),
                (-31.73370294004206, -5.992946229710096),
                (-19.57764637188272, -45.231632684686),
                (19.068459681220613, -36.06969403649895),
                (19.399001381324304, -46.65673771463496),
                (35.68660277464262, -48.1785066964556),
                (27.47040397496536, -41.474010277625624),
                (26.07532695910801, -9.908990918811437),
                (-13.392730536929992, -1.2383560708492212)
        ]
        plate_boundary_points = [pygplates.PointOnSphere(lat, lon) for lat, lon in plate_boundary_point_lat_lons]
        for stat in plate_boundary_stats:
            self.assertTrue(stat.boundary_point in plate_boundary_points)

        # Access PlateBoundaryStatistic attributes - just to make sure they can be queried.
        for plate_boundary_stat in plate_boundary_stats:
            plate_boundary_stat.boundary_point
            self.assertTrue(plate_boundary_stat.boundary_length <= 2*math.radians(10) and plate_boundary_stat.boundary_length >= 0)
            self.assertAlmostEqual(plate_boundary_stat.boundary_normal.get_magnitude(), 1.0)
            self.assertTrue(plate_boundary_stat.boundary_normal_azimuth <= 2*math.pi and plate_boundary_stat.boundary_normal_azimuth >= 0)
            self.assertTrue(plate_boundary_stat.boundary_velocity == pygplates.Vector3D.zero)
            plate_boundary_stat.boundary_velocity_magnitude
            plate_boundary_stat.boundary_velocity_obliquity
            plate_boundary_stat.boundary_velocity_orthogonal
            plate_boundary_stat.boundary_velocity_parallel
            plate_boundary_stat.left_plate
            plate_boundary_stat.left_plate_velocity
            plate_boundary_stat.left_plate_velocity_magnitude
            plate_boundary_stat.left_plate_velocity_obliquity
            plate_boundary_stat.left_plate_velocity_orthogonal
            plate_boundary_stat.left_plate_velocity_parallel
            self.assertTrue(plate_boundary_stat.left_plate_strain_rate == pygplates.StrainRate.zero)
            plate_boundary_stat.right_plate
            plate_boundary_stat.right_plate_velocity
            plate_boundary_stat.right_plate_velocity_magnitude
            plate_boundary_stat.right_plate_velocity_obliquity
            plate_boundary_stat.right_plate_velocity_orthogonal
            plate_boundary_stat.right_plate_velocity_parallel
            self.assertTrue(plate_boundary_stat.right_plate_strain_rate == pygplates.StrainRate.zero)
            plate_boundary_stat.convergence_velocity
            plate_boundary_stat.convergence_velocity_signed_magnitude
            plate_boundary_stat.convergence_velocity_magnitude
            plate_boundary_stat.convergence_velocity_obliquity
            plate_boundary_stat.convergence_velocity_orthogonal
            plate_boundary_stat.convergence_velocity_parallel
            plate_boundary_stat.distance_from_start_of_shared_sub_segment
            plate_boundary_stat.distance_to_end_of_shared_sub_segment
            plate_boundary_stat.distance_from_start_of_topological_section
            plate_boundary_stat.signed_distance_from_start_of_topological_section
            plate_boundary_stat.distance_to_end_of_topological_section
            plate_boundary_stat.signed_distance_to_end_of_topological_section
            
            # Test equality.
            self.assertTrue(plate_boundary_stat == plate_boundary_stat)

        # Return a dict mapping each shared sub-segment to its statistics.
        plate_boundary_stats_dict = snapshot.calculate_plate_boundary_statistics(math.radians(10),
                                                                                 include_network_boundaries=True,
                                                                                 return_shared_sub_segment_dict=True)
        self.assertTrue(len(plate_boundary_stats_dict) == 26)
        self.assertTrue(sum(len(shared_sub_segment_stats) for _, shared_sub_segment_stats in plate_boundary_stats_dict.items()) == 47)
        for shared_sub, shared_sub_segment_stats in plate_boundary_stats_dict.items():
            shared_sub_feature_name = shared_sub.get_feature().get_name()
            for shared_sub_segment_stat in shared_sub_segment_stats:
                self.assertTrue(shared_sub_segment_stat.shared_sub_segment == shared_sub)
                # Boundary feature matches shared sub-segment if it's an RFG, otherwise matches sub-segments of shared sub-segment if it's an RTL.
                if shared_sub_feature_name == 'section14':  # the only topological line
                    shared_sub_sub_feature_names = [shared_sub_sub_segment.get_feature().get_name() for shared_sub_sub_segment in shared_sub.get_sub_segments()]
                    self.assertTrue(shared_sub_segment_stat.boundary_feature.get_name() in shared_sub_sub_feature_names)
                else:
                    self.assertTrue(shared_sub_segment_stat.boundary_feature.get_name() == shared_sub_feature_name)

        # Filter boundary sections by feature type.
        plate_boundary_stats_filtered = snapshot.calculate_plate_boundary_statistics(math.radians(10),
                                                                                 include_network_boundaries=True,
                                                                                 # All boundary sections are this feature type...
                                                                                 boundary_section_filter=pygplates.FeatureType.gpml_unclassified_feature,
                                                                                 return_shared_sub_segment_dict=True)
        self.assertTrue(len(plate_boundary_stats_filtered) == 26)
        plate_boundary_stats_filtered = snapshot.calculate_plate_boundary_statistics(math.radians(10),
                                                                                 include_network_boundaries=True,
                                                                                 # None of the boundary sections include these feature types...
                                                                                 boundary_section_filter=[pygplates.FeatureType.gpml_subduction_zone, pygplates.FeatureType.gpml_mid_ocean_ridge],
                                                                                 return_shared_sub_segment_dict=True)
        self.assertTrue(len(plate_boundary_stats_filtered) == 0)
        plate_boundary_stats_filtered = snapshot.calculate_plate_boundary_statistics(math.radians(10),
                                                                                 include_network_boundaries=True,
                                                                                 # Filter boundary sections that are resolved topological lines...
                                                                                 boundary_section_filter=lambda rts: isinstance(rts.get_topological_section(), pygplates.ResolvedTopologicalLine),
                                                                                 return_shared_sub_segment_dict=True)
        # There is only one resolved topological line, but it has multiple shared sub-segments.
        topological_sections = set(shared_sub_segment.get_topological_section() for shared_sub_segment in plate_boundary_stats_filtered.keys())
        self.assertTrue(len(topological_sections) == 1)
    
    def test_pickle(self):
        snapshot = pygplates.TopologicalSnapshot(
            os.path.join(FIXTURES, 'topologies.gpml'),
            os.path.join(FIXTURES, 'rotations.rot'),
            pygplates.GeoTimeInstant(10))
        
        # Pickle the TopologicalSnapshot.
        pickled_snapshot = pickle.loads(pickle.dumps(snapshot))
        self.assertTrue(pickled_snapshot.get_rotation_model().get_rotation(100, 802) == snapshot.get_rotation_model().get_rotation(100, 802))
        # Check the original and pickled topological snapshots.
        resolved_topologies = snapshot.get_resolved_topologies(same_order_as_topological_features=True)
        pickled_resolved_topologies = pickled_snapshot.get_resolved_topologies(same_order_as_topological_features=True)
        self.assertTrue(len(pickled_resolved_topologies) == len(resolved_topologies))
        for index in range(len(pickled_resolved_topologies)):
            self.assertTrue(pickled_resolved_topologies[index].get_resolved_geometry() == resolved_topologies[index].get_resolved_geometry())


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            CalculateVelocitiesTestCase,
            CrossoverTestCase,
            InterpolateTotalReconstructionSequenceTestCase,
            NetRotationTestCase,
            PlatePartitionerTestCase,
            ReconstructModelTestCase,
            ReconstructSnapshotTestCase,
            ReconstructTestCase,
            ReconstructionTreeCase,
            ResolvedTopologiesTestCase,
            RotationModelTestCase,
            StrainTestCase,
            TopologicalModelTestCase,
            TopologicalSnapshotTestCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
