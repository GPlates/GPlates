/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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
#include "app-logic/ReconstructionGeometryUtils.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionGeometryFinder.h"
#include "qt-widgets/ViewportWindow.h"		//ViewState.


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
	GPlatesModel::FeatureHandle::properties_iterator new_geometry_property;
	if (new_associated_rg &&
		GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
				new_associated_rg, new_geometry_property))
	{
		d_associated_geometry_property_opt = new_geometry_property;
	}
	else
	{
		d_associated_geometry_property_opt = boost::none;
	}

	emit focus_changed(d_focused_feature, d_associated_reconstruction_geometry);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::FeatureHandle::properties_iterator new_associated_property)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focus();
		return;
	}
	if (d_focused_feature == new_feature_ref &&
		*d_associated_geometry_property_opt == new_associated_property)
	{
		// Avoid infinite signal/slot loops like the plague!
		return;
	}

	d_focused_feature = new_feature_ref;
	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property_opt = new_associated_property;
	// As this set_focus() is being called without an RG, but with a properties_iterator,
	// we can assume that it is the TopologySectionsTable doing the calling. It doesn't
	// know about RGs, and shouldn't.
	// However, the topology tools will want an RG to be highlighted after the table gets
	// clicked. The best place to do this lookup is here, rather than force the topology
	// tools to do the lookup (which forces an additional focus event, which is unnecessary.)
	find_new_associated_reconstruction_geometry(d_view_state_ptr->reconstruction());
	// In this specific case, find_new_associated_reconstruction_geometry() should always handle the emitting of
	// the focus_changed signal for us; we don't need to emit a second one.
	// Note that changing the semantics of find_new_associated_reconstruction_geometry() just for this method
	// wouldn't be a great idea, since it is also called in ViewportWindow after a new
	// reconstruction is made.

	// tell the rest of the application about the new focus
	emit focus_changed(d_focused_feature, d_associated_reconstruction_geometry);
}


void
GPlatesGui::FeatureFocus::unset_focus()
{
	d_focused_feature = GPlatesModel::FeatureHandle::weak_ref();
	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property_opt = boost::none;

	emit focus_changed(d_focused_feature, d_associated_reconstruction_geometry);
}


void
GPlatesGui::FeatureFocus::find_new_associated_reconstruction_geometry(
		GPlatesModel::Reconstruction &reconstruction)
{
	if (( ! is_valid()) || ( ! associated_geometry_property())) {
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
		GPlatesModel::FeatureHandle::properties_iterator new_geometry_property;
		if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
				new_rg, new_geometry_property))
		{
			continue;
		}

		if (new_geometry_property == *d_associated_geometry_property_opt)
		{
			// We have a match!
			d_associated_reconstruction_geometry = new_rg;
			emit focus_changed(d_focused_feature, d_associated_reconstruction_geometry);
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

	emit focus_changed(d_focused_feature, d_associated_reconstruction_geometry);
}


void
GPlatesGui::FeatureFocus::announce_modification_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have modified it, nothing is focused!
		return;
	}
	emit focused_feature_modified(d_focused_feature, d_associated_reconstruction_geometry);
}


void
GPlatesGui::FeatureFocus::announce_deletion_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have deleted it, nothing is focused!
		return;
	}
	emit focused_feature_deleted(d_focused_feature, d_associated_reconstruction_geometry);
	unset_focus();
}
