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
#include <boost/bind.hpp>
#include "ExportAnimationStrategyFactory.h"

GPlatesUtils::ExportAnimationStrategyFactory::ExporterIdType 
GPlatesUtils::ExportAnimationStrategyFactory::d_exporter_id_map; 

bool GPlatesUtils::ExportAnimationStrategyFactory::dummy=
	GPlatesUtils::ExportAnimationStrategyFactory::init_id_map();

bool
GPlatesUtils::ExportAnimationStrategyFactory::init_id_map()
{
	//TODO: define macro for the following code
	d_exporter_id_map[RECONSTRUCTED_GEOMETRIES_GMT]=
		boost::bind(
				&create_RECONSTRUCTED_GEOMETRIES_GMT,
				_1,
				_2);
	d_exporter_id_map[RECONSTRUCTED_GEOMETRIES_SHAPEFILE]=
		boost::bind(
				&create_RECONSTRUCTED_GEOMETRIES_SHAPEFILE,
				_1,
				_2);
	d_exporter_id_map[PROJECTED_GEOMETRIES_SVG]=
		boost::bind(
				&create_PROJECTED_GEOMETRIES_SVG,
				_1,
				_2);
	d_exporter_id_map[MESH_VILOCITIES_GPML]=
		boost::bind(
				&create_MESH_VILOCITIES_GPML,
				_1,
				_2);
	d_exporter_id_map[RESOLVED_TOPOLOGIES_GMT]=
		boost::bind(
				&create_RESOLVED_TOPOLOGIES_GMT,
				_1,
				_2);
	d_exporter_id_map[RELATIVE_ROTATION_CSV_COMMA]=
		boost::bind(
				&create_RELATIVE_ROTATION_CSV_COMMA,
				_1,
				_2);
	d_exporter_id_map[RELATIVE_ROTATION_CSV_SEMICOLON]=
		boost::bind(
				&create_RELATIVE_ROTATION_CSV_SEMICOLON,
				_1,
				_2);
	d_exporter_id_map[RELATIVE_ROTATION_CSV_TAB]=
		boost::bind(
				&create_RELATIVE_ROTATION_CSV_TAB,
				_1,
				_2);
	d_exporter_id_map[EQUIVALENT_ROTATION_CSV_COMMA]=
		boost::bind(
				&create_EQUIVALENT_ROTATION_CSV_COMMA,
				_1,
				_2);
	d_exporter_id_map[EQUIVALENT_ROTATION_CSV_SEMICOLON]=
		boost::bind(
				&create_EQUIVALENT_ROTATION_CSV_SEMICOLON,
				_1,
				_2);
	d_exporter_id_map[EQUIVALENT_ROTATION_CSV_TAB]=
		boost::bind(
				&create_EQUIVALENT_ROTATION_CSV_TAB,
				_1,
				_2);
	d_exporter_id_map[RASTER_BMP]=
		boost::bind(
				&create_RASTER_BMP,
				_1,
				_2);
	d_exporter_id_map[RASTER_JPG]=
		boost::bind(
				&create_RASTER_JPG,
				_1,
				_2);
	d_exporter_id_map[RASTER_JPEG]=
		boost::bind(
				&create_RASTER_JPEG,
				_1,
				_2);
	d_exporter_id_map[RASTER_PNG]=
		boost::bind(
				&create_RASTER_PNG,
				_1,
				_2);
	d_exporter_id_map[RASTER_PPM]=
		boost::bind(
				&create_RASTER_PPM,
				_1,
				_2);
	d_exporter_id_map[RASTER_TIFF]=
		boost::bind(
				&create_RASTER_TIFF,
				_1,
				_2);
	d_exporter_id_map[RASTER_XBM]=
		boost::bind(
				&create_RASTER_XBM,
				_1,
				_2);
	d_exporter_id_map[RASTER_XPM]=
		boost::bind(
				&create_RASTER_XPM,
				_1,
				_2);
				
	return true;
}
GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
GPlatesUtils::ExportAnimationStrategyFactory::create_exporter(
		Exporter_ID id,
		GPlatesGui::ExportAnimationContext &export_context,
		const GPlatesGui::ExportAnimationStrategy::Configuration &cfg)
{
	//TODO: make it singleton
	if(d_exporter_id_map.find(id)!=d_exporter_id_map.end())
		return d_exporter_id_map[id](export_context,cfg);
	else
		return create_DUMMY_EXPORTER(export_context,cfg);
}

