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

#ifndef GPLATES_GUI_FEATUREFOCUS_H
#define GPLATES_GUI_FEATUREFOCUS_H

#include <QObject>
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace GPlatesGui
{
	/**
	 * This class is used to store the feature which currently has focus.
	 *
	 * All feature clicks go through here, and anything interested in displaying the
	 * currently-focused feature can listen to events emitted from here.
	 *
	 * It @em might be a singleton in the future, but that largely depends on how we want
	 * feature focus to behave across multiple ViewportWindows, so for now it should be
	 * accessed as a member of ViewportWindow.
	 */
	class FeatureFocus:
			public QObject
	{
		Q_OBJECT
	public:
		
		explicit
		FeatureFocus()
		{  }

		virtual
		~FeatureFocus()
		{  }
		
		/**
		 * Accessor for the currently-focused feature.
		 * Remember to check is_valid()!
		 */
		GPlatesModel::FeatureHandle::weak_ref
		focused_feature()
		{
			return d_focused_feature;
		}
		
		/**
		 * Return whether the current focus is valid.
		 */
		bool
		is_valid()
		{
			return d_focused_feature.is_valid();
		}

		/**
		 * Accessor for the Reconstructed Feature Geometry (RFG) associated with the
		 * currently-focused feature (if there is one).
		 */
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type
		associated_rfg()
		{
			return d_associated_rfg;
		}

	public slots:

		/**
		 * Change which feature is currently focused.
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * After this slot is invoked, the associated Reconstructed Feature Geometry (RFG)
		 * pointer will be null.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref);

		/**
		 * Change which feature is currently focused, also specifying an associated
		 * Reconstructed Feature Geometry (RFG).
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * Note that nothing will be done with @a new_associated_rfg unless
		 * @a new_feature_ref is both valid and different to the previous feature.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type new_associated_rfg);

		/**
		 * Change which feature is currently focused, also specifying an associated
		 * Reconstructed Feature Geometry (RFG).
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * Note that nothing will be done with @a new_associated_rfg unless
		 * @a new_feature_ref is both valid and different to the previous feature.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry *new_associated_rfg);

		/**
		 * Clear the focus.
		 *
		 * Future calls to focused_feature() will return an invalid weak_ref.
		 * Will emit focus_changed() to anyone who cares.
		 */
		void
		unset_focus();

		/**
		 * Call this method when you have modified the properties of the currently focused
		 * feature.
		 *
		 * FeatureFocus will emit signals to notify anyone who needs to track modifications
		 * to the currently-focused feature.
		 */
		void
		announce_modfication_of_focused_feature();

	signals:

		/**
		 * Emitted when a new feature has been clicked on, or the current focus has been
		 * cleared.
		 * 
		 * Remember to check is_valid()!
		 */
		void
		focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type);

		/**
		 * Emitted when the currently focused feature has been modified.
		 * For example, when the user edits a property of the feature.
		 */
		void
		focused_feature_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type);	

	private:

		/**
		 * The currently-focused feature.
		 *
		 * Note that there might not be any currently-focused feature, in which case this
		 * would be an invalid weak-ref.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_focused_feature;

		/**
		 * The Reconstructed Feature Geometry (RFG) associated with the currently-focused
		 * feature.
		 *
		 * Note that there may not be a RFG associated with the currently-focused feature,
		 * in which case this would be a null pointer.
		 */
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type d_associated_rfg;

	};
}

#endif // GPLATES_GUI_FEATUREFOCUS_H
