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

#include "Enumeration.h"
#include "GmlDataBlock.h"
#include "GmlDataBlockCoordinateList.h"
#include "GmlLineString.h"
#include "GmlMultiPoint.h"
#include "GmlOrientableCurve.h"
#include "GmlPoint.h"
#include "GmlPolygon.h"
#include "GmlTimeInstant.h"
#include "GmlTimePeriod.h"
#include "GpmlArray.h"
#include "GpmlConstantValue.h"
#include "GpmlFiniteRotation.h"
#include "GpmlFiniteRotationSlerp.h"
#include "GpmlIrregularSampling.h"
#include "GpmlKeyValueDictionary.h"
#include "GpmlKeyValueDictionaryElement.h"
#include "GpmlMetadata.h"
#include "GpmlOldPlatesHeader.h"
#include "GpmlPiecewiseAggregation.h"
#include "GpmlPlateId.h"
#include "GpmlTimeSample.h"
#include "GpmlTimeWindow.h"
#include "XsBoolean.h"
#include "XsDouble.h"
#include "XsInteger.h"
#include "XsString.h"

#include "model/PropertyValue.h"
#include "model/RevisionedVector.h"
#include "model/TranscribeRevisionedVector.h"


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
		((GPlatesPropertyValues::Enumeration, \
			"GPlatesPropertyValues::Enumeration")) \
		\
		((GPlatesPropertyValues::GmlDataBlock, \
			"GPlatesPropertyValues::GmlDataBlock")) \
		\
		((GPlatesPropertyValues::GmlDataBlockCoordinateList, \
			"GPlatesPropertyValues::GmlDataBlockCoordinateList")) \
		\
		((GPlatesPropertyValues::GmlLineString, \
			"GPlatesPropertyValues::GmlLineString")) \
		\
		((GPlatesPropertyValues::GmlMultiPoint, \
			"GPlatesPropertyValues::GmlMultiPoint")) \
		\
		((GPlatesPropertyValues::GmlOrientableCurve, \
			"GPlatesPropertyValues::GmlOrientableCurve")) \
		\
		((GPlatesPropertyValues::GmlPoint, \
			"GPlatesPropertyValues::GmlPoint")) \
		\
		((GPlatesPropertyValues::GmlPolygon, \
			"GPlatesPropertyValues::GmlPolygon")) \
		\
		((GPlatesPropertyValues::GmlTimeInstant, \
			"GPlatesPropertyValues::GmlTimeInstant")) \
		\
		((GPlatesPropertyValues::GmlTimePeriod, \
			"GPlatesPropertyValues::GmlTimePeriod")) \
		\
		((GPlatesPropertyValues::GpmlArray, \
			"GPlatesPropertyValues::GpmlArray")) \
		\
		((GPlatesPropertyValues::GpmlConstantValue, \
			"GPlatesPropertyValues::GpmlConstantValue")) \
		\
		((GPlatesPropertyValues::GpmlFiniteRotation, \
			"GPlatesPropertyValues::GpmlFiniteRotation")) \
		\
		((GPlatesPropertyValues::GpmlFiniteRotationSlerp, \
			"GPlatesPropertyValues::GpmlFiniteRotationSlerp")) \
		\
		((GPlatesPropertyValues::GpmlIrregularSampling, \
			"GPlatesPropertyValues::GpmlIrregularSampling")) \
		\
		((GPlatesPropertyValues::GpmlKeyValueDictionary, \
			"GPlatesPropertyValues::GpmlKeyValueDictionary")) \
		\
		((GPlatesPropertyValues::GpmlKeyValueDictionaryElement, \
			"GPlatesPropertyValues::GpmlKeyValueDictionaryElement")) \
		\
		((GPlatesPropertyValues::GpmlMetadata, \
			"GPlatesPropertyValues::GpmlMetadata")) \
		\
		((GPlatesPropertyValues::GpmlOldPlatesHeader, \
			"GPlatesPropertyValues::GpmlOldPlatesHeader")) \
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
		((GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList>, \
			"GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList>")) \
		\
		((GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>, \
			"GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>")) \
		\
		((GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>, \
			"GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>")) \
		\
		((GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>, \
			"GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>")) \
		\
		((GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>, \
			"GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>")) \
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
