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
#include "app-logic/ReconstructGraph.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometryFinder.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "model/FeatureHandle.h"
#include "model/WeakReferenceCallback.h"


namespace
{
	/**
	 * Feature handle weak ref callback to unset the focused feature
	 * if it gets deactivated in the model.
	 */
	class FocusedFeatureDeactivatedCallback :
			public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureHandle>
	{
	public:

		explicit
		FocusedFeatureDeactivatedCallback(
				GPlatesGui::FeatureFocus &feature_focus) :
			d_feature_focus(feature_focus)
		{
		}

		void
		publisher_deactivated(
				const deactivated_event_type &)
		{
			d_feature_focus.unset_focus();
		}

	private:

		GPlatesGui::FeatureFocus &d_feature_focus;
	};
}


GPlatesGui::FeatureFocus::FeatureFocus(
		GPlatesAppLogic::ApplicationState &application_state):
	d_application_state_ptr(&application_state)
{
	// Get notified whenever a new reconstruction occurs.
	QObject::connect(
			d_application_state_ptr,
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(handle_reconstruction(GPlatesAppLogic::ApplicationState &)));
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type new_associated_rg)
{
	if ( ! new_feature_ref.is_valid())
	{
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
	// Attach callback to feature handle weak-ref so we can unset the focus when
	// the feature is deactivated in the model.
	d_focused_feature.attach_callback(new FocusedFeatureDeactivatedCallback(*this));

	d_associated_reconstruction_geometry = new_associated_rg.get();

	// See if the new_associated_rg has a geometry property.
	boost::optional<GPlatesModel::FeatureHandle::iterator> new_geometry_property =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
					new_associated_rg);

	// Either way we set the properties iterator - it'll either get set to
	// the default value (invalid) or to the found properties iterator.
	d_associated_geometry_property = new_geometry_property
			? new_geometry_property.get() : GPlatesModel::FeatureHandle::iterator();

	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::FeatureHandle::iterator new_associated_property)
{
	if ( ! new_feature_ref.is_valid())
	{
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
	// Attach callback to feature handle weak-ref so we can unset the focus when
	// the feature is deactivated in the model.
	d_focused_feature.attach_callback(new FocusedFeatureDeactivatedCallback(*this));

	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property = new_associated_property;

	// Find the ReconstructionGeometry associated with the geometry property.
	find_new_associated_reconstruction_geometry(d_application_state_ptr->get_current_reconstruction());

	// tell the rest of the application about the new focus
	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref)
{
	if ( ! new_feature_ref.is_valid())
	{
		unset_focus();
		return;
	}
	
	// Locate a geometry property within the feature.
	GPlatesAppLogic::ReconstructionGeometryFinder finder;
	finder.find_rgs_of_feature(new_feature_ref);
	if (finder.found_rgs_begin() != finder.found_rgs_end()) {
		// Found something, just focus the first one.
		set_focus(new_feature_ref, *finder.found_rgs_begin());
	} else {
		// None found, we cannot focus this.
		unset_focus();
	}
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
		const GPlatesAppLogic::Reconstruction &reconstruction)
{
	if ( !d_focused_feature.is_valid() ||
		!d_associated_geometry_property.is_still_valid())
	{
		// There is either no focused feature, or no geometry property
		// associated with the most recent ReconstructionGeometry of the focused feature.
		// Either way, there's nothing for us to do here.
		return;
	}

	// Find the new associated ReconstructionGeometry for the currently-focused feature (if any).
	//
	// When the reconstruction is re-calculated, it will be populated with all-new
	// RGs.  The old RGs will be meaningless (but due to the power of intrusive-ptrs,
	// the associated RG currently referenced by this class will still exist).
	boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
			new_associated_rg =
					GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometry(
							d_focused_feature,
							d_associated_geometry_property,
							*reconstruction.get_default_reconstruction_tree());

	if (!new_associated_rg)
	{
		// We looked at the relevant reconstruction geometries in the new reconstruction, without
		// finding a match.  Thus, it appears that there is no RG in the new reconstruction which
		// corresponds to the current associated geometry property and reconstruction tree.
		//
		// When there is no RG found, we lose the associated RG.  This will be apparent to the user
		// if he increments the reconstruction time to a time when there is no RG (meaning that the
		// associated RG will become NULL). However the geometry property and reconstruction tree
		// used by the RG will still be non-null so when the user then steps back one increment to
		// where he was before, a new RG will be found that uses the same geometry property and
		// so the RG will be non-null once again.
		d_associated_reconstruction_geometry = NULL;

		// NOTE: We don't change the associated geometry property since
		// the focused feature hasn't changed and hence it's still applicable.
		// We'll be using the geometry property to find the associated RG when/if one comes
		// back into existence.
		return;
	}

	// Assign the new associated reconstruction geometry.
	d_associated_reconstruction_geometry = new_associated_rg.get().get();
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
		GPlatesAppLogic::ApplicationState &application_state)
{
	find_new_associated_reconstruction_geometry(
			application_state.get_current_reconstruction());

	// A new ReconstructionGeometry has been found so we should
	// emit a signal in case clients need to know this.
	emit focus_changed(*this);
}
