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

#ifndef GPLATES_PRESENTATION_SYMBOL_H
#define GPLATES_PRESENTATION_SYMBOL_H

#include "SymbolVisitor.h"

#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class Symbol :
		public GPlatesUtils::ReferenceCount<Symbol>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a Symbol.
		typedef GPlatesUtils::non_null_intrusive_ptr<Symbol> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Symbol> non_null_ptr_to_const_type;


		virtual
		~Symbol()
		{  }


		/**
		 * Accept a ConstSymbolVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstSymbolVisitor &visitor) const = 0;

		/**
		 * Accept a SymbolVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				SymbolVisitor &visitor) = 0;
	};
}

#endif // GPLATES_PRESENTATION_SYMBOL_H

