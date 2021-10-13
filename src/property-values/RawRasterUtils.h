/* $Id$ */

/**
 * \file 
 * Contains utility functions and classes related to RawRaster.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_PROPERTYVALUES_RAWRASTERUTILS_H
#define GPLATES_PROPERTYVALUES_RAWRASTERUTILS_H

#include <algorithm>
#include <functional>
#include <utility>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "RasterType.h"
#include "RawRaster.h"

#include "maths/Real.h"


namespace GPlatesPropertyValues
{
	namespace RawRasterUtils
	{
		namespace RawRasterUtilsInternals
		{
			/**
			 * A visitor that returns a pointer to the raster if its derived type is
			 * @a TargetRawRasterType.
			 */
			template<class TargetRawRasterType>
			class RawRasterCastVisitor :
					public RawRasterVisitor
			{
			public:

				using RawRasterVisitor::visit;

				typedef typename TargetRawRasterType::non_null_ptr_type raster_ptr_type;

				virtual
				void
				visit(
						TargetRawRasterType &raster)
				{
					d_raster_ptr = raster_ptr_type(&raster);
				}

				boost::optional<raster_ptr_type>
				raster_ptr() const
				{
					return d_raster_ptr;
				}

			private:

				boost::optional<raster_ptr_type> d_raster_ptr;
			};


			// A dummy function for use by get_is_no_data_value_function.
			template<typename T>
			bool
			always_false(
					T value)
			{
				return false;
			}


			/**
			 * Helper class to support @a convert_integer_raster_to_float_raster.
			 */
			template<typename IntType, typename FloatType>
			class IntegerToFloat :
					public std::unary_function<IntType, FloatType>
			{
			public:

				IntegerToFloat(
						boost::function<bool (IntType)> is_no_data_value) :
					d_is_no_data_value(is_no_data_value)
				{
				}

				FloatType
				operator()(
						IntType value)
				{
					static const FloatType NAN_VALUE = GPlatesMaths::quiet_nan<FloatType>();

					if (d_is_no_data_value(value))
					{
						return NAN_VALUE;
					}
					else
					{
						return static_cast<FloatType>(value);
					}
				}

			private:

				boost::function<bool (IntType)> d_is_no_data_value;
			};
		}


		/**
		 * Gets the size (width and height) of the @a raster.
		 *
		 * Returns boost::none if the raster does not have a width and height.
		 */
		boost::optional<std::pair<unsigned int, unsigned int> >
		get_raster_size(
				RawRaster &raster);


		/**
		 * Gets a pointer to the RasterStatistics instance inside @a raster.
		 *
		 * Returns NULL if the raster does not have statistics.
		 */
		RasterStatistics *
		get_raster_statistics(
				RawRaster &raster);


		/**
		 * Returns a pointer to a @a TargetRawRasterType if the @a raster is indeed
		 * of that derived type.
		 */
		template<class TargetRawRasterType>
		boost::optional<typename TargetRawRasterType::non_null_ptr_type>
		try_raster_cast(
				RawRaster &raster)
		{
			RawRasterUtilsInternals::RawRasterCastVisitor<TargetRawRasterType> visitor;
			raster.accept_visitor(visitor);

			return visitor.raster_ptr();
		}


		/**
		 * Returns a pointer to a Rgba8RawRaster if @a raster is indeed a Rgba8RawRaster.
		 */
		boost::optional<Rgba8RawRaster::non_null_ptr_type>
		try_rgba8_raster_cast(
				RawRaster &raster);


		/**
		 * Returns a pointer to a ProxiedRgba8RawRaster if @a raster is indeed
		 * a ProxiedRgba8RawRaster.
		 */
		boost::optional<ProxiedRgba8RawRaster::non_null_ptr_type>
		try_proxied_rgba8_raster_cast(
				RawRaster &raster);


		/**
		 * Returns the no-data value for @a raster, if available.
		 */
		boost::optional<double>
		get_no_data_value(
				RawRaster &raster);


		/**
		 * Returns a function that takes one argument and returns a boolean value
		 * indicating whether that argument is the no-data value of @a raster.
		 *
		 * This overload is called if @a RawRasterType has a no-data value.
		 */
		template<class RawRasterType>
		boost::function<bool (typename RawRasterType::element_type)>
		get_is_no_data_value_function(
				const RawRasterType &raster,
				typename boost::enable_if_c<RawRasterType::has_no_data_value>::type *dummy = NULL)
		{
			return boost::bind(&RawRasterType::is_no_data_value, boost::cref(raster), _1);
		}


		// This overload is called if @a RawRasterType does not have a no-data value.
		template<class RawRasterType>
		boost::function<bool (typename RawRasterType::element_type)>
		get_is_no_data_value_function(
				const RawRasterType &raster,
				typename boost::enable_if_c<!RawRasterType::has_no_data_value>::type *dummy = NULL)
		{
			return &RawRasterUtilsInternals::always_false<typename RawRasterType::element_type>;
		}


		/**
		 * Returns whether the @a raster has data.
		 *
		 * Note this returns false if the @a raster contains proxied data.
		 */
		bool
		has_data(
				RawRaster &raster);


		/**
		 * Returns whether the @a raster has proxied data.
		 */
		bool
		has_proxied_data(
				RawRaster &raster);


		/**
		 * Returns the data type of the raster as an enumerated value.
		 */
		RasterType::Type
		get_raster_type(
				RawRaster &raster);


		/**
		 * Converts the integer @a source_raster into a floating-point raw raster.
		 *
		 * The @a element_type of @a FromRawRasterType must be an integral type.
		 * The @a element_type of @a ToRawRasterType must be a floating-point type
		 * (i.e. either float or double).
		 *
		 * If @a source_raster has a no-data value, these values are converted to NaN.
		 */
		template<class FromRawRasterType, class ToRawRasterType>
		typename ToRawRasterType::non_null_ptr_type
		convert_integer_raster_to_float_raster(
				const FromRawRasterType &source_raster)
		{
			typename ToRawRasterType::non_null_ptr_type result = ToRawRasterType::create(
					source_raster.width(), source_raster.height());
			
			typedef RawRasterUtilsInternals::IntegerToFloat<
				typename FromRawRasterType::element_type,
				typename ToRawRasterType::element_type
			> unary_operator;
			unary_operator op(get_is_no_data_value_function(source_raster));

			std::transform(
					source_raster.data(),
					source_raster.data() + source_raster.width() * source_raster.height(),
					result->data(),
					op);

			return result;
		}


		/**
		 * Applies a coverage raster to an RGBA raster, in place.
		 *
		 * The @a source_raster and the @a coverage_raster must be of the same
		 * dimensions. For each pixel in the @a source_raster, the alpha channel
		 * is multiplied by the value of the corresponding pixel in the
		 * @a coverage_raster.
		 */
		void
		apply_coverage_raster(
				const Rgba8RawRaster::non_null_ptr_type &source_raster,
				const CoverageRawRaster::non_null_ptr_type &coverage_raster);


		/**
		 * Writes @a raster out to @a filename.
		 *
		 * Returns whether the write was successful.
		 */
		bool
		write_rgba8_raster(
				const Rgba8RawRaster::non_null_ptr_type &raster,
				const QString &filename);


		/**
		 * Returns true if @a raster has any pixels with an alpha value of 255.
		 */
		bool
		has_fully_transparent_pixels(
				const Rgba8RawRaster::non_null_ptr_type &raster);

	}
}

#endif  // GPLATES_PROPERTYVALUES_RAWRASTERUTILS_H
