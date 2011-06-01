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

namespace GPlatesDataMining
{
	enum AttributeType
	{
		CO_REGISTRATION_ATTRIBUTE,
		DISTANCE_ATTRIBUTE,
		PRESENCE_ATTRIBUTE,
		NUMBER_OF_PRESENCE_ATTRIBUTE,
		SHAPE_FILE_ATTRIBUTE
	};

	enum ReducerType
	{
		REDUCER_MIN,
		REDUCER_MAX,
		REDUCER_MEAN,
		REDUCER_MEDIAN,
		REDUCER_LOOKUP,
		REDUCER_VOTE,
		REDUCER_WEIGHTED_MEAN,
		REDUCER_PERCENTILE,
		REDUCER_MIN_DISTANCE,
		REDUCER_RRESENCE,
		REDUCER_NUM_IN_ROI,
	};
}
#endif //GPLATESDATAMINING_REDUCERTYPES_H


