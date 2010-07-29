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
 
#ifndef GPLATES_GUI_MIPMAPPER_H
#define GPLATES_GUI_MIPMAPPER_H

#include <cstring> // for memcpy
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <QImage>
#include <QGLWidget>
#include <QDebug>
#include <QColor>
#include <Magick++.h>

#include "Colour.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "property-values/RawRaster.h"


namespace GPlatesGui
{
	namespace MipmapperInternals
	{
		/**
		 * Extends @a source_raster to the right and down by one pixel if its width
		 * and height are not multiples of two, respectively.
		 *
		 * When the raster is extended in a particular direction, this means that the
		 * row or column at the edge of the @a source_raster is copied to the new row
		 * or column. New corner points take on the value of the corresponding corner
		 * point in the @a source_raster.
		 *
		 * e.g. a 5x6 raster is extended to become a 6x6 raster by copying the last
		 * row of pixels into the new, sixth row.
		 *
		 * Returns a pointer to @a source_raster if its dimensions are already even.
		 */
		template<class RawRasterType>
		typename RawRasterType::non_null_ptr_to_const_type
		extend_raster(
				const RawRasterType &source_raster,
				// Can only be called if this type of raster has data.
				typename boost::enable_if_c<RawRasterType::has_data>::type *dummy = 0);

		/**
		 * Converts an @a array of pixels in RGBA8 format into ARGB8 format, in place.
		 */
		void
		convert_rgba_to_argb(
				rgba8_t *array,
				unsigned int num_pixels);
	}


	/**
	 * Mipmapper takes a raster of type RawRasterType and produces a sequence of
	 * mipmaps of successively smaller size.
	 */
	template<class RawRasterType, class Enable = void>
	class Mipmapper;
		// This is intentionally not defined.


	template<class RawRasterType>
	class Mipmapper<
		RawRasterType,
		// This version is for RGBA rasters.
		typename boost::enable_if<
			boost::is_same<typename RawRasterType::element_type, GPlatesGui::rgba8_t>
		>::type>
	{
	public:

		/**
		 * The type of the raster that is to be mipmapped.
		 */
		typedef RawRasterType source_raster_type;

		/**
		 * The type of the raster that is produced as a result of the mipmapping process.
		 */
		typedef RawRasterType output_raster_type;

		/**
		 * Constructs a Mipmapper that will progressively create mipmaps from
		 * @a source_raster until the largest dimension in the mipmap is less than or
		 * equal to the @a threshold_size.
		 *
		 * Note that you must call @a generate_next before retrieving the first mipmap
		 * using @a get_current_mipmap.
		 */
		Mipmapper(
				const typename source_raster_type::non_null_ptr_to_const_type &source_raster,
				unsigned int threshold_size = 1) :
			d_current_mipmap(source_raster),
			d_threshold_size(threshold_size)
		{
		}

		/**
		 * Generates the next mipmap in the sequence of mipmaps.
		 *
		 * Returns true if a mipmap was successfully created. Returns false if no new
		 * mipmap was generated; this occurs when the current mipmap's largest
		 * dimension is less than or equal to the @a threshold_size specified in the
		 * constructor.
		 */
		bool
		generate_next()
		{
			// Check whether the current mipmap is at the threshold size.
			if (d_current_mipmap->width() <= d_threshold_size &&
					d_current_mipmap->height() <= d_threshold_size)
			{
				// Early exit - no more levels to generate.
				return false;
			}

			// Make sure the dimensions are even. After this call, the dimensions of
			// d_current_mipmap may very well have changed.
			d_current_mipmap = MipmapperInternals::extend_raster(*d_current_mipmap);

			// Make a copy of the pixels and convert them into ARGB (because QImage can't
			// take RGBA).
			unsigned int num_pixels = d_current_mipmap->width() * d_current_mipmap->height();
			boost::scoped_array<rgba8_t> argb(new rgba8_t[num_pixels]);
			std::memcpy(argb.get(), d_current_mipmap->data(), num_pixels * sizeof(rgba8_t));
			MipmapperInternals::convert_rgba_to_argb(argb.get(), num_pixels);

			// Construct a QImage and get it to scale the image down.
			// Note that the QImage does not take ownership of the existing buffer argb.
			boost::scoped_ptr<QImage> argb_qimage(new QImage(
					d_current_mipmap->width(),
					d_current_mipmap->height(),
					QImage::Format_ARGB32));
#if 0
			for (int y = 0; y < argb_qimage->height(); ++y)
			{
				for (int x = 0; x < argb_qimage->width(); ++x)
				{
					rgba8_t &r = argb[y * argb_qimage->width() + x];
					qDebug() << r.red << r.green << r.blue << r.alpha;
					argb_qimage->setPixel(x, y, r.uint32_value);
					QColor c(argb_qimage->pixel(x, y));
					qDebug() << c.red() << c.green() << c.blue() << c.alpha();
				}
			}
#endif
			unsigned int new_width = d_current_mipmap->width() / 2;
			unsigned int new_height = d_current_mipmap->height() / 2;
			QImage scaled_argb_qimage = argb_qimage->scaled(
					QSize(new_width, new_height),
					Qt::IgnoreAspectRatio,
					Qt::SmoothTransformation /* bilinear filtering */);

			// We can now free the memory taken by argb_image and argb.
			argb_qimage.reset(NULL);
			argb.reset(NULL);

			// Convert scaled_argb_qimage back into RGBA and save as d_current_mipmap.
			// Note that we have to do the deep copy ourselves, because the deep copy
			// returned from the const version of bits() appears to be not allocated by
			// new[]; it would then crash when scoped_array (inside RawRaster) tried to
			// deallocate it.
			QImage scaled_rgba_qimage = QGLWidget::convertToGLFormat(scaled_argb_qimage);
			const QImage &const_scaled_rgba_qimage = scaled_rgba_qimage;
			const rgba8_t *const_rgba_bits = reinterpret_cast<const rgba8_t *>(const_scaled_rgba_qimage.bits() /* no deep copy */);
			rgba8_t *rgba_bits = new rgba8_t[new_width * new_height];
			std::memcpy(rgba_bits, const_rgba_bits, new_width * new_height * sizeof(rgba8_t));

			d_current_mipmap = output_raster_type::create(new_width, new_height, rgba_bits);

			return true;
		}

		/**
		 * Returns the current mipmap held by this Mipmapper.
		 */
		typename output_raster_type::non_null_ptr_to_const_type
		get_current_mipmap() const
		{
			return d_current_mipmap;
		}

	private:

		typename output_raster_type::non_null_ptr_to_const_type d_current_mipmap;

		unsigned int d_threshold_size;
	};


	template<class RawRasterType>
	typename RawRasterType::non_null_ptr_to_const_type
	MipmapperInternals::extend_raster(
			const RawRasterType &source_raster,
			typename boost::enable_if_c<RawRasterType::has_data>::type *dummy)
	{
		unsigned int source_width = source_raster.width();
		unsigned int source_height = source_raster.height();
		bool extend_right = (source_width % 2);
		bool extend_down = (source_height % 2);
		
		// Early exit if no work to do...
		if (!extend_right && !extend_down)
		{
			return typename RawRasterType::non_null_ptr_to_const_type(&source_raster);
		}

		unsigned int dest_width = (extend_right ? source_width + 1 : source_width);
		unsigned int dest_height = (extend_down ? source_height + 1 : source_height);

		// Doesn't hurt to check for overflow...
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				dest_width >= source_width && dest_height >= source_height,
				GPLATES_ASSERTION_SOURCE);

		// Acquire a pointer to the source buffer.
		typedef typename RawRasterType::element_type element_type;
		const element_type * const source_buf = source_raster.data();

		// Allocate the destination buffer and get a pointer to it.
		typename RawRasterType::non_null_ptr_type dest_raster =
			RawRasterType::create(dest_width, dest_height);
		element_type * const dest_buf = dest_raster->data();

		// Copy the source buffer to the dest buffer.
		const element_type *source_ptr = source_buf;
		const element_type * const source_end_ptr = (source_buf + source_width * source_height);
		element_type *dest_ptr = dest_buf;
		element_type * const dest_end_ptr = (dest_buf + dest_width * dest_height);
		while (source_ptr != source_end_ptr)
		{
			std::memcpy(dest_ptr, source_ptr, source_width * sizeof(element_type));
			source_ptr += source_width;
			dest_ptr += dest_width; // using dest_width ensures correct offset in new buffer
		}

		// Copy the last column over if extending to the right
		if (extend_right)
		{
			dest_ptr = dest_buf;
			while (dest_ptr != dest_end_ptr)
			{
				// Note that if we get to here, dest_width >= 2.
				dest_ptr += dest_width;
				memcpy(dest_ptr - 1, dest_ptr - 2, sizeof(element_type)); // using memcpy to be consistent
			}
		}

		// Copy the last source row down if extending down.
		// Note that this also copies the new corner, if extending both right and down.
		if (extend_down)
		{
			// Note that if we get to here, dest_height >= 2.
			element_type *second_last_row_ptr = (dest_buf + dest_width * (dest_height - 2));
			element_type *last_row_ptr = (dest_buf + dest_width * (dest_height - 1));
			memcpy(last_row_ptr, second_last_row_ptr, dest_width * sizeof(element_type));
		}
		
		return dest_raster;
	}


	void
	MipmapperInternals::convert_rgba_to_argb(
			rgba8_t *array,
			unsigned int num_pixels)
	{
		rgba8_t *end = array + num_pixels;

		while (array != end)
		{
			// rgba8_t temp(array->alpha, array->red, array->green, array->blue);
			rgba8_t temp(array->blue, array->green, array->red, array->alpha);
			*array = temp;

			++array;
		}
	}
}

#endif	// GPLATES_GUI_MIPMAPPER_H
