/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "FeatureFocus.h"
#include "model/Reconstruction.h"


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focus();
		return;
	}
	if (d_focused_feature == new_feature_ref) {
		// Avoid infinite signal/slot loops like the plague!
		return;
	}
	d_focused_feature = new_feature_ref;
	d_associated_rfg = NULL;

	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type new_associated_rfg)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focus();
		return;
	}
	if (d_focused_feature == new_feature_ref) {
		// Avoid infinite signal/slot loops like the plague!
		return;
	}
	d_focused_feature = new_feature_ref;
	d_associated_rfg = new_associated_rfg.get();

	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry *new_associated_rfg)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focus();
		return;
	}
	if (d_focused_feature == new_feature_ref) {
		// Avoid infinite signal/slot loops like the plague!
		return;
	}
	d_focused_feature = new_feature_ref;
	d_associated_rfg = new_associated_rfg;

	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::unset_focus()
{
	d_focused_feature = GPlatesModel::FeatureHandle::weak_ref();
	d_associated_rfg = NULL;

	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::find_new_associated_rfg(
		GPlatesModel::Reconstruction &reconstruction)
{
	using namespace GPlatesModel;

	if (( ! is_valid()) || ( ! associated_rfg())) {
		// There is either no focused feature, or no RFG associated with the focused
		// feature.  Either way, there's nothing for us to do here.
		return;
	}
	FeatureHandle::properties_iterator current_geometry_property = associated_rfg()->property();

	Reconstruction::geometry_collection_type::const_iterator iter =
			reconstruction.geometries().begin();
	Reconstruction::geometry_collection_type::const_iterator end =
			reconstruction.geometries().end();
	for ( ; iter != end; ++iter) {
		GPlatesModel::ReconstructionGeometry *rg = iter->get();

		// We use a dynamic cast here (despite the fact that dynamic casts are generally
		// considered bad form) because we only care about one specific derivation.
		// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
		// "if ... else if ..." chain would imply that we should be using polymorphism --
		// specifically, the double-dispatch of the Visitor pattern -- rather than updating
		// the "if ... else if ..." chain each time a new derivation is added.)
		GPlatesModel::ReconstructedFeatureGeometry *new_rfg =
				dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
		if (new_rfg) { 
			// It's an RFG, so let's look at its geometry property.
			FeatureHandle::properties_iterator new_geometry_property =
					new_rfg->property();
			if (new_geometry_property == current_geometry_property) {
				// We have a match!
				d_associated_rfg = new_rfg;
				emit focus_changed(d_focused_feature, d_associated_rfg);
				return;
			}
		}
	}
	// Otherwise, looked at all reconstruction geometries in the new reconstruction, without
	// finding a match.  Thus, it appears that there is no RFG in the new reconstruction which
	// corresponds to the current associated RFG.
	//
	// Note a minor irritation with this approach:  When there is no RFG found, we lose the
	// associated RFG.  This will be apparent to the user if he increments the reconstruction
	// time to a time when there is no RFG (meaning that the associated RFG will become NULL),
	// then steps back one increment -- even though the RFG should exist at this time, since
	// we've dropped the RFG, we won't search for a matching one at that time.  A possible way
	// around this might be to store the 'FeatureHandle::properties_iterator' for the current
	// geometry property, in addition to the current associated RFG.
	d_associated_rfg = NULL;
	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::announce_modification_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have modified it, nothing is focused!
		return;
	}
	emit focused_feature_modified(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::announce_deletion_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have deleted it, nothing is focused!
		return;
	}
	emit focused_feature_deleted(d_focused_feature, d_associated_rfg);
	unset_focus();
}
