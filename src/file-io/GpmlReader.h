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

#ifndef GPLATES_FILEIO_GPMLREADER_H
#define GPLATES_FILEIO_GPMLREADER_H

#include "File.h"
#include "FileInfo.h"
#include "GpmlPropertyStructuralTypeReader.h"
#include "ReadErrorAccumulation.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesFileIO
{
	class ExternalProgram;


	class GpmlReader
	{
	public:
		static
		void
		read_file(
				File::Reference &file,
				const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
				ReadErrorAccumulation &read_errors,
				bool use_gzip = false);

		static
		const ExternalProgram &
		gunzip_program();

	private:
		static const ExternalProgram *s_gunzip_program;
	};
}

#endif  // GPLATES_FILEIO_GPMLREADER_H
