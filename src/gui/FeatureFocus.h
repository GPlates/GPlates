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
#include "model/WeakReference.h"


namespace GPlatesGui
{
	/**
	 * This class is used to store the feature which currently has focus.
	 * All feature clicks go through here, and anything interested in displaying
	 * the currently-focused feature can listen to events emitted from here.
	 *
	 * It -might- be a singleton in the future, but that largely depends on how we want
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
		 * Accessor for the currently focused (i.e. clicked on) feature.
		 * Remember to check is_valid()!
		 */
		GPlatesModel::FeatureHandle::weak_ref
		focused_feature()
		{
			return d_feature_ref;
		}
		
		/**
		 * The current 'focus' may not exist. Always check that the focused_feature()
		 * is valid using this method, or the weak_ref::is_valid() method.
		 */
		bool
		is_valid()
		{
			return d_feature_ref.is_valid();
		}
		
			
	public slots:
	
		/**
		 * Accessor to change which feature is currently focused.
		 * Will emit focused_feature_changed() to anyone who cares,
		 * provided that @a new_feature_ref is actually different
		 * to the previous feature.
		 */
		void
		set_focused_feature(
				GPlatesModel::FeatureHandle::weak_ref new_feature_ref);
		
		/**
		 * Accessor to clear the focus. Future calls to focused_feature() will return
		 * an invalid weak_ref.
		 * Will emit focused_feature_changed() to anyone who cares.
		 */
		void
		unset_focused_feature();

		/**
		 * Call this method when you have modified the properties of the currently
		 * focused feature. FeatureFocus will emit signals to notify anyone who
		 * needs to track modifications of the current focus.
		 */
		void
		notify_of_focused_feature_modification();
	
	signals:
	
		/**
		 * Emitted when a new feature has been clicked on, or the current focus has
		 * been cleared. Remember to check is_valid()!
		 */
		void
		focused_feature_changed(GPlatesModel::FeatureHandle::weak_ref);

		/**
		 * Emitted when the currently focused feature has been modified.
		 * For example, when the user edits a property of the feature.
		 */
		void
		focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref);	
		
	private:
	
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

	};
}

#endif // GPLATES_GUI_FEATUREFOCUS_H


