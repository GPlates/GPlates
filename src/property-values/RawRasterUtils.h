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

#include <utility>
#include <boost/optional.hpp>

#include "RawRaster.h"

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
		 * Returns the no-data value for @a raster, if available.
		 */
		boost::optional<double>
		get_no_data_value(
				RawRaster &raster);


		/**
		 * An enumeration of raw raster types.
		 */
		enum RawRasterType
		{
			UNINITIALISED,
			INT8,
			UINT8,
			INT16,
			UINT16,
			INT32,
			UINT32,
			FLOAT,
			DOUBLE,
			RGBA8,
			UNKNOWN
		};


		/**
		 * This visitor allows you to query information about a RawRaster of unknown type.
		 */
		class RawRasterTypeInfoVisitor :
				public RawRasterVisitor
		{
		public:

			RawRasterTypeInfoVisitor() :
				d_type(UNKNOWN)
			{  }

			virtual
			void
			visit(
					UninitialisedRawRaster &raster)
			{
				d_type = UNINITIALISED;
			}

			virtual
			void
			visit(
					Int8RawRaster &raster)
			{
				d_type = INT8;
			}

			virtual
			void
			visit(
					UInt8RawRaster &raster)
			{
				d_type = UINT8;
			}

			virtual
			void
			visit(
					Int16RawRaster &raster)
			{
				d_type = INT16;
			}

			virtual
			void
			visit(
					UInt16RawRaster &raster)
			{
				d_type = UINT16;
			}

			virtual
			void
			visit(
					Int32RawRaster &raster)
			{
				d_type = INT32;
			}

			virtual
			void
			visit(
					UInt32RawRaster &raster)
			{
				d_type = UINT32;
			}

			virtual
			void
			visit(
					FloatRawRaster &raster)
			{
				d_type = FLOAT;
			}

			virtual
			void
			visit(
					DoubleRawRaster &raster)
			{
				d_type = DOUBLE;
			}

			virtual
			void
			visit(
					Rgba8RawRaster &raster)
			{
				d_type = RGBA8;
			}

			RawRasterType
			get_type() const
			{
				return d_type;
			}

			bool
			is_signed_integer() const
			{
				return d_type == INT8 ||
					d_type == INT16 ||
					d_type == INT32;
			}

			bool
			is_unsigned_integer() const
			{
				return d_type == UINT8 ||
					d_type == UINT16 ||
					d_type == UINT32;
			}

			bool
			is_integer() const
			{
				return is_signed_integer() || is_unsigned_integer();
			}

			bool
			is_floating_point() const
			{
				return d_type == FLOAT ||
					d_type == DOUBLE;
			}

		private:

			RawRasterType d_type;
		};
	}
}

#endif  // GPLATES_PROPERTYVALUES_RAWRASTERUTILS_H
