/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#ifndef _GPLATES_VISITOR_H_
#define _GPLATES_VISITOR_H_

namespace GPlatesGeo
{
	// Declarations to circumvent circular dependencies.
	class PointData;
	class LineData;
	class GridData;
	class DataGroup;
	class RotationData;

	/** 
	 * Abstract Visitor. 
	 * The default action for a Visit() is to do nothing.  Thus subclasses
	 * need only override those Visit() methods they are interested in. 
	 * @role NodeVisitor in the Visitor design pattern (p331).
	 */
	class Visitor
	{
		public:
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
			Visit(RotationData&) { }

			virtual void
			Visit(const RotationData&) { }

			virtual void
			Visit(DataGroup&) { }

			virtual void
			Visit(const DataGroup&) { }

		protected:
			Visitor() { }
	};
}

#endif  // _GPLATES_VISITOR_H_
