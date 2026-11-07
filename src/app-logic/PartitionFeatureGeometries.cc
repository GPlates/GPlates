/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "PartitionFeatureGeometries.h"

#include "GeometryUtils.h"
#include "ReconstructionFeatureProperties.h"
#include "ScalarCoverageFeatureProperties.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GeometryDistance.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/Vector3D.h"

#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTimeWindow.h"


namespace GPlatesAppLogic
{
	namespace PartitionFeatureUtils
	{
		namespace
		{
			/**
			 * Visit a feature property and, if it contains geometry, partitions it
			 * using partitioning polygons and stores results for later retrieval.
			 */
			class PartitionFeatureGeometryProperties :
					// TODO: Change this to ConstFeatureVisitor once the new bubble-up model
					// (that doesn't clone non-const visitors) is finished...
					public GPlatesModel::FeatureVisitor
			{
			public:

				PartitionFeatureGeometryProperties(
						const GeometryCookieCutter &geometry_cookie_cutter,
						const std::vector<ScalarCoverageFeatureProperties::Coverage> &geometry_coverages,
						boost::optional< std::vector<GPlatesModel::FeatureHandle::iterator> &> partitioned_properties) :
					d_cookie_cut_geometry(geometry_cookie_cutter),
					d_geometry_coverages(geometry_coverages),
					d_partitioned_properties(partitioned_properties)
				{
					// The partitioning results will go here.
					d_partition_results.reset(new PartitionedFeature());
				}


				boost::shared_ptr<const PartitionedFeature>
				get_partitioned_feature_geometries() const
				{
					return d_partition_results;
				}

			protected:
				virtual
				void
				visit_gml_line_string(
						gml_line_string_type &gml_line_string)
				{
					add_geometry(gml_line_string.get_polyline());
				}


				virtual
				void
				visit_gml_multi_point(
						gml_multi_point_type &gml_multi_point)
				{
					add_geometry(gml_multi_point.get_multipoint());
				}


				virtual
				void
				visit_gml_orientable_curve(
						gml_orientable_curve_type &gml_orientable_curve)
				{
					gml_orientable_curve.base_curve()->accept_visitor(*this);
				}


				virtual
				void
				visit_gml_point(
						gml_point_type &gml_point)
				{
					add_geometry(gml_point.get_point().get_geometry_on_sphere());
				}


				virtual
				void
				visit_gml_polygon(
						gml_polygon_type &gml_polygon)
				{
					add_geometry(gml_polygon.get_polygon());
				}


				virtual
				void
				visit_gpml_constant_value(
						gpml_constant_value_type &gpml_constant_value)
				{
					gpml_constant_value.value()->accept_visitor(*this);
				}


				/**
				 * A time-dependent geometry is partitioned only if it has a single time window, and
				 * that window contains the reconstruction time: then it is partitioned as a constant
				 * geometry, and its pieces (or, whole, the geometry) are written back as constant
				 * properties. With one window there are no others to leave undefined, and its begin
				 * and end say no more than the feature's valid time (early GPlates builds wrapped a
				 * single geometry this way). Any other time-dependent geometry is not partitioned (a
				 * window's pieces would leave the other windows undefined), so its property stays,
				 * unchanged. A GpmlIrregularSampling (the base visitor's no-op) is never partitioned.
				 */
				virtual
				void
				visit_gpml_piecewise_aggregation(
						gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
				{
					GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows =
							gpml_piecewise_aggregation.time_windows();
					if (time_windows.size() != 1 ||
						!time_windows.front()->valid_time()->contains(
								d_cookie_cut_geometry.get_reconstruction_time()))
					{
						return;
					}

					d_single_time_window_value = time_windows.front()->time_dependent_value();
					time_windows.front()->time_dependent_value()->accept_visitor(*this);
					d_single_time_window_value = boost::none;
				}

			private:
				//! Does the cookie-cutting.
				const GeometryCookieCutter &d_cookie_cut_geometry;

				//! Scalar coverages associated with geometry properties.
				std::vector<ScalarCoverageFeatureProperties::Coverage> d_geometry_coverages;

				//! Optional sequence of partitioned properties (geometry domains and associated ranges) to return to caller.
				boost::optional< std::vector<GPlatesModel::FeatureHandle::iterator> &> d_partitioned_properties;

				//! The results of the cookie-cutting.
				boost::shared_ptr<PartitionedFeature> d_partition_results;

				//! The value of the single time window being visited, if any (see above).
				boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_single_time_window_value;


				//! One value per element of a coverage's tuple (its scalar types).
				typedef std::vector<double> value_type;


				/**
				 * A geometry property's scalar coverage, and what carrying it to the partitioned
				 * geometries needs (spherical-geometry.md section 5.4): its domain's rings, so that
				 * a point's provenance (a vertex, or a segment and ratio, of the geometry property)
				 * finds its scalars.
				 */
				struct Range
				{
					// Number of scalars in range should match number of points in domain.
					// This should be the case but we'll double-check in case it's not.
					static
					bool
					range_matches_domain(
							const geometry_domain_type &domain_,
							const geometry_range_type &range_)
					{
						// Should have at least something in the range to compare with.
						if (range_.empty())
						{
							return false;
						}

						const unsigned int num_domain_points = GeometryUtils::get_num_geometry_points(*domain_);
						for (unsigned int s = 0; s < range_.size(); ++s)
						{
							if (num_domain_points != range_[s]->get_coordinates().size())
							{
								return false;
							}
						}

						return true;
					}

					/**
					 * @a domain_ is the coverage's domain ('ScalarCoverageFeatureProperties::Coverage::domain'),
					 * which is the exterior ring alone if @a exterior_ring_only_.
					 */
					Range(
							const geometry_domain_type &domain_,
							const geometry_range_type &range_,
							bool exterior_ring_only_) :
						domain(domain_),
						domain_type(GPlatesMaths::GeometryType::NONE),
						range(range_),
						exterior_ring_only(exterior_ring_only_)
					{
						// Get the geometry domain points (a polygon's exterior ring, then its interior rings).
						domain_type = GeometryUtils::get_geometry_points(*domain_, domain_points);

						// The domain's rings, in the same order (a polyline or multi-point is one).
						if (const GPlatesMaths::PolygonOnSphere *polygon =
								dynamic_cast<const GPlatesMaths::PolygonOnSphere *>(domain_.get()))
						{
							ring_sizes.push_back(polygon->number_of_vertices_in_exterior_ring());
							for (unsigned int i = 0; i < polygon->number_of_interior_rings(); ++i)
							{
								ring_sizes.push_back(
										polygon->number_of_vertices_in_interior_ring(i));
							}
						}
						else
						{
							ring_sizes.push_back(domain_points.size());
						}
						unsigned int offset = 0;
						for (unsigned int size : ring_sizes)
						{
							ring_offsets.push_back(offset);
							offset += size;
						}
					}

					//! The scalars of a vertex of the domain, if it has them (none for a hole's vertex
					//! under an exterior-ring-only coverage, whose domain is the exterior ring alone).
					boost::optional<value_type>
					get_vertex_value(
							unsigned int ring,
							unsigned int index) const
					{
						if (ring >= ring_sizes.size() || index >= ring_sizes[ring])
						{
							return boost::none;
						}
						value_type value;
						for (const auto &element : range)
						{
							value.push_back(element->get_coordinates()[ring_offsets[ring] + index]);
						}
						return value;
					}

					//! The scalars at @a ratio along a segment of the domain, interpolated between
					//! its ends (a polygon's ring's last segment ends at the ring's first vertex).
					boost::optional<value_type>
					get_segment_value(
							unsigned int ring,
							unsigned int segment,
							double ratio) const
					{
						if (ring >= ring_sizes.size())
						{
							return boost::none;
						}
						unsigned int next = segment + 1;
						if (domain_type == GPlatesMaths::GeometryType::POLYGON &&
							next == ring_sizes[ring])
						{
							next = 0;
						}
						const boost::optional<value_type> start = get_vertex_value(ring, segment);
						const boost::optional<value_type> end = get_vertex_value(ring, next);
						if (!start || !end)
						{
							return boost::none;
						}
						value_type value(start->size());
						for (unsigned int t = 0; t < value.size(); ++t)
						{
							value[t] = (1.0 - ratio) * (*start)[t] + ratio * (*end)[t];
						}
						return value;
					}

					/**
					 * The scalars at the nearest point of the domain's outline to @a point,
					 * interpolated along that segment (section 5.4, step 3): for an output ring
					 * none of whose points has a value of its own.
					 */
					value_type
					get_nearest_outline_value(
							const GPlatesMaths::PointOnSphere &point) const
					{
						unsigned int closest_point_index = 0;  // to be ignored (zero)
						unsigned int closest_domain_index = 0; // a segment (or a point)
						minimum_distance(
								*point.get_geometry_on_sphere(),
								*domain,
								false/*geometry1_interior_is_solid*/,
								false/*geometry2_interior_is_solid*/,
								boost::none/*minimum_distance_threshold*/,
								boost::none/*closest_positions*/,
								boost::make_tuple(
										boost::ref(closest_point_index),
										boost::ref(closest_domain_index)));

						if (domain_type != GPlatesMaths::GeometryType::POLYGON &&
							domain_type != GPlatesMaths::GeometryType::POLYLINE)
						{
							return *get_vertex_value(
									0,
									(domain_type == GPlatesMaths::GeometryType::MULTIPOINT)
											? closest_domain_index
											: 0);
						}

						// The segment's ring (segments are numbered like the points).
						unsigned int ring = 0;
						while (ring + 1 < ring_offsets.size() &&
							closest_domain_index >= ring_offsets[ring + 1])
						{
							++ring;
						}
						const unsigned int segment = closest_domain_index - ring_offsets[ring];
						const unsigned int next =
								(domain_type == GPlatesMaths::GeometryType::POLYGON &&
									segment + 1 == ring_sizes[ring])
										? 0
										: segment + 1;
						const GPlatesMaths::PointOnSphere &start =
								domain_points[ring_offsets[ring] + segment];
						const GPlatesMaths::PointOnSphere &end =
								domain_points[ring_offsets[ring] + next];
						// The ratio along the segment of the point's projection onto its great
						// circle (as the overlay's ratios are, section 5.4),
						// within the segment.
						const GPlatesMaths::Vector3D a(start.position_vector());
						const GPlatesMaths::Vector3D b(end.position_vector());
						const GPlatesMaths::Vector3D p(point.position_vector());
						const GPlatesMaths::Vector3D normal = cross(a, b);
						const double segment_angle =
								std::atan2(normal.magnitude().dval(), dot(a, b).dval());
						double ratio = 0.0;
						if (normal.magSqrd().dval() > 0 && segment_angle > 0)
						{
							const GPlatesMaths::Vector3D n =
									(1.0 / normal.magnitude().dval()) * normal;
							const GPlatesMaths::Vector3D projection = p - dot(p, n).dval() * n;
							const double along = std::atan2(
									dot(cross(a, projection), n).dval(),
									dot(a, projection).dval());
							ratio = (std::max)(0.0, (std::min)(1.0, along / segment_angle));
						}
						return *get_segment_value(ring, segment, ratio);
					}

					geometry_domain_type domain;
					GPlatesMaths::GeometryType::Value domain_type;
					std::vector<GPlatesMaths::PointOnSphere> domain_points;
					std::vector<unsigned int> ring_sizes;
					std::vector<unsigned int> ring_offsets;
					geometry_range_type range;

					//! See 'ScalarCoverageFeatureProperties::Coverage::exterior_ring_only'.
					bool exterior_ring_only;
				};


				/**
				 * Partition the geometry @a geometry of the current geometry property.
				 */
				void
				add_geometry(
						const geometry_domain_type &geometry_domain)
				{
					// The geometry domain may also have a range (scalar coverage).
					boost::optional<Range> geometry_range;

					// Create a new partition entry for the current geometry property.
					PartitionedFeature::GeometryProperty &partitioned_geometry_property =
							get_geometry_property(geometry_domain, geometry_range);

					// Partition the current geometry property and store results.
					partition_geometry(geometry_domain, geometry_range, partitioned_geometry_property);
				}

				static
				GPlatesModel::TopLevelProperty::non_null_ptr_type
				create_constant_property(
						const GPlatesModel::PropertyName &property_name,
						const GPlatesModel::PropertyValue::non_null_ptr_type &property_value)
				{
					const boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> property =
							GPlatesModel::ModelUtils::create_top_level_property(
									property_name,
									property_value,
									boost::none/*feature_type*/,
									false/*check_property_value_type*/);
					return property
							? property.get()
							: GPlatesModel::TopLevelProperty::non_null_ptr_type(
									GPlatesModel::TopLevelPropertyInline::create(property_name, property_value));
				}

				PartitionedFeature::GeometryProperty &
				get_geometry_property(
						const geometry_domain_type &geometry_domain,
						boost::optional<Range> &geometry_range)
				{
					// Property name and iterator of current geometry property.
					const GPlatesModel::PropertyName &geometry_domain_property_name = *current_top_level_propname();
					const feature_iterator_type &geometry_domain_property_iterator = *current_top_level_propiter();

					// If caller requests partitioned properties.
					if (d_partitioned_properties)
					{
						d_partitioned_properties->push_back(geometry_domain_property_iterator);
					}

					// Create a shallow clone of the current geometry property.
					// This is quite quick to create compared to the deep clone since it's a bunch
					// of intrusive pointer copies.
					// This might be used by the caller to move a geometry property between features.
					// For example, if this geometry property requires a different plate id.
					//
					// For a single time window's geometry, a constant property of the same name instead,
					// with the time-dependent wrapper (or none) the information model gives that name.
					const GPlatesModel::TopLevelProperty::non_null_ptr_type geometry_domain_property_clone =
							d_single_time_window_value
									? create_constant_property(
											geometry_domain_property_name,
											d_single_time_window_value.get()->clone())
									: (*geometry_domain_property_iterator)->clone();

					boost::optional<GPlatesModel::PropertyName> geometry_range_property_name;
					boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> geometry_range_property_clone;

					// See if there's a scalar coverage range associated with the geometry domain.
					const unsigned int num_geometry_coverages = d_geometry_coverages.size();
					for (unsigned int n = 0; n < num_geometry_coverages; ++n)
					{
						const ScalarCoverageFeatureProperties::Coverage &coverage = d_geometry_coverages[n];

						if (coverage.domain_property == geometry_domain_property_iterator)
						{
							// Number of scalars in range should match number of points in domain.
							// This should be the case but we'll double-check in case it's not.
							if (Range::range_matches_domain(coverage.domain, coverage.range))
							{
								geometry_range = boost::in_place(coverage.domain, coverage.range, coverage.exterior_ring_only);
							}

							geometry_range_property_name = (*coverage.range_property)->get_property_name();
							// Create a shallow clone of the range property. For a single time window's
							// geometry whose range is in a single time window too (the one containing
							// the reconstruction time, where the coverage was found), a constant
							// property of its window's value instead, as the domain's.
							const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
									range_value = GPlatesModel::ModelUtils::get_property_value(
											**coverage.range_property);
							const GPlatesPropertyValues::GpmlPiecewiseAggregation *range_windows = range_value
									? dynamic_cast<const GPlatesPropertyValues::GpmlPiecewiseAggregation *>(
											range_value->get())
									: nullptr;
							geometry_range_property_clone =
									(d_single_time_window_value && range_windows &&
										range_windows->time_windows().size() == 1)
											? create_constant_property(
													*geometry_range_property_name,
													range_windows->time_windows().front()->time_dependent_value()->clone())
											: (*coverage.range_property)->clone();

							// If caller requests partitioned properties.
							if (d_partitioned_properties)
							{
								d_partitioned_properties->push_back(coverage.range_property);
							}

							break;
						}
					}

					// Create a new entry for the current geometry property
					// (or return existing entry based on domain property name).
					std::pair<PartitionedFeature::partitioned_geometry_property_map_type::iterator, bool> inserted =
							d_partition_results->partitioned_geometry_properties.insert(
									PartitionedFeature::partitioned_geometry_property_map_type::value_type(
											geometry_domain_property_name,
											PartitionedFeature::GeometryProperty(geometry_domain_property_name)));

					// Get a reference to the entry just inserted (or existing entry).
					PartitionedFeature::GeometryProperty &geometry_property = inserted.first->second;

					// Add the current geometry property clone.
					geometry_property.property_clones.push_back(
							PartitionedFeature::GeometryPropertyClone(
									geometry_domain_property_clone,
									geometry_range_property_clone));

					// If there's a range for the current domain then add the range property name.
					//
					// Note that it's possible some domains will have no associated range while
					// other domains (with the same domain property name) will have associated ranges.
					// As long as one of the domains has an associated range then we'll set the range name.
					if (geometry_range_property_name)
					{
						geometry_property.range_property_name = geometry_range_property_name;
					}

					return geometry_property;
				}

				void
				partition_geometry(
						const geometry_domain_type &geometry_domain,
						const boost::optional<Range> &geometry_range,
						PartitionedFeature::GeometryProperty &partitioned_geometry_property)
				{
					// Partition the current geometry property and store results; with a coverage, each
					// piece with where its points came from (section 5.4).
					GeometryCookieCutter::partition_seq_type partitioned_inside_domains;
					GeometryCookieCutter::partitioned_geometry_seq_type partitioned_outside_domains;
					d_cookie_cut_geometry.partition_geometry(
							geometry_domain,
							partitioned_inside_domains,
							partitioned_outside_domains,
							static_cast<bool>(geometry_range)/*want_provenance*/);

					// Iterate over the partitioned polygons and add the partitioned *inside* geometries.
					GeometryCookieCutter::partition_seq_type::const_iterator partitioned_inside_domains_iter =
							partitioned_inside_domains.begin();
					GeometryCookieCutter::partition_seq_type::const_iterator partitioned_inside_domains_end =
							partitioned_inside_domains.end();
					for ( ; partitioned_inside_domains_iter != partitioned_inside_domains_end; ++partitioned_inside_domains_iter)
					{
						const GeometryCookieCutter::Partition &partitioned_inside_domain = *partitioned_inside_domains_iter;

						partitioned_geometry_property.partitioned_inside_geometries.push_back(
								PartitionedFeature::Partition(partitioned_inside_domain.reconstruction_geometry));
						PartitionedFeature::Partition &partition =
								partitioned_geometry_property.partitioned_inside_geometries.back();

						partition_geometries(
								geometry_range,
								partitioned_inside_domain.partitioned_geometries,
								partition.partitioned_geometries);
					}

					// Add the partitioned *outside* geometries.
					partition_geometries(
							geometry_range,
							partitioned_outside_domains,
							partitioned_geometry_property.partitioned_outside_geometries);
				}

				void
				partition_geometries(
						const boost::optional<Range> &geometry_range,
						const GeometryCookieCutter::partitioned_geometry_seq_type &partitioned_domains,
						PartitionedFeature::partitioned_geometry_seq_type &partitioned_geometries)
				{
					for (const GeometryCookieCutter::PartitionedGeometry &partitioned :
							partitioned_domains)
					{
						// If there is a geometry range associated with the geometry domain then
						// create a partitioned range associated with the partitioned domain.
						boost::optional<geometry_range_type> partitioned_range;
						if (geometry_range)
						{
							partitioned_range = geometry_range_type();
							partition_range(
									partitioned_range.get(),
									partitioned,
									geometry_range.get());
						}

						partitioned_geometries.push_back(
								PartitionedFeature::PartitionedGeometry(
										partitioned.geometry,
										partitioned_range));
					}
				}

				/**
				 * The coverage of a partitioned geometry, from its points' provenance
				 * (section 5.4, replacing an epsilon map of the original points and a search for
				 * the nearest original segment, which a cut across the geometry's interior
				 * defeats):
				 *
				 *  1. a point at a vertex of the original takes its scalars; a point on a segment of
				 *     the original interpolates its ends' (a ring's last segment ends at its first
				 *     vertex);
				 *  2. a run of points with none (a cut, where the partitioning polygon's outline
				 *     crosses the geometry; or a hole's points under an exterior-ring-only coverage)
				 *     is interpolated by arc length between the valued points at its ends;
				 *  3. a ring none of whose points has a value takes, at each point, the value at
				 *     the nearest point of the original's outline.
				 */
				void
				partition_range(
						geometry_range_type &partitioned_range,
						const GeometryCookieCutter::PartitionedGeometry &partitioned,
						const Range &geometry_range)
				{
					// The partitioned domain's points and rings (only a polygon's exterior ring for
					// an exterior-ring-only coverage, so that the partitioned coverage is read the
					// same way as the original).
					std::vector<GPlatesMaths::PointOnSphere> points;
					std::vector<unsigned int> ring_sizes;
					bool rings_are_closed = false;
					if (const GPlatesMaths::PolygonOnSphere *polygon =
							dynamic_cast<const GPlatesMaths::PolygonOnSphere *>(
									partitioned.geometry.get()))
					{
						rings_are_closed = true;
						ring_sizes.push_back(polygon->number_of_vertices_in_exterior_ring());
						if (!geometry_range.exterior_ring_only)
						{
							for (unsigned int i = 0; i < polygon->number_of_interior_rings(); ++i)
							{
								ring_sizes.push_back(
										polygon->number_of_vertices_in_interior_ring(i));
							}
						}
						GeometryUtils::get_geometry_points(*polygon, points);
						points.erase(
								points.begin() +
										std::accumulate(ring_sizes.begin(), ring_sizes.end(), 0u),
								points.end());
					}
					else
					{
						GeometryUtils::get_geometry_points(*partitioned.geometry, points);
						if (dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(
								partitioned.geometry.get()))
						{
							ring_sizes.push_back(points.size());
						}
						else
						{
							// A (multi-)point: each point its own ring (valued by step 1).
							ring_sizes.assign(points.size(), 1);
						}
					}

					// The cookie cutter gives every piece a table when asked (section 5.4).
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							partitioned.provenance &&
								partitioned.provenance->size() >= points.size(),
							GPLATES_ASSERTION_SOURCE);
					const GPlatesMaths::PolygonOverlay::provenance_seq_type &provenance =
							*partitioned.provenance;

					// Step 1.
					std::vector<boost::optional<value_type>> values(points.size());
					for (unsigned int i = 0; i < points.size(); ++i)
					{
						const boost::optional<GPlatesMaths::PolygonOverlay::Location> &location =
								provenance[i].subject;
						if (!location)
						{
							continue;  // a cut point
						}
						values[i] = location->ratio
								? geometry_range.get_segment_value(
										location->ring,
										location->index,
										*location->ratio)
								: geometry_range.get_vertex_value(location->ring, location->index);
					}

					// Steps 2 and 3, ring by ring.
					unsigned int ring_start = 0;
					for (unsigned int ring_size : ring_sizes)
					{
						interpolate_ring_values(
								values,
								points,
								ring_start,
								ring_size,
								rings_are_closed,
								geometry_range);
						ring_start += ring_size;
					}

					// Create partitioned GmlDataBlockCoordinateList's.
					const unsigned int range_tuple_size = geometry_range.range.size();
					for (unsigned int t = 0; t < range_tuple_size; ++t)
					{
						GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type coordinates;
						coordinates.reserve(points.size());
						for (const boost::optional<value_type> &value : values)
						{
							coordinates.push_back((*value)[t]);
						}

						const GPlatesPropertyValues::GmlDataBlockCoordinateList &range_tuple_element =
								*geometry_range.range[t];
						partitioned_range.push_back(
								GPlatesPropertyValues::GmlDataBlockCoordinateList::create(
										range_tuple_element.get_value_object_type(),
										range_tuple_element.get_value_object_xml_attributes(),
										coordinates));
					}
				}

				/**
				 * Steps 2 and 3 of @a partition_range on one ring of @a values (cyclic if @a closed, else
				 * a polyline): a run of points with no value between two that have one is interpolated
				 * by arc length between them (a polyline's run at an end takes its one neighbour's); a
				 * ring with no value at all takes each point's nearest outline value.
				 */
				static
				void
				interpolate_ring_values(
						std::vector<boost::optional<value_type>> &values,
						const std::vector<GPlatesMaths::PointOnSphere> &points,
						unsigned int ring_start,
						unsigned int ring_size,
						bool closed,
						const Range &geometry_range)
				{
					const auto angle = [&points](unsigned int a, unsigned int b)
					{
						const GPlatesMaths::UnitVector3D &u = points[a].position_vector();
						const GPlatesMaths::UnitVector3D &v = points[b].position_vector();
						return std::atan2(cross(u, v).magnitude().dval(), dot(u, v).dval());
					};

					// The ring's first point with a value; none means step 3.
					unsigned int first = 0;
					while (first < ring_size && !values[ring_start + first])
					{
						++first;
					}
					if (first == ring_size)
					{
						for (unsigned int k = 0; k < ring_size; ++k)
						{
							values[ring_start + k] =
									geometry_range.get_nearest_outline_value(
										points[ring_start + k]);
						}
						return;
					}

					// Walk from the first valued point (once round a closed ring, so that a run
					// wrapping past the ring's start is interpolated too; to the end of a
					// polyline), interpolating each run of points with none between its two
					// valued ends.
					std::vector<unsigned int> run;  // the run's point indices
					std::vector<double> distances;  // arc length from the run's start to each
					unsigned int previous = ring_start + first;
					unsigned int valued = previous;
					double distance = 0.0;
					const unsigned int num_steps = closed ? ring_size : ring_size - first - 1;
					for (unsigned int step = 1; step <= num_steps; ++step)
					{
						const unsigned int k = ring_start + (first + step) % ring_size;
						distance += angle(previous, k);
						previous = k;
						if (!values[k])
						{
							run.push_back(k);
							distances.push_back(distance);
							continue;
						}
						for (unsigned int r = 0; r < run.size(); ++r)
						{
							const double ratio = (distance > 0) ? distances[r] / distance : 0.0;
							value_type value(values[valued]->size());
							for (unsigned int t = 0; t < value.size(); ++t)
							{
								value[t] = (1.0 - ratio) * (*values[valued])[t] +
										ratio * (*values[k])[t];
							}
							values[run[r]] = value;
						}
						run.clear();
						distances.clear();
						valued = k;
						distance = 0.0;
					}

					// A polyline's run at its end, and at its start, takes its one valued
					// neighbour's. (A closed ring has none left: the walk went once round it, so
					// its run past its start was interpolated.)
					if (!closed)
					{
						for (unsigned int r : run)
						{
							values[r] = values[valued];
						}
						for (unsigned int k = 0; k < first; ++k)
						{
							values[ring_start + k] = values[ring_start + first];
						}
					}
				}
			};
		}
	}
}


boost::shared_ptr<const GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature>
GPlatesAppLogic::PartitionFeatureUtils::partition_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GeometryCookieCutter &geometry_cookie_cutter,
		bool respect_feature_time_period,
		boost::optional< std::vector<GPlatesModel::FeatureHandle::iterator> &> partitioned_properties)
{
	// Only partition features that exist at the partitioning reconstruction time if we've been requested.
	if (respect_feature_time_period &&
		!does_feature_exist_at_reconstruction_time(
			feature_ref,
			geometry_cookie_cutter.get_reconstruction_time()))
	{
		return boost::shared_ptr<const PartitionedFeature>();
	}

	// Get any scalar coverages associated with the feature's geometry properties.
	std::vector<ScalarCoverageFeatureProperties::Coverage> geometry_coverages;
	ScalarCoverageFeatureProperties::get_coverages(
			geometry_coverages,
			feature_ref,
			geometry_cookie_cutter.get_reconstruction_time());

	PartitionFeatureGeometryProperties feature_partitioner(
			geometry_cookie_cutter,
			geometry_coverages,
			partitioned_properties);
	feature_partitioner.visit_feature(feature_ref);

	return feature_partitioner.get_partitioned_feature_geometries();
}


bool
GPlatesAppLogic::PartitionFeatureUtils::does_feature_exist_at_reconstruction_time(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
		const double &reconstruction_time)
{
	ReconstructionFeatureProperties reconstruction_params;

	reconstruction_params.visit_feature(feature_ref);

	return reconstruction_params.is_feature_defined_at_recon_time(reconstruction_time);
}


