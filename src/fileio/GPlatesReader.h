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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_GPLATESREADER_H_
#define _GPLATES_FILEIO_GPLATESREADER_H_

#include <iostream>
#include <string>
#include "Reader.h"
#include "geo/DataGroup.h"

namespace GPlatesFileIO
{
	/** 
	 * GPlatesReader is responsible for converting an input stream in
	 * the GPlates data format into the GPlates internal representation.
	 */
	class GPlatesReader : public Reader
	{
		public:
			GPlatesReader(std::istream& istr)
				: _istr(istr) {  }

			/**
			 * Fill a DataGroup.
			 * Return NULL on error.
			 * It is the responsibility of the caller to delete the
			 * DataGroup.
			 * @role ConcreteCreator::FactoryMethod() in the Factory
			 *   Method design pattern (p107).
			 */
			GPlatesGeo::DataGroup*
			Read();

		private:
			std::istream& _istr;
	};
}

#endif  // _GPLATES_FILEIO_GPLATESREADER_H_
