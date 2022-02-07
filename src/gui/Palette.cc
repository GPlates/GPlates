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

#include <boost/foreach.hpp>

#include "FeatureTypeColourPalette.h"
#include "GMTColourNames.h"
#include "Palette.h"

#include "file-io/CptReader.h"

#include "model/Gpgim.h"


boost::optional<GPlatesGui::Colour>
GPlatesGui::RegularPalette::get_colour(const Key& k) const 
{
	boost::optional<double> key = k.to_double();
	boost::optional<Colour> c = boost::none;
	if(key)
	{
		BOOST_FOREACH(const ColourSpectrum& spect, d_spectrums)
		{
			c = spect.get_colour_at(*key);
			if(c)
				return c;
		}
	}
	return c;
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::CategoricalPalette::get_colour(const Key& k) const
{
	const Key new_key = mapping_key(k);
	ColourMapType::const_iterator it = d_color_map.find(new_key);

	if(it != d_color_map.end()) 
		return it->second;

	return boost::none;
}

void
GPlatesGui::DefaultPlateIdPalette::build_map()
{
	d_color_map = boost::assign::map_list_of 
		(Palette::Key(static_cast<long>(0)), Colour::get_yellow())
		(Palette::Key(static_cast<long>(1)), Colour::get_aqua())
		(Palette::Key(static_cast<long>(2)), html_colour("seagreen"))
		(Palette::Key(static_cast<long>(3)), Colour::get_fuchsia())
		(Palette::Key(static_cast<long>(4)), html_colour("slategray"))
		(Palette::Key(static_cast<long>(5)), Colour::get_lime())
		(Palette::Key(static_cast<long>(6)), html_colour("indigo"))
		(Palette::Key(static_cast<long>(7)), Colour::get_red())
		(Palette::Key(static_cast<long>(8)), html_colour("orange"))
		(Palette::Key(static_cast<long>(9)), html_colour("lightsalmon"))
		(Palette::Key(static_cast<long>(10)), Colour::get_navy())
				// The following is needed to workaround c++11 bug in boost assign...
				.convert_to_container< std::map<const Palette::Key, Colour> >();
}

boost::optional<GPlatesGui::Colour>
GPlatesGui::RegionalPlateIdPalette::get_colour(const Key& k)  const 
{
	boost::optional<long> int_val = k.to_long();
	if(int_val)
	{
		int region = get_region_from_plate_id(static_cast<GPlatesModel::integer_plate_id_type>(*int_val));
		boost::optional<Colour> c = CategoricalPalette::get_colour(Palette::Key(static_cast<long>(region)));

		if(c)
		{
			HSVColour hsv = Colour::to_hsv(*c);
			// spread the v values from 0.6-1.0
			const double V_MIN = 0.6; // why 0.6? enough variation while not being too dark
			const double V_MAX = 1.0;
			const int V_STEPS = 13; // why 13? same rationale as for DEFAULT_COLOUR_ARRAY above
			hsv.v = (*int_val % V_STEPS) / static_cast<double>(V_STEPS) * (V_MAX - V_MIN) + V_MIN;
			return Colour::from_hsv(hsv);
		}
	}
	return boost::none;
}

void
GPlatesGui::RegionalPlateIdPalette::build_map()
{
	d_color_map = boost::assign::map_list_of 
		(Palette::Key(static_cast<long>(0)), Colour::get_olive())
		(Palette::Key(static_cast<long>(1)), Colour::get_red())
		(Palette::Key(static_cast<long>(2)), Colour::get_blue())
		(Palette::Key(static_cast<long>(3)), Colour::get_lime())
		(Palette::Key(static_cast<long>(4)), html_colour("mistyrose"))
		(Palette::Key(static_cast<long>(5)), Colour::get_aqua())
		(Palette::Key(static_cast<long>(6)), Colour::get_yellow())
		(Palette::Key(static_cast<long>(7)), html_colour("orange"))
		(Palette::Key(static_cast<long>(8)), Colour::get_purple())
		(Palette::Key(static_cast<long>(9)), html_colour("slategray"))
				// The following is needed to workaround c++11 bug in boost assign...
				.convert_to_container< std::map<const Palette::Key, Colour> >();
}


void
GPlatesGui::FeatureTypePalette::build_map()
{
	FeatureTypeColourPalette::non_null_ptr_type feature_type_colour_palette =
			FeatureTypeColourPalette::create();

	// Populate the colours map with FeatureTypes that we know about.
	GPlatesModel::Gpgim::feature_type_seq_type all_feature_types =
			GPlatesModel::Gpgim::instance().get_concrete_feature_types();
	BOOST_FOREACH(const GPlatesModel::FeatureType &feature_type, all_feature_types)
	{
		boost::optional<Colour> feature_type_colour = feature_type_colour_palette->get_colour(feature_type);
		if (feature_type_colour)
		{
			d_color_map.insert(
					std::make_pair(
						Palette::Key(feature_type.get_name().qstring()),
						feature_type_colour.get()));
		}
	}

	// Any feature type not in the GPGIM uses the default colour.
	d_default_color = Colour::get_navy();
}


const GPlatesGui::Palette*
GPlatesGui::default_age_palette(
		const double upper, 
		const double lower)
{
	std::vector<ColourSpectrum> spects;	
	spects.push_back(ColourSpectrum(Colour(1, 0, 1), Colour(0, 0, 1), 450, 360));
	spects.push_back(ColourSpectrum(Colour(0, 0, 1), Colour(0, 1, 1), 360, 270));
	spects.push_back(ColourSpectrum(Colour(0, 1, 1), Colour(0, 1, 0), 270, 180));
	spects.push_back(ColourSpectrum(Colour(0, 1, 0), Colour(1, 1, 0), 180, 90));
	spects.push_back(ColourSpectrum(Colour(1, 1, 0), Colour(1, 0, 0), 90, 0));

	static Palette* p = new RegularPalette(spects);
	p->set_BFN_colour(Colour(1, 0, 1), Colour(1, 0, 0), Colour(1, 0, 1));
	return p;
}


const GPlatesGui::Palette*
GPlatesGui::mono_age_palette(
		const double upper, 
		const double lower)
{
	std::vector<ColourSpectrum> spect;
	spect.push_back(ColourSpectrum(Colour::get_white(), Colour::get_black(), 450, 0));
	static Palette* p = new RegularPalette(spect);
	p->set_BFN_colour(Colour::get_black(), Colour::get_white(), Colour::get_black());
	return p;
}


const GPlatesGui::Palette*
GPlatesGui::default_palette()
{
	std::vector<ColourSpectrum> spect;
	spect.push_back(ColourSpectrum(Colour::get_blue(), Colour::get_red(), 1000, 0));
	static Palette* p = new RegularPalette(spect);
	p->set_BFN_colour(Colour::get_black(), Colour::get_white(), Colour::get_black());
	return p;
}


namespace
{
	using namespace GPlatesGui;
	using namespace GPlatesFileIO;

	boost::optional<Colour>
	make_color(const CptParser::ColourData& data)
	{
		switch(data.model)
		{
		case CptParser::RGB:
			return Colour(
					data.float_array[0],
					data.float_array[1],
					data.float_array[2]);

		case  CptParser::HSV:
			return Colour(
					QColor::fromHsvF(
							data.float_array[0],
							data.float_array[1],
							data.float_array[2]));

		case  CptParser::CMYK:
			return Colour(
					QColor::fromCmykF(
							data.float_array[0],
							data.float_array[1],
							data.float_array[2],
							data.float_array[3]));

		case  CptParser::RGB_HEX: 
			return Colour(QColor(data.str_data));

		case  CptParser::GREY:
			return Colour( data.float_array[0], data.float_array[0], data.float_array[0]);
		
		case  CptParser::GMT_NAME:
			return GMTColourNames::instance().get_colour(data.str_data.toStdString());
		
		case  CptParser::EMPTY:
		default:
			return boost::none;
		}
	}
}


GPlatesGui::CptPalette::CptPalette(const QString& file) :
	d_cate_palette(new CategoricalPalette()),
	d_regular_palette(new RegularPalette())
{
	using namespace GPlatesFileIO;

	//parse cpt file.
	CptParser parser(file);
	
	std::vector<CptParser::ColourData> bfn = parser.bfn_data();
	boost::optional<Colour> 
			b = make_color(bfn[0]),
			f = make_color(bfn[1]),
			n = make_color(bfn[2]);
	if(b && f && n)
	{
		set_BFN_colour(*b,*f,*n);
		d_cate_palette->set_BFN_colour(*b,*f,*n);
		d_regular_palette->set_BFN_colour(*b,*f,*n);
	}

	//process categorical entries
	const std::vector<CptParser::CategoricalEntry>& cate_entries = 
		parser.catagorical_entries();
	BOOST_FOREACH(const CptParser::CategoricalEntry& entry, cate_entries)
	{
		boost::optional<Colour> c = make_color(entry.data);
		if(c)
		{
			d_cate_palette->insert(Palette::Key(entry.key), *c);
		}
	}

	//process regular entries
	const std::vector<CptParser::RegularEntry>& regular_entries = 
		parser.regular_entries();
	BOOST_FOREACH(const CptParser::RegularEntry& entry, regular_entries)
	{
		boost::optional<Colour> 
				color_1 = make_color(entry.data1),
				color_2 = make_color(entry.data2);
		if(!color_1 && !color_2)
		{
			continue;
		}
		if(!color_1)
		{
			color_1 = color_2;
		}
		if(!color_2)
		{
			color_2 = color_1;
		}
		d_regular_palette->append(
				ColourSpectrum(*color_2, *color_1, entry.key2, entry.key1));
	}
}
