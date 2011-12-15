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
#include "PyFeature.h"

#include "feature-visitors/ShapefileAttributeFinder.h"
#include "feature-visitors/KeyValueDictionaryFinder.h"

#include "property-values/GpmlKeyValueDictionary.h"

#include "utils/GetPropertyAsPythonObjVisitor.h"

#if !defined(GPLATES_NO_PYTHON)


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
		qDebug() << "Property name: " << QString(bp::extract<const char*>(all_names[i]));
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

	boost::optional<QString> shapefile_attr = GPlatesUtils::get_shapefile_attribute(q_property_name);
	if(shapefile_attr)
	{
		is_shapefile_attr = true;
	}

	boost::optional<PropertyName> p_name = is_shapefile_attr ? 
		PropertyName::create_gpml("shapefileAttributes") :
		GPlatesUtils::convert_property_name(q_property_name);
	
	if(!p_name)
	{
		qDebug() << "invalid property name: " << q_property_name;
		return ret;
	}

	FeatureHandle::const_iterator it = d_handle->begin(), it_end = d_handle->end();
	for(; it != it_end; it++)
	{
		if((*it)->property_name() == (*p_name))
		{
			if(!is_shapefile_attr)
			{
 				GPlatesUtils::GetPropertyAsPythonObjVisitor visitor;
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
					qDebug() << "No shape file attribute found.";
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
			qDebug() << "name: " << QString(buf);
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
	boost::tie(start, end) = GPlatesUtils::get_start_end_time(d_handle.handle_ptr());
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

	boost::optional<unsigned long> pid =
		GPlatesUtils::get_int_plate_id(d_handle.handle_ptr());
	return pid ? *pid : 0;
}


#endif   //GPLATES_NO_PYTHON

