/* $Id$ */

/**
 * \file validate filename template.
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

#ifndef GPLATES_UTILS_EXPORTANIMATIONSTRATEGYFACTORY_H
#define GPLATES_UTILS_EXPORTANIMATIONSTRATEGYFACTORY_H

#include <boost/shared_ptr.hpp>
#include <QString>
#include <string>
#include <iostream>
#include <sstream>

#include "ExportAnimationStrategyExporterID.h"

#include "gui/ExportAnimationStrategy.h"
#include "gui/ExportRotationAnimationStrategy.h"
#include "gui/ExportVelocityAnimationStrategy.h"
#include "gui/ExportReconstructedGeometryAnimationStrategy.h"
#include "gui/ExportResolvedTopologyAnimationStrategy.h"
#include "gui/ExportSvgAnimationStrategy.h"
#include "gui/ExportRotationParamsAnimationStrategy.h"
#include "gui/ExportRasterAnimationStrategy.h"


namespace GPlatesGui
{
	class ExportAnimationContext;
}

namespace GPlatesUtils
{	
	class ExportAnimationStrategyFactory
	{
	public:	
			typedef std::map<
				Exporter_ID, 
				boost::function<
				GPlatesGui::ExportAnimationStrategy::non_null_ptr_type 
				(GPlatesGui::ExportAnimationContext&,
				const GPlatesGui::ExportAnimationStrategy::Configuration&)> 
				> ExporterIdType; 
		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_exporter(
				Exporter_ID,
				GPlatesGui::ExportAnimationContext&,
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg
					=GPlatesGui::ExportAnimationStrategy::Configuration("dummy_%u_%d_%A.gpml"));
	private:
		static
		bool
		init_id_map();

		static ExporterIdType d_exporter_id_map; 

		//TODO: define macro for the following code
		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RECONSTRUCTED_GEOMETRIES_GMT(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportReconstructedGeometryAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportReconstructedGeometryAnimationStrategy::GMT,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RECONSTRUCTED_GEOMETRIES_SHAPEFILE(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportReconstructedGeometryAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportReconstructedGeometryAnimationStrategy::SHAPEFILE,
					cfg);

		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_PROJECTED_GEOMETRIES_SVG(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportSvgAnimationStrategy::create(
					export_context,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_MESH_VELOCITIES_GPML(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportVelocityAnimationStrategy::create(
					export_context,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RESOLVED_TOPOLOGIES_GMT(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportResolvedTopologyAnimationStrategy::create(
					export_context,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RELATIVE_ROTATION_CSV_COMMA(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportRotationAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationAnimationStrategy::RELATIVE_COMMA,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RELATIVE_ROTATION_CSV_SEMICOLON(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportRotationAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationAnimationStrategy::RELATIVE_SEMI,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RELATIVE_ROTATION_CSV_TAB(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportRotationAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationAnimationStrategy::RELATIVE_TAB,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_EQUIVALENT_ROTATION_CSV_COMMA(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportRotationAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationAnimationStrategy::EQUIVALENT_COMMA,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_EQUIVALENT_ROTATION_CSV_SEMICOLON(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportRotationAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationAnimationStrategy::EQUIVALENT_SEMI,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_EQUIVALENT_ROTATION_CSV_TAB(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{
			return GPlatesGui::ExportRotationAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationAnimationStrategy::EQUIVALENT_TAB,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_BMP(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::BMP,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_JPG(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::JPG,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_JPEG(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::JPEG,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_PNG(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::PNG,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_PPM(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::PPM,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_TIFF(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::TIFF,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_XBM(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::XBM,
					cfg);
			}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_RASTER_XPM(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::XPM,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_ROTATION_PARAMS_CSV_COMMA(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRotationParamsAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationParamsAnimationStrategy::COMMA,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_ROTATION_PARAMS_CSV_SEMICOLON(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRotationParamsAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationParamsAnimationStrategy::SEMICOLON,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_ROTATION_PARAMS_CSV_TAB(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			return GPlatesGui::ExportRotationParamsAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRotationParamsAnimationStrategy::TAB,
					cfg);
		}

		static
		GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
		create_DUMMY_EXPORTER(
				GPlatesGui::ExportAnimationContext& export_context,			
				const GPlatesGui::ExportAnimationStrategy::Configuration& cfg)
		{		
			qWarning("dummy exporter is creating... will abort!!!!!!!");
			//unknown exporter ID, which means there is error somewhere in the stack
			//we cannot recover from this error, abort to force OS to record the call stack
			abort();
			//cheat compiler
			return GPlatesGui::ExportRasterAnimationStrategy::create(
					export_context,
					GPlatesGui::ExportRasterAnimationStrategy::XPM,
					cfg);
		}


		static bool dummy;
	};
}

#endif // GPLATES_UTILS_EXPORTANIMATIONSTRATEGYFACTORY_H



