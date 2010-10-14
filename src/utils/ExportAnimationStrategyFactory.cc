/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#endif
#include <boost/bind.hpp>
#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#	include <cstdint>
#endif

#include "ExportAnimationStrategyFactory.h"

GPlatesUtils::ExportAnimationStrategyFactory::ExporterIdType 
GPlatesUtils::ExportAnimationStrategyFactory::d_exporter_id_map; 

bool GPlatesUtils::ExportAnimationStrategyFactory::dummy=
	GPlatesUtils::ExportAnimationStrategyFactory::init_id_map();

#define REGISTER_EXPORTER( EXPORTER_NAME ) \
	d_exporter_id_map[EXPORTER_NAME]= \
		boost::bind( \
			&create_ ## EXPORTER_NAME  , _1,_2);

bool
GPlatesUtils::ExportAnimationStrategyFactory::init_id_map()
{
	REGISTER_EXPORTER(RECONSTRUCTED_GEOMETRIES_GMT);
	REGISTER_EXPORTER(RECONSTRUCTED_GEOMETRIES_SHAPEFILE);
	REGISTER_EXPORTER(PROJECTED_GEOMETRIES_SVG);
	REGISTER_EXPORTER(MESH_VELOCITIES_GPML);
	REGISTER_EXPORTER(RESOLVED_TOPOLOGIES_GMT);
	REGISTER_EXPORTER(RELATIVE_ROTATION_CSV_COMMA);
	REGISTER_EXPORTER(RELATIVE_ROTATION_CSV_SEMICOLON);
	REGISTER_EXPORTER(RELATIVE_ROTATION_CSV_TAB);
	REGISTER_EXPORTER(EQUIVALENT_ROTATION_CSV_COMMA);
	REGISTER_EXPORTER(EQUIVALENT_ROTATION_CSV_SEMICOLON);
	REGISTER_EXPORTER(EQUIVALENT_ROTATION_CSV_TAB);
	REGISTER_EXPORTER(RASTER_BMP);
	REGISTER_EXPORTER(RASTER_JPG);
	REGISTER_EXPORTER(RASTER_JPEG);
	REGISTER_EXPORTER(RASTER_PNG);
	REGISTER_EXPORTER(RASTER_PPM);
	REGISTER_EXPORTER(RASTER_TIFF);
	REGISTER_EXPORTER(RASTER_XBM);
	REGISTER_EXPORTER(RASTER_XPM);
	REGISTER_EXPORTER(ROTATION_PARAMS_CSV_COMMA);
	REGISTER_EXPORTER(ROTATION_PARAMS_CSV_SEMICOLON);
	REGISTER_EXPORTER(ROTATION_PARAMS_CSV_TAB);
	return true;
}
GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
GPlatesUtils::ExportAnimationStrategyFactory::create_exporter(
		Exporter_ID id,
		GPlatesGui::ExportAnimationContext &export_context,
		const GPlatesGui::ExportAnimationStrategy::Configuration &cfg)
{
	//TODO: make the exporters singleton
	if(d_exporter_id_map.find(id)!=d_exporter_id_map.end())
		return d_exporter_id_map[id](export_context,cfg);
	else
		return create_DUMMY_EXPORTER(export_context,cfg);
}

