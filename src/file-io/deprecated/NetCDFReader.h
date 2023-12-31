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

#ifndef _GPLATES_FILEIO_NETCDFREADER_H_
#define _GPLATES_FILEIO_NETCDFREADER_H_

#include <wx/progdlg.h>
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
			static GPlatesGeo::GridData *Read (NcFile *ncf,
						wxProgressDialog *dlg = 0);
	};
}

#endif  // _GPLATES_FILEIO_NETCDFREADER_H_
