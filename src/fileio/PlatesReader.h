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

#ifndef _GPLATES_PLATESREADER_H_
#define _GPLATES_PLATESREADER_H_

#include <iostream>
#include "Reader.h"
#include "geo/DataGroup.h"

namespace GPlatesFileIO
{
	/** 
	 * PlatesReader is responsible for converting an input stream in
	 * the Plates-4 data format into the gPlates internal representation.
	 */
	class PlatesReader : public Reader
	{
		public:
			PlatesReader(std::istream&) { }

			/**
			 * Fill a DataGroup.
			 * @role ConcreteCreator::FactoryMethod() in the Factory
			 *   Method design pattern (p107).
			 */
			void
			Read(GPlatesGeo::DataGroup&);
	};
}

#endif  // _GPLATES_PLATESREADER_H_
