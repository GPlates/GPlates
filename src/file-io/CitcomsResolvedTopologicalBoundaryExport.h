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

#ifndef GPLATES_FILE_IO_CITCOMSRESOLVEDTOPOLOGICALBOUNDARYEXPORT_H
#define GPLATES_FILE_IO_CITCOMSRESOLVEDTOPOLOGICALBOUNDARYEXPORT_H

#include <vector>
#include <boost/optional.hpp>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}

	/**
	 * CitcomS-specific resolved topology export.
	 */
	namespace CitcomsResolvedTopologicalBoundaryExport
	{
		//! Formats of files that can export resolved topological boundaries.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GMT,               //!< '.xy' extension.

			OGRGMT,            //!< '.gmt' extension (and using OGR style gmt format).
			
			SHAPEFILE          //!< '.shp' extension.
		};


		/**
		 * Determine type of export file format based on filename extension.
		 *
		 * @param file_info file whose extension used to determine file format.
		 */
		Format
		get_export_file_format(
				const QFileInfo& file_info,
				const FeatureCollectionFileFormat::Registry &file_format_registry);


		/**
		 * Fine-grained control over the types of output files generated.
		 *
		 * Each of these boost optionals determines if the specific type of
		 * output is exported.
		 *
		 * The boost optionals containing placeholder strings represent
		 * the filename replacement of the substring defined by 'placeholder_format_string'
		 * in the function @a export_resolved_topological_boundaries - the 'file_basename'
		 * parameter of @a export_resolved_topological_boundaries is expected to contain
		 * that substring.
		 */

		// NOTE: check default_citcoms_resolved_topology_export_options 
		// in gui/ExportAnimationRegistry.cc  for the boolean defaults created in the actual gui 
		//
		struct OutputOptions
		{
			explicit
			OutputOptions(
					// Wrap polyline/polygon geometries to the dateline (mainly useful for ArcGIS shapefile users)...
					bool wrap_geometries_to_the_dateline_ = true,

					//
					// all polygon options
					//

					bool export_plate_polygons_to_all_polygons_file_        = false,
					bool export_network_polygons_to_all_polygons_file_      = false,
					bool export_slab_polygons_to_all_polygons_file_         = false,

					bool export_plate_boundaries_to_all_boundaries_file_    = false,
					bool export_network_boundaries_to_all_boundaries_file_  = false,
					bool export_slab_boundaries_to_all_boundaries_file_     = false,

					//
					// plate polygon options
					//

					bool export_individual_plate_polygon_files_      = false,
					bool export_plate_polygons_to_a_single_file_     = false,

					bool export_plate_boundaries_                    = false,

					//
					// network polygon options 
					//

					bool export_individual_network_polygon_files_       = false,
					bool export_network_polygons_to_a_single_file_      = false,

					bool export_network_boundaries_                     = false,

					//
					// slab polygon options 
					//

					bool export_individual_slab_polygon_files_      = false,
					bool export_slab_polygons_to_a_single_file_     = false,

					bool export_slab_boundaries_                    = false,

					//
					// all polygon placeholders
					//

					const QString &placeholder_all_polygons_                     = "polygons",

					const QString &placeholder_all_boundaries_                   = "boundaries",
					const QString &placeholder_all_boundaries_ridge_transform_   = "ridge_transform_boundaries",
					const QString &placeholder_all_boundaries_subduction_        = "subduction_boundaries",
					const QString &placeholder_all_boundaries_subduction_left_   = "subduction_boundaries_sL",
					const QString &placeholder_all_boundaries_subduction_right_  = "subduction_boundaries_sR",

					//
					// plate polygon place holders
					//

					const QString &placeholder_plate_polygons_                     = "platepolygons",

					const QString &placeholder_plate_boundaries_                   = "plate_boundaries",
					const QString &placeholder_plate_boundaries_ridge_transform_   = "plate_ridge_transform_boundaries",
					const QString &placeholder_plate_boundaries_subduction_        = "plate_subduction_boundaries",
					const QString &placeholder_plate_boundaries_subduction_left_   = "plate_subduction_boundaries_sL",
					const QString &placeholder_plate_boundaries_subduction_right_  = "plate_subduction_boundaries_sR",

					//
					// network polygon place holders
					//

					const QString &placeholder_networks_                             = "network_polygons",

					const QString &placeholder_network_boundaries_                   = "network_boundaries",
					const QString &placeholder_network_boundaries_ridge_transform_   = "network_ridge_transform_boundaries",
					const QString &placeholder_network_boundaries_subduction_        = "network_subduction_boundaries",
					const QString &placeholder_network_boundaries_subduction_left_   = "network_subduction_boundaries_sL",
					const QString &placeholder_network_boundaries_subduction_right_  = "network_subduction_boundaries_sR",

					//
					// slab polygon place holders 
					//

					const QString &placeholder_slab_polygons_             = "slab_polygons",

					const QString &placeholder_slab_edges_                = "slab_edges",
					const QString &placeholder_slab_edges_leading_        = "slab_edges_leading",
					const QString &placeholder_slab_edges_leading_left_   = "slab_edges_leading_sL",
					const QString &placeholder_slab_edges_leading_right_  = "slab_edges_leading_sR",
					const QString &placeholder_slab_edges_trench_         = "slab_edges_trench",
					const QString &placeholder_slab_edges_side_           = "slab_edges_side") :

				wrap_geometries_to_the_dateline(wrap_geometries_to_the_dateline_),

				//
				// all polygon options
				//

				export_plate_polygons_to_all_polygons_file(export_plate_polygons_to_all_polygons_file_),
				export_network_polygons_to_all_polygons_file(export_network_polygons_to_all_polygons_file_),
				export_slab_polygons_to_all_polygons_file(export_slab_polygons_to_all_polygons_file_),

				export_plate_boundaries_to_all_boundaries_file(export_plate_boundaries_to_all_boundaries_file_),
				export_network_boundaries_to_all_boundaries_file(export_network_boundaries_to_all_boundaries_file_),
				export_slab_boundaries_to_all_boundaries_file(export_slab_boundaries_to_all_boundaries_file_),

				//
				// plate polygon options
				//

				export_individual_plate_polygon_files(export_individual_plate_polygon_files_),
				export_plate_polygons_to_a_single_file(export_plate_polygons_to_a_single_file_),

				export_plate_boundaries(export_plate_boundaries_),

				//
				// network options
				//

				export_individual_network_polygon_files(export_individual_network_polygon_files_),
				export_network_polygons_to_a_single_file(export_network_polygons_to_a_single_file_),

				export_network_boundaries(export_network_boundaries_),

				//
				// slab polygon options 
				//

				export_individual_slab_polygon_files(export_individual_slab_polygon_files_),
				export_slab_polygons_to_a_single_file(export_slab_polygons_to_a_single_file_),

				export_slab_boundaries(export_slab_boundaries_),

				//
				// all polygon placeholders
				//

				placeholder_all_polygons(placeholder_all_polygons_),

				placeholder_all_boundaries(placeholder_all_boundaries_),
				placeholder_all_boundaries_ridge_transform(placeholder_all_boundaries_ridge_transform_),
				placeholder_all_boundaries_subduction(placeholder_all_boundaries_subduction_),
				placeholder_all_boundaries_subduction_left(placeholder_all_boundaries_subduction_left_),
				placeholder_all_boundaries_subduction_right(placeholder_all_boundaries_subduction_right_),

				//
				// plate polygon placeholders
				//

				placeholder_plate_polygons(placeholder_plate_polygons_),

				placeholder_plate_boundaries(placeholder_plate_boundaries_),
				placeholder_plate_boundaries_ridge_transform(placeholder_plate_boundaries_ridge_transform_),
				placeholder_plate_boundaries_subduction(placeholder_plate_boundaries_subduction_),
				placeholder_plate_boundaries_subduction_left(placeholder_plate_boundaries_subduction_left_),
				placeholder_plate_boundaries_subduction_right(placeholder_plate_boundaries_subduction_right_),

				//
				// network polygon placeholders
				//

				placeholder_networks(placeholder_networks_),

				placeholder_network_boundaries(placeholder_network_boundaries_),
				placeholder_network_boundaries_ridge_transform(placeholder_network_boundaries_ridge_transform_),
				placeholder_network_boundaries_subduction(placeholder_network_boundaries_subduction_),
				placeholder_network_boundaries_subduction_left(placeholder_network_boundaries_subduction_left_),
				placeholder_network_boundaries_subduction_right(placeholder_network_boundaries_subduction_right_),

				//
				// slab polygon placeholders
				//

				placeholder_slab_polygons(placeholder_slab_polygons_),

				placeholder_slab_edges(placeholder_slab_edges_),
				placeholder_slab_edges_leading(placeholder_slab_edges_leading_),
				placeholder_slab_edges_leading_left(placeholder_slab_edges_leading_left_),
				placeholder_slab_edges_leading_right(placeholder_slab_edges_leading_right_),
				placeholder_slab_edges_trench(placeholder_slab_edges_trench_),
				placeholder_slab_edges_side(placeholder_slab_edges_side_)

			{  }


			// Wrap polyline/polygon geometries to the dateline (mainly useful for ArcGIS shapefile users)...
			bool wrap_geometries_to_the_dateline;

			//
			// all polygon options 
			//

			bool export_plate_polygons_to_all_polygons_file;
			bool export_network_polygons_to_all_polygons_file;
			bool export_slab_polygons_to_all_polygons_file;

			bool export_plate_boundaries_to_all_boundaries_file;
			bool export_network_boundaries_to_all_boundaries_file;
			bool export_slab_boundaries_to_all_boundaries_file;

			//
			// plate polygon options 
			//

			bool export_individual_plate_polygon_files;
			bool export_plate_polygons_to_a_single_file;

			bool export_plate_boundaries;

			//
			// network polygon options 
			//

			bool export_individual_network_polygon_files;
			bool export_network_polygons_to_a_single_file;

			bool export_network_boundaries;

			//
			// slab polygon options 
			//

			bool export_individual_slab_polygon_files;
			bool export_slab_polygons_to_a_single_file;

			bool export_slab_boundaries;

			//
			// all polygon place holders 
			//
			// 
			QString placeholder_all_polygons;

			QString placeholder_all_boundaries;
			QString placeholder_all_boundaries_ridge_transform;
			QString placeholder_all_boundaries_subduction;
			QString placeholder_all_boundaries_subduction_left;
			QString placeholder_all_boundaries_subduction_right;

			//
			// plate polygon place holders 
			//

			QString placeholder_plate_polygons;

			QString placeholder_plate_boundaries;
			QString placeholder_plate_boundaries_ridge_transform;
			QString placeholder_plate_boundaries_subduction;
			QString placeholder_plate_boundaries_subduction_left;
			QString placeholder_plate_boundaries_subduction_right;

			//
			// network placeholder string.
			//

			QString placeholder_networks;

			QString placeholder_network_boundaries;
			QString placeholder_network_boundaries_ridge_transform;
			QString placeholder_network_boundaries_subduction;
			QString placeholder_network_boundaries_subduction_left;
			QString placeholder_network_boundaries_subduction_right;

			//
			// slab polygon subsegments placeholder strings.
			//

			QString placeholder_slab_polygons;

			QString placeholder_slab_edges;
			QString placeholder_slab_edges_leading;
			QString placeholder_slab_edges_leading_left;
			QString placeholder_slab_edges_leading_right;
			QString placeholder_slab_edges_trench;
			QString placeholder_slab_edges_side;
		
		};


		/**
		 * Exports resolved topologies and associated subsegments as specified
		 * by the options in @a output_options.
		 *
		 * @param export_format specifies which format to write.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_resolved_topological_boundaries(
				const QDir &target_dir,
				const QString &file_basename,
				const QString &placeholder_format_string,
				const OutputOptions &output_options,
				Format export_format,
				const std::vector<const GPlatesAppLogic::ReconstructionGeometry *> &resolved_topologies,
				const std::vector<const File::Reference *> &loaded_files,
				const std::vector<const File::Reference *> &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);
	}
}

#endif // GPLATES_FILE_IO_CITCOMSRESOLVEDTOPOLOGICALBOUNDARYEXPORT_H
