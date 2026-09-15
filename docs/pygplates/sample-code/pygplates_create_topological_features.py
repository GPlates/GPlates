import pygplates


# [fragment: find-referenced-features]
def find_referenced_features(
        features,
        referenced_feature_id_strings):

    referenced_feature_features = []

    for referenced_feature_id_string in referenced_feature_id_strings:
        for feature in features:
            if feature.get_feature_id().get_string() == referenced_feature_id_string:
                referenced_feature_features.append(feature)
                break

    return referenced_feature_features
# [end: find-referenced-features]


# [fragment: create-topological-sections]
def create_topological_sections(
        referenced_features,
        topological_geometry_type):

    topological_sections = []

    for referenced_feature in referenced_features:
        topological_section = pygplates.GpmlTopologicalSection.create(
            referenced_feature,
            topological_geometry_type=topological_geometry_type)
        if topological_section:
            topological_sections.append(topological_section)

    return topological_sections
# [end: create-topological-sections]


# [fragment: create-topological-network-interiors]
def create_topological_network_interiors(
        referenced_features):

    topological_network_interiors = []

    for referenced_feature in referenced_features:
        topological_network_interior = pygplates.GpmlTopologicalSection.create_network_interior(
            referenced_feature)
        if topological_network_interior:
            topological_network_interiors.append(topological_network_interior)

    return topological_network_interiors
# [end: create-topological-network-interiors]


# [fragment: create-topological-line-feature]
def create_topological_line_feature(
        features,
        topological_line_referenced_feature_id_strings,
        topological_line_feature_type):

    # [fragment: find-line-referenced-features]
    topological_line_referenced_features = find_referenced_features(
        features,
        topological_line_referenced_feature_id_strings)
    # [end: find-line-referenced-features]
    # [fragment: create-line-sections]
    topological_line_sections = create_topological_sections(
        topological_line_referenced_features,
        pygplates.GpmlTopologicalLine)
    # [end: create-line-sections]
    # [fragment: create-line]
    topological_line_feature = pygplates.Feature.create_topological_feature(
        topological_line_feature_type,
        pygplates.GpmlTopologicalLine(topological_line_sections))
    # [end: create-line]

    return topological_line_feature
# [end: create-topological-line-feature]


# [fragment: create-topological-polygon-feature]
def create_topological_polygon_feature(
        features,
        topological_polygon_referenced_feature_id_strings,
        topological_polygon_feature_type):

    # [fragment: find-polygon-referenced-features]
    topological_polygon_referenced_features = find_referenced_features(
        features,
        topological_polygon_referenced_feature_id_strings)
    # [end: find-polygon-referenced-features]
    # [fragment: create-polygon-sections]
    topological_polygon_sections = create_topological_sections(
        topological_polygon_referenced_features,
        pygplates.GpmlTopologicalPolygon)
    # [end: create-polygon-sections]
    # [fragment: create-polygon]
    topological_polygon_feature = pygplates.Feature.create_topological_feature(
        topological_polygon_feature_type,
        pygplates.GpmlTopologicalPolygon(topological_polygon_sections))
    # [end: create-polygon]

    return topological_polygon_feature
# [end: create-topological-polygon-feature]


# [fragment: create-topological-network-feature]
def create_topological_network_feature(
        features,
        topological_network_boundary_referenced_feature_id_strings,
        topological_network_interior_referenced_feature_id_strings):

    # [fragment: create-network-boundary-sections]
    topological_network_boundary_referenced_features = find_referenced_features(
        features,
        topological_network_boundary_referenced_feature_id_strings)
    topological_network_boundary_sections = create_topological_sections(
        topological_network_boundary_referenced_features,
        pygplates.GpmlTopologicalNetwork)
    # [end: create-network-boundary-sections]

    # [fragment: create-network-interiors]
    topological_network_interior_referenced_features = find_referenced_features(
        features,
        topological_network_interior_referenced_feature_id_strings)
    topological_network_interiors = create_topological_network_interiors(
        topological_network_interior_referenced_features)
    # [end: create-network-interiors]

    # [fragment: create-network]
    topological_network_feature = pygplates.Feature.create_topological_network_feature(
        pygplates.GpmlTopologicalNetwork(
            topological_network_boundary_sections,
            topological_network_interiors))
    # [end: create-network]

    return topological_network_feature
# [end: create-topological-network-feature]


# The topological features we'll create.
topological_features = []

# [fragment: load-features]
# Load the features that our topologies can reference.
features = pygplates.FeatureCollection('features.gpml')
# [end: load-features]

# [fragment: line-referenced-feature-ids]
topological_line_referenced_feature_id_strings = [
    'GPlates-56f3c23d-1ee5-47a9-a46e-006d2aa463c3',
    'GPlates-0ba4c93d-474e-4d9b-8f1b-618cb21024de',
    'GPlates-84be6d41-6c32-4184-9c44-c38e399090a0',
    'GPlates-3df7a9df-aefc-403e-a16c-faf203776fd1',
    'GPlates-56f22e61-ddd5-4c2f-ae41-54e5f66f47ec']
# [end: line-referenced-feature-ids]

# [fragment: create-topological-line]
# Create a topological line.
topological_line_feature = create_topological_line_feature(
        features,
        topological_line_referenced_feature_id_strings,
        pygplates.FeatureType.gpml_unclassified_feature)
# [end: create-topological-line]
topological_features.append(topological_line_feature)

# [fragment: add-topological-line]
# Add the topological line to the list of features that topologies can reference.
# The topological line will be referenced in turn by a topological polygon and a topological network (below).
features.add(topological_line_feature)
# [end: add-topological-line]

# [fragment: polygon-referenced-feature-ids]
topological_polygon_referenced_feature_id_strings = [
    'GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2',
    'GPlates-48bd0e0f-e7c8-4dea-9e0a-4bc0e1403db6',
    'GPlates-71470e03-9e99-4205-80d9-727d7a3700de',
    # Topological polygon references the topological line we created...
    topological_line_feature.get_feature_id().get_string()]
# [end: polygon-referenced-feature-ids]

# [fragment: create-topological-polygon]
# Create a topological polygon.
topological_polygon_feature = create_topological_polygon_feature(
    features,
    topological_polygon_referenced_feature_id_strings,
    pygplates.FeatureType.gpml_topological_closed_plate_boundary)
# [end: create-topological-polygon]
topological_features.append(topological_polygon_feature)

# [fragment: network-referenced-feature-ids]
topological_network_boundary_referenced_feature_id_strings = [
    'GPlates-63b81b91-b7a0-4ad7-908d-16db3c70e6ed',
    'GPlates-aa1d0d5a-0445-4380-a516-d2bc66e477a7',
    'GPlates-e184b54d-abb0-465b-8820-c73c543d2562',
    'GPlates-5369725b-5ca6-49b2-83c6-0417dbb5fca2',
    # Topological network references the topological line we created...
    topological_line_feature.get_feature_id().get_string(),
    'GPlates-cc5b9027-d227-4e97-bb06-df26786fd1ec']
topological_network_interior_referenced_feature_id_strings = [
    'GPlates-56ffca31-df55-4a3e-b943-06faa1407fed',
    'GPlates-a913e755-deaf-4bc5-918a-a124611341c1']
# [end: network-referenced-feature-ids]

# [fragment: create-topological-network]
# Create a topological network.
topological_network_feature = create_topological_network_feature(
    features,
    topological_network_boundary_referenced_feature_id_strings,
    topological_network_interior_referenced_feature_id_strings)
# [end: create-topological-network]
topological_features.append(topological_network_feature)

# Save the topological features we created.
pygplates.FeatureCollection(topological_features).write('topologies.gpml')
