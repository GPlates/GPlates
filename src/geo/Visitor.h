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

#ifndef _GPLATES_GEO_VISITOR_H_
#define _GPLATES_GEO_VISITOR_H_

namespace GPlatesGeo
{
	// Declarations to circumvent circular dependencies.
	class PointData;
	class LineData;
	class GridData;
	class DataGroup;
	class Tree;

	/** 
	 * Abstract Visitor. 
	 * The default action for a Visit() is to do nothing.  Thus subclasses
	 * need only override those Visit() methods they are interested in. 
	 * @role NodeVisitor in the Visitor design pattern (p331).
	 */
	class Visitor
	{
		public:
			virtual
			~Visitor() { }

			virtual void
			Visit(PointData&) { }

			virtual void
			Visit(const PointData&) { }
			
			virtual void
			Visit(LineData&) { }

			virtual void
			Visit(const LineData&) { }

			virtual void
			Visit(GridData&) { }

			virtual void
			Visit(const GridData&) { }

			virtual void
			Visit(DataGroup&) { }

			virtual void
			Visit(const DataGroup&) { }

			virtual void
			Visit(Tree&) { }

			virtual void
			Visit(const Tree&) { }

		protected:
			Visitor() { }
	};
}

#endif  // _GPLATES_GEO_VISITOR_H_
