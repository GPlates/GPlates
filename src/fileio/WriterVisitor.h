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

#ifndef _GPLATES_WRITERVISITOR_H_
#define _GPLATES_WRITERVISITOR_H_

#include "geo/Visitor.h"

namespace GPlatesFileIO
{
	// FIXME: Need there be any functionality in this class? Do
	// we need this extra layer of generality between the visitors
	// and the file i/o?  Only time will tell...
	class WriterVisitor : public GPlatesGeo::Visitor
	{
		protected:
			WriterVisitor() {}
	};
}

#endif
