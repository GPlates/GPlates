/* $Id$ */

/**
 * @file 
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

#include <iterator>
#include "PlatesReader.h"

using namespace GPlatesGeo;

void
PlatesReader::Read(DataGroup& group)
{
	// Read in each LineData and add it to the group.
	LineData line;
	std::back_insert_iterator<Attributes_t> inserter;

	inserter = group.AttributesInserter();
	*inserter++ = "Region Number";
	*inserter++ = "Reference Number";
	// Et cetera.
}
