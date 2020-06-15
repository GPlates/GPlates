/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2013 Geological Survey of Norway
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
#include <QFile>

#include "global/Version.h"

#include "ShapefileXmlWriter.h"

GPlatesFileIO::ShapefileXmlWriter::ShapefileXmlWriter()
{
	setAutoFormatting(true);
}

bool
GPlatesFileIO::ShapefileXmlWriter::write_file(
	const QString &filename,
	const QMap< QString,QString> &map)
{
	QFile file(filename);
	if (!file.open(QFile::WriteOnly | QFile::Text)){
		return false;
	}

	setDevice(&file);

	QString comment_string;
	comment_string.append("\n"
							"***************************************************************************** \n"
							"This file was generated by ");
	comment_string.append(GPlatesGlobal::Version::get_GPlates_version());
	comment_string.append(" (http://www.gplates.org) \n");
	comment_string.append("\n"
						"This file is used to record the mapping between shapefile attributes \n"
						"and GPlates model properties for a specific shapefile. \n"
						"\n"
						"The file consists of lines of the form <TagName>ShapefileAttribute</TagName> \n"
						"where TagName should be one of the recognised tabs listed below. \n"
						"\n"
						"For example, a line of the form: \n"
						"\n"
						"<ReconstructionPlateId>PLATEID</ReconstructionPlateId> \n"
						"\n"
						"indicates that the shapefile attribute with the name PLATEID will be used to \n"
						"generate GPlates gpml:ReconstructionPlateId properties. \n"
						"\n"
						"The following tags are recognised: \n"
						"\n"
						"Tag name ................ Description \n"
						"=====================.... ================================================================ \n"
						"ReconstructionPlateId ... The plate id used in reconstruction (gpml:reconstructionPlateId). \n"
						"FeatureType ............. The type of feature. \n"
						"FeatureId................ The (unique) identifier of the feature. \n"
						"Begin ................... The age of appearance (gml:begin part of gml:validTime). \n"
						"End ..................... The age of disappearance (gml:end part of gml:validTime). \n"
						"Name .................... The name of the feature (gml:name). \n"
						"Description ............. A description of the feature (gml:description). \n"
						"ConjugatePlateId......... The conjugate plate id. \n"
						"ReconstructionMethod......The type of reconstruction method used. \n"
						"LeftPlate.................The left plate id used for half-stage reconstructions. \n"
						"RightPlate................The right plate id used for half-stage reconstructions. \n"
						"SpreadingAsymmetry........The spreading asymmetry used in half-stage reconstructions. \n"
						"GeometryImportTime........The age the feature was digitized at (used to reconstruct using half-stages or topologies). \n"
						"\n"
						"On loading a shapefile, GPlates will use the mapping stored in this file, if it exists. \n"
						"If no such file exists, GPlates will generate a file according to the mapping \n"
						"selected by the user during file loading. \n"
						"\n"
						"The user can edit the mapping within GPlates by\n"
						"selecting File->ManageFeatureCollections (or Ctrl+M) and clicking the shapefile's \n"
						"Edit Configuration icon, or by editing this file manually. \n"
						"If the mapping is changed from within GPlates, this file will be overwritten. \n"
						"***************************************************************************** \n\n"); 


	writeStartDocument();

	writeComment(comment_string);

	writeStartElement("GPlatesShapefileMap");
	writeAttribute("version","1");

	QMap< QString,QString>::const_iterator it = map.constBegin();
	QMap< QString,QString>::const_iterator it_end = map.constEnd();

	for(; it != it_end; ++it)
	{
		write_map_item(it.key(),it.value());
	}

	writeEndDocument();
	return true;
}

void
GPlatesFileIO::ShapefileXmlWriter::write_map_item(
	QString key,
	QString value)
{
	writeStartElement(key);
	writeCharacters(value);
	writeEndElement();
}

