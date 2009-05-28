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
#include <boost/optional.hpp>

#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace GPlatesGui
{
	/**
	 * This class is used to store the notion of which feature currently has the focus.
	 *
	 * All feature-focus changes should update this, and anything interested in displaying the
	 * currently-focused feature can listen to events emitted from here.
	 */
	class FeatureFocus:
			public QObject
	{
		Q_OBJECT
	public:
		
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
		focused_feature() const
		{
			return d_focused_feature;
		}
		
		/**
		 * Return whether the current focus is valid.
		 */
		bool
		is_valid() const
		{
			return d_focused_feature.is_valid();
		}

		/**
		 * Accessor for the Reconstructed Feature Geometry (RFG) associated with the
		 * currently-focused feature (if there is one).
		 *
		 * FIXME: See mega fixme comment on @a focus_changed().
		 */
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type
		associated_rfg() const
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
		 * Change which feature is currently focused, also specifying an associated
		 * property iterator. This is for the benefit of the TopologySections{Table,Container},
		 * which doesn't (and shouldn't) know about RFGs.
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * As a stopgap, this also causes a call to @a find_new_associated_rfg #### AAAAAAG
		 * #### I CAN'T DO THAT IF I DON'T HAVE ACCESS TO THE RECONSTRUCTION!!!!!
		 * #### see also: side-quest hell.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
				GPlatesModel::FeatureHandle::properties_iterator new_associated_property);

		/**
		 * Clear the focus.
		 *
		 * Future calls to focused_feature() will return an invalid weak_ref.
		 * Will emit focus_changed() to anyone who cares.
		 */
		void
		unset_focus();

		/**
		 * Find the new associated RFG for the currently-focused feature (if any).
		 *
		 * When the reconstruction is re-calculated, it will be populated with all-new
		 * RFGs.  The old RFGs will be meaningless (but due to the power of intrusive-ptrs,
		 * the associated RFG currently referenced by this class will still exist).
		 *
		 * This function is used to iterate through the reconstruction geometries in the
		 * supplied Reconstruction and find the new RFG (if there is one) which corresponds
		 * to the current associated RFG; if a new RFG is found, it will be assigned to be
		 * the associated RFG.
		 *
		 * Currently this function is O(n) in the number of reconstruction geometries in
		 * the reconstruction, which is frustrating, but this function is necessary.
		 */
		void
		find_new_associated_rfg(
				GPlatesModel::Reconstruction &reconstruction);

		/**
		 * Call this method when you have modified the properties of the currently-focused
		 * feature.
		 *
		 * FeatureFocus will emit signals to notify anyone who needs to track modifications
		 * to the currently-focused feature.
		 */
		void
		announce_modification_of_focused_feature();

		/**
		 * Call this method when you have deleted the currently-focused feature
		 * from the model (i.e. the Delete Feature action).
		 *
		 * FeatureFocus will emit the focused_feature_deleted() signals to notify anyone
		 * who needs to know about the deletion, then unset the focus and emit the
		 * usual focus_changed(invalid weak_ref) signal.
		 *
		 * It is necessary to explicitly specify that the feature has been deleted,
		 * rather than just emit a focus change event, because certain dialogs (i.e. the
		 * EditFeaturePropertiesWidget) may have uncommited data that the user was in
		 * the middle of editing before they went insane and hit the Delete Feature button.
		 * Normally, the EditFeaturePropertiesWidget commits old data before switching to
		 * a new focused feature, but in this situation it would be possible to be in
		 * a situation where properties of the supposedly deleted feature were being modified.
		 *
		 * TODO: When we have entire selections of features that could be deleted,
		 * would it make sense to emit an entirely different signal for the same basic
		 * deletion event? Perhaps this indicates we want some sort of "FeatureEvents" class,
		 * which will simply emit multiple feature_deleted(weak_ref) events in that case.
		 * Of course, that depends on how well the Qt signal/slot mechanism can handle
		 * potentially thousands of signals. Perhaps it would be better to have a separate
		 * selected_features_deleted(vector<weak_ref>) in that case, and get listeners
		 * to iterate through it if they really care about the event.
		 */
		void
		announce_deletion_of_focused_feature();

	signals:

		/**
		 * Emitted when a new feature has been clicked on, or the current focus has been
		 * cleared.
		 * 
		 * Remember to check is_valid()!
		 *
		 * BIG FIXME COMMENT HURRAY:
		 * At this stage (nearing 0.9.5, needing to cram stuff into FeatureFocus),
		 * I'd really like to overhaul the FeatureFocus signal interface. Specifically,
		 * focus_changed() and pals should include less and less detail in their parameters.
		 * Despite this being super-convenient for several slot connection cases (i.e.
		 * being able to directly connect changed(FH) to do_stuff(FH)), I currently need
		 * to add the ability to focus a feature with a properties_iterator rather than
		 * RFG directly. This means we need to do the O(n) lookup of RFG via
		 * @a find_new_associated_rfg() sometime, and if we need to emit this signal with
		 * an RFG then our options are limited to:-
		 *  a) Emit a null RFG ptr focus event first, let client code call
		 *     @a find_new_associated_rfg() if they really want it (dumb, and causing
		 *     a spurious focus event...), or
		 *  b) Let FeatureFocus do the RFG-finding automatically whenever someone refocuses
		 *     via a properties_iterator rather than explicit RFG. This is likely the best
		 *     option for now, as only the TopologySectionsTable should be using that
		 *     set_focus() signature - but I don't like doing that heavy function call
		 *     on focus change, when it might not be necessary.
		 * 
		 * A better way forward would be to get focus listeners to explicitly ask for
		 * the associated {feature handle, rfg, properties_iterator} when the focus changes,
		 * if they're interested in that. Then we could do a lazy lookup of the RFG from
		 * the properties_iterator only if we really need to.
		 * But that is a big change to the FeatureFocus interface which we don't have time
		 * for now. -JC
		 */
		void
		focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type);

		/**
		 * Emitted when the currently-focused feature has been modified.
		 * For example, when the user edits a property of the feature.
		 */
		void
		focused_feature_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type);

		/**
		 * Emitted when the currently-focused feature has been deleted.
		 * When the user clicks the Delete Feature action,
		 * announce_deletion_of_focused_feature() will be called, this signal will
		 * be emitted, and immediately afterwards a focus_changed() signal will
		 * be emitted with an invalid weak_ref.
		 */
		void
		focused_feature_deleted(
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

		/**
		 * The geometry property used by the Reconstructed Feature Geometry (RFG)
		 * associated with the currently-focused feature.
		 *
		 * Note that when focused feature changes the associated.
		 */
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_associated_geometry_property_opt;


		/**
		 * Accessor for the geometry property used by the most recent RFG associated with the
		 * currently-focused feature (if there is one).
		 */
		const boost::optional<GPlatesModel::FeatureHandle::properties_iterator> &
		associated_geometry_property() const
		{
			return d_associated_geometry_property_opt;
		}
	};
}

#endif // GPLATES_GUI_FEATUREFOCUS_H
