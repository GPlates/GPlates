/* $Id$ */

/**
 * \file 
 * Contains the definition of the class NotificationGuard.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_NOTIFICATIONGUARD_H
#define GPLATES_MODEL_NOTIFICATIONGUARD_H


namespace GPlatesModel
{
	class Model;

	/**
	 * NotificationGuard is a RAII class that blocks notifications from model
	 * Handles while active.
	 *
	 * If there is at least one NotificationGuard attached to the model,
	 * notifications (or events) will not be sent by Handles in the model when
	 * they are modified, deactivated (conceptually deleted) or reactivated
	 * (conceptually undeleted); instead, these events are queued up, and will be
	 * sent when the final NotificationGuard is destroyed.
	 *
	 * Notifications about a Handle's impending deallocation in the C++ sense are
	 * always immediately sent, regardless of whether any NotificationGuards are
	 * active.
	 *
	 * Note that if there are multiple notifications from a Handle, all
	 * notifications of the same type are merged into one notification. If, for
	 * instance, a NotificationGuard was active when feature F in feature
	 * collection FC was modified and feature G was added to FC, only one
	 * modification notification will be sent by FC to its listeners.
	 */
	class NotificationGuard
	{
	public:

		/**
		 * If @a model_ptr is NULL then this notification guard does nothing.
		 */
		explicit
		NotificationGuard(
				Model *model_ptr);

		~NotificationGuard();


		/**
		 * Releases this guard early.
		 *
		 * If this is the first time 'this' guard is released then any queued notifications
		 * are delivered here instead of in the destructor (if 'this' is the top-level object
		 * in any nesting of notification guard objects).
		 *
		 * Does nothing if @a release_guard has already been called (and @a acquire_guard not
		 * subsequently called).
		 */
		void
		release_guard();


		/**
		 * Acquires this guard (if it has been released).
		 *
		 * This is useful if you need to temporarily release the guard and then acquire it again
		 * so that notifications are sent prior to a small section of code and notification
		 * blocking is resumed afterwards.
		 *
		 * Does nothing if @a release_guard has not yet been called.
		 */
		void
		acquire_guard();

	private:

		Model *d_model_ptr;
		bool d_guard_released;
	};
}

#endif  // GPLATES_MODEL_NOTIFICATIONGUARD_H
