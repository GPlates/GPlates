/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATESDATAMINING_DATAOPERATORTYPES_H
#define GPLATESDATAMINING_DATAOPERATORTYPES_H

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

	enum DataOperatorType
	{
		DATA_OPERATOR_MIN,
		DATA_OPERATOR_MAX,
		DATA_OPERATOR_MEAN,
		DATA_OPERATOR_MEDIAN,
		DATA_OPERATOR_LOOKUP,
		DATA_OPERATOR_VOTE,
		DATA_OPERATOR_WEIGHTED_MEAN,
		DATA_OPERATOR_PERCENTILE,
		DATA_OPERATOR_MIN_DISTANCE,
		DATA_OPERATOR_RRESENCE,
		DATA_OPERATOR_NUM_IN_ROI,
	};
}
#endif //GPLATESDATAMINING_DATAOPERATORTYPES_H


