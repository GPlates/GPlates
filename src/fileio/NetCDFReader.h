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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_NETCDFREADER_H_
#define _GPLATES_FILEIO_NETCDFREADER_H_

#include "Reader.h"
#include "geo/GridData.h"

class NcFile;

namespace GPlatesFileIO
{
	/** 
	 * NetCDFReader is responsible for converting an input stream in
	 * the NetCDF data format into the GPlates internal representation.
	 */
	class NetCDFReader
	{
		public:
			/**
			 * Create a GridData object.
			 * Return NULL on error.
			 */
			static GPlatesGeo::GridData *Read (NcFile *ncf);
	};
}

#endif  // _GPLATES_FILEIO_NETCDFREADER_H_
