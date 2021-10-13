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

#include <algorithm>
#include <map>
#include <boost/static_assert.hpp>
#include <QObject>

#include "ExportAnimationType.h"

#include "global/CompilerWarnings.h"
#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"


namespace GPlatesGui
{
	namespace ExportAnimationType
	{
		namespace
		{
			std::map<Type, QString>
			initialise_export_type_name_map()
			{
				std::map<Type, QString> export_type_name_map;

				export_type_name_map[RECONSTRUCTED_GEOMETRIES]=QObject::tr("Reconstructed Geometries");
				export_type_name_map[PROJECTED_GEOMETRIES]    =QObject::tr("Projected Geometries");
				export_type_name_map[MESH_VELOCITIES]         =QObject::tr("Colat/lon Mesh Velocities");
				export_type_name_map[RESOLVED_TOPOLOGIES]     =QObject::tr("Resolved Topologies");
				export_type_name_map[RELATIVE_ROTATION]       =QObject::tr("Relative Total Rotation");
				export_type_name_map[EQUIVALENT_ROTATION]     =QObject::tr("Equivalent Total Rotation");
				export_type_name_map[ROTATION_PARAMS]         =QObject::tr("Equivalent Stage Rotation");
				export_type_name_map[RASTER]                  =QObject::tr("Raster");
				export_type_name_map[FLOWLINES]               =QObject::tr("Flowlines");
				export_type_name_map[MOTION_PATHS]            =QObject::tr("Motion Paths");
				export_type_name_map[CO_REGISTRATION]         =QObject::tr("Co-registration data");

				return export_type_name_map;
			}


			std::map<Type, QString>
			initialise_export_type_description_map()
			{
				std::map<Type, QString> export_type_description_map;

				export_type_description_map[RECONSTRUCTED_GEOMETRIES] =
						QObject::tr("Export reconstructed geometries.");
				export_type_description_map[PROJECTED_GEOMETRIES] =
						QObject::tr("Export projected geometries data.");
				export_type_description_map[MESH_VELOCITIES] =
						QObject::tr("Export velocity data.");
				export_type_description_map[RESOLVED_TOPOLOGIES] = 
						QObject::tr(
							"Export resolved topologies:\n"
							"- exports resolved topological closed plate polygons,\n"
							"- optionally exports the subsegment geometries of polygon boundaries.\n");
				export_type_description_map[RELATIVE_ROTATION] = 
						QObject::tr(
							"Export relative total rotation data:\n"
							"- 'relative' is between a moving/fixed plate pair,\n"
							"- 'total' is from the export reconstruction time to present day.\n"
							"Each line in exported file(s) will contain the following entries...\n"
							" 'moving_plate_id' 'euler_pole_lat' 'euler_pole_lon' 'euler_pole_angle' 'fixed_plate_id'\n");
				export_type_description_map[EQUIVALENT_ROTATION] = 
						QObject::tr(
							"Export equivalent total rotation data:\n"
							"- 'equivalent' is from an exported plate id to the anchor plate,\n"
							"- 'total' is from the export reconstruction time to present day.\n"
							"Each line in exported file(s) will contain the following entries...\n"
							" 'plate_id' 'euler_pole_lat' 'euler_pole_lon' 'euler_pole_angle'\n");
				export_type_description_map[ROTATION_PARAMS] = 
						QObject::tr(
							"Export equivalent stage(1My) rotation data:\n"
							"- 'equivalent' is from an exported plate id to the anchor plate,\n"
							"- 'stage' is from 't+1' Ma to 't' Ma where 't' is the export reconstruction time.\n"
							"Each line in exported file(s) will contain the following entries...\n"
							" 'plate_id' 'stage_pole_x' 'stage_pole_y' 'stage_pole_z' 'stage_pole_1My_angle'\n");
				export_type_description_map[RASTER] =
						QObject::tr("Export raster data as image.");
				export_type_description_map[FLOWLINES] =
						QObject::tr("Export flowlines.");
				export_type_description_map[MOTION_PATHS] =
						QObject::tr("Export motion tracks.");
				export_type_description_map[CO_REGISTRATION] =
						QObject::tr("Co-registration data for data-mining.");

				return export_type_description_map;
			}


			std::map<Format, QString>
			initialise_export_format_description_map()
			{
				std::map<Format, QString> export_format_description_map;

				export_format_description_map[GMT]             =QObject::tr("GMT (*.xy)");
				export_format_description_map[GPML]            =QObject::tr("GPML (*.gpml)");
				export_format_description_map[SHAPEFILE]       =QObject::tr("Shapefiles (*.shp)");
				export_format_description_map[OGRGMT]		   =QObject::tr("OGR-GMT (*.gmt)");
				export_format_description_map[SVG]             =QObject::tr("SVG (*.svg)");
				export_format_description_map[CSV_COMMA]       =QObject::tr("CSV file (comma delimited) (*.csv)");
				export_format_description_map[CSV_SEMICOLON]   =QObject::tr("CSV file (semicolon delimited) (*.csv)");
				export_format_description_map[CSV_TAB]         =QObject::tr("CSV file (tab delimited) (*.csv)");
				export_format_description_map[BMP]             =QObject::tr("Windows Bitmap (*.bmp)");
				export_format_description_map[JPG]             =QObject::tr("Joint Photographic Experts Group (*.jpg)");
				export_format_description_map[JPEG]            =QObject::tr("Joint Photographic Experts Group (*.jpeg)");
				export_format_description_map[PNG]             =QObject::tr("Portable Network Graphics (*.png)");
				export_format_description_map[PPM]             =QObject::tr("Portable Pixmap (*.ppm)");
				export_format_description_map[TIFF]            =QObject::tr("Tagged Image File Format (*.tiff)");
				export_format_description_map[XBM]             =QObject::tr("X11 Bitmap (*.xbm)");
				export_format_description_map[XPM]             =QObject::tr("X11 Pixmap (*.xpm)");

				return export_format_description_map;
			}
		}
	}
}


const QString &
GPlatesGui::ExportAnimationType::get_export_type_name(
		Type type)
{
	static std::map<Type, QString> s_export_type_name_map =
			initialise_export_type_name_map();

	return s_export_type_name_map[type];
}


const QString &
GPlatesGui::ExportAnimationType::get_export_type_description(
		Type type)
{
	static std::map<Type, QString> s_export_type_description_map =
			initialise_export_type_description_map();

	return s_export_type_description_map[type];
}


const QString &
GPlatesGui::ExportAnimationType::get_export_format_description(
		Format format)
{
	static std::map<Format, QString> s_export_format_description_map =
			initialise_export_format_description_map();

	return s_export_format_description_map[format];
}

// For the BOOST_STATIC_ASSERT below with GCC 4.2.
DISABLE_GCC_WARNING("-Wold-style-cast")

GPlatesGui::ExportAnimationType::ExportID
GPlatesGui::ExportAnimationType::get_export_id(
		Type type,
		Format format)
{
	BOOST_STATIC_ASSERT(NUM_TYPES < (1 << 16) && NUM_FORMATS < (1 << 16));

	// Combine type and format as two unsigned 16-bit integers into one unsigned 32-bit integer.
	return (static_cast<ExportID>(type) << 16) | static_cast<ExportID>(format);
}

ENABLE_GCC_WARNING("-Wold-style-cast")

GPlatesGui::ExportAnimationType::Type
GPlatesGui::ExportAnimationType::get_export_type(
		ExportID export_id)
{
	// The export type is the high 16-bits of the export id.
	const boost::uint16_t type = ((export_id & 0xffff0000) >> 16);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			type < NUM_TYPES,
			GPLATES_ASSERTION_SOURCE);

	return static_cast<Type>(type);
}


GPlatesGui::ExportAnimationType::Format
GPlatesGui::ExportAnimationType::get_export_format(
		ExportID export_id)
{
	// The export format is the low 16-bits of the export id.
	const boost::uint16_t format = (export_id & 0xffff);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			format < NUM_FORMATS,
			GPLATES_ASSERTION_SOURCE);

	return static_cast<Format>(format);
}


std::vector<GPlatesGui::ExportAnimationType::Type>
GPlatesGui::ExportAnimationType::get_export_types(
		const std::vector<ExportID> &export_ids)
{
	std::vector<Type> export_types;
	export_types.reserve(export_ids.size()); // Avoid unnecessary reallocations.

	// Add the export types of each export id.
	std::vector<ExportID>::const_iterator export_id_iter;
	for (export_id_iter = export_ids.begin();
		export_id_iter != export_ids.end();
		++export_id_iter)
	{
		export_types.push_back(get_export_type(*export_id_iter));
	}

	// Sort in preparation for removing duplicate export types.
	std::sort(export_types.begin(), export_types.end());

	// Remove duplicates.
	export_types.erase(
			std::unique(export_types.begin(), export_types.end()),
			export_types.end());

	return export_types;
}


std::vector<GPlatesGui::ExportAnimationType::Format>
GPlatesGui::ExportAnimationType::get_export_formats(
		const std::vector<ExportID> &export_ids,
		Type export_type)
{
	std::vector<Format> formats;
	formats.reserve(export_ids.size()); // Avoid unnecessary reallocations.

	std::vector<ExportID>::const_iterator export_id_iter;
	for (export_id_iter = export_ids.begin();
		export_id_iter != export_ids.end();
		++export_id_iter)
	{
		if (get_export_type(*export_id_iter) == export_type)
		{
			formats.push_back(get_export_format(*export_id_iter));
		}
	}

	return formats;
}
