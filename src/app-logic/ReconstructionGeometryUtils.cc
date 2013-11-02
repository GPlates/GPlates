/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ReconstructionGeometryUtils.h"

#include "MultiPointVectorField.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFlowline.h"
#include "ReconstructedMotionPath.h"
#include "ReconstructedSmallCircle.h"
#include "ReconstructedVirtualGeomagneticPole.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryFinder.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalNetwork.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Returns those reconstruction geometries found by @a rg_finder that are in the subset
		 * @a reconstruction_geometries_subset.
		 */
		bool
		get_reconstruction_geometries_subset(
				ReconstructionGeometryUtils::reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
				const ReconstructionGeometryUtils::reconstruction_geom_seq_type &reconstruction_geometries_subset,
				const ReconstructionGeometryFinder &rg_finder)
		{
			bool found_match = false;

			ReconstructionGeometryFinder::rg_container_type::const_iterator found_rg_iter;
			for (found_rg_iter = rg_finder.found_rgs_begin();
				found_rg_iter != rg_finder.found_rgs_end();
				++found_rg_iter)
			{
				const ReconstructionGeometry::non_null_ptr_type &found_rg = *found_rg_iter;

				// If the found reconstruction geometry is not in the input reconstruction geometries then skip it.
				if (std::find(
					reconstruction_geometries_subset.begin(),
					reconstruction_geometries_subset.end(),
					found_rg) == reconstruction_geometries_subset.end())
				{
					continue;
				}

				// We found a match so add it to the caller's sequence.
				reconstruction_geometries_observing_feature.push_back(found_rg);

				found_match = true;
			}

			return found_match;
		}
	}
}


bool
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const reconstruction_geom_seq_type &reconstruction_geometries_subset,
		const ReconstructionGeometry &reconstruction_geometry,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	// Get the feature referenced by the old reconstruction geometry.
	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			ReconstructionGeometryUtils::get_feature_ref(&reconstruction_geometry);
	if (!feature_ref)
	{
		return false;
	}

	// Get the geometry property iterator from the old reconstruction geometry.
	boost::optional<GPlatesModel::FeatureHandle::iterator> geometry_property =
			ReconstructionGeometryUtils::get_geometry_property_iterator(&reconstruction_geometry);
	if (!geometry_property)
	{
		return false;
	}

	return find_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			reconstruction_geometries_subset,
			feature_ref.get(),
			geometry_property.get(),
			reconstruct_handles);
}


bool
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const reconstruction_geom_seq_type &reconstruction_geometries_subset,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	if ( !feature_ref.is_valid())
	{
		return false;
	}

	//
	// Iterate through the ReconstructionGeometrys that are observing 'feature_ref'.
	//
	// Of those ReconstructionGeometries, we're only interested in those that exist inside
	// the reconstruction geometries subset passed to us.
	//

	// Iterate over the ReconstructionGeometries that observe 'feature_ref' and were optionally
	// reconstructed from the reconstruction tree.
	ReconstructionGeometryFinder rg_finder(reconstruct_handles);
	rg_finder.find_rgs_of_feature(feature_ref);

	return get_reconstruction_geometries_subset(
			reconstruction_geometries_observing_feature, reconstruction_geometries_subset, rg_finder);
}


bool
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const reconstruction_geom_seq_type &reconstruction_geometries_subset,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	if ( !feature_ref.is_valid() || !geometry_property_iterator.is_still_valid())
	{
		return false;
	}

	//
	// Iterate through the ReconstructionGeometrys that are observing 'feature_ref' and
	// that were generated from 'feature_ref's geometry property 'geometry_property_iterator'.
	//
	// Of those ReconstructionGeometries, we're only interested in those that exist inside
	// the reconstruction geometries subset passed to us.
	//

	// Iterate over the ReconstructionGeometries that observe 'feature_ref' and were reconstructed
	// from its 'geometry_property_iterator' feature property and optionally from the reconstruction tree.
	ReconstructionGeometryFinder rg_finder(geometry_property_iterator, reconstruct_handles);
	rg_finder.find_rgs_of_feature(feature_ref);

	return get_reconstruction_geometries_subset(
			reconstruction_geometries_observing_feature, reconstruction_geometries_subset, rg_finder);
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetFeatureRef::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// A MultiPointVectorField references both a velocity point location and
	// a plate polygon of some sort.
	// Here we just return whichever feature reference is stored in the
	// MultiPointVectorField object itself - currently this is velocity point location.
	d_feature_ref = mpvf->get_feature_ref();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetFeatureRef::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_feature_ref = rfg->get_feature_ref();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetFeatureRef::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	d_feature_ref = rtg->get_feature_ref();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetFeatureRef::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_feature_ref = rtn->get_feature_ref();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetGeometryProperty::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// A MultiPointVectorField references both a velocity point location and
	// a plate polygon of some sort.
	// Here we just return whichever geometry property is stored in the
	// MultiPointVectorField object itself - currently this is velocity point location.
	d_property = mpvf->property();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetGeometryProperty::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_property = rfg->property();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetGeometryProperty::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	d_property = rtg->property();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetGeometryProperty::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_property = rtn->property();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetPlateId::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// A MultiPointVectorField instance does not correspond to any
	// single plate, and hence does not contain a plate ID, so nothing
	// to do here.
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetPlateId::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_plate_id = rfg->reconstruction_plate_id();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetPlateId::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	d_plate_id = rtg->plate_id();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetPlateId::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_plate_id = rtn->plate_id();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetTimeOfFormation::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// A MultiPointVectorField instance does not reference a feature,
	// and hence there is no time of formation, so nothing to do here.
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetTimeOfFormation::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_time_of_formation = rfg->time_of_formation();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetTimeOfFormation::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	d_time_of_formation = rtg->time_of_formation();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetTimeOfFormation::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_time_of_formation = rtn->time_of_formation();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTree::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// multi_point_vector_field_type does not need/support reconstruction trees.
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTree::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_reconstruction_tree = d_reconstruction_time
			? rfg->get_reconstruction_tree_creator().get_reconstruction_tree(d_reconstruction_time.get())
			: rfg->get_reconstruction_tree();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTree::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	d_reconstruction_tree = d_reconstruction_time
			? rtg->get_reconstruction_tree_creator().get_reconstruction_tree(d_reconstruction_time.get())
			: rtg->get_reconstruction_tree();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTree::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	// resolved_topological_network_type does not need/support reconstruction trees.
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTreeCreator::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// multi_point_vector_field_type does not need/support reconstruction trees.
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTreeCreator::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_reconstruction_tree_creator = rfg->get_reconstruction_tree_creator();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTreeCreator::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	d_reconstruction_tree_creator = rtg->get_reconstruction_tree_creator();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetReconstructionTreeCreator::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	// resolved_topological_network_type does not need/support reconstruction trees.
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetResolvedTopologicalBoundarySubSegmentSequence::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	// Only a resolved topological geometry with a *polygon* is a resolved topological *boundary*.
	if (rtg->resolved_topology_boundary())
	{
		d_sub_segment_sequence = rtg->get_sub_segment_sequence();
	}
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetResolvedTopologicalBoundarySubSegmentSequence::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_sub_segment_sequence = rtn->get_boundary_sub_segment_sequence();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetResolvedTopologicalBoundaryPolygon::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	// See if the resolved topology geometry is a polygon.
	// It might be a polyline in which case boost::none is returned.
	d_boundary_polygon = rtg->resolved_topology_boundary();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetResolvedTopologicalBoundaryPolygon::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_boundary_polygon = rtn->boundary_polygon();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetBoundaryPolygon::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	// See if the reconstructed feature geometry is a polygon.
	// It might be a polyline in which case boost::none is returned.
	d_boundary_polygon = GeometryUtils::get_polygon_on_sphere(*rfg->reconstructed_geometry());
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetBoundaryPolygon::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	// See if the resolved topology geometry is a polygon.
	// It might be a polyline in which case boost::none is returned.
	d_boundary_polygon = rtg->resolved_topology_boundary();
}


void
GPlatesAppLogic::ReconstructionGeometryUtils::GetBoundaryPolygon::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_boundary_polygon = rtn->boundary_polygon();
}
