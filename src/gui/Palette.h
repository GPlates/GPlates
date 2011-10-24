/* $Id: ColourPalette.h 12043 2011-08-02 05:50:57Z mchin $ */

/**
 * @file 
 * Contains the definition of the Palette class.
 *
 * Most recent change:
 *   $Date: 2011-08-02 15:50:57 +1000 (Tue, 02 Aug 2011) $
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_PALETTE_H
#define GPLATES_GUI_PALETTE_H
#include <map>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/variant.hpp>
#include <QDebug>
#include <QFile>

#include "Colour.h"
#include "ColourSpectrum.h"
#include "HTMLColourNames.h"
#include "model/types.h"

namespace GPlatesGui 
{
	/*
	*/
	class Palette
	{
		typedef boost::variant<
				long,
				double ,
				QString> KeyType;
	public:
		class Key
		{
			class less_op
				: public boost::static_visitor<bool>
			{
			public:
				template <typename T, typename U>
				bool operator()( const T & lhs, const U & rhs) const
				{
					//qWarning() << "Comparing different types in colour palette map.";
					//even the types are different, we still have a chance to compare them.
					boost::optional<double> 
						l_double = Key(lhs).to_double(), 
						r_double = Key(rhs).to_double();
					if(l_double && r_double)
						return *l_double < *r_double;

					return  Key(lhs).to_qstring() < Key(rhs).to_qstring();
				}

				template <typename T>
				bool operator()( const T & lhs, const T & rhs ) const
				{
					return lhs < rhs;
				}

			};

			class to_long_visitor
				: public boost::static_visitor<boost::optional<long> >
			{
			public:
				boost::optional<long> 
				operator()(
						const long value) const
				{
					return value;
				}

				boost::optional<long> 
				operator()(
						const double value) const
				{
					return boost::optional<long>(static_cast<long>(value));
				}

				boost::optional<long> 
				operator()(
						const QString& value) const
				{
					bool ok; long ret;
					ret = value.toLong(&ok);
					return ok ? boost::optional<long>(ret) : boost::none;
				}

				template <typename T>
				boost::optional<long> 
				operator()( const T & value ) const
				{
					return boost::none;
				}

			};


			class to_double_visitor
				: public boost::static_visitor<boost::optional<double> >
			{
			public:
				boost::optional<double> 
				operator()(
						const double value) const
				{
					return value;
				}

				boost::optional<double> 
				operator()(
						const long value) const
				{
					return boost::optional<double>(static_cast<double>(value));
				}

				boost::optional<double> 
				operator()(
						const QString& value) const
				{
					bool ok; long ret;
					ret = value.toDouble(&ok);
					return ok ? boost::optional<double>(ret) : boost::none;
				}

				template <typename T>
				boost::optional<double> 
				operator()( const T & value ) const
				{
					return boost::none;
				}

			};


			class to_qstring_visitor
				: public boost::static_visitor<QString>
			{
			public:
				QString 
				operator()(
						const QString& value) const
				{
					return value;
				}

				template <typename T>
				QString 
				operator()( const T & value ) const
				{
					return QString().setNum(value);
				}

			};

			friend 
			inline bool operator<(
					const Palette::Key& l, 
					const Palette::Key& r)
			{
				return boost::apply_visitor(less_op(), l.d_data, r.d_data);
			}

		public:
			Key(){}

			Key(const long k) :
				d_data(k)
			{ }

			Key(const int k) :
			d_data(static_cast<const long>(k))
			{ }

			Key(const double k) :
				d_data(k)
			{ }

			Key(const float k) :
				d_data(static_cast<const double>(k))
			{ }

			Key(const char* k) :
				d_data(QString::fromUtf8(k))
			{ }

			Key(const QString& k) :
				d_data(k)
			{ }

			boost::optional<long>
			to_long() const 
			{
				return boost::apply_visitor(to_long_visitor(), d_data);
			}

			boost::optional<double>
			to_double() const
			{
				return boost::apply_visitor(to_double_visitor(), d_data);
			}

			QString
			to_qstring() const
			{
				return boost::apply_visitor(to_qstring_visitor(), d_data);
			}

		private:
			KeyType d_data;
		};

		virtual
		~Palette(){ }

		
		virtual
		boost::optional<Colour>
		get_colour(const Key& k) const = 0;


		virtual
		void
		set_BFN_colour(
				const Colour& b, 
				const Colour& f, 
				const Colour& n)
		{
			d_background_color = b;
			d_foreground_color = f;
			d_default_color = n;
		}

		/*
		* Get background, foreground and NaN(default) color.
		*/
		virtual
		boost::tuple<Colour, Colour, Colour>
		get_BFN_colour() const 
		{
			return boost::make_tuple(d_background_color, d_foreground_color, d_default_color);
		}

		protected:
			Palette() : 
				d_background_color(Colour::get_black()),
				d_foreground_color(Colour::get_white()),
				d_default_color(Colour::get_blue())
			{ }
			Colour d_background_color;
			Colour d_foreground_color;
			Colour d_default_color;
	};


	class CategoricalPalette : public Palette
	{
	public:
		typedef std::map<const Palette::Key, Colour> ColourMapType;
		
		CategoricalPalette(){}

		CategoricalPalette(const ColourMapType& map) :
			d_color_map(map)
		{ }

		void
		insert(
				const Palette::Key& k, 
				Colour c)
		{
			d_color_map[k] = c;
		}

		virtual
		boost::optional<Colour>
		get_colour(const Key& k) const;
		
		virtual
		~CategoricalPalette(){ }

	protected:
		virtual
		void
		build_map(){ }

		virtual
		const Key
		mapping_key(const Key& k) const
		{
			return k;
		}

		std::map<const Palette::Key, Colour> d_color_map;
	};


	class RegularPalette : public Palette
	{
	public:
		explicit
		RegularPalette(const std::vector<ColourSpectrum>& spectrums) :
			d_spectrums(spectrums)
		{ }

		RegularPalette(){ }

		void
		append(const ColourSpectrum& sp)
		{
			d_spectrums.push_back(sp);
		}

		virtual
		boost::optional<Colour>
		get_colour(const Key& k) const;
		
		virtual
		~RegularPalette(){ }

	protected:

		std::vector<ColourSpectrum> d_spectrums;
	};


	class SingleColorPalette : public Palette
	{
	public:
		explicit
		SingleColorPalette(const Colour& c) :
			d_color(c)
		{ }
		
		virtual
		boost::optional<Colour>
		get_colour(const Key& k) const 
		{
			return d_color;
		}
		
		virtual
		~SingleColorPalette(){ }

	protected:
		const Colour d_color;
	};

	inline
	GPlatesGui::Colour
	html_colour(const char *name)
	{
		return *HTMLColourNames::instance().get_colour(name);
	}

	class DefaultPlateIdPalette : public CategoricalPalette
	{
	public:
		static 
		const DefaultPlateIdPalette*
		instance()
		{
			static DefaultPlateIdPalette* inst = new DefaultPlateIdPalette();
			return inst;
		}

		const Key
		mapping_key(const Key& k)  const 
		{
			boost::optional<long> int_val = k.to_long();
			if(int_val)
			{
				long tmp = *int_val % d_color_map.size();
				return Palette::Key(tmp);
			}
			return k;
		}
					
		~DefaultPlateIdPalette(){ }
		
	protected:
		void build_map();
		DefaultPlateIdPalette(){ build_map(); }
		DefaultPlateIdPalette(const DefaultPlateIdPalette&);
	};

	inline
	int 
	leading_digit(
			GPlatesModel::integer_plate_id_type plate_id)
	{
		while (plate_id >= 10)
		{
			plate_id /= 10;
		}
		return static_cast<int>(plate_id);
	}

	inline
	int
	get_region_from_plate_id(
			GPlatesModel::integer_plate_id_type plate_id)
	{
		if (plate_id < 100) // plate 0xx is treated as being in region zero
		{
			return 0;
		}
		else
		{
			return leading_digit(plate_id);
		}
	}

	class RegionalPlateIdPalette : public CategoricalPalette
	{
	public:
		static 
		const RegionalPlateIdPalette*
		instance()
		{
			static RegionalPlateIdPalette* inst = new RegionalPlateIdPalette();
			return inst;
		}

		boost::optional<Colour>
		get_colour(const Key& k)  const;

		
	protected:
		void build_map();
		RegionalPlateIdPalette(){build_map();}
		RegionalPlateIdPalette(const RegionalPlateIdPalette&);
	};


	const Palette*
	default_age_palette(
			const double upper = 450, 
			const double lower = 0);


	const Palette*
	mono_age_palette(
			const double upper = 450, 
			const double lower = 0);


	class FeatureTypePalette : public CategoricalPalette
	{
	public:
		static 
		const FeatureTypePalette*
		instance()
		{
			static FeatureTypePalette* inst = new FeatureTypePalette();
			return inst;
		}
					
		~FeatureTypePalette(){ }
		
	protected:
		void build_map();
		FeatureTypePalette(){ build_map(); }
		FeatureTypePalette(const FeatureTypePalette&);
	};


	class CptPalette : public Palette
	{
	public:
		CptPalette(const QString& file);
		
		boost::optional<Colour>
		get_colour(const Key& k) const 
		{
			boost::optional<Colour> c = d_cate_palette->get_colour(k);
			if(!c)
			{
				c = d_regular_palette->get_colour(k);
			}
			return c;
		}
	private:
		boost::scoped_ptr<CategoricalPalette> d_cate_palette;
		boost::scoped_ptr<RegularPalette>	 d_regular_palette;
	};


namespace
{	
	inline
	bool
	init_built_in_pallette(std::map<QString, const Palette*>& palette_map)
	{
		palette_map["DefaultPlateId"]		= DefaultPlateIdPalette::instance();
		palette_map["Region"]				= RegionalPlateIdPalette::instance();
		palette_map["FeatureAgeDefault"]	= default_age_palette();
		palette_map["FeatureAgeMono"]		= mono_age_palette();
		palette_map["FeatureType"]			= FeatureTypePalette::instance();
		return true;
	}
}

	inline
	const std::map<QString, const Palette*>& 
	built_in_palette()
	{
		static std::map<QString, const Palette*> palette_map;
		static bool dummy = init_built_in_pallette(palette_map); 
		dummy = !dummy;//dummy code to deceive compiler.
		return palette_map;
	}

	inline
	const Palette*
	built_in_palette(const QString& name)
	{
		std::map<QString, const Palette*>::const_iterator it = built_in_palette().find(name);
		if(it != built_in_palette().end())
			return it->second;
		else
			return NULL;
	}
}


namespace GPlatesApi
{
	class Palette
	{
	public:
		Palette(const GPlatesGui::Palette* p) :
		d_p(p)
		{ }

		  GPlatesGui::Colour
		  get_color(const GPlatesGui::Palette::Key k)
		  {
			  boost::optional<GPlatesGui::Colour> color = boost::none;
			  if(d_p)
			  {
				  color = d_p->get_colour(k);
				  if(color)
					  return *color;
				  else
					  return boost::get<2>(d_p->get_BFN_colour());
			  }
			  else
			  {
				  return GPlatesGui::Colour::get_black();
			  }
		  }

	private:
		const GPlatesGui::Palette* d_p;
	};
}

#endif  /* GPLATES_GUI_COLOURPALETTE_H */




