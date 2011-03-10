/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTOPTIONSUTILS_H
#define GPLATES_GUI_EXPORTOPTIONSUTILS_H


namespace GPlatesGui
{
	namespace ExportOptionsUtils
	{
		/**
		 * Options useful when exporting to Shapefile - either a single file or multiple files.
		 */
		struct ExportFileOptions
		{
			explicit
			ExportFileOptions(
					bool export_to_a_single_file_ = true,
					bool export_to_multiple_files_ = true) :
				export_to_a_single_file(export_to_a_single_file_),
				export_to_multiple_files(export_to_multiple_files_)
			{  }


			/**
			 * Export all @a ReconstructionGeometry derived objects to a single export file.
			 */
			bool export_to_a_single_file;

			/**
			 * Export @a ReconstructionGeometry derived objects to multiple export files.
			 *
			 * Each output file corresponds to an input file that the features
			 * (that generated the reconstruction geometries) came from.
			 */
			bool export_to_multiple_files;
		};
	}
}

#endif // GPLATES_GUI_EXPORTOPTIONSUTILS_H
