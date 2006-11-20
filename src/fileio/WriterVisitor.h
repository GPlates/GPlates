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

#ifndef _GPLATES_FILEIO_WRITERVISITOR_H_
#define _GPLATES_FILEIO_WRITERVISITOR_H_

#include <iostream>
#include "geo/Visitor.h"

namespace GPlatesFileIO
{
	/**
	 * The superclass for all the classes that will convert the
	 * internal GPlates representation of the data into some
	 * kind of output format.
	 * To use this class for output, do something like the following:
	 * @code
	 *   DataGroup *dg;
	 *   // Initialise dg in some way.
	 *   GPlatesWriter pw;
	 *   pw.Visit(dg);
	 *   pw.PrintOut(std::cout);
	 *   @endcode
	 * Thus, if you want to output a whole bunch of GeologicalData, just make
	 * @a gd a DataGroup - everything will be output as if it were magic!
	 * @role Builder in the Builder pattern (p97).
	 */
	class WriterVisitor : public GPlatesGeo::Visitor
	{
		public:
			/**
			 * @role Builder::BuildPart() in the Builder pattern (p97).
			 */
			virtual void
			PrintOut(std::ostream&) = 0;
	};
}

#endif  // _GPLATES_FILEIO_WRITERVISITOR_H_
