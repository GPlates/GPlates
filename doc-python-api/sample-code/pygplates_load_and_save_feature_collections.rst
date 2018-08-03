.. _pygplates_load_and_save_feature_collections:

Load and save feature collections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows how to load feature data that is already in a file format that pyGPlates
can load. If this is not the case then the feature data first needs to be
:ref:`imported<pygplates_import_geometries_and_assign_plate_ids>`.

.. seealso:: :ref:`pygplates_import_geometries_and_assign_plate_ids`

This example also shows how to save feature data.

.. contents::
   :local:
   :depth: 2

Create a file containing a subset of features from another file
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we make a new GPML file containing only the coastlines that have a plate ID of 801 (Australia).

Sample code
"""""""""""

::

    import pygplates


    # Load the global coastline features.
    input_feature_collection = pygplates.FeatureCollection('coastlines.gpml')

    # Start with an empty list of coastline features on plate 801.
    features_in_plate_801 = []

    # Iterate over all coastline features and add those on plate 801 to 'features_in_plate_801'.
    for feature in input_feature_collection:
        if feature.get_reconstruction_plate_id() == 801:
            features_in_plate_801.append(feature)

    # Write the coastline features for plate 801 to a new file.
    output_feature_collection = pygplates.FeatureCollection(features_in_plate_801)
    output_feature_collection.write('coastlines_801.gpml')

Details
"""""""

The general idea is to use :class:`pygplates.FeatureCollection` to both load and save
:class:`features<pygplates.Feature>` from/to files.

First we load a file containing GPlates coastline features into
a :class:`pygplates.FeatureCollection` from a file called ``'coastlines.gpml'``.

::

    features = pygplates.FeatureCollection('coastlines.gpml')

Alternatively we could have used the :meth:`pygplates.FeatureCollection.read` function as follows:
::

    features = pygplates.FeatureCollection.read('coastlines.gpml')

...or we could have used :class:`pygplates.FeatureCollectionFileFormatRegistry` as follows:

::

    registry = pygplates.FeatureCollectionFileFormatRegistry()
    features = registry.read('coastlines.gpml')

...however the last two examples are more complicated than necessary.

| Next we use a Python ``list`` to contain the subset of coastline features that are on plate 801.
| It's easier to use a Python ``list`` to contain the subset of features we will write to the new file.
| We could have used a :class:`pygplates.FeatureCollection` instead, but it doesn't really matter
  since we can easily create a :class:`pygplates.FeatureCollection` from the Python ``list`` when
  we need to save to a file.

::

    features_in_plate_801 = []

| Iterate over all the coastline features and only add those with plate ID 801 to the new list.
| Note that a :class:`pygplates.FeatureCollection` behaves like any sequence (eg, a Python ``list``)
  and so we can iterate over it like we would any sequence using the syntax ``for item in sequence:``.

::

    for feature in features:
        if feature.get_reconstruction_plate_id() == 801:
            features_in_plate_801.append(feature)

| As mentioned above, when we want to save features to a file we need to create a :class:`pygplates.FeatureCollection`
  (it accepts any Python sequence containing :class:`features<pygplates.Feature>`). In our case
  the Python sequence is our ``features_in_plate_801`` list.

::

    output_feature_collection = pygplates.FeatureCollection(features_in_plate_801)

| Now we can write the output feature collection to a new file.
| Here we're saving the coastline features for plate 801 to a file called ``'coastlines_801.gpml'``.

::

    output_feature_collection.write('coastlines_801.gpml')

Create a file containing features from multiple files
+++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we make a new GPML file containing ridges from one file and isochrons from another.

Sample code
"""""""""""

::

    import pygplates


    # The list of files to merge.
    filenames = ['ridges.gpml', 'isochrons.gpml']

    # The list of features from all input files.
    merged_features = []
    
    # Iterate over the input files and add their features to the merged list.
    for filename in filenames:
        features = pygplates.FeatureCollection(filename)
        merged_features.extend(features)
    
    # Write the merged features to a file.
    merged_feature_collection = pygplates.FeatureCollection(merged_features)
    merged_feature_collection.write('ridges_and_isochrons.gpml')

Details
"""""""

The general idea is to use :class:`pygplates.FeatureCollection` to both load and save
:class:`features<pygplates.Feature>` from/to files.


| Iterate over all the input files, read features from each file and add to a list of merged features.
| Note that a :class:`pygplates.FeatureCollection` behaves like any sequence (eg, a Python ``list``)
  and so we can pass it to the ``list.extend()`` method of our ``merged_features`` Python ``list`` and it
  will iterate over our :class:`pygplates.FeatureCollection` sequence to retrieve
  :class:`features<pygplates.Feature>` and extend the ``merged_features`` list.

::

    merged_features = []
    for filename in filenames:
        features = pygplates.FeatureCollection(filename)
        merged_features.extend(features)

Write the merged feature collection to a new file using :class:`pygplates.FeatureCollection`.
::

    merged_feature_collection = pygplates.FeatureCollection(merged_features)
    merged_feature_collection.write('ridges_and_isochrons.gpml')
