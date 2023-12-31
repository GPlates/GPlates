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
 
#ifndef GPLATES_GUI_EXPORTIMAGEANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTIMAGEANIMATIONSTRATEGY_H

#include <boost/optional.hpp>

#include <QImage>
#include <QSize>
#include <QString>

#include "ExportAnimationStrategy.h"
#include "ExportOptionsUtils.h"

#include "qt-widgets/GlobeAndMapWidget.h"

#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for saving the image
	 * (of the globe or map view) to a coloured image file at each timestep.
	 * 
	 * This class serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportImageAnimationStrategy:
			public QObject,
			public GPlatesGui::ExportAnimationStrategy
	{
		Q_OBJECT

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportImageAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportImageAnimationStrategy> non_null_ptr_type;
		

		/**
		 * Configuration options.
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
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
				XPM
			};


			Configuration(
					const QString& filename_template_,
					ImageType image_type_,
					const ExportOptionsUtils::ExportImageResolutionOptions &image_resolution_options_) :
				ConfigurationBase(filename_template_),
				image_type(image_type_),
				image_resolution_options(image_resolution_options_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			ImageType image_type;
			ExportOptionsUtils::ExportImageResolutionOptions image_resolution_options;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;


		static
		const non_null_ptr_type
		create(
				ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration)
		{
			return non_null_ptr_type(
					new ExportImageAnimationStrategy(
							export_animation_context,
							export_configuration));
		}

		virtual
		~ExportImageAnimationStrategy()
		{  }

		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		ExportImageAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration);

	private:
		//! Export configuration parameters.
		const_configuration_ptr d_configuration;

	};
}

#endif //GPLATES_GUI_EXPORTIMAGEANIMATIONSTRATEGY_H
