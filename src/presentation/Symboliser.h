
/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_SYMBOLISER_H
#define GPLATES_PRESENTATION_SYMBOLISER_H

#include "Symbol.h"

#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class Symboliser :
		public GPlatesUtils::ReferenceCount<Symboliser>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a Symboliser.
		typedef GPlatesUtils::non_null_intrusive_ptr<Symboliser> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Symboliser> non_null_ptr_to_const_type;


		virtual
		~Symboliser()
		{  }


	protected:
		Symboliser()
		{  }
	};
}

#endif // GPLATES_PRESENTATION_SYMBOLISER_H
