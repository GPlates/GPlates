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

#ifndef _GPLATES_FILEIO_WRITERVISITOR_H_
#define _GPLATES_FILEIO_WRITERVISITOR_H_

#include <iostream>
#include "geo/Visitor.h"

namespace GPlatesFileIO
{
	/**
	 * The superclass for all the classes that will convert the
	 * internal gPlates representation of the data into some
	 * kind of output format.
	 * To use this class for output, do something like the following:
	 * @code
	 *   GeologicalData *gd;
	 *   // Initialise gd in some way.
	 *   PlatesWriter pw;
	 *   gd->Accept(pw);
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
			 * @return False if there was insufficient information for the 
			 *   the file to be written properly.
			 * @role Builder::BuildPart() in the Builder pattern (p97).
			 */
			virtual bool
			PrintOut(std::ostream&) = 0;
	};
}

#endif  // _GPLATES_FILEIO_WRITERVISITOR_H_
