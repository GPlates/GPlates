/* $Id$ */

/**
 * \file 
 * Contains the definition of the RawRaster class.
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

#ifndef GPLATES_PROPERTYVALUES_RAWRASTER_H
#define GPLATES_PROPERTYVALUES_RAWRASTER_H

#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
#include <QString>

#include "RasterStatistics.h"

#include "file-io/RasterBandReaderHandle.h"

#include "gui/Colour.h"

#include "maths/Real.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	// Forward declarations.
	class ProxiedRasterResolver;
	class RawRasterVisitor;


	/**
	 * RawRaster is the abstract base class of classes that encapsulate a raster
	 * (a dynamically allocated array of some type) and associated information.
	 * RawRaster derivations store the raw information in a raster file before it
	 * is processed into textures for visualisation.
	 */
	class RawRaster :
			public GPlatesUtils::ReferenceCount<RawRaster>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RawRaster> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RawRaster> non_null_ptr_to_const_type;

		virtual
		~RawRaster();

		virtual
		void
		accept_visitor(
				RawRasterVisitor &visitor) = 0;
	};


	// Forward declaration.
	template
	<
		typename T,
		template <class> class DataPolicy,
		class StatisticsPolicy,
		template <class> class NoDataValuePolicy
	>
	class RawRasterImpl;

	
	namespace RawRasterDataPolicies
	{
		// Forward declarations.
		template <class> class WithData;
		template <class> class WithProxiedData;
		template <class> class WithoutData;
	}


	namespace RawRasterStatisticsPolicies
	{
		// Forward declarations.
		class WithStatistics;
		class WithoutStatistics;
	}


	namespace RawRasterNoDataValuePolicies
	{
		// Forward declarations.
		template <class> class WithNoDataValue;
		template <class> class NanNoDataValue;
		template <class> class WithoutNoDataValue;
	}


	// A note on types:
	//  - The exact-width integer types are said to be optional, but they should be
	//    present on all platforms that we're interested in. See the documentation
	//    for boost/cstdint.hpp for more information.
	//  - GDAL defines two float types, a 32-bit and a 64-bit float. Although there
	//    is no guarantee that 'float' is 32-bit and 'double' is 64-bit, that is
	//    the assumption that GDAL seems to make, and for the platforms that we are
	//    interested in, that is a fairly safe assumption.
	//  - With the FLOAT and DOUBLE types below, we don't specify the width; they
	//    mean whatever C++ says is 'float' and 'double' on a particular platform.
	//  - GDAL also has complex number types. These are not supported.

	typedef RawRasterImpl
	<
		void,
		RawRasterDataPolicies::WithoutData,
		RawRasterStatisticsPolicies::WithoutStatistics,
		RawRasterNoDataValuePolicies::WithoutNoDataValue
	> UninitialisedRawRaster;

	typedef RawRasterImpl
	<
		boost::int8_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> Int8RawRaster;

	typedef RawRasterImpl
	<
		boost::int8_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> ProxiedInt8RawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		boost::uint8_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> UInt8RawRaster;

	typedef RawRasterImpl
	<
		boost::uint8_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> ProxiedUInt8RawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		boost::int16_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> Int16RawRaster;

	typedef RawRasterImpl
	<
		boost::int16_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> ProxiedInt16RawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		boost::uint16_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> UInt16RawRaster;

	typedef RawRasterImpl
	<
		boost::uint16_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> ProxiedUInt16RawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		boost::int32_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> Int32RawRaster;

	typedef RawRasterImpl
	<
		boost::int32_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> ProxiedInt32RawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		boost::uint32_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> UInt32RawRaster;

	typedef RawRasterImpl
	<
		boost::uint32_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::WithNoDataValue
	> ProxiedUInt32RawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		float,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::NanNoDataValue
	> FloatRawRaster;

	typedef RawRasterImpl
	<
		float,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::NanNoDataValue
	> ProxiedFloatRawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		double,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::NanNoDataValue
	> DoubleRawRaster;

	typedef RawRasterImpl
	<
		double,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithStatistics,
		RawRasterNoDataValuePolicies::NanNoDataValue
	> ProxiedDoubleRawRaster; // Proxied version.

	typedef RawRasterImpl
	<
		GPlatesGui::rgba8_t,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithoutStatistics,
		RawRasterNoDataValuePolicies::WithoutNoDataValue
	> Rgba8RawRaster;

	typedef RawRasterImpl
	<
		GPlatesGui::rgba8_t,
		RawRasterDataPolicies::WithProxiedData,
		RawRasterStatisticsPolicies::WithoutStatistics,
		RawRasterNoDataValuePolicies::WithoutNoDataValue
	> ProxiedRgba8RawRaster; // Proxied version.

	/**
	 * A CoverageRawRaster is a raster that can be used to represent the "coverage"
	 * at each pixel of a downsampled raster. The "coverage" at a particular
	 * pixel is the proportion of pixels in the original raster corresponding to
	 * the pixel in the downsampled raster that are not the sentinel, or
	 * "no-data", value. This notion of a "coverage" has nothing to do with the
	 * GML coverage.
	 */
	typedef RawRasterImpl
	<
		float,
		RawRasterDataPolicies::WithData,
		RawRasterStatisticsPolicies::WithoutStatistics,
		RawRasterNoDataValuePolicies::WithoutNoDataValue
	> CoverageRawRaster;


	/**
	 * Contains policy classes for use with RawRasterImpl.
	 */
	namespace RawRasterStatisticsPolicies
	{
		/**
		 * Use WithStatistics if the RawRaster derivation stores statistics.
		 */
		class WithStatistics
		{
		public:

			enum
			{
				has_statistics = true
			};

			WithStatistics() :
				d_statistics()
			{  }

			// This is intentionally not explicit.
			WithStatistics(
					const RasterStatistics &statistics_) :
				d_statistics(statistics_)
			{  }

			RasterStatistics &
			statistics()
			{
				return d_statistics;
			}

			const RasterStatistics &
			statistics() const
			{
				return d_statistics;
			}

			void
			set_statistics(
					const RasterStatistics &statistics_)
			{
				d_statistics = statistics_;
			}

		protected:

			~WithStatistics()
			{  }

		private:

			RasterStatistics d_statistics;
		};

		/**
		 * Use WithoutStatistics if the RawRaster derivation does not store statistics.
		 */
		class WithoutStatistics
		{
		public:

			enum
			{
				has_statistics = false
			};

		protected:

			~WithoutStatistics()
			{  }
		};
	}


	/**
	 * Contains policy classes for use with RawRasterImpl.
	 */
	namespace RawRasterNoDataValuePolicies
	{
		/**
		 * Use WithNoDataValue if the RawRaster derivation stores a "no data" value.
		 */
		template<typename T>
		class WithNoDataValue
		{
		public:

			enum
			{
				has_no_data_value = true
			};

			WithNoDataValue() :
				d_no_data_value(boost::none)
			{  }

			// This is intentionally not explicit.
			WithNoDataValue(
					const boost::optional<T> &no_data_value_) :
				d_no_data_value(no_data_value_)
			{  }

			const boost::optional<T> &
			no_data_value() const
			{
				return d_no_data_value;
			}

			void
			set_no_data_value(
					const boost::optional<T> &no_data_value_)
			{
				d_no_data_value = no_data_value_;
			}

			bool
			is_no_data_value(
					T value) const
			{
				if (d_no_data_value)
				{
					return *d_no_data_value == value;
				}
				else
				{
					return false;
				}
			}

		protected:

			~WithNoDataValue()
			{  }

		private:

			boost::optional<T> d_no_data_value;
		};

		/**
		 * Use NanNoDataValue if the RawRaster derivation uses NaN as a fixed "no data" value.
		 */
		template<typename T>
		class NanNoDataValue
		{
		public:

			enum
			{
				has_no_data_value = true
			};

			const boost::optional<T> &
			no_data_value() const
			{
				static const boost::optional<T> result =
					static_cast<T>(GPlatesMaths::nan());
				return result;
			}

			bool
			is_no_data_value(
					T value) const
			{
				return GPlatesMaths::is_nan(value);
			}

		protected:

			~NanNoDataValue()
			{  }
		};

		/**
		 * Use WithoutNoDataValue if the RawRaster derivation does not have a "no data" value.
		 */
		template<typename T>
		class WithoutNoDataValue
		{
		public:

			enum
			{
				has_no_data_value = false
			};

		protected:

			~WithoutNoDataValue()
			{  }
		};
	}


	/**
	 * Contains policy classes for use with RawRasterImpl.
	 */
	namespace RawRasterDataPolicies
	{
		/**
		 * Use WithData if the RawRaster derivation stores a pointer to dynamically
		 * allocated memory.
		 */
		template<typename T>
		class WithData
		{
		public:

			enum
			{
				has_data = true
			};

			enum
			{
				has_proxied_data = false
			};

			WithData(
					unsigned int width_,
					unsigned int height_) :
				d_width(width_),
				d_height(height_),
				d_data(new T[width_ * height_])
			{  }

			WithData(
					unsigned int width_,
					unsigned int height_,
					T *data_) :
				d_width(width_),
				d_height(height_),
				d_data(data_)
			{  }

			unsigned int
			width() const
			{
				return d_width;
			}

			unsigned int
			height() const
			{
				return d_height;
			}

			T *
			data()
			{
				return d_data.get();
			}

			const T *
			data() const
			{
				return d_data.get();
			}

			T &
			at(
					unsigned int x_pixel,
					unsigned int y_line)
			{
				return d_data[y_line * d_width + x_pixel];
			}

			const T &
			at(
					unsigned int x_pixel,
					unsigned int y_line) const
			{
				return d_data[y_line * d_width + x_pixel];
			}

		protected:

			~WithData()
			{  }

		private:

			unsigned int d_width, d_height;
			boost::scoped_array<T> d_data;
		};


		/**
		 * Use WithProxiedData if the RawRaster derivation stores a reference to a file on
		 * disk instead of storing the entire raster data in memory all the time.
		 */
		template<typename T>
		class WithProxiedData
		{
		public:

			enum
			{
				has_data = false
			};

			enum
			{
				has_proxied_data = true
			};

			WithProxiedData(
					unsigned int width_,
					unsigned int height_,
					const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle) :
				d_width(width_),
				d_height(height_),
				d_raster_band_reader_handle(raster_band_reader_handle)
			{  }

			unsigned int
			width() const
			{
				return d_width;
			}

			unsigned int
			height() const
			{
				return d_height;
			}

		protected:

			~WithProxiedData()
			{  }

		private:

			const GPlatesFileIO::RasterBandReaderHandle &
			get_raster_band_reader_handle() const
			{
				return d_raster_band_reader_handle;
			}

			GPlatesFileIO::RasterBandReaderHandle &
			get_raster_band_reader_handle()
			{
				return d_raster_band_reader_handle;
			}

			friend class GPlatesPropertyValues::ProxiedRasterResolver;

			unsigned int d_width, d_height;
			GPlatesFileIO::RasterBandReaderHandle d_raster_band_reader_handle;
		};


		/**
		 * Use WithoutData if the RawRaster derivation does not store a pointer to
		 * dynamically allocated memory.
		 */
		template<typename T>
		class WithoutData
		{
		public:

			enum
			{
				has_data = false
			};

			enum
			{
				has_proxied_data = false
			};

		protected:

			~WithoutData()
			{  }
		};
	}


	/**
	 * RawRasterVisitor is a visitor that visits a RawRaster.
	 */
	class RawRasterVisitor
	{
	public:
		
		virtual
		~RawRasterVisitor() = 0;
			// Pure virtual, so that this class is abstract.
			// But it's defined in the .cc file.

		virtual
		void
		visit(
				UninitialisedRawRaster &raster)
		{  }

		virtual
		void
		visit(
				Int8RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedInt8RawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				UInt8RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedUInt8RawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				Int16RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedInt16RawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				UInt16RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedUInt16RawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				Int32RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedInt32RawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				UInt32RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedUInt32RawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				FloatRawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedFloatRawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				DoubleRawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedDoubleRawRaster &raster) // Proxied version.
		{  }

		virtual
		void
		visit(
				Rgba8RawRaster &raster)
		{  }

		virtual
		void
		visit(
				ProxiedRgba8RawRaster &raster) // Proxied version.
		{  }
		
		virtual
		void
		visit(
				CoverageRawRaster &raster)
		{  }
	};


	/**
	 * Allows us to write templated visitors to RawRasters.
	 */
	template<class ImplType>
	class TemplatedRawRasterVisitor :
			public RawRasterVisitor,
			public ImplType
	{
	public:

		TemplatedRawRasterVisitor()
		{  }

		template<class Arg1>
		TemplatedRawRasterVisitor(
				const Arg1 &arg1) :
			ImplType(arg1)
		{  }

		template<class Arg1, class Arg2>
		TemplatedRawRasterVisitor(
				const Arg1 &arg1,
				const Arg2 &arg2) :
			ImplType(arg1, arg2)
		{  }

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
				ProxiedInt8RawRaster &raster) // Proxied version.
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
				ProxiedUInt8RawRaster &raster) // Proxied version.
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
				ProxiedInt16RawRaster &raster) // Proxied version.
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
				ProxiedUInt16RawRaster &raster) // Proxied version.
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
				ProxiedInt32RawRaster &raster) // Proxied version.
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
				ProxiedUInt32RawRaster &raster) // Proxied version.
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
				ProxiedFloatRawRaster &raster) // Proxied version.
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
				ProxiedDoubleRawRaster &raster) // Proxied version.
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

		virtual
		void
		visit(
				ProxiedRgba8RawRaster &raster) // Proxied version.
		{
			ImplType::do_visit(raster);
		}

		virtual
		void
		visit(
				CoverageRawRaster &raster)
		{
			ImplType::do_visit(raster);
		}
	};



	/**
	 * The RawRasterImpl template class is used to create RawRaster derivations
	 * that store rasters of type T. The template parameters DataPolicy, StatisticsPolicy and
	 * NoDataValuePolicy are intended to be used with the policy classes contained in the
	 * RawRasterInternals namespace with corresponding names.
	 */
	template
	<
		typename T,
		template <class> class DataPolicy,
		class StatisticsPolicy,
		template <class> class NoDataValuePolicy
	>
	class RawRasterImpl :
			public RawRaster,
			public DataPolicy<T>,
			public StatisticsPolicy,
			public NoDataValuePolicy<T>
	{
	public:

		typedef RawRasterImpl<T, DataPolicy, StatisticsPolicy, NoDataValuePolicy> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

		typedef T element_type;

		typedef DataPolicy<T> data_policy_base_type;
		typedef StatisticsPolicy statistics_policy_base_type;
		typedef NoDataValuePolicy<T> no_data_value_policy_base_type;

		/**
		 * Creates an uninitialised raster with no data.
		 */
		static
		non_null_ptr_type
		create()
		{
			return new RawRasterImpl();
		}

		/**
		 * Creates an uninitialised raster of size @a width_ by @a height_.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_)
		{
			return new RawRasterImpl(width_, height_);
		}

		/**
		 * Creates a raster that has the given @a data_.
		 *
		 * This instance takes ownership of @a data_.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				T *data_)
		{
			return new RawRasterImpl(width_, height_, data_);
		}

		/**
		 * Creates a proxied raster where the source raster is at @a source_filename.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_)
		{
			return new RawRasterImpl(width_, height_, raster_band_reader_handle_);
		}

		/**
		 * Creates a raster that has the given @a data_ and @a statistics_.
		 *
		 * This instance takes ownership of @a data_.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				T *data_,
				const statistics_policy_base_type &statistics_)
		{
			return new RawRasterImpl(width_, height_, data_, statistics_);
		}

		/**
		 * Creates a proxied raster with the given @a statistics_, where the source
		 * raster is at @a source_filename.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_,
				const statistics_policy_base_type &statistics_)
		{
			return new RawRasterImpl(width_, height_, raster_band_reader_handle_, statistics_);
		}

		/**
		 * Creates a raster that has the given @a data_ and @a no_data_value_.
		 *
		 * This instance takes ownership of @a data_.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				T *data_,
				const no_data_value_policy_base_type &no_data_value_)
		{
			return new RawRasterImpl(width_, height_, data_, no_data_value_);
		}

		/**
		 * Creates a proxied raster that has the given @a no_data_value_, where the
		 * source raster is at @a source_filename.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_,
				const no_data_value_policy_base_type &no_data_value_)
		{
			return new RawRasterImpl(width_, height_, raster_band_reader_handle_, no_data_value_);
		}

		/**
		 * Creates a raster that has the given @a data_, @a statistics_ and
		 * @a no_data_value_.
		 *
		 * This instance takes ownership of @a data_.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				T *data_,
				const statistics_policy_base_type &statistics_,
				const no_data_value_policy_base_type &no_data_value_)
		{
			return new RawRasterImpl(width_, height_, data_, statistics_, no_data_value_);
		}

		/**
		 * Creates a proxied raster that has the given @a statistics_ and
		 * @a no_data_value_, where the source raster is at @a source_filename.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_,
				const statistics_policy_base_type &statistics_,
				const no_data_value_policy_base_type &no_data_value_)
		{
			return new RawRasterImpl(width_, height_, raster_band_reader_handle_, statistics_, no_data_value_);
		}

		virtual
		void
		accept_visitor(
				RawRasterVisitor &visitor)
		{
			visitor.visit(*this);
		}

	private:

		RawRasterImpl()
		{  }

		RawRasterImpl(
				unsigned int width_,
				unsigned int height_) :
			data_policy_base_type(width_, height_)
		{  }

		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				T *data_) :
			data_policy_base_type(width_, height_, data_)
		{  }

		// Proxied data version:
		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_) :
			data_policy_base_type(width_, height_, raster_band_reader_handle_)
		{  }

		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				T *data_,
				const statistics_policy_base_type &statistics_) :
			data_policy_base_type(width_, height_, data_),
			statistics_policy_base_type(statistics_)
		{  }

		// Proxied data version:
		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_,
				const statistics_policy_base_type &statistics_) :
			data_policy_base_type(width_, height_, raster_band_reader_handle_),
			statistics_policy_base_type(statistics_)
		{  }

		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				T *data_,
				const no_data_value_policy_base_type &no_data_value_) :
			data_policy_base_type(width_, height_, data_),
			no_data_value_policy_base_type(no_data_value_)
		{  }

		// Proxied data version:
		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_,
				const no_data_value_policy_base_type &no_data_value_) :
			data_policy_base_type(width_, height_, raster_band_reader_handle_),
			no_data_value_policy_base_type(no_data_value_)
		{  }

		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				T *data_,
				const statistics_policy_base_type &statistics_,
				const no_data_value_policy_base_type &no_data_value_) :
			data_policy_base_type(width_, height_, data_),
			statistics_policy_base_type(statistics_),
			no_data_value_policy_base_type(no_data_value_)
		{  }

		// Proxied data version:
		RawRasterImpl(
				unsigned int width_,
				unsigned int height_,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle_,
				const statistics_policy_base_type &statistics_,
				const no_data_value_policy_base_type &no_data_value_) :
			data_policy_base_type(width_, height_, raster_band_reader_handle_),
			statistics_policy_base_type(statistics_),
			no_data_value_policy_base_type(no_data_value_)
		{  }
	};
}

#endif  // GPLATES_PROPERTYVALUES_RAWRASTER_H
