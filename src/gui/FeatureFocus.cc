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
GPlatesGui::FeatureFocus::announce_modfication_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have modified it, nothing is focused!
		return;
	}
	emit focused_feature_modified(d_focused_feature, d_associated_rfg);
}
