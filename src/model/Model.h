/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODEL_H
#define GPLATES_MODEL_MODEL_H

#include "WeakReference.h"

#include "global/PointerTraits.h"


namespace GPlatesModel
{
	class FeatureStoreRootHandle;
	class NotificationGuard;

	/**
	 * The interface to the Model tier of GPlates.
	 *
	 * This contains a feature store and (later) Undo/Redo stacks.
	 *
	 * This class is hidden from the other GPlates tiers behind class ModelInterface, in an
	 * instance of the "p-impl" idiom.  This creates a "compiler firewall" (in order to speed
	 * up build times) as well as an architectural separation between the Model tier (embodied
	 * by the Model class) and the other GPlates tiers which use the Model.
	 *
	 * Classes outside the Model tier should pass around ModelInterface instances, never Model
	 * instances.  A ModelInterface instance will contain and hide a Model instance, managing
	 * its memory automatically.
	 *
	 * Header files outside of the Model tier should only #include "model/ModelInterface.h",
	 * never "model/Model.h".  "model/Model.h" should only be #included in ".cc" files when
	 * necessary (ie, whenever you want to access the members of the Model instance through the
	 * ModelInterface, and hence need the class definition of Model).
	 */
	class Model
	{

	public:

		/**
		 * Create a new instance of the Model, which contains an empty feature store.
		 */
		Model();

		/**
		 * Destructor.
		 */
		~Model();

		/**
		 * Returns a (non-const) weak-ref to the model's feature store root.
		 */
		const WeakReference<FeatureStoreRootHandle>
		root();

		/**
		 * Returns a const weak-ref to the model's feature store root.
		 */
		const WeakReference<const FeatureStoreRootHandle>
		root() const;

		/**
		 * Returns true if there are any NotificationGuard instances currently
		 * attached to the model.
		 */
		bool
		has_notification_guard() const;

	private:

		/**
		 * Increments the count of NotificationGuard instances attached to the model.
		 */
		void
		increment_notification_guard_count();

		/**
		 * Decrements the count of NotificationGuard instances attached to the model.
		 */
		void
		decrement_notification_guard_count();

		/**
		 * A persistent handle to the root of the feature store, which contains all
		 * loaded feature collections and their features.
		 */
		GPlatesGlobal::PointerTraits<FeatureStoreRootHandle>::non_null_ptr_type d_root;

		/**
		 * A count of the number of NotificationGuard instances attached to this
		 * model.
		 *
		 * If this number is greater than zero, notifications (or events) will not be
		 * sent by Handles in this model when they are modified, deactivated
		 * (conceptually deleted) or reactivated (conceptually undeleted); instead,
		 * these events are queued up, and will be sent when this number returns to
		 * zero.
		 *
		 * Notifications about a Handle's impending deallocation in the C++ sense are
		 * always immediately sent, regardless of this number.
		 *
		 * Note that if there are multiple notifications from a Handle, all
		 * notifications of the same type are merged into one notification. If, for
		 * instance, a NotificationGuard was active when feature F in feature
		 * collection FC was modified and feature G was added to FC, only one
		 * modification notification will be sent by FC to its listeners.
		 */
		unsigned int d_notification_guard_count;

		friend class NotificationGuard;

	};

}

#endif  // GPLATES_MODEL_MODEL_H
