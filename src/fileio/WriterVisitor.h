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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
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
