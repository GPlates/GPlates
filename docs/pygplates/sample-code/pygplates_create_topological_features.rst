.. _pygplates_create_topological_features:

Create topological features
^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example uses existing regular features to create three new topological features:

- a topological line feature,
- a topological polygon feature (which also uses the topological line feature), and
- a topological network feature (which also uses the topological line feature).

.. note:: Creating topological features is different from resolving them at a specific
   reconstruction time (which is done with a :class:`pygplates.TopologicalModel` or a
   :class:`pygplates.TopologicalSnapshot`). Topological features are typically built using GPlates,
   but they can also be created using pyGPlates, and this example shows how.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``features.gpml``
    The regular features that the new topologies reference. The script refers to them by feature ID,
    so it needs exactly the file reproduced in the :ref:`Input<pygplates_create_topological_features_input>`
    section below (save that listing as ``features.gpml``).

Sample code
"""""""""""

.. sample-code:: pygplates_create_topological_features.py

Details
"""""""

First the regular features are loaded from the GPML file ``features.gpml`` (see
:ref:`pygplates_create_topological_features_input`):

.. sample-code:: pygplates_create_topological_features.py
   :fragment: load-features

A topology references the features it is built from by their feature IDs. So next comes a list of
feature ID strings, identifying the regular features that the new topological *line* will reference
(see the ``gpml:identity`` tags in :ref:`pygplates_create_topological_features_input`):

.. sample-code:: pygplates_create_topological_features.py
   :fragment: line-referenced-feature-ids

The topological *line* feature is created by the script's own function
:ref:`create_topological_line_feature<pygplates_create_topological_line_feature>`. It is passed the
regular features, the list of feature IDs referenced by the topological line, and the feature type
of the topological line:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-line

The topological *line* feature is then added to the features, because the new topological polygon
and topological network will reference it in turn:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: add-topological-line

Next comes the list of feature ID strings that the new topological *polygon* will reference:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: polygon-referenced-feature-ids

.. note:: As well as regular features, the topological *polygon* references the topological *line*
   feature that was just created, since the line forms part of the boundary of the *polygon*.

The topological *polygon* feature is created by the script's own function
:ref:`create_topological_polygon_feature<pygplates_create_topological_polygon_feature>`. It is
passed the features (the regular features and the topological *line*), the list of feature IDs
referenced by the topological polygon, and the feature type of the topological polygon:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-polygon

Last come the feature ID strings that the new topological *network* will reference. A topological
network has a boundary, which is a list of boundary sections, and interior geometries, which are a
list of interior sections. So there are two lists of feature ID strings:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: network-referenced-feature-ids

.. note:: As well as regular features, the *boundary* of the topological network references the
   topological *line* feature that was created earlier.

The topological *network* feature is created by the script's own function
:ref:`create_topological_network_feature<pygplates_create_topological_network_feature>`. It is
passed the features (the regular features and the topological *line*), the list of feature IDs
referenced by the network *boundary*, and the list of feature IDs referenced by the network
*interior*:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-network

Finally the script saves the three topological features to ``topologies.gpml`` (see
:ref:`Output<pygplates_create_topological_features_output>`). That file does not contain the
regular features that the topologies reference. So, to resolve the topologies, load
``features.gpml`` along with it.

.. _pygplates_create_topological_line_feature:

Create a topological line feature
+++++++++++++++++++++++++++++++++

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-line-feature

First this function finds the regular features referenced by the topological line. It does so by
calling the script's own function :ref:`find_referenced_features<pygplates_find_referenced_features>`,
passing the regular features and the list of feature IDs referenced by the topological line:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: find-line-referenced-features

Then it wraps each referenced feature in a topological section. It does so by calling the script's
own function :ref:`create_topological_sections<pygplates_create_topological_sections>`, passing the
features referenced by the topological line. It also passes the type of topological geometry being
created, which is :class:`pygplates.GpmlTopologicalLine`:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-line-sections

Finally the topological sections (which join together to form a line) are used to create a
:class:`topological line geometry<pygplates.GpmlTopologicalLine>`. That geometry is passed, along
with the feature type, to :meth:`pygplates.Feature.create_topological_feature`, which creates the
topological line feature:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-line

.. _pygplates_create_topological_polygon_feature:

Create a topological polygon feature
++++++++++++++++++++++++++++++++++++

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-polygon-feature

First this function finds the regular features (and the topological *line*) referenced by the
topological polygon. It does so by calling the script's own function
:ref:`find_referenced_features<pygplates_find_referenced_features>`, passing the features and the
list of feature IDs referenced by the topological polygon:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: find-polygon-referenced-features

Then it wraps each referenced feature in a topological section. It does so by calling the script's
own function :ref:`create_topological_sections<pygplates_create_topological_sections>`, passing the
features referenced by the topological polygon. It also passes the type of topological geometry
being created, which is :class:`pygplates.GpmlTopologicalPolygon`:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-polygon-sections

Finally the topological sections (which join together to form a polygon) are used to create a
:class:`topological polygon geometry<pygplates.GpmlTopologicalPolygon>`. That geometry is passed,
along with the feature type, to :meth:`pygplates.Feature.create_topological_feature`, which creates
the topological polygon feature:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-polygon

.. _pygplates_create_topological_network_feature:

Create a topological network feature
++++++++++++++++++++++++++++++++++++

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-network-feature

First this function finds the regular features (and the topological *line*) referenced by the
*boundary* of the topological network. It does so by calling the script's own function
:ref:`find_referenced_features<pygplates_find_referenced_features>`, passing the features and the
list of feature IDs referenced by the network *boundary*.

It then wraps each referenced *boundary* feature in a topological section. It does so by calling
the script's own function :ref:`create_topological_sections<pygplates_create_topological_sections>`,
passing the features referenced by the network *boundary*. It also passes the type of topological
geometry being created, which is :class:`pygplates.GpmlTopologicalNetwork`:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-network-boundary-sections

Next it finds the regular features referenced by the *interior* of the topological network, again
by calling :ref:`find_referenced_features<pygplates_find_referenced_features>`, passing the features
and the list of feature IDs referenced by the network *interior*.

It then wraps each referenced *interior* feature in a topological network interior. It does so by
calling the script's own function
:ref:`create_topological_network_interiors<pygplates_create_topological_network_interiors>`, which
is similar to :ref:`create_topological_sections<pygplates_create_topological_sections>` but
specific to the network *interior*:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-network-interiors

Finally the topological *boundary* sections (which join together to form a polygon boundary) and
the topological *interiors* (which form interior constraints within the deforming region) are used
to create a :class:`topological network geometry<pygplates.GpmlTopologicalNetwork>`. That geometry
is passed to :meth:`pygplates.Feature.create_topological_network_feature`, which creates the
topological network feature:

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-network

.. _pygplates_find_referenced_features:

Find features referenced by a topology
++++++++++++++++++++++++++++++++++++++

.. sample-code:: pygplates_create_topological_features.py
   :fragment: find-referenced-features

This function iterates over the feature ID strings referenced by the topological geometry. For
each one it searches ``features`` for the feature with that
:meth:`feature ID<pygplates.Feature.get_feature_id>`.

The features that match are returned in the order of the feature ID strings, which is the order of
the sections in the topological geometry.


.. _pygplates_create_topological_sections:

Create a topological section for each referenced feature
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-sections

This function wraps each referenced feature in a
:class:`topological section<pygplates.GpmlTopologicalSection>`, by calling
:meth:`pygplates.GpmlTopologicalSection.create`.

The type of topological section depends on the geometry that it references:

- It is a :class:`pygplates.GpmlTopologicalPoint` if the referenced geometry is a point.
- It is a :class:`pygplates.GpmlTopologicalLineSection` if the referenced geometry is a polyline or
  a polygon. It is also one if the referenced geometry is a topological line, provided the section
  is being added to a topological polygon or network (as determined by
  ``topological_geometry_type``).

:meth:`pygplates.GpmlTopologicalSection.create` returns ``None`` if it cannot create a section from
a feature, and this function then leaves that feature out.

.. note:: No geometry property name is specified when calling
   :meth:`pygplates.GpmlTopologicalSection.create`. So the *default* geometry property name (for the
   referenced feature *type*) is used.

   A feature can have geometry in two different properties (with different property names). If the
   geometry to reference were in a non-default geometry property, then that property name would
   need to be specified explicitly.

   A feature can also have more than one geometry under the *same* property name.
   :meth:`pygplates.GpmlTopologicalSection.create` then returns ``None`` (and this function leaves
   the feature out), unless its *property_return* argument is set to
   ``pygplates.PropertyReturn.first`` to reference the first one found.

.. note:: The *reverse_order* argument of :meth:`pygplates.GpmlTopologicalSection.create` is not
   needed (it can be left as its default, as it is here) if the topological sections always
   intersect each other (or if they are points, which have no orientation). It is only used when a
   line section does not intersect both of its neighbouring sections. It then determines the
   orientation of the line section when the sections are joined together (known as
   *rubber banding*).


.. _pygplates_create_topological_network_interiors:

Create a topological network interior for each referenced feature
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. sample-code:: pygplates_create_topological_features.py
   :fragment: create-topological-network-interiors

This function wraps each referenced feature in a topological network interior, by calling
:meth:`pygplates.GpmlTopologicalSection.create_network_interior`.

A topological network interior is a :class:`pygplates.GpmlPropertyDelegate`. A property delegate
references a specific geometry property within a feature: it contains a feature ID and a property
name. It is also what a :class:`pygplates.GpmlTopologicalSection` stores inside it.

.. note:: Any regular geometry (point, multipoint, polyline or polygon), or a topological line, can
   be referenced by a topological network interior.

   If a regular polygon geometry is referenced, it is treated as a rigid interior block in the
   topological network, and is not part of the deforming region. Anything inside this interior
   polygon moves rigidly, using the plate ID of the referenced polygon feature (see
   :ref:`pygplates_primer_rigid_blocks`).

.. note:: No geometry property name is specified when calling
   :meth:`pygplates.GpmlTopologicalSection.create_network_interior`. So the *default* geometry
   property name (for the referenced feature *type*) is used.

   A feature can have geometry in two different properties (with different property names). If the
   geometry to reference were in a non-default geometry property, then that property name would
   need to be specified explicitly.

   A feature can also have more than one geometry under the *same* property name.
   :meth:`pygplates.GpmlTopologicalSection.create_network_interior` then returns ``None`` (and this
   function leaves the feature out), unless its *property_return* argument is set to
   ``pygplates.PropertyReturn.first`` to reference the first one found.


.. _pygplates_create_topological_features_input:

Input
"""""

The contents of the input file ``features.gpml``:
::

    <?xml version="1.0" encoding="UTF-8"?>
    <gpml:FeatureCollection xmlns:gpml="http://www.gplates.org/gplates" xmlns:gml="http://www.opengis.net/gml" xmlns:xsi="http://www.w3.org/XMLSchema-instance" gpml:version="1.6.0339" xsi:schemaLocation="http://www.gplates.org/gplates ../xsd/gpml.xsd http://www.opengis.net/gml ../../../gml/current/base">
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-63b81b91-b7a0-4ad7-908d-16db3c70e6ed</gpml:identity>
                <gpml:revision>GPlates-70312c4a-66d8-4f3d-8daf-1a5818ff7cc7</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">30.015933113679623 -89.653669889720803 0.1281258847639748 -89.468031335503937 -29.759681344151701 -89.282392781287072 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section1</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-aa1d0d5a-0445-4380-a516-d2bc66e477a7</gpml:identity>
                <gpml:revision>GPlates-425eb1d0-7376-4afd-9258-aaca689e1835</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">23.147306607655537 -96.151019287311172 38.740945161872403 -55.310537359600318 43.010631908860347 -19.853573504178655 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section2</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-0bfc4f2b-c672-47e1-a29e-7214bea0521e</gpml:identity>
                <gpml:revision>GPlates-78edb882-13a5-48e0-b058-eb7ad5db3e31</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">46.723402993197702 -33.033910853576231 38.926583716089262 -21.895597600564173 28.345186125727821 -6.673236154781053 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section3</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-48bd0e0f-e7c8-4dea-9e0a-4bc0e1403db6</gpml:identity>
                <gpml:revision>GPlates-8fdaa677-534a-4af4-a8ce-c5888c4cbb15</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">37.070198173920588 -10.200368684901546 13.86537889681216 -9.6434530222509185 1.0563186558483011 -8.7152602511665904 -15.651151223669773 -9.0865373596003245 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section4</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-71470e03-9e99-4205-80d9-727d7a3700de</gpml:identity>
                <gpml:revision>GPlates-46104565-0d30-4ffb-a833-93183c194738</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:subductionPolarity>
                    <gpml:ConstantValue>
                        <gpml:value>Right</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:SubductionPolarityEnumeration</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:subductionPolarity>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">-13.303320984942289 -0.29269216109329937 -13.117682430725424 -48.7443548116957 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section5</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-cc5b9027-d227-4e97-bb06-df26786fd1ec</gpml:identity>
                <gpml:revision>GPlates-3fb9f8bc-949e-46ea-ab68-89103727600c</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">-25.926742671689269 -95.525270474346286 -25.555465563255538 -77.703969269527022 -25.926742671689269 -67.493848787599291 -25.926742671689269 -55.056065655069176 -25.926742671689269 -42.989559630972792 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section6</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2</gpml:identity>
                <gpml:revision>GPlates-2c15eff6-7a6c-47c3-9c57-84a943f01aaf</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">17.883956123491444 -2.7059933659125783 18.255233231925192 -21.084210233382436 18.812148894575785 -32.408162040611359 19.36906455722638 -50.415101799647488 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section7</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-e184b54d-abb0-465b-8820-c73c543d2562</gpml:identity>
                <gpml:revision>GPlates-6222677c-8618-4881-a960-0e8276826fa3</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:subductionPolarity>
                    <gpml:ConstantValue>
                        <gpml:value>Left</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:SubductionPolarityEnumeration</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:subductionPolarity>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:OrientableCurve gml:orientation="+">
                                <gml:baseCurve>
                                    <gml:LineString>
                                        <gml:posList gml:dimension="2">43.8733537138529 -56.912451197237857 16.027570581322777 -33.893270474346288 </gml:posList>
                                    </gml:LineString>
                                </gml:baseCurve>
                            </gml:OrientableCurve>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:OrientableCurve</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section8</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-56f3c23d-1ee5-47a9-a46e-006d2aa463c3</gpml:identity>
                <gpml:revision>GPlates-40f8ed20-3ccc-4ba1-a30e-d022265d966e</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:Point>
                                <gml:pos>22.276785170409813 -47.328672085727149</gml:pos>
                            </gml:Point>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section9</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-0ba4c93d-474e-4d9b-8f1b-618cb21024de</gpml:identity>
                <gpml:revision>GPlates-0ad2bf4f-a77a-495c-9935-d1b9ec1cdc2a</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:Point>
                                <gml:pos>8.787666966864947 -47.25924637796075</gml:pos>
                            </gml:Point>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section10</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-84be6d41-6c32-4184-9c44-c38e399090a0</gpml:identity>
                <gpml:revision>GPlates-5707975c-a5f1-48bd-bd58-ae117cf4ed7d</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:Point>
                                <gml:pos>1.9190404608408473 -46.331053606876431</gml:pos>
                            </gml:Point>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section11</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-3df7a9df-aefc-403e-a16c-faf203776fd1</gpml:identity>
                <gpml:revision>GPlates-1ffa6087-3617-4e6a-bd25-b50335536dd3</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:Point>
                                <gml:pos>-6.2490559247013238 -45.588499390008934</gml:pos>
                            </gml:Point>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section12</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-56f22e61-ddd5-4c2f-ae41-54e5f66f47ec</gpml:identity>
                <gpml:revision>GPlates-058aabb9-b65d-42c2-839e-ba24b1522892</gpml:revision>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:Point>
                                <gml:pos>-31.867176406629042 -44.845945173141473</gml:pos>
                            </gml:Point>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name>section13</gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-56ffca31-df55-4a3e-b943-06faa1407fed</gpml:identity>
                <gpml:revision>GPlates-b5db7213-2b3d-4a86-a174-95c2b80a201c</gpml:revision>
                <gpml:geometryImportTime>
                    <gml:TimeInstant>
                        <gml:timePosition gml:frame="http://gplates.org/TRS/flat">0</gml:timePosition>
                    </gml:TimeInstant>
                </gpml:geometryImportTime>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:Polygon>
                                <gml:exterior>
                                    <gml:LinearRing>
                                        <gml:posList gml:dimension="2">18.100666221662301 -75.520561910076495 24.246112075556994 -63.567489221900708 3.281896991735711 -62.286190250042978 2.1276072929563159 -76.53473534276786 18.100666221662301 -75.520561910076495 </gml:posList>
                                    </gml:LinearRing>
                                </gml:exterior>
                            </gml:Polygon>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Polygon</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name></gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-a913e755-deaf-4bc5-918a-a124611341c1</gpml:identity>
                <gpml:revision>GPlates-b8219a47-fffd-4168-b7da-055f9b1b3e35</gpml:revision>
                <gpml:geometryImportTime>
                    <gml:TimeInstant>
                        <gml:timePosition gml:frame="http://gplates.org/TRS/flat">0</gml:timePosition>
                    </gml:TimeInstant>
                </gpml:geometryImportTime>
                <gpml:reconstructionPlateId>
                    <gpml:ConstantValue>
                        <gpml:value>0</gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:plateId</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:reconstructionPlateId>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gml:MultiPoint>
                                <gml:pointMember>
                                    <gml:Point>
                                        <gml:pos>-7.1465444497372816 -76.638108802987674</gml:pos>
                                    </gml:Point>
                                </gml:pointMember>
                                <gml:pointMember>
                                    <gml:Point>
                                        <gml:pos>-6.8407045187705684 -62.253091750364177</gml:pos>
                                    </gml:Point>
                                </gml:pointMember>
                                <gml:pointMember>
                                    <gml:Point>
                                        <gml:pos>-17.077692628611821 -62.601951440567611</gml:pos>
                                    </gml:Point>
                                </gml:pointMember>
                                <gml:pointMember>
                                    <gml:Point>
                                        <gml:pos>-16.232765704854504 -77.830176342213377</gml:pos>
                                    </gml:Point>
                                </gml:pointMember>
                            </gml:MultiPoint>
                        </gpml:value>
                        <gml:description></gml:description>
                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:MultiPoint</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
                <gml:name></gml:name>
                <gml:validTime>
                    <gml:TimePeriod>
                        <gml:begin>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantPast</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:begin>
                        <gml:end>
                            <gml:TimeInstant>
                                <gml:timePosition gml:frame="http://gplates.org/TRS/flat">http://gplates.org/times/distantFuture</gml:timePosition>
                            </gml:TimeInstant>
                        </gml:end>
                    </gml:TimePeriod>
                </gml:validTime>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
    </gpml:FeatureCollection>


.. _pygplates_create_topological_features_output:

Output
""""""

The contents of the output file ``topologies.gpml``. The feature IDs and revision IDs of the
topological features will differ for each run:
::

    <?xml version="1.0" encoding="UTF-8"?>
    <gpml:FeatureCollection xmlns:gpml="http://www.gplates.org/gplates" xmlns:gml="http://www.opengis.net/gml" xmlns:xsi="http://www.w3.org/XMLSchema-instance" gpml:version="1.6.0339" xsi:schemaLocation="http://www.gplates.org/gplates ../xsd/gpml.xsd http://www.opengis.net/gml ../../../gml/current/base">
        <gml:featureMember>
            <gpml:UnclassifiedFeature>
                <gpml:identity>GPlates-242ae377-f6fd-4ecd-a1fe-1760e2fe13a9</gpml:identity>
                <gpml:revision>GPlates-095a8012-72d5-48d3-bd37-d33672319f8a</gpml:revision>
                <gpml:unclassifiedGeometry>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gpml:TopologicalLine>
                                <gpml:section>
                                    <gpml:TopologicalPoint>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-56f3c23d-1ee5-47a9-a46e-006d2aa463c3</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalPoint>
                                </gpml:section>
                                <gpml:section>
                                    <gpml:TopologicalPoint>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-0ba4c93d-474e-4d9b-8f1b-618cb21024de</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalPoint>
                                </gpml:section>
                                <gpml:section>
                                    <gpml:TopologicalPoint>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-84be6d41-6c32-4184-9c44-c38e399090a0</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalPoint>
                                </gpml:section>
                                <gpml:section>
                                    <gpml:TopologicalPoint>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-3df7a9df-aefc-403e-a16c-faf203776fd1</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalPoint>
                                </gpml:section>
                                <gpml:section>
                                    <gpml:TopologicalPoint>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-56f22e61-ddd5-4c2f-ae41-54e5f66f47ec</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:Point</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalPoint>
                                </gpml:section>
                            </gpml:TopologicalLine>
                        </gpml:value>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:TopologicalLine</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:unclassifiedGeometry>
            </gpml:UnclassifiedFeature>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:TopologicalClosedPlateBoundary>
                <gpml:identity>GPlates-e13e3a00-8841-4642-9ef2-885de70fe3c7</gpml:identity>
                <gpml:revision>GPlates-057f735b-4f63-445b-8f5d-ebcb9a9f8b50</gpml:revision>
                <gpml:boundary>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gpml:TopologicalPolygon>
                                <gpml:exterior>
                                    <gpml:TopologicalSections>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-48bd0e0f-e7c8-4dea-9e0a-4bc0e1403db6</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-71470e03-9e99-4205-80d9-727d7a3700de</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-242ae377-f6fd-4ecd-a1fe-1760e2fe13a9</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:TopologicalLine</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                    </gpml:TopologicalSections>
                                </gpml:exterior>
                            </gpml:TopologicalPolygon>
                        </gpml:value>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:TopologicalPolygon</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:boundary>
            </gpml:TopologicalClosedPlateBoundary>
        </gml:featureMember>
        <gml:featureMember>
            <gpml:TopologicalNetwork>
                <gpml:identity>GPlates-425b5dbe-0c59-4b84-89bf-524bb2cae155</gpml:identity>
                <gpml:revision>GPlates-a3bfc37e-a018-46ef-8b44-e3acb56eedb3</gpml:revision>
                <gpml:network>
                    <gpml:ConstantValue>
                        <gpml:value>
                            <gpml:TopologicalNetwork>
                                <gpml:boundary>
                                    <gpml:TopologicalSections>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-63b81b91-b7a0-4ad7-908d-16db3c70e6ed</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-aa1d0d5a-0445-4380-a516-d2bc66e477a7</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-e184b54d-abb0-465b-8820-c73c543d2562</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-242ae377-f6fd-4ecd-a1fe-1760e2fe13a9</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:TopologicalLine</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                        <gpml:section>
                                            <gpml:TopologicalLineSection>
                                                <gpml:sourceGeometry>
                                                    <gpml:PropertyDelegate>
                                                        <gpml:targetFeature>GPlates-cc5b9027-d227-4e97-bb06-df26786fd1ec</gpml:targetFeature>
                                                        <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                        <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LineString</gpml:valueType>
                                                    </gpml:PropertyDelegate>
                                                </gpml:sourceGeometry>
                                                <gpml:reverseOrder>false</gpml:reverseOrder>
                                            </gpml:TopologicalLineSection>
                                        </gpml:section>
                                    </gpml:TopologicalSections>
                                </gpml:boundary>
                                <gpml:interior>
                                    <gpml:TopologicalNetworkInterior>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-56ffca31-df55-4a3e-b943-06faa1407fed</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:LinearRing</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalNetworkInterior>
                                </gpml:interior>
                                <gpml:interior>
                                    <gpml:TopologicalNetworkInterior>
                                        <gpml:sourceGeometry>
                                            <gpml:PropertyDelegate>
                                                <gpml:targetFeature>GPlates-a913e755-deaf-4bc5-918a-a124611341c1</gpml:targetFeature>
                                                <gpml:targetProperty xmlns:gpml="http://www.gplates.org/gplates">gpml:unclassifiedGeometry</gpml:targetProperty>
                                                <gpml:valueType xmlns:gml="http://www.opengis.net/gml">gml:MultiPoint</gpml:valueType>
                                            </gpml:PropertyDelegate>
                                        </gpml:sourceGeometry>
                                    </gpml:TopologicalNetworkInterior>
                                </gpml:interior>
                            </gpml:TopologicalNetwork>
                        </gpml:value>
                        <gpml:valueType xmlns:gpml="http://www.gplates.org/gplates">gpml:TopologicalNetwork</gpml:valueType>
                    </gpml:ConstantValue>
                </gpml:network>
            </gpml:TopologicalNetwork>
        </gml:featureMember>
    </gpml:FeatureCollection>

See also
""""""""

- Primer: :ref:`pygplates_primer_topologies`
- Reference: :meth:`pygplates.Feature.create_topological_feature`, :meth:`pygplates.Feature.create_topological_network_feature`,
  :meth:`pygplates.GpmlTopologicalSection.create`, :meth:`pygplates.GpmlTopologicalSection.create_network_interior`,
  :class:`pygplates.GpmlTopologicalLine`, :class:`pygplates.GpmlTopologicalPolygon`, :class:`pygplates.GpmlTopologicalNetwork`
- Sample code: :ref:`pygplates_create_common_feature_types`, :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`
