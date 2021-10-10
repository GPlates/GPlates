/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GLOBAL_INTRUSIVEPOINTERZEROREFCOUNTEXCEPTION_H
#define GPLATES_GLOBAL_INTRUSIVEPOINTERZEROREFCOUNTEXCEPTION_H

// FIXME:  When the definition of 'write_message' moves to a .cc file, replace this with <iosfwd>.
#include <ostream>
#include "global/InternalObjectInconsistencyException.h"


namespace GPlatesGlobal
{
	/**
	 * This is the exception thrown when an object has an intrusive-pointer ref-count of zero,
	 * when its ref-count should be greater than zero.
	 */
	class IntrusivePointerZeroRefCountException:
			public GPlatesGlobal::InternalObjectInconsistencyException
	{
	public:
		/**
		 * When this exception is thrown, presumably in a member function of the object
		 * whose ref-count has been observed to be zero, the parameters to this constructor
		 * should be @c this, @c __FILE__ and @c __LINE__, which indicate the object, the
		 * file name and the line number, respectively.
		 */
		IntrusivePointerZeroRefCountException(
				const void *ptr_to_referenced_object_,
				const char *filename_,
				int line_num_):
			d_ptr_to_referenced_object(ptr_to_referenced_object_),
			d_filename(filename_),
			d_line_num(line_num_)
		{  }

		virtual
		~IntrusivePointerZeroRefCountException()
		{  }

	protected:
		virtual
		const char *
		ExceptionName() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "IntrusivePointerZeroRefCountException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			// FIXME:  This function should really be defined in a .cc file.
		}

	private:
		const void *d_ptr_to_referenced_object;
		const char *d_filename;
		int d_line_num;
	};
}

#endif  // GPLATES_GLOBAL_INTRUSIVEPOINTERZEROREFCOUNTEXCEPTION_H
