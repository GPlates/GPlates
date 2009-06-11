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
#include "GpmlReaderUtils.h"

#include "model/FeatureId.h"
#include "model/RevisionId.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsInteger.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"
#include "property-values/Enumeration.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPolygon.h"
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
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "ReadErrors.h"


#define AS_PROP_VAL(create_prop) \
	inline \
	GPlatesModel::PropertyValue::non_null_ptr_type \
	create_prop##_as_prop_val( \
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem) \
	{ \
		GPlatesModel::PropertyValue::non_null_ptr_type prop_val(create_prop(elem)); \
		return prop_val; \
	}


namespace GPlatesFileIO
{
	namespace PropertyCreationUtils
	{
		class GpmlReaderException {

		public:
			GpmlReaderException(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &location_,
					const ReadErrors::Description &description_,
					const char *source_location_ = "not specified") :
				d_location(location_), d_description(description_), 
				d_source_location(source_location_)
			{  }

			const GPlatesModel::XmlElementNode::non_null_ptr_type
			location() const {
				return d_location;
			}

			ReadErrors::Description
			description() const {
				return d_description;
			}

			const char *
			source_location() const {
				return d_source_location;
			}

		private:
			GPlatesModel::XmlElementNode::non_null_ptr_type d_location;
			ReadErrors::Description d_description;
			const char *d_source_location;
		};


		typedef GPlatesModel::PropertyValue::non_null_ptr_type
			(*PropertyCreator)(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		typedef std::map<GPlatesModel::PropertyName, PropertyCreator> PropertyCreatorMap;


		GPlatesModel::FeatureId
		create_feature_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		GPlatesModel::RevisionId
		create_revision_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type
		create_plate_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_plate_id)


		GPlatesPropertyValues::XsBoolean::non_null_ptr_type
		create_xs_boolean(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_boolean)


		GPlatesPropertyValues::XsInteger::non_null_ptr_type
		create_xs_integer(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_integer)


		GPlatesPropertyValues::XsDouble::non_null_ptr_type
		create_xs_double(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_double)


		GPlatesPropertyValues::XsString::non_null_ptr_type
		create_xs_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_xs_string)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_absolute_reference_frame_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_absolute_reference_frame_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_continental_boundary_crust_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_continental_boundary_crust_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_continental_boundary_edge_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_continental_boundary_edge_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_continental_boundary_side_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_continental_boundary_side_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_dip_side_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_dip_side_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_dip_slip_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_dip_slip_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_fold_plane_annotation_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_fold_plane_annotation_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_slip_component_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_slip_component_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_strike_slip_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_strike_slip_enumeration)


		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_subduction_side_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_subduction_side_enumeration)


		GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type
		create_gpml_revision_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gpml_revision_id)


		GPlatesPropertyValues::GeoTimeInstant
		create_geo_time_instant(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		create_time_instant(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_time_instant)


		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		create_time_period(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_time_period)


		GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type
		create_polarity_chron_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_polarity_chron_id)


		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
		create_property_delegate(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_property_delegate)


		GPlatesPropertyValues::GpmlTimeSample
		create_time_sample(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		GPlatesPropertyValues::GpmlTimeWindow
		create_time_window(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
		create_old_plates_header(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_old_plates_header)


		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
		create_constant_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_constant_value)


		GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
		create_irregular_sampling(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_irregular_sampling)


		GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
		create_piecewise_aggregation(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_piecewise_aggregation)


		GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type
		create_interpolation_function(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_interpolation_function)


		GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
		create_finite_rotation(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_finite_rotation)


		GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type
		create_finite_rotation_slerp(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_finite_rotation_slerp)


		GPlatesPropertyValues::GmlPoint::non_null_ptr_type
		create_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_point)


		GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
		create_measure(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_measure)


		GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type
		create_hot_spot_trail_mark(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_hot_spot_trail_mark)


		GPlatesPropertyValues::GmlLineString::non_null_ptr_type
		create_line_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_line_string)


		GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
		create_gml_multi_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gml_multi_point)


		GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
		create_orientable_curve(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_orientable_curve)


		GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
		create_gml_polygon(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_gml_polygon)


		GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type
		create_feature_reference(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_feature_reference)


		GPlatesPropertyValues::GpmlFeatureSnapshotReference::non_null_ptr_type
		create_feature_snapshot_reference(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_feature_snapshot_reference)


		GPlatesModel::PropertyValue::non_null_ptr_type
		create_geometry(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_geometry)


		GPlatesModel::PropertyValue::non_null_ptr_type
		create_time_dependent_property_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_time_dependent_property_value)


		GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type
		create_topological_polygon(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_topological_polygon)


		GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type
		create_topological_section(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_topological_section)


		GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
		create_topological_line_section(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_topological_line_section)


		GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
		create_topological_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_topological_point)


		GPlatesPropertyValues::GpmlTopologicalIntersection
		create_topological_intersection(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		GPlatesPropertyValues::GpmlKeyValueDictionaryElement
		create_key_value_dictionary_element(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
		create_key_value_dictionary(
			const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);

		AS_PROP_VAL(create_key_value_dictionary)
	}
}

#endif // GPLATES_FILEIO_PROPERTYCREATIONUTILS_H
