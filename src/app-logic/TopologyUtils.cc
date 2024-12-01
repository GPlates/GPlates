/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2013 California Institute of Technology
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
#include <cstddef> // For std::size_t
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "AppLogicUtils.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalGeometrySubSegment.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolver.h"
#include "TopologyInternalUtils.h"
#include "TopologyNetworkResolver.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/PointOnSphere.h"
#include "maths/Real.h"

#include "model/FeatureHandle.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalLine.h"

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


namespace GPlatesAppLogic
{
	namespace
	{
		//
		// The following structures, typedefs and functions are used in 'find_resolved_topological_sections()'.
		//


		// Associates a resolved topological sub-segment with its owning resolved topology.
		struct ResolvedSubSegmentInfo
		{
			ResolvedSubSegmentInfo(
					const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment_,
					const ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topology_) :
				sub_segment(sub_segment_),
				resolved_topology(resolved_topology_)
			{  }

			ResolvedTopologicalGeometrySubSegment::non_null_ptr_type sub_segment;
			// The resolved topology that owns the sub-segment...
			ReconstructionGeometry::non_null_ptr_to_const_type resolved_topology;
		};

		// Type used to compare reconstruction geometries.
		//
		// Note: We don't actually compare ReconstructionGeometry pointers because two adjacent topologies
		// may reference different ReconstructionGeometry objects associated with the same topological section
		// (since different topological layers may each reconstruct the same section).
		// Instead we compare the topological section's feature reference and geometry property iterator since
		// they should be the same (regardless of how many times the same section is reconstructed).
		typedef std::pair<GPlatesModel::FeatureHandle::const_weak_ref, GPlatesModel::FeatureHandle::const_iterator>
				topological_section_compare_type;

		// Map of each topological section to all the resolved topologies that use it for a sub-segment.
		typedef std::map<topological_section_compare_type, std::vector<ResolvedSubSegmentInfo> >
				resolved_section_to_sharing_resolved_topologies_map_type;


		/**
		 * Maps each resolved topological section to all the resolved topologies that use it for a sub-segment.
		 */
		void
		map_resolved_topological_sections_to_resolved_topologies(
				resolved_section_to_sharing_resolved_topologies_map_type &resolved_section_to_sharing_resolved_topologies_map,
				const ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topology,
				const sub_segment_seq_type &section_sub_segments)
		{
			// Iterate over the sub-segments of the current topology.
			sub_segment_seq_type::const_iterator sub_segments_iter = section_sub_segments.begin();
			sub_segment_seq_type::const_iterator sub_segments_end = section_sub_segments.end();
			for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
			{
				const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segments_iter;

				//
				// NOTE: We do not get the sub-sub-segments of the current sub-segment.
				//
				// Currently the code in this module assumes that, when there is both a start and an end rubber band, the
				// start rubber band is at the *start* and the end rubber band is at the *end* (in which case the entire
				// section geometry also contributes to the sub-segment). This is always the case with sub-segments
				// (due to intersecting topological section geometries).
				//
				// However, if we were to include sub-sub-segments (of those sub-segments) then we'd need to update our handling
				// of rubber bands since it would then be possible to have both rubber bands at the start or both at the end
				// (in which case the section geometry is excluded from the sub-segment). This represents just a part of the
				// start (or end) rubber-band segment which usually only happens when a (resolved line) sub-segment is split
				// into its component sub-sub-segments (in which case any arbitrary part of a sub-sub-segment could contribute
				// to its parent sub-segment).
				//

				// Get the geometry property.
				boost::optional<GPlatesModel::FeatureHandle::iterator> section_geometry_property =
						ReconstructionGeometryUtils::get_geometry_property_iterator(
								sub_segment->get_reconstruction_geometry());
				if (section_geometry_property)  // This should always succeed.
				{
					const GPlatesModel::FeatureHandle::const_weak_ref section_feature_ref = sub_segment->get_feature_ref();

					// Add the current resolved topology to the list of those sharing the current section.
					std::vector<ResolvedSubSegmentInfo> &sub_segment_infos =
							resolved_section_to_sharing_resolved_topologies_map[
									topological_section_compare_type(
											section_feature_ref,
											section_geometry_property.get())];
					sub_segment_infos.push_back(ResolvedSubSegmentInfo(sub_segment, resolved_topology));
				}
			}
		}


		/**
		 * Convert a section reconstruction geometry to a pair containing section feature and geometry property iterator.
		 *
		 * The returned object can be used to compare sections instead of comparing reconstruction geometry pointers
		 * (see comment for 'topological_section_compare_type').
		 */
		boost::optional<topological_section_compare_type>
		get_topological_section_compare(
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_reconstruction_geometry)
		{
			// Get the feature ref and geometry property.
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> section_feature_ref =
					ReconstructionGeometryUtils::get_feature_ref(
							section_reconstruction_geometry.get());
			boost::optional<GPlatesModel::FeatureHandle::iterator> section_geometry_property =
					ReconstructionGeometryUtils::get_geometry_property_iterator(
							section_reconstruction_geometry.get());
			if (section_feature_ref &&
				section_geometry_property)  // This should always succeed.
			{
				return  topological_section_compare_type(
						section_feature_ref.get(),
						section_geometry_property.get());
			}

			return boost::none;
		}


		/**
		 * The start or end of a sub-segment within the section geometry.
		 *
		 * For point and multi-point sections there are no intersections, and so the sub-segments are
		 * always the entire section.
		 *
		 * For section polylines there can be optional intersections (which can be on a polyline vertex
		 * or in the middle of a segment/arc).
		 * Note that polygons have already had their exterior rings converted to polylines.
		 */
		struct ResolvedSubSegmentMarker
		{
			ResolvedSubSegmentMarker(
					const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo &resolved_topology_info_,
					unsigned int num_vertices_in_section_,
					const boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> &intersection_or_rubber_band_,
					bool is_start_of_section_,
					bool is_start_of_sub_segment_) :
				resolved_topology_info(resolved_topology_info_),
				num_vertices_in_section(num_vertices_in_section_),
				intersection_or_rubber_band(intersection_or_rubber_band_),
				is_start_of_section(is_start_of_section_),
				is_start_of_sub_segment(is_start_of_sub_segment_)
			{  }

			bool
			is_start_rubber_band() const
			{
				return is_start_of_section &&
						intersection_or_rubber_band &&
						static_cast<bool>(intersection_or_rubber_band->get_rubber_band());
			}

			bool
			is_end_rubber_band() const
			{
				return !is_start_of_section &&
						intersection_or_rubber_band &&
						static_cast<bool>(intersection_or_rubber_band->get_rubber_band());
			}

			/**
			 * Compare markers.
			 */
			bool
			is_equivalent_to(
					const ResolvedSubSegmentMarker &other) const
			{
				if (!intersection_or_rubber_band)
				{
					if (!other.intersection_or_rubber_band)
					{
						// Neither marker is an intersection or rubber band, so each marker is either at
						// the beginning or end of the section geometry. If they are both at the beginning,
						// or both at the end, then they represent the same position.
						return is_start_of_section == other.is_start_of_section;
					}
					else if (boost::optional<const ResolvedSubSegmentRangeInSection::Intersection &> other_intersection =
							other.intersection_or_rubber_band->get_intersection())
					{
						// Our marker is not an intersection or rubber band, so it's either at the start or
						// end of the section geometry. If the other intersection is also at the start or end
						// then both markers represent the same position.
						if (is_start_of_section)
						{
							return other_intersection->segment_index == 0 &&
									other_intersection->on_segment_start;
						}
						else
						{
							// Test if on start of fictitious one-past-the-last segment.
							return other_intersection->segment_index == num_vertices_in_section - 1 &&
									other_intersection->on_segment_start;
						}
					}
					else // rubber band ...
					{
						// A rubber band point is off the section, so cannot equal a section vertex.
						return false;
					}
				}
				else if (boost::optional<const ResolvedSubSegmentRangeInSection::Intersection &> intersection =
						intersection_or_rubber_band->get_intersection())
				{
					if (!other.intersection_or_rubber_band)
					{
						// The other marker is not an intersection or rubber band, so it's either at the start
						// or end of the section geometry. If our intersection is also at the start or end
						// then both markers represent the same position.
						if (other.is_start_of_section)
						{
							return intersection->segment_index == 0 &&
									intersection->on_segment_start;
						}
						else
						{
							// Test if on start of fictitious one-past-the-last segment.
							return intersection->segment_index == num_vertices_in_section - 1 &&
									intersection->on_segment_start;
						}
					}
					else if (boost::optional<const ResolvedSubSegmentRangeInSection::Intersection &> other_intersection =
							other.intersection_or_rubber_band->get_intersection())
					{
						// Both markers are intersections, if they intersect the same segment at the same
						// place then they represent the same position.
						return intersection->segment_index == other_intersection->segment_index &&
								// NOTE: This is an epsilon comparison.
								intersection->angle_in_segment == other_intersection->angle_in_segment;
					}
					else // rubber band ...
					{
						// A rubber band point is off the section, so cannot equal a section intersection.
						return false;
					}
				}
				else // rubber band ...
				{
					if (!other.intersection_or_rubber_band)
					{
						// A rubber band point is off the section, so cannot equal a section vertex.
						return false;
					}
					else if (other.intersection_or_rubber_band->get_intersection())
					{
						// A rubber band point is off the section, so cannot equal a section intersection.
						return false;
					}
					else // rubber band ...
					{
						// Both markers are rubber bands, so each marker is either before the beginning or
						// after the end of the section geometry. If they are both before the beginning,
						// or both after the end, then they can be compared.
						if (is_start_of_section == other.is_start_of_section)
						{
							boost::optional<const ResolvedSubSegmentRangeInSection::RubberBand &> rubber_band =
									intersection_or_rubber_band->get_rubber_band();
							boost::optional<const ResolvedSubSegmentRangeInSection::RubberBand &> other_rubber_band =
									other.intersection_or_rubber_band->get_rubber_band();

							const boost::optional<topological_section_compare_type> adjacent_section_cmp =
									get_topological_section_compare(rubber_band->adjacent_section_reconstruction_geometry);
							const boost::optional<topological_section_compare_type> other_adjacent_section_cmp =
									get_topological_section_compare(other_rubber_band->adjacent_section_reconstruction_geometry);

							// Compare the adjacent sections (using the comparison objects).
							return adjacent_section_cmp == other_adjacent_section_cmp &&
									// Also make sure both rubber bands are using same end of adjacent section.
									// We could have instead compared adjacent section 'is_at_start_of_adjacent_section's
									// but that gets tricky with *point* sections because we don't know which is the
									// start and end of a point. So we just compare the rubber band positions instead...
									rubber_band->position == other_rubber_band->position;
						}
						else
						{
							// One rubber band is before the beginning and the other after the end.
							return false;
						}
					}
				}
			}


			//! The resolved topology that owns the sub-segment (and its geometry reversal flag)...
			ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo resolved_topology_info;

			//! Number of vertices in the section geometry (point, multi-point or polyline).
			unsigned int num_vertices_in_section;

			/**
			 * Either (optional) start intersection/rubber-band if @a is_start_of_section is true, or
			 * (optional) end intersection/rubber-band if @a is_start_of_section is false.
			 */
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> intersection_or_rubber_band;

			/**
			 * Whether this marker is the start or end of the *section*.
			 *
			 * Note that this is different to the start or end of a *sub-segment* in that the end of a
			 * sub-segment can be the start of the section (this happens in some cases when the start
			 * of the sub-segment is a rubber band, but the sub-segment ends at the start of the section
			 * in order to distinguish from sub-segments associated with other rubber bands).
			 *
			 * Note: This is not needed for intersections.
			 */
			bool is_start_of_section;

			//! This marker is either the *sub-segment* start or end.
			bool is_start_of_sub_segment;
		};


		/**
		 * Predicate to sort @a ResolvedSubSegmentMarker from beginning to end of the section geometry.
		 */
		class SortResolvedSubSegmentMarkers
		{
		public:

			bool
			operator()(
					const ResolvedSubSegmentMarker &lhs,
					const ResolvedSubSegmentMarker &rhs) const
			{
				if (!lhs.intersection_or_rubber_band)
				{
					if (!rhs.intersection_or_rubber_band)
					{
						// Neither marker is an intersection or rubber band, so each marker is either at
						// the beginning or end of the section geometry. If 'lhs' is at the beginning and
						// 'rhs' at the end then 'lhs' is less than 'rhs'. Note that two markers at beginning
						// (or two markers at end) compare equivalent (ie, !(lhs < rhs) && !(rhs < lhs))
						// and so std::stable_sort will retain their original order.
						return lhs.is_start_of_section && !rhs.is_start_of_section;
					}
					else if (rhs.intersection_or_rubber_band->get_intersection())
					{
						// The 'lhs' marker does not have an intersection or rubber band, so it's either at the start or
						// end of the section geometry. If it's at the start then it's *before* all intersections
						// (since we consider intersections to be *inside* the section, even if they intersect
						// the start of the section). If it's at the end then it's *after* all intersections.
						//
						// Note that this gives the correct order for a zero-length sub-segment at the start
						// of the section geometry. In other words a sub-segment that starts at the start of the
						// section should only have an end intersection and so its start will be before its end.
						return lhs.is_start_of_section;
					}
					else // rubber band ...
					{
						// A start/end rubber band is before/after all intersections and section vertices.
						return !rhs.is_start_of_section;
					}
				}
				else if (boost::optional<const ResolvedSubSegmentRangeInSection::Intersection &> lhs_intersection =
						lhs.intersection_or_rubber_band->get_intersection())
				{
					if (!rhs.intersection_or_rubber_band)
					{
						// The 'rhs' marker does not have an intersection or rubber band, so it's either at the start or
						// end of the section geometry. If it's at the start then it's *before* all intersections
						// (since we consider intersections to be *inside* the section, even if they intersect
						// the start of the section). If it's at the end then it's *after* all intersections.
						//
						// Note that this gives the correct order for a zero-length sub-segment at the end
						// of the section geometry. In other words a sub-segment that ends at the end of the
						// section should only have a start intersection and so its end will be after its start.
						return !rhs.is_start_of_section;
					}
					else if (boost::optional<const ResolvedSubSegmentRangeInSection::Intersection &> rhs_intersection =
							rhs.intersection_or_rubber_band->get_intersection())
					{
						// Both markers are intersections, if they intersect the same segment at the same
						// place then they represent the same position.
						return lhs_intersection->segment_index < rhs_intersection->segment_index ||
								(lhs_intersection->segment_index == rhs_intersection->segment_index &&
								// NOTE: This is an epsilon comparison. We want to detect equivalent markers so
								//       that they can retain their original sort order (in 'stable_sort')...
								lhs_intersection->angle_in_segment < rhs_intersection->angle_in_segment);
					}
					else // rubber band ...
					{
						// A start/end rubber band is before/after all intersections and section vertices.
						return !rhs.is_start_of_section;
					}
				}
				else // rubber band ...
				{
					if (!rhs.intersection_or_rubber_band)
					{
						// A start/end rubber band is before/after all intersections and section vertices.
						return lhs.is_start_of_section;
					}
					else if (rhs.intersection_or_rubber_band->get_intersection())
					{
						// A start/end rubber band is before/after all intersections and section vertices.
						return lhs.is_start_of_section;
					}
					else // rubber band ...
					{
						// Both markers are rubber bands, so each marker is either before the beginning or
						// after the end of the section geometry. If they are both before the beginning,
						// or both after the end, then we compare their adjacent segments (to group together
						// markers with the same adjacent section).
						if (lhs.is_start_of_section == rhs.is_start_of_section)
						{
							boost::optional<const ResolvedSubSegmentRangeInSection::RubberBand &> lhs_rubber_band =
									lhs.intersection_or_rubber_band->get_rubber_band();
							boost::optional<const ResolvedSubSegmentRangeInSection::RubberBand &> rhs_rubber_band =
									rhs.intersection_or_rubber_band->get_rubber_band();

							const boost::optional<topological_section_compare_type> lhs_adjacent_segment_cmp =
									get_topological_section_compare(lhs_rubber_band->adjacent_section_reconstruction_geometry);
							const boost::optional<topological_section_compare_type> rhs_adjacent_segment_cmp =
									get_topological_section_compare(rhs_rubber_band->adjacent_section_reconstruction_geometry);

							// Compare the adjacent sections (using the comparison objects).
							//
							// Note that two start rubber bands (or two end rubber bands) with the same (equal)
							// adjacent section (end point) will compare equivalent (ie, !(lhs < rhs) && !(rhs < lhs))
							// and so std::stable_sort will retain their original order.
							return lhs_adjacent_segment_cmp < rhs_adjacent_segment_cmp ||
									(lhs_adjacent_segment_cmp == rhs_adjacent_segment_cmp &&
										// Also check whether both rubber bands are using same end of adjacent section...
										// We could have instead compared adjacent section 'is_at_start_of_adjacent_section's
										// but that gets tricky with *point* sections because we don't know which is the
										// start and end of a point. So we just compare the rubber band positions instead...
										d_point_on_sphere_predicate(lhs_rubber_band->position, rhs_rubber_band->position));
						}
						else
						{
							// If 'lhs' is at beginning and 'rhs' at end of section then 'lhs' is less than 'rhs'.
							return lhs.is_start_of_section && !rhs.is_start_of_section;
						}
					}
				}
			}

		private:
			GPlatesMaths::PointOnSphereMapPredicate d_point_on_sphere_predicate;
		};


		/**
		 * Create and add a shared sub-segment defined by the specified start and end markers.
		 *
		 * This associates a uniquely shared sub-segment with those resolved topologies sharing it.
		 */
		void
		add_shared_sub_segment(
				shared_sub_segment_seq_type &shared_sub_segments,
				const ResolvedSubSegmentMarker &start_sub_segment_marker,
				const ResolvedSubSegmentMarker &end_sub_segment_marker,
				const std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> &sharing_resolved_topologies,
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry,
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_rg,
				const GPlatesModel::FeatureHandle::const_weak_ref &section_feature_ref)
		{
			const ResolvedSubSegmentRangeInSection shared_sub_segment_range(
					section_geometry,
					start_sub_segment_marker.intersection_or_rubber_band/*start_intersection_or_rubber_band*/,
					end_sub_segment_marker.intersection_or_rubber_band/*end_intersection_or_rubber_band*/);

			// Associate a uniquely shared sub-segment with those resolved topologies sharing it.
			shared_sub_segments.push_back(
					ResolvedTopologicalSharedSubSegment::create(
							shared_sub_segment_range,
							sharing_resolved_topologies,
							section_feature_ref,
							section_rg));
		}


		/**
		 * Add marker's topology to list of topologies if a start marker, otherwise remove from list.
		 */
		void
		add_or_remove_marker_topology(
				std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> &sharing_resolved_topologies,
				const ResolvedSubSegmentMarker &sub_segment_marker)
		{
			if (sub_segment_marker.is_start_of_sub_segment)
			{
				// We've reached a *start* sub-segment marker.
				// So add the sharing resolved topology of the current sub-segment marker.
				sharing_resolved_topologies.push_back(sub_segment_marker.resolved_topology_info);
			}
			else // sub-segment end point ...
			{
				// We've reached an *end* sub-segment marker.
				// So remove the sharing resolved topology of the current sub-segment marker.
				auto sharing_resolved_topology_iter = sharing_resolved_topologies.begin();
				const auto sharing_resolved_topology_end = sharing_resolved_topologies.end();
				for ( ; sharing_resolved_topology_iter != sharing_resolved_topology_end; ++sharing_resolved_topology_iter)
				{
					if (sharing_resolved_topology_iter->resolved_topology ==
						sub_segment_marker.resolved_topology_info.resolved_topology)
					{
						sharing_resolved_topologies.erase(sharing_resolved_topology_iter);
						break;
					}
				}
			}
		}


		/**
		 * Record the start/end point locations of each sub-segment within the section geometry.
		 */
		void
		find_resolved_topological_section_sub_segment_markers(
				std::vector<ResolvedSubSegmentMarker> &resolved_sub_segment_marker_seq,
				const std::vector<ResolvedSubSegmentInfo> &section_sub_segment_infos,
				unsigned int num_points_in_section_geometry)
		{
			// Special case handling of *point* section rubber bands.
			// Record, for each adjacent rubber band section encountered, whether it was first added as a start or end marker.
			// This enables us to share each rubber banded sub-segment (for a specific adjacent section) across topologies.
			typedef std::map<boost::optional<topological_section_compare_type>, bool/*start marker*/> rubber_bands_type;
			boost::optional<rubber_bands_type> point_section_rubber_bands;

			// Iterate over the sub-segments referencing the section.
			std::vector<ResolvedSubSegmentInfo>::const_iterator sub_segments_iter = section_sub_segment_infos.begin();
			std::vector<ResolvedSubSegmentInfo>::const_iterator sub_segments_end = section_sub_segment_infos.end();
			for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
			{
				const ResolvedSubSegmentInfo &sub_segment_info = *sub_segments_iter;

				const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo resolved_topology_info(
						sub_segment_info.resolved_topology,
						sub_segment_info.sub_segment->get_use_reverse());

				const ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segment_info.sub_segment;
				const ResolvedSubSegmentRangeInSection &sub_segment_range = sub_segment.get_sub_segment();

				// For *point* sections with both start and end rubber bands we don't know which is the
				// start and which is the end of the section. This means two sections that should be
				// shared could have swapped start and end rubber bands and hence won't get shared.
				// To get around this we detect if one start/end rubber band pair is a swapped version
				// of another and generate equivalent start/end rubber band markers to ensure they
				// generate a shared sub-segment (rather than two un-shared sub-segments).
				// There are also more complex scenarios where there's more than two adjacent sections
				// (like a triple junction) - in these cases it might also be necessary to swap only
				// half a sub-segment (either the part from the start rubber band to point section or
				// the part from the point section to the end rubber band) - in which case extra markers
				// need to be inserted to account for this.
				if (sub_segment_range.get_num_points_in_section_geometry() == 1 &&
					// Note these *point* sections should have both start and end rubber bands because they
					// are contributing to topological boundaries and networks, both of which have polygon
					// boundaries and hence all contributing sections (whether lines or points) will have
					// *two* adjacent neighbours, and for point sections these will always be rubber bands...
					sub_segment_range.get_start_rubber_band() &&
					sub_segment_range.get_end_rubber_band())
				{
					if (!point_section_rubber_bands)
					{
						point_section_rubber_bands = rubber_bands_type();
					}

					ResolvedSubSegmentRangeInSection::RubberBand start_rubber_band =
							sub_segment_range.get_start_rubber_band().get();
					ResolvedSubSegmentRangeInSection::RubberBand end_rubber_band =
							sub_segment_range.get_end_rubber_band().get();

					const boost::optional<topological_section_compare_type> start_adjacent_section =
							get_topological_section_compare(start_rubber_band.adjacent_section_reconstruction_geometry);
					const boost::optional<topological_section_compare_type> end_adjacent_section =
							get_topological_section_compare(end_rubber_band.adjacent_section_reconstruction_geometry);

					auto start_rubber_band_iter = point_section_rubber_bands->find(start_adjacent_section);
					auto end_rubber_band_iter = point_section_rubber_bands->find(end_adjacent_section);

					if (start_rubber_band_iter == point_section_rubber_bands->end() &&
						end_rubber_band_iter == point_section_rubber_bands->end())
					{
						// We've not previously added markers for either adjacent start or end rubber-banded section.
						// So we can just simply add them now without modification.

						const ResolvedSubSegmentMarker sub_segment_start_marker(
								resolved_topology_info,
								num_points_in_section_geometry,
								ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
								true/*is_start_of_section*/,
								true/*is_start_of_sub_segment*/);
						const ResolvedSubSegmentMarker sub_segment_end_marker(
								resolved_topology_info,
								num_points_in_section_geometry,
								ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
								false/*is_start_of_section*/,
								false/*is_start_of_sub_segment*/);

						resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
						resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);

						// Record that we've added start and end markers for the start and end rubber band adjacent sections.
						point_section_rubber_bands->insert(
								rubber_bands_type::value_type(start_adjacent_section, true/*start marker*/));
						point_section_rubber_bands->insert(
								rubber_bands_type::value_type(end_adjacent_section, false/*start marker*/));
					}
					else if (start_rubber_band_iter != point_section_rubber_bands->end() &&
						end_rubber_band_iter != point_section_rubber_bands->end())
					{
						// We've previously added markers for both the adjacent start/end rubber-banded sections.

						if (start_rubber_band_iter->second/*start*/ && !end_rubber_band_iter->second/*start*/)
						{
							// The current start and end rubber bands were previously added as *start* and *end* markers.
							//
							// So we can just simply add the start/end markers without modification.
							const ResolvedSubSegmentMarker sub_segment_start_marker(
									resolved_topology_info,
									num_points_in_section_geometry,
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker sub_segment_end_marker(
									resolved_topology_info,
									num_points_in_section_geometry,
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);
						}
						else if (!start_rubber_band_iter->second/*start*/ && end_rubber_band_iter->second/*start*/)
						{
							// The current start rubber band was previously added as an *end* marker and
							// the current end rubber band was previously added as a *start* marker.
							//
							// In other words, the entire start-to-end sub-segment needs to be reversed.
							//
							// So current start rubber band will now be an end marker (and hence should be at end of section) and
							// current end rubber band will now be a start marker (and hence should be at start of section).
							start_rubber_band.is_at_start_of_current_section = false;
							end_rubber_band.is_at_start_of_current_section = true;

							// This also means reversing the entire start-to-end sub-segment contribution to the resolved topology.
							const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo reversed_resolved_topology_info(
									resolved_topology_info.resolved_topology,
									!resolved_topology_info.is_sub_segment_geometry_reversed);

							const ResolvedSubSegmentMarker sub_segment_start_marker(
									reversed_resolved_topology_info,
									num_points_in_section_geometry,
									// End rubber band is now a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker sub_segment_end_marker(
									reversed_resolved_topology_info,
									num_points_in_section_geometry,
									// Start rubber band is now an end marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);
						}
						else if (start_rubber_band_iter->second/*start*/ && end_rubber_band_iter->second/*start*/)
						{
							// The current start rubber band was previously added as a *start* marker but
							// the current end rubber band was also previously added as a *start* marker.
							//
							// So we need to divide the start-to-end sub-segment into two sub-segments.
							// One is the start rubber band to section point (which is not reversed) and the
							// other is the section point to end rubber band (which is reversed).
							//
							// So current start rubber band will remain a start marker and current end rubber band
							// will now also be a start marker (and hence should be at start of section).
							end_rubber_band.is_at_start_of_current_section = true;

							// We need to use an intersection to mark the start of a section.
							const ResolvedSubSegmentRangeInSection::Intersection start_of_section =
									ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
											*sub_segment_range.get_section_geometry(),
											true/*at_start*/);

							// The reversed section-to-end sub-segment contribution to the resolved topology.
							const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo reversed_resolved_topology_info(
									resolved_topology_info.resolved_topology,
									!resolved_topology_info.is_sub_segment_geometry_reversed);

							const ResolvedSubSegmentMarker first_sub_segment_start_marker(
									resolved_topology_info,  // not reversed
									num_points_in_section_geometry,
									// Start rubber band is a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker first_sub_segment_end_marker(
									resolved_topology_info,  // not reversed
									num_points_in_section_geometry,
									// Start of section (point) is an end marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_of_section),
									true/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							const ResolvedSubSegmentMarker second_sub_segment_start_marker(
									reversed_resolved_topology_info,  // reversed
									num_points_in_section_geometry,
									// End rubber band is now also a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker second_sub_segment_end_marker(
									reversed_resolved_topology_info,  // reversed
									num_points_in_section_geometry,
									// Start of section (point) is an end marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_of_section),
									true/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							// Note that the order doesn't really matter here since these will get
							// sorted by 'std::stable_sort' below such that the two rubber band markers
							// come before the two section (point) markers.
							resolved_sub_segment_marker_seq.push_back(first_sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(first_sub_segment_end_marker);
							resolved_sub_segment_marker_seq.push_back(second_sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(second_sub_segment_end_marker);
						}
						else  // !start_rubber_band_iter->second/*start*/) && !end_rubber_band_iter->second/*start*/ ...
						{
							// The current end rubber band was previously added as an *end* marker but
							// the current start rubber band was also previously added as an *end* marker.
							//
							// So we need to divide the start-to-end sub-segment into two sub-segments.
							// One is the start rubber band to section point (which is reversed) and the
							// other is the section point to end rubber band (which is not reversed).
							//
							// So current end rubber band will remain an end marker and current start rubber band
							// will now also be an end marker (and hence should be at end of section).
							start_rubber_band.is_at_start_of_current_section = false;

							// We need to use an intersection to mark the end of a section.
							const ResolvedSubSegmentRangeInSection::Intersection end_of_section =
									ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
											*sub_segment_range.get_section_geometry(),
											false/*at_start*/);

							// The reversed start-to-section sub-segment contribution to the resolved topology.
							const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo reversed_resolved_topology_info(
									resolved_topology_info.resolved_topology,
									!resolved_topology_info.is_sub_segment_geometry_reversed);

							const ResolvedSubSegmentMarker first_sub_segment_start_marker(
									reversed_resolved_topology_info,  // reversed
									num_points_in_section_geometry,
									// End of section (point) is a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_of_section),
									false/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker first_sub_segment_end_marker(
									reversed_resolved_topology_info,  // reversed
									num_points_in_section_geometry,
									// Start rubber band is an end marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							const ResolvedSubSegmentMarker second_sub_segment_start_marker(
									resolved_topology_info,  // not reversed
									num_points_in_section_geometry,
									// End of section (point) is a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_of_section),
									false/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker second_sub_segment_end_marker(
									resolved_topology_info,  // not reversed
									num_points_in_section_geometry,
									// End rubber band is now also a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							// Note that the order doesn't really matter here since these will get
							// sorted by 'std::stable_sort' below such that the two rubber band markers
							// come before the two section (point) markers.
							resolved_sub_segment_marker_seq.push_back(first_sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(first_sub_segment_end_marker);
							resolved_sub_segment_marker_seq.push_back(second_sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(second_sub_segment_end_marker);
						}
					}
					else if (start_rubber_band_iter != point_section_rubber_bands->end())
					{
						// We've previously added a marker for the adjacent *start* rubber-banded section,
						// but not for the adjacent *end* rubber-banded section.

						if (start_rubber_band_iter->second/*start*/)
						{
							// The current start rubber band was previously added as a *start* marker.
							//
							// So we can just simply add the start/end markers without modification.
							const ResolvedSubSegmentMarker sub_segment_start_marker(
									resolved_topology_info,
									num_points_in_section_geometry,
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker sub_segment_end_marker(
									resolved_topology_info,
									num_points_in_section_geometry,
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);

							// Record that we've added an end marker for the end rubber band adjacent section.
							// We've already recorded the start marker.
							point_section_rubber_bands->insert(
									rubber_bands_type::value_type(end_adjacent_section, false/*start marker*/));
						}
						else
						{
							// The current start rubber band was previously added as an *end* marker.
							//
							// So current start rubber band will now be an end marker (and hence should be at end of section) and
							// current end rubber band will now be a start marker (and hence should be at start of section).
							start_rubber_band.is_at_start_of_current_section = false;
							end_rubber_band.is_at_start_of_current_section = true;

							// This also means reversing the entire start-to-end sub-segment contribution to the resolved topology.
							const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo reversed_resolved_topology_info(
									resolved_topology_info.resolved_topology,
									!resolved_topology_info.is_sub_segment_geometry_reversed);

							const ResolvedSubSegmentMarker sub_segment_start_marker(
									reversed_resolved_topology_info,
									num_points_in_section_geometry,
									// End rubber band is now a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker sub_segment_end_marker(
									reversed_resolved_topology_info,
									num_points_in_section_geometry,
									// Start rubber band is now an end marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);

							// Record that we've added a start marker for the end rubber band adjacent section.
							// We've already recorded the end marker.
							point_section_rubber_bands->insert(
									rubber_bands_type::value_type(end_adjacent_section, true/*start marker*/));
						}
					}
					else // end_rubber_band_iter != point_section_rubber_bands->end() ...
					{
						// We've previously added a marker for the adjacent *end* rubber-banded section,
						// but not for the adjacent *start* rubber-banded section.

						if (!end_rubber_band_iter->second/*start*/)
						{
							// The current end rubber band was previously added as an *end* marker.
							//
							// So we can just simply add the start/end markers without modification.
							const ResolvedSubSegmentMarker sub_segment_start_marker(
									resolved_topology_info,
									num_points_in_section_geometry,
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker sub_segment_end_marker(
									resolved_topology_info,
									num_points_in_section_geometry,
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);

							// Record that we've added a start marker for the start rubber band adjacent section.
							// We've already recorded the end marker.
							point_section_rubber_bands->insert(
									rubber_bands_type::value_type(start_adjacent_section, true/*start marker*/));
						}
						else
						{
							// The current end rubber band was previously added as a *start* marker.
							//
							// So current start rubber band will now be an end marker (and hence should be at end of section) and
							// current end rubber band will now be a start marker (and hence should be at start of section).
							start_rubber_band.is_at_start_of_current_section = false;
							end_rubber_band.is_at_start_of_current_section = true;

							// This also means reversing the entire start-to-end sub-segment contribution to the resolved topology.
							const ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo reversed_resolved_topology_info(
									resolved_topology_info.resolved_topology,
									!resolved_topology_info.is_sub_segment_geometry_reversed);

							const ResolvedSubSegmentMarker sub_segment_start_marker(
									reversed_resolved_topology_info,
									num_points_in_section_geometry,
									// End rubber band is now a start marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band),
									true/*is_start_of_section*/,
									true/*is_start_of_sub_segment*/);
							const ResolvedSubSegmentMarker sub_segment_end_marker(
									reversed_resolved_topology_info,
									num_points_in_section_geometry,
									// Start rubber band is now an end marker...
									ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band),
									false/*is_start_of_section*/,
									false/*is_start_of_sub_segment*/);

							resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
							resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);

							// Record that we've added an end marker for the start rubber band adjacent section.
							// We've already recorded the start marker.
							point_section_rubber_bands->insert(
									rubber_bands_type::value_type(start_adjacent_section, false/*start marker*/));
						}
					}
				}
				else // non-points sections ...
				{
					const ResolvedSubSegmentMarker sub_segment_start_marker(
							resolved_topology_info,
							num_points_in_section_geometry,
							sub_segment_range.get_start_intersection_or_rubber_band(),
							true/*is_start_of_section*/,
							true/*is_start_of_sub_segment*/);
					const ResolvedSubSegmentMarker sub_segment_end_marker(
							resolved_topology_info,
							num_points_in_section_geometry,
							sub_segment_range.get_end_intersection_or_rubber_band(),
							false/*is_start_of_section*/,
							false/*is_start_of_sub_segment*/);

					// NOTE: We add the start marker before the end marker.
					// This is in case the sub-segment range is zero length
					// (start and end marker are coincident) in which case we want to
					// add topology from start marker first before removing it with end marker.
					// See 'stable_sort' comment below for more details.
					resolved_sub_segment_marker_seq.push_back(sub_segment_start_marker);
					resolved_sub_segment_marker_seq.push_back(sub_segment_end_marker);
				}
			}

			// Sort the markers from beginning to end of section geometry.
			//
			// NOTE: We use 'stable_sort' instead of 'sort' in case the start and end markers of a
			// line's sub-segment range are equal (ie, a zero-length range), we want to give preference
			// to the start marker (over the end marker) since we later add topologies when encountering
			// start markers and remove them when encountering end markers - and so we don't want to try
			// removing a topology that hasn't been added yet. Using 'stable_sort' guarantees the
			// ordering of the equal start and end markers (added above) will not get changed.
			std::stable_sort(
					resolved_sub_segment_marker_seq.begin(),
					resolved_sub_segment_marker_seq.end(),
					SortResolvedSubSegmentMarkers());
		}


		/**
		 * Handle start/end rubber band markers.
		 *
		 * If any start rubber band markers are different then we need separate sub-segments
		 * (once for each group of equivalent start rubber band markers) starting at a
		 * start rubber band and ending at the start of the section geometry.
		 * Otherwise we don't need to do anything (all shared sub-segments will start
		 * at the same start rubber band).
		 *
		 * If any end rubber band markers are different then we need separate sub-segments
		 * (once for each group of equivalent end rubber band markers) starting at the
		 * end of the section geometry and ending at an end rubber band.
		 * Otherwise we don't need to do anything (all shared sub-segments will end
		 * at the same end rubber band).
		 *
		 * On input, @a markers should be sorted.
		 * However on output, @a markers can be unsorted (if have different start rubber bands
		 * or different end rubber bands).
		 */
		void
		handle_rubber_band_sub_segment_markers(
				std::vector<ResolvedSubSegmentMarker> &markers,
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry)
		{
			if (markers.empty())
			{
				return;
			}

			//
			// Handle the start rubber bands.
			//

			// See if there are any start rubber bands (will be at start of sorted sequence).
			// Usually there won't be any.
			if (markers.front().is_start_rubber_band())
			{
				// Record any different start rubber band markers.
				std::vector<unsigned int> start_marker_groups;

				// Iterate over all start rubber band markers and see if any are different.
				//
				// Start rubber band markers should all be at the beginning of the sorted sequence.
				unsigned int start_marker_index = 0;
				for ( ;
					start_marker_index < markers.size() && markers[start_marker_index].is_start_rubber_band();
					++start_marker_index)
				{
					// See if current start rubber band differs from previous one.
					if (start_marker_index > 0 &&
						!markers[start_marker_index - 1].is_equivalent_to(markers[start_marker_index]))
					{
						start_marker_groups.push_back(start_marker_index);
					}
				}

				// If any start rubber band markers are different then we need separate sub-segments
				// (once for each group of equivalent start rubber band markers) starting at a
				// start rubber band and ending at the start of the section geometry.
				//
				// Otherwise we don't need to do anything (all shared sub-segments will start
				// at the same start rubber band).
				if (!start_marker_groups.empty())
				{
					// The end of the last group of start rubber band markers.
					start_marker_groups.push_back(start_marker_index);

					const unsigned int num_original_start_markers = start_marker_index;

					std::vector<ResolvedSubSegmentMarker> new_markers;
					new_markers.reserve(2 * num_original_start_markers);

					// We need to use an intersection to mark the start of a section.
					const ResolvedSubSegmentRangeInSection::Intersection start_of_section =
							ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
									*section_geometry,
									true/*at_start*/);

					// Each marker in each group of equivalent start rubber band markers will emit
					// a sub-segment start and end marker.
					unsigned int start_group_marker_index = 0;
					for (unsigned int s = 0; s < start_marker_groups.size(); ++s)
					{
						const unsigned int end_group_marker_index = start_marker_groups[s];

						// Add a sub-segment *start* marker for each marker in current group.
						for (unsigned int marker_index = start_group_marker_index;
							marker_index < end_group_marker_index;
							++marker_index)
						{
							const ResolvedSubSegmentMarker &start_marker = markers[marker_index];
							new_markers.push_back(start_marker);
						}

						// Add a sub-segment *end* marker for each marker in current group.
						for (unsigned int marker_index = start_group_marker_index;
							marker_index < end_group_marker_index;
							++marker_index)
						{
							// Copy the start marker.
							ResolvedSubSegmentMarker end_marker = markers[marker_index];

							// It's the end of the sub-segment.
							end_marker.is_start_of_sub_segment = false;
							// It's at the start of the section so add start-of-section intersection
							// (note that 'end_marker.is_start_of_section' should be true).
							//
							// We need to use an intersection to mark the start of a section.
							// It's tempting to set this to 'none' to signify *start* of section
							// but that creates a subtle problem when we later create a
							// ResolvedSubSegmentRangeInSection (for a shared sub-segment) in that the
							// ResolvedSubSegmentRangeInSection will think it's the *end* of section...
							end_marker.intersection_or_rubber_band = boost::in_place(start_of_section);

							new_markers.push_back(end_marker);
						}

						start_group_marker_index = end_group_marker_index;
					}

					// Modify all original start markers so that they refer to the start of the section.
					// These will now be the start of sub-segments that start at the start of the section.
					//
					// Note that the new markers take care of the sub-segments from start rubber bands to start of section.
					for (unsigned int marker_index = 0; marker_index < num_original_start_markers; ++marker_index)
					{
						ResolvedSubSegmentMarker &original_start_marker = markers[marker_index];

						// It's at the start of the section so add start-of-section intersection
						// (note that 'original_start_marker.is_start_of_section' should be true).
						//
						// We need to use an intersection to mark the start of a section.
						// It's tempting to set this to 'none' to signify *start* of section
						// but that creates a subtle problem when we later create a
						// ResolvedSubSegmentRangeInSection (for a shared sub-segment) in that the
						// ResolvedSubSegmentRangeInSection will think it's the *end* of section...
						original_start_marker.intersection_or_rubber_band = boost::in_place(start_of_section);
					}

					// Insert the new markers before the original start markers.
					markers.insert(markers.begin(), new_markers.begin(), new_markers.end());
				}
			}

			//
			// Handle the end rubber bands.
			//

			// See if there are any end rubber bands (will be at end of sorted sequence).
			// Usually there won't be any.
			if (markers.back().is_end_rubber_band())
			{
				// Record any different end rubber band markers.
				std::vector<unsigned int> end_marker_groups;

				// Iterate backward over all end rubber band markers to find the first one.
				// End rubber band markers should all be at the end of the sorted sequence.
				unsigned int first_end_marker_index = markers.size();
				while (markers[first_end_marker_index - 1].is_end_rubber_band())
				{
					--first_end_marker_index;
					if (first_end_marker_index == 0)
					{
						break;
					}
				}

				// Iterate over all end rubber band markers and see if any are different.
				//
				// End rubber band markers should all be at the end of the sorted sequence.
				unsigned int end_marker_index = first_end_marker_index;
				for ( ; end_marker_index < markers.size(); ++end_marker_index)
				{
					// See if current end rubber band differs from previous one.
					if (end_marker_index > first_end_marker_index &&
						!markers[end_marker_index - 1].is_equivalent_to(markers[end_marker_index]))
					{
						end_marker_groups.push_back(end_marker_index);
					}
				}

				// If any end rubber band markers are different then we need separate sub-segments
				// (once for each group of equivalent end rubber band markers) starting at the
				// end of the section geometry and ending at an end rubber band.
				//
				// Otherwise we don't need to do anything (all shared sub-segments will end
				// at the same end rubber band).
				if (!end_marker_groups.empty())
				{
					// The end of the last group of end rubber band markers.
					end_marker_groups.push_back(end_marker_index);

					const unsigned int num_original_end_markers = end_marker_index - first_end_marker_index;

					std::vector<ResolvedSubSegmentMarker> new_markers;
					new_markers.reserve(2 * num_original_end_markers);

					// We need to use an intersection to mark the end of a section.
					const ResolvedSubSegmentRangeInSection::Intersection end_of_section =
							ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
									*section_geometry,
									false/*at_start*/);

					// Each marker in each group of equivalent end rubber band markers will emit
					// a sub-segment start and end marker.
					unsigned int start_group_marker_index = first_end_marker_index;
					for (unsigned int e = 0; e < end_marker_groups.size(); ++e)
					{
						const unsigned int end_group_marker_index = end_marker_groups[e];

						// Add a sub-segment *start* marker for each marker in current group.
						for (unsigned int marker_index = start_group_marker_index;
							marker_index < end_group_marker_index;
							++marker_index)
						{
							// Copy the end marker.
							ResolvedSubSegmentMarker start_marker = markers[marker_index];

							// It's the start of the sub-segment.
							start_marker.is_start_of_sub_segment = true;
							// It's at the end of the section so add end-of-section intersection
							// (note that 'start_marker.is_start_of_section' should be true).
							//
							// We need to use an intersection to mark the end of a section.
							// It's tempting to set this to 'none' to signify *end* of section
							// but that creates a subtle problem when we later create a
							// ResolvedSubSegmentRangeInSection (for a shared sub-segment) in that the
							// ResolvedSubSegmentRangeInSection will think it's the *start* of section...
							start_marker.intersection_or_rubber_band = boost::in_place(end_of_section);

							new_markers.push_back(start_marker);
						}

						// Add a sub-segment *end* marker for each marker in current group.
						for (unsigned int marker_index = start_group_marker_index;
							marker_index < end_group_marker_index;
							++marker_index)
						{
							const ResolvedSubSegmentMarker &end_marker = markers[marker_index];
							new_markers.push_back(end_marker);
						}

						start_group_marker_index = end_group_marker_index;
					}

					// Modify all original end markers so that they refer to the end of the section.
					// These will now be the end of sub-segments that end at the end of the section.
					//
					// Note that the new markers take care of the sub-segments from end of section to end rubber bands.
					for (unsigned int marker_index = markers.size() - num_original_end_markers;
						marker_index < markers.size();
						++marker_index)
					{
						ResolvedSubSegmentMarker &original_end_marker = markers[marker_index];

						// It's at the end of the section so add end-of-section intersection
						// (note that 'original_end_marker.is_start_of_section' should be false).
						//
						// We need to use an intersection to mark the end of a section.
						// It's tempting to set this to 'none' to signify *end* of section
						// but that creates a subtle problem when we later create a
						// ResolvedSubSegmentRangeInSection (for a shared sub-segment) in that the
						// ResolvedSubSegmentRangeInSection will think it's the *start* of section...
						original_end_marker.intersection_or_rubber_band = boost::in_place(end_of_section);
					}

					// Insert the new markers after the original end markers.
					markers.insert(markers.end(), new_markers.begin(), new_markers.end());
				}
			}
		}


		/**
		 * Iterate over the resolved section polyline segment markers and emit shared sub-segments for the section.
		 *
		 * Note that @a resolved_sub_segment_marker_seq is not necessarily sorted
		 * (due to the "handle_rubber_band_sub_segment_markers" function).
		 */
		void
		get_resolved_topological_section_shared_sub_segments(
				shared_sub_segment_seq_type &shared_sub_segments,
				const std::vector<ResolvedSubSegmentMarker> &resolved_sub_segment_marker_seq,
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry,
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_rg,
				const GPlatesModel::FeatureHandle::const_weak_ref &section_feature_ref)
		{
			// As we progress through the sub-segment markers the list of resolved topologies sharing a
			// sub-segment will change.
			std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> sharing_resolved_topologies;

			boost::optional<const ResolvedSubSegmentMarker &> prev_sub_segment_marker;

			// Iterate over the segment markers sequence (ordered from start to end of section geometry).
			std::vector<ResolvedSubSegmentMarker>::const_iterator sub_segment_markers_iter =
					resolved_sub_segment_marker_seq.begin();
			std::vector<ResolvedSubSegmentMarker>::const_iterator sub_segment_markers_end =
					resolved_sub_segment_marker_seq.end();
			for ( ;
				sub_segment_markers_iter != sub_segment_markers_end;
				(prev_sub_segment_marker = *sub_segment_markers_iter), ++sub_segment_markers_iter)
			{
				const ResolvedSubSegmentMarker &sub_segment_marker = *sub_segment_markers_iter;
	
				//
				// If the previous and current markers are different then there is a shared sub-segment
				// between them that can be emitted. However there might not be any resolved topologies.
				// This can happen if previous marker was an *end* marker and current marker is a
				// *start* marker and there are no resolved topologies referencing the part of the
				// section between those markers.
				//
				// Also if a resolved topology has a sub-segment that is a point (and not a line)
				// then its start and end marker positions will be coincident and hence that resolved
				// topology will not have its (point) sub-segment emitted - this is fine since it's
				// zero length anyway and therefore doesn't really contribute as a topological section.
				//
				if (prev_sub_segment_marker &&
					!prev_sub_segment_marker->is_equivalent_to(sub_segment_marker) &&
					!sharing_resolved_topologies.empty())
				{
					add_shared_sub_segment(
							shared_sub_segments,
							prev_sub_segment_marker.get()/*start_sub_segment_marker*/,
							sub_segment_marker/*end_sub_segment_marker*/,
							sharing_resolved_topologies,
							section_geometry,
							section_rg,
							section_feature_ref);
				}

				// Add/remove marker's topology to/from list of topologies if a start/end marker.
				add_or_remove_marker_topology(sharing_resolved_topologies, sub_segment_marker);
			}
		}
	}
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		// If the current property is a topological geometry then we have a topological feature.
		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter))
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


boost::optional<GPlatesAppLogic::TopologyGeometry::Type>
GPlatesAppLogic::TopologyUtils::get_topological_geometry_type(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		// If the current property is a topological geometry then we have a topological feature.
		boost::optional<GPlatesPropertyValues::StructuralType> topology_geometry_property_value_type =
				TopologyInternalUtils::get_topology_geometry_property_value_type(iter);
		if (topology_geometry_property_value_type)
		{
			static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_LINE =
					GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLine");
			static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_POLYGON =
					GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");
			static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_NETWORK =
					GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");

			if (topology_geometry_property_value_type == GPML_TOPOLOGICAL_LINE)
			{
				return TopologyGeometry::LINE;
			}
			else if (topology_geometry_property_value_type == GPML_TOPOLOGICAL_POLYGON)
			{
				return TopologyGeometry::BOUNDARY;
			}
			else if (topology_geometry_property_value_type == GPML_TOPOLOGICAL_NETWORK)
			{
				return TopologyGeometry::NETWORK;
			}
		}
	}

	return boost::none;
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_line_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_LINE =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLine");

		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter) == GPML_TOPOLOGICAL_LINE)
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_line_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_line_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_line_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles,
		boost::optional<const std::set<GPlatesModel::FeatureId> &> topological_lines_referenced)
{
	PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each RTG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological line features.
	TopologyGeometryResolver topology_line_resolver(
			resolved_topological_lines,
			reconstruct_handle,
			reconstruction_tree_creator,
			reconstruction_time,
			topological_sections_reconstruct_handles);

	for (auto feature_collection : topological_line_features_collection)
	{
		if (feature_collection.is_valid())
		{
			for (auto feature : *feature_collection)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature->reference();

				if (topological_lines_referenced)
				{
					// Only visit feature if its feature ID matches those specified.
					const GPlatesModel::FeatureId &feature_id = feature_ref->feature_id();
					if (topological_lines_referenced->find(feature_id) != topological_lines_referenced->end())
					{
						topology_line_resolver.visit_feature(feature_ref);
					}
				}
				else
				{
					topology_line_resolver.visit_feature(feature_ref);
				}
			}
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_line_features,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles,
		boost::optional<const std::set<GPlatesModel::FeatureId> &> topological_lines_referenced)
{
	PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each RTG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological line features.
	TopologyGeometryResolver topology_line_resolver(
			resolved_topological_lines,
			reconstruct_handle,
			reconstruction_tree_creator,
			reconstruction_time,
			topological_sections_reconstruct_handles);

	for (auto feature_ref : topological_line_features)
	{
		if (feature_ref.is_valid())
		{
			if (topological_lines_referenced)
			{
				// Only visit feature if its feature ID matches those specified.
				const GPlatesModel::FeatureId &feature_id = feature_ref->feature_id();
				if (topological_lines_referenced->find(feature_id) != topological_lines_referenced->end())
				{
					topology_line_resolver.visit_feature(feature_ref);
				}
			}
			else
			{
				topology_line_resolver.visit_feature(feature_ref);
			}
		}
	}

	return reconstruct_handle;
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_boundary_feature(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_POLYGON =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");

		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter) == GPML_TOPOLOGICAL_POLYGON)
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_boundary_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_boundary_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_closed_plate_polygon_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles)
{
	PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each RTG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological boundary features.
	TopologyGeometryResolver topology_boundary_resolver(
			resolved_topological_boundaries,
			reconstruct_handle,
			reconstruction_tree_creator,
			reconstruction_time,
			topological_sections_reconstruct_handles);

	AppLogicUtils::visit_feature_collections(
			topological_closed_plate_polygon_features_collection.begin(),
			topological_closed_plate_polygon_features_collection.end(),
			topology_boundary_resolver);

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_closed_plate_polygon_features,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles)
{
	PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each RTG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological boundary features.
	TopologyGeometryResolver topology_boundary_resolver(
			resolved_topological_boundaries,
			reconstruct_handle,
			reconstruction_tree_creator,
			reconstruction_time,
			topological_sections_reconstruct_handles);

	AppLogicUtils::visit_features(
			topological_closed_plate_polygon_features.begin(),
			topological_closed_plate_polygon_features.end(),
			topology_boundary_resolver);

	return reconstruct_handle;
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_network_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_NETWORK =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");

		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter) == GPML_TOPOLOGICAL_NETWORK)
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_network_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter =
			feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end =
			feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_network_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_network_features_collection,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles,
		const TopologyNetworkParams &topology_network_params)
{
	PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each RTN.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological network features.
	TopologyNetworkResolver topology_network_resolver(
			resolved_topological_networks,
			reconstruction_time,
			reconstruct_handle,
			topological_geometry_reconstruct_handles,
			topology_network_params);

	AppLogicUtils::visit_feature_collections(
			topological_network_features_collection.begin(),
			topological_network_features_collection.end(),
			topology_network_resolver);

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_network_features,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles,
		const TopologyNetworkParams &topology_network_params)
{
	PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each RTN.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological network features.
	TopologyNetworkResolver topology_network_resolver(
			resolved_topological_networks,
			reconstruction_time,
			reconstruct_handle,
			topological_geometry_reconstruct_handles,
			topology_network_params);

	AppLogicUtils::visit_features(
			topological_network_features.begin(),
			topological_network_features.end(),
			topology_network_resolver);

	return reconstruct_handle;
}


void
GPlatesAppLogic::TopologyUtils::find_resolved_topological_sections(
		std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
		const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &resolved_topological_boundaries,
		const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &resolved_topological_networks)
{
	//
	// Find all topological sections referenced by the resolved topologies.
	// And build a list of resolved topologies (and their sub-segments) that reference each topological section.
	//

	resolved_section_to_sharing_resolved_topologies_map_type resolved_section_to_sharing_resolved_topologies_map;

	// Iterate over the plate polygons.
	BOOST_FOREACH(
			const ResolvedTopologicalBoundary::non_null_ptr_to_const_type &resolved_topological_boundary,
			resolved_topological_boundaries)
	{
		map_resolved_topological_sections_to_resolved_topologies(
				resolved_section_to_sharing_resolved_topologies_map,
				resolved_topological_boundary,
				resolved_topological_boundary->get_sub_segment_sequence());
	}

	// Iterate over the deforming networks.
	BOOST_FOREACH(
			const ResolvedTopologicalNetwork::non_null_ptr_to_const_type &resolved_topological_network,
			resolved_topological_networks)
	{
		map_resolved_topological_sections_to_resolved_topologies(
				resolved_section_to_sharing_resolved_topologies_map,
				resolved_topological_network,
				// Only interested in boundary (not interior) since only boundary sub-segments
				// are shared with adjacent plate/network topologies...
				resolved_topological_network->get_boundary_sub_segment_sequence());
	}

	//
	// For each topological section (referenced by the resolved topologies) build sub-segments that are
	// uniquely shared by one or more resolved topologies.
	//

	// Iterate over the sections referenced by resolved topologies.
	BOOST_FOREACH(
			const resolved_section_to_sharing_resolved_topologies_map_type::value_type &
					section_to_sharing_resolved_topologies_entry,
			resolved_section_to_sharing_resolved_topologies_map)
	{
		const std::vector<ResolvedSubSegmentInfo> &sub_segments = section_to_sharing_resolved_topologies_entry.second;

		// All sub-segments share the same section feature and section geometry (so pick any sub-segment).
		//
		// However note that the sub-segments may have different ReconstructionGeometry's
		// (if the layers of the topologies that they came from are different and each layer independently
		// reconstructed the same topological section).
		// In this case we just arbitrarily choose one of them (its attributes should all be the same anyway).
		const GPlatesModel::FeatureHandle::const_weak_ref section_feature_ref =
				sub_segments.front().sub_segment->get_feature_ref();
		const ReconstructionGeometry::non_null_ptr_to_const_type section_rg =
				sub_segments.front().sub_segment->get_reconstruction_geometry();
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry =
				sub_segments.front().sub_segment->get_section_geometry();
		const unsigned int num_points_in_section_geometry =
				sub_segments.front().sub_segment->get_num_points_in_section_geometry();

		// Record the start/end point locations of each sub-segment within the section geometry.
		std::vector<ResolvedSubSegmentMarker> resolved_sub_segment_marker_seq;
		find_resolved_topological_section_sub_segment_markers(
				resolved_sub_segment_marker_seq,
				sub_segments,
				num_points_in_section_geometry);

		// Handle start/end rubber band markers.
		handle_rubber_band_sub_segment_markers(
				resolved_sub_segment_marker_seq,
				section_geometry);

		// Iterate over the segment markers and emit shared sub-segments for the current section.
		shared_sub_segment_seq_type shared_sub_segments;
		get_resolved_topological_section_shared_sub_segments(
				shared_sub_segments,
				resolved_sub_segment_marker_seq,
				section_geometry,
				section_rg,
				section_feature_ref);

		// Now that we've gathered all the shared sub-segments for the current section,
		// add them to a ResolvedTopologicalSection.
		resolved_topological_sections.push_back(
				ResolvedTopologicalSection::create(
						shared_sub_segments.begin(),
						shared_sub_segments.end(),
						section_rg,
						section_feature_ref));
	}
}


void
GPlatesAppLogic::TopologyUtils::map_resolved_topological_boundaries_networks_to_shared_sub_segments(
		resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &resolved_topological_shared_sub_segments_map,
		const std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections)
{
	resolved_topological_shared_sub_segments_map.clear();

	for (auto resolved_topological_section : resolved_topological_sections)
	{
		for (auto shared_sub_segment : resolved_topological_section->get_shared_sub_segments())
		{
			// Add the current shared sub-segment to all the resolved topologies sharing it (and add their reversal flags).
			for (const auto &sharing_resolved_topology : shared_sub_segment->get_sharing_resolved_topologies())
			{
				// Get the geometry property referenced by the resolved topology.
				boost::optional<GPlatesModel::FeatureHandle::iterator> resolved_topology_geometry_property =
						ReconstructionGeometryUtils::get_geometry_property_iterator(sharing_resolved_topology.resolved_topology);
				if (resolved_topology_geometry_property)  // this should always succeed
				{
					resolved_topological_shared_sub_segments_map[resolved_topology_geometry_property.get()].push_back(
						{ shared_sub_segment, sharing_resolved_topology.is_sub_segment_geometry_reversed });
				}
			}
		}
	}
}
