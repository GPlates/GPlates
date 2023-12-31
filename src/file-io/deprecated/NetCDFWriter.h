/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
