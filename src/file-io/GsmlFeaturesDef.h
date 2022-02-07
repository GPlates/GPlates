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

#ifndef GPLATES_FILEIO_GSMLFEATURESDEF_H
#define GPLATES_FILEIO_GSMLFEATURESDEF_H

#include "GsmlConst.h"
#include "GsmlFeatureHandlers.h"
#include "GsmlPropertyDef.h"
#include "GsmlPropertyHandlers.h"

namespace GPlatesFileIO
{
	/*
	* Add new feature type:
	* 1. Add an entry in AllFeatureTypes.
	* 2. Determine the properties contained in the new feature and initialize a properties array.
	* 3. If you want to use a new properties, check GsmlPropertyDef.h.
	*/
	struct FeatureInfo
	{
		const char* name;
		const PropertyInfo**properties;
		unsigned property_num;
	};

	//
	//define the properties that are contained in feature.
	//
	static const PropertyInfo* MappedFeatureProperties[] = 
	{
		&GeometryProperty,
		&ObservationMethodProperty,
		&GmlName,
		&GmlDesc,
		&GmlValidTime
	};

	static const PropertyInfo* GeologicUnitProperties[] = 
	{	
		&OccurrenceGeometryProperty,
		&GmlName,
		&GmlDesc,
		&GmlValidTime
	};

	static const PropertyInfo* UnclassifiedFeatureProperties[] = 
	{
		&GeometryProperty, 
		&GmlName, 
		&GmlDesc, 
		&GmlValidTime
	};

	static const PropertyInfo* RockUnitFeatureProperties[] = 
	{
		&GeometryProperty, 
		&GmlName, 
		&GmlDesc, 
		&GmlValidTime,
		&GpmlRockType,
		&GpmlRockMaxThick,
		&GpmlRockMinThick,
	};

	static const PropertyInfo* FossilCollectionFeatureProperties[] = 
	{
		&GeometryProperty, 
		&GmlName, 
		&GmlDesc, 
		&GmlValidTime,
		&GpmlFossilDiversity,
	};


	// define all features.
	static const FeatureInfo AllFeatureTypes[] = 
	{
		{
			"MappedFeature",
			MappedFeatureProperties,
			sizeof(MappedFeatureProperties)/sizeof(PropertyInfo*)
		},
		{
			"GeologicUnit",
			GeologicUnitProperties,
			sizeof(GeologicUnitProperties)/sizeof(PropertyInfo*)
		},
		{
			"UnclassifiedFeature",
			UnclassifiedFeatureProperties,
			sizeof(UnclassifiedFeatureProperties)/sizeof(PropertyInfo*)
		},
		{
			"RockUnit", // prefix for features like RockUnit_volcanic
			RockUnitFeatureProperties,
			sizeof(RockUnitFeatureProperties)/sizeof(PropertyInfo*)
		},
		{
			"FossilCollection", // prefix for features like FossilCollection_large
			FossilCollectionFeatureProperties,
			sizeof(FossilCollectionFeatureProperties)/sizeof(PropertyInfo*)
		}

	};	
}

#endif  // GPLATES_FILEIO_GSMLFEATURESDEF_H

