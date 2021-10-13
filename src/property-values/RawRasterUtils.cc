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

#include "global/CompilerWarnings.h"

PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING( 4251 )
#include <Magick++.h>
POP_MSVC_WARNINGS

#include "RawRasterUtils.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "utils/Profile.h"
#include "utils/TypeTraits.h"


namespace
{
	using namespace GPlatesPropertyValues;

	class RawRasterSizeVisitorImpl
	{
	public:

		const boost::optional<std::pair<unsigned int, unsigned int> > &
		get_raster_size() const
		{
			return d_raster_size;
		}

	private:

		template<class RawRasterType, bool has_size>
		class ExtractRasterSize
		{
		public:

			static
			void
			extract_raster_size(
					RawRasterType &raster,
					boost::optional<std::pair<unsigned int, unsigned int> > &raster_size)
			{
				// Nothing to do; has_size = false case handled here.
			}
		};

		template<class RawRasterType>
		class ExtractRasterSize<RawRasterType, true>
		{
		public:

			static
			void
			extract_raster_size(
					RawRasterType &raster,
					boost::optional<std::pair<unsigned int, unsigned int> > &raster_size)
			{
				raster_size = std::make_pair(raster.width(), raster.height());
			}
		};

		template<class RawRasterType>
		void
		do_visit(
				RawRasterType &raster)
		{
			ExtractRasterSize<RawRasterType, RawRasterType::has_data || RawRasterType::has_proxied_data>::
				extract_raster_size(raster, d_raster_size);
		}

		friend class TemplatedRawRasterVisitor<RawRasterSizeVisitorImpl>;

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

		RasterStatistics *
		get_raster_statistics() const
		{
			return d_raster_statistics;
		}

	private:

		void
		do_visit(
				RawRasterStatisticsPolicies::WithStatistics &with_statistics)
		{
			d_raster_statistics = &with_statistics.statistics();
		}

		void
		do_visit(
				RawRasterStatisticsPolicies::WithoutStatistics &without_statistics)
		{  }

		friend class TemplatedRawRasterVisitor<RawRasterStatisticsVisitorImpl>;

		RasterStatistics *d_raster_statistics;
	};

	/**
	 * This is a visitor that pulls out statistics from a RawRaster.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterStatisticsVisitorImpl> RawRasterStatisticsVisitor;

	class RawRasterNoDataValueVisitorImpl
	{
	public:

		boost::optional<double>
		get_no_data_value() const
		{
			return d_no_data_value;
		}

	private:

		template<typename T>
		void
		do_visit(
				RawRasterNoDataValuePolicies::WithNoDataValue<T> &with_no_data_value)
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
				RawRasterNoDataValuePolicies::NanNoDataValue<T> &nan_no_data_value)
		{
			d_no_data_value = GPlatesMaths::quiet_nan<T>();
		}

		template<typename T>
		void
		do_visit(
				RawRasterNoDataValuePolicies::WithoutNoDataValue<T> &without_no_data_value)
		{  }

		friend class TemplatedRawRasterVisitor<RawRasterNoDataValueVisitorImpl>;

		boost::optional<double> d_no_data_value;
	};

	/**
	 * This is a visitor that pulls out the no-data value from a RawRaster.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterNoDataValueVisitorImpl> RawRasterNoDataValueVisitor;

	class RawRasterHasDataVisitorImpl
	{
	public:

		RawRasterHasDataVisitorImpl() :
			d_has_data(false),
			d_has_proxied_data(false)
		{
		}

		bool
		has_data() const
		{
			return d_has_data;
		}

		bool
		has_proxied_data() const
		{
			return d_has_proxied_data;
		}

	private:

		template<class RawRasterType>
		void
		do_visit(
				RawRasterType &)
		{
			d_has_data = RawRasterType::has_data;
			d_has_proxied_data = RawRasterType::has_proxied_data;
		}

		friend class TemplatedRawRasterVisitor<RawRasterHasDataVisitorImpl>;

		bool d_has_data;
		bool d_has_proxied_data;
	};

	/**
	 * This is a visitor that determines whether a raster has data / proxied data.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterHasDataVisitorImpl> RawRasterHasDataVisitor;

	class RawRasterTypeInfoVisitorImpl
	{
	public:

		RawRasterTypeInfoVisitorImpl() :
			d_type(RasterType::UNKNOWN)
		{  }

		RasterType::Type
		get_type() const
		{
			return d_type;
		}

	private:

		template<class RawRasterType>
		void
		do_visit(
				RawRasterType &raster)
		{
			d_type = RasterType::get_type_as_enum<typename RawRasterType::element_type>();
		}

		friend class TemplatedRawRasterVisitor<RawRasterTypeInfoVisitorImpl>;

		RasterType::Type d_type;
	};

	/**
	 * This is a visitor that extracts the data type of a raster as an enumerated value.
	 */
	typedef TemplatedRawRasterVisitor<RawRasterTypeInfoVisitorImpl> RawRasterTypeInfoVisitor;
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


boost::optional<GPlatesPropertyValues::ProxiedRgba8RawRaster::non_null_ptr_type>
GPlatesPropertyValues::RawRasterUtils::try_proxied_rgba8_raster_cast(
		RawRaster &raster)
{
	return try_raster_cast<ProxiedRgba8RawRaster>(raster);
}


boost::optional<double>
GPlatesPropertyValues::RawRasterUtils::get_no_data_value(
		RawRaster &raster)
{
	RawRasterNoDataValueVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.get_no_data_value();
}


bool
GPlatesPropertyValues::RawRasterUtils::has_data(
		RawRaster &raster)
{
	RawRasterHasDataVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.has_data();
}


bool
GPlatesPropertyValues::RawRasterUtils::has_proxied_data(
		RawRaster &raster)
{
	RawRasterHasDataVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.has_proxied_data();
}


GPlatesPropertyValues::RasterType::Type
GPlatesPropertyValues::RawRasterUtils::get_raster_type(
		RawRaster &raster)
{
	RawRasterTypeInfoVisitor visitor;
	raster.accept_visitor(visitor);

	return visitor.get_type();
}


void
GPlatesPropertyValues::RawRasterUtils::apply_coverage_raster(
		const Rgba8RawRaster::non_null_ptr_type &source_raster,
		const CoverageRawRaster::non_null_ptr_type &coverage_raster)
{
	//PROFILE_FUNC();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			source_raster->width() == coverage_raster->width() &&
			source_raster->height() == coverage_raster->height(),
			GPLATES_ASSERTION_SOURCE);

	Rgba8RawRaster::element_type *source_data = source_raster->data();
	Rgba8RawRaster::element_type * const source_data_end = source_data + source_raster->width() * source_raster->height();
	CoverageRawRaster::element_type *coverage_data = coverage_raster->data();

	while (source_data != source_data_end)
	{
		CoverageRawRaster::element_type coverage_value = *coverage_data;
		if (coverage_value < 0.0)
		{
			coverage_value = 0.0;
		}
		if (coverage_value > 1.0)
		{
			coverage_value = 1.0;
		}
		source_data->alpha = static_cast<boost::uint8_t>(source_data->alpha * coverage_value);

		++source_data;
		++coverage_data;
	}
}


bool
GPlatesPropertyValues::RawRasterUtils::write_rgba8_raster(
		const Rgba8RawRaster::non_null_ptr_type &raster,
		const QString &filename)
{
	try
	{
		Magick::Image image(raster->width(), raster->height(), "RGBA", Magick::CharPixel, raster->data());
		image.write(filename.toStdString());
		return true;
	}
	catch (std::exception &exc)
	{
		qWarning() << "GPlatesPropertyValues::RawRasterUtils::write_rgba8_raster: " << exc.what();
		return false;
	}
	catch (...)
	{
		qWarning() << "GPlatesPropertyValues::RawRasterUtils::write_rgba8_raster: unknown error";
		return false;
	}
}


bool
GPlatesPropertyValues::RawRasterUtils::has_fully_transparent_pixels(
		const Rgba8RawRaster::non_null_ptr_type &raster)
{
	GPlatesGui::rgba8_t *data = raster->data();
	GPlatesGui::rgba8_t *data_end = data + raster->width() * raster->height();

	while (data != data_end)
	{
		if (data->alpha == 0)
		{
			return true;
		}
		++data;
	}

	return false;
}

