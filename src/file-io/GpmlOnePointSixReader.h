/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GPMLONEPOINTSIXREADER_H
#define GPLATES_FILEIO_GPMLONEPOINTSIXREADER_H

#include "FileInfo.h"
#include "ReadErrorAccumulation.h"
#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	class ExternalProgram;


	class GpmlOnePointSixReader
	{
	public:
		static
		void
		read_file(
				FileInfo &fileinfo,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors,
				bool use_gzip = false);

		static
		const ExternalProgram &
		gunzip_program();

	private:
		static const ExternalProgram *s_gunzip_program;
	};
}

#endif  // GPLATES_FILEIO_GPMLONEPOINTSIXREADER_H
