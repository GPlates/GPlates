/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <map>
#include <numeric>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>
#include <QString>

#include "PartitionFeatureUtils.h"

#include "GeometrySetter.h"
#include "GeometryUtils.h"
#include "PartitionFeatureTask.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ReconstructMethodRegistry.h"
#include "ScalarCoverageFeatureProperties.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/AngularExtent.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"
#include "maths/GeometryDistance.h"
#include "maths/MathsUtils.h"
#include "maths/Vector3D.h"

#include "model/Gpgim.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/PropertyName.h"
#include "model/PropertyValueFinder.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"


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

			private:
				//! Does the cookie-cutting.
				const GeometryCookieCutter &d_cookie_cut_geometry;

				//! Scalar coverages associated with geometry properties.
				std::vector<ScalarCoverageFeatureProperties::Coverage> d_geometry_coverages;

				//! Optional sequence of partitioned properties (geometry domains and associated ranges) to return to caller.
				boost::optional< std::vector<GPlatesModel::FeatureHandle::iterator> &> d_partitioned_properties;

				//! The results of the cookie-cutting.
				boost::shared_ptr<PartitionedFeature> d_partition_results;


				//! One value per element of a coverage's tuple (its scalar types).
				typedef std::vector<double> value_type;


				/**
				 * A geometry property's scalar coverage, and what carrying it to the partitioned
				 * geometries needs (phase4b-design.md section 7.3): its domain's rings, so that a
				 * point's provenance (a vertex, or a segment and ratio, of the geometry property)
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
					* interpolated along that segment (section 7.3 step 3, the developer's answer to
					* question 2): for an output ring none of whose points has a value of its own.
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
						// circle (as the overlay's ratios are, phase4b-design.md section 2.8),
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
					const GPlatesModel::TopLevelProperty::non_null_ptr_type geometry_domain_property_clone =
							(*geometry_domain_property_iterator)->clone();

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
							// Create a shallow clone of the range property.
							geometry_range_property_clone = (*coverage.range_property)->clone();

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
					// piece with where its points came from (phase4b-design.md section 7.2).
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
				* (phase4b-design.md
				 * section 7.3, replacing an epsilon map of the original points and a search for the
				 * nearest original segment, which a cut across the geometry's interior defeats):
				 *
				*  1. a point at a vertex of the original takes its scalars; a point on a segment of
				*     the original interpolates its ends' (a ring's last segment ends at its first
				*     vertex);
				*  2. a run of points with none (a cut, where the partitioning polygon's outline
				*     crosses the geometry; or a hole's points under an exterior-ring-only coverage)
				*     is interpolated by arc length between the valued points at its ends
				*     (requirements Q1);
				*  3. a ring none of whose points has a value takes, at each point, the value at
				*     the nearest point of the original's outline (section 14, question 2).
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

					// The cookie cutter gives every piece a table when asked (section 7.2).
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

						const GPlatesPropertyValues::GmlDataBlockCoordinateList &range_tuple_element = *geometry_range.range[t];
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

					// Walk from the first valued point (once round a closed ring, to the end of a
					// polyline),
					// interpolating each run of points with none between its two valued ends.
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
					// neighbour's.
					for (unsigned int r : run)
					{
						values[r] = values[valued];
					}
					for (unsigned int k = 0; k < first; ++k)
					{
						values[ring_start + k] = values[ring_start + first];
					}
				}
			};



			/**
			 * Calculate polyline distance along unit radius sphere.
			 */
			template <typename GreatCircleArcForwardIteratorType>
			GPlatesMaths::real_t
			calculate_arc_distance(
					GreatCircleArcForwardIteratorType gca_begin,
					GreatCircleArcForwardIteratorType gca_end)
			{
				GPlatesMaths::real_t distance = 0;

				GreatCircleArcForwardIteratorType gca_iter = gca_begin;
				for ( ; gca_iter != gca_end; ++gca_iter)
				{
					distance += gca_iter->arc_length();
				}

				return distance;
			}


			/**
			 * Accumulates a geometry's size: a polygon's area, a polyline's arc length, a point's or
			 * multi-point's number of points (phase4b-design.md section 8.3). A polygon's arc length
			 * is not used: its cuts would inflate it.
			 */
			class GeometrySize :
					public GPlatesMaths::ConstGeometryOnSphereVisitor
			{
			public:
				GeometrySize(
						unsigned int &num_points,
						GPlatesMaths::real_t &arc_distance,
						GPlatesMaths::real_t &area) :
					d_num_points(num_points),
					d_arc_distance(arc_distance),
					d_area(area)
				{  }

				virtual
				void
				visit_multi_point_on_sphere(
						GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
				{
					d_num_points += multi_point_on_sphere->number_of_points();
				}

				virtual
				void
				visit_point_on_sphere(
						GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
				{
					++d_num_points;
				}

				virtual
				void
				visit_polygon_on_sphere(
						GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
				{
					d_area += polygon_on_sphere->get_area();
				}

				virtual
				void
				visit_polyline_on_sphere(
						GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
				{
					d_arc_distance += polyline_on_sphere->get_arc_length();
				}

				unsigned int &d_num_points;
				GPlatesMaths::real_t &d_arc_distance;
				GPlatesMaths::real_t &d_area;
			};


			/**
			 * Calculates the accumulated size metric for all partitioned inside geometries
			 * of @a partition.
			 */
			GeometrySizeMetric
			calculate_partition_size_metric(
					const PartitionedFeature::Partition &partition)
			{
				GeometrySizeMetric partition_size_metric;

				//
				// Iterate over the geometries inside the current partitioning polygon.
				//
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator inside_geometries_iter =
						partition.partitioned_geometries.begin();
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator inside_geometries_end =
						partition.partitioned_geometries.end();
				for ( ; inside_geometries_iter != inside_geometries_end; ++inside_geometries_iter)
				{
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &inside_geometry =
							inside_geometries_iter->geometry_domain;

					partition_size_metric.accumulate(*inside_geometry);
				}

				return partition_size_metric;
			}


			/**
			 * Returns "gpml:reconstructionPlateId".
			 */
			const GPlatesModel::PropertyName &
			get_reconstruction_plate_id_property_name()
			{
				static const GPlatesModel::PropertyName reconstruction_plate_id_property_name =
						GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

				return reconstruction_plate_id_property_name;
			}


			/**
			 * Returns "gpml:conjugatePlateId".
			 */
			const GPlatesModel::PropertyName &
			get_conjugate_plate_id_property_name()
			{
				static const GPlatesModel::PropertyName conjugate_plate_id_property_name =
						GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

				return conjugate_plate_id_property_name;
			}


			/**
			 * Returns "gml:validTime".
			 */
			const GPlatesModel::PropertyName &
			get_valid_time_property_name()
			{
				static const GPlatesModel::PropertyName valid_time_property_name =
						GPlatesModel::PropertyName::create_gml("validTime");

				return valid_time_property_name;
			}


			/**
			 * Adds partitioned geometries to the partitioned feature associated with @a partition.
			 *
			 * If @a partition is none then adds to the special feature associated with no partition.
			 *
			 * All partitioned geometries are reverse reconstructed using the plate id of their partitioning polygon
			 * (if has a plate id) and/or deformed if @a reconstruct_method_context contains deformation.
			 */
			void
			add_partitioned_geometries_to_feature(
					const PartitionedFeature::partitioned_geometry_seq_type &partitioned_geometries,
					const GPlatesModel::PropertyName &geometry_domain_property_name,
					const boost::optional<GPlatesModel::PropertyName> &geometry_range_property_name,
					PartitionedFeatureManager &partitioned_feature_manager,
					const ReconstructMethodInterface::Context &reconstruct_method_context,
					const double &reconstruction_time,
					boost::optional<const ReconstructionGeometry *> partition = boost::none)
			{
				//
				// Iterate over the partitioned geometries.
				//
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator partitioned_geometries_iter =
						partitioned_geometries.begin();
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator partitioned_geometries_end =
						partitioned_geometries.end();
				for ( ; partitioned_geometries_iter != partitioned_geometries_end; ++partitioned_geometries_iter)
				{
					const PartitionedFeature::PartitionedGeometry &partitioned_geometry = *partitioned_geometries_iter;

					const bool geometry_domain_has_associated_range =
							geometry_range_property_name && partitioned_geometry.geometry_range;

					// Note that we only get the partitioned feature when we know we
					// are going to append a geometry property to it.
					// If there are no partitioned geometries then it doesn't get called
					// which means a new feature won't get cloned. 
					const GPlatesModel::FeatureHandle::weak_ref partitioned_feature =
							partitioned_feature_manager.get_feature_for_partition(
									geometry_domain_property_name,
									geometry_domain_has_associated_range,
									partition);

					// Reverse reconstruct to get the present day geometry.
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_partitioned_geometry =
							reverse_reconstruct(
									partitioned_geometry.geometry_domain,
									partitioned_feature,
									reconstruct_method_context,
									reconstruction_time);

					// Add the geometry domain property.
					append_geometry_domain_to_feature(
							present_day_partitioned_geometry,
							geometry_domain_property_name,
							partitioned_feature);

					// If there's an associated geometry range then add it as a property.
					if (geometry_domain_has_associated_range)
					{
						append_geometry_range_to_feature(
								partitioned_geometry.geometry_range.get(),
								geometry_range_property_name.get(),
								partitioned_feature);
					}
				}
			}
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


GPlatesAppLogic::PartitionFeatureUtils::GenericFeaturePropertyAssigner::GenericFeaturePropertyAssigner(
		const GPlatesModel::FeatureHandle::const_weak_ref &original_feature,
		const AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign,
		bool verify_information_model) :
	d_verify_information_model(verify_information_model),
	d_default_reconstruction_plate_id(
			get_reconstruction_plate_id_from_feature(original_feature)),
	d_default_conjugate_plate_id(
			get_conjugate_plate_id_from_feature(original_feature)),
	d_default_valid_time(
			get_valid_time_from_feature(original_feature)),
	d_feature_property_types_to_assign(feature_property_types_to_assign)
{
}


void
GPlatesAppLogic::PartitionFeatureUtils::GenericFeaturePropertyAssigner::assign_property_values(
		const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
		boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*partitioned_feature->model_ptr());

	// Get the reconstruction plate id.
	// Either from the partitioning feature or the default reconstruction plate id.
	// If we are not supposed to assign plate ids then use the default reconstruction plate id as
	// that has the effect of keeping the original reconstruction plate id.
	boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id;
	if (partitioning_feature &&
		d_feature_property_types_to_assign.test(AssignPlateIds::RECONSTRUCTION_PLATE_ID))
	{
		reconstruction_plate_id = get_reconstruction_plate_id_from_feature(
				partitioning_feature.get());
	}
	else
	{
		reconstruction_plate_id = d_default_reconstruction_plate_id;
	}
	assign_reconstruction_plate_id_to_feature(
			reconstruction_plate_id,
			partitioned_feature,
			d_verify_information_model);

	// Get the conjugate plate id.
	// Either from the partitioning feature or the default conjugate plate id.
	// If we are not supposed to assign plate ids then use the default conjugate plate id as
	// that has the effect of keeping the original conjugate plate id.
	boost::optional<GPlatesModel::integer_plate_id_type> conjugate_plate_id;
	if (partitioning_feature &&
		d_feature_property_types_to_assign.test(AssignPlateIds::CONJUGATE_PLATE_ID))
	{
		conjugate_plate_id = get_conjugate_plate_id_from_feature(
				partitioning_feature.get());
	}
	else
	{
		conjugate_plate_id = d_default_conjugate_plate_id;
	}
	assign_conjugate_plate_id_to_feature(
			conjugate_plate_id,
			partitioned_feature,
			d_verify_information_model);

	// Get the time period.
	// Either from the partitioning feature or the default time period or a mixture of both.
	// If we are not supposed to assign time periods then use the default time period as
	// that has the effect of keeping the original time period.
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time;
	if (partitioning_feature && (
		d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_APPEARANCE) ||
		d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_DISAPPEARANCE)))
	{
		valid_time = get_valid_time_from_feature(partitioning_feature.get());

		if (valid_time)
		{
			// If only copying time of disappearance (not appearance) then replace the appearance time
			// with the default appearance time (or distant past if none).
			if (!d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_APPEARANCE))
			{
				valid_time = GPlatesModel::ModelUtils::create_gml_time_period(
						d_default_valid_time
								? d_default_valid_time.get()->begin()->get_time_position()
								: GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
						valid_time.get()->end()->get_time_position());
			}

			// If only copying time of appearance (not disappearance) then replace the disappearance time
			// with the default disappearance time (or distant future if none).
			if (!d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_DISAPPEARANCE))
			{
				valid_time = GPlatesModel::ModelUtils::create_gml_time_period(
						valid_time.get()->begin()->get_time_position(),
						d_default_valid_time
								? d_default_valid_time.get()->end()->get_time_position()
								: GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
			}
		}
	}
	else
	{
		valid_time = d_default_valid_time;
	}
	assign_valid_time_to_feature(
			valid_time,
			partitioned_feature,
			d_verify_information_model);
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_partitioned_geometry_to_feature(
		const PartitionedFeature::GeometryProperty &geometry_property,
		PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		const double &reconstruction_time)
{
	// Iterate over the partitioning polygons and add the *inside* geometries to features.
	PartitionedFeature::partition_seq_type::const_iterator partition_iter =
			geometry_property.partitioned_inside_geometries.begin();
	PartitionedFeature::partition_seq_type::const_iterator partition_end =
			geometry_property.partitioned_inside_geometries.end();
	for ( ; partition_iter != partition_end; ++partition_iter)
	{
		const PartitionedFeature::Partition &partition = *partition_iter;

		add_partitioned_geometries_to_feature(
				partition.partitioned_geometries,
				geometry_property.domain_property_name,
				geometry_property.range_property_name,
				partitioned_feature_manager,
				reconstruct_method_context,
				reconstruction_time,
				partition.reconstruction_geometry.get());
	}

	// Add partitioned *outside* geometries and to special feature associated with no partition.
	add_partitioned_geometries_to_feature(
			geometry_property.partitioned_outside_geometries,
			geometry_property.domain_property_name,
			geometry_property.range_property_name,
			partitioned_feature_manager,
			reconstruct_method_context,
			reconstruction_time);
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_unpartitioned_geometry_to_feature(
		const PartitionedFeature::GeometryProperty &geometry_property,
		PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		const double &reconstruction_time,
		boost::optional<const ReconstructionGeometry *> partition)
{
	PartitionedFeature::geometry_property_clone_seq_type::const_iterator property_clone_iter =
			geometry_property.property_clones.begin();
	PartitionedFeature::geometry_property_clone_seq_type::const_iterator property_clone_end =
			geometry_property.property_clones.end();
	for ( ; property_clone_iter != property_clone_end; ++property_clone_iter)
	{
		const PartitionedFeature::GeometryPropertyClone &property_clone = *property_clone_iter;

		const GPlatesModel::FeatureHandle::weak_ref feature =
				partitioned_feature_manager.get_feature_for_partition(
						geometry_property.domain_property_name,
						static_cast<bool>(property_clone.range)/*geometry_domain_has_associated_range*/,
						partition);

		// Extract the geometry from the geometry property clone.
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> reconstructed_geometry =
				GeometryUtils::get_geometry_from_property(property_clone.domain);
		if (!reconstructed_geometry)
		{
			// Shouldn't get here since geometry property should contain a geometry.
			continue;
		}

		// Reverse reconstruct to get the present day geometry.
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
				reverse_reconstruct(reconstructed_geometry.get(), feature, reconstruct_method_context, reconstruction_time);

		// Store the present day geometry back in the cloned property.
		GPlatesAppLogic::GeometrySetter geometry_setter(present_day_geometry);
		geometry_setter.set_geometry(property_clone.domain.get());

		// Set the cloned geometry domain property (and optional cloned range property) on the feature.
		const GPlatesModel::FeatureHandle::iterator geometry_domain_feature_iterator =
				feature->add(property_clone.domain);
		if (property_clone.range)
		{
			feature->add(property_clone.range.get());
		}
	}
}


boost::optional<const GPlatesAppLogic::ReconstructionGeometry *>
GPlatesAppLogic::PartitionFeatureUtils::find_partition_containing_most_geometry(
		const PartitionedFeature &partitioned_feature)
{
	const PartitionedFeature::partitioned_geometry_property_map_type &geometry_properties =
			partitioned_feature.partitioned_geometry_properties;

	// Return early if no geometry properties.
	if (geometry_properties.empty())
	{
		return boost::none;
	}

	// Keep track of the various partitions and their size metrics.
	typedef std::map<
			const ReconstructionGeometry *,
			GeometrySizeMetric> partition_size_metrics_type;
	partition_size_metrics_type partition_size_metrics;

	GeometrySizeMetric max_partition_size_metric;
	boost::optional<const ReconstructionGeometry *> max_partition;

	//
	// Iterate over the geometry properties.
	//
	PartitionedFeature::partitioned_geometry_property_map_type::const_iterator geometry_property_iter =
			geometry_properties.begin();
	PartitionedFeature::partitioned_geometry_property_map_type::const_iterator geometry_property_end =
			geometry_properties.end();
	for ( ; geometry_property_iter != geometry_property_end; ++geometry_property_iter)
	{
		const PartitionedFeature::GeometryProperty &geometry_property = geometry_property_iter->second;

		//
		// Iterate over the partitioning polygons and accumulate size metrics.
		//
		PartitionedFeature::partition_seq_type::const_iterator partition_iter =
				geometry_property.partitioned_inside_geometries.begin();
		PartitionedFeature::partition_seq_type::const_iterator partition_end =
				geometry_property.partitioned_inside_geometries.end();
		for ( ; partition_iter != partition_end; ++partition_iter)
		{
			const PartitionedFeature::Partition &partition = *partition_iter;

			// Get the map entry keyed by the partition's partitioning reconstruction geometry.
			GeometrySizeMetric &partition_size_metric =
					partition_size_metrics[partition.reconstruction_geometry.get()];

			// Accumulate the geometry metric for partitioned inside geometries
			// of the current partition.
			partition_size_metric.accumulate(
					calculate_partition_size_metric(partition));

			// If the current partition contains more geometry then
			// set it as the maximum partition so far.
			if (partition_size_metric > max_partition_size_metric)
			{
				max_partition_size_metric = partition_size_metric;
				max_partition = partition.reconstruction_geometry.get();
			}
		}
	}

	return max_partition;
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


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::PartitionFeatureUtils::reverse_reconstruct(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstructed_geometry,
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		const double &reconstruction_time)
{
	if (!feature.is_valid())
	{
		return reconstructed_geometry;
	}

	const ReconstructMethodRegistry reconstruct_method_registry;

	return ReconstructUtils::reconstruct_geometry(
					reconstructed_geometry,
					reconstruct_method_registry,
					feature,
					reconstruction_time,
					reconstruct_method_context,
					true/*reverse_reconstruct*/);
}


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::PartitionFeatureUtils::get_reconstruction_plate_id_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> recon_plate_id =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature_ref,
					get_reconstruction_plate_id_property_name());
	if (!recon_plate_id)
	{
		return boost::none;
	}

	return recon_plate_id.get()->get_value();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_reconstruction_plate_id_to_feature(
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		bool verify_information_model)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_reconstruction_plate_id_property_name());

	// Only assign a new reconstruction plate id if we've been given one.
	if (!reconstruction_plate_id)
	{
		return;
	}

	// Append a new property to the feature.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_reconstruction_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(reconstruction_plate_id.get());
	// If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			get_reconstruction_plate_id_property_name(),
			gpml_reconstruction_plate_id,
			verify_information_model/*check_property_name_allowed_for_feature_type*/);
}


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::PartitionFeatureUtils::get_conjugate_plate_id_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> conjugate_plate_id =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature_ref,
					get_conjugate_plate_id_property_name());
	if (!conjugate_plate_id)
	{
		return boost::none;
	}

	return conjugate_plate_id.get()->get_value();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_conjugate_plate_id_to_feature(
		boost::optional<GPlatesModel::integer_plate_id_type> conjugate_plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		bool verify_information_model)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_conjugate_plate_id_property_name());

	// Only assign a new conjugate plate id if we've been given one.
	if (!conjugate_plate_id)
	{
		return;
	}

	// Append a new property to the feature.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_conjugate_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(conjugate_plate_id.get());
	// If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			get_conjugate_plate_id_property_name(),
			gpml_conjugate_plate_id,
			verify_information_model/*check_property_name_allowed_for_feature_type*/);
}


boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
GPlatesAppLogic::PartitionFeatureUtils::get_valid_time_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> time_period =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					feature_ref,
					get_valid_time_property_name());
	if (!time_period)
	{
		return boost::none;
	}

	return time_period.get();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_valid_time_to_feature(
		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		bool verify_information_model)
{
	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_valid_time_property_name());

	// Only assign a new time period if we've been given one.
	if (!valid_time)
	{
		return;
	}

	// Append a new property to the feature.
	// If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			get_valid_time_property_name(),
			valid_time.get()->clone(),
			verify_information_model/*check_property_name_allowed_for_feature_type*/);
}


void
GPlatesAppLogic::PartitionFeatureUtils::append_geometry_domain_to_feature(
		const geometry_domain_type &geometry_domain,
		const GPlatesModel::PropertyName &geometry_domain_property_name,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	GPlatesModel::PropertyValue::non_null_ptr_type geometry_domain_property =
			GeometryUtils::create_geometry_property_value(geometry_domain);

	// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
	// necessary time-dependent wrapper is added.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			geometry_domain_property_name,
			geometry_domain_property);
}


void
GPlatesAppLogic::PartitionFeatureUtils::append_geometry_range_to_feature(
		const geometry_range_type &geometry_range,
		const GPlatesModel::PropertyName &geometry_range_property_name,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	// Clone to get 'non-const' from 'const'.
	// Might also need cloning if cannot share child revisionable objects across parents?
	std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type> geometry_range_clone;

	geometry_range_type::const_iterator geometry_range_iter = geometry_range.begin();
	geometry_range_type::const_iterator geometry_range_end = geometry_range.end();
	for ( ; geometry_range_iter != geometry_range_end; ++geometry_range_iter)
	{
		geometry_range_clone.push_back((*geometry_range_iter)->clone());
	}

	// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
	// necessary time-dependent wrapper is added.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			geometry_range_property_name,
			GPlatesPropertyValues::GmlDataBlock::create(
					geometry_range_clone.begin(),
					geometry_range_clone.end()));
}


GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::PartitionedFeatureManager(
		const GPlatesModel::FeatureHandle::weak_ref &original_feature,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const boost::shared_ptr<PropertyValueAssigner> &property_value_assigner) :
	d_original_feature(original_feature),
	d_has_original_feature_been_claimed(false),
	d_feature_to_clone_from(original_feature->clone()),
	d_feature_collection(feature_collection),
	d_property_value_assigner(property_value_assigner)
{
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::get_feature_for_partition(
		const GPlatesModel::PropertyName &geometry_domain_property_name,
		bool geometry_domain_has_associated_range,
		boost::optional<const ReconstructionGeometry *> partition)
{
	//
	// Search for an existing feature we can re-use.
	//

	// First iterate over the existing features created for the current partition.
	feature_info_seq_type &feature_infos = d_partitioned_features[partition];
	feature_info_seq_type::iterator feature_infos_iter = feature_infos.begin();
	feature_info_seq_type::iterator feature_infos_end = feature_infos.end();
	for ( ; feature_infos_iter != feature_infos_end; ++feature_infos_iter)
	{
		FeatureInfo &feature_info = *feature_infos_iter;

		// See if the current feature has a geometry property with the same property name
		// that the caller will be adding to the feature.
		const std::pair<feature_contents_type::iterator, bool> feature_contents =
				feature_info.contents.insert(
						feature_contents_type::value_type(
								geometry_domain_property_name,
								geometry_domain_has_associated_range));

		// If the current feature does not have the geometry property name then the caller
		// can add the geometry property without any conflict (in the domain/range mapping).
		if (feature_contents.second/*inserted*/)
		{
			return feature_info.feature;
		}
		// ...else the current feature has a geometry domain property with the same property name.

		// If either:
		//   (1) the caller is adding a range (associated with domain), or
		//   (2) the current feature already has a range (associated with the domain)
		// ...then we should create a new feature to avoid the problem of two domains
		// (with the same property name) having the same number of points and hence there being an
		// ambiguous mapping to the range(s). Note that we could check to see if the number of points
		// are the same, but it's better to keep in separate features anyway since the user could
		// subsequently delete points in one domain leading to the same problem.
		if (!geometry_domain_has_associated_range &&
			!feature_contents.first->second)
		{
			// No prior or subsequent ranges (associated with domain) in the current feature.
			// So we can re-use it since can have any number of geometry domains with the same
			// property name in a single feature without any ambiguity.
			return feature_info.feature;
		}
	}

	// Create a new feature.
	const GPlatesModel::FeatureHandle::weak_ref new_feature = create_feature();

	// Assign property values (from 'partition's feature) to the new feature.
	assign_property_values(new_feature, partition);

	// Record new feature and its contents.
	feature_infos.push_back(FeatureInfo(new_feature));
	FeatureInfo &feature_info = feature_infos.back();
	feature_info.contents.insert(
			feature_contents_type::value_type(
					geometry_domain_property_name,
					geometry_domain_has_associated_range));
	
	return new_feature;
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::create_feature()
{
	if (!d_has_original_feature_been_claimed)
	{
		d_has_original_feature_been_claimed = true;
		return d_original_feature;
	}

	return d_feature_to_clone_from->clone(d_feature_collection);
}


void
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::assign_property_values(
		const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
		boost::optional<const ReconstructionGeometry *> partition)
{
	boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature_opt;

	// If there's a partitioning polygon then get its feature so we can copy
	// property values from it.
	if (partition)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> partitioning_feature =
				ReconstructionGeometryUtils::get_feature_ref(partition.get());
		if (partitioning_feature)
		{
			partitioning_feature_opt = partitioning_feature;
		}
	}

	d_property_value_assigner->assign_property_values(
			partitioned_feature,
			partitioning_feature_opt);
}


GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::GeometrySizeMetric() :
	d_num_points(0),
	d_arc_distance(0),
	d_area(0)
{
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GPlatesMaths::GeometryOnSphere &geometry)
{
	GeometrySize geometry_size(d_num_points, d_arc_distance, d_area);
	geometry.accept_visitor(geometry_size);
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GeometrySizeMetric &geometry_size_metric)
{
	d_num_points += geometry_size_metric.d_num_points;
	d_arc_distance += geometry_size_metric.d_arc_distance;
	d_area += geometry_size_metric.d_area;
}


bool
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::operator<(
		const GeometrySizeMetric &rhs) const
{
	// Area (polygons) first, then arc length (polylines), then the number of points: so a feature
	// with polygons is decided by them (phase4b-design.md section 8.3).
	if (d_area != rhs.d_area)
	{
		return d_area < rhs.d_area;
	}
	if (d_arc_distance != rhs.d_arc_distance)
	{
		return d_arc_distance < rhs.d_arc_distance;
	}
	return d_num_points < rhs.d_num_points;
}
