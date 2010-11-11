/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#include "ModelUtils.h"

#include "GPGIMInfo.h"
#include "Model.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/TemplateTypeParameterType.h"


const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesModel::ModelUtils::create_gml_orientable_curve(
		const GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string,
		bool reverse_orientation)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("orientation");
	XmlAttributeValue xml_attribute_value(reverse_orientation ? "-" : "+");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	return GPlatesPropertyValues::GmlOrientableCurve::create(gml_line_string, xml_attributes);
}


const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesModel::ModelUtils::create_gml_time_period(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_end)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant_begin =
			GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant_begin, xml_attributes);

	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant_end =
			GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant_end, xml_attributes);

	return GPlatesPropertyValues::GmlTimePeriod::create(gml_time_instant_begin, gml_time_instant_end);
}


const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
GPlatesModel::ModelUtils::create_gml_time_instant(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	return GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant, xml_attributes);
}


const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesModel::ModelUtils::create_gpml_constant_value(
		const PropertyValue::non_null_ptr_type property_value,
		const GPlatesPropertyValues::TemplateTypeParameterType &template_type_parameter_type)
{
	return GPlatesPropertyValues::GpmlConstantValue::create(property_value,
			template_type_parameter_type);
}


namespace
{
	class ExtractGeometryPropertyVisitor :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		ExtractGeometryPropertyVisitor(
				bool time_dependent) :
			d_time_dependent(time_dependent),
			d_is_property_value_constant_value(false),
			d_is_property_value_irregular_sampling_or_piecewise_aggregation(false)
		{  }

		// Note: We only visit geometry types and time-dependent types.

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			store_property(gml_line_string);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_LINE_STRING =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
			d_template_type_parameter_type = GML_LINE_STRING;
		}

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			store_property(gml_multi_point);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_MULTI_POINT =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint");
			d_template_type_parameter_type = GML_MULTI_POINT;
		}

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			store_property(gml_orientable_curve);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_ORIENTABLE_CURVE =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve");
			d_template_type_parameter_type = GML_ORIENTABLE_CURVE;
		}

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point)
		{
			store_property(gml_point);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_POINT =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point");
			d_template_type_parameter_type = GML_POINT;
		}

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{
			store_property(gml_polygon);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_POLYGON =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon");
			d_template_type_parameter_type = GML_POLYGON;
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			// If the new property value needs a time-dependent wrapper, then we just use
			// the existing time-dependent wrapper. Otherwise, unwrap the time-dependent wrapper.
			if (d_time_dependent)
			{
				store_property(gpml_constant_value);
				d_is_property_value_constant_value = true;
			}
			else
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}
		}

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
		{
			// Note that even if the new property value doesn't need a time-dependent
			// wrapper, there's nothing we can do to unwrap the irregular sampling into a
			// single value.
			store_property(gpml_irregular_sampling);
			d_is_property_value_irregular_sampling_or_piecewise_aggregation = true;
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			// Note that even if the new property value doesn't need a time-dependent
			// wrapper, there's nothing we can do to unwrap the piecewise aggregation
			// into a single value.
			store_property(gpml_piecewise_aggregation);
			d_is_property_value_irregular_sampling_or_piecewise_aggregation = true;
		}

		virtual
		void
		visit_gpml_topological_polygon(
				const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
		{
			store_property(gpml_topological_polygon);

			static const GPlatesPropertyValues::TemplateTypeParameterType GPML_TOPOLOGICAL_POLYGON =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");
			d_template_type_parameter_type = GPML_TOPOLOGICAL_POLYGON;
		}

		virtual
		void
		visit_gpml_topological_line_section(
				const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_topological_line_section)
		{
			store_property(gpml_topological_line_section);

			static const GPlatesPropertyValues::TemplateTypeParameterType GPML_TOPOLOGICAL_LINE_SECTION =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalLineSection");
			d_template_type_parameter_type = GPML_TOPOLOGICAL_LINE_SECTION;
		}

		virtual
		void
		visit_gpml_topological_point(
				const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_topological_point)
		{
			store_property(gpml_topological_point);

			static const GPlatesPropertyValues::TemplateTypeParameterType GPML_TOPOLOGICAL_POINT =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPoint");
			d_template_type_parameter_type = GPML_TOPOLOGICAL_POINT;
		}

		const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> &
		get_property_value() const
		{
			return d_property_value;
		}

		bool
		is_property_value_constant_value() const
		{
			return d_is_property_value_constant_value;
		}

		bool
		is_property_value_irregular_sampling_or_piecewise_aggregation() const
		{
			return d_is_property_value_irregular_sampling_or_piecewise_aggregation;
		}

		const boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> &
		get_template_type_parameter_type() const
		{
			return d_template_type_parameter_type;
		}

	private:

		void
		store_property(
				const GPlatesModel::PropertyValue &prop)
		{
			d_property_value = prop.deep_clone_as_prop_val();
		}

		/**
		 * Whether the new property expects a time dependent wrapper.
		 */
		bool d_time_dependent;

		bool d_is_property_value_constant_value;
		bool d_is_property_value_irregular_sampling_or_piecewise_aggregation;
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_property_value;
		boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> d_template_type_parameter_type;
	};
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesModel::ModelUtils::rename_geometric_property(
		const TopLevelProperty &top_level_property,
		const PropertyName &new_property_name,
		RenameGeometricPropertyError::Type *error_code)
{
	using namespace RenameGeometricPropertyError;

	try
	{
		const TopLevelPropertyInline &tlpi = dynamic_cast<const TopLevelPropertyInline &>(top_level_property);
		if (tlpi.size() != 1)
		{
			if (error_code)
			{
				*error_code = NOT_ONE_PROPERTY_VALUE;
			}
			return boost::none;
		}

		bool time_dependent = GPGIMInfo::expects_time_dependent_wrapper(tlpi.property_name());

		ExtractGeometryPropertyVisitor visitor(time_dependent);
		tlpi.accept_visitor(visitor);

		const boost::optional<PropertyValue::non_null_ptr_type> &property_value =
			visitor.get_property_value();
		if (!property_value)
		{
			if (error_code)
			{
				*error_code = NOT_GEOMETRY;
			}
			return boost::none;
		}

		if (time_dependent)
		{
			if (visitor.is_property_value_constant_value() ||
					visitor.is_property_value_irregular_sampling_or_piecewise_aggregation())
			{
				// Since the property_value is already a time-dependent property, we can
				// create the new geometry property directly.
				return TopLevelProperty::non_null_ptr_type(TopLevelPropertyInline::create(
						new_property_name, *property_value, tlpi.xml_attributes()));
			}
			else
			{
				// If the property_value is not already a time-dependent property, we will
				// need to wrap it up in a constant value.
				GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
					GPlatesPropertyValues::GpmlConstantValue::create(
							*property_value, *visitor.get_template_type_parameter_type());
				return TopLevelProperty::non_null_ptr_type(TopLevelPropertyInline::create(
						new_property_name, constant_value, tlpi.xml_attributes()));
			}
		}
		else
		{
			if (visitor.is_property_value_irregular_sampling_or_piecewise_aggregation())
			{
				// We need to unwrap the time-dependent property, but was unable to.
				if (error_code)
				{
					*error_code = COULD_NOT_UNWRAP_TIME_DEPENDENT_PROPERTY;
				}
				return boost::none;
			}
			else
			{
				return TopLevelProperty::non_null_ptr_type(TopLevelPropertyInline::create(
						new_property_name, *property_value, tlpi.xml_attributes()));
			}
		}
	}
	catch (const std::bad_cast &)
	{
		if (error_code)
		{
			*error_code = NOT_TOP_LEVEL_PROPERTY_INLINE;
		}
		return boost::none;
	}
}


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::ModelUtils::create_total_reconstruction_pole(
		const std::vector<TotalReconstructionPoleData> &five_tuples)
{
	std::vector<GPlatesPropertyValues::GpmlTimeSample> time_samples;
	GPlatesPropertyValues::TemplateTypeParameterType value_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("FiniteRotation");

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	for (std::vector<TotalReconstructionPoleData>::const_iterator iter = five_tuples.begin(); 
				iter != five_tuples.end(); ++iter) 
	{
		std::pair<double, double> gpml_euler_pole =
				std::make_pair(iter->lon_of_euler_pole, iter->lat_of_euler_pole);
		GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type gpml_finite_rotation =
				GPlatesPropertyValues::GpmlFiniteRotation::create(gpml_euler_pole,
				iter->rotation_angle);

		GPlatesPropertyValues::GeoTimeInstant geo_time_instant(iter->time);
		GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant =
				GPlatesPropertyValues::GmlTimeInstant::create(geo_time_instant, xml_attributes);

		UnicodeString comment_string(iter->comment);
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description =
				GPlatesPropertyValues::XsString::create(comment_string);

		time_samples.push_back(GPlatesPropertyValues::GpmlTimeSample(gpml_finite_rotation, gml_time_instant,
				get_intrusive_ptr(gml_description), value_type));
	}

	GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type gpml_finite_rotation_slerp =
			GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(value_type);

	PropertyValue::non_null_ptr_type gpml_irregular_sampling =
			GPlatesPropertyValues::GpmlIrregularSampling::create(time_samples,
			GPlatesUtils::get_intrusive_ptr(gpml_finite_rotation_slerp), value_type);

	PropertyName property_name =
		PropertyName::create_gpml("totalReconstructionPole");
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	TopLevelProperty::non_null_ptr_type top_level_property_inline =
			TopLevelPropertyInline::create(property_name,
			gpml_irregular_sampling, xml_attributes2);

	return top_level_property_inline;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ModelUtils::create_total_recon_seq(
		ModelInterface &model,
		const FeatureCollectionHandle::weak_ref &target_collection,
		unsigned long fixed_plate_id,
		unsigned long moving_plate_id,
		const std::vector<TotalReconstructionPoleData> &five_tuples)
{
	FeatureType feature_type = FeatureType::create_gpml("TotalReconstructionSequence");
	FeatureHandle::weak_ref feature = FeatureHandle::create(target_collection, feature_type);

	TopLevelProperty::non_null_ptr_type total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples);

	// DummyTransactionHandle pc1(__FILE__, __LINE__);
	feature->add(total_reconstruction_pole_container);
	// pc1.commit();

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type fixed_ref_frame(
			GPlatesPropertyValues::GpmlPlateId::create(fixed_plate_id));
	feature->add(
			TopLevelPropertyInline::create(
				PropertyName::create_gpml("fixedReferenceFrame"),
				fixed_ref_frame));

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type moving_ref_frame(
			GPlatesPropertyValues::GpmlPlateId::create(moving_plate_id));
	feature->add(
			TopLevelPropertyInline::create(
				PropertyName::create_gpml("movingReferenceFrame"),
				moving_ref_frame));

	return feature;
}

