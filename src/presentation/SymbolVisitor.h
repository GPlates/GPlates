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

#ifndef GPLATES_PRESENTATION_SYMBOLVISITOR_H
#define GPLATES_PRESENTATION_SYMBOLVISITOR_H

#include "utils/CopyConst.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesPresentation
{
	class Symbol;

	// Forward declaration of template visitor class.
	template <class SymbolType>
	class SymbolVisitorBase;


	/**
	 * Typedef for visitor over non-const @a Symbol objects.
	 */
	typedef SymbolVisitorBase<Symbol> SymbolVisitor;

	/**
	 * Typedef for visitor over const @a Symbol objects.
	 */
	typedef SymbolVisitorBase<const Symbol> ConstSymbolVisitor;


	// Forward declarations of Symbol derived types.
	class PointSymbol;
	class PolygonSymbol;
	class PolylineSymbol;


	/**
	 * This class defines an abstract interface for a Visitor to visit reconstruction
	 * geometries.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 */
	template <class SymbolType>
	class SymbolVisitorBase
	{
	public:

		//! Typedef for @a PointSymbol of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				SymbolType, PointSymbol>::type
						point_symbol_type;

		//! Typedef for @a PolygonSymbol of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				SymbolType, PolygonSymbol>::type
						polygon_symbol_type;

		//! Typedef for @a PolylineSymbol of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				SymbolType, PolylineSymbol>::type
						polyline_symbol_type;

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~SymbolVisitorBase() = 0;


		//
		// Please keep these symbol derivations ordered alphabetically.
		//

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<point_symbol_type> &point_symbol)
		{  }

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<polygon_symbol_type> &polygon_symbol)
		{  }

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<polyline_symbol_type> &polyline_symbol)
		{  }


	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		SymbolVisitorBase &
		operator=(
				const SymbolVisitorBase &);

	};


	template <class SymbolType>
	inline
	SymbolVisitorBase<SymbolType>::~SymbolVisitorBase()
	{  }

}

#endif // GPLATES_PRESENTATION_SYMBOLVISITOR_H

