.. _pygplates_foundations:

Foundations
===========

This document covers some plate tectonic foundations of pyGPlates.

.. contents::
   :local:
   :depth: 3


.. _pygplates_foundations_plate_reconstruction_hierarchy:

Plate reconstruction hierarchy
------------------------------

.. note:: `Next-generation plate-tectonic reconstructions using GPlates <http://www.gplates.org/publications.html>`_
   contains a good introduction to plate reconstruction hierarchies.

A plate-reconstruction hierarchy consists of a tree of total reconstruction poles at an instant in geological time.

Plate motions are described in terms of relative rotations between pairs of plates.
Every plate in the model moves relative to some other plate where, within each
of these plate pairs, one plate is considered the *moving* plate relative to the
other *fixed* plate. That *fixed* plate, in turn, moves relative to another plate
thus forming a tree-like structure known as the *reconstruction tree*.
Each of these *relative* rotations is an *edge* in the tree.

The following diagram shows a subset of the hierarchy of relative rotations used in GPlates:
::

                  000
                   |
                   |  finite rotation (001 rel. 000)
                   |
                  001
                   |
                   |  finite rotation (701 rel. 001)
                   |
                  701(AFR)
                  /|\
                 / | \  finite rotation (802 rel. 701)
                /  |  \
             201  702  802(ANT)
              /   / \    \
             /   /   \    \  finite rotation (801 rel. 802)
            /   /     \    \
         202  704     705  801(AUS)
         / \
        /   \
       /     \
     290     291

...where *000* is the anchored plate (the top of the reconstruction tree).
The edge *802 rel. 701* contains the rotation of *802* (the moving plate in the pair) relative to
*701* (the fixed plate in the pair).

An *equivalent* rotation is the rotation of a plate relative to the *anchored* plate.
So the equivalent rotation of plate *802* is the composition of relative rotations along the
plate circuit *edge* path from anchored plate *000* to plate *802*.

A *relative* rotation is the rotation of one plate relative to *another* plate
(as opposed to the *anchored* plate). Note that, like *equivalent* rotations,
the plate circuit *edge* path can consist of one or more edges.
For example, the rotation of plate *801* relative to plate *291* follows an *edge*
path that goes via plates *202*, *201*, *701* and *802*. However it should be noted
that if the edge between *001* and *701* did not exist then, even though a path
would still exist between *291* and *801*, the *relative* rotation (and *equivalent*
rotations of *291* and *801* for that matter) would be an :meth:`identity rotation
<pygplates.FiniteRotation.represents_identity_rotation>`. This is because the sub-tree
below *001* would not get built into the reconstruction tree and hence all plates
in the sub-tree would be missing. This can happen when the rotation sequence
for a moving/fixed plate pair (eg, *701*/*101*) does not span a large enough time
period. You can work around this situation by setting the anchor plate to the relative plate
(eg, *291* in the above example).

A *total* rotation is a rotation at a time in the past relative to *present day* (0Ma).
In other words *from* present day *to* a past time.

A *stage* rotation is a rotation at a time in the past relative to *another* time
in the past.


.. _pygplates_foundations_working_with_finite_rotations:

Working with finite rotations
-----------------------------

A finite rotation represents the motion of a plate (relative to another plate) on the surface of the
globe over a period of geological time.

In pyGPlates, finite rotations are represented by :class:`pygplates.FiniteRotation`.

In the following sections we will first cover some rotation maths and then derive the four
fundamental finite rotation categories:

* :ref:`pygplates_foundations_equivalent_total_rotation`
* :ref:`pygplates_foundations_relative_total_rotation`
* :ref:`pygplates_foundations_equivalent_stage_rotation`
* :ref:`pygplates_foundations_relative_stage_rotation`

In pyGPlates, these can be obtained from a :class:`pygplates.RotationModel`.


.. _pygplates_foundations_composing_finite_rotations:

Composing finite rotations
^^^^^^^^^^^^^^^^^^^^^^^^^^

In the following examples a composed rotation :math:`R2 \times R1` means the rotation :math:`R1`
is the first rotation to be applied followed by the rotation :math:`R2` such that a geometry is
rotated in the following way:

.. math::

   \text{geometry_final} &= R2 \times(R1 \times \text{geometry_initial}) \\
                         &= R2 \times R1 \times \text{geometry_initial}

...which is the equivalent of...

.. math::

   \text{geometry_intermediate} &= R1 \times \text{geometry_initial} \\
   \text{geometry_final} &= R2 \times \text{geometry_intermediate} \\
                         &= R2 \times (R1 \times \text{geometry_initial}) \\
                         &= R2 \times R1 \times \text{geometry_initial}

.. note:: Rotations are *not* commutative (:math:`R2 \times R1 \neq R1 \times R2`)

The composed rotation :math:`R2 \times R1` can be written in pyGPlates as either:
::

  R2 * R1

...or...
::

  pygplates.FiniteRotation.compose(R2, R1)

For example, the above geometry rotation can be written as either:
::

  geometry_final = R2 * R1 * geometry_initial

...or...
::

  geometry_final = pygplates.FiniteRotation.compose(R2, R1) * geometry_initial


.. _pygplates_foundations_plate_circuit_paths:

Plate circuit paths
^^^^^^^^^^^^^^^^^^^

The rotation from present day (0Ma) to the *geological time* :math:`t_{2}` (via time :math:`t_{1}`) is given by:

.. math::

   R(0 \rightarrow t_{2}) = R(t_{1} \rightarrow t_{2}) \times R(0 \rightarrow t_{1})

...or by post-multiplying both sides by :math:`R(t_{1} \rightarrow 0)`, and then swapping sides, this becomes...

.. math::

   R(0 \rightarrow t_{2}) \times R(t_{1} \rightarrow 0) &= R(t_{1} \rightarrow t_{2}) \times R(0 \rightarrow t_{1}) \times R(t_{1} \rightarrow 0) \\
   R(0 \rightarrow t_{2}) \times R(t_{1} \rightarrow 0) &= R(t_{1} \rightarrow t_{2}) \\
   R(t_{1} \rightarrow t_{2}) &= R(0 \rightarrow t_{2}) \times R(t_{1} \rightarrow 0)

The *plate circuit path* rotation from anchor plate :math:`P_{A}` to moving plate :math:`P_{M}` (via fixed plate :math:`P_{F}`) is given by:

.. math::

   R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})

...or by pre-multiplying both sides by :math:`R(P_{F} \rightarrow P_{A})` this becomes...

.. math::

   R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{M}) &= R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M}) \\
   R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{M}) &= R(P_{F} \rightarrow P_{M}) \\
   R(P_{F} \rightarrow P_{M}) &= R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{M})

Note that the rotations for relative times and for relative plates have the opposite order of each other !

In other words:

* For times :math:`0 \rightarrow t_{1} \rightarrow t_{2}` you apply the :math:`0 \rightarrow t_{1}` rotation first followed by the :math:`t_{1} \rightarrow t_{2}` rotation:
  
  .. math::

     R(0 \rightarrow t_{2})  = R(t_{1} \rightarrow t_{2}) \times R(0 \rightarrow t_{1})

* For plate circuit :math:`P_{A} \rightarrow P_{F} \rightarrow P_{M}` you apply the :math:`P_{F} \rightarrow P_{M}` rotation first followed by the :math:`P_{A} \rightarrow P_{F}` rotation:
  
  .. math::

     R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})

  .. note:: This is not :math:`P_{A} \rightarrow P_{F}` followed by :math:`P_{F} \rightarrow P_{M}` as you might expect (looking at the time example).

This is probably best explained by the difference between thinking in terms of the grand fixed
coordinate system and local coordinate system (see http://glprogramming.com/red/chapter03.html#name2).
Essentially, in the plate circuit :math:`P_{A} \rightarrow P_{F} \rightarrow P_{M}`, the :math:`P_{F} \rightarrow P_{M}` rotation can be thought of as a rotation
within the local coordinate system of :math:`P_{A} \rightarrow P_{F}`. In other words :math:`P_{F} \rightarrow P_{M}` is not a rotation that
occurs relative to the global spin axis but a rotation relative to the local coordinate system
of plate :math:`P_{F}` *after* it has been rotated relative to the anchor plate :math:`P_{A}`.

For the times :math:`0 \rightarrow t_{1} \rightarrow t_{2}` this local/relative coordinate system concept does not apply.

Note that a rotation must be relative to present day (0Ma) before it can be separated into a (plate circuit) chain of moving/fixed plate pairs.
Hence :math:`R(t_{1} \rightarrow t_{2},P_{A} \rightarrow P_{C}) \neq R(t_{1} \rightarrow t_{2},P_{A} \rightarrow P_{B}) \times R(t_{1} \rightarrow t_{2},P_{B} \rightarrow P_{C})`
demonstrates this mistake.

The following shows the correct way to separate :math:`P_{A} \rightarrow P_{C}` into the (plate circuit) chain of moving/fixed plate pairs :math:`P_{A} \rightarrow P_{B}` and :math:`P_{B} \rightarrow P_{C}`...

.. math::

   R(t_{1} \rightarrow t_{2},P_{A} \rightarrow P_{C}) \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{C}) \times R(t_{1} \rightarrow 0,P_{A} \rightarrow P_{C}) \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{C}) \times R(0 \rightarrow t_{1},P_{A} \rightarrow P_{C})^{-1} \\
   &   \text{// Now that all times are relative to 0Ma we can split } P_{A} \rightarrow P_{C} \text{ into } P_{A} \rightarrow P_{B} \rightarrow P_{C} \text{ ...} \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{B}) \times R(0 \rightarrow t_{2},P_{B} \rightarrow P_{C}) \times [R(0 \rightarrow t_{1},P_{A} \rightarrow P_{B}) \times R(0 \rightarrow t_{1},P_{B} \rightarrow P_{C})]^{-1} \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{B}) \times R(0 \rightarrow t_{2},P_{B} \rightarrow P_{C}) \times R(0 \rightarrow t_{1},P_{B} \rightarrow P_{C})^{-1} \times R(0 \rightarrow t_{1},P_{A} \rightarrow P_{B})^{-1}

...where :math:`P_{A} \rightarrow P_{B} \rightarrow P_{C}` means :math:`P_{B} \rightarrow P_{C}` is the rotation of :math:`P_{C}` relative to :math:`P_{B}` and :math:`P_{A} \rightarrow P_{B}` is
the rotation of :math:`P_{B}` relative to :math:`P_{A}`. The need for rotation :math:`P_{A} \rightarrow P_{C}` to be relative
to present day (0Ma) before it can be split into :math:`P_{A} \rightarrow P_{B}` and :math:`P_{B} \rightarrow P_{C}` is because
:math:`P_{A} \rightarrow P_{B}` and :math:`P_{B} \rightarrow P_{C}` are defined (in the rotation file) as total reconstruction
poles which are always relative to present day.

.. note:: | The inverse of rotation :math:`R` is denoted :math:`R^{-1}`.
          | Such that :math:`R \times R^{-1} = R^{-1} \times R = I` where :math:`I` is the :meth:`identify rotation<pygplates.FiniteRotation.represents_identity_rotation>`.


.. _pygplates_foundations_equivalent_total_rotation:

Equivalent total rotation
^^^^^^^^^^^^^^^^^^^^^^^^^

The equivalent total rotation of moving plate :math:`P_{M}` relative to anchor plate :math:`P_{A}`, and
from present day time :math:`0` to time :math:`t_{to}` is:

.. math::

   R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M})

In pyGPlates, the equivalent total rotation can be obtained :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  equivalent_total_rotation = rotation_model.get_rotation(to_time, moving_plate)


.. _pygplates_foundations_relative_total_rotation:

Relative total rotation
^^^^^^^^^^^^^^^^^^^^^^^

The relative total rotation of moving plate :math:`P_{M}` relative to fixed plate :math:`P_{F}`, and
from present day time :math:`0` to time :math:`t_{to}` is:

.. math::

   R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{A}) \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M})

...where :math:`P_{A}` is the anchor plate.

In pyGPlates, the relative total rotation can be obtained from :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  relative_total_rotation = rotation_model.get_rotation(to_time, moving_plate, fixed_plate_id=fixed_plate)


.. _pygplates_foundations_equivalent_stage_rotation:

Equivalent stage rotation
^^^^^^^^^^^^^^^^^^^^^^^^^

The equivalent stage rotation of moving plate :math:`P_{M}` relative to anchor plate :math:`P_{A}`, and
from time :math:`t_{from}` to time :math:`t_{to}` is:

.. math::

   R(t_{from} \rightarrow t_{to},P_{A} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(t_{from} \rightarrow 0,P_{A} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})^{-1}

In pyGPlates, the equivalent stage rotation can be obtained :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  equivalent_stage_rotation = rotation_model.get_rotation(to_time, moving_plate, from_time)


.. _pygplates_foundations_relative_stage_rotation:

Relative stage rotation
^^^^^^^^^^^^^^^^^^^^^^^

The relative stage rotation of moving plate :math:`P_{M}` relative to fixed plate :math:`P_{F}`, and
from time :math:`t_{from}` to time :math:`t_{to}` is:

.. math::

   R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(t_{from} \rightarrow 0,P_{F} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})^{-1} \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{A}) \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times [R(0 \rightarrow t_{from},P_{F} \rightarrow P_{A}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})]^{-1} \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times [R(0 \rightarrow t_{from},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})]^{-1} \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})^{-1} \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{F})

...where :math:`P_{A}` is the anchor plate.

In pyGPlates, the relative stage rotation can be obtained :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  relative_stage_rotation = rotation_model.get_rotation(to_time, moving_plate, from_time, fixed_plate)
