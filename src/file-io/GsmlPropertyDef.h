/* $Id: GsmlPropertyHandlers.h 11273 2011-04-07 03:28:55Z mchin $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2011-04-07 13:28:55 +1000 (Thu, 07 Apr 2011) $
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

#ifndef GPLATES_FILEIO_GSMLPROPERTYDEF_H
#define GPLATES_FILEIO_GSMLPROPERTYDEF_H

#include "GsmlConst.h"
#include "GsmlPropertyHandlers.h"

namespace GPlatesFileIO
{
	/*
	* Add new property support.
	* It is very straightforward. 
	* Determine name, query and handler and define the static const object.
	* Add these call backs to file-io/GsmlFeaturesDef.h
	*/
	struct PropertyInfo
	{
		const char* name;
		const char* query;
		void(GsmlPropertyHandlers::*handler)(QBuffer&);
	};

	//define properties here

	//geometry property
	static const PropertyInfo GeometryProperty = 
	{
		"GeometryProperty",
		"//gsml:shape",
		&GsmlPropertyHandlers::handle_geometry_property
	};

	//observation method property
	static const PropertyInfo ObservationMethodProperty = 
	{
		"ObservationMethod",
		"/gsml:MappedFeature/gsml:observationMethod",
		&GsmlPropertyHandlers::handle_observation_method
	};

	//occurrence geometry property
	static const PropertyInfo OccurrenceGeometryProperty = 
	{
		"OccurrenceGeometryProperty",
		"//gsml:occurrence",
		&GsmlPropertyHandlers::handle_occurrence_property
	};

	//gml name property 
	static const PropertyInfo GmlName = 
	{
		"GmlName",
		"//gml:name",
		&GsmlPropertyHandlers::handle_gml_name
	};

	//gml description property
	static const PropertyInfo GmlDesc = 
	{
		"GmlDesc",
		"//gml:description",
		&GsmlPropertyHandlers::handle_gml_desc
	};
	
	//gml validTime property
	static const PropertyInfo GmlValidTime = 
	{
		"GmlValidTime",
		"//gml:validTime",
		&GsmlPropertyHandlers::handle_gml_valid_time
	};

	//gml validTime property
	static const PropertyInfo GpmlValidTimeRange = 
	{
		"GpmlValidTimeRange",
		"//gpml:validTimeRange",
		&GsmlPropertyHandlers::handle_gpml_valid_time_range
	};

	
	// Props for RockUnit_* types from Macrosrtat

	static const PropertyInfo GpmlRockType = 
	{
		"GpmlRockType",
		"//gpml:rock_type",
		&GsmlPropertyHandlers::handle_gpml_rock_type
	};

	static const PropertyInfo GpmlRockMaxThick = 
	{
		"GpmlRockMaxThick",
		"//gpml:rock_max_thick",
		&GsmlPropertyHandlers::handle_gpml_rock_max_thick
	};

	static const PropertyInfo GpmlRockMinThick = 
	{
		"GpmlRockMinThick",
		"//gpml:rock_min_thick",
		&GsmlPropertyHandlers::handle_gpml_rock_min_thick
	};

	// Props for FossilCollection_* types from Macrosrtat

	static const PropertyInfo GpmlFossilDiversity = 
	{
		"GpmlFossilDiversity",
		"//gpml:fossil_diversity",
		&GsmlPropertyHandlers::handle_gpml_fossil_diversity
	};


}

#endif  // GPLATES_FILEIO_GSMLPROPERTYDEF_H

