/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef GPLATES_DATA_MINING_SCRIBEEXPORTPROPERTYVALUES_H
#define GPLATES_DATA_MINING_SCRIBEEXPORTPROPERTYVALUES_H

#include "GmlDataBlock.h"
#include "GmlDataBlockCoordinateList.h"
#include "GmlTimeInstant.h"
#include "GmlTimePeriod.h"
#include "GpmlConstantValue.h"
#include "GpmlFiniteRotationSlerp.h"
#include "GpmlIrregularSampling.h"
#include "GpmlPiecewiseAggregation.h"
#include "GpmlPlateId.h"
#include "GpmlTimeSample.h"
#include "GpmlTimeWindow.h"
#include "XsBoolean.h"
#include "XsDouble.h"
#include "XsInteger.h"
#include "XsString.h"


/**
 * Scribe export registered classes/types in the 'property-values' source sub-directory.
 *
 * See "ScribeExportRegistration.h" for more details.
 *
 *******************************************************************************
 * WARNING: Changing the string ids will break backward/forward compatibility. *
 *******************************************************************************
 */
#define SCRIBE_EXPORT_PROPERTY_VALUES \
		\
		((GPlatesPropertyValues::GmlDataBlock, \
			"GPlatesPropertyValues::GmlDataBlock")) \
		\
		((GPlatesPropertyValues::GmlDataBlockCoordinateList, \
			"GPlatesPropertyValues::GmlDataBlockCoordinateList")) \
		\
		((GPlatesPropertyValues::GmlTimeInstant, \
			"GPlatesPropertyValues::GmlTimeInstant")) \
		\
		((GPlatesPropertyValues::GmlTimePeriod, \
			"GPlatesPropertyValues::GmlTimePeriod")) \
		\
		((GPlatesPropertyValues::GpmlConstantValue, \
			"GPlatesPropertyValues::GpmlConstantValue")) \
		\
		((GPlatesPropertyValues::GpmlFiniteRotationSlerp, \
			"GPlatesPropertyValues::GpmlFiniteRotationSlerp")) \
		\
		((GPlatesPropertyValues::GpmlIrregularSampling, \
			"GPlatesPropertyValues::GpmlIrregularSampling")) \
		\
		((GPlatesPropertyValues::GpmlPiecewiseAggregation, \
			"GPlatesPropertyValues::GpmlPiecewiseAggregation")) \
		\
		((GPlatesPropertyValues::GpmlPlateId, \
			"GPlatesPropertyValues::GpmlPlateId")) \
		\
		((GPlatesPropertyValues::GpmlTimeSample, \
			"GPlatesPropertyValues::GpmlTimeSample")) \
		\
		((GPlatesPropertyValues::GpmlTimeWindow, \
			"GPlatesPropertyValues::GpmlTimeWindow")) \
		\
		((GPlatesPropertyValues::XsBoolean, \
			"GPlatesPropertyValues::XsBoolean")) \
		\
		((GPlatesPropertyValues::XsDouble, \
			"GPlatesPropertyValues::XsDouble")) \
		\
		((GPlatesPropertyValues::XsInteger, \
			"GPlatesPropertyValues::XsInteger")) \
		\
		((GPlatesPropertyValues::XsString, \
			"GPlatesPropertyValues::XsString")) \
		\


#endif // GPLATES_DATA_MINING_SCRIBEEXPORTPROPERTYVALUES_H
