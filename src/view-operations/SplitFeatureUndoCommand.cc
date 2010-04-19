/* $Id$ */

/**
 * \file 
 * 
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

#include "SplitFeatureUndoCommand.h"

#include "utils/GeometryUtils.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/GeometrySetter.h"
#include "feature-visitors/PropertyValueFinder.h"
void
GPlatesViewOperations::SplitFeatureUndoCommand::redo()
{
	RenderedGeometryCollection::UpdateGuard update_guard;

	d_old_feature = 
		d_feature_focus->focused_feature();
	if(!(*d_old_feature).is_valid())
	{
		return;
	}
	d_feature_collection_ref = 
		*GPlatesAppLogic::get_feature_collection_containing_feature(
				d_view_state->get_application_state().get_feature_collection_file_state(),
				*d_old_feature);
	if(!d_feature_collection_ref.is_valid())
	{
		return;
	}
	// We exclude geometry property when cloning the feature because the new
	// split geometry property will be appended to the cloned feature later.
	d_new_feature = (*d_old_feature)->clone(
			d_feature_collection_ref,
			&GPlatesFeatureVisitors::is_not_geometry_property);
#if 0
	d_new_feature_2 = feature_ref->clone(
			d_feature_collection_ref,
			&GPlatesFeatureVisitors::is_not_geometry_property);
#endif
	GPlatesModel::FeatureHandle::iterator property_iter = 
		*GPlatesFeatureVisitors::find_first_geometry_property(*d_old_feature);

	//Here we assume there is only one geometry in the feature
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
		GPlatesFeatureVisitors::find_first_geometry(property_iter);

	std::vector<GPlatesMaths::PointOnSphere> points;
	GPlatesUtils::GeometryUtils::get_geometry_points(
			*geometry_on_sphere,
			points);

	GPlatesModel::PropertyName property_name = 
		(*property_iter)->property_name(); 

#if 0
	//we remove the geometry from the feature and then add the new one into it
	GPlatesUtils::GeometryUtils::remove_geometry_properties_from_feature(*d_old_feature);
#endif

	//keep the old geometry property for "Undo"
	d_old_geometry_property = 
		GPlatesModel::TopLevelPropertyInline::create(
				property_name,
				*GPlatesUtils::GeometryUtils::create_geometry_property_value(
						points.begin(), 
						points.end(),
						GPlatesViewOperations::GeometryType::POLYLINE));
	
	//we need to reverse reconstruct the inserted point to present day first
	if (d_oriented_pos_on_globe)
	{
		const GPlatesModel::ReconstructedFeatureGeometry *rfg = NULL;
		if (GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
				d_feature_focus->associated_reconstruction_geometry(), rfg))
		{
			GPlatesMaths::PointOnSphere reconstructed_point =
			GPlatesAppLogic::ReconstructUtils::reconstruct(
					*d_oriented_pos_on_globe,
					*rfg->reconstruction_plate_id(),
					d_view_state->get_reconstruct().get_current_reconstruction().reconstruction_tree(),
					true);
		
			points.insert(
					points.begin()+ d_point_index_to_insert_at, 
					reconstructed_point);
		}
		else
		{
			points.insert(
					points.begin()+ d_point_index_to_insert_at, 
					*d_oriented_pos_on_globe);
		}
	}
	GeometryBuilder::PointIndex point_index_to_split;
	point_index_to_split = d_point_index_to_insert_at + 1;
	
	//if the point is at the beginning or the end of the ployline, we do nothing but return
	if (d_point_index_to_insert_at == 0 ||
		(points.begin() + d_point_index_to_insert_at + 1) == points.end())
	{
		return;
	}

	GPlatesFeatureVisitors::GeometrySetter geometry_setter(
			GPlatesMaths::PolylineOnSphere::create_on_heap(
					points.begin(), 
					points.begin() + point_index_to_split));

	//FIXME:
	//since the non-const value finder has been disabled, 
	//use const_cast as workaround for now
	const GPlatesPropertyValues::GmlLineString* const_val;
	GPlatesFeatureVisitors::get_property_value(
			*d_old_feature,	
			property_name,
			const_val);
	GPlatesPropertyValues::GmlLineString* val=
		const_cast<GPlatesPropertyValues::GmlLineString*>(const_val);
	geometry_setter.set_geometry(val);

	// TODO: currently the ploy line type has been hard-coded here, 
	// we need to support other geometry type in the future
#if 0
	(*d_old_feature)->add(
			GPlatesModel::TopLevelPropertyInline::create(
				property_name,
				*GPlatesUtils::GeometryUtils::create_geometry_property_value(
						points.begin(), 
						points.begin() + point_index_to_split,
						GPlatesViewOperations::GeometryType::POLYLINE)));
#endif
	
	(*d_new_feature)->add(
			GPlatesModel::TopLevelPropertyInline::create(
				property_name,
				*GPlatesUtils::GeometryUtils::create_geometry_property_value(
						points.begin() + point_index_to_split -1, 
						points.end(),
						GPlatesViewOperations::GeometryType::POLYLINE)));

	d_feature_focus->set_focus(
			*d_old_feature,
			*GPlatesFeatureVisitors::find_first_geometry_property(
					*d_old_feature));
	d_feature_focus->announce_modification_of_focused_feature();
}

void
GPlatesViewOperations::SplitFeatureUndoCommand::undo()
{
	RenderedGeometryCollection::UpdateGuard update_guard;
	
	//restore the old geometry
	GPlatesUtils::GeometryUtils::remove_geometry_properties_from_feature(*d_old_feature);
	(*d_old_feature)->add(*d_old_geometry_property);

	GPlatesModel::ModelUtils::remove_feature(
			d_feature_collection_ref, *d_new_feature);

	//TODO: FIXME:
	//The following code is a kind of hacking. But if we don't set_focus in undo,
	//the behavior of GPlates will be quite annoying. This is the reason why I want to do it anyway,
	//even if the Undo object has been destroyed in the middle of this.
	//DON'T USE ANY DATA OF "UNDO OBJECT" AFTER SETTING FOCUS.
#if 1	
	GPlatesModel::FeatureHandle::iterator it=
			*GPlatesFeatureVisitors::find_first_geometry_property(
					*d_old_feature);
	
	GPlatesGui::FeatureFocus * focus=d_feature_focus;
	focus->set_focus(
			*d_old_feature,
			it);
	focus->announce_modification_of_focused_feature();
#endif

#if 0
	GPlatesModel::ModelUtils::remove_feature(
			d_feature_collection_ref, *d_new_feature_2);
#endif
}

