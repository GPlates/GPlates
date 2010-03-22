/* $Id$ */

/**
 * \file 
 * Contains the definition of the class WeakReferenceCallback.
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

#ifndef GPLATES_MODEL_WEAKREFERENCECALLBACK_H
#define GPLATES_MODEL_WEAKREFERENCECALLBACK_H

#include <boost/intrusive_ptr.hpp>
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	/**
	 * WeakReferenceCallback instances can be attached to WeakReference instances
	 * to enable the owner of a WeakReference to receive callbacks when the
	 * WeakReference's publisher is modified, deactivated, reactivated and
	 * about to be destroyed.
	 */
	template<typename H>
	class WeakReferenceCallback :
		public GPlatesUtils::ReferenceCount<WeakReferenceCallback<H> >
	{
	
	public:

		//! A convenience typedef for boost::intrusive_ptr<WeakReferenceCallback<H> >.
		typedef boost::intrusive_ptr<WeakReferenceCallback<H> > shared_ptr_type;

		//! Virtual destructor.
		virtual
		~WeakReferenceCallback()
		{
		}

		/**
		 * Called by WeakReference when its publisher is modified.
		 * Reimplement in a subclass to customise behaviour on publisher modification.
		 * @param publisher_ptr A pointer to the publisher that was modified.
		 */
		virtual
		void
		publisher_modified(
				H *publisher_ptr)
		{
		}

		/**
		 * Called by WeakReference when its publisher is deactivated. A publisher is
		 * deactivated when it is conceptually deleted from the model, but the object
		 * still exists for undo purposes.
		 * Reimplement in a subclass to customise behaviour on publisher deactivation.
		 * @param publisher_ptr A pointer to the publisher that was deactivated.
		 */
		virtual
		void
		publisher_deactivated(
				H *publisher_ptr)
		{
		}

		/**
		 * Called by WeakReference when its publisher is reactivated. A publisher is
		 * reactivated when it was conceptually deleted from the model, but the user
		 * requested that the deletion be undone.
		 * Reimplement in a subclass to customise behaviour on publisher reactivation.
		 * @param publisher_ptr A pointer to the publisher that was reactivated.
		 */
		virtual
		void
		publisher_reactivated(
				H *publisher_ptr)
		{
		}

		/**
		 * Called by WeakReference when its publisher is about to be destroyed (in
		 * the C++ sense). This might occur when the publisher was conceptually
		 * deleted from the model, and the user requested that the undo history stack
		 * be purged.
		 * Reimplement in a subclass to customise behaviour on publisher destruction.
		 * @param publisher_ptr A pointer to the publisher that will be destroyed.
		 */
		virtual
		void
		publisher_about_to_be_destroyed(
				H *publisher_ptr)
		{
		}

	};
}

#endif  // GPLATES_MODEL_WEAKREFERENCECALLBACK_H
