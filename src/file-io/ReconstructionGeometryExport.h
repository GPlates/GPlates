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

#ifndef GPLATES_FILE_IO_RECONSTRUCTIONGEOMETRYEXPORT_H
#define GPLATES_FILE_IO_RECONSTRUCTIONGEOMETRYEXPORT_H

#include <vector>
#include <QString>

#include "ErrorOpeningFileForWritingException.h"
#include "FeatureCollectionFileFormat.h"
#include "FileFormatNotSupportedException.h"
#include "ReconstructionGeometryExportImpl.h"
#include "ShapefileUtils.h"

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
	class ReconstructedFlowline;
	class ReconstructedMotionPath;
}

namespace GPlatesFileIO
{
	namespace ReconstructionGeometryExport
	{
		//! Formats of files that can export reconstruction geometries.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GMT,               //!< '.xy' extension.
			
			SHAPEFILE          //!< '.shp' extension.
		};


		/**
		 * Unspecialised class for communicating options to @a export_reconstruction_geometries.
		 *
		 * Contains no options.
		 *
		 * Specialise this class to define options for a derived type of @a ReconstructionGeometry.
		 */
		template <class ReconstructionGeometryType>
		class Options
		{ };


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		Format
		get_export_file_format(
				const QFileInfo& file_info);

		/**
		 * Exports @a ReconstructionGeometry objects of derived type @a ReconstructionGeometryType.
		 *
		 * @param export_format specifies which format to write.
		 * @param export_single_output_file specifies whether to write all reconstruction geometries
		 *        to a single file.
		 * @param export_per_input_file specifies whether to group
		 *        reconstruction geometries according to the input files their features came from
		 *        and write to corresponding output files.
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		template <class ReconstructionGeometryType>
		void
		export_reconstruction_geometries(
				const QString &filename,
				Format export_format,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geom_seq,
				const std::vector<const File::Reference *> &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				const Options<ReconstructionGeometryType> &export_options = Options<ReconstructionGeometryType>());


		/**
		 * Exports @a ReconstructedFeatureGeometry objects of derived type @a ReconstructionGeometryType.
		 *
		 * @param file_info file whose extension is used to determine which format to write.
		 * @param export_single_output_file specifies whether to write all reconstruction geometries
		 *        to a single file.
		 * @param export_per_input_file specifies whether to group
		 *        reconstruction geometries according to the input files their features came from
		 *        and write to corresponding output files.
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		template <class ReconstructionGeometryType>
		inline
		void
		export_reconstruction_geometries(
				const QString &filename,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geom_seq,
				const std::vector<const File::Reference *> &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				const Options<ReconstructionGeometryType> &export_options = Options<ReconstructionGeometryType>())
		{
			export_reconstruction_geometries(
					filename,
					get_export_file_format(filename),
					reconstruction_geom_seq,
					active_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					export_single_output_file,
					export_per_input_file,
					export_options);
		}
	}


	////////////////////
	// Implementation //
	////////////////////


	namespace ReconstructionGeometryExport
	{
		QString
		build_flat_structure_filename(
			const QString &export_path,
			const QString &collection_filename,
			const QString &export_filename);

		/**
		 * Builds output file name for folder-format output, and creates and subfolders
		 * if they do not already exist. 
		 */
		QString
		build_folder_structure_filename(
			const QString &export_path,
			const QString &collection_filename,
			const QString &export_filename);


		/**
		 * Template function for exporting as a single file.
		 *
		 * NOTE: This unspecialised function is intentionally not defined anywhere.
		 * Specisalisations must be defined for each derived type of @a ReconstructionGeometry.
		 */
		template <class ReconstructionGeometryType>
		void
		export_as_single_file(
				const QString &filename,
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<ReconstructionGeometryType> > &grouped_recon_geoms_seq,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<ReconstructionGeometryType> &export_options);

		//! Specialisation for @a ReconstructedFeatureGeometry - defined in '.cc' file.
		template <>
		void
		export_as_single_file<GPlatesAppLogic::ReconstructedFeatureGeometry>(
				const QString &filename,
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFeatureGeometry> > &
								grouped_recon_geoms_seq,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<GPlatesAppLogic::ReconstructedFeatureGeometry> &export_options);

		//! Specialisation for @a ReconstructedFlowline - defined in '.cc' file.
		template <>
		void
		export_as_single_file<GPlatesAppLogic::ReconstructedFlowline>(
				const QString &filename,
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFlowline> > &
								grouped_recon_geoms_seq,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<GPlatesAppLogic::ReconstructedFlowline> &export_options);

		//! Specialisation for @a ReconstructedMotionPath - defined in '.cc' file.
		template <>
		void
		export_as_single_file<GPlatesAppLogic::ReconstructedMotionPath>(
				const QString &filename,
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<GPlatesAppLogic::ReconstructedMotionPath> > &
								grouped_recon_geoms_seq,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<GPlatesAppLogic::ReconstructedMotionPath> &export_options);


		/**
		 * Template function for exporting as a collection.
		 *
		 * NOTE: This unspecialised function is intentionally not defined anywhere.
		 * Specisalisations must be defined for each derived type of @a ReconstructionGeometry.
		 */
		template <class ReconstructionGeometryType>
		void
		export_per_collection(
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<ReconstructionGeometryType> > &grouped_recon_geoms_seq,
				const QString &filename,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<ReconstructionGeometryType> &export_options);

		//! Specialisation for @a ReconstructedFeatureGeometry - defined in '.cc' file.
		template<>
		void
		export_per_collection<GPlatesAppLogic::ReconstructedFeatureGeometry>(
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFeatureGeometry> > &
								grouped_recon_geoms_seq,
				const QString &filename,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<GPlatesAppLogic::ReconstructedFeatureGeometry> &export_options);

		//! Specialisation for @a ReconstructedFlowline - defined in '.cc' file.
		template<>
		void
		export_per_collection<GPlatesAppLogic::ReconstructedFlowline>(
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFlowline> > &
								grouped_recon_geoms_seq,
				const QString &filename,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<GPlatesAppLogic::ReconstructedFlowline> &export_options);

		//! Specialisation for @a ReconstructedMotionPath - defined in '.cc' file.
		template<>
		void
		export_per_collection<GPlatesAppLogic::ReconstructedMotionPath>(
				Format export_format,
				const std::list< ReconstructionGeometryExportImpl::
						FeatureGeometryGroup<GPlatesAppLogic::ReconstructedMotionPath> > &
								grouped_recon_geoms_seq,
				const QString &filename,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				const Options<GPlatesAppLogic::ReconstructedMotionPath> &export_options);


		/**
		 * Template function for exporting as a single file.
		 *
		 * NOTE: This unspecialised function is intentionally not defined anywhere.
		 * Specisalisations must be defined for each derived type of @a ReconstructionGeometry.
		 */
		template <class ReconstructionGeometryType>
		void
		export_per_collection(
			const QString &filename,
			Format export_format,
			const std::list< ReconstructionGeometryExportImpl::
					FeatureCollectionFeatureGroup<ReconstructionGeometryType> > &grouped_features_seq,
			const std::vector<const File::Reference *> &referenced_files,
			const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
			const double &reconstruction_time,
			const Options<ReconstructionGeometryType> &export_options)
		{
			using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;

			std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> >::const_iterator 
				it = grouped_features_seq.begin(),
				end = grouped_features_seq.end();

			QFileInfo export_qfile_info(filename);
			QString export_path = export_qfile_info.absolutePath();
			QString export_filename = export_qfile_info.fileName();

			for (; it != end; ++it)
			{
				const File::Reference *file_ptr = it->file_ptr;	
				FileInfo file_info = file_ptr->get_file_info();
				QFileInfo qfile_info = file_info.get_qfileinfo();
				QString collection_filename = qfile_info.completeBaseName();

		#if 1
				// Folder-structure output
				QString output_filename = build_folder_structure_filename(
						export_path, collection_filename, export_filename);
		#else	
				// Flat-structure output.
				QString output_filename = build_flat_structure_filename(
						export_path, collection_filename, export_filename);
		#endif


				boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> kvd;
				ShapefileUtils::create_default_kvd_from_collection(file_ptr->get_feature_collection(),kvd);

				export_per_collection(
						export_format,
						it->feature_geometry_groups,
						output_filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						export_options);
			} // iterate over collections
		}


		template <class ReconstructionGeometryType>
		void
		export_reconstruction_geometries(
			const QString &filename,
			Format export_format,
			const std::vector<const ReconstructionGeometryType *> &reconstruction_geom_seq,
			const std::vector<const File::Reference *> &active_files,
			const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
			const double &reconstruction_time,
			bool export_single_output_file,
			bool export_per_input_file,
			const Options<ReconstructionGeometryType> &export_options)
		{
			using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;

			// Get the list of active reconstructable feature collection files that contain
			// the features referenced by the ReconstructedFeatureGeometry objects.
			feature_handle_to_collection_map_type feature_to_collection_map;
			std::vector<const File::Reference *> referenced_files;
			get_files_referenced_by_geometries(
					referenced_files, reconstruction_geom_seq, active_files,
					feature_to_collection_map);

			// Group the ReconstructionGeometry objects by their feature.
			std::list< FeatureGeometryGroup<ReconstructionGeometryType> > grouped_recon_geom_seq;
			group_reconstruction_geometries_with_their_feature(
					grouped_recon_geom_seq, reconstruction_geom_seq);


			// Group the feature-groups with their collections. 
			std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> > grouped_features_seq;
			group_feature_geom_groups_with_their_collection(
					feature_to_collection_map,
					grouped_features_seq,
					grouped_recon_geom_seq);


			if (export_single_output_file)
			{
				export_as_single_file(
						filename,
						export_format,
						grouped_recon_geom_seq,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						export_options);
			}

			if (export_per_input_file)
			{
				export_per_collection(
						filename,
						export_format,
						grouped_features_seq,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						export_options);
			}
		}
	}
}

#endif // GPLATES_FILE_IO_RECONSTRUCTIONGEOMETRYEXPORT_H