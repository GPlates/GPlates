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

#ifndef _GPLATES_FILEIO_PLATESWRITER_H_
#define _GPLATES_FILEIO_PLATESWRITER_H_

#include "WriterVisitor.h"

namespace GPlatesFileIO
{
	/** 
	 * This class is responsible for taking the internal representation
	 * of the data and converting it into a stream (e.g.\ a file).
	 * Subclasses need only supply implementations of the Visit() methods 
	 * (from Visitor) that they require.
	 * @role 
	 *   - ConcreteBuilder in the Builder pattern (p97).
	 *   - ConcreteVisitor1 in the Visitor pattern (p331).
	 * @todo Yet to be implemented.
	 */
	class PlatesWriter : public WriterVisitor
	{
		public:
			using namespace GPlatesGeo;

			virtual void
			Visit(const PointOnSphere&);

			virtual void
			Visit(const LineData&);

			/**
			 * @warning After a call to PrintOut, no more "Visit()ing" can occur
			 *   because the internal representation is "frozen"; i.e. the method
			 *   std::ostringstream::freeze() is called on _strstream.
			 * @role ConcreteBuilder::GetResult() in the Builder pattern (p97).
			 */
			virtual bool
			PrintOut(std::ostream&);

		private:
			/**
			 * Holds the accumulated information.
			 */
			std::ostringstream _strstream;
	};
}

#endif  // _GPLATES_FILEIO_PLATESWRITER_H_
