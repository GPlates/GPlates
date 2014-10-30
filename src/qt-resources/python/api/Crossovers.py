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
from functools import partial
import math


# The maximum number of iterations over all crossovers to be synchronised.
# If this is exceeded then it's likely there was an infinite cycle.
_MAX_CROSSOVER_ITERATIONS = 8


# An enumeration (class with static integers) to determine how to synchronise a crossover.
class CrossoverType(object):
    unknown = 0
    synch_old_crossover_and_stages = 1
    synch_old_crossover_only = 2
    synch_young_crossover_and_stages = 3
    synch_young_crossover_only = 4
    
    # Private static method.
    @staticmethod
    def _is_valid(type):
        """Returns True if 'type' is a valid enumeration."""
        return (isinstance(type, int) and
            type >= CrossoverType.unknown and
            type <= CrossoverType.synch_young_crossover_only)
    
    # Private static method.
    @staticmethod
    def _synch_young_crossover(type):
        """Returns True if 'type' specifies that the young crossover should be synchronised."""
        return (type == CrossoverType.synch_young_crossover_and_stages or
            type == CrossoverType.synch_young_crossover_only)
    
    # Private static method.
    @staticmethod
    def _synch_old_crossover(type):
        """Returns True if 'type' specifies that the old crossover should be synchronised."""
        return (type == CrossoverType.synch_old_crossover_and_stages or
            type == CrossoverType.synch_old_crossover_only)
    
    # Private static method.
    @staticmethod
    def _synch_stages(type):
        """Returns True if 'type' specifies that the stages in the crossover sequence should be synchronised."""
        return (type == CrossoverType.synch_old_crossover_and_stages or
            type == CrossoverType.synch_young_crossover_and_stages)


# A (class) namespace for some common functions to determine crossover type.
class CrossoverTypeFunction(object):
    @staticmethod
    def type_from_xo_tags_in_comment(
            crossover_time,
            moving_plate_id,
            young_crossover_fixed_plate_id,
            old_crossover_fixed_plate_id,
            young_crossover_rotation_sequence,
            old_crossover_rotation_sequence):
        """Extracts the crossover type using the @xo_ys, @xo_yf, @xo_os and @xo_of tags in the
        comment/description field of the 'young' crossover pole."""
        # The time sample specifying the crossover type (in its description field) is the younger crossover.
        annotated_time_sample_description = (
                young_crossover_rotation_sequence.get_enabled_time_samples()[-1].get_description())
        # If description field is present (not None) and is a non-empty string.
        if annotated_time_sample_description:
            # We should have one of '@xo_ys', '@xo_yf', '@xo_os' or '@xo_of'.
            annotate_index = annotated_time_sample_description.find('@xo_')
            if annotate_index >= 0:
                type_field = annotated_time_sample_description[annotate_index+4 : annotate_index+6]
                if type_field == 'ys':
                    return CrossoverType.synch_old_crossover_and_stages
                elif type_field == 'yf':
                    return CrossoverType.synch_old_crossover_only
                elif type_field == 'os':
                    return CrossoverType.synch_young_crossover_and_stages
                elif type_field == 'of':
                    return CrossoverType.synch_young_crossover_only
        
        return CrossoverType.unknown


# An enumeration (class with static integers) of crossover results (whether and how they were processed).
class CrossoverResult(object):
    error = 0             # Crossover was not processed due to an error (eg, crossover type was unknown).
    synchronised = 1      # Crossover was synchronised.
    not_synchronised = 2  # Crossover did not need synchronisation (was already in-synch).


Crossover = namedtuple("Crossover",
    "type "
    "time "
    "moving_plate_id "
    "young_crossover_fixed_plate_id "
    "old_crossover_fixed_plate_id "
    "young_crossover_rotation_sequence "
    "old_crossover_rotation_sequence ")


def find_crossovers(
        rotation_features,
        crossover_filter=None,
        crossover_type_function=CrossoverTypeFunction.type_from_xo_tags_in_comment):
    """find_crossovers(rotation_features, [crossover_filter], \
    [crossover_type_function=CrossoverTypeFunction.type_from_xo_tags_in_comment]) -> list
    Find crossovers in  rotation features.
    
    :param rotation_features: A rotation feature collection, or rotation filename, or \
    rotation feature, or sequence of rotation features, or a sequence (eg, ``list`` or ``tuple``) \
    of any combination of those four types
    :type rotation_features: :class:`FeatureCollection`, or string, or :class:`Feature`, \
    or sequence of :class:`Feature`, or sequence of any combination of those four types
    :param crossover_filter: a predicate function to determine which crossovers to return
    :type crossover_filter: a callable accepting a single named-tuple 'Crossover' argument
    :param crossover_type_function: a function that determines a crossover's type, or one of the \
    *CrossoverType* enumerated values, or *CrossoverTypeFunction.type_from_xo_tags_in_comment* if using default \
    scheme for determining crossover type (see below) - default is *CrossoverTypeFunction.type_from_xo_tags_in_comment*
    :type crossover_type_function: a callable, or a *CrossoverType* enumerated value, or None
    :returns: a time-sorted list, from most recent (youngest) to least recent (oldest), of crossover \
    named-tuple 'Crossover' with named elements (time, moving_plate_id, young_crossover_fixed_plate_id, \
    old_crossover_fixed_plate_id, young_crossover_rotation_sequence, old_crossover_rotation_sequence)
    :rtype: list of named-tuple 'Crossover' (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`)
    
    A crossover is a named-tuple 'Crossover' with named elements (time, moving_plate_id,
    young_crossover_fixed_plate_id, old_crossover_fixed_plate_id,
    young_crossover_rotation_sequence, old_crossover_rotation_sequence) and with type
    (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`).
    Both the young and old crossover rotation sequences will each have at least one enabled time sample.
    
    A crossover occurs when the motion of a (moving) plate crosses over from one (fixed) plate to move relative
    to another (fixed) plate at a particular geological time. The named-tuple 'Crossover' stores the crossover time,
    moving plate id, the fixed plate id after (younger) and before (older) the crossover and the time sequence of
    :class:`finite rotations<GpmlFiniteRotation>` after (younger) and before (older) the crossover.
    Younger means more recent (smaller time values) and older means less recent (larger time values).
    
    Note that the returned list of crossovers is sorted by time from most recent (younger) to least recent (older).
    The returned list is also optionally filtered if *crossover_filter* is specified.
    
    The following arguments are supported by *crossover_type_function*:
    
    +----------------------------------------------------+---------------------------------------------------------------+
    | Type                                               | Description                                                   |
    +====================================================+===============================================================+
    | Arbitrary callable (function)                      | A callable accepting the following arguments:                 |
    |                                                    |                                                               |
    |                                                    | - crossover_time                                              |
    |                                                    | - moving_plate_id                                             |
    |                                                    | - young_crossover_fixed_plate_id                              |
    |                                                    | - old_crossover_fixed_plate_id                                |
    |                                                    | - young_crossover_rotation_sequence                           |
    |                                                    | - old_crossover_rotation_sequence                             |
    |                                                    |                                                               |
    |                                                    | ...and returning a *CrossoverType* enumerated value.          |
    +----------------------------------------------------+---------------------------------------------------------------+
    | CrossoverTypeFunction.type_from_xo_tags_in_comment | A callable (with same arguments as arbitrary callable) that   |
    |                                                    | uses the comment/description field of the *young* crossover   |
    |                                                    | pole to determine the crossover type according to the         |
    |                                                    | presence of the following strings/tags:                       |
    |                                                    |                                                               |
    |                                                    | - \@xo_ys : *CrossoverType.synch_old_crossover_and_stages*    |
    |                                                    | - \@xo_yf : *CrossoverType.synch_old_crossover_only*          |
    |                                                    | - \@xo_os : *CrossoverType.synch_young_crossover_and_stages*  |
    |                                                    | - \@xo_of : *CrossoverType.synch_young_crossover_only*        |
    |                                                    |                                                               |
    |                                                    | ...and if none of those tags are present then the crossover   |
    |                                                    | type is *CrossoverType.unknown*.                              |
    |                                                    |                                                               |
    |                                                    | This is the default for *crossover_type_function*.            |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.unknown*                            | All crossovers will be of unknown type                        |
    |                                                    | (no crossovers will be synchronised).                         |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_old_crossover_and_stages*     | All finite rotations in all *old* crossover                   |
    |                                                    | sequences will be synchronised (such that old *stage*         |
    |                                                    | rotations are preserved). All finite rotations in             |
    |                                                    | *young* crossover sequences are preserved.                    |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_old_crossover_only*           | Only the crossover finite rotation in all *old* crossover     |
    |                                                    | sequences will be synchronised (such that the older           |
    |                                                    | *finite* rotations are preserved). All finite rotations       |
    |                                                    | in *young* crossover sequences are preserved.                 |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_young_crossover_and_stages*   | All finite rotations in all *young* crossover                 |
    |                                                    | sequences will be synchronised (such that *young* stage       |
    |                                                    | rotations are preserved). All finite rotations in             |
    |                                                    | *old* crossover sequences are preserved.                      |
    |                                                    | **Note:** This can result in non-zero finite rotations at     |
    |                                                    | present day if a younger sequence includes present day.       |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_young_crossover_only*         | Only the crossover finite rotation in all *young* crossover   |
    |                                                    | sequences will be synchronised (such that the younger         |
    |                                                    | *finite* rotations are preserved). All finite rotations       |
    |                                                    | in *old* crossover sequences are preserved.                   |
    +----------------------------------------------------+---------------------------------------------------------------+
    
    
    A rotation feature has a :class:`feature type<FeatureType>` of `total reconstruction sequence
    <http://www.earthbyte.org/Resources/GPGIM/public/#TotalReconstructionSequence>`_ and contains a time sequence
    of :class:`finite rotations<GpmlFiniteRotation>` (see :meth:`total reconstruction pole<Feature.get_total_reconstruction_pole>`)
    for a specific fixed/moving plate pair. Each crossover essentially returns information of the
    :meth:`total reconstruction pole<Feature.get_total_reconstruction_pole>` for the rotation feature after the crossover and
    for the rotation feature before the crossover as well as the time of the crossover - note that both rotation features will have the same
    moving plate id but differing fixed plate ids.
    
    Note that modifying the rotation sequences in the returned crossovers will modify the rotation features.
    This is what happens if the returned crossovers are passed into :func:`synchronise_crossovers`.
    The modified rotation features can then be used to create a new :class:`RotationModel` with updated rotations.
    
    If any rotation filenames are specified then this method uses
    :class:`FeatureCollectionFileFormatRegistry` internally to read the rotation files.
    
    Find all crossovers in a rotation file where crossover types are determined by '\@xo_' tags
    in the young crossover description/comment:
    ::
    
      crossovers = pygplates.find_crossovers('rotations.rot')
      for crossover in crossovers:
          print 'Crossover time: %f' % crossover.time
          print 'Crossover moving plate id: %d' % crossover.moving_plate_id
          print 'Crossover younger fixed plate id: %d' % crossover.young_crossover_fixed_plate_id
          print 'Crossover older fixed id: %d' % crossover.old_crossover_fixed_plate_id
          print 'Crossover younger finite rotation: %d' % \\
              crossover.young_crossover_rotation_sequence.get_value(crossover.time) \\
              .get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
          print 'Crossover older finite rotation: %d' % \\
              crossover.old_crossover_rotation_sequence.get_value(crossover.time) \\
              .get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
    
    Find crossovers affecting moving plate 801 assuming all crossovers are of type
    *CrossoverType.synch_old_crossover_and_stages*:
    ::
    
      crossovers_801 = pygplates.find_crossovers(
          rotation_feature_collection,
          lambda crossover: crossover.moving_plate_id==801,
          pygplates.CrossoverType.synch_old_crossover_and_stages)
    """
    
    # Use helper class to convert 'rotation_features' argument to a list of features.
    rotation_features = FeaturesFunctionArgument(rotation_features)
    
    # If crossover-type function is one of the crossover types themselves then
    # wrap in a function that returns that type...
    if CrossoverType._is_valid(crossover_type_function):
        # Convert crossover type to a function returning that crossover type.
        # Use partial to avoid lambda function referencing itself.
        crossover_type_function = partial(lambda *args: args[0], crossover_type_function)
    # ...else it's a function so leave it as it is.
    
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
            for young_index, young_crossover_total_reconstruction_pole in enumerate(total_reconstruction_poles):
                # fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
                young_crossover_fixed_plate_id = young_crossover_total_reconstruction_pole[0]
                young_crossover_rotation_sequence = young_crossover_total_reconstruction_pole[2]
                # Find the last enabled time sample using 'next()' builtin function.
                young_crossover_last_enabled_time_sample = next(
                    (ts for ts in reversed(young_crossover_rotation_sequence) if ts.is_enabled()),
                    None)
                # If no time samples are enabled for some reason then skip.
                if not young_crossover_last_enabled_time_sample:
                    continue
                
                for old_index, old_crossover_total_reconstruction_pole in enumerate(total_reconstruction_poles):
                    if young_index == old_index:
                        continue
                    # fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
                    old_crossover_fixed_plate_id = old_crossover_total_reconstruction_pole[0]
                    old_crossover_rotation_sequence = old_crossover_total_reconstruction_pole[2]
                    # Find the first enabled time sample using 'next()' builtin function.
                    old_crossover_first_enabled_time_sample = next(
                        (ts for ts in old_crossover_rotation_sequence if ts.is_enabled()),
                        None)
                    # If no time samples are enabled for some reason then skip.
                    if not old_crossover_first_enabled_time_sample:
                        continue
                    
                    # If the time of the last sample of young crossover sequence matches the time of
                    # the first sample of the old crossover sequence then we've found a crossover.
                    #
                    # We use the tolerance built into GeoTimeInstant for equality comparison.
                    # Avoids precision issues of directly comparing floating-point numbers for equality.
                    if (GeoTimeInstant(young_crossover_last_enabled_time_sample.get_time()) ==
                            old_crossover_first_enabled_time_sample.get_time()):
                        
                        crossover_time = old_crossover_first_enabled_time_sample.get_time()
                        # Determine the type of crossover.
                        crossover_type = crossover_type_function(
                            crossover_time,
                            moving_plate_id,
                            young_crossover_fixed_plate_id,
                            old_crossover_fixed_plate_id,
                            young_crossover_rotation_sequence,
                            old_crossover_rotation_sequence)
                        
                        # Create the actual Crossover class instance.
                        crossover = Crossover(
                            crossover_type,
                            crossover_time,
                            moving_plate_id,
                            young_crossover_fixed_plate_id,
                            old_crossover_fixed_plate_id,
                            young_crossover_rotation_sequence,
                            old_crossover_rotation_sequence)
                        # If caller is filtering crossovers then append crossover conditionally.
                        if not crossover_filter or crossover_filter(crossover):
                            crossovers.append(crossover)
    
    # Sort the list of crossovers by time.
    crossovers.sort(key = lambda crossover: crossover.time)
    
    return crossovers


def synchronise_crossovers(
        rotation_features,
        crossover_filter=None,
        crossover_threshold_degrees=None,
        crossover_type_function=CrossoverTypeFunction.type_from_xo_tags_in_comment,
        crossover_results=None):
    """synchronise_crossovers(rotation_features, [crossover_filter], [crossover_threshold_degrees], \
    [crossover_type_function=CrossoverTypeFunction.type_from_xo_tags_in_comment], [crossover_results])
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
    young_crossover_fixed_plate_id, old_crossover_fixed_plate_id, \
    young_crossover_rotation_sequence, old_crossover_rotation_sequence)
    :type crossover_filter: a callable accepting a single named-tuple 'Crossover' argument, or \
    a sequence of named-tuple 'Crossover' (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`)
    :param crossover_threshold_degrees: If specified then crossovers are synchronised only if the \
    old-crossover rotation latitude, longitude or angle differ from those in young-crossover rotation by \
    more than this amount
    :type crossover_threshold_degrees: float or None
    :param crossover_type_function: a function that determines a crossover's type, or one of the \
    *CrossoverType* enumerated values, or *CrossoverTypeFunction.type_from_xo_tags_in_comment* if using default \
    scheme for determining crossover type (see below) - default is *CrossoverTypeFunction.type_from_xo_tags_in_comment*
    :type crossover_type_function: a callable, or a *CrossoverType* enumerated value, or None
    :param crossover_results: if specified then a tuple of (Crossover, int) is appended for each filtered \
    crossover where the integer value is *CrossoverResult.synchronised* if the crossover was synchronised, or \
    *CrossoverResult.not_synchronised* if the crossover did not require synchronising, or \
    *CrossoverResult.error* if the crossover was unable to be processed (due to a crossover type of \
    *CrossoverType.unknown*) - the list is sorted by crossover time - default is None
    :type crossover_results: list or None
    :returns: True on success. False if the type of any crossover is *CrossoverType.unknown* (which \
    generates a *CrossoverResult.error* in *crossover_results*, if specified, due to inability to \
    process crossover). Also returns False if unable to synchronise all crossovers due to an infinite \
    cycle between crossovers. In all cases (success or failure) as many crossovers as possible will be \
    processed and results saved to rotation features (and rotation files, if any).
    :rtype: bool
    
    A crossover is a named-tuple 'Crossover' with named elements (time, moving_plate_id,
    young_crossover_fixed_plate_id, old_crossover_fixed_plate_id,
    young_crossover_rotation_sequence, old_crossover_rotation_sequence) and with type
    (float, int, int, int, :class:`GpmlIrregularSampling`, :class:`GpmlIrregularSampling`).
    Both the young and old crossover rotation sequences will each have at least one enabled time sample.
    
    A crossover occurs when the motion of a (moving) plate crosses over from one (fixed) plate to move relative
    to another (fixed) plate at a particular geological time. Synchronising a crossover involves adjusting the finite rotations
    before (older) the crossover to match the finite rotation after (younger) the crossover.
    Younger means more recent (smaller time values) and older means less recent (larger time values).
    
    Note that *rotation_features* should contain all features that directly or indirectly affect the crossovers to be
    synchronised (typically *rotation_features* is one or more rotation files), otherwise crossovers may not be correctly synchronised.
    
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
    synchronised (and you must ensure that both the young and old sequences in each crossover have at least one enabled time sample
    otherwise the crossover can't really be a crossover).
    
    *crossover_threshold_degrees* can optionally be used to synchronise (fix) only those crossovers whose
    difference in young and old crossover rotation latitudes, longitudes or angles exceeds this amount. This is
    useful since some PLATES rotation files are typically only accurate to 2 decimal places (or threshold of 0.01).
    
    The following arguments are supported by *crossover_type_function*:
    
    +----------------------------------------------------+---------------------------------------------------------------+
    | Type                                               | Description                                                   |
    +====================================================+===============================================================+
    | Arbitrary callable (function)                      | A callable accepting the following arguments:                 |
    |                                                    |                                                               |
    |                                                    | - crossover_time                                              |
    |                                                    | - moving_plate_id                                             |
    |                                                    | - young_crossover_fixed_plate_id                              |
    |                                                    | - old_crossover_fixed_plate_id                                |
    |                                                    | - young_crossover_rotation_sequence                           |
    |                                                    | - old_crossover_rotation_sequence                             |
    |                                                    |                                                               |
    |                                                    | ...and returning a *CrossoverType* enumerated value.          |
    +----------------------------------------------------+---------------------------------------------------------------+
    | CrossoverTypeFunction.type_from_xo_tags_in_comment | A callable (with same arguments as arbitrary callable) that   |
    |                                                    | uses the comment/description field of the *young* crossover   |
    |                                                    | pole to determine the crossover type according to the         |
    |                                                    | presence of the following strings/tags:                       |
    |                                                    |                                                               |
    |                                                    | - \@xo_ys : *CrossoverType.synch_old_crossover_and_stages*    |
    |                                                    | - \@xo_yf : *CrossoverType.synch_old_crossover_only*          |
    |                                                    | - \@xo_os : *CrossoverType.synch_young_crossover_and_stages*  |
    |                                                    | - \@xo_of : *CrossoverType.synch_young_crossover_only*        |
    |                                                    |                                                               |
    |                                                    | ...and if none of those tags are present then the crossover   |
    |                                                    | type is *CrossoverType.unknown*.                              |
    |                                                    |                                                               |
    |                                                    | This is the default for *crossover_type_function*.            |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.unknown*                            | All crossovers will be of unknown type                        |
    |                                                    | (no crossovers will be synchronised).                         |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_old_crossover_and_stages*     | All finite rotations in all *old* crossover                   |
    |                                                    | sequences will be synchronised (such that old *stage*         |
    |                                                    | rotations are preserved). All finite rotations in             |
    |                                                    | *young* crossover sequences are preserved.                    |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_old_crossover_only*           | Only the crossover finite rotation in all *old* crossover     |
    |                                                    | sequences will be synchronised (such that the older           |
    |                                                    | *finite* rotations are preserved). All finite rotations       |
    |                                                    | in *young* crossover sequences are preserved.                 |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_young_crossover_and_stages*   | All finite rotations in all *young* crossover                 |
    |                                                    | sequences will be synchronised (such that *young* stage       |
    |                                                    | rotations are preserved). All finite rotations in             |
    |                                                    | *old* crossover sequences are preserved.                      |
    |                                                    | **Note:** This can result in non-zero finite rotations at     |
    |                                                    | present day if a younger sequence includes present day.       |
    +----------------------------------------------------+---------------------------------------------------------------+
    | *CrossoverType.synch_young_crossover_only*         | Only the crossover finite rotation in all *young* crossover   |
    |                                                    | sequences will be synchronised (such that the younger         |
    |                                                    | *finite* rotations are preserved). All finite rotations       |
    |                                                    | in *old* crossover sequences are preserved.                   |
    +----------------------------------------------------+---------------------------------------------------------------+
    
    
    *crossover_results* can optionally be used to obtain a list of the synchronisation results of all
    filtered crossovers (see *crossover_filter*). Each list element is a tuple of (Crossover, int)
    where the integer value is *CrossoverResult.synchronised* if the crossover was synchronised, or
    *CrossoverResult.not_synchronised* if the crossover did not require synchronising, or
    *CrossoverResult.error* if the crossover was unable to be processed (due to a crossover type of
    *CrossoverType.unknown*). The list is sorted by crossover time.
    
    A rotation feature has a :class:`feature type<FeatureType>` of `total reconstruction sequence
    <http://www.earthbyte.org/Resources/GPGIM/public/#TotalReconstructionSequence>`_ and contains a time sequence
    of :class:`finite rotations<GpmlFiniteRotation>` (see :meth:`total reconstruction pole<Feature.get_total_reconstruction_pole>`)
    for a specific fixed/moving plate pair.
    
    Synchronise all crossovers found in a rotation file (and save modifications back to the same rotation file)
    where crossover types are determined by '\@xo_' tags in the young crossover description/comment:
    ::
    
      pygplates.synchronise_crossovers('rotations.rot')
    
    Synchronise crossovers between present day and 20Ma found in a PLATES rotation file that has rotation
    latitudes, longitudes and angles rounded to 2 decimal places (and save modifications back to the same
    rotation file) assuming all crossovers are of type *CrossoverType.synch_old_crossover_and_stages*:
    ::
      
      crossover_results = []
      pygplates.synchronise_crossovers(
          'rotations.rot',
          lambda crossover: crossover.time <= 20,
          0.01, # Equivalent to 2 decimal places
          pygplates.CrossoverType.synch_old_crossover_and_stages,
          crossover_results)
      print 'Fixed %d crossovers' % sum(
          1 for result in crossover_results if result[1]==pygplates.CrossoverResult.synchronised)
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
        crossovers = find_crossovers(rotation_feature_sequence, crossover_filter, crossover_type_function)
        
    # Create a list of crossover results if requested.
    if crossover_results is not None:
        # Initialise caller's list to all crossovers passed (were not synchronised).
        # We'll change this as needed during crossover iteration.
        crossover_results[:] = [(crossover, CrossoverResult.not_synchronised) for crossover in crossovers]
    
    # If any crossovers cannot be processed (has unknown crossover type) then this will get set to False.
    success = True
    
    # Perform up to a maximum number of iterations over all the crossovers until there are no remaining
    # crossovers to synchronise. If this is exceeded then it's likely there was an infinite cycle.
    iteration = 0
    while iteration < _MAX_CROSSOVER_ITERATIONS:
        synchronised_crossovers_in_current_iteration = False
        for crossover_index, crossover in enumerate(crossovers):
            # One of the younger or older will be the preserved crossover and the other synchronised.
            # This depends on the crossover type.
            #
            # Get the synchronised-crossover rotation time samples - ignore disabled samples.
            # Note: there must be at least one enabled time sample otherwise we wouldn't have found this crossover.
            if (crossover.type == CrossoverType.unknown):
                # Record that we were unable to process the crossover - this will be the same at each iteration.
                success = False
                if crossover_results is not None:
                    crossover_results[crossover_index] = (crossover, CrossoverResult.error)
                continue
            elif CrossoverType._synch_old_crossover(crossover.type):
                synchronised_crossover_time_samples = crossover.old_crossover_rotation_sequence.get_enabled_time_samples()
                synchronised_crossover_fixed_plate_id = crossover.old_crossover_fixed_plate_id
            else: # sync young crossover...
                synchronised_crossover_time_samples = crossover.young_crossover_rotation_sequence.get_enabled_time_samples()
                # Reverse the sequence so that the first time sample is the crossover and the rest have decreasing times.
                synchronised_crossover_time_samples = synchronised_crossover_time_samples[::-1]
                synchronised_crossover_fixed_plate_id = crossover.young_crossover_fixed_plate_id
            
            # Temporarily disable the synchronised-crossover time sample.
            # Note that this disables it directly in the rotation sequence/feature.
            # This is done so we can calculate the correct synchronised-crossover rotation (to match
            # the preserved-crossover rotation) without the previous synchronised-crossover rotation interfering.
            # In other words, we don't want to take the plate circuit path through the synchronised-crossover
            # fixed plate - instead we want to take the path through the preserved-crossover fixed plate.
            synchronised_crossover_time_samples[0].set_disabled()
            
            # Note that the rotation model will include the crossover modifications made so far because
            # the modifications were made to the properties of the rotation features that we're passing
            # into the rotation model here.
            # So it is important that this RotationModel is created *inside* the loop over crossovers.
            #
            # Also rotation modifications up to this point include the disabled synchronised-crossover time sample.
            # This ensures the rotation model does not take the synchronised-crossover rotation into account.
            rotation_model = RotationModel(rotation_feature_sequence)
            
            # The preserved-crossover is correct and we want to adjust the synchronised-crossover to match.
            # So we get the rotation relative to the synchronised-crossover fixed plate at the crossover time
            # (but with the synchronised-crossover time sample disabled above).
            current_synchronised_crossover_moving_fixed_relative_rotation = rotation_model.get_rotation(
                crossover.time,
                crossover.moving_plate_id,
                # Note: Setting anchor plate instead of fixed plate in case there's no plate circuit path
                # from plate zero (default anchor plate) to moving or fixed plates (see pygplates API docs)...
                anchor_plate_id=synchronised_crossover_fixed_plate_id)
            
            # Now that we've obtained the current synchronised-crossover rotation we can re-enable the
            # synchronised-crossover time sample (that we disabled above).
            #
            # NOTE: We could do this immediately after constructing a RotationModel (and hence before
            # calculating the current crossover rotation), but currently RotationModel can see changes
            # to the rotation features *after* it is constructed.
            # This will be fixed in pygplates soon by having RotationModel clone its input rotation
            # features such that it won't see subsequent modifications to those features.
            # For now we just place it after the rotation has been calculated.
            synchronised_crossover_time_samples[0].set_enabled()
            
            # The previous synchronised-crossover rotation.
            previous_synchronised_crossover_moving_fixed_relative_rotation = (
                    synchronised_crossover_time_samples[0].get_value().get_finite_rotation())
            
            # Determine the crossover adjustment to apply to all young or old crossover time samples.
            # Whether it's applied to the younger or older crossover is determined by the crossover type.
            #
            # The preserved-crossover total rotation at time t2 (assuming t2 is a crossover time):
            #   R(0->t2, Fp->M)
            # The previous synchronised-crossover total rotation at time t2:
            #   R(0->t2, Fn->M)
            # The current synchronised-crossover total rotation at time t2:
            #   R'(0->t2, Fn->M)
            #
            # The previous synchronised-crossover (Fn->M) total rotation at a younger or older time t is composed as:
            #   R(0->t) = R(t2->t) * R(0->t2)
            # The stage pole from t2->t:
            #   R(t2->t) = R(0->t) * inverse[R(0->t2)]
            # The current synchronised-crossover total rotation at a younger or older time t is composed as:
            #   R'(0->t) = R(t2->t) * R'(0->t2)
            # Which, using the stage pole above to replace 'R(t2->t)', is:
            #   R'(0->t) = R(0->t) * inverse[R(0->t2)] * R'(0->t2)
            #            = R(0->t) * crossover_adjustment
            # So each synchronised total pole gets post-multiplied by a constant crossover adjustment of:
            #   crossover_adjustment = inverse[R(0->t2)] * R'(0->t2)
            #
            # Note that this is the same regardless of whether the synchronised crossover is younger or older.
            #
            crossover_adjustment = (previous_synchronised_crossover_moving_fixed_relative_rotation.get_inverse() *
                    current_synchronised_crossover_moving_fixed_relative_rotation)
            
            crossover_passed = False
            
            # Always test for identity rotation first because an identity rotation has zero angle but
            # can have an arbitrary pole and doing a threshold test against an arbitrary pole will almost
            # always fail (so we do that test second if not an identity rotation).
            if crossover_adjustment.represents_identity_rotation():
                crossover_passed = True
            elif crossover_threshold_degrees is not None:
                previous_sync_crossover_lat, previous_sync_crossover_lon, previous_sync_crossover_angle = (
                        previous_synchronised_crossover_moving_fixed_relative_rotation.get_lat_lon_euler_pole_and_angle_degrees())
                
                current_sync_crossover_pole, current_sync_crossover_angle_radians = (
                        current_synchronised_crossover_moving_fixed_relative_rotation.get_euler_pole_and_angle())
                current_sync_crossover_lat, current_sync_crossover_lon = current_sync_crossover_pole.to_lat_lon()
                current_sync_crossover_angle = math.degrees(current_sync_crossover_angle_radians)
                
                # See if both rotations are close enough that we don't require crossover adjustments.
                if (abs(current_sync_crossover_lat - previous_sync_crossover_lat) <= crossover_threshold_degrees and
                    abs(current_sync_crossover_lon - previous_sync_crossover_lon) <= crossover_threshold_degrees and
                    abs(current_sync_crossover_angle - previous_sync_crossover_angle) <= crossover_threshold_degrees):
                    crossover_passed = True
                else:
                    # The antipodal rotation pole with a negated angle represents the exact same rotation.
                    # So we compare that for closeness also.
                    current_sync_crossover_pole_x, current_sync_crossover_pole_y, current_sync_crossover_pole_z = (
                            current_sync_crossover_pole.to_xyz())
                    current_sync_crossover_pole_antipodal = PointOnSphere((
                            -current_sync_crossover_pole_x, -current_sync_crossover_pole_y, -current_sync_crossover_pole_z))
                    current_sync_crossover_lat_antipodal, current_sync_crossover_lon_antipodal = (
                            current_sync_crossover_pole_antipodal.to_lat_lon())
                    current_sync_crossover_angle_antipodal = -current_sync_crossover_angle
                    if (abs(current_sync_crossover_lat_antipodal - previous_sync_crossover_lat) <= crossover_threshold_degrees and
                        abs(current_sync_crossover_lon_antipodal - previous_sync_crossover_lon) <= crossover_threshold_degrees and
                        abs(current_sync_crossover_angle_antipodal - previous_sync_crossover_angle) <= crossover_threshold_degrees):
                        crossover_passed = True
            
            # Skip synchronising crossover if no adjustment is needed.
            #
            # Note that we don't record the crossover result state (if any) as 'not_synchronised' because the same
            # crossover may have been synchronised on a previous iteration and we want to keep that result.
            if crossover_passed:
                continue
            
            # Record that the crossover was synchronised.
            synchronised_crossovers_in_current_iteration = True
            if crossover_results is not None:
                crossover_results[crossover_index] = (crossover, CrossoverResult.synchronised)
            
            # Change the first rotation time sample.
            # This is the first sample if old crossover sequence or last sample if young crossover sequence.
            synchronised_crossover_time_samples[0].get_value().set_finite_rotation(
                current_synchronised_crossover_moving_fixed_relative_rotation)
            
            # If the crossover type requires synchronising of the stage rotations then apply crossover adjustment.
            if CrossoverType._synch_stages(crossover.type):
                # Change the remaining rotation time samples such that their stage rotations are preserved.
                #
                # Note that if the stage poles of the young crossover sequence are being preserved then
                # we are advancing towards present day and we will likely end up with a non-zero present
                # day rotation (if sequence goes all the way back to present day).
                for crossover_time_sample in synchronised_crossover_time_samples[1:]:
                    # Get, adjust and set.
                    crossover_rotation = crossover_time_sample.get_value().get_finite_rotation()
                    crossover_rotation = crossover_rotation * crossover_adjustment
                    crossover_time_sample.get_value().set_finite_rotation(crossover_rotation)
        
        # If no crossovers required synchronisation in the current iteration then break out of the iteration loop.
        if not synchronised_crossovers_in_current_iteration:
            break
        
        iteration += 1
    
    # If we exceeded the maximum number of iterations then it's likely there was an infinite cycle
    # and so not all crossovers will be synchronised.
    if iteration == _MAX_CROSSOVER_ITERATIONS:
        success = False
    
    # If any rotation features came from files then write those feature collections back out to the same files.
    #
    # Only interested in those feature collections that came from files (have filenames).
    rotation_files = rotation_features.get_files()
    if rotation_files:
        file_format_registry = FeatureCollectionFileFormatRegistry()
        for feature_collection, filename in rotation_files:
            # This can raise OpenFileForWritingError if file is not writable.
            file_format_registry.write(feature_collection, filename)

    return success