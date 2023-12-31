/* $Id$ */

/**
 * \file 
 * Contains the template class CptReader.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.cc")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "CptReader.cc")
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

#include "CptReader.h"

#include "global/CompilerWarnings.h"

#include "gui/GMTColourNames.h"

// On some versions of g++ with some versions of Qt, it's not liking at() and
// operator[] in QStringList.
DISABLE_GCC_WARNING("-Wstrict-overflow")


using boost::tuples::get;


namespace GPlatesFileIO
{
	namespace CptReaderInternals
	{
		template<>
		boost::tuples::null_type
		parse_components<boost::tuples::null_type>(
				const QStringList &tokens,
				unsigned int starting_index)
		{
			return boost::tuples::null_type();
		}


		bool
		TryProcessTokensImpl<RegularCptFileFormat>::operator()(
				const QStringList &tokens,
				ParserState<RegularCptFileFormat> &parser_state)
		{
			// Note the use of the short-circuiting mechanism.
			return try_process_regular_cpt_rgb_or_hsv_or_cmyk_colour_slice(tokens, parser_state) ||
					try_process_regular_cpt_colour_slice<RegularCptSliceColourSpecification>(tokens, parser_state) ||
					try_process_rgb_or_hsv_or_cmyk_bfn<RegularCptFileFormat>(tokens, parser_state) ||

					// R/G/B and H-S-V and C/M/Y/K don't depend on COLOR_MODEL (like "R G B" and "H S V" and "C M Y K" do)...
					try_process_bfn<RegularCptFileFormat, RGBTripletColourSpecification>(tokens, parser_state) ||
					try_process_bfn<RegularCptFileFormat, HSVTripletColourSpecification>(tokens, parser_state) ||
					try_process_bfn<RegularCptFileFormat, CMYKTripletColourSpecification>(tokens, parser_state) ||

					try_process_bfn<RegularCptFileFormat, GMTNameColourSpecification>(tokens, parser_state) ||
					try_process_bfn<RegularCptFileFormat, CMYKColourSpecification>(tokens, parser_state) ||
					try_process_bfn<RegularCptFileFormat, GreyColourSpecification>(tokens, parser_state) ||
					try_process_bfn<RegularCptFileFormat, InvisibleColourSpecification>(tokens, parser_state) ||
					try_process_bfn<RegularCptFileFormat, PatternFillColourSpecification>(tokens, parser_state);
		}
	}
}


bool
GPlatesFileIO::CptReaderInternals::in_rgb_range(
		double value)
{
	return 0.0 <= value && value <= 255.0;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_rgb_colour(
		double r, 
		double g, 
		double b)
{
	if (in_rgb_range(r) && in_rgb_range(g) && in_rgb_range(b))
	{
		return GPlatesGui::Colour(
				static_cast<GLfloat>(r / 255.0),
				static_cast<GLfloat>(g / 255.0),
				static_cast<GLfloat>(b / 255.0));
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::in_h_range(
		double value)
{
	return 0.0 <= value && value <= 360.0;
}


bool
GPlatesFileIO::CptReaderInternals::in_sv_range(
		double value)
{
	return 0.0 <= value && value <= 1.0;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_hsv_colour(
		double h, 
		double s, 
		double v)
{
	if (in_h_range(h) && in_sv_range(s) && in_sv_range(v))
	{
		return GPlatesGui::Colour::from_hsv(GPlatesGui::HSVColour(
				h / 360.0, s, v));
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::in_cmyk_range(
		double value)
{
	return 0.0 <= value && value <= 100.0;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_cmyk_colour(
		double c, 
		double m, 
		double y, 
		double k)
{
	if (in_cmyk_range(c) && in_cmyk_range(m) && in_cmyk_range(y) && in_cmyk_range(k))
	{
		return GPlatesGui::Colour::from_cmyk(
				GPlatesGui::CMYKColour(
					c / 100.0,
					m / 100.0,
					y / 100.0,
					k / 100.0));
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::in_grey_range(
		double value)
{
	return 0.0 <= value && value <= 255.0;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_grey_colour(
		double value)
{
	if (in_grey_range(value))
	{
		GLfloat f = static_cast<GLfloat>(value / 255.0);
		return GPlatesGui::Colour(f, f, f);
	}
	else
	{
		throw BadComponentsException();
	}
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_gmt_colour(
		const QString &name)
{
	boost::optional<GPlatesGui::Colour> result =
		GPlatesGui::GMTColourNames::instance().get_colour(name.toLower().toStdString());
	if (result)
	{
		return *result;
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::try_process_regular_cpt_rgb_or_hsv_or_cmyk_colour_slice(
		const QStringList &tokens,
		ParserState<RegularCptFileFormat> &parser_state)
{
	// Use COLOR_MODEL to determine whether to read as "R G B", "H S V" or "C M Y K".
	// In each case the lower *and* upper colours are read using the same colour specification
	// (eg, RGB reads lower and upper colours as "R G B").
	switch (parser_state.colour_model)
	{
	case GPlatesGui::ColourModel::RGB:
		return try_process_regular_cpt_colour_slice<RGBColourSpecification>(tokens, parser_state);

	case GPlatesGui::ColourModel::HSV:
		return try_process_regular_cpt_colour_slice<HSVColourSpecification>(tokens, parser_state);

	case GPlatesGui::ColourModel::CMYK:
	default:
		return try_process_regular_cpt_colour_slice<CMYKColourSpecification>(tokens, parser_state);
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesFileIO::CptReaderInternals::parse_categorical_fill(
		const QString &token)
{
	if (token.contains('/'))
	{
		// R/G/B or C/M/Y/K triplet.
		QStringList subtokens = token.split('/');
		if (subtokens.size() == 3)
		{
			// Convert the R/G/B colour.
			return convert_tokens<RGBColourSpecification>(subtokens);
		}
		else if (subtokens.size() == 4)
		{
			// Convert the C/M/Y/K colour.
			return convert_tokens<CMYKColourSpecification>(subtokens);
		}
		else
		{
			throw BadTokenException();
		}
	}
	else if (token.startsWith('#'))
	{
		// Hexadecimal RGB code.
		if (token.length() != 7)
		{
			throw BadTokenException();
		}

		// It should be of the form #xxyyzz.
		QStringList subtokens;
		subtokens.append(token.mid(1, 2));
		subtokens.append(token.mid(3, 2));
		subtokens.append(token.mid(5, 2));

		// Convert the colour.
		return convert_tokens<HexRGBColourSpecification>(subtokens);
	}
	else if (token.contains('-'))
	{
		// H-S-V triplet.
		QStringList subtokens = token.split('-');
		if (subtokens.size() != 3)
		{
			throw BadTokenException();
		}

		// Convert the colour.
		return convert_tokens<HSVColourSpecification>(subtokens);
	}
	else
	{
		// Try parsing it as a single number.
		try
		{
			double grey = parse_token<double>(token);
			return make_grey_colour(grey);
		}
		catch (...)
		{
		}
		
		// See whether it's a GMT colour name.
		try
		{
			return make_gmt_colour(token);
		}
		catch (...)
		{
		}

		// If it starts with a p, let's assume it's a pattern fill.
		if (is_pattern_fill_specification(token))
		{
			throw PatternFillEncounteredException();
		}

		// Don't know what we were given...
		throw BadTokenException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::is_pattern_fill_specification(
		const QString &token)
{
	// For now, we just say that it's a pattern fill if it starts with 'p'.
	// There's obviously more to it, but since we don't support pattern fills,
	// this test is sufficient for now.
	return token.startsWith('p');
}


/************************************************
* New implementation of cpt reader.
*************************************************/

GPlatesFileIO::CptParser::CptParser(const QString& file_path) :
	d_default_model(RGB)
{
	QFile file(file_path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("Cannot open cpt file: " + file_path));
	}

	QTextStream in(&file);
	while (!in.atEnd()) 
	{
		//Remove the white spaces from the start and the end.
		//Keep the white spaces inside the line 
		//because they could be a part of the "keys" enclosed in quotation marks.
		//For example 'Marine   abyssal' 150/100/230,
		//the white spaces enclosed by single quotes should be preserved.
		QString line = in.readLine().trimmed();
		if(line.length() > 0)
		{
			try
			{
				process_line(line);
			}
			catch (GPlatesGlobal::LogException& e)
			{
				std::ostringstream ostr;
				e.write(ostr);
				qWarning() << ostr.str().c_str();
			}
		}
	}
}


QStringList
GPlatesFileIO::CptParser::split_into_tokens(
		const QString& line)
{
	QStringList tokens;
	QString token;
	bool inside_quotes = false, inside_token = false;
	for(QString::const_iterator it=line.begin(); it!=line.end(); it++)
	{
		if((*it).isSpace())
		{
			if(inside_quotes)
			{
				token.append(*it);//preserve the spaces inside quotation marks.
			}
			else if(!inside_token)
			{
				continue;//ignore the leading spaces and the spaces between tokens
			}
			else
			{
				//qDebug() << "token: " << token;
				tokens.append(token);//the current token ends.
				token = "";
				inside_token = false;
			}
		}
		else if((*it)=='\"' || (*it)=='\'')
		{
			if(!inside_quotes)
			{
				if(inside_token)
				{
					token.append(*it);//the quote is a part of a token
				}
				else
				{
					inside_quotes = true; // the begin of quotation marks
				}
			}else
			{
				//the end of quotation marks
				//qDebug() << "token: " << token;
				tokens.append(token);
				token = "";
				inside_quotes = false;
			}
		}
		else if((*it)==';')
		{
			if(inside_token || inside_quotes)
			{
				token.append(*it);
			}
			else
			{
				for(;it!=line.end(); it++) //the rest of the line is "label"
				{
					token.append(*it);
				}
				//qDebug() << "token: " << token;
				tokens.append(token);
				token = "";
				break;
			}
		}
		else //other characters
		{
			if(inside_token || inside_quotes)
			{
				token.append(*it); //normal characters inside token
			}else
			{
				//start a new token
				if(!token.isEmpty())
				{
					//qDebug() << "token: " << token;
					tokens.append(token);
					token = "";
				}
				token.append(*it);
				inside_token = true;
			}
		}
	}
	if(!token.isEmpty())
	{
		//qDebug() << "token: " << token;
		tokens.append(token);
	}
	return tokens;
}


void
GPlatesFileIO::CptParser::process_line(
		const QString& line)
{
	//qDebug() << "Processing: " << line;
	//we could use finite state machine here,
	//however, since cpt file is so simple that FSM is a little bit overkilled...

	if(line.startsWith('#'))
	{
		process_comment(line);
		return;
	}

	QStringList tokens = split_into_tokens(line);
	
	if(tokens.size() < 2) // no enough tokens
	{
		qWarning() << "Invalid line in cpt file: [" << line << "]";
		return;
	}

	if(tokens.size() <= 3) // only could be categorical line
	{
		return process_categorical_line(tokens);
	}

	
	QString first = tokens.at(0);
	if(first == "B")
		process_bfn(tokens,d_back);
	else if(first == "F")
		process_bfn(tokens,d_fore);
	else if(first == "N")
		process_bfn(tokens,d_nan);
	else
		process_regular_line(tokens);
			
	return;
}


void
GPlatesFileIO::CptParser::process_regular_line(
		QStringList& tokens)
{
	RegularEntry entry;

	//strip off annotation fields
	bool annotation_flag = false;
	for(int i = 0; i < tokens.size(); )
	{
		//annotation begins with ";", until the end of line.
		if(!annotation_flag && tokens.at(i).simplified().startsWith(";"))
		{
			annotation_flag = true;
		}
		if(annotation_flag)
		{
			entry.label.append(tokens.takeAt(i) + " ");
		}
		else
		{
			++i;
		}
	}

	QString last_token = tokens.last();
	if(last_token == "L" || last_token == "U" || last_token == "B")
	{
		entry.label_opt = last_token;
		tokens.removeLast();
	}

	//each of the following steps will remove the token(s) which has(have) been parsed.
	entry.key1 = tokens.takeFirst().toFloat();
	entry.data1 = read_first_colour_data(tokens); 
	entry.key2 = tokens.takeFirst().toFloat();
	entry.data2 = read_second_colour_data(tokens);
	
	d_regular_entries.push_back(entry);
}


GPlatesFileIO::CptParser::ColourData
GPlatesFileIO::CptParser::read_first_colour_data(
		QStringList& tokens)
{
	ColourData data;
	QString token = tokens.at(0);
	if(token == "-")
	{
		data.model = EMPTY;
		tokens.removeFirst();
		return data;
	}

	int len = tokens.size();
	if(len == 3) // the length indicates that the first color is empty, color name or gmt fill.
	{
		QString first_token = tokens.takeAt(0);
		if(is_gmt_color_name(first_token))
		{
			data.model = GMT_NAME;
			data.str_data = first_token;
		}
		else
		{
			data = parse_gmt_fill(first_token);
		}
	}
	else if(7 == len || 5 == len)// the length indicates rgb or hsv
	{
		data.model = d_default_model;
		if(d_default_model == RGB)
		{
			parse_rbg_data(tokens,data);
		}
		else if(d_default_model == HSV)
		{
			parse_hsv_data(tokens,data);
		}
	}
	else if(9 == len || 6 ==len) // the length indicates cmyk
	{
		data.model = CMYK;
		parse_cmyk_data(tokens,data);
	}
	else
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("Failed to parse regular cpt line."));
	}
	return data;
}


GPlatesFileIO::CptParser::ColourData
GPlatesFileIO::CptParser::read_second_colour_data(
		QStringList& tokens)
{
	ColourData data;
	int len = tokens.size();
	if(len == 1) // the length indicates empty, color name or gmt fill.
	{
		QString first_token = tokens.takeAt(0);
		if(first_token == "-")
		{
			data.model = EMPTY;
		}
		else if(is_gmt_color_name(first_token))
		{
			data.model = GMT_NAME;
			data.str_data = first_token;
		}
		else
		{
			data = parse_gmt_fill(first_token);
		}
	}
	else if(3 == len)// the length indicates rgb or hsv
	{
		data.model = d_default_model;
		if(d_default_model == RGB)
		{
			parse_rbg_data(tokens,data);
		}
		else if(d_default_model == HSV)
		{
			parse_hsv_data(tokens,data);
		}
	}
	else if(4 == len) // the length indicates cmyk
	{
		data.model = CMYK;
		parse_cmyk_data(tokens,data);
	}
	else
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("Failed to parse regular cpt line."));
	}
	return data;
}


void
GPlatesFileIO::CptParser::process_categorical_line(
		QStringList& tokens)
{
	if(tokens.size() < 2)
		return;

	CategoricalEntry entry;
	entry.key = tokens.at(0);
	entry.data = parse_gmt_fill(tokens.at(1));
	if(tokens.size() == 3)
	{
		entry.label = tokens.at(2);
	}
	d_categorical_entries.push_back(entry);
}

void
GPlatesFileIO::CptParser::process_comment(
		const QString& line)
{
	//remove all spaces
	QString str = line.toUpper();
	int i = 0;
	while( i<str.length() )
	{
		if(str.at(i).isSpace())
			str = str.remove(i,1);
		else
			i++;
	}

	static const QRegExp hsv_regex("COLOR_MODEL\\s*=\\s*\\+?HSV");			
	if(-1 != hsv_regex.indexIn(line))
	{
			d_default_model = HSV;
	}
}

GPlatesFileIO::CptParser::ColourData 
GPlatesFileIO::CptParser::parse_gmt_fill(
		const QString& token)
{
	QRegExp rx("p\\d+/");
	if(rx.indexIn(token) != -1)
	{
		//We don't support "fill pattern" yet. 
		//It looks something like "p200/16". 
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("Do not support pattern yet: " + token));
	}

	ColourData data;

	//TODO: this function needs to be rewritten.
	//try QRegExp
	if (token.contains('/'))
	{
		// R/G/B triplet.
		QStringList subtokens = token.split('/');
		if (subtokens.size() != 3)
		{
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					("Failed to parse fill token: " + token));
		}
		data.model = RGB;
		parse_rbg_data(subtokens, data);
		return data;
	}
	else if (token.startsWith('#'))
	{
		if (token.length() != 7)
		{
			throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				("Failed to parse fill token: " + token));
		}
		// Hexadecimal RGB code.
		data.model = RGB_HEX;
		data.str_data = token;
		return data;
	}
	else if (token.contains('-'))
	{
		// H-S-V triplet.
		QStringList subtokens = token.split('-');
		if (subtokens.size() != 3)
		{
			throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				("Failed to parse fill token: " + token));
		}
		data.model = HSV;
		parse_hsv_data(subtokens, data);
		return data;
	}
	else
	{
		// Try parsing it as a single integer.
		bool ok;
		float f = token.toFloat(&ok);
		if(ok)
		{
			data.model = GREY;
			data.float_array.push_back(f);
			return data;
		}
		else
		{
			// See whether it's a GMT colour name.
			if(is_gmt_color_name(token))
			{
				data.model = GMT_NAME;
				data.str_data = token;
				return data;
			}
		}
	}
	return data;
}

bool
GPlatesFileIO::CptParser::is_gmt_color_name(
		const QString& name)
{
	//TODO::
	//move GMTColourNames to somewhere else
	//file-io should not dependent on gui.
	const std::map<std::string, std::vector<int> >& name_map =  
			GPlatesGui::GMTColourNames::instance().get_name_map();

	if(name_map.find(name.toStdString()) != name_map.end())
		return true;
	else
		return false;
}

void
GPlatesFileIO::CptParser::parse_rbg_data(
		QStringList& tokens, 
		ColourData& data)
{
	bool ok_1 = false, ok_2 = false, ok_3 = false;
	float f_1 = tokens.takeFirst().toFloat(&ok_1);
	float f_2 = tokens.takeFirst().toFloat(&ok_2);
	float f_3 = tokens.takeFirst().toFloat(&ok_3);
	
	if(!ok_1 || !ok_2 || !ok_3 || !is_valid_rgb(f_1,f_2,f_3))
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("invalid RBG data."));
	}
	data.float_array.push_back(f_1/255.0);
	data.float_array.push_back(f_2/255.0);
	data.float_array.push_back(f_3/255.0);
}


void
GPlatesFileIO::CptParser::parse_hsv_data(
		QStringList& tokens, 
		ColourData& data)
{
	bool ok_1 = false, ok_2 = false, ok_3 = false;
	float f_1 = tokens.takeFirst().toFloat(&ok_1);
	float f_2 = tokens.takeFirst().toFloat(&ok_2);
	float f_3 = tokens.takeFirst().toFloat(&ok_3);

	if(!ok_1 || !ok_2 || !ok_3 || !is_valid_hsv(f_1,f_2,f_3))
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("invalid HSV data."));
	}
	f_1 /= 360.0; 

	data.float_array.push_back(f_1);
	data.float_array.push_back(f_2);
	data.float_array.push_back(f_3);
}


void
GPlatesFileIO::CptParser::parse_cmyk_data(
		QStringList& tokens, 
		ColourData& data)
{
	bool ok_1 = false, ok_2 = false, ok_3 = false, ok_4 = false;
	float f_1 = tokens.takeFirst().toFloat(&ok_1);
	float f_2 = tokens.takeFirst().toFloat(&ok_2);
	float f_3 = tokens.takeFirst().toFloat(&ok_3);
	float f_4 = tokens.takeFirst().toFloat(&ok_4);
	if(!ok_1 || !ok_2 || !ok_3 || !ok_4 || !is_valid_cmyk(f_1,f_2,f_3,f_4))
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("invalid CMYK values"));
	}
	data.float_array.push_back(f_1/100.0);
	data.float_array.push_back(f_2/100.0);
	data.float_array.push_back(f_3/100.0);
	data.float_array.push_back(f_4/100.0);
}

void
GPlatesFileIO::CptParser::process_bfn(
		QStringList& tokens,
		ColourData& data)
{
	tokens.removeFirst();
	if(tokens.size() == 3)
	{
		if(d_default_model == RGB)
		{
			parse_rbg_data(tokens,data);
		}
		else if(d_default_model == HSV)
		{
			parse_hsv_data(tokens,data);
		}
	}
	else if(tokens.size() == 4)
	{
		parse_cmyk_data(tokens,data);
	}
	else
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			("Invalid bfn line."));
	}
	data.model = d_default_model;
}





