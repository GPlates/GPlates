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
GPlatesGui::FeatureFocus::set_focused_feature(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref)
{
	if ( ! new_feature_ref.is_valid()) {
		unset_focused_feature();
		return;
	}
	if (d_feature_ref == new_feature_ref) {
		// Avoid infinite signal/slot loops like the plague!
		return;
	}
	d_feature_ref = new_feature_ref;
	emit focused_feature_changed(d_feature_ref);
}


void
GPlatesGui::FeatureFocus::unset_focused_feature()
{
	d_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	emit focused_feature_changed(d_feature_ref);
}


void
GPlatesGui::FeatureFocus::notify_of_focused_feature_modification()
{
	if ( ! d_feature_ref.is_valid()) {
		// You can't have modified it, nothing is focused!
		return;
	}
	emit focused_feature_modified(d_feature_ref);
}


