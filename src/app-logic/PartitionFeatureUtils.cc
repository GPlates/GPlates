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
#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>
#include <QString>

#include "PartitionFeatureUtils.h"

#include "GeometryUtils.h"
#include "PartitionFeatureTask.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ReconstructMethodRegistry.h"
#include "ScalarCoverageFeatureProperties.h"

#include "feature-visitors/GeometrySetter.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "maths/AngularExtent.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"
#include "maths/GeometryDistance.h"
#include "maths/MathsUtils.h"

#include "model/Gpgim.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/PropertyName.h"
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
					add_geometry(gml_point.get_point());
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


				/**
				 * Distance threshold used when determining interpolated scalar values for points in
				 * partitioned geometries that don't correspond to any point in original geometry.
				 */
				static const GPlatesMaths::AngularExtent POLY_GEOMETRY_DISTANCE_THRESHOLD;


				/**
				 * Typedef for mapping points in the geometry domain to indices into geometry domain/range.
				 *
				 * NOTE: Since 'GPlatesMaths::PointOnSphereMapPredicate' uses an epsilon test it's
				 * possible that two points close enough together will map to the same map entry.
				 * This means we lose a mapping to one of the point's associated range.
				 * TODO: We should build in a mapping into the geometry partitioning process itself
				 * (eg, something similar to DateLineWrapper which provides information mapping the
				 * points in the wrapped/clipped geometries back to the original unwrapped geometries).
				 */
				typedef std::map<
						GPlatesMaths::PointOnSphere,
						unsigned int/*point index*/,
						GPlatesMaths::PointOnSphereMapPredicate>
								domain_to_range_map_type;

				//! Contains the geometry range and information to map the associated domain to this range.
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

						const unsigned int num_domain_points = GeometryUtils::get_num_geometry_exterior_points(*domain_);
						for (unsigned int s = 0; s < range_.size(); ++s)
						{
							if (num_domain_points != range_[s]->get_coordinates().size())
							{
								return false;
							}
						}

						return true;
					}

					Range(
							const geometry_domain_type &domain_,
							const geometry_range_type &range_) :
						domain_type(GPlatesMaths::GeometryType::NONE),
						range(range_)
					{
						// Get the geometry domain points.
						// We're getting the *exterior* points because that's what the scalar coverage
						// extraction code currently does.
						//
						// TODO: Support polygons with interior holes (ie, allow scalar values on
						// the points in the polygon's interior rings).
						domain_type = GeometryUtils::get_geometry_exterior_points(*domain_, domain_points);

						// Map the geometry domain points to their indices into geometry domain/range.
						const unsigned int num_domain_points = domain_points.size();
						for (unsigned int n = 0; n < num_domain_points; ++n)
						{
							domain_to_range_map.insert(std::make_pair(domain_points[n], n));
						}
					}

					GPlatesMaths::GeometryType::Value domain_type;
					std::vector<GPlatesMaths::PointOnSphere> domain_points;
					domain_to_range_map_type domain_to_range_map;
					geometry_range_type range;
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
							if (Range::range_matches_domain(geometry_domain, coverage.range))
							{
								geometry_range = boost::in_place(geometry_domain, coverage.range);
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
					// Partition the current geometry property and store results.
					GeometryCookieCutter::partition_seq_type partitioned_inside_domains;
					GeometryCookieCutter::partitioned_geometry_seq_type partitioned_outside_domains;
					d_cookie_cut_geometry.partition_geometry(
							geometry_domain,
							partitioned_inside_domains,
							partitioned_outside_domains);

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
								geometry_domain,
								geometry_range,
								partitioned_inside_domain.partitioned_geometries,
								partition.partitioned_geometries);
					}

					// Add the partitioned *outside* geometries.
					partition_geometries(
							geometry_domain,
							geometry_range,
							partitioned_outside_domains,
							partitioned_geometry_property.partitioned_outside_geometries);
				}

				void
				partition_geometries(
						const geometry_domain_type &geometry_domain,
						const boost::optional<Range> &geometry_range,
						const GeometryCookieCutter::partitioned_geometry_seq_type &partitioned_domains,
						PartitionedFeature::partitioned_geometry_seq_type &partitioned_geometries)
				{
					GeometryCookieCutter::partitioned_geometry_seq_type::const_iterator
							partitioned_domains_iter = partitioned_domains.begin();
					GeometryCookieCutter::partitioned_geometry_seq_type::const_iterator
							partitioned_domains_end = partitioned_domains.end();
					for ( ; partitioned_domains_iter != partitioned_domains_end; ++partitioned_domains_iter)
					{
						const geometry_domain_type &partitioned_domain = *partitioned_domains_iter;

						// If there is a geometry range associated with the geometry domain then
						// create a partitioned range associated with the partitioned domain.
						boost::optional<geometry_range_type> partitioned_range;
						if (geometry_range)
						{
							partitioned_range = geometry_range_type();
							partition_range(partitioned_range.get(), partitioned_domain, geometry_range.get(), geometry_domain);
						}

						partitioned_geometries.push_back(
								PartitionedFeature::PartitionedGeometry(partitioned_domain, partitioned_range));
					}
				}

				void
				partition_range(
						geometry_range_type &partitioned_range,
						const geometry_domain_type &partitioned_domain,
						const Range &geometry_range,
						const geometry_domain_type &geometry_domain)
				{
					// Get the partitioned domain points.
					std::vector<GPlatesMaths::PointOnSphere> partitioned_domain_points;
					GeometryUtils::get_geometry_exterior_points(*partitioned_domain, partitioned_domain_points);

					const unsigned int num_partitioned_domain_points = partitioned_domain_points.size();

					std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type> partitioned_range_coordinates;

					// Allocated memory for partitioned range.
					const unsigned int range_tuple_size = geometry_range.range.size();
					partitioned_range_coordinates.resize(range_tuple_size);
					for (unsigned int t = 0; t < range_tuple_size; ++t)
					{
						partitioned_range_coordinates[t].reserve(num_partitioned_domain_points);
					}

					// Map the geometry domain points to their indices into geometry domain/range and
					// copy the associated range scalars into the partitioned range.
					for (unsigned int n = 0; n < num_partitioned_domain_points; ++n)
					{
						const GPlatesMaths::PointOnSphere &partitioned_domain_point = partitioned_domain_points[n];

						domain_to_range_map_type::const_iterator iter =
								geometry_range.domain_to_range_map.find(partitioned_domain_point);
						if (iter != geometry_range.domain_to_range_map.end())
						{
							// Look up the range scalar values in the original, unpartitioned range
							// and add them to the partitioned range.
							const unsigned int range_scalar_index = iter->second;
							for (unsigned int t = 0; t < range_tuple_size; ++t)
							{
								partitioned_range_coordinates[t].push_back(
										geometry_range.range[t]->get_coordinates()[range_scalar_index]);
							}
						}
						else
						{
							// Partitioned domain point not found in original, unpartitioned domain geometry.
							// This most likely happens where a polyline or polygon intersected the
							// partitioning polygon (note that this shouldn't happen for a point or multi-point
							// geometry since partitioning those types does not generate any new points).
							// So we'll find the segment that the intersection point lies on and use that to
							// interpolate the scalar values of that segment's end points.
							unsigned int closest_point_index; // To be ignored - will also be zero.
							unsigned int closest_domain_index; // Should be segment index into polyline/polygon.
							// Since the current implementation of the partitioner generates polylines
							// even when a polygon is partitioned (against a partitioning polygon)
							// we know that all intersection points should lie *on* the partitioned polylines.
							// Hence we can speed up the minimum distance test by using an arbitrarily
							// small threshold since the minimum distance should theoretically be zero.
							// We'll back it up with a slower non-threshold test just to be sure though.
							//
							// TODO: This needs to be changed once the partitioner is properly implemented
							// to generate partitioned *polygons* instead of polylines. When that happens
							// we can get partitioned points that don't fall on the original polygon.
							// In this case we really should get the partitioner itself to generate
							// interpolate information similar to what DateLineWrapper does.
							if (GPlatesMaths::AngularExtent::PI == minimum_distance(
									partitioned_domain_point,
									*geometry_domain,
									false/*geometry1_interior_is_solid*/,
									false/*geometry2_interior_is_solid*/,
									POLY_GEOMETRY_DISTANCE_THRESHOLD,
									boost::none/*closest_positions*/,
									boost::make_tuple(boost::ref(closest_point_index), boost::ref(closest_domain_index))))
							{
								// The minimum distance exceeded our threshold. This shouldn't happen
								// but if it does then we'll do the test again without a threshold.
								minimum_distance(
										partitioned_domain_point,
										*geometry_domain,
										false/*geometry1_interior_is_solid*/,
										false/*geometry2_interior_is_solid*/,
										boost::none/*minimum_distance_threshold*/,
										boost::none/*closest_positions*/,
										boost::make_tuple(boost::ref(closest_point_index), boost::ref(closest_domain_index)));
							}

							if (geometry_range.domain_type == GPlatesMaths::GeometryType::POLYGON ||
								geometry_range.domain_type == GPlatesMaths::GeometryType::POLYLINE)
							{
								const unsigned int closest_segment_index = closest_domain_index;

								// Calculate the interpolation ratio of the point along the great circle arc of the segment.
								const GPlatesMaths::PointOnSphere &segment_start_point = geometry_range.domain_points[closest_segment_index];
								const GPlatesMaths::PointOnSphere &segment_end_point = geometry_range.domain_points[closest_segment_index + 1];
								const GPlatesMaths::AngularDistance segment_len = minimum_distance(segment_start_point, segment_end_point);
								if (segment_len != GPlatesMaths::AngularDistance::ZERO)
								{
									const double interpolate_ratio =
											minimum_distance(segment_start_point, partitioned_domain_point).calculate_angle().dval() /
											segment_len.calculate_angle().dval();

									// Interpolate the scalar values of segment's end points.
									const unsigned int range_scalar_start_index = closest_segment_index;
									for (unsigned int t = 0; t < range_tuple_size; ++t)
									{
										const GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type &
												range_scalars = geometry_range.range[t]->get_coordinates();
										const double interpolated_scalar =
												(1.0 - interpolate_ratio) * range_scalars[range_scalar_start_index] +
													interpolate_ratio * range_scalars[range_scalar_start_index + 1];

										partitioned_range_coordinates[t].push_back(interpolated_scalar);
									}
								}
								else // zero length segment...
								{
									// Both end points of the segment are the same (within numerical tolerance)
									// so just pick the segment start point.
									const unsigned int range_scalar_index = closest_segment_index;
									for (unsigned int t = 0; t < range_tuple_size; ++t)
									{
										partitioned_range_coordinates[t].push_back(
												geometry_range.range[t]->get_coordinates()[range_scalar_index]);
									}
								}
							}
							else if (geometry_range.domain_type == GPlatesMaths::GeometryType::MULTIPOINT)
							{
								// We shouldn't be able to get here but if we do then we'll just
								// use the scalar value of the closest point.
								// For multipoints the closest index is a point index into multipoint.
								const unsigned int range_scalar_index = closest_domain_index;
								for (unsigned int t = 0; t < range_tuple_size; ++t)
								{
									partitioned_range_coordinates[t].push_back(
											geometry_range.range[t]->get_coordinates()[range_scalar_index]);
								}
							}
							else // geometry_range.domain_type == GPlatesMaths::GeometryType::POINT
							{
								for (unsigned int t = 0; t < range_tuple_size; ++t)
								{
									partitioned_range_coordinates[t].push_back(
											geometry_range.range[t]->get_coordinates()[0/*range_scalar_index*/]);
								}
							}
						}
					}

					// Create partitioned GmlDataBlockCoordinateList's.
					for (unsigned int t = 0; t < range_tuple_size; ++t)
					{
						const GPlatesPropertyValues::GmlDataBlockCoordinateList &range_tuple_element = *geometry_range.range[t];

						partitioned_range.push_back(
								GPlatesPropertyValues::GmlDataBlockCoordinateList::create(
										range_tuple_element.get_value_object_type(),
										range_tuple_element.get_value_object_xml_attributes(),
										partitioned_range_coordinates[t]));
					}
				}
			};

			const GPlatesMaths::AngularExtent PartitionFeatureGeometryProperties::POLY_GEOMETRY_DISTANCE_THRESHOLD =
					GPlatesMaths::AngularExtent::create_from_angle(GPlatesMaths::convert_deg_to_rad(0.5));


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
					distance += acos(gca_iter->dot_of_endpoints());
				}

				return distance;
			}


			class GeometrySize :
					public GPlatesMaths::ConstGeometryOnSphereVisitor
			{
			public:
				GeometrySize(
						unsigned int &num_points,
						GPlatesMaths::real_t &arc_distance,
						bool &using_arc_distance) :
					d_num_points(num_points),
					d_arc_distance(arc_distance),
					d_using_arc_distance(using_arc_distance)
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
						GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
				{
					++d_num_points;
				}

				virtual
				void
				visit_polygon_on_sphere(
						GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
				{
					d_arc_distance += polygon_on_sphere->get_arc_length();
					d_using_arc_distance = true;
				}

				virtual
				void
				visit_polyline_on_sphere(
						GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
				{
					d_arc_distance += polyline_on_sphere->get_arc_length();
					d_using_arc_distance = true;
				}

				unsigned int &d_num_points;
				GPlatesMaths::real_t &d_arc_distance;
				bool &d_using_arc_distance;
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
		GPlatesFeatureVisitors::GeometrySetter geometry_setter(present_day_geometry);
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
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
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
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
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
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
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
	d_using_arc_distance(false)
{
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GPlatesMaths::GeometryOnSphere &geometry)
{
	GeometrySize geometry_size(d_num_points, d_arc_distance, d_using_arc_distance);
	geometry.accept_visitor(geometry_size);
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GeometrySizeMetric &geometry_size_metric)
{
	d_num_points += geometry_size_metric.d_num_points;
	d_arc_distance += geometry_size_metric.d_arc_distance;

	if (geometry_size_metric.d_using_arc_distance)
	{
		d_using_arc_distance = true;
	}
}


bool
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::operator<(
		const GeometrySizeMetric &rhs) const
{
	// Prefer to compare arc distance if we have visited any
	// line geometry.
	if (d_using_arc_distance || rhs.d_using_arc_distance)
	{
		return d_arc_distance < rhs.d_arc_distance;
	}

	return d_num_points < rhs.d_num_points;
}
