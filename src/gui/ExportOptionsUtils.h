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


		/**
		 * Common rotations options useful when exporting either total or stage rotations.
		 */
		struct ExportRotationOptions
		{
			//! How to write out an identity rotation.
			enum IdentityRotationFormatType
			{
				WRITE_IDENTITY_AS_INDETERMINATE,
				WRITE_IDENTITY_AS_NORTH_POLE
			};

			//! How to write out a Euler pole.
			enum EulerPoleFormatType
			{
				WRITE_EULER_POLE_AS_CARTESIAN,
				WRITE_EULER_POLE_AS_LATITUDE_LONGITUDE,
			};


			explicit
			ExportRotationOptions(
					IdentityRotationFormatType identity_rotation_format_,
					EulerPoleFormatType euler_pole_format_) :
				identity_rotation_format(identity_rotation_format_),
				euler_pole_format(euler_pole_format_)
			{  }


			IdentityRotationFormatType identity_rotation_format;
			EulerPoleFormatType euler_pole_format;
		};


		/**
		 * Rotations options useful when exporting *stage* rotations only.
		 */
		struct ExportStageRotationOptions
		{
			explicit
			ExportStageRotationOptions(
					const double &time_interval_) :
				time_interval(time_interval_)
			{  }


			//! The stage rotation time interval (in My).
			double time_interval;
		};
	}
}

#endif // GPLATES_GUI_EXPORTOPTIONSUTILS_H
