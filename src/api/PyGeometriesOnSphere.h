/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYGEOMETRIESONSPHERE_H
#define GPLATES_API_PYGEOMETRIESONSPHERE_H

#include <QString>

#include "global/PreconditionViolationError.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * The wrong type of GeometryOnSphere was specified.
	 */
	class GeometryTypeException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:

		explicit
		GeometryTypeException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const QString &message) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_message(message)
		{  }

		~GeometryTypeException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "GeometryTypeException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			write_string_message(os, d_message.toStdString());
		}

	private:

		QString d_message;
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYGEOMETRIESONSPHERE_H
