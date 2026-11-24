.. _pygplates_load_and_save_feature_collections:

Load and save feature collections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example loads features from files, and saves features to files.

The feature data must already be in a file format that pyGPlates can load
(:class:`pygplates.FeatureCollection` lists them). If it is not, it first needs to be
:ref:`imported<pygplates_import_geometries_and_assign_plate_ids>`.

.. seealso:: :ref:`pygplates_import_geometries_and_assign_plate_ids`

.. contents::
   :local:
   :depth: 2

Create a file containing a subset of features from another file
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example creates a new GPML file containing only the coastlines that have a plate ID of 801
(Australia).

Data files
""""""""""

``coastlines.gpml``
    Coastline features, each with a reconstruction plate ID. Those on plate 801 are written to the new file.
    The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_load_and_save_feature_collections_subset.py

Details
"""""""

A :class:`pygplates.FeatureCollection` is used both to load :class:`features<pygplates.Feature>`
from a file and to save them to a file.

Creating a :class:`pygplates.FeatureCollection` from a filename loads the features in that file.
Here the file is ``coastlines.gpml``, which contains coastline features.

.. sample-code:: pygplates_load_and_save_feature_collections_subset.py
   :fragment: load-coastlines

Alternatively the :meth:`pygplates.FeatureCollection.read` function could have been used:
::

    features = pygplates.FeatureCollection.read('coastlines.gpml')

For a single file that is more complicated than necessary. It is useful for reading several files
at once: given a list of filenames, it returns a list of feature collections.

The coastline features that are on plate 801 are collected in a Python ``list``. A
:class:`pygplates.FeatureCollection` could have been used instead, but a list is easier, and it
makes no difference: a feature collection is easily created from the list when it is time to save
to a file.

.. sample-code:: pygplates_load_and_save_feature_collections_subset.py
   :fragment: empty-list

The script iterates over all the coastline features, and adds only those with a plate ID of 801 to
the list. A :class:`pygplates.FeatureCollection` can be iterated over like any Python sequence
(such as a ``list``), using ``for item in sequence:``.

.. sample-code:: pygplates_load_and_save_feature_collections_subset.py
   :fragment: select-features

Features are saved to a file through a :class:`pygplates.FeatureCollection`, so one is created from
the ``features_in_plate_801`` list. It accepts any Python sequence of
:class:`features<pygplates.Feature>`.

.. sample-code:: pygplates_load_and_save_feature_collections_subset.py
   :fragment: output-feature-collection

:meth:`pygplates.FeatureCollection.write` then writes the feature collection to a new file. Here
the coastline features of plate 801 are saved to a file called ``coastlines_801.gpml``. The
filename extension selects the file format.

.. sample-code:: pygplates_load_and_save_feature_collections_subset.py
   :fragment: write-output

Create a file containing features from multiple files
+++++++++++++++++++++++++++++++++++++++++++++++++++++

This example creates a new GPML file containing the ridges from one file and the isochrons from
another.

Data files
""""""""""

``ridges.gpml``, ``isochrons.gpml``
    Any two files of features. Nothing in them is queried; their features are simply merged into one file.
    The GPlates `sample data <https://www.gplates.org/download/>`_ has files of mid-ocean ridges and isochrons.

Sample code
"""""""""""

.. sample-code:: pygplates_load_and_save_feature_collections_merge.py

Details
"""""""

A :class:`pygplates.FeatureCollection` is used both to load :class:`features<pygplates.Feature>`
from a file and to save them to a file.

The script iterates over the input files. It loads the features of each file, and adds them to a
single list, ``merged_features``.

The features are added with the ``list.extend()`` method, which accepts anything that can be
iterated over. It iterates over the :class:`pygplates.FeatureCollection`, and appends each
:class:`feature<pygplates.Feature>` to the list.

.. sample-code:: pygplates_load_and_save_feature_collections_merge.py
   :fragment: merge-features

A :class:`pygplates.FeatureCollection` is created from the merged features, and writes them to a
new file.

.. sample-code:: pygplates_load_and_save_feature_collections_merge.py
   :fragment: write-merged

See also
++++++++

- Reference: :class:`pygplates.FeatureCollection`, :meth:`pygplates.FeatureCollection.read`,
  :meth:`pygplates.FeatureCollection.write`, :meth:`pygplates.Feature.get_reconstruction_plate_id`
- Sample code: :ref:`pygplates_import_geometries_and_assign_plate_ids`, :ref:`pygplates_query_common_feature_types`,
  :ref:`pygplates_create_common_feature_types`
