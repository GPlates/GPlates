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
#include "model/Reconstruction.h"
#include "model/ReconstructedFeatureGeometryFinder.h"


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref)
{
	set_focus(new_feature_ref, NULL);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type new_associated_rfg)
{
	set_focus(new_feature_ref, new_associated_rfg.get());
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
	if (d_focused_feature == new_feature_ref &&
		d_associated_rfg == new_associated_rfg)
	{
		// Avoid infinite signal/slot loops like the plague!
		return;
	}

	d_focused_feature = new_feature_ref;
	d_associated_rfg = new_associated_rfg;

	if (new_associated_rfg)
	{
		d_associated_geometry_property = new_associated_rfg->property();
	}
	else
	{
		d_associated_geometry_property = boost::none;
	}

	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::unset_focus()
{
	d_focused_feature = GPlatesModel::FeatureHandle::weak_ref();
	d_associated_rfg = NULL;
	d_associated_geometry_property = boost::none;

	emit focus_changed(d_focused_feature, d_associated_rfg);
}


void
GPlatesGui::FeatureFocus::find_new_associated_rfg(
		GPlatesModel::Reconstruction &reconstruction)
{
	using namespace GPlatesModel;

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
	GPlatesModel::ReconstructedFeatureGeometryFinder rfgFinder(&reconstruction);
	rfgFinder.find_rfgs_of_feature(d_focused_feature);

	GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator rfgIter;
	for (rfgIter = rfgFinder.found_rfgs_begin();
		rfgIter != rfgFinder.found_rfgs_end();
		++rfgIter)
	{
		GPlatesModel::ReconstructedFeatureGeometry *new_rfg = rfgIter->get();

		// Look at the new rfg's geometry property.
		FeatureHandle::properties_iterator new_geometry_property = new_rfg->property();
		if (new_geometry_property == *d_associated_geometry_property)
		{
			// We have a match!
			d_associated_rfg = new_rfg;
			emit focus_changed(d_focused_feature, d_associated_rfg);
			return;
		}
	}

	// Otherwise, looked at all reconstruction geometries in the new reconstruction, without
	// finding a match.  Thus, it appears that there is no RFG in the new reconstruction which
	// corresponds to the current associated geometry property.
	//
	// When there is no RFG found, we lose the associated RFG.  This will be apparent to the user
	// if he increments the reconstruction time to a time when there is no RFG (meaning that the
	// associated RFG will become NULL). However the geometry property used by the RFG will
	// still be non-null so when the user then steps back one increment to where he was before
	// a new RFG will be found that uses the same geometry property and so the RFG will be
	// non-null once again.
	d_associated_rfg = NULL;

	// NOTE: We don't change the associated geometry property since the focused feature
	// hasn't changed and hence it's still applicable. We'll be using the geometry property
	// to find the associated RFG when/if one comes back into existence.

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
