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
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "RasterType.h"
#include "RawRaster.h"

#include "maths/MathsUtils.h"
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


			/**
			 * Allows us to decompose the policies out of a proxied raster type.
			 */
			template<class ProxiedRawRasterType>
			struct RawRasterTraits;
				// This is intentionally not defined anywhere.
			
			template
			<
				typename T,
				class StatisticsPolicy,
				template <class> class NoDataValuePolicy
			>
			struct RawRasterTraits<RawRasterImpl<
				T, RawRasterDataPolicies::WithProxiedData, StatisticsPolicy, NoDataValuePolicy> >
			{
				/**
				 * Basically, it takes uses the same element_type, statistics and
				 * no data value policies as ExistingRawRasterType, but swaps out
				 * the data policy to be WithData (i.e. not proxied).
				 */
				typedef RawRasterImpl
				<
					T,
					RawRasterDataPolicies::WithData,
					StatisticsPolicy,
					NoDataValuePolicy
				> with_data_type;

				typedef RawRasterImpl
				<
					T,
					RawRasterDataPolicies::WithProxiedData,
					StatisticsPolicy,
					NoDataValuePolicy
				> with_proxied_data_type;

				/**
				 * Converts a proxied raw raster to the unproxied raw raster with
				 * the same data type.
				 */
				static
				typename with_data_type::non_null_ptr_type
				convert_proxied_raster_to_unproxied_raster(
						typename with_proxied_data_type::non_null_ptr_type proxied_raster,
						unsigned int region_width,
						unsigned int region_height,
						T *data)
				{
					return with_data_type::create(
							region_width,
							region_height,
							data,
							*proxied_raster /* copy statistics policy */,
							*proxied_raster /* copy no-data policy */);
				}
			};


			/**
			 * Creates a coverage raster from a raster.
			 */
			template<class RawRasterType, bool has_no_data_value>
			class CreateCoverageRawRaster
			{
			public:

				static
				boost::optional<CoverageRawRaster::non_null_ptr_type>
				create_coverage_raster(
						const RawRasterType &raster)
				{
					// has_no_data_value = false case handled here.
					// No work to do, because our raster can't have sentinel values anyway.
					return boost::none;
				}
			};

			template<class RawRasterType>
			class CreateCoverageRawRaster<RawRasterType, true>
			{
				class CoverageFunctor :
						public std::unary_function<typename RawRasterType::element_type, CoverageRawRaster::element_type>
				{
				public:

					typedef boost::function<bool (typename RawRasterType::element_type)> is_no_data_value_function_type;

					CoverageFunctor(
							is_no_data_value_function_type is_no_data_value) :
						d_is_no_data_value(is_no_data_value)
					{
					}

					CoverageRawRaster::element_type
					operator()(
							typename RawRasterType::element_type value)
					{
						static const CoverageRawRaster::element_type NO_DATA_COVERAGE_VALUE = 0.0f;
						static const CoverageRawRaster::element_type DATA_PRESENT_VALUE = 1.0f;

						if (d_is_no_data_value(value))
						{
							return NO_DATA_COVERAGE_VALUE;
						}
						else
						{
							return DATA_PRESENT_VALUE;
						}
					}

				private:

					is_no_data_value_function_type d_is_no_data_value;
				};

			public:

				static
				boost::optional<CoverageRawRaster::non_null_ptr_type>
				create_coverage_raster(
						const RawRasterType &raster)
				{
					CoverageRawRaster::non_null_ptr_type coverage =
							CoverageRawRaster::create(raster.width(), raster.height());

					std::transform(
							raster.data(),
							raster.data() + raster.width() * raster.height(),
							coverage->data(),
							CoverageFunctor(
								boost::bind(
									&RawRasterType::is_no_data_value,
									boost::cref(raster),
									_1)));

					return coverage;
				}
			};


			/**
			 * Determines if a raster has a no-data value and is not fully opaque.
			 */
			template<class RawRasterType, bool has_no_data_value>
			class DoesRasterContainANoDataValue
			{
			public:
				static
				bool
				does_raster_contain_a_no_data_value(
						const RawRasterType &raster)
				{
					// has_no_data_value = false case handled here.
					// No work to do, because our raster can't have sentinel values anyway.
					return false;
				}
			};


			template<class RawRasterType>
			class DoesRasterContainANoDataValue<RawRasterType, true/*has_no_data_value*/>
			{
			public:
				static
				bool
				does_raster_contain_a_no_data_value(
						const RawRasterType &raster)
				{
					// Iterate over the pixels and see if any are the sentinel value meaning
					// that that pixel is transparent.
					const unsigned int raster_width = raster.width();
					const unsigned int raster_height = raster.height();
					for (unsigned int j = 0; j < raster_height; ++j)
					{
						const typename RawRasterType::element_type *const row =
								raster.data() + j * raster_width;

						for (unsigned int i = 0; i < raster_width; ++i)
						{
							if (raster.is_no_data_value(row[i]))
							{
								// Raster contains a sentinel value so it's not fully opaque.
								return true;
							}
						}
					}

					// Raster contains no sentinel values, hence it is fully opaque.
					return false;
				};
			};


			/**
			 * Adds a no-data value to a raster - also converts no-data pixel values (in raster data)
			 * from the value used to load the raster data to the value expected by the raster type.
			 *
			 * This is useful when you've loaded data into a raster and need to set the no-data value
			 * that's appropriate for the data loaded.
			 */
			template<class RawRasterType>
			struct AddNoDataValue
			{
				typedef typename RawRasterType::element_type raster_element_type;

				static
				void
				add_no_data_value(
						RawRasterType &raster,
						const raster_element_type &no_data_value)
				{
					// Default case: do nothing.
				}
			};

			// Specialisation for rasters that have a no-data value that can be set.
			template<typename T, template <class> class DataPolicy, class StatisticsPolicy>
			struct AddNoDataValue<
				RawRasterImpl<T, DataPolicy, StatisticsPolicy,
					RawRasterNoDataValuePolicies::WithNoDataValue> >
			{
				typedef RawRasterImpl<T, DataPolicy, StatisticsPolicy,
					RawRasterNoDataValuePolicies::WithNoDataValue> RawRasterType;
				typedef typename RawRasterType::element_type raster_element_type;

				static
				void
				add_no_data_value(
						RawRasterType &raster,
						const raster_element_type &no_data_value)
				{
					raster.set_no_data_value(no_data_value);

					// Rasters that have data will be caught by the next specialisation which will
					// alter the data matching the old no-data value to the new no-data value...
				}
			};

			// Specialisation for rasters that have a no-data value that can be set *and* have data.
			template<typename T, class StatisticsPolicy>
			struct AddNoDataValue<
				RawRasterImpl<T, RawRasterDataPolicies::WithData,
					StatisticsPolicy, RawRasterNoDataValuePolicies::WithNoDataValue> >
			{
				typedef RawRasterImpl<T, RawRasterDataPolicies::WithData, StatisticsPolicy,
					RawRasterNoDataValuePolicies::WithNoDataValue> RawRasterType;
				typedef typename RawRasterType::element_type raster_element_type;

				static
				void
				add_no_data_value(
						RawRasterType &raster,
						const raster_element_type &no_data_value)
				{
					// If there is already a no-data value on the raster and it is different than
					// 'no_data_value' then convert all values that match the previous no-data value
					// to 'no_data_value'.
					if (raster.no_data_value() &&
						!raster.is_no_data_value(no_data_value))
					{
						raster_element_type *const data = raster.data();
						const unsigned int num_pixels = raster.width() * raster.height();
						for (unsigned int n = 0; n < num_pixels; ++n)
						{
							if (raster.is_no_data_value(data[n]))
							{
								data[n] = no_data_value;
							}
						}
					}

					raster.set_no_data_value(no_data_value);
				}
			};

			// Specialisation for rasters that have data and always use NaN as no-data value.
			template<typename T, class StatisticsPolicy>
			struct AddNoDataValue<
				RawRasterImpl<T, RawRasterDataPolicies::WithData,
					StatisticsPolicy, RawRasterNoDataValuePolicies::NanNoDataValue> >
			{
				typedef RawRasterImpl<T, RawRasterDataPolicies::WithData,
					StatisticsPolicy, RawRasterNoDataValuePolicies::NanNoDataValue> RawRasterType;
				typedef typename RawRasterType::element_type raster_element_type;

				static
				void
				add_no_data_value(
						RawRasterType &raster,
						const raster_element_type &no_data_value)
				{
					// If the no-data value of the raster data is NaN, then there is nothing
					// to do, because this RawRasterType expects NaN as the no-data value.
					if (GPlatesMaths::is_nan(no_data_value))
					{
						return;
					}

					// If it is not NaN, however, we will have to convert all values that match
					// no_data_value to NaN.
					raster_element_type casted_nan_value = GPlatesMaths::quiet_nan<raster_element_type>();

					std::replace_if(
							raster.data(),
							raster.data() + raster.width() * raster.height(),
							boost::bind(
								&GPlatesMaths::are_almost_exactly_equal<raster_element_type>,
								_1,
								no_data_value),
							casted_nan_value);
				}
			};


			/**
			 * Adds statistics to a raster.
			 *
			 * This is useful when you've loaded data into a raster and need to set the statistics afterwards.
			 */
			template<class RawRasterType>
			struct AddRasterStatistics
			{
				static
				void
				add_raster_statistics(
						RawRasterType &raster,
						const RasterStatistics &raster_statistics)
				{
					// Default case: do nothing.
				}
			};

			// Specialisation for rasters that have raster statistics.
			template<typename T, template <class> class DataPolicy, template <class> class NoDataValuePolicy>
			struct AddRasterStatistics<
				RawRasterImpl<T, DataPolicy, RawRasterStatisticsPolicies::WithStatistics, NoDataValuePolicy> >
			{
				typedef RawRasterImpl<T, DataPolicy, RawRasterStatisticsPolicies::WithStatistics,
						NoDataValuePolicy> RawRasterType;
				typedef typename RawRasterType::element_type raster_element_type;

				static
				void
				add_raster_statistics(
						RawRasterType &raster,
						const RasterStatistics &raster_statistics)
				{
					raster.set_statistics(raster_statistics);
				}
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
		 * Returns true if the specified raster contains numerical data such as floating-point
		 * or integer pixels (but not RGBA colour pixels).
		 */
		bool
		does_raster_contain_numerical_data(
				RawRaster &raster);

		/**
		 * Returns true if the specified raster contains colour data such as RGBA pixel
		 * (but not numerical data such as floating-point or integer pixels).
		 */
		inline
		bool
		does_raster_contain_colour_data(
				RawRaster &raster)
		{
			return !does_raster_contain_numerical_data(raster);
		}


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
		 * Returns true if the specified raster has a no-data sentinel value in the raster.
		 *
		 * NOTE: RGBA rasters have an alpha-channel and hence can be transparent but
		 * do not have a no-data value (because of the alpha-channel).
		 */
		template<class RawRasterType>
		boost::optional<CoverageRawRaster::non_null_ptr_type>
		create_coverage_raster(
				const RawRasterType &raster)
		{
			return RawRasterUtilsInternals::CreateCoverageRawRaster<
					RawRasterType, RawRasterType::has_no_data_value>
							::create_coverage_raster(raster);
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
		 * Returns true if @a raster has any pixels with an alpha value of 255.
		 */
		bool
		has_fully_transparent_pixels(
				const Rgba8RawRaster::non_null_ptr_type &raster);


		/**
		 * Returns true if the specified raster has a no-data sentinel value in the raster.
		 *
		 * NOTE: RGBA rasters have an alpha-channel and hence can be transparent but
		 * do not have a no-data value (because of the alpha-channel).
		 */
		template<class RawRasterType>
		bool
		does_raster_contain_a_no_data_value(
				const RawRasterType &raster)
		{
			return RawRasterUtilsInternals::DoesRasterContainANoDataValue<
					RawRasterType, RawRasterType::has_no_data_value>
							::does_raster_contain_a_no_data_value(raster);
		}


		/**
		 * Adds a no-data value to a raster - also converts no-data pixel values (in raster data)
		 * from the value used to load the raster data to the value expected by the raster type
		 * (this applies to floating-point raster types - they always have NaN as the no-data value).
		 *
		 * If there is a previous no-data value set on the raster and it differs from @a no_data_value
		 * then pixel values matching the previous no-data value will be converted to @a no_data_value
		 * (this applies to integer raster types - they have no-data values that can be any integer).
		 *
		 * This is useful when you've loaded data into a raster and need to set the no-data value
		 * that's appropriate for the data loaded.
		 */
		template<class RawRasterType>
		void
		add_no_data_value(
				RawRasterType &raster,
				const typename RawRasterType::element_type &no_data_value)
		{
			return RawRasterUtilsInternals::AddNoDataValue<RawRasterType>::add_no_data_value(
					raster, no_data_value);
		}


		/**
		 * Adds raster statistics to a raster.
		 */
		template<class RawRasterType>
		void
		add_raster_statistics(
				RawRasterType &raster,
				const RasterStatistics &raster_statistics)
		{
			return RawRasterUtilsInternals::AddRasterStatistics<RawRasterType>::add_raster_statistics(
					raster, raster_statistics);
		}


		/**
		 * Given a proxied raw raster type converts to the equivalent unproxied raw raster type.
		 *
		 * Basically, it uses the same element_type, statistics and
		 * no data value policies as ProxiedRawRasterType, but swaps out
		 * the data policy to be WithData (i.e. not proxied).
		 */
		template <class ProxiedRawRasterType>
		struct ConvertProxiedRasterToUnproxiedRaster
		{
			typedef typename RawRasterUtilsInternals::RawRasterTraits<ProxiedRawRasterType>
					::with_data_type unproxied_raster_type;
		};

		/**
		 * Takes @a data of specified dimensions and returns it in an unproxied raster of the
		 * same element type.
		 *
		 * Only the statistics and no data value policy parts are copied from @a proxied_raw_raster
		 * to the returned raster.
		 *
		 * @a data must have been allocated using the array version of operator 'new' and
		 * ownership is passed to the returned unproxied raster.
		 */
		template <class ProxiedRawRasterType>
		typename ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>
				::unproxied_raster_type::non_null_ptr_type
		convert_proxied_raster_to_unproxied_raster(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				unsigned int width,
				unsigned int height,
				typename ProxiedRawRasterType::element_type *data)
		{
			return RawRasterUtilsInternals::RawRasterTraits<ProxiedRawRasterType>
					::convert_proxied_raster_to_unproxied_raster(
							proxied_raw_raster,
							width,
							height,
							data);
		}
	}
}

#endif  // GPLATES_PROPERTYVALUES_RAWRASTERUTILS_H
