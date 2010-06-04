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

#include "RawRasterUtils.h"

#include "utils/TypeTraits.h"

namespace
{
	using namespace GPlatesPropertyValues;

	template<class ImplType>
	class TemplatedRawRasterVisitor :
			public RawRasterVisitor,
			public ImplType
	{
	public:

		virtual
		void
		visit(
				UninitialisedRawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				Int8RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				UInt8RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				Int16RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				UInt16RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				Int32RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				UInt32RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				FloatRawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				DoubleRawRaster &raster)
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				Rgba8RawRaster &raster)
		{
			ImplType::do_visit(raster);
		}
	};

	class RawRasterSizeVisitorImpl
	{
	public:

		template<typename T>
		void
		do_visit(
				RawRasterInternals::WithData<T> &with_data)
		{
			d_raster_size = std::make_pair(with_data.width(), with_data.height());
		}

		template<typename T>
		void
		do_visit(
				RawRasterInternals::WithoutData<T> &without_data)
		{  }

		boost::optional<std::pair<unsigned int, unsigned int > >
		get_raster_size() const
		{
			return d_raster_size;
		}

	private:

		boost::optional<std::pair<unsigned int, unsigned int> > d_raster_size;
	};

	/**
	 * This is a visitor that pulls out the size from a RawRaster.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterSizeVisitorImpl> RawRasterSizeVisitor;

	class RawRasterStatisticsVisitorImpl
	{
	public:

		RawRasterStatisticsVisitorImpl() :
			d_raster_statistics(NULL)
		{  }

		void
		do_visit(
				RawRasterInternals::WithStatistics &with_statistics)
		{
			d_raster_statistics = &with_statistics.statistics();
		}

		void
		do_visit(
				RawRasterInternals::WithoutStatistics &without_statistics)
		{  }

		RasterStatistics *
		get_raster_statistics() const
		{
			return d_raster_statistics;
		}

	private:
		
		RasterStatistics *d_raster_statistics;
	};

	/**
	 * This is a visitor that pulls out statistics from a RawRaster.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterStatisticsVisitorImpl> RawRasterStatisticsVisitor;

	class RawRasterNoDataValueVisitorImpl
	{
	public:

		template<typename T>
		void
		do_visit(
				RawRasterInternals::WithNoDataValue<T> &with_no_data_value)
		{
			boost::optional<T> no_data_value = with_no_data_value.no_data_value();
			if (no_data_value)
			{
				d_no_data_value = static_cast<double>(*no_data_value);
			}
		}

		template<typename T>
		void
		do_visit(
				RawRasterInternals::NanNoDataValue<T> &nan_no_data_value)
		{
			d_no_data_value = GPlatesMaths::Real::nan().dval();
		}

		template<typename T>
		void
		do_visit(
				RawRasterInternals::WithoutNoDataValue<T> &without_no_data_value)
		{  }

		boost::optional<double>
		get_no_data_value() const
		{
			return d_no_data_value;
		}

	private:

		boost::optional<double> d_no_data_value;
	};

	/**
	 * This is a visitor that pulls out the no-data value from a RawRaster.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterNoDataValueVisitorImpl> RawRasterNoDataValueVisitor;
}


boost::optional<std::pair<unsigned int, unsigned int> >
GPlatesPropertyValues::RawRasterUtils::get_raster_size(
		RawRaster &raster)
{
	RawRasterSizeVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.get_raster_size();
}


GPlatesPropertyValues::RasterStatistics *
GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(
		RawRaster &raster)
{
	RawRasterStatisticsVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.get_raster_statistics();
}


boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type>
GPlatesPropertyValues::RawRasterUtils::try_rgba8_raster_cast(
		RawRaster &raster)
{
	return try_raster_cast<Rgba8RawRaster>(raster);
}


boost::optional<double>
GPlatesPropertyValues::RawRasterUtils::get_no_data_value(
		RawRaster &raster)
{
	RawRasterNoDataValueVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.get_no_data_value();
}

