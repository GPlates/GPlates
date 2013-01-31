/* $Id$ */

/**
 * \file Exports multi-point vector fields to a file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H
#define GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H

#include <vector>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class MultiPointVectorField;
}

namespace GPlatesModel
{
	class Gpgim;
	class ModelInterface;
}

namespace GPlatesFileIO
{
	namespace MultiPointVectorFieldExport
	{
		//
		// Export *velocity* multi-point vector fields.
		//


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to the GPML file format.
		 *
		 * @param export_single_output_file specifies whether to write all velocity vector fields to a single file.
		 * @param export_per_input_file specifies whether to group velocity vector fields according
		 *        to the input files their features came from and write to corresponding output files.
		 * @param export_separate_output_directory_per_input_file
		 *        Save each exported file to a different directory based on the file basename.
		 *        Only applies if @a export_per_input_file is 'true'.
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_velocity_vector_fields_to_gpml_format(
				const QString &filename,
				const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
				const GPlatesModel::Gpgim &gpgim,
				GPlatesModel::ModelInterface &model,
				const std::vector<const File::Reference *> &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file);
	}
}

#endif // GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H

