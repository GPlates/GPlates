/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYRECONSTRUCTIONTREE_H
#define GPLATES_API_PYRECONSTRUCTIONTREE_H

#include "global/PreconditionViolationError.h"


namespace GPlatesApi
{
	/**
	 * The anchored plates of two reconstruction trees are not the same.
	 */
	class DifferentAnchoredPlatesInReconstructionTreesException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		DifferentAnchoredPlatesInReconstructionTreesException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~DifferentAnchoredPlatesInReconstructionTreesException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "DifferentAnchoredPlatesInReconstructionTreesException";
		}

	private:
	};
}

#endif // GPLATES_API_PYRECONSTRUCTIONTREE_H
