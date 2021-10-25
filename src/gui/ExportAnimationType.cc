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

				export_type_name_map[RECONSTRUCTED_GEOMETRIES]    =QObject::tr("Reconstructed Geometries");
				export_type_name_map[PROJECTED_GEOMETRIES]        =QObject::tr("Projected Geometries (and Rasters)");
				export_type_name_map[IMAGE]                       =QObject::tr("Image (screenshot)");
				export_type_name_map[COLOUR_RASTER]               =QObject::tr("Colour Raster");
				export_type_name_map[NUMERICAL_RASTER]            =QObject::tr("Numerical Raster");
				export_type_name_map[VELOCITIES]                  =QObject::tr("Velocities");
				export_type_name_map[RESOLVED_TOPOLOGIES]         =QObject::tr("Resolved Topologies (General)");
				export_type_name_map[RESOLVED_TOPOLOGIES_CITCOMS] =QObject::tr("Resolved Topologies (CitcomS specific)");
				export_type_name_map[RELATIVE_TOTAL_ROTATION]     =QObject::tr("Relative Total Rotation");
				export_type_name_map[EQUIVALENT_TOTAL_ROTATION]   =QObject::tr("Equivalent Total Rotation");
				export_type_name_map[RELATIVE_STAGE_ROTATION]     =QObject::tr("Relative Stage Rotation");
				export_type_name_map[EQUIVALENT_STAGE_ROTATION]   =QObject::tr("Equivalent Stage Rotation");
				export_type_name_map[FLOWLINES]                   =QObject::tr("Flowlines");
				export_type_name_map[MOTION_PATHS]                =QObject::tr("Motion Paths");
				export_type_name_map[CO_REGISTRATION]             =QObject::tr("Co-registration data");

				return export_type_name_map;
			}


			std::map<Type, QString>
			initialise_export_type_description_map()
			{
				std::map<Type, QString> export_type_description_map;

				export_type_description_map[RECONSTRUCTED_GEOMETRIES] =
						QObject::tr(
							"<html><body>"
							"<p>Export reconstructed geometries.</p>"
							"</body></html>");
				export_type_description_map[PROJECTED_GEOMETRIES] =
						QObject::tr(
							"<html><body>"
							"<p>Export projected geometries (and projected raster) data.</p>"
							"</body></html>");
				export_type_description_map[IMAGE] =
						QObject::tr(
							"<html><body>"
							"<p>Export image (screenshot) of current view (globe or map)."
							"</body></html>");
				export_type_description_map[COLOUR_RASTER] =
						QObject::tr(
							"<html><body>"
							"<p>Export 8-bit (per channel) RGBA (or RGB) coloured raster data:</p>"
							"<ul>"
							"<li>Exports each visible raster layer to a single file (per time step).</li>"
							"<li>RGBA (and RGB) raster layers contain colour pixels.</li>"
							"<li>Numerical raster layers converted to colour using layer's palette.</li>"
							"<li>Geo-referenced region stored in raster formats that support it.</li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[NUMERICAL_RASTER] =
						QObject::tr(
							"<html><body>"
							"Export 32-bit floating-point numerical (non-coloured) raster data:"
							"<ul>"
							"<li>Exports each visible (numerical) raster layer to a single file (per time step).</li>"
							"<li>RGBA (and RGB) raster layers are not exported.</li>"
							"<li>Numerical raster layers contain floating-point pixels.</li>"
							"<li>NaN no-data value stored in pixels not covered by raster data.</li>"
							"<li>Geo-referenced region stored in raster.</li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[VELOCITIES] =
						QObject::tr(
							"<html><body>"
							"<p>Export velocity data.</p>"
							"</body></html>");
				export_type_description_map[RESOLVED_TOPOLOGIES] = 
						QObject::tr(
							"<html><body>"
							"<p>Export resolved topologies:</p>"
							"<ul>"
							"<li>Exports resolved topological lines and polygons (but not networks) for any feature type.</li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[RESOLVED_TOPOLOGIES_CITCOMS] = 
						QObject::tr(
							"<html><body>"
							"<p>Export resolved topologies for use by CitcomS software:</p>"
							"<ul>"
							"<li>Exports boundaries of resolved topological closed plate polygons/networks.</li>"
							"<li>Optionally exports the subsegment geometries of polygon/network boundaries.</li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[RELATIVE_TOTAL_ROTATION] = 
						QObject::tr(
							"<html><body>"
							"<p>Export relative total rotation data:</p>"
							"<ul>"
							"<li><em>relative</em> is between a moving/fixed plate pair.</li>"
							"<li><em>total</em> is from the export reconstruction time to present day.</li>"
							"<li>Each line in latitude/longitude format will contain:<br />"
							"<tt>moving_plate_id euler_pole_lat euler_pole_lon euler_pole_angle fixed_plate_id</tt></li>"
							"<li>Each line in 3D cartesian format will contain:<br />"
							"<tt>moving_plate_id euler_pole_x euler_pole_y euler_pole_z euler_pole_angle fixed_plate_id</tt></li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[EQUIVALENT_TOTAL_ROTATION] = 
						QObject::tr(
							"<html><body>"
							"<p>Export equivalent total rotation data:</p>"
							"<ul>"
							"<li><em>equivalent</em> is from an exported plate id to the anchor plate.</li>"
							"<li><em>total</em> is from the export reconstruction time to present day.</li>"
							"<li>Each line in latitude/longitude format will contain:<br />"
							"<tt>plate_id euler_pole_lat euler_pole_lon euler_pole_angle</tt></li>"
							"<li>Each line in 3D cartesian format will contain:<br />"
							"<tt>plate_id euler_pole_x euler_pole_y euler_pole_z euler_pole_angle</tt></li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[RELATIVE_STAGE_ROTATION] = 
						QObject::tr(
							"<html><body>"
							"<p>Export relative stage rotation data:</p>"
							"<ul>"
							"<li><em>relative</em> is between a moving/fixed plate pair.</li>"
							"<li><em>stage</em> is from <tt>t+interval</tt> Ma to <tt>t</tt> Ma where "
							"<tt>t</tt> is the export reconstruction time.</li>"
							"<li>Each line in latitude/longitude format will contain:<br />"
							"<tt>moving_plate_id stage_pole_lat stage_pole_lon stage_pole_angle fixed_plate_id</tt></li>"
							"<li>Each line in 3D cartesian format will contain:<br />"
							"<tt>moving_plate_id stage_pole_x stage_pole_y stage_pole_z stage_pole_angle fixed_plate_id</tt></li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[EQUIVALENT_STAGE_ROTATION] = 
						QObject::tr(
							"<html><body>"
							"<p>Export equivalent stage rotation data:</p>"
							"<ul>"
							"<li><em>equivalent</em>' is from an exported plate id to the anchor plate.</li>"
							"<li><em>stage</em> is from <tt>t+interval</tt> Ma to <tt>t</tt> Ma where "
							"<tt>t</tt> is the export reconstruction time.</li>"
							"<li>Each line in latitude/longitude format will contain:<br />"
							"<tt>plate_id stage_pole_lat stage_pole_lon stage_pole_angle</tt></li>"
							"<li>Each line in 3D cartesian format will contain:<br />"
							"<tt>plate_id stage_pole_x stage_pole_y stage_pole_z stage_pole_angle</tt></li>"
							"</ul>"
							"</body></html>");
				export_type_description_map[FLOWLINES] =
						QObject::tr(
							"<html><body>"
							"<p>Export flowlines.</p>"
							"</body></html>");
				export_type_description_map[MOTION_PATHS] =
						QObject::tr(
							"<html><body>"
							"<p>Export motion tracks.</p>"
							"</body></html>");
				export_type_description_map[CO_REGISTRATION] =
						QObject::tr(
							"<html><body>"
							"<p>Co-registration data for data-mining.</p>"
							"</body></html>");

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
				export_format_description_map[NETCDF]          =QObject::tr("NetCDF (*.nc)");
				export_format_description_map[GMT_NETCDF]      =QObject::tr("GMT NetCDF (*.grd)");
				export_format_description_map[GEOTIFF]         =QObject::tr("GeoTIFF (*.tif)");
				export_format_description_map[ERDAS_IMAGINE]   =QObject::tr("Erdas Imagine (*.img)");
				export_format_description_map[ERMAPPER]        =QObject::tr("ERMapper (*.ers)");
				export_format_description_map[CITCOMS_GLOBAL]  =QObject::tr("CitcomS global (*)");
				export_format_description_map[TERRA_TEXT]      =QObject::tr("Terra text format (*)");

				return export_format_description_map;
			}


			std::map<Format, QString>
			initialise_export_format_filename_extension_map()
			{
				std::map<Format, QString> export_format_filename_extension_map;

				export_format_filename_extension_map[GMT]             ="xy";
				export_format_filename_extension_map[GPML]            ="gpml";
				export_format_filename_extension_map[SHAPEFILE]       ="shp";
				export_format_filename_extension_map[OGRGMT]          ="gmt";
				export_format_filename_extension_map[SVG]             ="svg";
				export_format_filename_extension_map[CSV_COMMA]       ="csv";
				export_format_filename_extension_map[CSV_SEMICOLON]   ="csv";
				export_format_filename_extension_map[CSV_TAB]         ="csv";
				export_format_filename_extension_map[BMP]             ="bmp";
				export_format_filename_extension_map[JPG]             ="jpg";
				export_format_filename_extension_map[JPEG]            ="jpeg";
				export_format_filename_extension_map[PNG]             ="png";
				export_format_filename_extension_map[PPM]             ="ppm";
				export_format_filename_extension_map[TIFF]            ="tiff";
				export_format_filename_extension_map[XBM]             ="xbm";
				export_format_filename_extension_map[XPM]             ="xpm";
				export_format_filename_extension_map[NETCDF]          ="nc";
				export_format_filename_extension_map[GMT_NETCDF]      ="grd";
				export_format_filename_extension_map[GEOTIFF]         ="tif";
				export_format_filename_extension_map[ERDAS_IMAGINE]   ="img";
				export_format_filename_extension_map[ERMAPPER]        ="ers";
				export_format_filename_extension_map[CITCOMS_GLOBAL]  ="";
				export_format_filename_extension_map[TERRA_TEXT]      ="";

				return export_format_filename_extension_map;
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


const QString &
GPlatesGui::ExportAnimationType::get_export_format_filename_extension(
		Format format)
{
	static std::map<Format, QString> s_export_format_filename_extension_map =
			initialise_export_format_filename_extension_map();

	return s_export_format_filename_extension_map[format];
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
