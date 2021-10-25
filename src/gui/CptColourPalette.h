/* $Id$ */

/**
 * @file 
 * Contains the definition of the CptColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CPTCOLOURPALETTE_H
#define GPLATES_GUI_CPTCOLOURPALETTE_H

#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <QString>

#include "Colour.h"
#include "ColourPalette.h"
#include "ColourPaletteVisitor.h"

#include "maths/Real.h"

#include "utils/Parse.h"
#include "utils/Profile.h"
#include "utils/Select.h"
#include "utils/TypeTraits.h"

namespace GPlatesGui
{
	/**
	 * When rendering a colour scale, it is possible to annotate the particular
	 * z-slice with either the formatted z-values or a user-defined label.
	 *
	 * These types of annotations are used in regular CPT files.
	 */
	namespace ColourScaleAnnotation
	{
		enum Type
		{
			NONE,	/**< Corresponds to the absence of the A flag in a CPT file */
			LOWER,	/**< Corresponds to the L option */
			UPPER,	/**< Corresponds to the U option */
			BOTH	/**< Correpsonds to the B option */
		};
	}
}


namespace GPlatesUtils
{
	using namespace GPlatesGui;

	// Specialisation of Parse for ColourScaleAnnotation::Type.
	template<>
	struct Parse<ColourScaleAnnotation::Type>
	{
		ColourScaleAnnotation::Type
		operator()(
				const QString &s)
		{
			if (s.isEmpty())
			{
				return ColourScaleAnnotation::NONE;
			}
			else if (s == "L")
			{
				return ColourScaleAnnotation::LOWER;
			}
			else if (s == "U")
			{
				return ColourScaleAnnotation::UPPER;
			}
			else
			{
				throw ParseError();
			}
		}
	};
}


namespace GPlatesGui
{
	/**
	 * A colour slice specifies a gradient of colour between two real values.
	 *
	 * These are used to store entries from regular CPT files.
	 */
	class ColourSlice
	{
	public:

		typedef GPlatesMaths::Real value_type;

		explicit
		ColourSlice(
				value_type lower_value_,
				boost::optional<Colour> lower_colour_,
				value_type upper_value_,
				boost::optional<Colour> upper_colour_,
				ColourScaleAnnotation::Type annotation_ = ColourScaleAnnotation::NONE,
				boost::optional<QString> label_ = boost::none);

		bool
		can_handle(
				value_type value) const
		{
			return d_lower_value.dval() <= value.dval() && value.dval() <= d_upper_value.dval();
		}

		boost::optional<Colour>
		get_colour(
				value_type value) const;

		value_type
		lower_value() const
		{
			return d_lower_value;
		}

		void
		set_lower_value(
				value_type lower_value_)
		{
			d_lower_value = lower_value_;
			set_inverse_value_range();
		}

		value_type
		upper_value() const
		{
			return d_upper_value;
		}

		void
		set_upper_value(
				value_type upper_value_)
		{
			d_upper_value = upper_value_;
			set_inverse_value_range();
		}

		const boost::optional<Colour> &
		lower_colour() const
		{
			return d_lower_colour;
		}

		void
		set_lower_colour(
				const boost::optional<Colour> &lower_colour_)
		{
			d_lower_colour = lower_colour_;
		}

		const boost::optional<Colour> &
		upper_colour() const
		{
			return d_upper_colour;
		}

		void
		set_upper_colour(
				const boost::optional<Colour> &upper_colour_)
		{
			d_upper_colour = upper_colour_;
		}

		ColourScaleAnnotation::Type
		annotation() const
		{
			return d_annotation;
		}
		
		void
		set_annotation(
				ColourScaleAnnotation::Type annotation_)
		{
			d_annotation = annotation_;
		}

		const boost::optional<QString> &
		label() const
		{
			return d_label;
		}

		void
		set_label(
				const boost::optional<QString> &label_)
		{
			d_label = label_;
		}

	private:

		value_type d_lower_value, d_upper_value;
		value_type d_inverse_value_range;
		boost::optional<Colour> d_lower_colour, d_upper_colour;
		ColourScaleAnnotation::Type d_annotation;
		boost::optional<QString> d_label;

		void
		set_inverse_value_range()
		{
			d_inverse_value_range = 1.0 / (d_upper_value - d_lower_value);
		}
	};


	bool
	operator<(
			ColourSlice::value_type value,
			const ColourSlice &colour_slice);


	bool
	operator>(
			ColourSlice::value_type value,
			const ColourSlice &colour_slice);

	/**
	 * ColourEntry stores a mapping from one value to one colour.
	 *
	 * These are used to store entries from categorical CPT files.
	 */
	template<typename T, class Enable = void>
	class ColourEntry;
		// This is intentionally not defined.

	/**
	 * In the version of ColourEntry for non-ints, the label is used as the value
	 * that is mapped to the colour.
	 */
	template<typename T>
	class ColourEntry<T, typename boost::disable_if<boost::is_integral<T> >::type>
	{
	public:

		typedef T value_type;

		enum
		{
			is_label_optional = false
		};

		explicit
		ColourEntry(
				int key_,
				Colour colour_,
				const T &label_) :
			d_key(key_),
			d_colour(colour_),
			d_label(label_)
		{
		}

		bool
		can_handle(
				const T &value) const
		{
			return d_label == value;
		}

		// This is not a useless duplicate of colour(). CptColourPalette expects a
		// get_colour() method to calculate the colour for a given value, while
		// colour() is the accessor to the instance variable.
		const Colour &
		get_colour(
				const T &value) const
		{
			return d_colour;
		}

		int
		key() const
		{
			return d_key;
		}

		void
		set_key(
				int key_)
		{
			d_key = key;
		}

		const Colour &
		colour() const
		{
			return d_colour;
		}

		void
		set_colour(
				const Colour &colour_)
		{
			d_colour = colour_;
		}

		const T &
		label() const
		{
		}

		void
		set_label(
				const T &label_)
		{
			d_label = label_;
		}

	private:

		int d_key;
		Colour d_colour;
		T d_label;
	};


	/**
	 * In the specialisation of ColourEntry for int, the integer key is used as the
	 * value that is mapped to the colour, and the label is used as a text label
	 * for rendering purposes.
	 */
	template<typename IntType>
	class ColourEntry<IntType, typename boost::enable_if<boost::is_integral<IntType> >::type>
	{
	public:

		typedef IntType value_type;

		enum
		{
			is_label_optional = true
		};

		explicit
		ColourEntry(
				IntType key_,
				Colour colour_,
				const boost::optional<QString> &label_) :
			d_key(key_),
			d_colour(colour_),
			d_label(label_)
		{
		}

		bool
		can_handle(
				IntType value) const
		{
			return d_key == value;
		}

		// This is not a useless duplicate of colour(). CptColourPalette expects a
		// get_colour() method to calculate the colour for a given value, while
		// colour() is the accessor to the instance variable.
		const Colour &
		get_colour(
				IntType value) const
		{
			return d_colour;
		}

		IntType
		key() const
		{
			return d_key;
		}

		void
		set_key(
				IntType key_)
		{
			d_key = key_;
		}

		const Colour &
		colour() const
		{
			return d_colour;
		}

		void
		set_colour(
				const Colour &colour_)
		{
			d_colour = colour_;
		}

		const boost::optional<QString> &
		label() const
		{
			return d_label;
		}

		void
		set_label(
				const boost::optional<QString> &label_)
		{
			d_label = label_;
		}

	private:

		IntType d_key;
		Colour d_colour;
		boost::optional<QString> d_label;
	};


	template<typename IntType>
	bool
	operator<(
			typename boost::enable_if<boost::is_integral<IntType>, IntType>::type value,
			const ColourEntry<IntType> &colour_entry)
	{
		return value < colour_entry.key();
	}


	template<typename IntType>
	bool
	operator>(
			typename boost::enable_if<boost::is_integral<IntType>, IntType>::type value,
			const ColourEntry<IntType> &colour_entry)
	{
		return value > colour_entry.key();
	}


	namespace CptColourPaletteInternals
	{
		template<typename T, class Enable = void>
		class MakeColourEntry;

		template<typename T>
		class MakeColourEntry<T, typename boost::disable_if<boost::is_integral<T> >::type>
		{
		public:

			typedef int key_type;

			static
			ColourEntry<T>
			make_colour_entry(
					int key,
					const Colour &colour,
					const boost::optional<QString> &label)
			{
				if (!label)
				{
					throw GPlatesUtils::ParseError();
				}
				GPlatesUtils::Parse<T> parse;
				return ColourEntry<T>(key, colour, parse(*label));
			}
		};

		template<typename IntType>
		class MakeColourEntry<IntType, typename boost::enable_if<boost::is_integral<IntType> >::type>
		{
		public:

			typedef IntType key_type;

			static
			ColourEntry<IntType>
			make_colour_entry(
					IntType key,
					const Colour &colour,
					const boost::optional<QString> &label)
			{
				return ColourEntry<IntType>(key, colour, label);
			}
		};
	}


	template<typename T>
	ColourEntry<T>
	make_colour_entry(
			typename CptColourPaletteInternals::MakeColourEntry<T>::key_type key,
			const Colour &colour,
			const boost::optional<QString> &label)
	{
		return CptColourPaletteInternals::MakeColourEntry<T>::make_colour_entry(key, colour, label);
	}


	/**
	 * CptColourPalette stores the in-memory representation of a CPT file, whether
	 * regular or categorical. It is, essentially, a sequence of the in-memory
	 * representations of lines successfully parsed from a CPT file.
	 *
	 * For regular CPT files, the template parameter EntryType is ColourSlice,
	 * which stores the upper and lower values of a z-slice and their associated
	 * colour.
	 *
	 * For categorical CPT files, the template parameter EntryType is ColourEntry<T>,
	 * which stores one key and its associated colour and label.
	 *
	 * A description of a "regular" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node69.html 
	 *
	 * A description of a "categorical" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node68.html 
	 */
	template<class EntryType>
	class CptColourPalette :
			public ColourPalette<typename EntryType::value_type>
	{
	public:

		typedef typename ColourPalette<typename EntryType::value_type>::value_type value_type;

		/**
		 * Adds an entry to the colour palette.
		 *
		 * Entries for regular CPT files and categorical CPT files where the value
		 * type is int should be added in increasing order otherwise the background
		 * and foreground colours are likely to be applied incorrectly.
		 */
		void
		add_entry(
				const EntryType &entry)
		{
			d_entries.push_back(entry);
		}

		/**
		 * Sets the background colour, used for values that go before the first entry.
		 *
		 * This colour is ignored for categorical CPT files where the value type is
		 * not int.
		 */
		void
		set_background_colour(
				const Colour &colour)
		{
			d_background_colour = colour;
		}

		/**
		 * Sets the foreground colour, used for values that go after the last entry.
		 *
		 * This colour is ignored for categorical CPT files where the value type is
		 * not int.
		 */
		void
		set_foreground_colour(
				const Colour &colour)
		{
			d_foreground_colour = colour;
		}

		/**
		 * Sets the NaN colour, used for values that are:
		 *  - NaN
		 *  - not present, and
		 *  - values not covered by entries in the CPT file or the background/
		 *    foreground colours.
		 */
		void
		set_nan_colour(
				const Colour &colour)
		{
			d_nan_colour = colour;
		}

		/**
		 * For regular CPT files, this sets whether colours with three components are
		 * interpreted as RGB or HSV, for both colour slices and FBN lines.
		 *
		 * For categorical CPT files, this setting is only used for FBN lines.
		 */
		void
		set_rgb_colour_model(
				bool rgb_colour_model)
		{
			d_rgb_colour_model = rgb_colour_model;
		}

		/**
		 * @see set_rgb_colour_model().
		 */
		bool
		is_rgb_colour_model() const
		{
			return d_rgb_colour_model;
		}

		/**
		 * Retrieves a Colour based on the @a value given.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				value_type value) const
		{
			//PROFILE_FUNC();

			if (d_entries.empty())
			{
				return d_nan_colour;
			}

			// See if we should use background colour.
			if (use_background_colour(value))
			{
				// Use NaN colour if background colour is not set.
				if (d_background_colour)
				{
					return d_background_colour;
				}
				else
				{
					return d_nan_colour;
				}
			}

			// See if we should use foreground colour.
			if (use_foreground_colour(value))
			{
				// Use NaN colour if foreground colour is not set.
				if (d_foreground_colour)
				{
					return d_foreground_colour;
				}
				else
				{
					return d_nan_colour;
				}
			}

			// Else try and find an entry that accepts the value, else return NaN colour.
			typename std::vector<EntryType>::const_iterator entries_iter = d_entries.begin();
			const typename std::vector<EntryType>::const_iterator entries_end = d_entries.end();
			for ( ; entries_iter != entries_end; ++entries_iter)
			{
				if (entries_iter->can_handle(value))
				{
					return entries_iter->get_colour(value);
				}
			}

			return d_nan_colour;
		}

		size_t
		size() const
		{
			return d_entries.size();
		}

	protected:

		CptColourPalette() :
			d_rgb_colour_model(true)
		{  }

		virtual
		bool
		use_background_colour(
				value_type value) const = 0;

		virtual
		bool
		use_foreground_colour(
				value_type value) const = 0;

		std::vector<EntryType> d_entries;

	private:

		boost::optional<Colour> d_background_colour;
		boost::optional<Colour> d_foreground_colour;
		boost::optional<Colour> d_nan_colour;

		/**
		 * True if the colour model in this CPT file is RGB.
		 * If false, the colour model is HSV.
		 */
		bool d_rgb_colour_model;
	};


	/**
	 * A colour palette that stores entries from a regular CPT file.
	 */
	class RegularCptColourPalette :
			public CptColourPalette<ColourSlice>
	{
	public:

		typedef RegularCptColourPalette this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;
		typedef boost::intrusive_ptr<this_type> maybe_null_ptr_type;
		typedef boost::intrusive_ptr<const this_type> maybe_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return new RegularCptColourPalette();
		}

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_regular_cpt_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_regular_cpt_colour_palette(*this);
		}

		GPlatesMaths::Real
		get_lower_bound() const
		{
			return d_entries.front().lower_value();
		}

		GPlatesMaths::Real
		get_upper_bound() const
		{
			return d_entries.back().upper_value();
		}

	protected:

		virtual
		bool
		use_background_colour(
				value_type value) const
		{
			// Background colour is used if value comes before first slice.
			return value < d_entries.front();
		}

		virtual
		bool
		use_foreground_colour(
				value_type value) const
		{
			// Foreground colour is used if value comes after last slice.
			return value > d_entries.back();
		}

	private:

		explicit
		RegularCptColourPalette()
		{
		}
	};


	namespace CategoricalCptColourPaletteInternals
	{
		template<typename T, class Enable = void>
		struct UseForegroundBackgroundColour;

		template<typename T>
		struct UseForegroundBackgroundColour<T, typename boost::enable_if<boost::is_integral<T> >::type>
		{
			template<typename ValueType>
			static
			bool
			use_background_colour(
					const std::vector<ColourEntry<T> > &entries,
					ValueType value)
			{
				// Background colour is used if value comes before first slice.
				return value < entries.front();
			}

			template<typename ValueType>
			static
			bool
			use_foreground_colour(
					const std::vector<ColourEntry<T> > &entries,
					ValueType value)
			{
				// Foreground colour is used if value comes after last slice.
				return value > entries.back();
			}
		};

		template<typename T>
		struct UseForegroundBackgroundColour<T, typename boost::disable_if<boost::is_integral<T> >::type>
		{
			template<typename ValueType>
			static
			bool
			use_background_colour(
					const std::vector<ColourEntry<T> > &entries,
					ValueType value)
			{
				// Do not use background colour. For categorical CPT files whose value type is
				// not integral, we use the label as the value type, and there is no requirement
				// that the labels are presented in sorted order (in fact, there may be no order).
				return false;
			}

			template<typename ValueType>
			static
			bool
			use_foreground_colour(
					const std::vector<ColourEntry<T> > &entries,
					ValueType value)
			{
				// Do not use foreground colour. For categorical CPT files whose value type is
				// not integral, we use the label as the value type, and there is no requirement
				// that the labels are presented in sorted order (in fact, there may be no order).
				return false;
			}
		};

		/**
		 * This exists because this class is visitable only for certain template parameters T.
		 */
		template<typename T>
		struct AcceptVisitor
		{
			static
			void
			do_accept_visitor(
					ConstColourPaletteVisitor &visitor,
					const CategoricalCptColourPalette<T> &colour_palette)
			{
				// Do nothing.
			}

			static
			void
			do_accept_visitor(
					ColourPaletteVisitor &visitor,
					CategoricalCptColourPalette<T> &colour_palette)
			{
				// Do nothing.
			}
		};

		template<>
		struct AcceptVisitor<boost::int32_t>
		{
			static
			void
			do_accept_visitor(
					ConstColourPaletteVisitor &visitor,
					const CategoricalCptColourPalette<boost::int32_t> &colour_palette)
			{
				visitor.visit_int32_categorical_cpt_colour_palette(colour_palette);
			}

			static
			void
			do_accept_visitor(
					ColourPaletteVisitor &visitor,
					CategoricalCptColourPalette<boost::int32_t> &colour_palette)
			{
				visitor.visit_int32_categorical_cpt_colour_palette(colour_palette);
			}
		};

		template<>
		struct AcceptVisitor<boost::uint32_t>
		{
			static
			void
			do_accept_visitor(
					ConstColourPaletteVisitor &visitor,
					const CategoricalCptColourPalette<boost::uint32_t> &colour_palette)
			{
				visitor.visit_uint32_categorical_cpt_colour_palette(colour_palette);
			}

			static
			void
			do_accept_visitor(
					ColourPaletteVisitor &visitor,
					CategoricalCptColourPalette<boost::uint32_t> &colour_palette)
			{
				visitor.visit_uint32_categorical_cpt_colour_palette(colour_palette);
			}
		};
	}


	/**
	 * A colour palette that stores entries from a categorical CPT file.
	 */
	template<typename T>
	class CategoricalCptColourPalette :
			public CptColourPalette<ColourEntry<T> >
	{
		typedef CptColourPalette<ColourEntry<T> > base_type;

	public:

		typedef CategoricalCptColourPalette<T> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;
		typedef boost::intrusive_ptr<this_type> maybe_null_ptr_type;
		typedef boost::intrusive_ptr<const this_type> maybe_null_ptr_to_const_type;

		typedef typename base_type::value_type value_type;

		static
		non_null_ptr_type
		create()
		{
			return new CategoricalCptColourPalette();
		}

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			CategoricalCptColourPaletteInternals::AcceptVisitor<T>::do_accept_visitor(visitor, *this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			CategoricalCptColourPaletteInternals::AcceptVisitor<T>::do_accept_visitor(visitor, *this);
		}

		/**
		 * Returns the lower bound of the range covered by this colour palette.
		 * This function can only be called if @a T is integral.
		 */
		T
		get_lower_bound() const
		{
			return d_entries.front().key();
		}

		/**
		 * Returns the upper bound of the range covered by this colour palette.
		 * This function can only be called if @a T is integral.
		 */
		T
		get_upper_bound() const
		{
			return d_entries.back().key();
		}

	protected:

		explicit
		CategoricalCptColourPalette()
		{
		}

		virtual
		bool
		use_background_colour(
				value_type value) const
		{
			return CategoricalCptColourPaletteInternals::UseForegroundBackgroundColour<T>::use_background_colour(
					d_entries, value);
		}

		virtual
		bool
		use_foreground_colour(
				value_type value) const
		{
			return CategoricalCptColourPaletteInternals::UseForegroundBackgroundColour<T>::use_foreground_colour(
					d_entries, value);
		}

	private:

		using base_type::d_entries;
	};
}

#endif  /* GPLATES_GUI_CPTCOLOURPALETTE_H */
