/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004 The GPlates Consortium
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
 */

#ifndef _GPLATES_FILEIO_NETCDFWRITER_H_
#define _GPLATES_FILEIO_NETCDFWRITER_H_

#include <string>
#include <wx/progdlg.h>
#include "geo/GridData.h"

namespace GPlatesFileIO
{
	/** 
	 * NetCDFWriter is responsible for outputting a GridData object,
	 * in the netCDF data format.
	 */
	class NetCDFWriter
	{
		public:
			/**
			 * Output a GridData object.
			 * Return false on error.
			 */
			static bool Write (const std::string &filename,
					GPlatesGeo::GridData *grid,
					wxProgressDialog *dlg = 0);
	};
}

#endif  // _GPLATES_FILEIO_NETCDFWRITER_H_
