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


from collections import namedtuple
import math


Crossover = namedtuple("Crossover",
        "time "
        "moving_plate_id "
        "pre_crossover_fixed_plate_id post_crossover_fixed_plate_id "
        "pre_crossover_rotation_sequence post_crossover_rotation_sequence")


def find_crossovers(rotation_features, crossover_filter=None):
    """find_crossovers(rotation_features[, crossover_filter]) -> list
    Find crossovers in  rotation features.
    
    :param rotation_features: A rotation feature collection, or rotation filename, or \
    rotation feature, or sequence of rotation features, or a sequence (eg, ``list`` or ``tuple``) \
    of any combination of those four types
    :type rotation_features: :class:`FeatureCollection`, or string, or :class:`Feature`, \
    or sequence of :class:`Feature`, or sequence of any combination of those four types
    :param crossover_filter: a predicate function to determine which crossovers to return
    :type crossover_filter: a callable accepting a single named-tuple 'Crossover' argument
    :returns: a time-sorted list (from most recent to least recent) of crossover named-tuple 'Crossover' with named elements \
    (time, moving_plate_id, pre_crossover_fixed_plate_id, post_crossover_fixed_plate_id, \
    pre_crossover_rotation_sequence, post_crossover_rotation_sequence)
    :rtype: list of named-tuple 'Crossover' (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`)
    
    A crossover is a named-tuple 'Crossover' with named elements (time, moving_plate_id,
    pre_crossover_fixed_plate_id, post_crossover_fixed_plate_id,
    pre_crossover_rotation_sequence, post_crossover_rotation_sequence) and with type
    (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`).
    
    A crossover occurs when the motion of a (moving) plate crosses over from one (fixed) plate to move relative
    to another (fixed) plate at a particular geological time. The named-tuple 'Crossover' stores the crossover time,
    moving plate id, the fixed plate id before and after the crossover and the time sequence of
    :class:`finite rotations<GpmlFiniteRotation>` before and after the crossover. Before (pre) means younger (or more recent)
    and after (post) means older (or less recent).
    
    Note that the returned list of crossovers is sorted by time from most recent to least recent.
    This is useful when synchronising crossovers since more recent crossovers can only affect crossovers further in the past.
    The returned list is also optionally filtered if *crossover_filter* is specified.
    
    A rotation feature has a :class:`feature type<FeatureType>` of `total reconstruction sequence
    <http://www.earthbyte.org/Resources/GPGIM/public/#TotalReconstructionSequence>`_ and contains a time sequence
    of :class:`finite rotations<GpmlFiniteRotation>` (see :meth:`total reconstruction pole<Feature.get_total_reconstruction_pole>`)
    for a specific fixed/moving plate pair. Each crossover essentially returns information of the
    :meth:`total reconstruction pole<Feature.get_total_reconstruction_pole>` for the rotation feature before the crossover and
    for the rotation feature after the crossover as well as the time of the crossover - note that both rotation features will have the same
    moving plate id but differing fixed plate ids.
    
    Note that modifying the rotation sequences in the returned crossovers will modify the rotation features.
    This is what happens if the returned crossovers are passed into :func:`synchronise_crossovers`.
    The modified rotation features can then be used to create a new :class:`RotationModel` with updated rotations.
    
    If any rotation filenames are specified then this method uses
    :class:`FeatureCollectionFileFormatRegistry` internally to read the rotation files.
    
    Find all crossovers in a rotation file:
    ::
    
      crossovers = pygplates.find_crossovers('rotations.rot')
      for crossover in crossovers:
          print 'Crossover time: %f' % crossover.time
          print 'Crossover moving plate id: %d' % crossover.moving_plate_id
          print 'Crossover younger fixed plate id: %d' % crossover.pre_crossover_fixed_plate_id
          print 'Crossover older fixed id: %d' % crossover.post_crossover_fixed_plate_id
          print 'Crossover younger finite rotation: %d' % \\
              crossover.pre_crossover_rotation_sequence.get_value(crossover.time) \\
              .get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
          print 'Crossover older finite rotation: %d' % \\
              crossover.post_crossover_rotation_sequence.get_value(crossover.time) \\
              .get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
    
    Find crossovers affecting moving plate 801:
    ::
    
      crossovers_801 = pygplates.find_crossovers(
          rotation_feature_collection,
          lambda crossover: crossover.moving_plate_id==801)
    """
    
    # Use helper class to convert 'rotation_features' argument to a list of features.
    rotation_features = FeaturesFunctionArgument(rotation_features)
    
    # A 'dict' to map each moving plate to a list of total reconstruction poles
    # (one per moving/fixed plate pair)
    total_reconstruction_poles_by_moving_plate = {}
    
    # Get the moving/fixed total reconstruction poles.
    for rotation_feature in rotation_features.get_features():
        total_reconstruction_pole = rotation_feature.get_total_reconstruction_pole()
        # If the current feature is a valid rotation feature...
        if total_reconstruction_pole:
            fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
            # Each moving plate has a list of fixed plates.
            total_reconstruction_poles_by_moving_plate.setdefault(
                moving_plate_id, []).append(total_reconstruction_pole)
    
    crossovers = []
    
    # Iterate over the moving plates.
    for moving_plate_id, total_reconstruction_poles in total_reconstruction_poles_by_moving_plate.iteritems():
        # Only one fixed plate means no crossovers for the current moving plate.
        if len(total_reconstruction_poles) > 1:
            # Iterate over all possible pairs of fixed plates (of the current moving plate) - two 'for' loops.
            for pre_index, pre_crossover_total_reconstruction_pole in enumerate(total_reconstruction_poles):
                # fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
                pre_crossover_fixed_plate_id = pre_crossover_total_reconstruction_pole[0]
                pre_crossover_rotation_sequence = pre_crossover_total_reconstruction_pole[2]
                # Find the last enabled time sample using 'next()' builtin function.
                pre_crossover_last_enabled_time_sample = next(
                    (ts for ts in reversed(pre_crossover_rotation_sequence.get_time_samples()) if ts.is_enabled()),
                    None)
                # If no time samples are enabled for some reason then skip.
                if not pre_crossover_last_enabled_time_sample:
                    continue
                
                for post_index, post_crossover_total_reconstruction_pole in enumerate(total_reconstruction_poles):
                    if pre_index == post_index:
                        continue
                    # fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
                    post_crossover_fixed_plate_id = post_crossover_total_reconstruction_pole[0]
                    post_crossover_rotation_sequence = post_crossover_total_reconstruction_pole[2]
                    # Find the first enabled time sample using 'next()' builtin function.
                    post_crossover_first_enabled_time_sample = next(
                        (ts for ts in post_crossover_rotation_sequence.get_time_samples() if ts.is_enabled()),
                        None)
                    # If no time samples are enabled for some reason then skip.
                    if not post_crossover_first_enabled_time_sample:
                        continue
                    
                    # If the time of the last sample of pre crossover sequence matches the time of
                    # the first sample of the post crossover sequence then we've found a crossover.
                    #
                    # We use the tolerance built into GeoTimeInstant for equality comparison.
                    # Avoids precision issues of directly comparing floating-point numbers for equality.
                    if GeoTimeInstant(pre_crossover_last_enabled_time_sample.get_time()) == \
                            post_crossover_first_enabled_time_sample.get_time():
                        crossover = Crossover(
                            post_crossover_first_enabled_time_sample.get_time(),
                            moving_plate_id,
                            pre_crossover_fixed_plate_id,
                            post_crossover_fixed_plate_id,
                            pre_crossover_rotation_sequence,
                            post_crossover_rotation_sequence)
                        # If caller is filtering crossovers then append crossover conditionally.
                        if not crossover_filter or crossover_filter(crossover):
                            crossovers.append(crossover)
    
    # Sort the list of crossovers by time since more recent crossovers can only affect
    # crossovers further in the past.
    # This enables the caller to process crossovers from most recent to least recent.
    crossovers.sort(key = lambda crossover: crossover.time)
    
    return crossovers


def synchronise_crossovers(
        rotation_features,
        crossover_filter=None,
        crossover_threshold_degrees=None,
        crossover_results=None):
    """synchronise_crossovers(rotation_features, [crossover_filter], [crossover_threshold_degrees], [crossover_results])
    Synchronise crossovers in rotation features.
    
    :param rotation_features: A rotation feature collection, or rotation filename, or \
    rotation feature, or sequence of rotation features, or a sequence (eg, ``list`` or ``tuple``) \
    of any combination of those four types - all features are used as input and output
    :type rotation_features: :class:`FeatureCollection`, or string, or :class:`Feature`, \
    or sequence of :class:`Feature`, or sequence of any combination of those four types
    :param crossover_filter: optional predicate function (accepting a single crossover argument) that determines \
    which crossovers (in *rotation_features*) to synchronise, or an optional sequence of crossovers (in any order) to \
    synchronise - if nothing is specified then all crossovers (in *rotation_features*) are synchronised - \
    a crossover is a named-tuple 'Crossover' with named elements (time, moving_plate_id, \
    pre_crossover_fixed_plate_id, post_crossover_fixed_plate_id, \
    pre_crossover_rotation_sequence, post_crossover_rotation_sequence)
    :type crossover_filter: a callable accepting a single named-tuple 'Crossover' argument, or \
    a sequence of named-tuple 'Crossover' (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`)
    :param crossover_threshold_degrees: If specified then crossovers are synchronised (fixed) only if the \
    post-crossover rotation latitude, longitude or angle differ from those in pre-crossover rotation by \
    more than this amount
    :type crossover_threshold_degrees: float or None
    :param crossover_results: if specified then a tuple of (Crossover, bool) is appended for each filtered \
    crossover where the boolean value is ``True`` if the crossover passed (did not require synchronising) or \
    ``False`` if crossover was synchronised - the list is sorted by crossover time - note that the list is \
    *not* cleared first - default is None
    :type crossover_results: list or None
    
    A crossover is a named-tuple 'Crossover' with named elements (time, moving_plate_id,
    pre_crossover_fixed_plate_id, post_crossover_fixed_plate_id,
    pre_crossover_rotation_sequence, post_crossover_rotation_sequence) and with type
    (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`).
    
    A crossover occurs when the motion of a (moving) plate crosses over from one (fixed) plate to move relative
    to another (fixed) plate at a particular geological time. Synchronising a crossover involves adjusting the finite rotations
    after the crossover to match the finite rotation before the crossover. Before (pre) means younger (more recent)
    and after (post) means older (less recent). Crossovers are synchronised in time order from most recent to least recent
    (since more recent crossovers can only affect crossovers further in the past).
    
    Note that *rotation_features* should contain all features that directly or indirectly affect the crossovers to be
    synchronised (typically *rotation_features* is an entire rotation file), otherwise crossovers may not be correctly synchronised.
    
    Synchronising crossovers results in modifications to *rotation_features*.
    The modified rotation features can then be used to create a new :class:`RotationModel` with updated rotations.
    If any filenames are specified in *rotation_features* then the modified feature collection(s) (containing synchronised crossovers)
    that are associated with those files are written back out to those same files.
    :class:`FeatureCollectionFileFormatRegistry` is used internally to read/write feature collections from/to those files.
    
    *crossover_filter* can optionally be used to limit (or specify) the crossovers to synchronise. It can either be a predicate function
    (accepting a single crossover argument) or a sequence of crossovers. If *crossover_filter* is not specified then all
    crossovers (found using :func:`find_crossovers` on *rotation_features*) will be synchronised. If *crossover_filter* is a
    predicate function then only those crossovers (found using :func:`find_crossovers` on *rotation_features*) that pass the
    predicate test will be synchronised. If *crossover_filter* is a sequence of crossovers then only crossovers in that sequence will be
    synchronised.
    
    *crossover_threshold_degrees* can optionally be used to synchronise (fix) only those crossovers whose
    difference in pre and post crossover rotation latitudes, longitudes or angles exceeds this amount. This is
    useful some PLATES rotation files that are typically accurate to 2 decimal places (or threshold of 0.01).
    
    *crossover_results* can optionally be used to obtain a list of the synchronisation results of all
    filtered crossovers (see *crossover_filter*). Each list element is a tuple of (Crossover, bool)
    where the boolean value is ``True`` if the crossover passed (did not require synchronisation) or
    ``False`` if the crossover was synchronised. The list is sorted by crossover time.
    
    A rotation feature has a :class:`feature type<FeatureType>` of `total reconstruction sequence
    <http://www.earthbyte.org/Resources/GPGIM/public/#TotalReconstructionSequence>`_ and contains a time sequence
    of :class:`finite rotations<GpmlFiniteRotation>` (see :meth:`total reconstruction pole<Feature.get_total_reconstruction_pole>`)
    for a specific fixed/moving plate pair.
    
    Synchronise all crossovers found in a rotation file (and save modifications back to the same rotation file):
    ::
    
      pygplates.synchronise_crossovers('rotations.rot')
    
    Synchronise crossovers between present day and 20Ma found in a PLATES rotation file that has rotation
    latitudes, longitudes and angles rounded to 2 decimal places (and save modifications back to the same rotation file):
    ::
      
      crossover_results = []
      pygplates.synchronise_crossovers(
          'rotations.rot',
          lambda crossover: crossover.time <= 20,
          0.01, # Equivalent to 2 decimal places
          crossover_results)
      print 'Fixed %d crossovers' % sum(1 for result in crossover_results if not result[1])
    """
    
    # Use helper class to convert 'rotation_features' argument to a list of features.
    rotation_features = FeaturesFunctionArgument(rotation_features)
    
    # Also note that using 'FeaturesFunctionArgument' for 'rotation_features' ensures that the rotation features
    # are not getting reloaded (from files, if any) at each iteration of the crossovers loop (into RotationModel).
    # If we passed files (if any) directly to RotationModel then we'd be forced to write the modifications back
    # out to file for each iteration of the crossovers loop.
    rotation_feature_sequence = rotation_features.get_features()
    
    # Make sure threshold is a number.
    if crossover_threshold_degrees is not None:
        crossover_threshold_degrees = float(crossover_threshold_degrees)

    # If caller specified a sequence of crossovers then use them, otherwise find them in the rotation features.
    if hasattr(crossover_filter, '__iter__'):
        # Sort the sequence of crossovers by time since more recent crossovers can only affect
        # crossovers further in the past.
        # This means we'll process crossovers from most recent to least recent.
        # We use builtin function 'sorted' since we don't know whether 'crossover_filter' iterable is a list or not.
        crossovers = sorted(crossover_filter, key = lambda crossover: crossover.time)
    else: # ...'crossover_filter' is None or a callable...
        # The returned sequence is already sorted by time.
        crossovers = find_crossovers(rotation_feature_sequence, crossover_filter)
    
    for crossover in crossovers:
        #print 'Fixing crossover at time(%f), mpid(%d), pre_fpid(%d), post_fpid(%d)' % (
        #        crossover.time,
        #        crossover.moving_plate_id,
        #        crossover.pre_crossover_fixed_plate_id,
        #        crossover.post_crossover_fixed_plate_id)
        
        # Note that the rotation model will include the crossover modifications made so far because
        # the modifications were made to the properties of the rotation features that we're passing
        # into the rotation model here.
        # So it is important that this RotationModel is created *inside* the loop over crossovers.
        rotation_model = RotationModel(rotation_feature_sequence)
        
        # Get the post-crossover rotation time samples - ignore disabled samples.
        post_crossover_time_samples = crossover.post_crossover_rotation_sequence.get_enabled_time_samples()
        
        # The first post-crossover time sample.
        old_post_crossover_moving_fixed_relative_rotation = \
            post_crossover_time_samples[0].get_value().get_finite_rotation()

        # We assume the first half of the crossover (pre-crossover) is correct and we want to adjust
        # the second half (post-crossover) to match. So we get the rotation relative to the
        # *post*-crossover fixed plate but at a *pre*-crossover time.
        #
        # Use a small delta time decrement from the crossover time to ensure we get the
        # rotation just prior to the crossover (ie, we don't want to take the plate circuit path
        # through the *post*-crossover fixed plate).
        #
        # The delta is a bit arbitrary. We make it small enough to avoid deviating from the true
        # rotation too much, but not too small that pygplates includes both pre and post crossover
        # paths in its reconstruction tree thus making the plate circuit path (used for the rotation)
        # a bit arbitrary - pygplates tolerance for this is currently about 1e-9.
        new_post_crossover_moving_fixed_relative_rotation = rotation_model.get_rotation(
            crossover.time - 1e-4,
            crossover.moving_plate_id,
            # Note: Setting anchor plate instead of fixed plate in case there's no plate circuit path
            # from plate zero (default anchor plate) to moving or fixed plates (see pygplates API docs)...
            anchor_plate_id=crossover.post_crossover_fixed_plate_id)
            
        # Determine the crossover adjustment to apply to all post-crossover time samples.
        #
        # The pre-crossover total rotation at time t2 (assuming t2 is a crossover time):
        #   R(0->t2, Fa->M)
        # The old post-crossover total rotation at time t2:
        #   R(0->t2, Fb->M)
        # The new (fixed) post-crossover total rotation at time t2:
        #   R'(0->t2, Fb->M)
        #
        # The old post-crossover total rotation at an older time t4 is composed of stage poles:
        #   R(0->t4) = R(t3->t4) * R(t2->t3) * R(0->t2, Fb->M)
        #            = R(t2->t4) * R(0->t2, Fb->M)
        # The stage pole from t2->t4:
        #   R(t2->t4) = R(0->t4) * inverse[R(0->t2, Fb->M)]
        # The new post-crossover total rotation at an older time t4 is composed of stage poles:
        #   R'(0->t4) = R(t3->t4) * R(t2->t3) * R'(0->t2, Fb->M)
        #             = R(t2->t4) * R'(0->t2, Fb->M)
        # Which, using the stage pole above to replace 'R(t2->t4)', is:
        #   R'(0->t4) = R(0->t4) * inverse[R(0->t2, Fb->M)] * R'(0->t2, Fb->M)
        #             = R(0->t4) * crossover_adjustment
        # So each total pole gets post-multiplied by a constant crossover adjustment of:
        #   crossover_adjustment = inverse[R(0->t2, Fb->M)] * R'(0->t2, Fb->M)
        #
        crossover_adjustment = (old_post_crossover_moving_fixed_relative_rotation.get_inverse() *
                new_post_crossover_moving_fixed_relative_rotation)
        
        # Skip fixing crossover if no adjustment is needed.
        crossover_passed = False
        
        # Always test for identity rotation first because an identity rotation has zero angle but
        # can have an arbitrary pole and doing a threshold test against an arbitrary pole will almost
        # always fail (so we do that test second if not an identity rotation).
        if crossover_adjustment.represents_identity_rotation():
            crossover_passed = True
        elif crossover_threshold_degrees is not None:
            old_post_crossover_lat, old_post_crossover_lon, old_post_crossover_angle = \
                    old_post_crossover_moving_fixed_relative_rotation.get_lat_lon_euler_pole_and_angle_degrees()
            
            new_post_crossover_pole, new_post_crossover_angle_radians = \
                    new_post_crossover_moving_fixed_relative_rotation.get_euler_pole_and_angle()
            new_post_crossover_lat, new_post_crossover_lon = new_post_crossover_pole.to_lat_lon()
            new_post_crossover_angle = math.degrees(new_post_crossover_angle_radians)
            
            # See if both rotations are close enough that we don't require crossover adjustments.
            if (abs(new_post_crossover_lat - old_post_crossover_lat) <= crossover_threshold_degrees and
                abs(new_post_crossover_lon - old_post_crossover_lon) <= crossover_threshold_degrees and
                abs(new_post_crossover_angle - old_post_crossover_angle) <= crossover_threshold_degrees):
                crossover_passed = True
            else:
                # The antipodal rotation pole with a negated angle represents the exact same rotation.
                # So we compare that for closeness also.
                new_post_crossover_pole_x, new_post_crossover_pole_y, new_post_crossover_pole_z = \
                        new_post_crossover_pole.to_xyz()
                new_post_crossover_pole_antipodal = PointOnSphere((
                        -new_post_crossover_pole_x, -new_post_crossover_pole_y, -new_post_crossover_pole_z))
                new_post_crossover_lat_antipodal, new_post_crossover_lon_antipodal = \
                        new_post_crossover_pole_antipodal.to_lat_lon()
                new_post_crossover_angle_antipodal = -new_post_crossover_angle
                if (abs(new_post_crossover_lat_antipodal - old_post_crossover_lat) <= crossover_threshold_degrees and
                    abs(new_post_crossover_lon_antipodal - old_post_crossover_lon) <= crossover_threshold_degrees and
                    abs(new_post_crossover_angle_antipodal - old_post_crossover_angle) <= crossover_threshold_degrees):
                    crossover_passed = True
        
        # Add to the list of crossover results if requested.
        if crossover_results is not None:
            crossover_results.append((crossover, crossover_passed))
        
        if crossover_passed:
            continue
        
        # Change the first time sample.
        post_crossover_time_samples[0].get_value().set_finite_rotation(
            new_post_crossover_moving_fixed_relative_rotation)
        # Change the remaining rotation time samples.
        for post_crossover_time_sample in post_crossover_time_samples[1:]:
            # Get, adjust and set.
            post_crossover_rotation = post_crossover_time_sample.get_value().get_finite_rotation()
            post_crossover_rotation = post_crossover_rotation * crossover_adjustment
            post_crossover_time_sample.get_value().set_finite_rotation(post_crossover_rotation)

    # If any rotation features came from files then write those feature collections back out to the same files.
    #
    # Only interested in those feature collections that came from files (have filenames).
    rotation_files = rotation_features.get_files()
    if rotation_files:
        file_format_registry = FeatureCollectionFileFormatRegistry()
        for feature_collection, filename in rotation_files:
            # This can raise OpenFileForWritingError if file is not writable.
            file_format_registry.write(feature_collection, filename)
