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
#include <QLocale> 
#include <QThread>

#include "ExportRasterAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruct.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"

#include "maths/MathsUtils.h"

#include "model/ReconstructionTreeEdge.h"

#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/GlobeCanvas.h"

#include "utils/FloatingPointComparisons.h"

#include "presentation/ViewState.h"

 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_BMP_FILENAME_TEMPLATE
		="raster_%0.2f.bmp";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_JPG_FILENAME_TEMPLATE
		 ="raster_%0.2f.jpg";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_JPEG_FILENAME_TEMPLATE
		 ="raster_%0.2f.jpeg";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_PNG_FILENAME_TEMPLATE
	 ="raster_%0.2f.png";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_PPM_FILENAME_TEMPLATE
		 ="raster_%0.2f.ppm";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_TIFF_FILENAME_TEMPLATE
		 ="raster_%0.2f.tiff";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_XBM_FILENAME_TEMPLATE
		 ="raster_%0.2f.xbm";
 const QString GPlatesGui::ExportRasterAnimationStrategy::DEFAULT_RASTER_XPM_FILENAME_TEMPLATE
		="raster_%0.2f.xpm";
 const QString GPlatesGui::ExportRasterAnimationStrategy::RASTER_DESC
 		="Export raster data as image.";
 const QString GPlatesGui::ExportRasterAnimationStrategy::RASTER_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;
 

const GPlatesGui::ExportRasterAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportRasterAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		ImageType type,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportRasterAnimationStrategy* ptr=
		new ExportRasterAnimationStrategy(
				export_animation_context,
				cfg.filename_template());
	
	ptr->d_type=type;

		return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportRasterAnimationStrategy::ExportRasterAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
}

bool
GPlatesGui::ExportRasterAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Figure out a filename from the template filename sequence.
	QString basename = *filename_it++;
	// Add the target dir to that to figure out the absolute path + name.
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	d_export_animation_context_ptr->viewport_window().reconstruction_view_widget().
			globe_and_map_widget().get_globe_canvas().repaint_canvas();

	QCoreApplication::processEvents();
	QImage img=
	d_export_animation_context_ptr->viewport_window().reconstruction_view_widget().
		globe_and_map_widget().grab_frame_buffer();
	img.save(full_filename);

	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}

const QString&
GPlatesGui::ExportRasterAnimationStrategy::get_default_filename_template()
{
	switch(d_type)
	{
		case BMP:
			return DEFAULT_RASTER_BMP_FILENAME_TEMPLATE;
			break;
		case JPG:
			return DEFAULT_RASTER_JPG_FILENAME_TEMPLATE;
			break;
		case JPEG:
			return DEFAULT_RASTER_JPEG_FILENAME_TEMPLATE;
			break;
		case PNG:
			return DEFAULT_RASTER_PNG_FILENAME_TEMPLATE;
			break;
		case PPM:
			return DEFAULT_RASTER_PPM_FILENAME_TEMPLATE;
			break;
		case TIFF:
			return DEFAULT_RASTER_TIFF_FILENAME_TEMPLATE;
			break;
		case XBM:
			return DEFAULT_RASTER_XBM_FILENAME_TEMPLATE;
			break;
		case XPM:
			return DEFAULT_RASTER_XPM_FILENAME_TEMPLATE;
			break;
		default:
			return DEFAULT_RASTER_JPG_FILENAME_TEMPLATE;
			break;
	}
}

const QString&
GPlatesGui::ExportRasterAnimationStrategy::get_filename_template_desc()
{
	return RASTER_FILENAME_TEMPLATE_DESC;
}


