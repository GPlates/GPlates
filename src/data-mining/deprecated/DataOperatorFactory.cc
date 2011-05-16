/* $Id: DataOperatorFactory.cc 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file .
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

#include "DataOperatorFactory.h"

#include "MinDistanceDataOperator.h"
#include "LookupDataOperator.h"
#include "MinDataOperator.h"
#include "PresenceDataOperator.h"
#include "NumInROIDataOperator.h"

using namespace GPlatesDataMining;

DataOperator*
DataOperatorFactory::create(
		DataOperatorType type,
		DataOperatorParameters cfg)
{
	switch (type)
	{
	case DATA_OPERATOR_MIN:

		return new MinDataOperator(cfg);
	
	case DATA_OPERATOR_LOOKUP:

		return new LookUpDataOperator(cfg);

	case DATA_OPERATOR_MIN_DISTANCE:

		return new MinDistanceDataOperator(cfg);

	case DATA_OPERATOR_RRESENCE:
	
		return new PresenceDataOperator(cfg);
	
	case DATA_OPERATOR_NUM_IN_ROI:

		return new NumInROIDataOperator(cfg);
		
	default:
		return new MinDataOperator();
		break;
	}
}


