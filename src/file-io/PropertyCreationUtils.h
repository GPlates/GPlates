/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_PROPERTYCREATIONUTILS_H
#define GPLATES_FILEIO_PROPERTYCREATIONUTILS_H

#include <map>
#include <QtXml/QXmlStreamReader>
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "ReadErrorOccurrence.h"
#include "ReadErrorAccumulation.h"
#include "GpmlReaderUtils.h"

#include "model/FeatureId.h"
#include "model/RevisionId.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsInteger.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsString.h"


#define AS_PROP_VAL(create_prop) \
	inline \
	GPlatesModel::PropertyValue::non_null_ptr_type \
	create_prop##_as_prop_val( \
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem) \
	{ \
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> \
			prop_val(create_prop(elem)); \
		if (prop_val) { \
			return *prop_val; \
		} \
		return GPlatesPropertyValues::UninterpretedPropertyValue::create(elem); \
	}


namespace GPlatesFileIO
{
	namespace PropertyCreationUtils
	{
		typedef GPlatesModel::PropertyValue::non_null_ptr_type
			(*PropertyCreator)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		typedef std::map<GPlatesModel::PropertyName, PropertyCreator> PropertyCreatorMap;


		boost::optional< GPlatesModel::FeatureId >
		create_feature_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		boost::optional< GPlatesModel::RevisionId >
		create_revision_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		boost::optional< GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type > 
		create_plate_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_plate_id)


		boost::optional< GPlatesPropertyValues::XsBoolean::non_null_ptr_type >
		create_xs_boolean(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_boolean)


		boost::optional< GPlatesPropertyValues::XsInteger::non_null_ptr_type >
		create_xs_integer(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_integer)


		boost::optional< GPlatesPropertyValues::XsDouble::non_null_ptr_type >
		create_xs_double(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_double)


		boost::optional< GPlatesPropertyValues::XsString::non_null_ptr_type >
		create_xs_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_string)


		boost::optional< GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type > 
		create_gpml_revision_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_revision_id)


		boost::optional< GPlatesPropertyValues::GeoTimeInstant >
		create_geo_time_instant(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		boost::optional< GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type > 
		create_time_instant(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_time_instant)


		boost::optional< GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type > 
		create_time_period(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_time_period)


		boost::optional< GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type > 
		create_polarity_chron_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_polarity_chron_id)


		boost::optional< GPlatesPropertyValues::GpmlTimeSample > 
		create_time_sample(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		boost::optional< GPlatesPropertyValues::GpmlTimeWindow > 
		create_time_window(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		boost::optional< GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type > 
		create_old_plates_header(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_old_plates_header)


		boost::optional< GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type > 
		create_constant_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_constant_value)


		boost::optional< GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type > 
		create_irregular_sampling(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_irregular_sampling)


		boost::optional< GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type > 
		create_piecewise_aggregation(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_piecewise_aggregation)


		boost::optional< GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type >
		create_interpolation_function(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_interpolation_function)


		boost::optional< GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type >
		create_finite_rotation(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_finite_rotation)


		boost::optional< GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type >
		create_finite_rotation_slerp(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_finite_rotation_slerp)


		boost::optional< GPlatesPropertyValues::GmlPoint::non_null_ptr_type >
		create_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_point)


		boost::optional< GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type >
		create_measure(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_measure)


		boost::optional< GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type >
		create_hot_spot_trail_mark(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_hot_spot_trail_mark)


		boost::optional< GPlatesPropertyValues::GmlLineString::non_null_ptr_type >
		create_line_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_line_string)


		boost::optional< GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type >
		create_orientable_curve(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_orientable_curve)


		boost::optional< GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type >
		create_feature_reference(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_feature_reference)


		boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type > 
		create_geometry(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_geometry)


		boost::optional< GPlatesModel::PropertyValue::non_null_ptr_type > 
		create_time_dependent_property_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_time_dependent_property_value)
	}
}

#endif // GPLATES_FILEIO_PROPERTYCREATIONUTILS_H
