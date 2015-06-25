/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYFEATURE_H
#define GPLATES_API_PYFEATURE_H

#include "global/PreconditionViolationError.h"

#include "model/PropertyName.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * This exception can be thrown when either:
	 *  (1) there is more than one matching coverage *domain* with same number of points, or
	 *  (2) there is more than one matching coverage *range* with same number of scalars.
	 */
	class AmbiguousGeometryCoverageException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		enum AmbiguityType
		{
			AmbigiousDomain,
			AmbigiousRange
		};

		explicit
		AmbiguousGeometryCoverageException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				AmbiguityType ambiguity_type,
				const GPlatesModel::PropertyName &domain_property_name) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_ambiguity_type(ambiguity_type),
			d_domain_property_name(domain_property_name)
		{  }

		~AmbiguousGeometryCoverageException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "AmbiguousGeometryCoverageException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:

		AmbiguityType d_ambiguity_type;
		GPlatesModel::PropertyName d_domain_property_name;
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYFEATURE_H
