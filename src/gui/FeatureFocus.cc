/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include <boost/none.hpp>

#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionGeometryFinder.h"


GPlatesGui::FeatureFocus::FeatureFocus(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesAppLogic::Reconstruct &reconstruct):
	d_reconstruct_ptr(&reconstruct)
{
	// Get notified whenever a new reconstruction occurs.
	QObject::connect(
			d_reconstruct_ptr,
			SIGNAL(reconstructed(GPlatesAppLogic::Reconstruct &, bool, bool)),
			this,
			SLOT(handle_reconstruction(GPlatesAppLogic::Reconstruct &)));

	// Get notified whenever a file is being removed or modified.
	QObject::connect(
			&application_state.get_feature_collection_file_state(),
			SIGNAL(begin_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(modifying_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));
	QObject::connect(
			&application_state.get_feature_collection_file_state(),
			SIGNAL(begin_reset_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(modifying_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));
	QObject::connect(
			&application_state.get_feature_collection_file_state(),
			SIGNAL(begin_reclassify_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(modifying_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type new_associated_rg)
{
	set_focus(new_feature_ref, new_associated_rg.get());
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::ReconstructionGeometry *new_associated_rg)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focus();
		return;
	}
	if (d_focused_feature == new_feature_ref &&
		d_associated_reconstruction_geometry == new_associated_rg)
	{
		// Avoid infinite signal/slot loops like the plague!
		return;
	}

	d_focused_feature = new_feature_ref;
	d_associated_reconstruction_geometry = new_associated_rg;

	// See if the new_associated_rg has a geometry property.
	GPlatesModel::FeatureHandle::iterator new_geometry_property;
	if (new_associated_rg)
	{
		GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
				new_associated_rg, new_geometry_property);
	}

	// Either way we set the properties iterator - it'll either get set to
	// the default value (invalid) or to the found properties iterator.
	d_associated_geometry_property = new_geometry_property;

	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::FeatureHandle::iterator new_associated_property)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focus();
		return;
	}
	if (d_focused_feature == new_feature_ref &&
		d_associated_geometry_property == new_associated_property)
	{
		// Avoid infinite signal/slot loops like the plague!
		return;
	}

	d_focused_feature = new_feature_ref;
	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property = new_associated_property;

	// Find the ReconstructionGeometry associated with the geometry property.
	find_new_associated_reconstruction_geometry(d_reconstruct_ptr->get_current_reconstruction());

	// tell the rest of the application about the new focus
	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::unset_focus()
{
	d_focused_feature = GPlatesModel::FeatureHandle::weak_ref();
	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property = GPlatesModel::FeatureHandle::iterator();

	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::find_new_associated_reconstruction_geometry(
		GPlatesModel::Reconstruction &reconstruction)
{
	if (( ! is_valid())) {
		// There is either no focused feature, or no geometry property associated with the
		// most recent RFG of the focused feature.
		// Either way, there's nothing for us to do here.
		// Note that it's ok to have no RFG though - we can still find a new RFG
		// if we have a geometry property.
		return;
	}

	// Iterate through the RFGs (belonging to 'reconstruction')
	// that are observing the focused feature.
	GPlatesModel::ReconstructionGeometryFinder rgFinder(&reconstruction);
	rgFinder.find_rgs_of_feature(d_focused_feature);

	GPlatesModel::ReconstructionGeometryFinder::rg_container_type::const_iterator rgIter;
	for (rgIter = rgFinder.found_rgs_begin();
		rgIter != rgFinder.found_rgs_end();
		++rgIter)
	{
		GPlatesModel::ReconstructionGeometry *new_rg = rgIter->get();

		// See if the new ReconstructionGeometry has a geometry property.
		GPlatesModel::FeatureHandle::iterator new_geometry_property;
		if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
				new_rg, new_geometry_property))
		{
			continue;
		}

		if (new_geometry_property == d_associated_geometry_property)
		{
			// We have a match!
			d_associated_reconstruction_geometry = new_rg;
			return;
		}
	}

	// Otherwise, looked at all reconstruction geometries in the new reconstruction, without
	// finding a match.  Thus, it appears that there is no RG in the new reconstruction which
	// corresponds to the current associated geometry property.
	//
	// When there is no RG found, we lose the associated RG.  This will be apparent to the user
	// if he increments the reconstruction time to a time when there is no RG (meaning that the
	// associated RG will become NULL). However the geometry property used by the RG will
	// still be non-null so when the user then steps back one increment to where he was before
	// a new RG will be found that uses the same geometry property and so the RG will be
	// non-null once again.
	d_associated_reconstruction_geometry = NULL;

	// NOTE: We don't change the associated geometry property since the focused feature
	// hasn't changed and hence it's still applicable. We'll be using the geometry property
	// to find the associated RG when/if one comes back into existence.
}


void
GPlatesGui::FeatureFocus::announce_modification_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have modified it, nothing is focused!
		return;
	}

	if (!associated_geometry_property().is_still_valid()) {
		// There is no geometry property - it must have been removed
		// during the feature modification.
		// We'll need to unset the focused feature.
		unset_focus();
	}

	emit focused_feature_modified(*this);
}


void
GPlatesGui::FeatureFocus::announce_deletion_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have deleted it, nothing is focused!
		return;
	}
	emit focused_feature_deleted(*this);
	unset_focus();
}


void
GPlatesGui::FeatureFocus::handle_reconstruction(
		GPlatesAppLogic::Reconstruct &reconstructer)
{
	find_new_associated_reconstruction_geometry(
			reconstructer.get_current_reconstruction());

	// A new ReconstructionGeometry has been found so we should
	// emit a signal in case clients need to know this.
	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::modifying_feature_collection(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	// FIXME: This is a temporary hack to stop highlighting the focused feature if
	// it's in the feature collection we're about to unload or modify (such as reload).
	if (!is_valid())
	{
		return;
	}

	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file->get_feature_collection();

	GPlatesModel::FeatureCollectionHandle::iterator feature_iter;
	for (feature_iter = feature_collection->begin();
		feature_iter != feature_collection->end();
		++feature_iter)
	{
		if ((*feature_iter).get() == d_focused_feature.handle_ptr())
		{
			unset_focus();
			break;
		}
	}
}
