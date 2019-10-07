/* $Id: FeatureCollection.cc 11961 2011-07-07 03:49:38Z mchin $ */

/**
 * \file 
 * $Revision: 11961 $
 * $Date: 2011-07-07 13:49:38 +1000 (Thu, 07 Jul 2011) $
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <iostream>
#include <sstream>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <QRegExp>
#include <QString>

#include "PyFeature.h"

#include "PythonUtils.h"

#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/ShapefileAttributeFinder.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"

#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlGridEnvelope.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlStringList.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	namespace
	{
		/*
		 * Get property value as python object.
		 * This visitor visits one property value each time. 
		 * Use outer loop to iterator through all properties in a feature.
		 */
		class GetPropertyAsPythonObjVisitor : 
				public GPlatesModel::ConstFeatureVisitor
		{
		public:
		
			void 
			visit_gml_data_block(
					gml_data_block_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_plate_id(
					gpml_plate_id_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_gml_time_period(
					gml_time_period_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_enumeration(
					enumeration_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_gml_line_string(
					gml_line_string_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_gml_multi_point(
					gml_multi_point_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_gml_orientable_curve(
						gml_orientable_curve_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void
			visit_gml_point(
						gml_point_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_gml_polygon(
						gml_polygon_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}

			void 
			visit_gml_time_instant(
					gml_time_instant_type &v)
			{
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_feature_reference(
					gpml_feature_reference_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_feature_snapshot_reference(
					gpml_feature_snapshot_reference_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_finite_rotation(
					gpml_finite_rotation_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_finite_rotation_slerp(
					gpml_finite_rotation_slerp_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_hot_spot_trail_mark(
					gpml_hot_spot_trail_mark_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_irregular_sampling(
					gpml_irregular_sampling_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_key_value_dictionary(
					gpml_key_value_dictionary_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_measure(
					gpml_measure_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_old_plates_header(
					gpml_old_plates_header_type &v) 
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_piecewise_aggregation(
					gpml_piecewise_aggregation_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_polarity_chron_id(
					gpml_polarity_chron_id_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_gpml_property_delegate(
					gpml_property_delegate_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_revision_id(
					gpml_revision_id_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_topological_polygon(
					gpml_topological_polygon_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}
		

			void
			visit_gpml_topological_line_section(
					gpml_topological_line_section_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void
			visit_gpml_topological_point(
					gpml_topological_point_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_uninterpreted_property_value(
					uninterpreted_property_value_type &v)
			{ 
				//TODO:
				d_val =	PythonUtils::qstring_to_python_string(to_qstring(v));
			}


			void 
			visit_xs_boolean(
					xs_boolean_type &v)
			{
				d_val = bp::object(v.value());
			}


			void
			visit_xs_double(
					xs_double_type &v)
			{
				d_val = bp::object(v.value());
			}
		

			void 
			visit_xs_integer(
					xs_integer_type &v)
			{
				d_val = bp::object(v.value());
			}


			void 
			visit_xs_string(
					xs_string_type &xs_string)
			{
				const QByteArray buf = xs_string.value().get().qstring().toUtf8();
				d_val = bp::object(bp::str(buf.constData()));
			}


			void 
			visit_gpml_constant_value(
					gpml_constant_value_type &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			bp::object
			get_data()
			{
				return d_val;
			}

		protected:
			QString
			to_qstring(
					const GPlatesModel::PropertyValue& data)
			{
				std::stringstream ss;
				data.print_to(ss);
				return QString(ss.str().c_str());
			}
		
		private:
			bp::object d_val;
		};


		boost::optional<GPlatesModel::PropertyName>
		convert_property_name(
				const QString& name)
		{
			QRegExp rx("^\\s*(gpml|gml)\\s*:\\s*(\\w+)\\s*"); // (gpml|gml):(name)
			rx.indexIn(name);
			QString prefix = rx.cap(1);
			QString short_name = rx.cap(2);
	
			boost::optional<GPlatesModel::PropertyName> ret = boost::none;
			if(prefix.length() == 0 || short_name.length() == 0)
			{
				//qWarning() << "The prefix or name is empty.";
				//qWarning() << prefix << " : " << short_name;
			}

			if(prefix == "gpml")
			{
				ret = GPlatesModel::PropertyName::create_gpml(short_name);
			}

			if(prefix == "gml")
			{
				ret = GPlatesModel::PropertyName::create_gml(short_name);
			}

			return ret; 
		}


		boost::optional<QString>
		get_shapefile_attribute(
				const QString& name)
		{
			boost::optional<QString> shape_name = boost::none;
			QRegExp rx("^\\s*(gpml:shapefileAttributes)\\s*:\\s*\\b(\\w+)\\b\\s*"); // gpml:shapefileAttributes
			if(rx.indexIn(name) != -1)
			{
				shape_name = rx.cap(2);
			//	qDebug() << "Shapefile attribute name: " << *shape_name;
			}
			return shape_name;
		}


		boost::tuple<
				GPlatesMaths::Real, 
				GPlatesMaths::Real>
		get_start_end_time(
				const GPlatesModel::FeatureHandle* feature_ptr)
		{
			GPlatesMaths::Real begin_time(0);
			GPlatesMaths::Real end_time(0);

			static const GPlatesModel::PropertyName GML_VALID_TIME = GPlatesModel::PropertyName::create_gml("validTime");

			boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> gml_valid_time =
					GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
							feature_ptr->reference(),
							GML_VALID_TIME);
			if (gml_valid_time)
			{
				// Note that begin/end times can be finite, or positive/negative infinity.
				begin_time = gml_valid_time.get()->begin()->time_position().value();
				end_time = gml_valid_time.get()->end()->time_position().value();
			}

			return boost::make_tuple(begin_time, end_time);
		}
	}
}

void
export_feature()
{
	using namespace boost::python;
	class_<GPlatesApi::Feature>("Feature", no_init )
		.def("get_properties",				&GPlatesApi::Feature::get_properties)
		.def("get_properties_by_name",		&GPlatesApi::Feature::get_properties_by_name)
		.def("plate_id",					&GPlatesApi::Feature::plate_id)
		.def("feature_id",					&GPlatesApi::Feature::feature_id)
		.def("feature_type",				&GPlatesApi::Feature::feature_type)
		.def("valid_time",					&GPlatesApi::Feature::valid_time)
		.def("begin_time",					&GPlatesApi::Feature::begin_time)
		.def("end_time",					&GPlatesApi::Feature::end_time)
		.def("get_all_property_names",		&GPlatesApi::Feature::get_all_property_names)
//  		.add_property("feature_id", &GPlatesApi::Feature::feature_id)
//  		.add_property("feature_type", &GPlatesApi::Feature::feature_type)
//  		.add_property("valid_time",&GPlatesApi::Feature::valid_time)
		;
}


bp::list
GPlatesApi::Feature::get_properties()
{
	bp::list ret;
	
	if(!d_handle.is_valid())
		return ret;

	bp::list all_names = get_all_property_names();
	bp::ssize_t n = bp::len(all_names);
	for(bp::ssize_t i=0; i<n; i++) 
	{
		//qDebug() << "Property name: " << QString(bp::extract<const char*>(all_names[i]));
 		ret.append(get_properties_by_name(all_names[i]));
	}
	return ret;
}


bp::list
GPlatesApi::Feature::get_properties_by_name(
		bp::object prop_name)
{
	bp::list ret;

	if(!d_handle.is_valid())
		return ret;

	using namespace GPlatesModel;
	bool is_shapefile_attr = false;
	const char* property_name = bp::extract<const char*>(prop_name);
	QString q_property_name = QString(property_name);

	boost::optional<QString> shapefile_attr = get_shapefile_attribute(q_property_name);
	if(shapefile_attr)
	{
		is_shapefile_attr = true;
	}

	boost::optional<PropertyName> p_name = is_shapefile_attr ? 
		PropertyName::create_gpml("shapefileAttributes") :
		convert_property_name(q_property_name);
	
	if(!p_name)
	{
		//qDebug() << "invalid property name: " << q_property_name;
		return ret;
	}

	FeatureHandle::const_iterator it = d_handle->begin(), it_end = d_handle->end();
	for(; it != it_end; it++)
	{
		if((*it)->property_name() == (*p_name))
		{
			if(!is_shapefile_attr)
			{
 				GetPropertyAsPythonObjVisitor visitor;
 				(*it)->accept_visitor(visitor);
 				bp::object data = visitor.get_data(); 
				if(!(data.ptr() == Py_None))
 				{
 					ret.append(data);
 				}
			}
			else
			{
				GPlatesFeatureVisitors::ShapefileAttributeFinder visitor(*shapefile_attr);
				(*it)->accept_visitor(visitor);
				
				if(1 < std::distance(visitor.found_qvariants_begin(),visitor.found_qvariants_end()))
				{
					qWarning() << "More than one shape file attributes(with the same name) have been found.";
				}

				if(0 == std::distance(visitor.found_qvariants_begin(),visitor.found_qvariants_end()))
				{
					//qDebug() << "No shape file attribute found.";
					continue;
				}

				QVariant data = *visitor.found_qvariants_begin();
				switch (data.type())
				{
					case QVariant::Bool:
						ret.append(data.toBool());
						break;

					case QVariant::Int:
						ret.append(data.toInt());
						break;

					case QVariant::Double:
						ret.append(data.toDouble());
						break;

					case QVariant::String:
						ret.append(PythonUtils::qstring_to_python_string(data.toString()));
						break;
					default:
						qDebug() << "Unknown type of shape file attribute.";
						break;
				}
			}
 		}
	}
	return ret;
}


bp::list
GPlatesApi::Feature::get_all_property_names()
{
	using namespace GPlatesModel;
	bp::list ret;
	
	if(!d_handle.is_valid())
		return ret;

	FeatureHandle::const_iterator it = d_handle->begin(), it_end = d_handle->end();
	for(; it != it_end; it++)
	{
		const static PropertyName shape_file_attr_name = PropertyName::create_gpml("shapefileAttributes");
		PropertyName name = (*it)->property_name();
		
		if(shape_file_attr_name == name)//shape file attributes
		{
			GPlatesFeatureVisitors::KeyValueDictionaryFinder finder(shape_file_attr_name);
			(*it)->accept_visitor(finder);
			//finder.visit_feature(d_handle);
			
			if (finder.found_key_value_dictionaries_begin() != finder.found_key_value_dictionaries_end())
			{
				GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type dictionary =
					*(finder.found_key_value_dictionaries_begin());

				std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
					iter = dictionary->elements().begin(),
					end = dictionary->elements().end();

				for (; iter != end; ++iter)
				{
					const QByteArray buf = (shape_file_attr_name.build_aliased_name().qstring() + ":" + 
						GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get())).toUtf8();
					//qDebug() << "name: " << QString(buf);
					ret.append(bp::str(buf.data()));
				}
			}
		}
		else
		{
			const QByteArray buf = name.build_aliased_name().qstring().toUtf8();
			//qDebug() << "name: " << QString(buf);
			ret.append(bp::str(buf.data()));
		}
	}
	return ret;
}


bp::object
GPlatesApi::Feature::feature_id()
{
	if(!d_handle.is_valid())
		return bp::object();

	const QByteArray buf = d_handle->feature_id().get().qstring().toUtf8();
	return bp::str(buf.data());
}


bp::tuple
GPlatesApi::Feature::valid_time()
{
	if(!d_handle.is_valid())
		return bp::tuple();

	GPlatesMaths::Real start, end;
	boost::tie(start, end) = get_start_end_time(d_handle.handle_ptr());
	return bp::make_tuple(start.dval(),end.dval());
}


bp::object
GPlatesApi::Feature::begin_time()
{
	if(!d_handle.is_valid())
		return bp::object();

	return valid_time()[0];
}


bp::object
GPlatesApi::Feature::end_time()
{
	if(!d_handle.is_valid())
		return bp::object();

	return valid_time()[1];
}



bp::object
GPlatesApi::Feature::feature_type()
{
	if(!d_handle.is_valid())
		return bp::object();

	const QByteArray buf = d_handle->feature_type().get_name().qstring().toUtf8();
	return bp::str(buf.data());
}


unsigned long
GPlatesApi::Feature::plate_id()
{
	if(!d_handle.is_valid())
		return 0;

	static const GPlatesModel::PropertyName GPML_RECONSTRUCTION_PLATE_ID =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> gpml_plate_id =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					d_handle,
					GPML_RECONSTRUCTION_PLATE_ID);
	if (!gpml_plate_id)
	{
		return 0;
	}

	return gpml_plate_id.get()->value();
}


#endif   //GPLATES_NO_PYTHON

