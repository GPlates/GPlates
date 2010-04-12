/* $Id$ */

/**
 * \file 
 * This file just includes all header files for PropertyValue derivations.
 * This assists in development by allowing us to check for compilation errors
 * in PropertyValue derivations independent of the rest of the program.
 *
 * Most recent change:
 *   $Date$
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

#include "Enumeration.h"
#include "GmlDataBlock.h"
#include "GmlLineString.h"
#include "GmlMultiPoint.h"
#include "GmlOrientableCurve.h"
#include "GmlPoint.h"
#include "GmlPolygon.h"
#include "GmlTimeInstant.h"
#include "GmlTimePeriod.h"
#include "GpmlConstantValue.h"
#include "GpmlFeatureReference.h"
#include "GpmlFeatureSnapshotReference.h"
#include "GpmlFiniteRotation.h"
#include "GpmlHotSpotTrailMark.h"
#include "GpmlInterpolationFunction.h"
#include "GpmlIrregularSampling.h"
#include "GpmlKeyValueDictionary.h"
#include "GpmlMeasure.h"
#include "GpmlOldPlatesHeader.h"
#include "GpmlPiecewiseAggregation.h"
#include "GpmlPlateId.h"
#include "GpmlPolarityChronId.h"
#include "GpmlPropertyDelegate.h"
#include "GpmlRevisionId.h"
#include "GpmlTopologicalLineSection.h"
#include "GpmlTopologicalPoint.h"
#include "GpmlTopologicalPolygon.h"
#include "GpmlTopologicalSection.h"
#include "UninterpretedPropertyValue.h"
#include "XsBoolean.h"
#include "XsDouble.h"
#include "XsInteger.h"
#include "XsString.h"

