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
#include <iterator>
#include <map>
#include <utility>
#include <vector>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "AppLogicUtils.h"
#include "GeometryUtils.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolver.h"
#include "TopologyInternalUtils.h"
#include "TopologyNetworkResolver.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GeometryDistance.h"
#include "maths/PointInPolygon.h"
#include "maths/PolygonIntersections.h"
#include "maths/Real.h"

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
					const ResolvedTopologicalGeometrySubSegment &sub_segment_,
					const ReconstructionGeometry::non_null_ptr_type &resolved_topology_) :
				sub_segment(sub_segment_),
				resolved_topology(resolved_topology_)
			{  }

			ResolvedTopologicalGeometrySubSegment sub_segment;
			// The resolved topology that owns the sub-segment...
			ReconstructionGeometry::non_null_ptr_type resolved_topology;
		};

		// Map of each topological section to all the resolved topologies that use it for a sub-segment.
		typedef std::map<const ReconstructionGeometry *, std::vector<ResolvedSubSegmentInfo> >
				resolved_section_to_sharing_resolved_topologies_map_type;


		/**
		 * Maps each resolved topological section to all the resolved topologies that use it for a sub-segment.
		 */
		void
		map_resolved_topological_sections_to_resolved_topologies(
				resolved_section_to_sharing_resolved_topologies_map_type &resolved_section_to_sharing_resolved_topologies_map,
				const ReconstructionGeometry::non_null_ptr_type &resolved_topology,
				const sub_segment_seq_type &section_sub_segments)
		{
			// Iterate over the sub-segments of the current topology.
			sub_segment_seq_type::const_iterator sub_segments_iter = section_sub_segments.begin();
			sub_segment_seq_type::const_iterator sub_segments_end = section_sub_segments.end();
			for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
			{
				const ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segments_iter;

				// Skip sub-segments that are the result of joining adjacent deforming points.
				//
				// Joining adjacent deforming points was meant to be a temporary hack to be removed
				// when resolved *line* topologies were implemented. However, unfortunately it seems
				// we need to keep this hack in place for any old data files that use the old method.
				//
				// The hack involves joining adjacent deforming points that are spread along a deforming zone
				// boundary such that exported sub-segments are lines instead of a sequence of points.
				//
				// Skipping these sub-segments will result in missing sub-segments but the data should
				// be using topological lines anyway (which avoids the problem). We could hack around
				// this situation (instead of just skipping sub-segments) but each joined sub-segment
				// only references one of (joined) point features/recon-geoms, so the whole idea of
				// sub-segments sharing topological section feature/recon-geom doesn't really work.
				if (sub_segment.get_joined_adjacent_deforming_points())
				{
					continue;
				}

				// Add the current resolved topology to the list of those sharing the current section.
				resolved_section_to_sharing_resolved_topologies_map[sub_segment.get_reconstruction_geometry().get()]
						.push_back(ResolvedSubSegmentInfo(sub_segment, resolved_topology));
			}
		}


		// The start or end point of a sub-segment (on a particular polyline segment/arc of section polyline).
		struct ResolvedSubSegmentMarker
		{
			ResolvedSubSegmentMarker(
					const ReconstructionGeometry::non_null_ptr_type &resolved_topology_,
					bool is_sub_segment_geometry_reversed_,
					const GPlatesMaths::PointOnSphere &sub_segment_point_,
					bool is_sub_segment_start_,
					unsigned int section_polyline_segment_index_,
					const GPlatesMaths::Real &dot_polyline_segment_start_point_) :
				resolved_topology_info(resolved_topology_, is_sub_segment_geometry_reversed_),
				sub_segment_point(sub_segment_point_),
				is_sub_segment_start(is_sub_segment_start_),
				section_polyline_segment_index(section_polyline_segment_index_),
				dot_polyline_segment_start_point(dot_polyline_segment_start_point_)
			{  }

			// The resolved topology that owns the sub-segment (and its geometry reversal flag)...
			ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo resolved_topology_info;

			GPlatesMaths::PointOnSphere sub_segment_point;
			bool is_sub_segment_start; // It's either the sub-segment start point or end point.

			// Index of section polyline segment/arc on which this marker lies.
			unsigned int section_polyline_segment_index;

			// The distance (dot product) of sub-segment start/end point from the start point
			// of the polyline segment/arc that it resides in.
			GPlatesMaths::Real dot_polyline_segment_start_point;
		};

		typedef std::vector<ResolvedSubSegmentMarker> resolved_sub_segment_marker_seq_type;

		// All sub-segment markers on the same polyline segment/arc.
		struct ResolvedSectionPolylineSegmentMarkers
		{
			resolved_sub_segment_marker_seq_type sub_segment_markers;
		};

		typedef std::vector<ResolvedSectionPolylineSegmentMarkers> resolved_section_polyline_segment_markers_seq_type;

		// Map polyline segment/arc indices into above sequence since not all polyline segments/arcs will
		// contain markers (in fact very few will) - so this just saves a bit of memory and set up time.
		typedef std::map<unsigned int, unsigned int>
				resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type;


		/**
		 * Record the start/end point locations of each sub-segment within the section geometry.
		 */
		void
		find_resolved_topological_section_sub_segment_markers(
				resolved_section_polyline_segment_markers_seq_type &resolved_section_polyline_segment_markers_seq,
				resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type &
						resolved_section_polyline_segment_index_to_polyline_segment_markers_map,
				const std::vector<ResolvedSubSegmentInfo> &section_sub_segment_infos,
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &section_polyline,
				const GPlatesMaths::AngularExtent &minimum_distance_threshold)
		{
			// Iterate over the sub-segments referencing the section.
			std::vector<ResolvedSubSegmentInfo>::const_iterator sub_segments_iter = section_sub_segment_infos.begin();
			std::vector<ResolvedSubSegmentInfo>::const_iterator sub_segments_end = section_sub_segment_infos.end();
			for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
			{
				const ResolvedSubSegmentInfo &sub_segment_info = *sub_segments_iter;

				// Get the start/end points of the current sub-segment geometry.
				std::pair<
						GPlatesMaths::PointOnSphere/*start point*/,
						GPlatesMaths::PointOnSphere/*end point*/> sub_segment_end_points =
								GeometryUtils::get_geometry_exterior_end_points(
										*sub_segment_info.sub_segment.get_geometry());

				enum { START, END };

				// Find the closest position (and segment arc) of the sub-segment start/end point to the section polyline.
				GPlatesMaths::UnitVector3D closest_start_end_positions_on_section_polyline[2] =
				{
					GPlatesMaths::UnitVector3D::xBasis()/*dummy value*/,
					GPlatesMaths::UnitVector3D::xBasis()/*dummy value*/
				};
				unsigned int closest_start_end_segment_indices_in_section_polyline[2];
				// 'minimum_distance()' should be quite efficient since internally it uses a bounding tree
				// over its segments/arcs to make point location efficient.
				if (minimum_distance(
						sub_segment_end_points.first/*start point*/,
						*section_polyline,
						minimum_distance_threshold,
						closest_start_end_positions_on_section_polyline[START],
						closest_start_end_segment_indices_in_section_polyline[START])
								== GPlatesMaths::AngularDistance::PI ||
					minimum_distance(
						sub_segment_end_points.second/*end point*/,
						*section_polyline,
						minimum_distance_threshold,
						closest_start_end_positions_on_section_polyline[END],
						closest_start_end_segment_indices_in_section_polyline[END])
								== GPlatesMaths::AngularDistance::PI)
				{
					// We shouldn't get here - sub-segment geometry should be a subset of the section geometry.
					qWarning() << "Expected topological sub-segment geometry to be a subset of section geometry.";

					// Continue to the next sub-segment.
					continue;
				}
				GPlatesMaths::PointOnSphere closest_start_end_point_on_section_polyline[2] =
				{
					GPlatesMaths::PointOnSphere(closest_start_end_positions_on_section_polyline[START]),
					GPlatesMaths::PointOnSphere(closest_start_end_positions_on_section_polyline[END])
				};

				// If the sub-segment start point equals the start point of the polyline segment/arc it lies on
				// (and its not the first polyline segment/arc) then move it to the end point of the previous segment/arc.
				// This makes it easier to later emit shared sub-segments that avoid duplicating the start vertex.
				if (closest_start_end_point_on_section_polyline[START] ==
						section_polyline->get_segment(closest_start_end_segment_indices_in_section_polyline[START]).start_point() &&
					closest_start_end_segment_indices_in_section_polyline[START] > 0)
				{
					--closest_start_end_segment_indices_in_section_polyline[START];
				}
				// If the sub-segment end point equals the end point of the polyline segment/arc it lies on
				// (and its not the last polyline segment/arc) then move it to the start point of the next segment/arc.
				// This makes it easier to later emit shared sub-segments that avoid duplicating the end vertex.
				if (closest_start_end_point_on_section_polyline[END] ==
						section_polyline->get_segment(closest_start_end_segment_indices_in_section_polyline[END]).end_point() &&
					closest_start_end_segment_indices_in_section_polyline[END] < section_polyline->number_of_segments() - 1)
				{
					++closest_start_end_segment_indices_in_section_polyline[END];
				}

				GPlatesMaths::Real dot_sub_segment_start_end_point_and_polyline_segment_start_point[2];
				unsigned int n;
				for (n = 0; n < 2; ++n) // n is 'START' and 'END'
				{
					// The section polyline segment/arc that the sub-segment start/end point lies on.
					const GPlatesMaths::GreatCircleArc &start_end_section_polyline_segment =
							section_polyline->get_segment(closest_start_end_segment_indices_in_section_polyline[n]);

					dot_sub_segment_start_end_point_and_polyline_segment_start_point[n] = dot(
							closest_start_end_point_on_section_polyline[n].position_vector(),
							start_end_section_polyline_segment.start_point().position_vector());
				}

				// Check that the sub-segment end point is not before the start point.
				// This shouldn't happen since all sub-segment geometries are in their un-reversed forms
				// and so they should progress (from start point to end point) along the same direction as
				// the section polyline.
				// But we'll check just in case (eg, some numerical tolerances issues might pop up).
				//
				// Needs swapping if either:
				// (1) the sub-segment end point polyline segment/arc index is less than the start, or
				// (2) they are equal and the end point is closer to the start of polyline segment/arc.
				if ((closest_start_end_segment_indices_in_section_polyline[END] <
						closest_start_end_segment_indices_in_section_polyline[START]) ||
					(closest_start_end_segment_indices_in_section_polyline[END] ==
						closest_start_end_segment_indices_in_section_polyline[START] &&
					dot_sub_segment_start_end_point_and_polyline_segment_start_point[END].dval() >
						dot_sub_segment_start_end_point_and_polyline_segment_start_point[START].dval()))
				{
					std::swap(sub_segment_end_points.first, sub_segment_end_points.second);
					std::swap(
							closest_start_end_positions_on_section_polyline[START],
							closest_start_end_positions_on_section_polyline[END]);
					std::swap(
							closest_start_end_point_on_section_polyline[START],
							closest_start_end_point_on_section_polyline[END]);
					std::swap(
							closest_start_end_segment_indices_in_section_polyline[START],
							closest_start_end_segment_indices_in_section_polyline[END]);
					std::swap(
							dot_sub_segment_start_end_point_and_polyline_segment_start_point[START],
							dot_sub_segment_start_end_point_and_polyline_segment_start_point[END]);
				}

				for (n = 0; n < 2; ++n) // n is 'START' and 'END'
				{
					// Insert the sub-segment start/end point markers.
					const std::pair<resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type::iterator, bool>
							polyline_segment_index_insert_result =
									resolved_section_polyline_segment_index_to_polyline_segment_markers_map.insert(
											resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type::value_type(
													closest_start_end_segment_indices_in_section_polyline[n],
													resolved_section_polyline_segment_markers_seq.size()/*index*/));
					if (polyline_segment_index_insert_result.second)
					{
						// Insertion successful - first time seen polyline segment.
						resolved_section_polyline_segment_markers_seq.push_back(ResolvedSectionPolylineSegmentMarkers());
					}
					ResolvedSectionPolylineSegmentMarkers &polyline_segment_markers =
							resolved_section_polyline_segment_markers_seq[
									polyline_segment_index_insert_result.first->second/*index*/];

					// Iterate over the existing polyline segment markers (in the polyline segment/arc) and insert
					// a new marker in the correct order (according to distance from polyline segment/arc start point).

					resolved_sub_segment_marker_seq_type::iterator sub_segment_markers_iter =
							polyline_segment_markers.sub_segment_markers.begin();
					const resolved_sub_segment_marker_seq_type::iterator sub_segment_markers_end =
							polyline_segment_markers.sub_segment_markers.end();
					for ( ; sub_segment_markers_iter != sub_segment_markers_end; ++sub_segment_markers_iter)
					{
						const ResolvedSubSegmentMarker &sub_segment_marker = *sub_segment_markers_iter;

						// Next test if dot product exceeds previously inserted sub-segment marker position.
						//
						// This is an *epsilon* test to ensure that subsequently added markers with the
						// same position are inserted *after* the previously added marker (with same position).
						// This ensures *end* markers get added after *start* markers if they have the same position.
						if (dot_sub_segment_start_end_point_and_polyline_segment_start_point[n] >
								sub_segment_marker.dot_polyline_segment_start_point)
						{
							break;
						}
					}

					polyline_segment_markers.sub_segment_markers.insert(
							sub_segment_markers_iter,
							ResolvedSubSegmentMarker(
									sub_segment_info.resolved_topology,
									sub_segment_info.sub_segment.get_use_reverse(),
									closest_start_end_point_on_section_polyline[n],
									n == 0/*is_sub_segment_start*/,
									closest_start_end_segment_indices_in_section_polyline[n],
									dot_sub_segment_start_end_point_and_polyline_segment_start_point[n]));
				}
			}
		}


		/**
		 * Iterate over the resolved section polyline segment markers and emit shared sub-segments for the section.
		 */
		void
		get_resolved_topological_section_shared_sub_segments(
				std::vector<ResolvedTopologicalSharedSubSegment> &shared_sub_segments,
				const resolved_section_polyline_segment_markers_seq_type &resolved_section_polyline_segment_markers_seq,
				const resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type &
						resolved_section_polyline_segment_index_to_polyline_segment_markers_map,
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &section_polyline,
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_rg,
				const GPlatesModel::FeatureHandle::const_weak_ref &section_feature_ref)
		{
			// As we progress through the sub-segment markers the list of resolved topologies sharing a
			// sub-segment will change.
			std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> sharing_resolved_topologies;

			boost::optional<const ResolvedSubSegmentMarker &> prev_sub_segment_marker;

			// Iterate over the polygon segment markers sequence (map).
			// This iterates from high polyline segment indices to low ones (ie, from start to end of section polyline).
			resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type::const_iterator
					polyline_segment_index_entries_iter =
							resolved_section_polyline_segment_index_to_polyline_segment_markers_map.begin();
			const resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type::const_iterator
					polyline_segment_index_entries_end =
							resolved_section_polyline_segment_index_to_polyline_segment_markers_map.end();
			for ( ;
				polyline_segment_index_entries_iter != polyline_segment_index_entries_end;
				++polyline_segment_index_entries_iter)
			{
				const resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type::value_type &
						polyline_segment_index_entry = *polyline_segment_index_entries_iter;
				const ResolvedSectionPolylineSegmentMarkers &polyline_segment_markers =
						resolved_section_polyline_segment_markers_seq[polyline_segment_index_entry.second];

				// Iterate over the sub-segment markers in the current polyline segment/arc.
				resolved_sub_segment_marker_seq_type::const_iterator sub_segment_markers_iter =
						polyline_segment_markers.sub_segment_markers.begin();
				const resolved_sub_segment_marker_seq_type::const_iterator sub_segment_markers_end =
						polyline_segment_markers.sub_segment_markers.end();
				for ( ;
					sub_segment_markers_iter != sub_segment_markers_end;
					(prev_sub_segment_marker = *sub_segment_markers_iter), ++sub_segment_markers_iter)
				{
					const ResolvedSubSegmentMarker &sub_segment_marker = *sub_segment_markers_iter;

					// If the previous and current marker positions are different then there is a
					// shared sub-segment between them that can be emitted.
					// However there might not be any resolved topologies. This can happen if last marker
					// was an *end* marker and current marker is a *start* marker and there are no
					// resolved topologies referencing the part of the section between those markers.
					// Also if a resolved topology has a sub-segment that is a point (and not a line)
					// then its start and end marker positions will be the same and hence that resolved
					// topology will not have its (point) sub-segment emitted - this is fine since it's
					// zero length anyway and therefore doesn't really contribute as a topological section.
					if (prev_sub_segment_marker &&
						sub_segment_marker.sub_segment_point != prev_sub_segment_marker->sub_segment_point &&
						!sharing_resolved_topologies.empty())
					{
						std::vector<GPlatesMaths::PointOnSphere> shared_sub_segment_points;

						// Add the previous sub-segment marker point unless it is equal to the
						// end point of the polyline segment/arc that the marker lies on
						// (because it will get added below as part of the section polyline).
						if (prev_sub_segment_marker->sub_segment_point !=
							section_polyline->get_vertex(prev_sub_segment_marker->section_polyline_segment_index + 1))
						{
							shared_sub_segment_points.push_back(prev_sub_segment_marker->sub_segment_point);
						}

						// Copy the points (if any) of the section polyline between the previous and
						// current sub-segment markers.
						std::copy(
								section_polyline->vertex_begin() +
										prev_sub_segment_marker->section_polyline_segment_index + 1,
								section_polyline->vertex_begin() +
										sub_segment_marker.section_polyline_segment_index + 1,
								std::back_inserter(shared_sub_segment_points));

						// Add the current sub-segment marker point unless it is equal to the
						// start point of the polyline segment/arc that the marker lies on
						// (because it's already been added above as part of the section polyline).
						if (sub_segment_marker.sub_segment_point !=
							section_polyline->get_vertex(sub_segment_marker.section_polyline_segment_index))
						{
							shared_sub_segment_points.push_back(sub_segment_marker.sub_segment_point);
						}

						// Due to numerical tolerance in the epsilon-point-equality tests it might be
						// possible to end up with *one* point here (instead of the minimum two required
						// for a polyline) if both segment markers are almost exactly the same position
						// and almost exactly the same as a section polyline vertex.
						if (shared_sub_segment_points.size() >= 2)
						{
							const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type shared_sub_segment_geometry =
									GPlatesMaths::PolylineOnSphere::create_on_heap(
											shared_sub_segment_points.begin(),
											shared_sub_segment_points.end());

							// Add a shared sub-segment for the current uniquely shared sub-segment geometry and
							// those resolved topologies sharing it.
							shared_sub_segments.push_back(
									ResolvedTopologicalSharedSubSegment(
											shared_sub_segment_geometry,
											section_rg,
											section_feature_ref,
											sharing_resolved_topologies));
						}
					}

					if (sub_segment_marker.is_sub_segment_start)
					{
						// We've reached a *start* sub-segment marker.
						// So add the sharing resolved topology of the current sub-segment marker.
						sharing_resolved_topologies.push_back(sub_segment_marker.resolved_topology_info);
					}
					else // sub-segment end point ...
					{
						// We've reached an *end* sub-segment marker.
						// So remove the sharing resolved topology of the current sub-segment marker.
						std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo>::iterator
								sharing_resolved_topology_iter = sharing_resolved_topologies.begin();
						const std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo>::iterator
								sharing_resolved_topology_end = sharing_resolved_topologies.end();
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
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles)
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

	AppLogicUtils::visit_feature_collections(
			topological_line_features_collection.begin(),
			topological_line_features_collection.end(),
			topology_line_resolver);

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_line_features,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles)
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

	AppLogicUtils::visit_features(
			topological_line_features.begin(),
			topological_line_features.end(),
			topology_line_resolver);

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
		const TopologyNetworkParams &topology_network_params,
		boost::optional<std::set<GPlatesModel::FeatureId> &> topological_sections_referenced)
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
			topology_network_params,
			topological_sections_referenced);

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
		const TopologyNetworkParams &topology_network_params,
		boost::optional<std::set<GPlatesModel::FeatureId> &> topological_sections_referenced)
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
			topology_network_params,
			topological_sections_referenced);

	AppLogicUtils::visit_features(
			topological_network_features.begin(),
			topological_network_features.end(),
			topology_network_resolver);

	return reconstruct_handle;
}


void
GPlatesAppLogic::TopologyUtils::find_resolved_topological_sections(
		std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
		const std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks)
{
	//
	// Find all topological sections referenced by the resolved topologies.
	// And build a list of resolved topologies (and their sub-segments) that reference each topological section.
	//

	resolved_section_to_sharing_resolved_topologies_map_type resolved_section_to_sharing_resolved_topologies_map;

	// Iterate over the plate polygons.
	BOOST_FOREACH(
			const ResolvedTopologicalBoundary::non_null_ptr_type &resolved_topological_boundary,
			resolved_topological_boundaries)
	{
		map_resolved_topological_sections_to_resolved_topologies(
				resolved_section_to_sharing_resolved_topologies_map,
				resolved_topological_boundary,
				resolved_topological_boundary->get_sub_segment_sequence());
	}

	// Iterate over the deforming networks.
	BOOST_FOREACH(
			const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_topological_network,
			resolved_topological_networks)
	{
		map_resolved_topological_sections_to_resolved_topologies(
				resolved_section_to_sharing_resolved_topologies_map,
				resolved_topological_network,
				resolved_topological_network->get_boundary_sub_segment_sequence());
	}


	//
	// For each topological section (referenced by the resolved topologies) build sub-segments that are
	// uniquely shared by one or more resolved topologies.
	//


	// The minimum distance threshold of sub-segment start/end points from a section's polyline.
	// This threshold is quite relaxed compared to that used for equality of points
	// (which currently is 'dot_product > 1 - 1e-12').
	// If the minimum distance is exceeds this threshold then something is wrong because the
	// sub-segment geometry should be a subset of the section geometry.
	const GPlatesMaths::AngularExtent minimum_distance_threshold =
			GPlatesMaths::AngularExtent::create_from_cosine(1 - 1e-6);

	// Iterate over the sections referenced by resolved topologies.
	BOOST_FOREACH(
			const resolved_section_to_sharing_resolved_topologies_map_type::value_type &
					section_to_sharing_resolved_topologies_entry,
			resolved_section_to_sharing_resolved_topologies_map)
	{
		const ReconstructionGeometry *section_rg = section_to_sharing_resolved_topologies_entry.first;
		const std::vector<ResolvedSubSegmentInfo> &sub_segments = section_to_sharing_resolved_topologies_entry.second;

		//
		// Get the current section geometry as a polyline.
		// The segment is either a reconstructed feature geometry or a resolved topological *line*.
		//

		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> section_geometry =
				ReconstructionGeometryUtils::get_resolved_topological_boundary_section_geometry(section_rg);
		if (!section_geometry)
		{
			// We shouldn't get here - topological networks and boundaries should only reference
			// RFGs and resolved topological lines.
			qWarning() << "Expected topological section to be either a reconstructed feature geometry "
					"or a resolved topological line.";

			// Continue to the next section.
			continue;
		}

		// All sub-segments share the same section feature (so pick any sub-segment).
		//
		// Actually this doesn't apply to the old 'joined adjacent deforming points' hack where
		// multiple point features are joined into one segment which, in turn, arbitrarily references
		// one of the deforming point features - but that is all handled as a special case.
		const GPlatesModel::FeatureHandle::const_weak_ref section_feature_ref =
				sub_segments.front().sub_segment.get_feature_ref();

		// Points and multipoints are not intersected (with neighbouring topological sections).
		// Therefore all point/multipoint sub-segments (referencing the section) have the same (unclipped) geometry.
		const GPlatesMaths::GeometryType::Value section_geometry_type =
				GeometryUtils::get_geometry_type(*section_geometry.get());
		if (section_geometry_type == GPlatesMaths::GeometryType::POINT ||
			section_geometry_type == GPlatesMaths::GeometryType::MULTIPOINT)
		{
			// Gather all resolved topologies referencing section.
			std::vector<ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> sharing_resolved_topologies;

			std::vector<ResolvedSubSegmentInfo>::const_iterator sub_segments_iter = sub_segments.begin();
			std::vector<ResolvedSubSegmentInfo>::const_iterator sub_segments_end = sub_segments.end();
			for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
			{
				const ResolvedSubSegmentInfo &sub_segment_info = *sub_segments_iter;

				sharing_resolved_topologies.push_back(
						ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo(
								sub_segment_info.resolved_topology,
								sub_segment_info.sub_segment.get_use_reverse()));
			}

			// Add a single shared sub-segment since all resolved topologies (referencing the section)
			// use the same (unclipped) sub-segment geometry.
			const std::vector<ResolvedTopologicalSharedSubSegment> shared_sub_segments(
					1,
					ResolvedTopologicalSharedSubSegment(
							section_geometry.get(),
							section_rg,
							section_feature_ref,
							sharing_resolved_topologies));

			resolved_topological_sections.push_back(
					ResolvedTopologicalSection::create(
							shared_sub_segments.begin(),
							shared_sub_segments.end(),
							section_rg,
							section_feature_ref));

			// Continue to the next section.
			continue;
		}

		// The section geometry is either a polyline or a polygon.
		// Convert it to a polyline (in most cases, due to intersections, it will already be a polyline).
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> section_polyline_opt =
				GeometryUtils::convert_geometry_to_polyline(
						*section_geometry.get(),
						// We don't care if the polygon has interior rings (just using the exterior ring)...
						false/*exclude_polygons_with_interior_rings*/);
		// A polyline or polygon should always be convertible to a polyline.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				section_polyline_opt,
				GPLATES_ASSERTION_SOURCE);
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type section_polyline = section_polyline_opt.get();

		//
		// Record the start/end point locations of each sub-segment within the section geometry.
		//

		resolved_section_polyline_segment_markers_seq_type resolved_section_polyline_segment_markers_seq;

		// Map polyline segment/arc indices into a sequence since not all polyline segments/arcs will
		// contain markers (in fact very few will) - so this just saves a bit of memory and set up time.
		resolved_section_polyline_segment_index_to_polyline_segment_markers_map_type
				resolved_section_polyline_segment_index_to_polyline_segment_markers_map;

		find_resolved_topological_section_sub_segment_markers(
				resolved_section_polyline_segment_markers_seq,
				resolved_section_polyline_segment_index_to_polyline_segment_markers_map,
				sub_segments,
				section_polyline,
				minimum_distance_threshold);

		//
		// Iterate over the polyline segment markers and emit shared sub-segments for the current section.
		//

		// Each emitted shared sub-segment will get added to this.
		std::vector<ResolvedTopologicalSharedSubSegment> shared_sub_segments;

		get_resolved_topological_section_shared_sub_segments(
				shared_sub_segments,
				resolved_section_polyline_segment_markers_seq,
				resolved_section_polyline_segment_index_to_polyline_segment_markers_map,
				section_polyline,
				section_rg,
				section_feature_ref);

		// Now that we've gathered all the shared sub-segments for the current section
		// add them to a resolved topological section.
		resolved_topological_sections.push_back(
				ResolvedTopologicalSection::create(
						shared_sub_segments.begin(),
						shared_sub_segments.end(),
						section_rg,
						section_feature_ref));
	}
}
