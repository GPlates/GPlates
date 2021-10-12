/* $Id$ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date$
* 
* Copyright (C) 2009 Geological Survey of Norway
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

#include <QMessageBox>

#include "ShapefileUtils.h"
#include "ShapefileXmlWriter.h"


QString
GPlatesFileIO::ShapefileUtils::make_shapefile_xml_filename(
		const QFileInfo &file_info)
{

	QString shapefile_xml_filename = file_info.absoluteFilePath() + ".gplates.xml";

	return shapefile_xml_filename;
}


void
GPlatesFileIO::ShapefileUtils::save_attribute_map_as_xml_file(
		const QString &filename,
		const QMap<QString,QString> &model_to_attribute_map)
{
	GPlatesFileIO::ShapefileXmlWriter writer;
	if (!writer.write_file(filename,model_to_attribute_map))
	{
		QMessageBox::warning(0,QObject::tr("ShapefileXmlWriter"),
			QObject::tr("Cannot write to file %1.")
			.arg(filename));
	};
}

const GPlatesFileIO::ShapefileUtils::feature_map_type &
GPlatesFileIO::ShapefileUtils::build_feature_map()
{

	// The data for the following map has been taken from 
	// the "build_feature_map_type" map in PlatesLineFormatReader.cc 
	
	//
	// FIXME: we should get this information from a common source, rather than having two independent sources.  
	static feature_map_type map;
	map["AR"] = "AseismicRidge";
	map["BA"] = "Bathymetry";
	map["BS"] = "Basin";
	map["CB"] = "PassiveContinentalBoundary";
	map["CF"] = "ContinentalFragment";
	map["CM"] = "PassiveConinentalBoundary";
	map["CO"] = "PassiveContinentalBoundary";
	map["CR"] = "Craton";
	map["CS"] = "Coastline";
	map["EC"] = "ExtendedContinentalCrust";
	map["FT"] = "Fault";
	map["FZ"] = "FractureZone";
	map["GR"] = "OldPlatesGridMark";
	map["GV"] = "Gravimetry";
	map["HF"] = "HeatFlow";
	map["HS"] = "HotSpot";
	map["HT"] = "HotSpotTrail";
	map["IA"] = "IslandArc";
	map["IC"] = "Isochron";
	map["IM"] = "Isochron";
	map["IP"] = "SedimentThickness";
	map["IR"] = "IslandArc";
	map["IS"] = "UnclassifiedFeature";
	map["LI"] = "GeologicalLineation";
	map["MA"] = "Magnetics";
	map["NF"] = "gpmlFault";
	map["OB"] = "OrogenicBelt";
	map["OP"] = "BasicRockUnit";
	map["OR"] = "OrogenicBelt";
	map["PB"] = "InferredPaleoBoundary";
	map["PA"] = "MagneticAnomalyIdentification";
	map["PC"] = "MagneticAnomalyIdentification";
	map["PM"] = "MagneticAnomalyIdentification";
	map["RA"] = "IslandArc";
	map["RF"] = "Fault";
	map["RI"] = "MidOceanRidge";
	map["SM"] = "Seamount";
	map["SS"] = "Fault";
	map["SU"] = "Suture";
	map["TB"] = "TerraneBoundary";
	map["TC"] = "TransitionalCrust";
	map["TF"] = "Transform";
	map["TH"] = "Fault";
	map["TO"] = "Topography";
	map["TR"] = "SubductionZone";
	map["UN"] = "UnclassifiedFeature";
	map["VO"] = "Volcano";
	map["VP"] = "LargeIgneousProvince";
	map["XR"] = "MidOceanRidge";
	map["XT"] = "SubductionZone";

	return map;
}

