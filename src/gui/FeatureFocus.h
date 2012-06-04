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

#ifndef GPLATES_GUI_FEATUREFOCUS_H
#define GPLATES_GUI_FEATUREFOCUS_H

#include <boost/optional.hpp>
#include <QObject>

#include "app-logic/ReconstructionGeometry.h"

#include "model/FeatureHandle.h"

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

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
		
		explicit
		FeatureFocus(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection);

		virtual
		~FeatureFocus()
		{  }

		/**
		 * Accessor for the currently-focused feature.
		 *
		 * NOTE: Remember to check 'is_valid()' on returned feature reference.
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
		 * Accessor for the @a ReconstructGeometry associated with the currently-focused feature (if there is one).
		 */
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type
		associated_reconstruction_geometry() const
		{
			return d_associated_reconstruction_geometry;
		}

		/**
		 * Accessor for the geometry property used by the most recent RFG associated with the
		 * currently-focused feature (if there is one).
		 *
		 * NOTE: Remember to check 'is_still_valid()' on returned properties iterator.
		 */
		const GPlatesModel::FeatureHandle::iterator &
		associated_geometry_property() const
		{
			return d_associated_geometry_property;
		}

	public slots:

		/**
		 * Change which feature is currently focused, also specifying an associated
		 * ReconstructionGeometry.
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * Note that nothing will be done with @a new_associated_rg unless
		 * @a new_feature_ref is both valid and different to the previous feature.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type new_associated_rg);

		/**
		 * Change which feature is currently focused, also specifying an associated
		 * property iterator.
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * Also attempts to find a suitable ReconstructionGeometry given the properties iterator.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
				GPlatesModel::FeatureHandle::iterator new_associated_property);

		/**
		 * Change which feature is currently focused, @em without specifying any geometry property.
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_feature_ref
		 * is actually different to the previous feature.
		 *
		 * This method will set the focus to the first (if any!) geometry found within the
		 * supplied feature. While it may seem counterproductive to set this so arbitrarily,
		 * it is sometimes desirable to focus an entire feature rather than a specific geometry
		 * within it, such as after using the Clone Feature button.
		 * Also, finding exactly one geometry should be the most common case.
		 * If no geometry can be found, this method behaves as @a unset_focus().
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref);

		/**
		 * Clear the focus.
		 *
		 * Future calls to focused_feature() will return an invalid weak_ref.
		 * Will emit focus_changed() to anyone who cares.
		 */
		void
		unset_focus();

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

		/**
		 * Notification of an update to the rendered geometry collection - we use this to find a
		 * new associated reconstruction geometry for the focused feature (if any) - the
		 * rendered geometry collection contains only the visible geometries.
		 */
		void
		handle_rendered_geometry_collection_update();

	signals:

		/**
		 * Emitted when a new feature has been clicked on, or the current focus has been
		 * cleared.
		 * 
		 * Remember to check is_valid()!
		 */
		void
		focus_changed(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Emitted when the currently-focused feature has been modified.
		 * For example, when the user edits a property of the feature.
		 */
		void
		focused_feature_modified(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Emitted when the currently-focused feature has been deleted.
		 * When the user clicks the Delete Feature action,
		 * announce_deletion_of_focused_feature() will be called, this signal will
		 * be emitted, and immediately afterwards a focus_changed() signal will
		 * be emitted with an invalid weak_ref.
		 */
		void
		focused_feature_deleted(
				GPlatesGui::FeatureFocus &feature_focus);	

	private:

		/**
		 * The currently-focused feature.
		 *
		 * Note that there might not be any currently-focused feature, in which case this
		 * would be an invalid weak-ref.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_focused_feature;

		/**
		 * Keep another reference to the currently focused feature to contain our model callback.
		 *
		 * This avoids the problem where the callback is copied when the weak ref is copied.
		 * Having a separate weak ref means the callback cannot escape into the wild and cause
		 * a crash during shutdown when the view state is destroyed before the application-logic state
		 * (the callback then references a destroyed @a FeatureFocus object).
		 */
		GPlatesModel::FeatureHandle::weak_ref d_callback_focused_feature;

		/**
		 * The ReconstructionGeometry associated with the currently-focused feature.
		 *
		 * Note that there may not be a RG associated with the currently-focused feature.
		 */
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type d_associated_reconstruction_geometry;

		/**
		 * The geometry property used by the ReconstructionGeometry
		 * associated with the currently-focused feature.
		 */
		GPlatesModel::FeatureHandle::iterator d_associated_geometry_property;

		/**
		 * Used to find the visible reconstruction geometries as a short-list for searching
		 * for the focused feature geometry because we don't want to pick up any old
		 * or invisible reconstruction geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Find the new associated ReconstructionGeometry for the currently-focused feature (if any).
		 *
		 * When the rendered geometry collection is re-calculated, it will reference back to all-new
		 * RGs.  The old RGs will be meaningless (but due to the power of intrusive-ptrs,
		 * the associated RG currently referenced by this class will still exist).
		 *
		 * This function is used to iterate through the reconstruction geometries in the
		 * referenced by the rendered geometry collection and find the new RG (if there is one)
		 * which corresponds to the current associated RG; if a new RG is found, it will be
		 * assigned to be the associated RG.
		 */
		void
		find_new_associated_reconstruction_geometry();
	};

	boost::optional<const GPlatesMaths::LatLonPoint>
	locate_focus();
}

#endif // GPLATES_GUI_FEATUREFOCUS_H
