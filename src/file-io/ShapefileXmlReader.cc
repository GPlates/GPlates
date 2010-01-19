/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#include <iostream>

#include <QFile>
#include <QMap>

#include "qt-widgets/ShapefilePropertyMapper.h"

#include "ShapefileXmlReader.h"

GPlatesFileIO::ShapefileXmlReader::ShapefileXmlReader():
	d_map(0)
{

}

bool
GPlatesFileIO::ShapefileXmlReader::read_file(
	QString &filename,
	QMap<QString,QString> *attribute_map)
{
	d_map = attribute_map;

	QFile file(filename);
	if (!file.open(QFile::ReadOnly | QFile::Text)){
		return false;
	}

	setDevice(&file);

    while (!atEnd()) {
         readNext();

         if (isStartElement()) {
             if (name() == "GPlatesShapefileMap" && attributes().value("version") == "1")
                 read_xml();
             else
                 raiseError(QObject::tr("The file is not a GPlatesShapefileMap version 1 file."));
         }
     }

     return !error();

}

void
GPlatesFileIO::ShapefileXmlReader::read_xml()
{
    Q_ASSERT(isStartElement() && name() == "GPlatesShapefileMap");

     while (!atEnd()) {
         readNext();

		 if (isEndElement()){
             break;
		 }

		 if (isStartElement())
		 {
			 read_map_item();
		 }

     }
}


void 
GPlatesFileIO::ShapefileXmlReader::read_map_item()
{
	QString key,value;
	key = name().toString();
	value = readElementText();
	d_map->insert(key,value);
}

