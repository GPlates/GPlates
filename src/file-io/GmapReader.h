/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_GMAPREADER_H
#define GPLATES_FILEIO_GMAPREADER_H

#include "File.h"
#include "FileInfo.h"
#include "ReadErrorAccumulation.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesFileIO
{
	class GmapReader
	{
		public:
		
			static
			void
			read_file(
				File::Reference &file,
				const GPlatesModel::Gpgim &gpgim,
				ReadErrorAccumulation &read_errors);
				
	};
}

#endif //GPLATES_FILEIO_GMAPREADER_H
