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
 
#ifndef GPLATES_GUI_EXPORTRASTERSTRATEGY_H
#define GPLATES_GUI_EXPORTRASTERSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "utils/ExportTemplateFilenameSequence.h"

#include "gui/ExportAnimationStrategy.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for 
	 * writing reconstructed feature geometries at each timestep.
	 * 
	 * ExportReconstructedGeometryAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportRasterAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
	//http://doc.qt.nokia.com/4.6/qimage.html#reading-and-writing-image-files
	// 		Format	Description								Qt's support 
	// 		BMP		Windows Bitmap							Read/write 
	// 		GIF		Graphic Interchange Format (optional)	Read 
	// 		JPG		Joint Photographic Experts Group		Read/write 
	// 		JPEG	Joint Photographic Experts Group		Read/write 
	// 		PNG		Portable Network Graphics				Read/write 
	// 		PBM		Portable Bitmap							Read 
	// 		PGM		Portable Graymap						Read 
	// 		PPM		Portable Pixmap							Read/write 
	// 		TIFF	Tagged Image File Format				Read/write 
	// 		XBM		X11 Bitmap								Read/write 
	// 		XPM		X11 Pixmap								Read/write 

		enum ImageType
		{
			BMP,
			JPG,
			JPEG,
			PNG,
			PPM,
			TIFF,
			XBM,
			XPM,
			INVALID_TPYE=999
		};

 		static const QString DEFAULT_RASTER_BMP_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_JPG_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_JPEG_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_PNG_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_PPM_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_TIFF_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_XBM_FILENAME_TEMPLATE;
		static const QString DEFAULT_RASTER_XPM_FILENAME_TEMPLATE;
		static const QString RASTER_FILENAME_TEMPLATE_DESC;
		static const QString RASTER_DESC;
		
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportReconstructedGeometryAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportRasterAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
		
		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				ImageType,
				const ExportAnimationStrategy::Configuration &cfg=
					ExportAnimationStrategy::Configuration(
							DEFAULT_RASTER_BMP_FILENAME_TEMPLATE));

		virtual
		~ExportRasterAnimationStrategy()
		{  }

		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);

		virtual
		const QString&
		get_default_filename_template();

		virtual
		const QString&
		get_filename_template_desc();

		virtual
		const QString&
		get_description()
		{
			return RASTER_DESC;
		}

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportRasterAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		ExportRasterAnimationStrategy();
		ImageType d_type;
		
		
	};
}


#endif //GPLATES_GUI_EXPORTRASTERSTRATEGY_H


