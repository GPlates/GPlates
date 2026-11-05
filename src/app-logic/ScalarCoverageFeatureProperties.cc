/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <map>
#include <utility>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "ScalarCoverageFeatureProperties.h"

#include "GeometryUtils.h"

#include "maths/PolygonOnSphere.h"

#include "model/FeatureVisitor.h"

#include "property-values/GmlDataBlock.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTimeWindow.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Whether a range sized to a polygon's exterior ring, with no range sized to all its points,
		 * is read as a coverage of the exterior ring alone (see 'Coverage::exterior_ring_only').
		 *
		 * GPlates matched polygon coverages by their exterior rings before GPlates 2.6 and
		 * pyGPlates 1.1, so older files can have such coverages. Switching this off in a later release
		 * means removing 'Coverage::exterior_ring_only' and everything that checks it.
		 *
		 * Limitation: the exterior ring is tried only for a polygon that found no range by all its points.
		 * So if another domain with the same property name has as many points as the polygon has in all
		 * its rings, the first pass finds the two domains ambiguous, and neither gets a coverage (where
		 * older versions, counting only exterior points, read both). Doing better means choosing the
		 * ranges of all the domains together, which is not worth it for a fallback.
		 */
		const bool READ_EXTERIOR_RING_ONLY_COVERAGES = true;


		typedef std::map<GPlatesModel::PropertyName, GPlatesModel::PropertyName>
				coverage_domain_to_range_name_map_type;

		coverage_domain_to_range_name_map_type
		initialise_coverage_domain_to_range_name_mapping()
		{
			coverage_domain_to_range_name_map_type coverage_domain_to_range_name_map;

			// Insert default domain/range names ('gpml:domainSet' and 'gpml:rangeSet').
			coverage_domain_to_range_name_map.insert(
					ScalarCoverageFeatureProperties::get_default_domain_range_property_names());

			coverage_domain_to_range_name_map.insert(
					std::make_pair(
							GPlatesModel::PropertyName::create_gpml("boundary"),
							GPlatesModel::PropertyName::create_gpml("boundaryCoverage")));
			coverage_domain_to_range_name_map.insert(
					std::make_pair(
							GPlatesModel::PropertyName::create_gpml("centerLineOf"),
							GPlatesModel::PropertyName::create_gpml("centerLineOfCoverage")));
			coverage_domain_to_range_name_map.insert(
					std::make_pair(
							GPlatesModel::PropertyName::create_gpml("meshPoints"),
							GPlatesModel::PropertyName::create_gpml("meshPointsCoverage")));
			coverage_domain_to_range_name_map.insert(
					std::make_pair(
							GPlatesModel::PropertyName::create_gpml("multiPosition"),
							GPlatesModel::PropertyName::create_gpml("multiPositionCoverage")));
			coverage_domain_to_range_name_map.insert(
					std::make_pair(
							GPlatesModel::PropertyName::create_gpml("outlineOf"),
							GPlatesModel::PropertyName::create_gpml("outlineOfCoverage")));
			coverage_domain_to_range_name_map.insert(
					std::make_pair(
							GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
							GPlatesModel::PropertyName::create_gpml("unclassifiedGeometryCoverage")));

			return coverage_domain_to_range_name_map;
		}


		const coverage_domain_to_range_name_map_type &
		get_coverage_domain_to_range_name_mapping()
		{
			static const coverage_domain_to_range_name_map_type s_map =
					initialise_coverage_domain_to_range_name_mapping();

			return s_map;
		}


		/**
		 * Visits a scalar coverage feature and extracts domain/range coverages from it.
		 *
		 * The heuristic that we're using here is that it is a scalar coverage feature if there is:
		 *  - A geometry property and a GmlDataBlock property with property names that match
		 *   a list of predefined property names (eg, 'gpml:domainSet'/'gpml:rangeSet').
		 *
		 * NOTE: The coverages are extracted at the specified reconstruction time.
		 */
		template <class FeatureHandleType> // Could be either const or non-const FeatureHandle.
		class ExtractScalarCoverageFeatureProperties :
				public GPlatesModel::FeatureVisitorBase<FeatureHandleType>
		{
		public:

			typedef GPlatesModel::FeatureVisitorBase<FeatureHandleType> feature_visitor_type;

			struct Domain
			{
				Domain(
						typename feature_visitor_type::feature_iterator_type property_,
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_) :
					property(property_),
					geometry(geometry_)
				{  }

				typename feature_visitor_type::feature_iterator_type property;
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry;
			};

			struct Range
			{
				Range(
						typename feature_visitor_type::feature_iterator_type property_,
						const std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> &scalar_data_) :
					property(property_),
					scalar_data(scalar_data_)
				{  }

				typename feature_visitor_type::feature_iterator_type property;
				std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type> scalar_data;
			};

			struct Coverage
			{
				Coverage(
						const Domain &domain_,
						const Range &range_,
						bool exterior_ring_only_) :
					domain(domain_),
					range(range_),
					exterior_ring_only(exterior_ring_only_)
				{  }

				Domain domain;
				Range range;

				//! See 'ScalarCoverageFeatureProperties::Coverage::exterior_ring_only'.
				bool exterior_ring_only;
			};


			explicit
			ExtractScalarCoverageFeatureProperties(
					const double &reconstruction_time = 0) :
				d_reconstruction_time(reconstruction_time)
			{  }


			const std::vector<Coverage> &
			get_coverages() const
			{
				return d_coverages;
			}


			virtual
			bool
			initialise_pre_feature_properties(
					typename feature_visitor_type::feature_handle_type &feature_handle)
			{
				d_domains.clear();
				d_ranges.clear();

				return true;
			}


			virtual
			void
			finalise_post_feature_properties(
					typename feature_visitor_type::feature_handle_type &feature_handle)
			{
				if (d_domains.empty() ||
					d_ranges.empty())
				{
					return;
				}

				// Copy the ranges so we can erase from them as we find coverages.
				std::vector<Range> ranges(d_ranges);

				// First match each domain by all its points (a polygon's interior rings included).
				std::vector<const Domain *> domains;
				for (const Domain &domain : d_domains)
				{
					domains.push_back(&domain);
				}
				std::vector<const Domain *> unmatched_domains;
				match_domains(feature_handle, domains, ranges, false/*exterior_ring_only*/, unmatched_domains);

				if (!READ_EXTERIOR_RING_ONLY_COVERAGES)
				{
					return;
				}

				// Then match the polygons with interior rings left over by their exterior rings alone, as
				// older files have them. Doing this second means it cannot take a range that an
				// all-points match needed.
				std::vector<const Domain *> polygons_with_interior_rings;
				for (const Domain *domain : unmatched_domains)
				{
					if (GeometryUtils::get_num_geometry_exterior_points(*domain->geometry) !=
						GeometryUtils::get_num_geometry_points(*domain->geometry))
					{
						polygons_with_interior_rings.push_back(domain);
					}
				}
				std::vector<const Domain *> still_unmatched_domains;
				match_domains(
						feature_handle,
						polygons_with_interior_rings,
						ranges,
						true/*exterior_ring_only*/,
						still_unmatched_domains);
			}


			virtual
			void
			visit_gpml_constant_value(
					typename feature_visitor_type::gpml_constant_value_type &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			virtual
			void
			visit_gpml_piecewise_aggregation(
					typename feature_visitor_type::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
			{
				// Note: We're avoiding declaring iterators over time windows since they can be const or
				// non-const depending on whether this class is instantiated with const or non-const FeatureHandle.
				const std::size_t num_time_windows = gpml_piecewise_aggregation.time_windows().size();
				for (std::size_t time_window_index = 0; time_window_index < num_time_windows; ++time_window_index)
				{
					const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
							gpml_piecewise_aggregation.time_windows()[time_window_index]->valid_time();

					// If the time window period contains the current reconstruction time then visit.
					// The time periods should be mutually exclusive - if we happen to be in
					// two time periods then we're probably right on the boundary between the two
					// and then it doesn't really matter which one we choose.
					if (time_period->contains(d_reconstruction_time))
					{
						gpml_piecewise_aggregation.time_windows()[time_window_index]->time_dependent_value()
								->accept_visitor(*this);
					}
				}
			}


			virtual
			void
			visit_gml_data_block(
					typename feature_visitor_type::gml_data_block_type &gml_data_block)
			{
				const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &
						tuple_list = gml_data_block.tuple_list();

				d_ranges.push_back(
						Range(
								this->current_top_level_propiter().get(),
								std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type>(
										tuple_list.begin(), tuple_list.end())));
			}


			virtual
			void
			visit_gml_line_string(
					typename feature_visitor_type::gml_line_string_type &gml_line_string)
			{
				d_domains.push_back(
						Domain(this->current_top_level_propiter().get(), gml_line_string.get_polyline()));
			}


			virtual
			void
			visit_gml_multi_point(
					typename feature_visitor_type::gml_multi_point_type &gml_multi_point)
			{
				d_domains.push_back(
						Domain(this->current_top_level_propiter().get(), gml_multi_point.get_multipoint()));
			}


			virtual
			void
			visit_gml_orientable_curve(
					typename feature_visitor_type::gml_orientable_curve_type &gml_orientable_curve)
			{
				gml_orientable_curve.base_curve()->accept_visitor(*this);
			}


			virtual
			void
			visit_gml_point(
					typename feature_visitor_type::gml_point_type &gml_point)
			{
				d_domains.push_back(
						Domain(this->current_top_level_propiter().get(), gml_point.get_point().get_geometry_on_sphere()));
			}


			virtual
			void
			visit_gml_polygon(
					typename feature_visitor_type::gml_polygon_type &gml_polygon)
			{
				d_domains.push_back(
						Domain(this->current_top_level_propiter().get(), gml_polygon.get_polygon()));
			}

		private:

			static
			unsigned int
			get_num_domain_points(
					const GPlatesMaths::GeometryOnSphere &domain_geometry,
					bool exterior_ring_only)
			{
				return exterior_ring_only
						? GeometryUtils::get_num_geometry_exterior_points(domain_geometry)
						: GeometryUtils::get_num_geometry_points(domain_geometry);
			}


			/**
			 * Match each of @a domains, in order, to the range of its coverage, which is the range with
			 * the associated property name and one scalar per domain point (counting only a polygon's
			 * exterior ring if @a exterior_ring_only).
			 *
			 * Matched ranges are erased from @a ranges. Domains with a range property name but no range
			 * of the right size are appended to @a unmatched_domains. An ambiguous match (more than one
			 * matching range, or more than one matching domain) is not unmatched: it is warned about and
			 * creates no coverage.
			 */
			void
			match_domains(
					typename feature_visitor_type::feature_handle_type &feature_handle,
					const std::vector<const Domain *> &domains,
					std::vector<Range> &ranges,
					bool exterior_ring_only,
					std::vector<const Domain *> &unmatched_domains)
			{
				const unsigned int num_domains = domains.size();
				for (unsigned int domain_index = 0; domain_index < num_domains; ++domain_index)
				{
					const Domain &domain = *domains[domain_index];

					const GPlatesModel::PropertyName &domain_property_name =
							(*domain.property)->get_property_name();

					// Look for a range name associated with the current domain name.
					boost::optional<GPlatesModel::PropertyName> range_property_name_opt =
							ScalarCoverageFeatureProperties::get_range_property_name_from_domain(
									domain_property_name);
					if (!range_property_name_opt)
					{
						// Geometry property name is not associated with any coverage range.
						continue;
					}
					const GPlatesModel::PropertyName &range_property_name = range_property_name_opt.get();

					const unsigned int num_domain_geometry_points =
							get_num_domain_points(*domain.geometry, exterior_ring_only);

					// Search the ranges found for the matching range property name.
					boost::optional<Range> matching_range;
					bool more_than_one_matching_range = false;
					typename std::vector<Range>::iterator ranges_iter = ranges.begin();
					while (ranges_iter != ranges.end())
					{
						const Range &range = *ranges_iter;

						if ((*range.property)->get_property_name() == range_property_name)
						{
							// See if the number of scalars matches the number of points in the domain geometry.
							if (!range.scalar_data.empty() &&
								range.scalar_data.front()->get_coordinates().size() == num_domain_geometry_points)
							{
								if (matching_range)
								{
									// Already (previously) found a matching range.
									more_than_one_matching_range = true;
								}
								else
								{
									matching_range = range;
								}

								// Remove range so we can't use it again for another domain.
								ranges_iter = ranges.erase(ranges_iter);
								continue;
							}
						}

						++ranges_iter;
					}

					if (!matching_range)
					{
						unmatched_domains.push_back(&domain);
						continue;
					}

					// If found more than one matching range with same number of scalars then it's ambiguous as to
					// which one we should use - so we skip the current domain (without creating a coverage for it).
					if (more_than_one_matching_range)
					{
						qWarning()
							<< "Ambiguous"
							<< convert_qualified_xml_name_to_qstring(range_property_name)
							<< "coverage range for feature-id"
							<< feature_handle.feature_id().get().qstring()
							<< "- more than one matching coverage range with same number of scalars - ignoring all matches.";
						continue;
					}

					// Search the domains we haven't visited yet to make sure there is only one
					// domain that matches the range.
					unsigned int remaining_domain_index = domain_index + 1;
					for ( ; remaining_domain_index < num_domains; ++remaining_domain_index)
					{
						const Domain &remaining_domain = *domains[remaining_domain_index];

						if ((*remaining_domain.property)->get_property_name() == domain_property_name)
						{
							// See if the number of geometry points matches.
							if (num_domain_geometry_points ==
								get_num_domain_points(*remaining_domain.geometry, exterior_ring_only))
							{
								break;
							}
						}
					}

					if (remaining_domain_index != num_domains)
					{
						// Found another domain with same property name and number of points...
						qWarning()
							<< "Ambiguous"
							<< convert_qualified_xml_name_to_qstring(domain_property_name)
							<< "coverage domain for feature-id"
							<< feature_handle.feature_id().get().qstring()
							<< "- more than one matching coverage domain with same number of points - ignoring all matches.";
						continue;
					}

					d_coverages.push_back(
							Coverage(domain, matching_range.get(), exterior_ring_only));
				}
			}


			/**
			 * The reconstruction time at which properties are extracted.
			 */
			GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time;

			std::vector<Domain> d_domains;
			std::vector<Range> d_ranges;

			std::vector<Coverage> d_coverages;
		};
	}
}


bool
GPlatesAppLogic::ScalarCoverageFeatureProperties::is_scalar_coverage_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	ExtractScalarCoverageFeatureProperties<const GPlatesModel::FeatureHandle> visitor;
	visitor.visit_feature(feature);
	return !visitor.get_coverages().empty();
}


bool
GPlatesAppLogic::ScalarCoverageFeatureProperties::contains_scalar_coverage_feature(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	ExtractScalarCoverageFeatureProperties<const GPlatesModel::FeatureHandle> visitor;
	for (GPlatesModel::FeatureCollectionHandle::const_iterator iter = feature_collection->begin();
			iter != feature_collection->end(); ++iter)
	{
		visitor.visit_feature(iter);
		if (!visitor.get_coverages().empty())
		{
			return true;
		}
	}

	return false;
}


boost::optional<GPlatesModel::PropertyName>
GPlatesAppLogic::ScalarCoverageFeatureProperties::get_range_property_name_from_domain(
		const GPlatesModel::PropertyName &domain_property_name)
{
	const coverage_domain_to_range_name_map_type &coverage_domain_to_range_name_mapping =
			get_coverage_domain_to_range_name_mapping();

	// Look for a range name associated with the domain name.
	coverage_domain_to_range_name_map_type::const_iterator range_property_name_iter =
			coverage_domain_to_range_name_mapping.find(domain_property_name);
	if (range_property_name_iter == coverage_domain_to_range_name_mapping.end())
	{
		// Domain property name is not associated with any coverage range.
		return boost::none;
	}

	return range_property_name_iter->second;
}


std::pair<GPlatesModel::PropertyName/*domain*/, GPlatesModel::PropertyName/*range*/>
GPlatesAppLogic::ScalarCoverageFeatureProperties::get_default_domain_range_property_names()
{
	return std::make_pair(
			GPlatesModel::PropertyName::create_gpml("domainSet"),
			GPlatesModel::PropertyName::create_gpml("rangeSet"));
}


bool
GPlatesAppLogic::ScalarCoverageFeatureProperties::get_coverages(
		std::vector<Coverage> &coverages,
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const double &reconstruction_time)
{
	typedef ExtractScalarCoverageFeatureProperties<GPlatesModel::FeatureHandle>
			extract_scalar_coverage_feature_properties_type;

	extract_scalar_coverage_feature_properties_type visitor(reconstruction_time);
	visitor.visit_feature(feature);

	const std::vector<extract_scalar_coverage_feature_properties_type::Coverage> &
			feature_coverages = visitor.get_coverages();
	if (feature_coverages.empty())
	{
		return false;
	}

	BOOST_FOREACH(
			const extract_scalar_coverage_feature_properties_type::Coverage &feature_coverage,
			feature_coverages)
	{
		coverages.push_back(
				Coverage(
						feature_coverage.domain.property,
						feature_coverage.range.property,
						feature_coverage.exterior_ring_only
								? get_exterior_ring_domain(*feature_coverage.domain.geometry)
								: feature_coverage.domain.geometry,
						feature_coverage.range.scalar_data,
						feature_coverage.exterior_ring_only));
	}

	return true;
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ScalarCoverageFeatureProperties::get_exterior_ring_domain(
		const GPlatesMaths::GeometryOnSphere &domain_geometry)
{
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon =
			GeometryUtils::get_polygon_on_sphere(domain_geometry);
	if (!polygon)
	{
		// Only a polygon has rings other than its exterior.
		return domain_geometry.get_non_null_pointer();
	}

	if (polygon.get()->number_of_interior_rings() == 0)
	{
		return polygon.get();
	}

	return GPlatesMaths::PolygonOnSphere::create(
			polygon.get()->exterior_ring_vertex_begin(),
			polygon.get()->exterior_ring_vertex_end());
}
