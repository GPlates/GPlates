.. _pygplates_modify_reconstruction_pole:

Modify a reconstruction pole
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example:

- reads a rotation file,
- modifies a total reconstruction pole so that a point reconstructs to a desired location at a
  reconstruction time, and
- writes the modified rotations back to the same rotation file.

This is similar to what the ``Modify Reconstruction Pole`` tool does
in `GPlates <http://www.gplates.org>`_.

.. seealso:: :ref:`pygplates_create_total_reconstruction_sequence_feature` and
   :ref:`pygplates_query_total_reconstruction_sequence_feature`

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It is read, modified and then written back to the same file, so keep a copy.
    It must contain a total reconstruction sequence whose moving plate is 801 (the plate of the point
    being adjusted) with enabled rotation samples spanning 60Ma, and rotations for that sequence's
    fixed plate at 60Ma. Crossover tags (``@xo_...``) in the pole comments are used if present.
    Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_modify_reconstruction_pole.py


Details
"""""""

The script starts with the filenames of one or more rotation files. The modified rotations are
written back to these files at the end.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: rotation-filenames

The rotation is adjusted at 60Ma.

The reconstruction time is a :class:`pygplates.GeoTimeInstant` rather than a plain number. Its
comparison operators (``==``, ``!=``, ``<``, ``<=``, ``>``, ``>=``) compare times within a
numerical tolerance, which is a good idea whenever floating-point numbers are compared. The script
compares the reconstruction time with the times of rotation samples further down.

Here the script would probably still work with ``reconstruction_time = 60``.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: reconstruction-time

There are two positions. The present day position is the point to reconstruct. The desired
reconstructed position is where that point should be at 60Ma.

Each is a :class:`pygplates.PointOnSphere`, created from a latitude and a longitude.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: positions

A point is reconstructed as part of a :class:`pygplates.Feature`. The feature holds what the
reconstruction needs: the plate ID (801) and the present day position.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: point-feature

The rotation files are loaded with the utility class :class:`pygplates.FeaturesFunctionArgument`.
It keeps the features of each file together with the filename, which makes it a little easier to
write the modified rotation features back to their files.

Alternatively, each rotation file could be loaded into its own :class:`pygplates.FeatureCollection`
and later :meth:`saved<pygplates.FeatureCollection.write>` back to its file.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: load-rotation-features

A :class:`rotation model<pygplates.RotationModel>` is created from the rotation features before
they are modified. It is used to reconstruct the point, and later to work out the adjustment to the
total reconstruction pole.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: rotation-model-before-adjustment

The point feature is :class:`reconstructed<pygplates.ReconstructSnapshot>` to 60Ma, to find where
the point *actually* is at that time.

The point feature has no :meth:`valid time<pygplates.Feature.set_valid_time>`, which means it
exists at all times. So there is exactly one :class:`pygplates.ReconstructedFeatureGeometry`, and
its :meth:`reconstructed geometry<pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry>`
is the reconstructed point.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: reconstruct-before-adjustment

If the actual reconstructed position differs from the desired reconstructed position then the
rotations of plate 801 need to be adjusted.

The rotation adjustment is the rotation from ``reconstructed_position`` to
``desired_reconstructed_position``. This :meth:`constructor<pygplates.FiniteRotation.__init__>` of
:class:`pygplates.FiniteRotation` creates the rotation that moves one point to another along the
great circle arc between them.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: rotation-adjustment

The script then iterates over the rotation features, looking for those whose moving plate ID is
the plate ID of the point feature. Changing the rotation of that plate moves the point, along with
everything else on that plate and on its
:ref:`child plates<pygplates_primer_plate_reconstruction_hierarchy>` (the plates that move
relative to it at the reconstruction time).

:meth:`pygplates.Feature.get_total_reconstruction_pole` returns the fixed plate ID, the moving
plate ID and the time-varying total reconstruction pole of a rotation feature. It returns ``None``
for a feature that does not have all three, which means it is not a rotation feature.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: find-rotation-features

The total reconstruction pole is a :class:`sequence of time samples<pygplates.GpmlIrregularSampling>`.
Each sample is the total rotation of the moving plate relative to the fixed plate at one time.

Some of the samples may be disabled.
:meth:`pygplates.GpmlIrregularSampling.get_enabled_time_samples` returns only the enabled ones.

The script continues with this rotation feature only if the reconstruction time is within the
time range of its enabled samples, which are ordered from youngest to oldest.

Because ``reconstruction_time`` is a :class:`pygplates.GeoTimeInstant`, these comparisons are made
within a numerical tolerance (as mentioned above). So the test passes when the reconstruction time
is the time of the first or the last sample.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: enabled-rotation-samples

:meth:`pygplates.GpmlIrregularSampling.get_value` returns the original rotation at the
reconstruction time. If the reconstruction time is between two rotation samples then it
:meth:`interpolates<pygplates.FiniteRotation.interpolate>` between them.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: original-rotation

The next step is to adjust the original rotation, so that the present day position reconstructs
to the desired reconstructed position.

A point on moving plate :math:`P_{M}` is reconstructed to time :math:`t` by the
:ref:`equivalent total rotation<pygplates_primer_equivalent_total_rotation>` of that plate. This
is its rotation relative to the anchor plate :math:`P_{A}`, from present day to time :math:`t`:

.. math::

   \text{reconstructed\_position} = R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \times \text{present\_day\_position}

Using the approach in :ref:`pygplates_primer_composing_finite_rotations`, the desired reconstructed
position can be written in terms of the actual reconstructed position:

.. math::

   \text{desired\_reconstructed\_position} &= R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position}) \times \text{reconstructed\_position} \\
                         &= R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \times \text{present\_day\_position}

Here :math:`R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position})`
is the rotation adjustment: the :class:`rotation<pygplates.FiniteRotation>` from
:math:`\text{reconstructed\_position}` to :math:`\text{desired\_reconstructed\_position}`. In
pyGPlates it is ``pygplates.FiniteRotation(reconstructed_position, desired_reconstructed_position)``.

The composed rotation, from the present day position to the desired reconstructed position, is the
adjusted *equivalent* rotation:

.. math::

   \text{desired\_reconstructed\_position} &= R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} \times \text{present\_day\_position} \\
   R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M})

But the pole of a rotation feature is not an equivalent rotation. It is a *relative* rotation, of
the moving plate relative to the fixed plate :math:`P_{F}` of the rotation feature.

So the adjusted equivalent rotation (relative to the anchor plate) is rewritten as an adjusted
relative rotation (relative to the fixed plate), using the result
:math:`R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})`
from :ref:`pygplates_primer_plate_circuit_paths`:

.. math::

   R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \\
   R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})

Pre-multiplying both sides by :math:`R(0 \rightarrow t,P_{A} \rightarrow P_{F})^{-1}` gives:

.. math::

   R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted} = R(0 \rightarrow t,P_{A} \rightarrow P_{F})^{-1} \times R(\text{reconstructed\_position} \rightarrow \text{desired\_reconstructed\_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})

This gives the *adjusted* relative rotation :math:`R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted}`
in terms of the *original* relative rotation :math:`R(0 \rightarrow t,P_{F} \rightarrow P_{M})`.

In pyGPlates this is written as follows, where ``fixed_plate_frame`` is
:math:`R(0 \rightarrow t,P_{A} \rightarrow P_{F})`, the equivalent total rotation of the fixed plate:

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: adjust-rotation

A rotation sample usually has a description, which is the comment at the end of its line in the
rotation file. If one of the enabled samples is at the reconstruction time then the script gets
its description, so that it is not lost when the adjusted rotation replaces that sample.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: rotation-description

:meth:`pygplates.GpmlIrregularSampling.set_value` sets the adjusted relative rotation in the
rotation feature, at the reconstruction time. If there is a sample at that time then it is
modified, otherwise a new sample is inserted. Either way the sample gets the description found
above (or none).

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: set-adjusted-rotation

The adjustment can put crossovers out of step. A crossover is a time at which a plate stops moving
relative to one fixed plate and starts moving relative to another, so two of its rotation
sequences meet there.

The adjustment changes the motion of plate 801 around 60Ma, and with it the motion of every plate
below plate 801 in the hierarchy (the plates that move relative to it, the plates that move
relative to those, and so on). Suppose a plate crosses over, at a time the adjustment affects,
between one of those plates and a plate elsewhere in the hierarchy. Its two rotation sequences no
longer agree at the crossover time, so the plate would jump in the reconstruction.

So the script calls :func:`pygplates.synchronise_crossovers`. It synchronises every crossover in
the rotation features that is out of step by more than *crossover_threshold_degrees*, and
modifies the rotation features in place.

The function needs to know how to synchronise each crossover: by changing the rotation sequence
that is younger than the crossover time, or the one that is older. Here that is decided by
``pygplates.CrossoverTypeFunction.type_from_xo_tags_in_comment_default_xo_ys``. It looks for an
``@xo_`` tag in the description of the crossover's younger pole (its comment in the rotation
file), and treats a crossover without a tag as if it had the ``@xo_ys`` tag. See
:func:`pygplates.synchronise_crossovers` for the tags and what they mean.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: synchronise-crossovers


The point feature is then reconstructed again, this time with a rotation model created from the
modified rotation features. Its reconstructed position should now be the desired reconstructed
position.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: reconstruct-after-adjustment

The last step is to write the modified rotation features back to the files they came from.
:meth:`pygplates.FeaturesFunctionArgument.get_files` returns each feature collection that was
loaded from a file, along with its filename.

.. sample-code:: pygplates_modify_reconstruction_pole.py
   :fragment: write-rotation-files

Output
""""""

::

    Reconstructed lat/lon position before adjustment (-45.962028, 131.398490)
    Desired reconstructed lat/lon position (-45.000000, 130.000000)
    Reconstructed lat/lon position after adjustment (-45.000000, 130.000000)

See also
""""""""

- Primer: :ref:`pygplates_primer_equivalent_total_rotation`, :ref:`pygplates_primer_composing_finite_rotations`,
  :ref:`pygplates_primer_plate_circuit_paths`
- Reference: :class:`pygplates.FeaturesFunctionArgument`, :meth:`pygplates.Feature.get_total_reconstruction_pole`,
  :meth:`pygplates.GpmlIrregularSampling.get_value`, :meth:`pygplates.GpmlIrregularSampling.set_value`,
  :func:`pygplates.synchronise_crossovers`, :class:`pygplates.ReconstructSnapshot`
- Sample code: :ref:`pygplates_create_total_reconstruction_sequence_feature`,
  :ref:`pygplates_query_total_reconstruction_sequence_feature`, :ref:`pygplates_plate_rotation_hierarchy`
