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

#ifndef GPLATES_MATHS_UNABLETONORMALILEZEROVECTOREXCEPTION_H
#define GPLATES_MATHS_UNABLETONORMALILEZEROVECTOREXCEPTION_H

#include "MathematicalException.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	/**
	 * This is the exception thrown when an attempt is made to normalise a zero vector.
	 */
	class UnableToNormaliseZeroVectorException :
			public MathematicalException
	{
	public:

		explicit
		UnableToNormaliseZeroVectorException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			MathematicalException(exception_source)
		{  }

		~UnableToNormaliseZeroVectorException() throw() { }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "UnableToNormaliseZeroVectorException";
		}

	};
}

#endif  // GPLATES_MATHS_UNABLETONORMALILEZEROVECTOREXCEPTION_H
