/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourPaletteAdapter template class.
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

#ifndef GPLATES_GUI_COLOURPALETTEADAPTER_H
#define GPLATES_GUI_COLOURPALETTEADAPTER_H

#include "ColourPalette.h"

#include "maths/Real.h"

#include "utils/Select.h"
#include "utils/TypeTraits.h"


namespace GPlatesGui
{
	template<typename FromType, typename ToType>
	struct StaticCastConverter
	{
		typedef typename GPlatesUtils::Select
		<
			GPlatesUtils::TypeTraits<FromType>::is_built_in,
			FromType,
			const FromType &
		>::result parameter_type;

		ToType
		operator()(
				parameter_type value) const
		{
			return static_cast<ToType>(value);
		}
	};


	template<typename T>
	struct RealToBuiltInConverter
	{
		T
		operator()(
				const GPlatesMaths::Real &value) const
		{
			return static_cast<T>(value.dval());
		}
	};


	/**
	 * This class adapts a ColourPalette<FromType> into a ColourPalette<ToType>.
	 * For example, a ColourPalette that maps doubles to colours can be adapted
	 * into a ColourPalette that maps ints to colours.
	 *
	 * The default converter uses a static_cast to convert the FromType into a
	 * ToType. If this behaviour is not appropriate, provide a functor that
	 * converts a FromType into a ToType.
	 */
	template
	<
		typename FromType,
		typename ToType,
		class ConverterType = StaticCastConverter<FromType, ToType>
	>
	class ColourPaletteAdapter :
			public ColourPalette<ToType>
	{
	public:

		typedef typename ColourPalette<ToType>::value_type value_type;

		typedef ColourPaletteAdapter<FromType, ToType, ConverterType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;
		typedef boost::intrusive_ptr<this_type> maybe_null_ptr_type;
		typedef boost::intrusive_ptr<const this_type> maybe_null_ptr_to_const_type;

		template<class SourceColourPalettePointerType>
		static
		non_null_ptr_type
		create(
				SourceColourPalettePointerType adaptee,
				ConverterType convert = ConverterType())
		{
			return new ColourPaletteAdapter(adaptee.get(), convert);
		}

		virtual
		boost::optional<Colour>
		get_colour(
				value_type value) const
		{
			return d_adaptee->get_colour(d_convert(value));
		}

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			d_adaptee->accept_visitor(visitor);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			d_adaptee->accept_visitor(visitor);
		}

	private:

		/**
		 * Constructs a ColourPaletteAdapter that adapts the @a adaptee.
		 * This adapter object takes ownership of @a adaptee.
		 */
		ColourPaletteAdapter(
				typename ColourPalette<FromType>::non_null_ptr_type adaptee,
				ConverterType convert) :
			d_adaptee(adaptee),
			d_convert(convert)
		{
		}

		typename ColourPalette<FromType>::non_null_ptr_type d_adaptee;
		ConverterType d_convert;
	};


	namespace ColourPaletteAdapterInternals
	{
		// This exists because we can't partially specialise function templates.
		template
		<
			typename FromType,
			typename ToType,
			class ConverterType
		>
		struct ConvertColourPalette
		{
			static
			typename ColourPalette<ToType>::non_null_ptr_type
			convert_colour_palette(
					typename ColourPalette<FromType>::non_null_ptr_type adaptee,
					ConverterType convert)
			{
				return ColourPaletteAdapter<FromType, ToType, ConverterType>::create(
						adaptee, convert);
			}
		};

		template
		<
			typename Type,
			class ConverterType
		>
		struct ConvertColourPalette<Type, Type, ConverterType>
		{
			static
			typename ColourPalette<Type>::non_null_ptr_type
			convert_colour_palette(
					typename ColourPalette<Type>::non_null_ptr_type adaptee,
					ConverterType convert)
			{
				return adaptee;
			}
		};
	}


	/**
	 * Converts a ColourPalette<FromType> to ColourPalette<ToType>. See the
	 * comments for ColourPaletteAdapter for a description of ConverterType.
	 *
	 * If FromType and ToType are the same, the return value is the colour palette
	 * that was passed in, without being wrapped in an adapter.
	 */
	template
	<
		typename FromType,
		typename ToType,
		class ConverterType
	>
	typename ColourPalette<ToType>::non_null_ptr_type
	convert_colour_palette(
			typename ColourPalette<FromType>::non_null_ptr_type adaptee,
			ConverterType convert)
	{
		return ColourPaletteAdapterInternals::ConvertColourPalette
			<FromType, ToType, ConverterType>::convert_colour_palette(
					adaptee, convert);
	}

	/**
	 * Converts a ColourPalette<FromType> to ColourPalette<ToType>.
	 *
	 * If FromType and ToType are the same, the return value is the colour palette
	 * that was passed in, without being wrapped in an adapter.
	 *
	 * This overload uses StaticCastConverter as the converter.
	 */
	template
	<
		typename FromType,
		typename ToType
	>
	typename ColourPalette<ToType>::non_null_ptr_type
	convert_colour_palette(
			typename ColourPalette<FromType>::non_null_ptr_type adaptee)
	{
		return convert_colour_palette<FromType, ToType>(
				adaptee, StaticCastConverter<FromType, ToType>());
	}
}

#endif  /* GPLATES_GUI_COLOURPALETTEADAPTER_H */
