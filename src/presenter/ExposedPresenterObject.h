/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTER_EXPOSEDPRESENTEROBJECT_H
#define GPLATES_PRESENTER_EXPOSEDPRESENTEROBJECT_H

namespace GPlatesPresenter
{
	/** 
	 * This class is inherited by all objects exposed by the Presenter to the View.
	 *
	 * It allocates a unique identifier upon creation.
	 *
	 * This class should be used as a virtual base class.
	 */
	class ExposedPresenterObject {

		public:

			typedef unsigned long id_type;

			/**
			 * Return the identifier.
			 */
			id_type
			id() const
			{
				return d_id;
			}

		protected:

			/**
			 * When creating a new ExposedPresenterObject
			 * we assign the next id in sequence.
			 * This method is protected because we don't want
			 * to create stand-alone ExposedPresenterObject
			 * instances.
			 */
			ExposedPresenterObject() :
				d_id(get_next_id())
			{
			}

		private:

			// The 'global' identifier id sequence counter
			static id_type d_next_id;

			// The identifer for this object
			id_type d_id;

			/**
			 * Return the next avilable identifier value
			 *
			 * FIXME (ticket:18): At the moment this can only be used in
			 * a single thread because there is no mutex protection
			 * around the next_id value;
			 */
			static
			id_type
			get_next_id()
			{
				return ++d_next_id;
			}

			/**
			 * ExposedPresenterObjects should not be copyable.
			 */
			ExposedPresenterObject(ExposedPresenterObject &);

			/**
			 * ExposedPresenterObjects should not be assignable
			 */
			ExposedPresenterObject &operator=(const ExposedPresenterObject &);

	};

}

#endif  // GPLATES_PRESENTER_EXPOSEDPRESENTEROBJECT_H
