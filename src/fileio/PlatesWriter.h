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

#ifndef _GPLATES_PLATESWRITER_H_
#define _GPLATES_PLATESWRITER_H_

#include <iostream>
#include "WriterVisitor.h"

namespace GPlatesFileIO
{
	/** 
	 * This class is responsible for taking the internal representation
	 * of the data and converting it into a stream (e.g.\ a file).
	 * Subclasses need only supply implementations of the Visit() methods 
	 * (from Visitor) that they require.
	 * @todo Yet to be implemented.
	 * @see The Builder pattern (p97) for a possible method of 
	 *   implementing this class.
	 */
	class PlatesWriter : public WriterVisitor
	{
		public:
			PlatesWriter(std::ostream&) { }
	};
}

#endif  // _GPLATES_PLATESWRITER_H_
