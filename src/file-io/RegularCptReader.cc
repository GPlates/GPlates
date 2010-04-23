/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.cc")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "RegularCptReader.cc")
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

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "RegularCptReader.h"

#include "gui/GenericContinuousColourPalette.h"


namespace
{
#if 0
	/**
	 * Parse a line from the cpt file defining a colour ramp.
	 * 
	 * The line should have 8 items:
	 *  d1    float, lower data value
	 *  r1	  int in range 0-255, red
	 *  g1    int in range 0-255, green   
	 *  b1	  int in range 0-255, blue       
	 *  d2    float, upper data value
	 *  r2	  int in range 0-255, red
	 *  g2    int in range 0-255, green   
	 *  b2	  int in range 0-255, blue  	                                                        
	 */
	void
	process_ramp_line(
		const QStringList &list,
		GPlatesGui::ColourPaletteTable &cpt)
	{
		// For conversions.
		bool ok = false;
		
		double data1,data2;
		int r1,r2,g1,g2,b1,b2;
		
		// First data value
		data1 = list.at(0).toFloat(&ok);
		if (!ok){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// First r value
		r1 = list.at(1).toInt(&ok);
		if (!ok || !((r1>=0)&&(r1<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// First g value
		g1 = list.at(2).toInt(&ok);
		if (!ok || !((g1>=0)&&(g1<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// First b value
		b1 = list.at(3).toInt(&ok);
		if (!ok || !((b1>=0)&&(b1<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}			
		// Second data value	
		data2 = list.at(4).toFloat(&ok);
		if (!ok){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// Second r value
		r2 = list.at(5).toInt(&ok);
		if (!ok || !((r2>=0)&&(r2<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// Second g value
		g2 = list.at(6).toInt(&ok);
		if (!ok || !((g2>=0)&&(g2<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// Second b value
		b2 = list.at(7).toInt(&ok);
		if (!ok || !((b2>=0)&&(b2<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}			
		GPlatesGui::ColourRamp ramp(data1,QColor(r1,g1,b1),data2,QColor(r2,g2,b2));
		cpt.ramps().push_back(ramp);
	}
	
	/**
	 *  Parse the content of a "fbn" line, i.e. a line containing one of 
	 *  F R G B  (foreground rgb value)
	 *  B R G B  (background rgb value)      
	 *  N R G B  (nan rgb value)           
	 * 
	 * The first item should be a single character, and should be one of "F", "B" or "N"
	 * The second, third and fourth terms should be integers in the range 0-255, representing
	 * red, green and blue respectively.                                                     
	 */
	void
	process_fbn_line(
		const QStringList &list,
		GPlatesGui::ColourPaletteTable &cpt)
	{
		int r,g,b;
		bool ok = false;
		// First r value
		r = list.at(1).toInt(&ok);
		if (!ok || !((r>=0)&&(r<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// First g value
		g = list.at(2).toInt(&ok);
		if (!ok || !((g>=0)&&(g<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}
		// First b value
		b = list.at(3).toInt(&ok);
		if (!ok || !((b>=0)&&(b<=255))){
			throw (GPlatesFileIO::ErrorReadingCptFileException());
		}			
		
		QString letter = list.at(0);
		if (letter == "B")
		{
			cpt.set_background_rgb(QColor(r,g,b));
		}
		else if (letter == "F")
		{
			cpt.set_foreground_rgb(QColor(r,g,b));
		}
		else if (letter == "N")
		{			
			cpt.set_nan_rgb(QColor(r,g,b));
		}
		else
		{
			throw(GPlatesFileIO::ErrorReadingCptFileException());
		}	
	}
#endif

	void
	process_line(
			QString line,
			GPlatesGui::GenericContinuousColourPalette &palette,
			bool &hsv)
	{
		QStringList list = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);		
		
#if 0
		if (list.size() == 8)
		{
			process_ramp_line(list,cpt);
		}
		else if (list.size() == 4)
		{
			process_fbn_line(list,cpt);
		}
#endif
	}
}

void
GPlatesFileIO::RegularCptReader::read_file(
		QString filename,
		GPlatesGui::GenericContinuousColourPalette &palette)
{
	QFile qfile(filename);
	
	if (!qfile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return;
	}
	
	QTextStream text_stream(&qfile);

	bool hsv = false;
	
	while (!text_stream.atEnd())
	{
		QString line = text_stream.readLine();
		process_line(line, palette, hsv);
	}
}

