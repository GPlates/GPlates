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

#include "app-logic/ApplicationState.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/GeometrySetter.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/TopLevelPropertyInline.h"


void
GPlatesViewOperations::SplitFeatureUndoCommand::redo()
{
	RenderedGeometryCollection::UpdateGuard update_guard;

	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(*d_model_interface.access_model());

	d_old_feature =  d_feature_focus->focused_feature();
	if (!(*d_old_feature).is_valid())
	{
		return;
	}
	GPlatesModel::FeatureCollectionHandle *feature_collection_ptr = (*d_old_feature)->parent_ptr();
	if (!feature_collection_ptr)
	{
		return;
	}
	d_feature_collection_ref = feature_collection_ptr->reference();
	if (!d_feature_collection_ref.is_valid())
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
	boost::optional<GPlatesModel::FeatureHandle::iterator> property_iter_opt = 
		*GPlatesFeatureVisitors::find_first_geometry_property(*d_old_feature);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			property_iter_opt,
			GPLATES_ASSERTION_SOURCE);

GPlatesModel::FeatureHandle::iterator property_iter = *property_iter_opt;
	//Here we assume there is only one geometry in the feature
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
		GPlatesFeatureVisitors::find_first_geometry(property_iter);

	std::vector<GPlatesMaths::PointOnSphere> points;
	GPlatesAppLogic::GeometryUtils::get_geometry_exterior_points(
			*geometry_on_sphere,
			points);

	GPlatesModel::PropertyName property_name = 
		(*property_iter)->property_name(); 

	//keep the old geometry property for "Undo"
	GPlatesModel::PropertyValue::non_null_ptr_type old_geometry_property_value =
			GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
						points.begin(), 
						points.end(),
						GPlatesMaths::GeometryType::POLYLINE).get();

	// Attempt to create a property wrapped in the correct time-dependent wrapper based on the
	// property name (according to the GPGIM).
	d_old_geometry_property = 
		GPlatesModel::ModelUtils::create_top_level_property(
				property_name,
				old_geometry_property_value);
	// If that fails (eg, because property name not recognised) then just add an unwrapped property value.
	if (!d_old_geometry_property)
	{
		d_old_geometry_property = 
			GPlatesModel::TopLevelPropertyInline::create(
					property_name,
					old_geometry_property_value);
	}
	
	//we need to reverse reconstruct the inserted point to present day first
	if (d_oriented_pos_on_globe)
	{
		boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfg =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						const GPlatesAppLogic::ReconstructedFeatureGeometry *>(
								d_feature_focus->associated_reconstruction_geometry());
		if (rfg)
		{
			GPlatesMaths::PointOnSphere reconstructed_point =
			GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
					*d_oriented_pos_on_globe,
					*rfg.get()->reconstruction_plate_id(),
					*rfg.get()->get_reconstruction_tree(),
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
	else
	{
		//if the point is at the beginning or the end of the ployline, we do nothing but return
		if (d_point_index_to_insert_at == 0 ||
			(points.begin() + d_point_index_to_insert_at + 1) == points.end())
		{
			d_nothing_has_been_done = true;
			return;
		}
	}

#if 1
	//we remove the geometry from the feature and then add the new one into it
	GPlatesAppLogic::GeometryUtils::remove_geometry_properties_from_feature(*d_old_feature);
#endif

	GeometryBuilder::PointIndex point_index_to_split;
	point_index_to_split = d_point_index_to_insert_at + 1;
	
	GPlatesFeatureVisitors::GeometrySetter geometry_setter(
			GPlatesMaths::PolylineOnSphere::create(
					points.begin(), 
					points.begin() + point_index_to_split));
#if 0
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
#endif

#if 0
	GPlatesModel::TopLevelProperty::non_null_ptr_type clone 
		= (*property_iter)->deep_clone();
	GPlatesModel::TopLevelPropertyInline::non_null_ptr_type geom_prop_clone
		= dynamic_cast<GPlatesModel::TopLevelPropertyInline *>(clone.get());
	GPlatesPropertyValues::GmlLineString *val;

	GPlatesModel::TopLevelPropertyInline::iterator tlpi_iter = geom_prop_clone->begin();
	for (;tlpi_iter != geom_prop_clone->end(); ++tlpi_iter)
	{
		GPlatesPropertyValues::GmlLineString *curr_val =
			dynamic_cast<GPlatesPropertyValues::GmlLineString *>((*tlpi_iter).get());
		if (curr_val)
		{
			val = curr_val;
			break;
		}
	}
	if(tlpi_iter == geom_prop_clone->end())
	{
		qWarning()<<"Cannot find the proper geometry value!";
		return;
	}

	geometry_setter.set_geometry(val);
	*property_iter = geom_prop_clone;
#endif

	// TODO: currently the ploy line type has been hard-coded here, 
	// we need to support other geometry type in the future
#if 1
	GPlatesModel::PropertyValue::non_null_ptr_type before_split_point_geometry_property_value =
			GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
						points.begin(), 
						points.begin() + point_index_to_split,
						GPlatesMaths::GeometryType::POLYLINE).get();

	// Attempt to create a property wrapped in the correct time-dependent wrapper based on the
	// property name (according to the GPGIM).
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> before_split_point_geometry_property = 
		GPlatesModel::ModelUtils::create_top_level_property(
				property_name,
				before_split_point_geometry_property_value);
	// If that fails (eg, because property name not recognised) then just add an unwrapped property value.
	if (!before_split_point_geometry_property)
	{
		before_split_point_geometry_property = 
			GPlatesModel::TopLevelPropertyInline::create(
					property_name,
					before_split_point_geometry_property_value);
	}

	// Add the geometry *before* the split point to the *old* feature.
	d_old_feature.get()->add(before_split_point_geometry_property.get());
#endif
	
	GPlatesModel::PropertyValue::non_null_ptr_type after_split_point_geometry_property_value =
			GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
						points.begin() + point_index_to_split -1, 
						points.end(),
						GPlatesMaths::GeometryType::POLYLINE).get();

	// Attempt to create a property wrapped in the correct time-dependent wrapper based on the
	// property name (according to the GPGIM).
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> after_split_point_geometry_property = 
		GPlatesModel::ModelUtils::create_top_level_property(
				property_name,
				after_split_point_geometry_property_value);
	// If that fails (eg, because property name not recognised) then just add an unwrapped property value.
	if (!after_split_point_geometry_property)
	{
		after_split_point_geometry_property = 
			GPlatesModel::TopLevelPropertyInline::create(
					property_name,
					after_split_point_geometry_property_value);
	}

	// Add the geometry *after* the split point to the *new* feature.
	d_new_feature.get()->add(after_split_point_geometry_property.get());

	// We release the model notification guard which will cause a reconstruction to occur
	// because we modified the model.
	model_notification_guard.release_guard();

	// Disabling setting of focus for now since we now need to know the reconstruction tree
	// used to reconstruct the original feature - this is doable - but I wonder if we really
	// need to set focus anyway (it's kind of arbitrary which geometry we're setting focus
	// on anyway - probably should leave it up to the user to explicitly set focus by
	// clicking on geometry. In any case they probably only split a feature once.
	// For now let's only set focus when the user sets focus.
#if 1
	d_feature_focus->unset_focus();
#else
	d_feature_focus->set_focus(
			*d_old_feature,
			*GPlatesFeatureVisitors::find_first_geometry_property(
					*d_old_feature));
#endif

	d_feature_focus->announce_modification_of_focused_feature();
}

void
GPlatesViewOperations::SplitFeatureUndoCommand::undo()
{
	if(d_nothing_has_been_done)
	{
		return;
	}

	RenderedGeometryCollection::UpdateGuard update_guard;
	
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(*d_model_interface.access_model());

	//restore the old geometry
	GPlatesAppLogic::GeometryUtils::remove_geometry_properties_from_feature(*d_old_feature);
	(*d_old_feature)->add(*d_old_geometry_property);

#if 0
	GPlatesModel::ModelUtils::remove_feature(
			d_feature_collection_ref, *d_new_feature);
#endif
	(*d_new_feature)->remove_from_parent();

	//TODO: FIXME:
	//The following code is a kind of hacking. But if we don't set_focus in undo,
	//the behavior of GPlates will be quite annoying. This is the reason why I want to do it anyway,
	//even if the Undo object has been destroyed in the middle of this.
	//DON'T USE ANY DATA OF "UNDO OBJECT" AFTER SETTING FOCUS.
#if 1	
	GPlatesModel::FeatureHandle::iterator it=
			*GPlatesFeatureVisitors::find_first_geometry_property(
					*d_old_feature);
	
	// We release the model notification guard which will cause a reconstruction to occur
	// because we modified the model.
	// NOTE: DON'T USE ANY DATA MEMBER OF "UNDO OBJECT" AFTER RECONSTRUCTIOIN HAS BEEN CALLED.
	model_notification_guard.release_guard();

	// Disabling setting of focus for now since we now need to know the reconstruction tree
	// used to reconstruct the original feature - this is doable - but I wonder if we really
	// need to set focus anyway (it's kind of arbitrary which geometry we're setting focus
	// on anyway - probably should leave it up to the user to explicitly set focus by
	// clicking on geometry. In any case they probably only split a feature once.
	// For now let's only set focus when the user sets focus.
#if 1
	//save the 	d_feature_focus pointer to local variable before it is destroyed by reconstruct.
	GPlatesGui::FeatureFocus * feature_focus = d_feature_focus;
	feature_focus->unset_focus();
#else
	d_feature_focus->set_focus(*d_old_feature, it);
#endif

	feature_focus->announce_modification_of_focused_feature();
#endif

#if 0
	GPlatesModel::ModelUtils::remove_feature(
			d_feature_collection_ref, *d_new_feature_2);
#endif
}

