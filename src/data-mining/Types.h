/* $Id: DataOperatorTypes.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATESDATAMINING_REDUCERTYPES_H
#define GPLATESDATAMINING_REDUCERTYPES_H

#include <QString>

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"
#include "scribe/TranscribeEnumProtocol.h"


namespace GPlatesDataMining
{
	enum AttributeType
	{
		CO_REGISTRATION_GPML_ATTRIBUTE = 0,
		CO_REGISTRATION_SHAPEFILE_ATTRIBUTE,
		CO_REGISTRATION_RASTER_ATTRIBUTE,
		DISTANCE_ATTRIBUTE,
		PRESENCE_ATTRIBUTE,
		NUMBER_OF_PRESENCE_ATTRIBUTE,

		// NOTE: Any new values should also be added to @a transcribe.

		NUM_OF_Attribute_Type
	};

	inline
	QString
	to_string(
		AttributeType type)
	{
		const static char* names[] = 
		{
			"CO_REGISTRATION_GPML_ATTRIBUTE",
			"CO_REGISTRATION_SHAPEFILE_ATTRIBUTE",
			"CO_REGISTRATION_RASTER_ATTRIBUTE",
			"DISTANCE_ATTRIBUTE",
			"PRESENCE_ATTRIBUTE",
			"NUMBER_OF_PRESENCE_ATTRIBUTE"
		};
		if(static_cast<unsigned>(type)<static_cast<unsigned>(NUM_OF_Attribute_Type))
		{
			return names[static_cast<unsigned>(type)];
		}
		else
		{
			return "";
		}
		
	}

	enum ReducerType
	{
		REDUCER_MIN = 0,
		REDUCER_MAX,
		REDUCER_MEAN,
		REDUCER_STANDARD_DEVIATION,
		REDUCER_MEDIAN,
		REDUCER_LOOKUP,
		REDUCER_VOTE,
		REDUCER_WEIGHTED_MEAN,
		REDUCER_PERCENTILE,
		REDUCER_MIN_DISTANCE,
		REDUCER_PRESENCE,
		REDUCER_NUM_IN_ROI,

		// NOTE: Any new values should also be added to @a transcribe.

		NUM_OF_Reducer_Type
	};

	inline
	QString
	to_string(
			ReducerType type)
	{
		const static char* names[] = 
		{
			"REDUCER_MIN",
			"REDUCER_MAX",
			"REDUCER_MEAN",
			"REDUCER_STANDARD_DEVIATION",
			"REDUCER_MEDIAN",
			"REDUCER_LOOKUP",
			"REDUCER_VOTE",
			"REDUCER_WEIGHTED_MEAN",
			"REDUCER_PERCENTILE",
			"REDUCER_MIN_DISTANCE",
			"REDUCER_PRESENCE",
			"REDUCER_NUM_IN_ROI"
		};
		if(static_cast<unsigned>(type) < static_cast<unsigned>(NUM_OF_Reducer_Type))
		{
			return names[static_cast<unsigned>(type)];
		}
		else
		{
			return "";
		}

	}


	/**
	 * Transcribe for sessions/projects.
	 */
	inline
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			AttributeType &attribute_type,
			bool transcribed_construct_data)
	{
		// WARNING: Changing the string ids will break backward/forward compatibility.
		//          So don't change the string ids even if the enum name changes.
		static const GPlatesScribe::EnumValue enum_values[] =
		{
			GPlatesScribe::EnumValue("CO_REGISTRATION_GPML_ATTRIBUTE", CO_REGISTRATION_GPML_ATTRIBUTE),
			GPlatesScribe::EnumValue("CO_REGISTRATION_SHAPEFILE_ATTRIBUTE", CO_REGISTRATION_SHAPEFILE_ATTRIBUTE),
			GPlatesScribe::EnumValue("CO_REGISTRATION_RASTER_ATTRIBUTE", CO_REGISTRATION_RASTER_ATTRIBUTE),
			GPlatesScribe::EnumValue("DISTANCE_ATTRIBUTE", DISTANCE_ATTRIBUTE),
			GPlatesScribe::EnumValue("PRESENCE_ATTRIBUTE", PRESENCE_ATTRIBUTE),
			GPlatesScribe::EnumValue("NUMBER_OF_PRESENCE_ATTRIBUTE", NUMBER_OF_PRESENCE_ATTRIBUTE)
		};

		return GPlatesScribe::transcribe_enum_protocol(
				TRANSCRIBE_SOURCE,
				scribe,
				attribute_type,
				enum_values,
				enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
	}


	/**
	 * Transcribe for sessions/projects.
	 */
	inline
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			ReducerType &reducer_type,
			bool transcribed_construct_data)
	{
		// WARNING: Changing the string ids will break backward/forward compatibility.
		//          So don't change the string ids even if the enum name changes.
		static const GPlatesScribe::EnumValue enum_values[] =
		{
			GPlatesScribe::EnumValue("REDUCER_MIN", REDUCER_MIN),
			GPlatesScribe::EnumValue("REDUCER_MAX", REDUCER_MAX),
			GPlatesScribe::EnumValue("REDUCER_MEAN", REDUCER_MEAN),
			GPlatesScribe::EnumValue("REDUCER_STANDARD_DEVIATION", REDUCER_STANDARD_DEVIATION),
			GPlatesScribe::EnumValue("REDUCER_MEDIAN", REDUCER_MEDIAN),
			GPlatesScribe::EnumValue("REDUCER_LOOKUP", REDUCER_LOOKUP),
			GPlatesScribe::EnumValue("REDUCER_VOTE", REDUCER_VOTE),
			GPlatesScribe::EnumValue("REDUCER_WEIGHTED_MEAN", REDUCER_WEIGHTED_MEAN),
			GPlatesScribe::EnumValue("REDUCER_PERCENTILE", REDUCER_PERCENTILE),
			GPlatesScribe::EnumValue("REDUCER_MIN_DISTANCE", REDUCER_MIN_DISTANCE),
			GPlatesScribe::EnumValue("REDUCER_PRESENCE", REDUCER_PRESENCE),
			GPlatesScribe::EnumValue("REDUCER_NUM_IN_ROI", REDUCER_NUM_IN_ROI)
		};

		return GPlatesScribe::transcribe_enum_protocol(
				TRANSCRIBE_SOURCE,
				scribe,
				reducer_type,
				enum_values,
				enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
	}
}
#endif //GPLATESDATAMINING_REDUCERTYPES_H



