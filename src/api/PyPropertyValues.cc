/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <algorithm>
#include <iterator>
#include <sstream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "PyPropertyValues.h"

#include "PyFeature.h"
#include "PyInformationModel.h"
#include "PyQualifiedXmlNames.h"
#include "PyRevisionedVector.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructionFeatureProperties.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"

#include "global/python.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/Gpgim.h"
#include "model/GpgimEnumerationType.h"
#include "model/ModelUtils.h"

#include "property-values/Enumeration.h"
#include "property-values/EnumerationContent.h"
#include "property-values/EnumerationType.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlDataBlockCoordinateList.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/TextContent.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "utils/UnicodeString.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	// Select the 'non-const' overload of 'PropertyValue::accept_visitor()'.
	void
	property_value_accept_visitor(
			GPlatesModel::PropertyValue &property_value,
			GPlatesModel::FeatureVisitor &visitor)
	{
		property_value.accept_visitor(visitor);
	}

	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	property_value_get_geometry(
			GPlatesModel::PropertyValue &property_value)
	{
		// Extract the geometry from the property value.
		return GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(property_value);
	}
}


void
export_property_value()
{
	/*
	 * PropertyValue - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base property value wrapper class.
	 *
	 * Enables 'isinstance(obj, PropertyValue)' in python - not that it's that useful.
	 */
	bp::class_<
			GPlatesModel::PropertyValue,
			GPlatesModel::PropertyValue::non_null_ptr_type,
			boost::noncopyable>(
					"PropertyValue",
					"The base class inherited by all derived property value classes. "
					"Property values are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). Two property values will only "
					"compare equal if they have the same derived property value *type* (and the same internal values). "
					"For example, a :class:`GpmlPlateId` property value instance and a :class:`XsInteger` "
					"property value instance will always compare as ``False``.\n"
					"\n"
					"The list of derived property value classes includes:\n"
					"\n"
					"* :class:`Enumeration`\n"
					"* :class:`GmlDataBlock`\n"
					"* :class:`GmlLineString`\n"
					"* :class:`GmlMultiPoint`\n"
					"* :class:`GmlOrientableCurve`\n"
					"* :class:`GmlPoint`\n"
					"* :class:`GmlPolygon`\n"
					"* :class:`GmlTimeInstant`\n"
					"* :class:`GmlTimePeriod`\n"
					"* :class:`GpmlArray`\n"
					"* :class:`GpmlConstantValue`\n"
					"* :class:`GpmlFiniteRotation`\n"
					//"* :class:`GpmlFiniteRotationSlerp`\n"
					//"* :class:`GpmlHotSpotTrailMark`\n"
					"* :class:`GpmlIrregularSampling`\n"
					"* :class:`GpmlKeyValueDictionary`\n"
					"* :class:`GpmlOldPlatesHeader`\n"
					"* :class:`GpmlPiecewiseAggregation`\n"
					"* :class:`GpmlPlateId`\n"
					"* :class:`GpmlPolarityChronId`\n"
					"* :class:`XsBoolean`\n"
					"* :class:`XsDouble`\n"
					"* :class:`XsInteger`\n"
					"* :class:`XsString`\n"
					"\n"
					"The following subset of derived property value classes are topological geometries:\n"
					"\n"
					"* :class:`GpmlTopologicalLine`\n"
					"* :class:`GpmlTopologicalPolygon`\n"
					"* :class:`GpmlTopologicalNetwork`\n"
					"\n"
					"The following subset of derived property value classes are time-dependent wrappers:\n"
					"\n"
					"* :class:`GpmlConstantValue`\n"
					"* :class:`GpmlIrregularSampling`\n"
					"* :class:`GpmlPiecewiseAggregation`\n"
					"\n"
					"You can use :meth:`get_value` to extract a value at a specific time "
					"from a time-dependent wrapper.\n"
					"\n"
					"A property value can be deep copied using :meth:`clone`.\n",
					bp::no_init)
		.def("clone",
				&GPlatesModel::PropertyValue::clone,
				"clone()\n"
				"  Create a duplicate of this property value (derived) instance, including a recursive copy "
				"of any nested property values that this instance might contain.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("accept_visitor",
				&GPlatesApi::property_value_accept_visitor,
				(bp::arg("visitor")),
				"accept_visitor(visitor)\n"
				"  Accept a property value visitor so that it can visit this property value. "
				"As part of the visitor pattern, this enables the visitor instance to discover "
				"the derived class type of this property. Note that there is no common interface "
				"shared by all property value types, hence the visitor pattern provides one way "
				"to find out which type of property value is being visited.\n"
				"\n"
				"  :param visitor: the visitor instance visiting this property value\n"
				"  :type visitor: :class:`PropertyValueVisitor`\n")
		.def("get_geometry",
				&GPlatesApi::property_value_get_geometry,
				"get_geometry()\n"
				"  Extracts the :class:`geometry<GeometryOnSphere>` if this property value contains a geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere` or None\n"
				"\n"
				"  This function searches for a geometry in the following standard geometry property value types:\n"
				"\n"
				"  * :class:`GmlPoint`\n"
				"  * :class:`GmlMultiPoint`\n"
				"  * :class:`GmlPolygon`\n"
				"  * :class:`GmlLineString`\n"
				"  * :class:`GmlOrientableCurve` (a possible wrapper around :class:`GmlLineString`)\n"
				"  * :class:`GpmlConstantValue` (a possible wrapper around any of the above)\n"
				"\n"
				"  If this property value does not contain a geometry then ``None`` is returned.\n"
				"\n"
				"  Time-dependent geometry properties are *not* yet supported, so the only time-dependent "
				"property value wrapper currently supported by this function is :class:`GpmlConstantValue`.\n"
				"\n"
				"  To extract geometry from a specific feature property:\n"
				"  ::\n"
				"\n"
				"    property_value = feature.get_value(pygplates.PropertyName.gpml_pole_position)\n"
				"    if property_value:\n"
				"        geometry = property_value.get_geometry()\n"
				"\n"
				"  ...however :meth:`Feature.get_geometry` provides an easier way to extract "
				"geometry from a feature with:\n"
				"  ::\n"
				"\n"
				"    geometry = feature.get_geometry(pygplates.PropertyName.gpml_pole_position)\n"
				"\n"
				"  To extract all geometries from a feature (regardless of which properties they came from):\n"
				"  ::\n"
				"\n"
				"    all_geometries = []\n"
				"    for property in feature:\n"
				"        property_value = property.get_value()\n"
				"        if property_value:\n"
				"            geometry = property_value.get_geometry()\n"
				"            if geometry:\n"
				"                all_geometries.append(geometry)\n"
				"\n"
				"  ...however again :meth:`Feature.get_geometry` does this more easily with:\n"
				"  ::\n"
				"\n"
				"    all_geometries = feature.get_geometry(lambda property: True, pygplates.PropertyReturn.all)\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesModel::PropertyValue>();
}

//////////////////////////////////////////////////////////////////////////
// NOTE: Please keep the property values alphabetically ordered.
//////////////////////////////////////////////////////////////////////////


namespace GPlatesApi
{
	namespace
	{
		void
		verify_enumeration_type_and_content(
				const GPlatesPropertyValues::EnumerationType &type,
				const GPlatesPropertyValues::EnumerationContent &content)
		{
			// Get the GPGIM enumeration type.
			boost::optional<GPlatesModel::GpgimEnumerationType::non_null_ptr_to_const_type> gpgim_enumeration_type =
					GPlatesModel::Gpgim::instance().get_property_enumeration_type(
							GPlatesPropertyValues::StructuralType(type));
			if (!gpgim_enumeration_type)
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The enumeration type '") +
								convert_qualified_xml_name_to_qstring(type) +
								"' was not recognised as a valid type by the GPGIM");
			}

			// Ensure the enumeration content is allowed, by the GPGIM, for the enumeration type.
			bool is_content_valid = false;
			const GPlatesModel::GpgimEnumerationType::content_seq_type &enum_contents =
					gpgim_enumeration_type.get()->get_contents();
			BOOST_FOREACH(const GPlatesModel::GpgimEnumerationType::Content &enum_content, enum_contents)
			{
				if (content.get().qstring() == enum_content.value)
				{
					is_content_valid = true;
					break;
				}
			}

			if (!is_content_valid)
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The enumeration content '") +
								content.get().qstring() +
								"' is not supported by enumeration type '" +
								convert_qualified_xml_name_to_qstring(type) + "'");
			}
		}
	}

	const GPlatesPropertyValues::Enumeration::non_null_ptr_type
	enumeration_create(
			const GPlatesPropertyValues::EnumerationType &type,
			const GPlatesPropertyValues::EnumerationContent &content,
			VerifyInformationModel::Value verify_information_model)
	{
		if (verify_information_model == VerifyInformationModel::YES)
		{
			verify_enumeration_type_and_content(type, content);
		}

		return GPlatesPropertyValues::Enumeration::create(type, content);
	}

	void
	enumeration_set_content(
			GPlatesPropertyValues::Enumeration &enumeration,
			const GPlatesPropertyValues::EnumerationContent &content,
			VerifyInformationModel::Value verify_information_model)
	{
		if (verify_information_model == VerifyInformationModel::YES)
		{
			verify_enumeration_type_and_content(enumeration.get_type(), content);
		}

		enumeration.set_value(content);
	}
}

void
export_enumeration()
{
	//
	// Enumeration - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::Enumeration,
			GPlatesPropertyValues::Enumeration::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"Enumeration",
					"A property value that represents a finite set of accepted (string) values per "
					"enumeration type.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::enumeration_create,
						bp::default_call_policies(),
						(bp::arg("type"),
								bp::arg("content"),
								bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES)),
				"__init__(type, content, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Create an enumeration property value from an enumeration type and content (value).\n"
				"\n"
				"  :param type: the type of the enumeration\n"
				"  :type type: :class:`EnumerationType`\n"
				"  :param content: the content (value) of the enumeration\n"
				"  :type content: string\n"
				"  :param verify_information_model: whether to check the information model for valid "
				"enumeration *type* and *content*\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and either *type* is not a recognised enumeration type or *content* is not a valid value "
				"for *type*\n"
				"\n"
				"  ::\n"
				"\n"
				"    dip_slip_enum = pygplates.Enumeration(\n"
				"        pygplates.EnumerationType.create_gpml('DipSlipEnumeration'),\n"
				"        'Extension')\n")
		.def("get_type",
				&GPlatesPropertyValues::Enumeration::get_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_type()\n"
				"  Returns the type of this enumeration.\n"
				"\n"
				"  :rtype: :class:`EnumerationType`\n")
		.def("get_content",
				&GPlatesPropertyValues::Enumeration::get_value,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_content()\n"
				"  Returns the content (value) of this enumeration.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_content",
				&GPlatesApi::enumeration_set_content,
				(bp::arg("content"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"set_content(content, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Sets the content (value) of this enumeration.\n"
				"\n"
				"  :param content: the content (value)\n"
				"  :type content: string\n"
				"  :param verify_information_model: whether to check the information model for valid "
				"enumeration *value*\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *content* is not a valid value for this enumeration :meth:`type<get_type>`\n"
				"\n"
				"  ::\n"
				"\n"
				"    dip_slip_enum.set_content('Extension')\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::Enumeration::non_null_ptr_type>())
	;

#if 0  // Not registering Enumeration because it represents many different structural types (it stores an EnumerationType).
	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::Enumeration>();
#endif

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::Enumeration>();
}


namespace GPlatesApi
{
	/**
	 * An iterator that checks the index is dereferenceable - we don't use bp::iterator in order
	 * to avoid issues with C++ iterators being invalidated from the C++ side and causing issues
	 * on the python side (eg, removing container elements on the C++ side while iterating over
	 * the container on the python side, for example, via a pyGPlates call back into the C++ code).
	 */
	class GmlDataBlockCoordinateListIterator
	{
	public:

		explicit
		GmlDataBlockCoordinateListIterator(
				GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &coordinate_lists) :
			d_coordinate_lists(coordinate_lists),
			d_index(0)
		{  }

		GmlDataBlockCoordinateListIterator &
		self()
		{
			return *this;
		}

		const GPlatesPropertyValues::ValueObjectType &
		next()
		{
			if (d_index >= d_coordinate_lists.size())
			{
				PyErr_SetString(PyExc_StopIteration, "No more data.");
				bp::throw_error_already_set();
			}
			return d_coordinate_lists[d_index++]->get_value_object_type();
		}

	private:
		//! Typedef for revisioned vector size type.
		typedef GPlatesModel::RevisionedVector<
				GPlatesPropertyValues::GmlDataBlockCoordinateList>::size_type size_type;

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &d_coordinate_lists;
		size_type d_index;
	};


	bp::dict
	create_dict_from_gml_data_block(
			const GPlatesPropertyValues::GmlDataBlock &gml_data_block)
	{
		std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type>
				gml_data_block_coordinate_lists;

		const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &
				scalar_value_lists = gml_data_block.tuple_list();
		const unsigned int num_scalar_types = scalar_value_lists.size();
		for (unsigned int n = 0; n < num_scalar_types; ++n)
		{
			gml_data_block_coordinate_lists.push_back(scalar_value_lists[n]);
		}

		return create_dict_from_gml_data_block_coordinate_lists(
				gml_data_block_coordinate_lists.begin(),
				gml_data_block_coordinate_lists.end());
	}

	std::map<
			GPlatesPropertyValues::ValueObjectType/*scalar type*/,
			std::vector<double>/*scalars*/>
	create_scalar_type_to_values_map(
			bp::object scalar_type_to_values_mapping_object,
			const char *type_error_string)
	{
		if (!type_error_string)
		{
			type_error_string = "Expected a 'dict' or a sequence of (scalar type, sequence of scalar values) 2-tuples";
		}

		// Extract the key/value pairs from a Python 'dict' or from a sequence of (key, value) tuples.
		PythonExtractUtils::key_value_map_type scalar_type_object_to_values_object_map;
		PythonExtractUtils::extract_key_value_map(
				scalar_type_object_to_values_object_map,
				scalar_type_to_values_mapping_object,
				type_error_string);

		std::map<
				GPlatesPropertyValues::ValueObjectType/*scalar type*/,
				std::vector<double>/*scalars*/> scalar_type_to_values_map;

		for (const auto &scalar_type_object_to_values_object : scalar_type_object_to_values_object_map)
		{
			bp::extract<GPlatesPropertyValues::ValueObjectType> extract_scalar_type(scalar_type_object_to_values_object.first);
			if (!extract_scalar_type.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}
			const GPlatesPropertyValues::ValueObjectType scalar_type = extract_scalar_type();

			// Add empty sequence of scalars for current scalar type.
			std::vector<double> &scalar_values = scalar_type_to_values_map[scalar_type];
			// This clears previous scalars (if have duplicate scalar types - which user shouldn't).
			scalar_values.clear();

			// Attempt to extract the scalar values for the current scalar type.
			PythonExtractUtils::extract_iterable(scalar_values, scalar_type_object_to_values_object.second, type_error_string);

			// Make sure the each scalar type has the same number of scalar values.
			for (const auto &scalar_type_to_values : scalar_type_to_values_map)
			{
				if (scalar_values.size() != scalar_type_to_values.second.size())
				{
					PyErr_SetString(PyExc_ValueError, "Each scalar type must have the same number of scalar values");
					bp::throw_error_already_set();
				}
			}
		}

		// Need at least one scalar type (and associated scalar values).
		if (scalar_type_to_values_map.empty())
		{
			PyErr_SetString(
					PyExc_ValueError,
					"At least one scalar type (and associated scalar values) is required");
			bp::throw_error_already_set();
		}

		return scalar_type_to_values_map;
	}

	const GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type
	create_gml_data_block(
			bp::object scalar_type_to_values_mapping_object,
			const char *type_error_string)
	{
		// Extract the mapping of scalar types to scalar values.
		const std::map<
				GPlatesPropertyValues::ValueObjectType/*scalar type*/,
				std::vector<double>/*scalars*/> scalar_type_to_values_map =
						create_scalar_type_to_values_map(
								scalar_type_to_values_mapping_object,
								type_error_string);

		std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type> coordinate_lists;

		// Store each map entry (scalar type and scalar values) in a GmlDataBlockCoordinateList.
		for (const auto &scalar_type_to_values : scalar_type_to_values_map)
		{
			const GPlatesPropertyValues::ValueObjectType &scalar_type = scalar_type_to_values.first;
			const std::vector<double> &scalar_values = scalar_type_to_values.second;

			// Add a new list of scalar values (associated with scalar type) to the data block.
			coordinate_lists.push_back(
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create(
							scalar_type,
							GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type(),
							scalar_values));
		}

		return GPlatesPropertyValues::GmlDataBlock::create(coordinate_lists);
	}

	const GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type
	gml_data_block_create(
			bp::object scalar_type_to_values_mapping_object)
	{
		return create_gml_data_block(scalar_type_to_values_mapping_object);
	}

	GmlDataBlockCoordinateListIterator
	gml_data_block_get_iter(
			GPlatesPropertyValues::GmlDataBlock &gml_data_block)
	{
		return GmlDataBlockCoordinateListIterator(gml_data_block.tuple_list());
	}

	unsigned int
	gml_data_block_len(
			GPlatesPropertyValues::GmlDataBlock &gml_data_block)
	{
		return gml_data_block.tuple_list().size();
	}

	bool
	gml_data_block_contains_scalar_type(
			GPlatesPropertyValues::GmlDataBlock &gml_data_block,
			const GPlatesPropertyValues::ValueObjectType &scalar_type)
	{
		const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &coordinate_lists =
				gml_data_block.tuple_list();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList>::const_iterator
				coordinate_lists_iter = coordinate_lists.begin(),
				coordinate_lists_end = coordinate_lists.end();

		// Search for the coordinate list associated with 'scalar_type'.
		for ( ; coordinate_lists_iter != coordinate_lists_end ; ++coordinate_lists_iter)
		{
			const GPlatesPropertyValues::GmlDataBlockCoordinateList &coordinate_list = **coordinate_lists_iter;

			const GPlatesPropertyValues::ValueObjectType &coordinate_list_scalar_type = coordinate_list.get_value_object_type();
			if (scalar_type == coordinate_list_scalar_type)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Return a list of scalar values for a scalar type, or None.
	 */
	bp::object
	gml_data_block_get_scalar_values(
			GPlatesPropertyValues::GmlDataBlock &gml_data_block,
			const GPlatesPropertyValues::ValueObjectType &scalar_type)
	{
		const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &coordinate_lists =
				gml_data_block.tuple_list();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList>::const_iterator
				coordinate_lists_iter = coordinate_lists.begin(),
				coordinate_lists_end = coordinate_lists.end();

		// Search for the coordinate list associated with 'scalar_type'.
		for ( ; coordinate_lists_iter != coordinate_lists_end ; ++coordinate_lists_iter)
		{
			const GPlatesPropertyValues::GmlDataBlockCoordinateList &coordinate_list = **coordinate_lists_iter;

			const GPlatesPropertyValues::ValueObjectType &coordinate_list_scalar_type = coordinate_list.get_value_object_type();
			if (scalar_type == coordinate_list_scalar_type)
			{
				bp::list scalar_value_list;

				// Add the coordinates to a Python list.
				GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type::const_iterator
						coordinate_list_iter = coordinate_list.get_coordinates().begin();
				const GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type::const_iterator
						coordinate_list_end = coordinate_list.get_coordinates().end();
				for ( ; coordinate_list_iter != coordinate_list_end; ++coordinate_list_iter)
				{
					scalar_value_list.append(*coordinate_list_iter);
				}

				return scalar_value_list;
			}
		}

		return bp::object()/*Py_None*/;
	}

	void
	gml_data_block_set(
			GPlatesPropertyValues::GmlDataBlock &gml_data_block,
			const GPlatesPropertyValues::ValueObjectType &scalar_type,
			bp::object scalar_values_object)
	{
		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &coordinate_lists =
				gml_data_block.tuple_list();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList>::iterator
				coordinate_lists_iter = coordinate_lists.begin(),
				coordinate_lists_end = coordinate_lists.end();

		// Search for the coordinate list associated with 'scalar_type'.
		for ( ; coordinate_lists_iter != coordinate_lists_end ; ++coordinate_lists_iter)
		{
			const GPlatesPropertyValues::GmlDataBlockCoordinateList &coordinate_list = **coordinate_lists_iter;

			const GPlatesPropertyValues::ValueObjectType &coordinate_list_scalar_type = coordinate_list.get_value_object_type();
			if (scalar_type == coordinate_list_scalar_type)
			{
				// Later (after potential exceptions are thrown) we'll replace this coordinate list.
				break;
			}
		}

		// Attempt to extract the scalar values from Python.
		GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type scalar_values;
		PythonExtractUtils::extract_iterable(scalar_values, scalar_values_object, "Expected a sequence of float");

		// Make sure has the same number of scalar values as existing scalar values.
		if (!coordinate_lists.empty() &&
			scalar_values.size() != coordinate_lists.front()->get_coordinates().size())
		{
			PyErr_SetString(PyExc_ValueError, "Each scalar type must have the same number of scalar values");
			bp::throw_error_already_set();
		}

		// Now that all exceptions have been dealt with we can make modifications.
		if (coordinate_lists_iter != coordinate_lists_end)
		{
			// Remove the existing list of scalar values (we'll add the new list of scalar values after the loop).
			coordinate_lists.erase(coordinate_lists_iter);
		}

		// Add a new list of scalar values to the data block.
		coordinate_lists.push_back(
				GPlatesPropertyValues::GmlDataBlockCoordinateList::create(
						scalar_type,
						GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type(),
						scalar_values));
	}

	void
	gml_data_block_remove(
			GPlatesPropertyValues::GmlDataBlock &gml_data_block,
			const GPlatesPropertyValues::ValueObjectType &scalar_type)
	{
		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList> &coordinate_lists =
				gml_data_block.tuple_list();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GmlDataBlockCoordinateList>::iterator
				coordinate_lists_iter = coordinate_lists.begin(),
				coordinate_lists_end = coordinate_lists.end();

		// Search for the coordinate list associated with 'scalar_type'.
		for ( ; coordinate_lists_iter != coordinate_lists_end ; ++coordinate_lists_iter)
		{
			const GPlatesPropertyValues::GmlDataBlockCoordinateList &coordinate_list = **coordinate_lists_iter;

			const GPlatesPropertyValues::ValueObjectType &coordinate_list_scalar_type = coordinate_list.get_value_object_type();
			if (scalar_type == coordinate_list_scalar_type)
			{
				// Remove the coordinate list and return.
				coordinate_lists.erase(coordinate_lists_iter);
				return;
			}
		}
	}
}

void
export_gml_data_block()
{
	// Iterator over scalar coordinate lists.
	// Note: We don't docstring this - it's not an interface the python user needs to know about.
	bp::class_<GPlatesApi::GmlDataBlockCoordinateListIterator>(
			// Prefix with '_' so users know it's an implementation detail (they should not be accessing it directly).
			"_GmlDataBlockCoordinateListIterator",
			bp::no_init)
		.def("__iter__", &GPlatesApi::GmlDataBlockCoordinateListIterator::self, bp::return_value_policy<bp::copy_non_const_reference>())
		.def(
#if PY_MAJOR_VERSION < 3
				"next",
#else
				"__next__",
#endif
				&GPlatesApi::GmlDataBlockCoordinateListIterator::next,
				bp::return_value_policy<bp::copy_const_reference>())
	;


	//
	// GmlDataBlock - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlDataBlock,
			GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlDataBlock",
					"A data block that associates each scalar type with a sequence of floating-point scalar values.\n"
					"\n"
					"This is typically used to store one or more scalar values for each point in a "
					":class:`geometry<GeometryOnSphere>` where each scalar value (stored at a point) "
					"has a different :class:`scalar type<ScalarType>`.\n"
					"\n"
					"The following operations are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(d)``                  number of scalar types in the data block *d*\n"
					"``for s in d``              iterates over the scalar type *s* in data block *d*\n"
					"``s in d``                  ``True`` if *s* is a scalar type in data block *d*\n"
					"``s not in d``              ``False`` if *s* is a scalar type in data block *d*\n"
					"=========================== ==========================================================\n"
					"\n"
					"For example:\n"
					"::\n"
					"\n"
					"  for scalar_type in data_block:\n"
					"      scalar_values = data_block.get_scalar_values(scalar_type)\n"
					"\n"
					"The following methods support getting, setting and removing scalar values in a data block:\n"
					"\n"
					"* :meth:`get_scalar_values`\n"
					"* :meth:`set`\n"
					"* :meth:`remove`\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_data_block_create,
						bp::default_call_policies(),
						(bp::arg("scalar_type_to_values_mapping"))),
				"__init__(scalar_type_to_values_mapping)\n"
				"  Create a data block containing one or more :class:`scalar types<ScalarType>` and their "
				"associated scalar values.\n"
				"\n"
				"  :param scalar_type_to_values_mapping: maps each scalar type to a sequence of scalar values\n"
				"  :type scalar_type_to_values_mapping: ``dict`` mapping each :class:`ScalarType` to a sequence "
				"of float, or a sequence of (:class:`ScalarType`, sequence of float) tuples\n"
				"  :raises: ValueError if *scalar_type_to_values_mapping* is empty, or if each "
				":class:`scalar type<ScalarType>` is not mapped to the same number of scalar values.\n"
				"\n"
				"  To create ``gpml:VelocityColat`` and ``gpml:VelocityLon`` scalar values:\n"
				"  ::\n"
				"\n"
				"    data_block = pygplates.GmlDataBlock(\n"
				"        [\n"
				"            (pygplates.ScalarType.create_gpml('VelocityColat'), [-1.5, -1.6, -1.55]),\n"
				"            (pygplates.ScalarType.create_gpml('VelocityLon'), [0.36, 0.37, 0.376])])\n"
				"\n"
				"  To do the same thing using a ``dict``:\n"
				"  ::\n"
				"\n"
				"    data_block = pygplates.GmlDataBlock(\n"
				"        {\n"
				"            pygplates.ScalarType.create_gpml('VelocityColat') : [-1.5, -1.6, -1.55],\n"
				"            pygplates.ScalarType.create_gpml('VelocityLon') : [0.36, 0.37, 0.376]})\n")
		.def("__iter__", &GPlatesApi::gml_data_block_get_iter)
		.def("__len__", &GPlatesApi::gml_data_block_len)
		.def("__contains__", &GPlatesApi::gml_data_block_contains_scalar_type)
		.def("get_scalar_values",
				&GPlatesApi::gml_data_block_get_scalar_values,
				(bp::arg("scalar_type")),
				"get_scalar_values(scalar_type)\n"
				"  Returns the list of scalar values associated with a scalar type.\n"
				"\n"
				"  :param scalar_type: the type of the scalars\n"
				"  :type scalar_type: :class:`ScalarType`\n"
				"  :returns: the scalar values associated with *scalar_type*, otherwise ``None`` if *scalar_type* does not exist\n"
				"  :rtype: list of float, or None\n"
				"\n"
				"  To test if a scalar type is present and retrieve its list of scalar values:\n"
				"  ::\n"
				"\n"
				"    velocity_colat_scalar_type = pygplates.ScalarType.create_gpml('VelocityColat')\n"
				"    velocity_colat_scalar_values = data_block.get_scalar_values(velocity_colat_scalar_type)\n"
				"    if velocity_colat_scalar_values:\n"
				"        for scalar_value in velocity_colat_scalar_values:\n"
				"            ...\n"
				"    # ...or a less efficient approach...\n"
				"    if velocity_colat_scalar_type in data_block:\n"
				"        velocity_colat_scalar_values = data_block.get_scalar_values(velocity_colat_scalar_type)\n")
		.def("set",
				&GPlatesApi::gml_data_block_set,
				(bp::arg("scalar_type"),
						bp::arg("scalar_values")),
				"set(scalar_type, scalar_values)\n"
				"  Sets the scalar values of the data block associated with a scalar type.\n"
				"\n"
				"  :param scalar_type: the type of the scalars\n"
				"  :type scalar_type: :class:`ScalarType`\n"
				"  :param scalar_values: the scalar values associated with *scalar_type*\n"
				"  :type scalar_values: sequence (eg, ``list`` or ``tuple``) of float\n"
				"  :raises: ValueError if the length of *scalar_values* does not match the length of "
				"existing scalar values for other :class:`scalar types<ScalarType>`.\n"
				"\n"
				"  To set (or replace) the scalar values associated with ``gpml:VelocityColat``:\n"
				"  ::\n"
				"\n"
				"    gml_data_block.set(\n"
				"        pygplates.ScalarType.create_gpml('VelocityColat'),\n"
				"        [-1.5, -1.6, -1.55])\n"
				"\n"
				"  .. note:: If there is no list of scalar values associated with *scalar_type* then "
				"a new list is added, otherwise the existing list is replaced.\n")
		.def("remove",
				&GPlatesApi::gml_data_block_remove,
				(bp::arg("scalar_type")),
				"remove(scalar_type)\n"
				"  Removes the list of scalar values associated with a scalar type.\n"
				"\n"
				"  :param scalar_type: the type of the scalars\n"
				"  :type scalar_type: :class:`ScalarType`\n"
				"\n"
				"  To remove the scalar values associated with ``gpml:VelocityColat``:\n"
				"  ::\n"
				"\n"
				"    gml_data_block.remove(pygplates.ScalarType.create_gpml('VelocityColat'))\n"
				"\n"
				"  .. note:: If *scalar_type* does not exist in the data block then it is ignored and nothing is done.\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlDataBlock>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlDataBlock>();
}


void
export_gml_line_string()
{
	//
	// GmlLineString - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlLineString,
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlLineString",
					"A property value representing a polyline geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::GmlLineString::create,
						bp::default_call_policies(),
						(bp::arg("polyline"))),
				"__init__(polyline)\n"
				"  Create a property value representing a polyline geometry.\n"
				"\n"
				"  :param polyline: the polyline geometry\n"
				"  :type polyline: :class:`PolylineOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"   line_string_property = pygplates.GmlLineString(polyline)\n")
		.def("get_polyline",
				&GPlatesPropertyValues::GmlLineString::get_polyline,
				"get_polyline()\n"
				"  Returns the polyline geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n")
		.def("set_polyline",
				&GPlatesPropertyValues::GmlLineString::set_polyline,
				(bp::arg("polyline")),
				"set_polyline(polyline)\n"
				"  Sets the polyline geometry of this property value.\n"
				"\n"
				"  :param polyline: the polyline geometry\n"
				"  :type polyline: :class:`PolylineOnSphere`\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlLineString::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlLineString>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlLineString>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
	gml_multi_point_create(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
	{
		return GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);
	}
}

void
export_gml_multi_point()
{
	//
	// GmlMultiPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlMultiPoint,
			GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlMultiPoint",
					"A property value representing a multi-point geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_multi_point_create,
						bp::default_call_policies(),
						(bp::arg("multi_point"))),
				"__init__(multi_point)\n"
				"  Create a property value representing a multi-point geometry.\n"
				"\n"
				"  :param multi_point: the multi-point geometry\n"
				"  :type multi_point: :class:`MultiPointOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"    multi_point_property = pygplates.GmlMultiPoint(multi_point)\n")
		.def("get_multi_point",
				&GPlatesPropertyValues::GmlMultiPoint::get_multipoint,
				"get_multi_point()\n"
				"  Returns the multi-point geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`MultiPointOnSphere`\n")
		.def("set_multi_point",
				&GPlatesPropertyValues::GmlMultiPoint::set_multipoint,
				(bp::arg("multi_point")),
				"set_multi_point(multi_point)\n"
				"  Sets the multi-point geometry of this property value.\n"
				"\n"
				"  :param multi_point: the multi-point geometry\n"
				"  :type multi_point: :class:`MultiPointOnSphere`\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlMultiPoint>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlMultiPoint>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
	gml_orientable_curve_create(
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string)
	{
		// Ignore the reverse flag for now - it never gets used by any client code.
		return GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
	}
}

void
export_gml_orientable_curve()
{
	// Use the 'non-const' overload so GmlOrientableCurve can be modified via python...
	const GPlatesPropertyValues::GmlLineString::non_null_ptr_type
			(GPlatesPropertyValues::GmlOrientableCurve::*base_curve)() =
					&GPlatesPropertyValues::GmlOrientableCurve::base_curve;

	//
	// GmlOrientableCurve - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlOrientableCurve,
			GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlOrientableCurve",
					"A property value representing a polyline geometry with a positive or negative orientation. "
					"However, currently the orientation is always positive so this is essentially no different "
					"than a :class:`GmlLineString`.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_orientable_curve_create,
						bp::default_call_policies(),
						(bp::arg("gml_line_string")/*, bp::arg(reverse_orientation)=false)*/)),
				//"__init__(gml_line_string, [reverse_orientation=False])\n"
				"__init__(gml_line_string)\n"
				"  Create an orientable polyline property value that wraps a polyline geometry and "
				"gives it an orientation.\n"
				"\n"
				"  :param gml_line_string: the line string (polyline) property value\n"
				"  :type gml_line_string: :class:`GmlLineString`\n"
				"\n"
				"  ::\n"
				"\n"
				"    orientable_curve_property = pygplates.GmlOrientableCurve(gml_line_string)\n"
				"\n"
				"  .. note:: Currently the orientation is always *positive* "
				"so this is essentially no different than a :class:`GmlLineString`.\n")
		.def("get_base_curve",
				base_curve,
				"get_base_curve()\n"
				"  Returns the line string (polyline) property value of this wrapped property value.\n"
				"\n"
				"  :rtype: :class:`GmlLineString`\n")
		.def("set_base_curve",
				&GPlatesPropertyValues::GmlOrientableCurve::set_base_curve,
				(bp::arg("base_curve")),
				"set_base_curve(base_curve)\n"
				"  Sets the line string (polyline) property value of this wrapped property value.\n"
				"\n"
				"  :param base_curve: the line string (polyline) property value\n"
				"  :type base_curve: :class:`GmlLineString`\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlOrientableCurve>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlOrientableCurve>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
	gml_point_create(
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		// Use the default value for the second argument.
		return GPlatesPropertyValues::GmlPoint::create(point_on_sphere);
	}
}

void
export_gml_point()
{
	//
	// GmlPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlPoint,
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlPoint",
					"A property value representing a point geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_point_create,
						bp::default_call_policies(),
						(bp::arg("point"))),
				"__init__(point)\n"
				"  Create a property value representing a point geometry.\n"
				"\n"
				"  :param point: the point geometry\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"\n"
				"  ::\n"
				"\n"
				"    point_property = pygplates.GmlPoint(point)\n")
		.def("get_point",
				&GPlatesPropertyValues::GmlPoint::get_point,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_point()\n"
				"  Returns the point geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("set_point",
				&GPlatesPropertyValues::GmlPoint::set_point,
				(bp::arg("point")),
				"set_point(point)\n"
				"  Sets the point geometry of this property value.\n"
				"\n"
				"  :param point: the point geometry\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlPoint::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlPoint>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlPoint>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
	gml_polygon_create(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
	{
		// We ignore interior polygons for now - later they will get stored a single PolygonOnSphere...
		return GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);
	}
}

void
export_gml_polygon()
{
	//
	// GmlPolygon - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlPolygon,
			GPlatesPropertyValues::GmlPolygon::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlPolygon",
					"A property value representing a polygon geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_polygon_create,
						bp::default_call_policies(),
						(bp::arg("polygon"))),
				"__init__(polygon)\n"
				"  Create a property value representing a polygon geometry.\n"
				"\n"
				"  :param polygon: the polygon geometry\n"
				"  :type polygon: :class:`PolygonOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"   polygon_property = pygplates.GmlPolygon(polygon)\n")
		.def("get_polygon",
				// We ignore interior polygons for now - later they will get stored in a single PolygonOnSphere...
				&GPlatesPropertyValues::GmlPolygon::get_polygon,
				"get_polygon()\n"
				"  Returns the polygon geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PolygonOnSphere`\n")
		.def("set_polygon",
				// We ignore interior polygons for now - later they will get stored in a single PolygonOnSphere...
				&GPlatesPropertyValues::GmlPolygon::set_polygon,
				(bp::arg("polygon")),
				"set_polygon(polygon)\n"
				"  Sets the polygon geometry of this property value.\n"
				"\n"
				"  :param polygon: the polygon geometry\n"
				"  :type polygon: :class:`PolygonOnSphere`\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlPolygon::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlPolygon>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlPolygon>();
}


void
export_gml_time_instant()
{
	//
	// GmlTimeInstant - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlTimeInstant",
					"A property value representing an instant in geological time.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesModel::ModelUtils::create_gml_time_instant,
						bp::default_call_policies(),
						(bp::arg("time_position"))),
				"__init__(time_position)\n"
				"  Create a property value representing a specific time instant.\n"
				"\n"
				"  :param time_position: the time position\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"\n"
				"  ::\n"
				"\n"
				"    begin_time_instant = pygplates.GmlTimeInstant(pygplates.GeoTimeInstant.create_distant_past())\n"
				"    end_time_instant = pygplates.GmlTimeInstant(0)\n")
		.def("get_time",
				&GPlatesPropertyValues::GmlTimeInstant::get_time_position,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_time()\n"
				"  Returns the time position of this property value.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to "
				"check for *distant past* or *distant future* for example:\n"
				"\n"
				"  ::\n"
				"\n"
				"    float_time = time_instant.get_time()\n"
				"    print 'Time instant is distant past: %s' % pygplates.GeoTimeInstant(float_time).is_distant_past()\n"
				"\n"
				"  ...or just test directly using ``float`` comparisons...\n"
				"\n"
				"  ::\n"
				"\n"
				"    float_time = time_instant.get_time()\n"
				"    print 'Time instant is distant past: %s' % (float_time == float('inf'))\n")
		.def("set_time",
				&GPlatesPropertyValues::GmlTimeInstant::set_time_position,
				(bp::arg("time_position")),
				"set_time(time_position)\n"
				"  Sets the time position of this property value.\n"
				"\n"
				"  :param time_position: the time position\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlTimeInstant>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlTimeInstant>();
}


namespace GPlatesApi
{
	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
	gml_time_period_create(
			const GPlatesPropertyValues::GeoTimeInstant &begin_time,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		return GPlatesModel::ModelUtils::create_gml_time_period(
				begin_time,
				end_time,
				true/*check_begin_end_times*/);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gml_time_period_get_begin_time(
			const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
	{
		return gml_time_period.begin()->get_time_position();
	}

	void
	gml_time_period_set_begin_time(
			GPlatesPropertyValues::GmlTimePeriod &gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time)
	{
		// We can check begin/end time class invariant due to our restricted (python) interface whereas
		// the C++ GmlTimePeriod cannot because clients can modify indirectly via 'GmlTimeInstant'.
		GPlatesGlobal::Assert<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
				begin_time <= gml_time_period.end()->get_time_position(),
				GPLATES_ASSERTION_SOURCE);

		gml_time_period.begin()->set_time_position(begin_time);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gml_time_period_get_end_time(
			const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
	{
		return gml_time_period.end()->get_time_position();
	}

	void
	gml_time_period_set_end_time(
			GPlatesPropertyValues::GmlTimePeriod &gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		// We can check begin/end time class invariant due to our restricted (python) interface whereas
		// the C++ GmlTimePeriod cannot because clients can modify indirectly via 'GmlTimeInstant'.
		GPlatesGlobal::Assert<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
				gml_time_period.begin()->get_time_position() <= end_time,
				GPLATES_ASSERTION_SOURCE);

		gml_time_period.end()->set_time_position(end_time);
	}

	bool
	gml_time_period_contains(
			GPlatesPropertyValues::GmlTimePeriod &gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &time)
	{
		return gml_time_period.contains(time);
	}
}

void
export_gml_time_period()
{
	//
	// GmlTimePeriod - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlTimePeriod",
					"A property value representing a period in geological time (time of appearance to time of disappearance).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_time_period_create,
						bp::default_call_policies(),
						(bp::arg("begin_time"), bp::arg("end_time"))),
				"__init__(begin_time, end_time)\n"
				"  Create a property value representing a specific time period.\n"
				"\n"
				"  :param begin_time: the begin time (time of appearance)\n"
				"  :type begin_time: float or :class:`GeoTimeInstant`\n"
				"  :param end_time: the end time (time of disappearance)\n"
				"  :type end_time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n"
				"\n"
				"  ::\n"
				"\n"
				"    time_period = pygplates.GmlTimePeriod(pygplates.GeoTimeInstant.create_distant_past(), 0)\n")
		.def("get_begin_time",
				&GPlatesApi::gml_time_period_get_begin_time,
				"get_begin_time()\n"
				"  Returns the begin time position (time of appearance) of this property value.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_begin_time",
				&GPlatesApi::gml_time_period_set_begin_time,
				(bp::arg("time_position")),
				"set_begin_time(time_position)\n"
				"  Sets the begin time position (time of appearance) of this property value.\n"
				"\n"
				"  :param time_position: the begin time position (time of appearance)\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		.def("get_end_time",
				&GPlatesApi::gml_time_period_get_end_time,
				"get_end_time()\n"
				"  Returns the end time position (time of disappearance) of this property value.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_end_time",
				&GPlatesApi::gml_time_period_set_end_time,
				(bp::arg("time_position")),
				"set_end_time(time_position)\n"
				"  Sets the end time position (time of disappearance) of this property value.\n"
				"\n"
				"  :param time_position: the end time position (time of disappearance)\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		.def("contains",
				&GPlatesApi::gml_time_period_contains,
				(bp::arg("time_position")),
				"contains(time_position)\n"
				"  Determine if a time lies within this time period.\n"
				"\n"
				"  :param time_position: the time position to test\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"  :rtype: bool\n"
				"\n"
				"  .. note:: *time_position* is considered to lie *within* a time period if it "
				"coincides with the :meth:`begin<get_begin_time>` or :meth:`end<get_end_time>` time.\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlTimePeriod>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GmlTimePeriod>();
}


namespace GPlatesApi
{
	namespace
	{
		const GPlatesPropertyValues::GpmlArray::non_null_ptr_type
		gpml_array_create_impl(
				const std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &elements)
		{
			// We need at least one time sample to determine the value type, otherwise we need to
			// ask the python user for it and that might be a little confusing for them.
			if (elements.empty())
			{
				PyErr_SetString(
						PyExc_RuntimeError,
						"A non-empty sequence of PropertyValue elements is required");
				bp::throw_error_already_set();
			}

			return GPlatesPropertyValues::GpmlArray::create(
					elements,
					elements[0]->get_structural_type());
		}
	}

	const GPlatesPropertyValues::GpmlArray::non_null_ptr_type
	gpml_array_create(
			bp::object elements) // Any python sequence (eg, list, tuple).
	{
		// Copy into a vector.
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> elements_vector;
		PythonExtractUtils::extract_iterable(
				elements_vector,
				elements,
				"Expected a sequence of PropertyValue");

		return gpml_array_create_impl(elements_vector);
	}

	GPlatesPropertyValues::GpmlArray::non_null_ptr_type
	gpml_array_create_from_revisioned_vector(
			GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type revisioned_vector,
			const GPlatesPropertyValues::GpmlArray &other_gpml_array)
	{
		// Copy into a vector.
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> elements_vector;
		std::copy(revisioned_vector->begin(), revisioned_vector->end(), std::back_inserter(elements_vector));

		return gpml_array_create_impl(elements_vector);
	}

	GPlatesPropertyValues::GpmlArray::non_null_ptr_type
	gpml_array_return_as_non_null_ptr_type(
			GPlatesPropertyValues::GpmlArray &gpml_array)
	{
		return GPlatesUtils::get_non_null_pointer(&gpml_array);
	}

	GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type
	gpml_array_get_revisioned_vector(
			GPlatesPropertyValues::GpmlArray &gpml_array)
	{
		return GPlatesUtils::get_non_null_pointer(&gpml_array.members());
	}
}

void
export_gpml_array()
{
	const char *const gpml_array_class_name = "GpmlArray";

	std::stringstream gpml_array_class_docstring_stream;
	gpml_array_class_docstring_stream <<
			"A sequence of property value elements.\n"
			"\n"
			<< gpml_array_class_name <<
			" behaves like a regular python ``list`` in that the following operations are supported:\n"
			"\n"
			<< GPlatesApi::get_python_list_operations_docstring(gpml_array_class_name) <<
			"\n"
			"All elements should have the same type (such as :class:`GmlTimePeriod`).\n";

	//
	// GpmlArray - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlArray,
			GPlatesPropertyValues::GpmlArray::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable> gpml_array_class(
					gpml_array_class_name,
					gpml_array_class_docstring_stream.str().c_str(),
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init);

	gpml_array_class
		.def("__init__",
			bp::make_constructor(
					&GPlatesApi::gpml_array_create,
					bp::default_call_policies(),
					(bp::arg("elements"))),
			"__init__(elements)\n"
			"  Create an array from a sequence of property value elements.\n"
			"\n"
			"  :param elements: A sequence of :class:`PropertyValue` elements.\n"
			"  :type elements: Any sequence such as a ``list`` or a ``tuple``\n"
			"  :raises: RuntimeError if sequence is empty\n"
			"\n"
			"  Note that all elements should have the same type (such as :class:`GmlTimePeriod`).\n"
			"\n"
			"  .. note:: The sequence of elements must **not** be empty (for technical implementation reasons), "
			"otherwise a *RuntimeError* exception will be thrown.\n"
			"\n"
			"  ::\n"
			"\n"
			"    array = pygplates.GpmlArray(elements)\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlArray::non_null_ptr_type>())
	;

	// Make 'GpmlArray' look like a python list (RevisionedVector<PropertyValue>).
	GPlatesApi::wrap_python_class_as_revisioned_vector<
			GPlatesPropertyValues::GpmlArray,
			GPlatesModel::PropertyValue,
			GPlatesPropertyValues::GpmlArray::non_null_ptr_type,
			&GPlatesApi::gpml_array_create_from_revisioned_vector,
			&GPlatesApi::gpml_array_return_as_non_null_ptr_type,
			&GPlatesApi::gpml_array_get_revisioned_vector>(
					gpml_array_class,
					gpml_array_class_name);

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlArray>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlArray>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
	gpml_constant_value_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			boost::optional<GPlatesUtils::UnicodeString> description = boost::none)
	{
		// ModelUtils takes care of determining the structural type of the nested property value.
		return GPlatesModel::ModelUtils::create_gpml_constant_value(property_value, description);
	}

	// Select the 'non-const' overload of 'GpmlConstantValue::value()'.
	GPlatesModel::PropertyValue::non_null_ptr_type
	gpml_constant_value_get_value(
			GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
	{
		return gpml_constant_value.value();
	}
}

void
export_gpml_constant_value()
{
	//
	// GpmlConstantValue - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlConstantValue,
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlConstantValue",
					"The most basic case of a time-dependent property value is one "
					"that is constant for all time. The other two types are :class:`GpmlIrregularSampling` "
					"and :class:`GpmlPiecewiseAggregation`. The GPlates Geological Information Model (GPGIM) "
					"defines those properties that are time-dependent (see http://www.gplates.org/gpml.html) and "
					"those that are not. For example, a :class:`GpmlPlateId` property value is used "
					"in *gpml:reconstructionPlateId* properties, of general :class:`feature types<FeatureType>`, and also in "
					"*gpml:relativePlate* properties of motion path features. In the former case "
					"it is expected to be wrapped in a :class:`GpmlConstantValue` while in the latter "
					"case it is not.\n"
					"  ::\n"
					"\n"
					"    reconstruction_plate_id = pygplates.Property(\n"
					"        pygplates.PropertyName.gpml_reconstruction_plate_id,\n"
					"        pygplates.GpmlConstantValue(pygplates.GpmlPlateId(701)))\n"
					"\n"
					"    relative_plate_id = pygplates.Property(\n"
					"        pygplates.PropertyName.gpml_relative_plate,\n"
					"        pygplates.GpmlPlateId(701))\n"
					"\n"
					"If a property is created without a time-dependent wrapper where one is expected, "
					"or vice versa, then you can still save it to a GPML file and a subsequent read "
					"of that file will attempt to correct the property when it is created during "
					"the reading phase (by the GPML file format reader). This usually works for the "
					"simpler :class:`GpmlConstantValue` time-dependent wrapper but does not always "
					"work for the more advanced :class:`GpmlIrregularSampling` and "
					":class:`GpmlPiecewiseAggregation` time-dependent wrapper types.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_constant_value_create,
						bp::default_call_policies(),
						(bp::arg("property_value"),
							bp::arg("description") = boost::optional<GPlatesUtils::UnicodeString>())),
					"__init__(property_value, [description])\n"
					"  Wrap a property value in a time-dependent wrapper that identifies the "
					"property value as constant for all time.\n"
					"\n"
					"  :param property_value: arbitrary property value\n"
					"  :type property_value: :class:`PropertyValue`\n"
					"  :param description: description of this constant value wrapper\n"
					"  :type description: string or None\n"
					"\n"
					"  Optionally provide a description string. If *description* is not specified "
					"then :meth:`get_description` will return ``None``.\n"
					"  ::\n"
					"\n"
					"    constant_property_value = pygplates.GpmlConstantValue(property_value)\n")
		// This is a private method (has leading '_'), and we don't provide a docstring...
		// This method is accessed by pure python API code.
		.def("_get_value",
				&GPlatesApi::gpml_constant_value_get_value)
		.def("set_value",
				&GPlatesPropertyValues::GpmlConstantValue::set_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value of this constant value wrapper.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n")
		.def("get_description",
				&GPlatesPropertyValues::GpmlConstantValue::get_description,
				"get_description()\n"
				"  Returns the *optional* description of this constant value wrapper, or ``None``.\n"
				"\n"
				"  :rtype: string or None\n")
		.def("set_description",
				&GPlatesPropertyValues::GpmlConstantValue::set_description,
				(bp::arg("description") = boost::optional<GPlatesUtils::UnicodeString>()),
				"set_description([description])\n"
				"  Sets the description of this constant value wrapper, or removes it if none specified.\n"
				"\n"
				"  :param description: description of this constant value wrapper\n"
				"  :type description: string or None\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type>())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlConstantValue>();
}


namespace GPlatesApi
{
	GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
	gpml_finite_rotation_create(
			const GPlatesMaths::FiniteRotation &finite_rotation)
	{
		return GPlatesPropertyValues::GpmlFiniteRotation::create(finite_rotation);
	}
}

void
export_gpml_finite_rotation()
{
	//
	// GpmlFiniteRotation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlFiniteRotation,
			GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlFiniteRotation",
					"A property value that represents a finite rotation.",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_finite_rotation_create,
						bp::default_call_policies(),
						(bp::arg("finite_rotation"))),
				"__init__(finite_rotation)\n"
				"  Create a finite rotation property value from a finite rotation.\n"
				"\n"
				"  :param finite_rotation: the finite rotation\n"
				"  :type finite_rotation: :class:`FiniteRotation`\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation_property = pygplates.GpmlFiniteRotation(finite_rotation)\n")
		.def("get_finite_rotation",
				&GPlatesPropertyValues::GpmlFiniteRotation::get_finite_rotation,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_finite_rotation()\n"
				"  Returns the finite rotation.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n")
		.def("set_finite_rotation",
				&GPlatesPropertyValues::GpmlFiniteRotation::set_finite_rotation,
				(bp::arg("finite_rotation")),
				"set_finite_rotation(finite_rotation)\n"
				"  Sets the finite rotation.\n"
				"\n"
				"  :param finite_rotation: the finite rotation\n"
				"  :type finite_rotation: :class:`FiniteRotation`\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlFiniteRotation>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlFiniteRotation>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type
	gpml_finite_rotation_slerp_create()
	{
		return GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(
				GPlatesPropertyValues::StructuralType::create_gpml("FiniteRotation"));
	}
}

void
export_gpml_finite_rotation_slerp()
{
	// Not including GpmlFiniteRotationSlerp since it is not really used (yet) in GPlates and hence
	// is just extra baggage for the python API user (we can add it later though)...
#if 0
	//
	// GpmlFiniteRotationSlerp - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlFiniteRotationSlerp,
			GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type,
			bp::bases<GPlatesPropertyValues::GpmlInterpolationFunction>,
			boost::noncopyable>(
					"GpmlFiniteRotationSlerp",
					"An interpolation function designed to interpolate between finite rotations.\n"
					"\n"
					"There are no (non-static) methods or attributes in this class. The presence of an instance of this "
					"property value is simply intended to signal that interpolation should be Spherical "
					"Linear intERPolation (SLERP). Currently this is the only type of interpolation function "
					"(the only type derived from :class:`GpmlInterpolationFunction`).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(&GPlatesApi::gpml_finite_rotation_slerp_create),
				"__init__()\n"
				"  Create an instance of GpmlFiniteRotationSlerp.\n"
				"  ::\n"
				"\n"
				"    finite_rotation_slerp = pygplates.GpmlFiniteRotationSlerp()\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlFiniteRotationSlerp>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlFiniteRotationSlerp>();
#endif
}


void
export_gpml_hot_spot_trail_mark()
{
#if 0
	// Use the 'non-const' overload so GmlTimeInstant can be modified via python...
	const boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type>
			(GPlatesPropertyValues::GpmlHotSpotTrailMark::*measured_age)() =
					&GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age;

	//
	// GpmlHotSpotTrailMark - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlHotSpotTrailMark,
			GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlHotSpotTrailMark",
					"The marks that define the HotSpotTrail.\n",
					bp::no_init)
		//.def("create", &GPlatesPropertyValues::GpmlHotSpotTrailMark::create)
		//.staticmethod("create")
		//.def("position", &GPlatesPropertyValues::GpmlHotSpotTrailMark::position)
		//.def("set_position", &GPlatesPropertyValues::GpmlHotSpotTrailMark::set_position)
		.def("get_measured_age", measured_age)
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlHotSpotTrailMark>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlHotSpotTrailMark>();
#endif
}


void
export_gpml_interpolation_function()
{
	// Not including GpmlInterpolationFunction since it is not really used (yet) in GPlates and hence
	// is just extra baggage for the python API user (we can add it later though)...
#if 0
	/*
	 * GpmlInterpolationFunction - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base class for interpolation function property values.
	 *
	 * Enables 'isinstance(obj, GpmlInterpolationFunction)' in python - not that it's that useful.
	 */
	bp::class_<
			GPlatesPropertyValues::GpmlInterpolationFunction,
			GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlInterpolationFunction",
					"The base class inherited by all derived *interpolation function* property value classes.\n"
					"\n"
					"The list of derived interpolation function property value classes includes:\n"
					"\n"
					"* :class:`GpmlFiniteRotationSlerp`\n",
					bp::no_init)
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlInterpolationFunction>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlInterpolationFunction>();
#endif
}


namespace GPlatesApi
{
	namespace
	{
		GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
		gpml_irregular_sampling_create_impl(
				const std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type> &time_samples
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				, boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
						interpolation_function = boost::none
#endif
				)
		{
			// We need at least one time sample to determine the value type, otherwise we need to
			// ask the python user for it and that might be a little confusing for them.
			if (time_samples.empty())
			{
				PyErr_SetString(
						PyExc_RuntimeError,
						"A non-empty sequence of GpmlTimeSample elements is required");
				bp::throw_error_already_set();
			}

			return GPlatesPropertyValues::GpmlIrregularSampling::create(
					time_samples,
					// Not including interpolation function since it is not really used (yet) in GPlates and hence
					// is just extra baggage for the python API user (we can add it later though)...
#if 0
					interpolation_function,
#else
					boost::none,
#endif
					time_samples[0]->get_value_type());
		}
	}

	GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
	gpml_irregular_sampling_create(
			bp::object time_samples // Any python sequence (eg, list, tuple).
			// Not including interpolation function since it is not really used (yet) in GPlates and hence
			// is just extra baggage for the python API user (we can add it later though)...
#if 0
			, boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
					interpolation_function = boost::none
#endif
			)
	{
		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type> time_samples_vector;
		PythonExtractUtils::extract_iterable(
				time_samples_vector,
				time_samples,
				"Expected a sequence of GpmlTimeSample");

		return gpml_irregular_sampling_create_impl(time_samples_vector);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>::non_null_ptr_type
	gpml_irregular_sampling_get_time_samples(
			GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
	{
		return &gpml_irregular_sampling.time_samples();
	}

	bp::list
	gpml_irregular_sampling_get_enabled_time_samples(
			GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
	{
		bp::list enabled_time_samples;

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample> &time_samples =
				gpml_irregular_sampling.time_samples();
		BOOST_FOREACH(GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type time_sample, time_samples)
		{
			if (time_sample->is_disabled())
			{
				continue;
			}

			enabled_time_samples.append(time_sample);
		}

		return enabled_time_samples;
	}

	GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
	gpml_irregular_sampling_create_from_revisioned_vector(
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>::non_null_ptr_type revisioned_vector,
			const GPlatesPropertyValues::GpmlIrregularSampling &other_gpml_irregular_sampling)
	{
		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type> time_samples_vector;
		std::copy(revisioned_vector->begin(), revisioned_vector->end(), std::back_inserter(time_samples_vector));

		return gpml_irregular_sampling_create_impl(time_samples_vector);
	}

	GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
	gpml_irregular_sampling_return_as_non_null_ptr_type(
			GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
	{
		return GPlatesUtils::get_non_null_pointer(&gpml_irregular_sampling);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>::non_null_ptr_type
	gpml_irregular_sampling_get_revisioned_vector(
			GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
	{
		return gpml_irregular_sampling_get_time_samples(gpml_irregular_sampling);
	}
}

void
export_gpml_irregular_sampling()
{
	// Not including interpolation function since it is not really used (yet) in GPlates and hence
	// is just extra baggage for the python API user (we can add it later though)...
#if 0
	// Use the 'non-const' overload...
	const boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
			(GPlatesPropertyValues::GpmlIrregularSampling::*get_interpolation_function)() =
					&GPlatesPropertyValues::GpmlIrregularSampling::interpolation_function;
#endif

	const char *const gpml_irregular_sampling_class_name = "GpmlIrregularSampling";

	std::stringstream gpml_irregular_sampling_class_docstring_stream;
	gpml_irregular_sampling_class_docstring_stream <<
			"A time-dependent property consisting of a sequence of time samples irregularly spaced in time.\n"
			"\n"
			<< gpml_irregular_sampling_class_name <<
			" behaves like a regular python ``list`` (of :class:`GpmlTimeSample` elements) in that "
			"the following operations are supported:\n"
			"\n"
			<< GPlatesApi::get_python_list_operations_docstring(gpml_irregular_sampling_class_name) <<
			"\n"
			"For example:\n"
			"::\n"
			"\n"
			"  # Sort samples by time.\n"
			"  irregular_sampling = pygplates.GpmlIrregularSampling([pygplates.GpmlTimeSample(...), ...])\n"
			"  ...\n"
			"  irregular_sampling.sort(key = lambda ts: ts.get_time())\n"
			"\n"
			"In addition to the above ``list``-style operations there are also the following methods...\n";

	//
	// GpmlIrregularSampling - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlIrregularSampling,
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable> gpml_irregular_sampling_class(
					gpml_irregular_sampling_class_name,
					gpml_irregular_sampling_class_docstring_stream.str().c_str(),
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init);

	gpml_irregular_sampling_class
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_irregular_sampling_create,
						bp::default_call_policies(),
						(bp::arg("time_samples")
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
							, bp::arg("interpolation_function") =
								boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>()
#endif
							)),
				"__init__(time_samples"
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				", [interpolation_function]"
#endif
				")\n"
				"  Create an irregularly sampled time-dependent property from a sequence of time samples."
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				" Optionally provide an interpolation function."
#endif
				"\n"
				"\n"
				"  :param time_samples: A sequence of :class:`GpmlTimeSample` elements.\n"
				"  :type time_samples: Any sequence such as a ``list`` or a ``tuple``\n"
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				"  :param interpolation_function: identifies function used to interpolate\n"
				"  :type interpolation_function: an instance derived from :class:`GpmlInterpolationFunction`\n"
#endif
				"  :raises: RuntimeError if time sample sequence is empty\n"
				"\n"
				"  .. note:: The sequence of time samples must **not** be empty (for technical implementation reasons), "
				"otherwise a *RuntimeError* exception will be thrown.\n"
				"\n"
				"  ::\n"
				"\n"
				"    irregular_sampling = pygplates.GpmlIrregularSampling(time_samples)\n")
		.def("get_time_samples",
				&GPlatesApi::gpml_irregular_sampling_get_time_samples,
				"get_time_samples()\n"
				"  Returns the :class:`time samples<GpmlTimeSample>` in a sequence that behaves as a python ``list``.\n"
				"\n"
				"  :rtype: :class:`GpmlTimeSampleList`\n"
				"\n"
				"  Modifying the returned sequence will modify the internal state of the *GpmlIrregularSampling* instance:\n"
				"  ::\n"
				"\n"
				"    time_samples = irregular_sampling.get_time_samples()\n"
				"\n"
				"    # Sort samples by time.\n"
				"    time_samples.sort(key = lambda ts: ts.get_time())\n"
				"\n"
				"  The same effect can be achieved by working directly on the *GpmlIrregularSampling* "
				"instance:\n"
				"  ::\n"
				"\n"
				"    # Sort samples by time.\n"
				"    irregular_sampling.sort(key = lambda ts: ts.get_time())\n")
		.def("get_enabled_time_samples",
				&GPlatesApi::gpml_irregular_sampling_get_enabled_time_samples,
				"get_enabled_time_samples()\n"
				"  Filter out the disabled :class:`time samples<GpmlTimeSample>` and return a list of enabled time samples.\n"
				"\n"
				"  :rtype: list\n"
				"  :return: the list of enabled :class:`time samples<GpmlTimeSample>` (if any)\n"
				"\n"
				"  Returns an empty list if all time samples are disabled.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    return filter(lambda ts: ts.is_enabled(), get_time_samples())\n"
				"\n"
				"  **NOTE:** Unlike :meth:`get_time_samples`, the returned sequence is a new ``list`` object and\n"
				"  hence modifications to the ``list`` such as ``list.sort`` (as opposed to modifications to the list\n"
				"  *elements*) will **not** modify the internal state of the :class:`GpmlIrregularSampling` instance\n"
				"  (it only modifies the returned ``list``).\n")
		.def("get_value_type",
				&GPlatesPropertyValues::GpmlIrregularSampling::get_value_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_value_type()\n"
				"  Returns the type of property value returned by :meth:`get_value`.\n"
				"\n"
				"  For example, it might return ``pygplates.GmlLineString`` which is a *class* object (not an instance).\n"
				"\n"
				"  :rtype: a class object of the property type (derived from :class:`PropertyValue`)\n"
				"\n"
				"  .. versionadded:: 0.21\n")
		// Not including interpolation function since it is not really used (yet) in GPlates and hence
		// is just extra baggage for the python API user (we can add it later though)...
#if 0
		.def("get_interpolation_function",
				get_interpolation_function,
				"get_interpolation_function()\n"
				"  Returns the function used to interpolate between time samples, or ``None``.\n"
				"\n"
				"  :rtype: an instance derived from :class:`GpmlInterpolationFunction`, or ``None``\n")
		.def("set_interpolation_function",
				&GPlatesPropertyValues::GpmlIrregularSampling::set_interpolation_function,
				(bp::arg("interpolation_function") =
					boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>()),
				"set_interpolation_function([interpolation_function])\n"
				"  Sets the function used to interpolate between time samples, "
				"or removes it if none specified.\n"
				"\n"
				"  :param interpolation_function: the function used to interpolate between time samples\n"
				"  :type interpolation_function: an instance derived from :class:`GpmlInterpolationFunction`, or None\n")
#endif
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type>())
	;

	// Make 'GpmlIrregularSampling' look like a python list (RevisionedVector<GpmlTimeSample>).
	GPlatesApi::wrap_python_class_as_revisioned_vector<
			GPlatesPropertyValues::GpmlIrregularSampling,
			GPlatesPropertyValues::GpmlTimeSample,
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type,
			&GPlatesApi::gpml_irregular_sampling_create_from_revisioned_vector,
			&GPlatesApi::gpml_irregular_sampling_return_as_non_null_ptr_type,
			&GPlatesApi::gpml_irregular_sampling_get_revisioned_vector>(
					gpml_irregular_sampling_class,
					gpml_irregular_sampling_class_name);

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlIrregularSampling>();
}


namespace GPlatesApi
{
	/**
	 * The key-value dictionary element value types.
	 */
	typedef boost::variant<
			int,    // 'int' before 'double' since otherwise all 'int's will match, and become, 'double's.
			double,
			GPlatesPropertyValues::TextContent>
					dictionary_element_value_type;


	/**
	 * Visits a property value to retrieve the dictionary element value from it.
	 */
	class GetDictionaryElementValueFromPropertyValueVisitor : 
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		boost::optional<dictionary_element_value_type>
		get_dictionary_element_value_from_property_value(
				const GPlatesModel::PropertyValue &property_value)
		{
			d_dictionary_element_value = boost::none;

			property_value.accept_visitor(*this);

			return d_dictionary_element_value;
		}

	private:

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean)
		{
			d_dictionary_element_value = dictionary_element_value_type(int(xs_boolean.get_value()));
		}

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double)
		{
			d_dictionary_element_value = dictionary_element_value_type(xs_double.get_value());
		}

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger& xs_integer)
		{
			d_dictionary_element_value = dictionary_element_value_type(xs_integer.get_value());
		}

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string)
		{
			d_dictionary_element_value = dictionary_element_value_type(xs_string.get_value());
		}


		boost::optional<dictionary_element_value_type> d_dictionary_element_value;
	};

	/**
	 * Visits a dictionary element value (variant) to wrap a property value around it.
	 */
	class CreatePropertyValueFromDictionaryElementValueVisitor :
			public boost::static_visitor<GPlatesModel::PropertyValue::non_null_ptr_type>
	{
	public:

		GPlatesModel::PropertyValue::non_null_ptr_type
		operator()(
				int value) const
		{
			return GPlatesPropertyValues::XsInteger::create(value);
		}

		GPlatesModel::PropertyValue::non_null_ptr_type
		operator()(
				const double &value) const
		{
			return GPlatesPropertyValues::XsDouble::create(value);
		}

		GPlatesModel::PropertyValue::non_null_ptr_type
		operator()(
				const GPlatesPropertyValues::TextContent &value) const
		{
			return GPlatesPropertyValues::XsString::create(value.get());
		}

	};

	/**
	 * Visits a dictionary element value (variant) and sets it on an existing property value if the correct type.
	 */
	class SetPropertyValueFromDictionaryElementValueVisitor :
			public boost::static_visitor<bool>
	{
	public:

		explicit
		SetPropertyValueFromDictionaryElementValueVisitor(
				GPlatesModel::PropertyValue &property_value) :
			d_property_value(property_value)
		{  }

		bool
		operator()(
				int value) const
		{
			// If property value is integer type then set new value.
			GPlatesPropertyValues::XsInteger *integer_property_value =
					dynamic_cast<GPlatesPropertyValues::XsInteger *>(&d_property_value);
			if (integer_property_value != NULL)
			{
				integer_property_value->set_value(value);
				return true;
			}

			return false;
		}

		bool
		operator()(
				const double &value) const
		{
			// If property value is double type then set new value.
			GPlatesPropertyValues::XsDouble *double_property_value =
					dynamic_cast<GPlatesPropertyValues::XsDouble *>(&d_property_value);
			if (double_property_value != NULL)
			{
				double_property_value->set_value(value);
				return true;
			}

			return false;
		}

		bool
		operator()(
				const GPlatesPropertyValues::TextContent &value) const
		{
			// If property value is string type then set new value.
			GPlatesPropertyValues::XsString *string_property_value =
					dynamic_cast<GPlatesPropertyValues::XsString *>(&d_property_value);
			if (string_property_value != NULL)
			{
				string_property_value->set_value(value);
				return true;
			}

			return false;
		}

	private:

		GPlatesModel::PropertyValue &d_property_value;

	};


	/**
	 * An iterator that checks the index is dereferenceable - we don't use bp::iterator in order
	 * to avoid issues with C++ iterators being invalidated from the C++ side and causing issues
	 * on the python side (eg, removing container elements on the C++ side while iterating over
	 * the container on the python side, for example, via a pyGPlates call back into the C++ code).
	 */
	class GpmlKeyValueDictionaryIterator
	{
	public:

		explicit
		GpmlKeyValueDictionaryIterator(
				GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &elements) :
			d_elements(elements),
			d_index(0)
		{  }

		GpmlKeyValueDictionaryIterator &
		self()
		{
			return *this;
		}

		const GPlatesPropertyValues::TextContent &
		next()
		{
			if (d_index >= d_elements.size())
			{
				PyErr_SetString(PyExc_StopIteration, "No more data.");
				bp::throw_error_already_set();
			}
			return d_elements[d_index++]->key()->get_value();
		}

	private:
		//! Typedef for revisioned vector size type.
		typedef GPlatesModel::RevisionedVector<
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::size_type size_type;

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &d_elements;
		size_type d_index;
	};


	const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
	gpml_key_value_dictionary_create(
			bp::object key_value_mapping_object)
	{
		if (key_value_mapping_object == bp::object()/*Py_None*/)
		{
			// Return empty dictionary.
			return GPlatesPropertyValues::GpmlKeyValueDictionary::create();
		}

		const char *type_error_string = "Expected a 'dict' or a sequence of (string, int or float or string) 2-tuples";

		// Extract the key/value pairs from a Python 'dict' or from a sequence of (key, value) tuples.
		PythonExtractUtils::key_value_map_type key_value_map;
		PythonExtractUtils::extract_key_value_map(key_value_map, key_value_mapping_object, type_error_string);

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type> elements;

		PythonExtractUtils::key_value_map_type::const_iterator key_value_map_iter = key_value_map.begin();
		PythonExtractUtils::key_value_map_type::const_iterator key_value_map_end = key_value_map.end();
		for ( ; key_value_map_iter != key_value_map_end; ++key_value_map_iter)
		{
			const PythonExtractUtils::key_value_type &key_value = *key_value_map_iter;

			// Extract the key.
			bp::extract<GPlatesPropertyValues::TextContent> extract_key(key_value.first);
			if (!extract_key.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}
			const GPlatesPropertyValues::TextContent key = extract_key();

			// Make sure the each element key only occurs once - otherwise remove duplicate (similar behaviour to 'dict').
			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type>::iterator
					elements_iter = elements.begin();
			const std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type>::iterator
					elements_end = elements.end();
			for ( ; elements_iter != elements_end; ++elements_iter)
			{
				const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element = *elements_iter->get();

				const GPlatesPropertyValues::TextContent &element_key = element.key()->get_value();
				if (key == element_key)
				{
					elements.erase(elements_iter);
					break;
				}
			}

			// Extract the value.
			bp::extract<dictionary_element_value_type> extract_value(key_value.second);
			if (!extract_value.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}
			const dictionary_element_value_type value = extract_value();

			// Convert value to a property value.
			const GPlatesModel::PropertyValue::non_null_ptr_type property_value =
					boost::apply_visitor(
							CreatePropertyValueFromDictionaryElementValueVisitor(),
							value);

			// Add a key/value element to the dictionary.
			elements.push_back(
					GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
							GPlatesPropertyValues::XsString::create(key.get()),
							property_value,
							property_value->get_structural_type()));
		}

		return GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements);
	}

	GpmlKeyValueDictionaryIterator
	gpml_key_value_dictionary_get_iter(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
	{
		return GpmlKeyValueDictionaryIterator(gpml_key_value_dictionary.elements());
	}

	unsigned int
	gpml_key_value_dictionary_len(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
	{
		return gpml_key_value_dictionary.elements().size();
	}

	bool
	gpml_key_value_dictionary_contains_key(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary,
			const GPlatesPropertyValues::TextContent &key)
	{
		const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &elements =
				gpml_key_value_dictionary.elements();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator
				elements_iter = elements.begin(),
				elements_end = elements.end();

		// Search for the element associated with 'key'.
		for ( ; elements_iter != elements_end ; ++elements_iter)
		{
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element = **elements_iter;

			const GPlatesPropertyValues::TextContent &element_key = element.key()->get_value();
			if (key == element_key)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Return a dictionary element value as an integer, float or string or a default value.
	 */
	bp::object
	gpml_key_value_dictionary_get(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary,
			const GPlatesPropertyValues::TextContent &key,
			bp::object default_value)
	{
		const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &elements =
				gpml_key_value_dictionary.elements();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator
				elements_iter = elements.begin(),
				elements_end = elements.end();

		// Search for the element associated with 'key'.
		for ( ; elements_iter != elements_end ; ++elements_iter)
		{
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element = **elements_iter;

			const GPlatesPropertyValues::TextContent &element_key = element.key()->get_value();
			if (key == element_key)
			{
				// Get the int, float or string value from the property value.
				GetDictionaryElementValueFromPropertyValueVisitor visitor;
				boost::optional<dictionary_element_value_type> value =
						visitor.get_dictionary_element_value_from_property_value(*element.value());
				// Could be boost::none but normally shouldn't since shapefile attributes should
				// always be int, float or string.
				if (!value)
				{
					return default_value;
				}

				return bp::object(value.get());
			}
		}

		return default_value;
	}

	void
	gpml_key_value_dictionary_set(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary,
			const GPlatesPropertyValues::TextContent &key,
			const dictionary_element_value_type &value)
	{
		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &elements =
				gpml_key_value_dictionary.elements();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator
				elements_iter = elements.begin(),
				elements_end = elements.end();

		// Search for the element associated with 'key'.
		for ( ; elements_iter != elements_end ; ++elements_iter)
		{
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element = **elements_iter;

			const GPlatesPropertyValues::TextContent &element_key = element.key()->get_value();
			if (key == element_key)
			{
				// Set the int, float or string value on the property value (if correct) type.
				SetPropertyValueFromDictionaryElementValueVisitor visitor(*element.value());
				if (boost::apply_visitor(visitor, value))
				{
					// The value was successfully set on the property value so nothing left to do.
					return;
				}

				// The property value was the wrong type so remove it (we'll add a new one after the loop).
				elements.erase(elements_iter);
				break;
			}
		}

		//
		// 'key' was not found so create a new property value and add a new element.
		//

		const GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				boost::apply_visitor(
						CreatePropertyValueFromDictionaryElementValueVisitor(),
						value);

		elements.push_back(
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
						GPlatesPropertyValues::XsString::create(key.get()),
						property_value,
						property_value->get_structural_type()));
	}

	void
	gpml_key_value_dictionary_remove(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary,
			const GPlatesPropertyValues::TextContent &key)
	{
		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &elements =
				gpml_key_value_dictionary.elements();

		GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator
				elements_iter = elements.begin(),
				elements_end = elements.end();

		// Search for the element associated with 'key'.
		for ( ; elements_iter != elements_end ; ++elements_iter)
		{
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element = **elements_iter;

			const GPlatesPropertyValues::TextContent &element_key = element.key()->get_value();
			if (key == element_key)
			{
				// Remove the element and return.
				elements.erase(elements_iter);
				return;
			}
		}
	}
}

void
export_gpml_key_value_dictionary()
{
	// Iterator over key value dictionary elements.
	// Note: We don't docstring this - it's not an interface the python user needs to know about.
	bp::class_<GPlatesApi::GpmlKeyValueDictionaryIterator>(
			// Prefix with '_' so users know it's an implementation detail (they should not be accessing it directly).
			"_GpmlKeyValueDictionaryIterator",
			bp::no_init)
		.def("__iter__", &GPlatesApi::GpmlKeyValueDictionaryIterator::self, bp::return_value_policy<bp::copy_non_const_reference>())
		.def(
#if PY_MAJOR_VERSION < 3
				"next",
#else
				"__next__",
#endif
				&GPlatesApi::GpmlKeyValueDictionaryIterator::next,
				bp::return_value_policy<bp::copy_const_reference>())
	;


	//
	// GpmlKeyValueDictionary - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlKeyValueDictionary,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlKeyValueDictionary",
					"A dictionary of key/value pairs that associates string keys with integer, float "
					"or string values.\n"
					"\n"
					"This is typically used to store attributes imported from a Shapefile so that they "
					"are available for querying and can be written back out when saving to Shapefile.\n"
					"\n"
					"The following operations are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(d)``                  number of elements in dictionary *d*\n"
					"``for k in d``              iterates over the keys *k* in dictionary *d*\n"
					"``k in d``                  ``True`` if *k* is a key in dictionary *d*\n"
					"``k not in d``              ``False`` if *k* is a key in dictionary *d*\n"
					"=========================== ==========================================================\n"
					"\n"
					"For example:\n"
					"::\n"
					"\n"
					"  for key in dictionary:\n"
					"      value = dictionary.get(key)\n"
					"\n"
					"The following methods support getting, setting and removing elements in a dictionary:\n"
					"\n"
					"* :meth:`get`\n"
					"* :meth:`set`\n"
					"* :meth:`remove`\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_key_value_dictionary_create,
						bp::default_call_policies(),
						(bp::arg("key_value_mapping") = bp::object()/*Py_None*/)),
				"__init__([key_value_mapping])\n"
				"  Create a dictionary containing zero or more key/value pairs.\n"
				"\n"
				"  :param key_value_mapping: optional mapping of keys to values\n"
				"  :type key_value_mapping: ``dict`` mapping each key (string) to a value "
				"(integer, float or string), or a sequence of (key, value) tuples, or None\n"
				"\n"
				"  To create an empty dictionary:\n"
				"  ::\n"
				"\n"
				"    dictionary = pygplates.GpmlKeyValueDictionary()\n"
				"\n"
				"  To create a dictionary with two key/value pairs:\n"
				"  ::\n"
				"\n"
				"    dictionary = pygplates.GpmlKeyValueDictionary(\n"
				"        [('name', 'Test'), ('id', 23)])\n"
				"\n"
				"  To do the same thing using a ``dict``:\n"
				"  ::\n"
				"\n"
				"    dictionary = pygplates.GpmlKeyValueDictionary(\n"
				"        {'name' : 'Test', 'id' : 23})\n")
		.def("__iter__", &GPlatesApi::gpml_key_value_dictionary_get_iter)
		.def("__len__", &GPlatesApi::gpml_key_value_dictionary_len)
		.def("__contains__", &GPlatesApi::gpml_key_value_dictionary_contains_key)
		.def("get",
				&GPlatesApi::gpml_key_value_dictionary_get,
				(bp::arg("key"),
						bp::arg("default_value") = bp::object()/*Py_None*/),
				"get(key, [default_value])\n"
				"  Returns the value of the dictionary element associated with a key.\n"
				"\n"
				"  :param key: the key of the dictionary element\n"
				"  :type key: string\n"
				"  :param default_value: the default value to return if the key does not exist in the "
				"dictionary (if not specified then it defaults to None)\n"
				"  :type default_value: int or float or string or None\n"
				"  :returns: the value associated with *key*, otherwise *default_value* if *key* does not exist\n"
				"  :rtype: integer or float or string or type(*default_value*) or None\n"
				"\n"
				"  To test if a key is present and retrieve its value:\n"
				"  ::\n"
				"\n"
				"    value = dictionary.get('key')\n"
				"    # Compare with None since an integer (or float) value of zero, or an empty string, evaluates to False.\n"
				"    if value is not None:\n"
				"    ...\n"
				"    # ...or a less efficient approach...\n"
				"    if 'key' in dictionary:\n"
				"        value = dictionary.get('key')\n"
				"\n"
				"  Return the integer value of the attribute associated with 'key' (default to zero if not present):\n"
				"  ::\n"
				"\n"
				"    integer_value = dictionary.get('key', 0)\n")
		.def("set",
				&GPlatesApi::gpml_key_value_dictionary_set,
				(bp::arg("key"),
						bp::arg("value")),
				"set(key, value)\n"
				"  Sets the value of the dictionary element associated with a key.\n"
				"\n"
				"  :param key: the key of the dictionary element\n"
				"  :type key: string\n"
				"  :param value: the value of the dictionary element\n"
				"  :type value: integer, float or string\n"
				"\n"
				"  If there is no dictionary element associated with *key* then a new element is created, "
				"  otherwise the existing element is modified.\n")
		.def("remove",
				&GPlatesApi::gpml_key_value_dictionary_remove,
				(bp::arg("key")),
				"remove(key)\n"
				"  Removes the dictionary element associated with a key.\n"
				"\n"
				"  :param key: the key of the dictionary element to remove\n"
				"  :type key: string\n"
				"\n"
				"  If *key* does not exist in the dictionary then it is ignored and nothing is done.\n")
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlKeyValueDictionary>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlKeyValueDictionary>();

	// Register the dictionary element value types.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::dictionary_element_value_type>();
	// Enable boost::optional<dictionary_element_value_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::dictionary_element_value_type>();
}

void
export_gpml_old_plates_header()
{
	//
	// GpmlOldPlatesHeader - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlOldPlatesHeader,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlOldPlatesHeader",
					"A property value containing metadata inherited from imported PLATES data files.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::GpmlOldPlatesHeader::create,
						bp::default_call_policies(),
						(bp::arg("region_number"),
								bp::arg("reference_number"),
								bp::arg("string_number"),
								bp::arg("geographic_description"),
								bp::arg("plate_id_number"),
								bp::arg("age_of_appearance"),
								bp::arg("age_of_disappearance"),
								bp::arg("data_type_code"),
								bp::arg("data_type_code_number"),
								bp::arg("data_type_code_number_additional"),
								bp::arg("conjugate_plate_id_number"),
								bp::arg("colour_code"),
								bp::arg("number_of_points"))),
				"__init__(region_number, "
				"reference_number, "
				"string_number, "
				"geographic_description, "
				"plate_id_number, "
				"age_of_appearance, "
				"age_of_disappearance, "
				"data_type_code, "
				"data_type_code_number, "
				"data_type_code_number_additional, "
				"conjugate_plate_id_number, "
				"colour_code, "
				"number_of_points)\n"
				"  Create a property value containing PLATES metadata.\n"
				"\n"
				"  :param region_number: region number\n"
				"  :type region_number: int\n"
				"  :param reference_number: reference number\n"
				"  :type reference_number: int\n"
				"  :param string_number: string number\n"
				"  :type string_number: int\n"
				"  :param geographic_description: geographic description\n"
				"  :type geographic_description: string\n"
				"  :param plate_id_number: plate id number\n"
				"  :type plate_id_number: float\n"
				"  :param age_of_appearance: age of appearance\n"
				"  :type age_of_appearance: float\n"
				"  :param age_of_disappearance: age of disappearance\n"
				"  :type age_of_disappearance: int\n"
				"  :param data_type_code: data type code\n"
				"  :type data_type_code: string\n"
				"  :param data_type_code_number: data type code number\n"
				"  :type data_type_code_number: int\n"
				"  :param data_type_code_number_additional: data type code number additional\n"
				"  :type data_type_code_number_additional: string\n"
				"  :param conjugate_plate_id_number: conjugate plate id number\n"
				"  :type conjugate_plate_id_number: int\n"
				"  :param colour_code: colour code\n"
				"  :type colour_code: int\n"
				"  :param number_of_points: number of points - not counting the final 'terminator' "
				"point (99.0000,99.0000)\n"
				"  :type number_of_points: int\n")
		.def("get_region_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_region_number,
				"get_region_number()\n"
				"  Returns the region number.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_region_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_region_number,
				(bp::arg("region_number")),
				"set_region_number(region_number)\n"
				"  Sets the region number.\n"
				"\n"
				"  :param region_number: region number\n"
				"  :type region_number: int\n")
		.def("get_reference_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_reference_number,
				"get_reference_number()\n"
				"  Returns the reference number.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_reference_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_reference_number,
				(bp::arg("reference_number")),
				"set_reference_number(reference_number)\n"
				"  Sets the reference number.\n"
				"\n"
				"  :param reference_number: reference number\n"
				"  :type reference_number: int\n")
		.def("get_string_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_string_number,
				"get_string_number()\n"
				"  Returns the string number.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_string_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_string_number,
				(bp::arg("string_number")),
				"set_string_number(string_number)\n"
				"  Sets the string number.\n"
				"\n"
				"  :param string_number: string number\n"
				"  :type string_number: int\n")
		.def("get_geographic_description",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_geographic_description,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_geographic_description()\n"
				"  Returns the geographic description.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_geographic_description",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_geographic_description,
				(bp::arg("geographic_description")),
				"set_geographic_description(geographic_description)\n"
				"  Sets the geographic description.\n"
				"\n"
				"  :param geographic_description: geographic description\n"
				"  :type geographic_description: string\n")
		.def("get_plate_id_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_plate_id_number,
				"get_plate_id_number()\n"
				"  Returns the plate id number.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_plate_id_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_plate_id_number,
				(bp::arg("plate_id_number")),
				"set_plate_id_number(plate_id_number)\n"
				"  Sets the plate id number.\n"
				"\n"
				"  :param plate_id_number: plate id number\n"
				"  :type plate_id_number: int\n")
		.def("get_age_of_appearance",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_age_of_appearance,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_age_of_appearance()\n"
				"  Returns the age of appearance.\n"
				"\n"
				"  :rtype: float\n")
		.def("set_age_of_appearance",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_age_of_appearance,
				(bp::arg("age_of_appearance")),
				"set_age_of_appearance(age_of_appearance)\n"
				"  Sets the age of appearance.\n"
				"\n"
				"  :param age_of_appearance: age of appearance\n"
				"  :type age_of_appearance: float\n")
		.def("get_age_of_disappearance",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_age_of_disappearance,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_age_of_disappearance()\n"
				"  Returns the age of disappearance.\n"
				"\n"
				"  :rtype: float\n")
		.def("set_age_of_disappearance",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_age_of_disappearance,
				(bp::arg("age_of_disappearance")),
				"set_age_of_disappearance(age_of_disappearance)\n"
				"  Sets the age of disappearance.\n"
				"\n"
				"  :param age_of_disappearance: age of disappearance\n"
				"  :type age_of_disappearance: float\n")
		.def("get_data_type_code",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_data_type_code,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_data_type_code()\n"
				"  Returns the data type code.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_data_type_code",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_data_type_code,
				(bp::arg("data_type_code")),
				"set_data_type_code(data_type_code)\n"
				"  Sets the data type code.\n"
				"\n"
				"  :param data_type_code: data type code\n"
				"  :type data_type_code: string\n")
		.def("get_data_type_code_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_data_type_code_number,
				"get_data_type_code_number()\n"
				"  Returns the data type code number.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_data_type_code_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_data_type_code_number,
				(bp::arg("data_type_code_number")),
				"set_data_type_code_number(data_type_code_number)\n"
				"  Sets the data type code number.\n"
				"\n"
				"  :param data_type_code_number: data type code number\n"
				"  :type data_type_code_number: int\n")
		.def("get_data_type_code_number_additional",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_data_type_code_number_additional,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_data_type_code_number_additional()\n"
				"  Returns the data type code number additional.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_data_type_code_number_additional",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_data_type_code_number_additional,
				(bp::arg("data_type_code_number_additional")),
				"set_data_type_code_number_additional(data_type_code_number_additional)\n"
				"  Sets the data type code number additional.\n"
				"\n"
				"  :param data_type_code_number_additional: data type code number additional\n"
				"  :type data_type_code_number_additional: string\n")
		.def("get_conjugate_plate_id_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_conjugate_plate_id_number,
				"get_conjugate_plate_id_number()\n"
				"  Returns the conjugate plate id number.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_conjugate_plate_id_number",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_conjugate_plate_id_number,
				(bp::arg("conjugate_plate_id_number")),
				"set_conjugate_plate_id_number(conjugate_plate_id_number)\n"
				"  Sets the conjugate plate id number.\n"
				"\n"
				"  :param conjugate_plate_id_number: conjugate plate id number\n"
				"  :type conjugate_plate_id_number: int\n")
		.def("get_colour_code",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_colour_code,
				"get_colour_code()\n"
				"  Returns the colour code.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_colour_code",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_colour_code,
				(bp::arg("colour_code")),
				"set_colour_code(colour_code)\n"
				"  Sets the colour code.\n"
				"\n"
				"  :param colour_code: colour code\n"
				"  :type colour_code: int\n")
		.def("get_number_of_points",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::get_number_of_points,
				"get_number_of_points()\n"
				"  Returns the number of points.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_number_of_points",
				&GPlatesPropertyValues::GpmlOldPlatesHeader::set_number_of_points,
				(bp::arg("number_of_points")),
				"set_number_of_points(number_of_points)\n"
				"  Sets the number of points.\n"
				"\n"
				"  :param number_of_points: number of points\n"
				"  :type number_of_points: int\n")
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlOldPlatesHeader>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlOldPlatesHeader>();
}

namespace GPlatesApi
{
	namespace
	{
		GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
		gpml_piecewise_aggregation_create_impl(
				const std::vector<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type> &time_windows)
		{
			// We need at least one time sample to determine the value type, otherwise we need to
			// ask the python user for it and that might be a little confusing for them.
			if (time_windows.empty())
			{
				PyErr_SetString(
						PyExc_RuntimeError,
						"A non-empty sequence of GpmlTimeWindow elements is required");
				bp::throw_error_already_set();
			}

			return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(
					time_windows,
					time_windows[0]->get_value_type());
		}
	}

	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
	gpml_piecewise_aggregation_create(
			bp::object time_windows) // Any python sequence (eg, list, tuple).
	{
		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type> time_windows_vector;
		PythonExtractUtils::extract_iterable(
				time_windows_vector,
				time_windows,
				"Expected a sequence of GpmlTimeWindow");

		return gpml_piecewise_aggregation_create_impl(time_windows_vector);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::non_null_ptr_type
	gpml_piecewise_aggregation_get_time_windows(
			GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
	{
		return &gpml_piecewise_aggregation.time_windows();
	}

	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
	gpml_piecewise_aggregation_create_from_revisioned_vector(
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::non_null_ptr_type revisioned_vector,
			const GPlatesPropertyValues::GpmlPiecewiseAggregation &other_gpml_piecewise_aggregation)
	{
		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type> time_windows_vector;
		std::copy(revisioned_vector->begin(), revisioned_vector->end(), std::back_inserter(time_windows_vector));

		return gpml_piecewise_aggregation_create_impl(time_windows_vector);
	}

	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
	gpml_piecewise_aggregation_return_as_non_null_ptr_type(
			GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
	{
		return GPlatesUtils::get_non_null_pointer(&gpml_piecewise_aggregation);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::non_null_ptr_type
	gpml_piecewise_aggregation_get_revisioned_vector(
			GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
	{
		return gpml_piecewise_aggregation_get_time_windows(gpml_piecewise_aggregation);
	}
}

void
export_gpml_piecewise_aggregation()
{
	const char *const gpml_piecewise_aggregation_class_name = "GpmlPiecewiseAggregation";

	std::stringstream gpml_piecewise_aggregation_class_docstring_stream;
	gpml_piecewise_aggregation_class_docstring_stream <<
			"A time-dependent property consisting of a sequence of time windows each with a *constant* property value.\n"
			"\n"
			<< gpml_piecewise_aggregation_class_name <<
			" behaves like a regular python ``list`` (of :class:`GpmlTimeWindow` elements) in that "
			"the following operations are supported:\n"
			"\n"
			<< GPlatesApi::get_python_list_operations_docstring(gpml_piecewise_aggregation_class_name) <<
			"\n"
			"For example:\n"
			"::\n"
			"\n"
			"  # Replace the second and third time windows with a new time window that spans both.\n"
			"  piecewise_aggregation = pygplates.GpmlPiecewiseAggregation([pygplates.GpmlTimeWindow(...), ...])\n"
			"  ...\n"
			"  piecewise_aggregation[1:3] = [\n"
			"      pygplates.GpmlTimeWindow(\n"
			"          new_property_value,\n"
			"          piecewise_aggregation[2].get_begin_time(),\n"
			"          piecewise_aggregation[1].get_end_time())]\n"
			"\n"
			"In addition to the above ``list``-style operations there are also the following methods...\n";

	//
	// GpmlPiecewiseAggregation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPiecewiseAggregation,
			GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable> gpml_piecewise_aggregation_class(
					gpml_piecewise_aggregation_class_name,
					gpml_piecewise_aggregation_class_docstring_stream.str().c_str(),
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init);

	gpml_piecewise_aggregation_class
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_piecewise_aggregation_create,
						bp::default_call_policies(),
						(bp::arg("time_windows"))),
				"__init__(time_windows)\n"
				"  Create a piecewise-constant time-dependent property from a sequence of time windows.\n"
				"\n"
				"  :param time_windows: A sequence of :class:`GpmlTimeWindow` elements.\n"
				"  :type time_windows: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :raises: RuntimeError if time window sequence is empty\n"
				"\n"
				"  .. note:: The sequence of time windows must **not** be empty (for technical implementation reasons), "
				"otherwise a *RuntimeError* exception will be thrown.\n"
				"\n"
				"  ::\n"
				"\n"
				"    piecewise_aggregation = pygplates.GpmlPiecewiseAggregation(time_windows)\n")
		.def("get_time_windows",
				&GPlatesApi::gpml_piecewise_aggregation_get_time_windows,
				"get_time_windows()\n"
				"  Returns the :class:`time windows<GpmlTimeWindow>` in a sequence that behaves as a python ``list``.\n"
				"\n"
				"  :rtype: :class:`GpmlTimeWindowList`\n"
				"\n"
				"  Modifying the returned sequence will modify the internal state of the *GpmlPiecewiseAggregation* instance:\n"
				"  ::\n"
				"\n"
				"    time_windows = piecewise_aggregation.get_time_windows()\n"
				"\n"
				"    # Sort windows by begin time\n"
				"    time_windows.sort(key = lambda tw: tw.get_begin_time())\n"
				"\n"
				"  The same effect can be achieved by working directly on the *GpmlPiecewiseAggregation* "
				"instance:\n"
				"  ::\n"
				"\n"
				"    # Sort windows by begin time.\n"
				"    piecewise_aggregation.sort(key = lambda tw: tw.get_begin_time())\n")
		.def("get_value_type",
				&GPlatesPropertyValues::GpmlPiecewiseAggregation::get_value_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_value_type()\n"
				"  Returns the type of property value returned by :meth:`get_value`.\n"
				"\n"
				"  For example, it might return ``pygplates.GmlLineString`` which is a *class* object (not an instance).\n"
				"\n"
				"  :rtype: a class object of the property type (derived from :class:`PropertyValue`)\n"
				"\n"
				"  .. versionadded:: 0.21\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type>())
	;

	// Make 'GpmlPiecewiseAggregation' look like a python list (RevisionedVector<GpmlTimeWindow>).
	GPlatesApi::wrap_python_class_as_revisioned_vector<
			GPlatesPropertyValues::GpmlPiecewiseAggregation,
			GPlatesPropertyValues::GpmlTimeWindow,
			GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type,
			&GPlatesApi::gpml_piecewise_aggregation_create_from_revisioned_vector,
			&GPlatesApi::gpml_piecewise_aggregation_return_as_non_null_ptr_type,
			&GPlatesApi::gpml_piecewise_aggregation_get_revisioned_vector>(
					gpml_piecewise_aggregation_class,
					gpml_piecewise_aggregation_class_name);

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlPiecewiseAggregation>();
}


void
export_gpml_plate_id()
{
	//
	// GpmlPlateId - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlPlateId",
					"A property value that represents a plate id. A plate id is an integer that "
					"identifies a particular tectonic plate and is typically used to look up a "
					"rotation in a :class:`ReconstructionTree`.",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::GpmlPlateId::create,
						bp::default_call_policies(),
						(bp::arg("plate_id"))),
				"__init__(plate_id)\n"
				"  Create a plate id property value from an integer plate id.\n"
				"\n"
				"  :param plate_id: integer plate id\n"
				"  :type plate_id: int\n"
				"\n"
				"  ::\n"
				"\n"
				"    plate_id_property = pygplates.GpmlPlateId(plate_id)\n")
		.def("get_plate_id",
				&GPlatesPropertyValues::GpmlPlateId::get_value,
				"get_plate_id()\n"
				"  Returns the integer plate id.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_plate_id",
				&GPlatesPropertyValues::GpmlPlateId::set_value,
				(bp::arg("plate_id")),
				"set_plate_id(plate_id)\n"
				"  Sets the integer plate id.\n"
				"\n"
				"  :param plate_id: integer plate id\n"
				"  :type plate_id: int\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlPlateId>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlPlateId>();
}


namespace GPlatesApi
{
	namespace
	{
		void
		verify_era(
				const QString &era)
		{
			if (era != "Cenozoic" &&
				era != "Mesozoic")
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The era '") + era +
								"' was not recognised as a valid value by the GPGIM");
			}
		}
	}

	const GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type
	gpml_polarity_chron_id_create(
			boost::optional<QString> era,
			boost::optional<unsigned int> major_region,
			boost::optional<QString> minor_region,
			VerifyInformationModel::Value verify_information_model)
	{
		if (verify_information_model == VerifyInformationModel::YES)
		{
			if (era)
			{
				verify_era(era.get());
			}
		}

		return GPlatesPropertyValues::GpmlPolarityChronId::create(era, major_region, minor_region);
	}

	void
	gpml_polarity_chron_id_set_era(
			GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id,
			const QString &era,
			VerifyInformationModel::Value verify_information_model)
	{
		if (verify_information_model == VerifyInformationModel::YES)
		{
			verify_era(era);
		}

		gpml_polarity_chron_id.set_era(era);
	}
}


void
export_gpml_polarity_chron_id()
{
	//
	// GpmlPolarityChronId - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPolarityChronId,
			GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlPolarityChronId",
					"A property value that identifies an :class:`Isochron<FeatureType>` or "
					":class:`MagneticAnomalyIdentification<FeatureType>`.",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_polarity_chron_id_create,
						bp::default_call_policies(),
						(bp::arg("era") = boost::optional<QString>(),
								bp::arg("major_region") = boost::optional<unsigned int>(),
								bp::arg("minor_region") = boost::optional<QString>(),
								bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES)),
				"__init__([era], [major_region], [minor_region], [verify_information_model=VerifyInformationModel.yes])\n"
				"  Create a polarity chron id property value.\n"
				"\n"
				"  :param era: the era of the chron ('Cenozoic' or 'Mesozoic')\n"
				"  :type era: string\n"
				"  :param major_region: the number indicating the major region the chron is in - "
				"Cenozoic isochrons have been classified into broad regions identified by the numbers 1 to 34, "
				"Mesozoic isochrons use the numbers 1 to 29\n"
				"  :type major_region: int\n"
				"  :param minor_region: the sequence of letters indicating the sub-region the chron is "
				"located in - the letters a-z are used for the initial sub-region, and if further polarity "
				"reversals have been discovered within that chron, a second letter is appended, and so on\n"
				"  :type minor_region: string\n"
				"  :param verify_information_model: whether to check the information model for valid *era*\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *era* is not a recognised era value\n"
				"\n"
				"  ::\n"
				"\n"
				"    # Create the identifier 'C34ad' for Cenozoic isochron, major region 34, sub region a, sub region d:\n"
				"    polarity_chron_id_property = pygplates.GpmlPolarityChronId('Cenozoic', 34, 'ad')\n")
		.def("get_era",
				&GPlatesPropertyValues::GpmlPolarityChronId::get_era,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_era()\n"
				"  Returns the era.\n"
				"\n"
				"  :returns: the era, or None if the era was not initialised\n"
				"  :rtype: string or None\n")
		.def("set_era",
				&GPlatesApi::gpml_polarity_chron_id_set_era,
				(bp::arg("era"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"set_era(era, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Sets the era.\n"
				"\n"
				"  :param era: the era of the chron ('Cenozoic' or 'Mesozoic')\n"
				"  :type era: string\n"
				"  :param verify_information_model: whether to check the information model for valid *era*\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *era* is not a recognised era string value\n")
		.def("get_major_region",
				&GPlatesPropertyValues::GpmlPolarityChronId::get_major_region,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_major_region()\n"
				"  Returns the major region.\n"
				"\n"
				"  :returns: the major region, or None if the major region was not initialised\n"
				"  :rtype: int or None\n")
		.def("set_major_region",
				&GPlatesPropertyValues::GpmlPolarityChronId::set_major_region,
				(bp::arg("major_region")),
				"set_major_region(major_region)\n"
				"  Sets the major region.\n"
				"\n"
				"  :param major_region: the number indicating the major region the chron is in - "
				"Cenozoic isochrons have been classified into broad regions identified by the numbers 1 to 34, "
				"Mesozoic isochrons use the numbers 1 to 29\n"
				"  :type major_region: int\n")
		.def("get_minor_region",
				&GPlatesPropertyValues::GpmlPolarityChronId::get_minor_region,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_minor_region()\n"
				"  Returns the minor region.\n"
				"\n"
				"  :returns: the minor region, or None if the minor region was not initialised\n"
				"  :rtype: string or None\n")
		.def("set_minor_region",
				&GPlatesPropertyValues::GpmlPolarityChronId::set_minor_region,
				(bp::arg("minor_region")),
				"set_minor_region(minor_region)\n"
				"  Sets the minor region.\n"
				"\n"
				"  :param minor_region: the sequence of letters indicating the sub-region the chron is "
				"located in - the letters a-z are used for the initial sub-region, and if further polarity "
				"reversals have been discovered within that chron, a second letter is appended, and so on\n"
				"  :type minor_region: string\n")
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlPolarityChronId>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlPolarityChronId>();
}

void
export_gpml_property_delegate()
{
	//
	// GpmlPropertyDelegate - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPropertyDelegate,
			GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlPropertyDelegate",
					"A property value that represents a reference, or delegation, to a property in another feature.\n"
					"\n"
					"  .. versionadded:: 0.21\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::GpmlPropertyDelegate::create,
						bp::default_call_policies(),
						(bp::arg("feature_id"),
							bp::arg("property_name"),
							bp::arg("property_type"))),
				"__init__(feature_id, property_name, property_type)\n"
				"  Create a reference, or delegation, to a property in another feature.\n"
				"\n"
				"  :param feature_id: the referenced feature\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param property_name: the name of the referenced property\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_type: the type of the referenced property\n"
				"  :type property_type: a class object of a property type (derived from :class:`PropertyValue`) "
			"except :class:`Enumeration` and time-dependent wrappers :class:`GpmlConstantValue`, "
			":class:`GpmlIrregularSampling` and :class:`GpmlPiecewiseAggregation`.\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_delegate = pygplates.GpmlPropertyDelegate(\n"
				"        referenced_feature.get_feature_id(),\n"
				"        pygplates.PropertyName.gpml_center_line_of,\n"
				"        pygplates.GmlLineString)\n")
		.def("get_feature_id",
				&GPlatesPropertyValues::GpmlPropertyDelegate::get_feature_id,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_feature_id()\n"
				"  Returns the feature ID of the feature containing the delegated property.\n"
				"\n"
				"  :rtype: :class:`FeatureId`\n")
		.def("get_property_name",
				&GPlatesPropertyValues::GpmlPropertyDelegate::get_target_property_name,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_property_name()\n"
				"  Returns the property name of the delegated property.\n"
				"\n"
				"  :rtype: :class:`PropertyName`\n")
		.def("get_property_type",
				&GPlatesPropertyValues::GpmlPropertyDelegate::get_value_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_property_type()\n"
				"  Returns the property type of the delegated property.\n"
				"\n"
				"  For example, it might return ``pygplates.GmlLineString`` which is a *class* object (not an instance).\n"
				"\n"
				"  :rtype: a class object of the property type (derived from :class:`PropertyValue`)\n")
		;

	// Create a python class "GpmlPropertyDelegateList" for RevisionedVector<GpmlPropertyDelegate> that behaves like a list of GpmlPropertyDelegate.
	//
	// This enables the RevisionedVector<GpmlPropertyDelegate> returned by 'GpmlTopologicalNetwork.get_interiors()' to be treated as a list.
	GPlatesApi::create_python_class_as_revisioned_vector<GPlatesPropertyValues::GpmlPropertyDelegate>("GpmlPropertyDelegate");

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlPropertyDelegate>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlPropertyDelegate>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type
	gpml_time_sample_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			const GPlatesPropertyValues::GeoTimeInstant &time,
			boost::optional<GPlatesPropertyValues::TextContent> description,
			bool is_enabled)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> description_xs_string;
		if (description)
		{
			description_xs_string = GPlatesPropertyValues::XsString::create(description->get());
		}

		return GPlatesPropertyValues::GpmlTimeSample::create(
				property_value,
				GPlatesModel::ModelUtils::create_gml_time_instant(time),
				description_xs_string,
				property_value->get_structural_type(),
				!is_enabled/*is_disabled*/);
	}

	// Select the 'non-const' overload of 'GpmlTimeSample::value()'.
	GPlatesModel::PropertyValue::non_null_ptr_type
	gpml_time_sample_get_value(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		return gpml_time_sample.value();
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_sample_get_time(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		return gpml_time_sample.valid_time()->get_time_position();
	}

	void
	gpml_time_sample_set_time(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample,
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		gpml_time_sample.valid_time()->set_time_position(time_position);
	}

	// Make it easier for client by converting from XsString to a regular string.
	boost::optional<GPlatesPropertyValues::TextContent>
	gpml_time_sample_get_description(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> xs_string =
				gpml_time_sample.description();
		if (!xs_string)
		{
			return boost::none;
		}

		return xs_string.get()->get_value();
	}

	// Make it easier for client by converting from a regular string to XsString.
	void
	gpml_time_sample_set_description(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample,
			boost::optional<GPlatesPropertyValues::TextContent> description)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> xs_string;
		if (description)
		{
			xs_string = GPlatesPropertyValues::XsString::create(description->get());
		}

		gpml_time_sample.set_description(xs_string);
	}

	bool
	gpml_time_sample_is_enabled(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		return !gpml_time_sample.is_disabled();
	}

	void
	gpml_time_sample_set_enabled(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample,
			bool is_enabled)
	{
		return gpml_time_sample.set_disabled(!is_enabled);
	}
}

void
export_gpml_time_sample()
{
	//
	// GpmlTimeSample - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlTimeSample,
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type,
			boost::noncopyable>(
					"GpmlTimeSample",
					"A time sample associates an arbitrary property value with a specific time instant. "
					"Typically a sequence of time samples are used in a :class:`GpmlIrregularSampling`. "
					"The most common example of this is a time-dependent sequence of total reconstruction poles.\n"
					"\n"
					"Time samples are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). This includes comparing the property value "
					"in the two time samples being compared (see :class:`PropertyValue`) as well as the time instant, "
					"description string and disabled flag.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_time_sample_create,
						bp::default_call_policies(),
						(bp::arg("property_value"),
							bp::arg("time"),
							bp::arg("description") = boost::optional<GPlatesPropertyValues::TextContent>(),
							bp::arg("is_enabled") = true)),
				"__init__(property_value, time, [description], [is_enabled=True])\n"
				"  Create a time sample given a property value and time and optionally a description string "
				"and disabled flag.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param time: the time position associated with the property value\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :param description: description of the time sample\n"
				"  :type description: string or None\n"
				"  :param is_enabled: whether time sample is enabled\n"
				"  :type is_enabled: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    time_sample = pygplates.GpmlTimeSample(property_value, time)\n")
		.def("get_value",
				&GPlatesApi::gpml_time_sample_get_value,
				"get_value()\n"
				"  Returns the property value of this time sample.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("get_value_type",
				&GPlatesPropertyValues::GpmlTimeSample::get_value_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_value_type()\n"
				"  Returns the type of property value returned by :meth:`get_value`.\n"
				"\n"
				"  For example, it might return ``pygplates.GmlLineString`` which is a *class* object (not an instance).\n"
				"\n"
				"  :rtype: a class object of the property type (derived from :class:`PropertyValue`)\n"
				"\n"
				"  .. versionadded:: 0.21\n")
		.def("set_value",
				&GPlatesPropertyValues::GpmlTimeSample::set_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value associated with this time sample.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n")
		.def("get_time",
				&GPlatesApi::gpml_time_sample_get_time,
				"get_time()\n"
				"  Returns the time position of this time sample.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_time",
				&GPlatesApi::gpml_time_sample_set_time,
				(bp::arg("time")),
				"set_time(time)\n"
				"  Sets the time position associated with this time sample.\n"
				"\n"
				"  :param time: the time position associated with the property value\n"
				"  :type time: float or :class:`GeoTimeInstant`\n")
		.def("get_description",
				&GPlatesApi::gpml_time_sample_get_description,
				"get_description()\n"
				"  Returns the description of this time sample, or ``None``.\n"
				"\n"
				"  :rtype: string or None\n")
		.def("set_description",
				&GPlatesApi::gpml_time_sample_set_description,
				(bp::arg("description") = boost::optional<GPlatesPropertyValues::TextContent>()),
				"set_description([description])\n"
				"  Sets the description associated with this time sample, or removes it if none specified.\n"
				"\n"
				"  :param description: description of the time sample\n"
				"  :type description: string or None\n")
		.def("is_enabled",
				&GPlatesApi::gpml_time_sample_is_enabled,
				"is_enabled()\n"
				"  Returns whether this time sample is enabled.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  For example, only enabled total reconstruction poles (in a GpmlIrregularSampling sequence) "
				"are considered when interpolating rotations at some arbitrary time.\n")
		.def("set_enabled",
				&GPlatesApi::gpml_time_sample_set_enabled,
				(bp::arg("set_enabled")=true),
				"set_enabled([is_enabled=True])\n"
				"  Sets whether this time sample is enabled.\n"
				"\n"
				"  :param is_enabled: whether time sample is enabled (defaults to ``True``)\n"
				"  :type is_enabled: bool\n")
		.def("is_disabled",
				&GPlatesPropertyValues::GpmlTimeSample::is_disabled,
				"is_disabled()\n"
				"  Returns whether this time sample is disabled or not.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  For example, a disabled total reconstruction pole (in a GpmlIrregularSampling sequence) "
				"is ignored when interpolating rotations at some arbitrary time.\n")
		.def("set_disabled",
				&GPlatesPropertyValues::GpmlTimeSample::set_disabled,
				(bp::arg("is_disabled")=true),
				"set_disabled([is_disabled=True])\n"
				"  Sets whether this time sample is disabled.\n"
				"\n"
				"  :param is_disabled: whether time sample is disabled (defaults to ``True``)\n"
				"  :type is_disabled: bool\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type>())
	;

	// Create a python class "GpmlTimeSampleList" for RevisionedVector<GpmlTimeSample> that behaves like a list of GpmlTimeSample.
	//
	// This enables the RevisionedVector<GpmlTimeSample> returned by 'GpmlIrregularSampling.get_time_samples()' to be treated as a list.
	GPlatesApi::create_python_class_as_revisioned_vector<GPlatesPropertyValues::GpmlTimeSample>("GpmlTimeSample");

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTimeSample>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type
	gpml_time_window_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		return GPlatesPropertyValues::GpmlTimeWindow::create(
				property_value,
				GPlatesModel::ModelUtils::create_gml_time_period(
						begin_time,
						end_time,
						true/*check_begin_end_times*/),
				property_value->get_structural_type());
	}

	// Select the 'non-const' overload of 'GpmlTimeWindow::time_dependent_value()'.
	GPlatesModel::PropertyValue::non_null_ptr_type
	gpml_time_window_get_value(
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		return gpml_time_window.time_dependent_value();
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_window_get_begin_time(
			const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		return gpml_time_window.valid_time()->begin()->get_time_position();
	}

	void
	gpml_time_window_set_begin_time(
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time)
	{
		// Use 'assert' protected function to ensure proper exception is raised if GmlTimePeriod
		// class invariant is violated.
		gml_time_period_set_begin_time(*gpml_time_window.valid_time(), begin_time);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_window_get_end_time(
			const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		return gpml_time_window.valid_time()->end()->get_time_position();
	}

	void
	gpml_time_window_set_end_time(
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		// Use 'assert' protected function to ensure proper exception is raised if GmlTimePeriod
		// class invariant is violated.
		gml_time_period_set_end_time(*gpml_time_window.valid_time(), end_time);
	}
}

void
export_gpml_time_window()
{
	//
	// GpmlTimeWindow - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlTimeWindow,
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type,
			boost::noncopyable>(
					"GpmlTimeWindow",
					"A time window associates an arbitrary property value with a specific time period. "
					"The property value does not vary over the time period of the window. "
					"Typically a sequence of time windows are used in a :class:`GpmlPiecewiseAggregation` "
					"where the sequence of windows form a *piecewise-constant* (staircase function) "
					"property value over time (since each time window has a *constant* property value) "
					"assuming the windows do not have overlaps or gaps in time.\n"
					"\n"
					"Time windows are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). This includes comparing the property value "
					"in the two time windows being compared (see :class:`PropertyValue`) as well as the time period.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_time_window_create,
						bp::default_call_policies(),
						(bp::arg("property_value"), bp::arg("begin_time"), bp::arg("end_time"))),
				"__init__(property_value, begin_time, end_time)\n"
				"  Create a time window given a property value and time range.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param begin_time: the begin time of the time window\n"
				"  :type begin_time: float or :class:`GeoTimeInstant`\n"
				"  :param end_time: the end time of the time window\n"
				"  :type end_time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n"
				"\n"
				"  ::\n"
				"\n"
				"    time_window = pygplates.GpmlTimeWindow(property_value, begin_time, end_time)\n"
				"\n"
				"  Note that *begin_time* must be further in the past than the *end_time* "
				"``begin_time > end_time``.\n")
		.def("get_value",
				&GPlatesApi::gpml_time_window_get_value,
				"get_value()\n"
				"  Returns the property value of this time window.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("get_value_type",
				&GPlatesPropertyValues::GpmlTimeWindow::get_value_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_value_type()\n"
				"  Returns the type of property value returned by :meth:`get_value`.\n"
				"\n"
				"  For example, it might return ``pygplates.GmlLineString`` which is a *class* object (not an instance).\n"
				"\n"
				"  :rtype: a class object of the property type (derived from :class:`PropertyValue`)\n"
				"\n"
				"  .. versionadded:: 0.21\n")
		.def("set_value",
				&GPlatesPropertyValues::GpmlTimeWindow::set_time_dependent_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value associated with this time window.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n")
		.def("get_begin_time",
				&GPlatesApi::gpml_time_window_get_begin_time,
				"get_begin_time()\n"
				"  Returns the begin time of this time window.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_begin_time",
				&GPlatesApi::gpml_time_window_set_begin_time,
				(bp::arg("time")),
				"set_begin_time(time)\n"
				"  Sets the begin time of this time window.\n"
				"\n"
				"  :param time: the begin time of this time window\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		.def("get_end_time",
				&GPlatesApi::gpml_time_window_get_end_time,
				"get_end_time()\n"
				"  Returns the end time of this time window.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_end_time",
				&GPlatesApi::gpml_time_window_set_end_time,
				(bp::arg("time")),
				"set_end_time(time)\n"
				"  Sets the end time of this time window.\n"
				"\n"
				"  :param time: the end time of this time window\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type>())
	;

	// Create a python class "GpmlTimeWindowList" for RevisionedVector<GpmlTimeWindow> that behaves like a list of GpmlTimeWindow.
	//
	// This enables the RevisionedVector<GpmlTimeWindow> returned by 'GpmlPiecewiseAggregation.get_time_windows()' to be treated as a list.
	GPlatesApi::create_python_class_as_revisioned_vector<GPlatesPropertyValues::GpmlTimeWindow>("GpmlTimeWindow");

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTimeWindow>();
}


namespace GPlatesApi
{
	namespace
	{
		/**
		 * If the section is used in a topological network then it can only be reconstructable by plate ID or half-stage rotation.
		 *
		 * These are the only supported reconstructable types inside the deforming network code (in Delaunay vertices).
		 */
		bool
		can_use_as_topological_section_of_topological_network(
				GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle)
		{
			// Get the feature's reconstruction method.
			GPlatesAppLogic::ReconstructionFeatureProperties reconstruction_feature_properties;
			reconstruction_feature_properties.visit_feature(feature_handle->reference());

			// Note that no reconstruction method implies ByPlateId (which is allowed).
			boost::optional<GPlatesPropertyValues::EnumerationContent> reconstruction_method =
				reconstruction_feature_properties.get_reconstruction_method();
			if (reconstruction_method)
			{
				const QString reconstruction_method_string = reconstruction_method->get().qstring();
				if (reconstruction_method_string != "ByPlateId" &&
					!reconstruction_method_string.startsWith("HalfStageRotation"))
				{
					return false;
				}
			}

			return true;
		}
	}

	/**
	 * Create a topological point or line section referencing the geometry of a feature.
	 */
	boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
	gpml_topological_section_create(
			GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle,
			boost::optional<GPlatesModel::PropertyName> geometry_property_name,
			bool reverse_order,
			boost::optional<GPlatesPropertyValues::StructuralType> topological_geometry_type)
	{
		// Make sure topological geometry type is a topological line, polygon or network.
		if (topological_geometry_type)
		{
			if (topological_geometry_type.get() != GPlatesPropertyValues::GpmlTopologicalLine::STRUCTURAL_TYPE &&
				topological_geometry_type.get() != GPlatesPropertyValues::GpmlTopologicalPolygon::STRUCTURAL_TYPE &&
				topological_geometry_type.get() != GPlatesPropertyValues::GpmlTopologicalNetwork::STRUCTURAL_TYPE)
			{
				// Raise the 'ValueError' python exception if unexpected topological geometry type.
				PyErr_SetString(
					PyExc_ValueError,
					"Topological geometry type should be GpmlTopologicalLine, GpmlTopologicalPolygon or GpmlTopologicalNetwork");
				bp::throw_error_already_set();
			}
		}

		// If a geometry property name is not specified then use the default geometry property of the feature's type.
		if (!geometry_property_name)
		{
			geometry_property_name = get_default_geometry_property_name(feature_handle->feature_type());
		}

		bp::object feature_object(feature_handle);

		// Find the geometry associated with the property name.
		//
		// Call python since Feature.get_geometry is implemented in python code...
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> feature_geometry =
				bp::extract< boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> >(
						feature_object.attr("get_geometry")(geometry_property_name));
		if (feature_geometry)
		{
			// If the section is used in a topological network then it can only be reconstructable by plate ID or half-stage rotation.
			// These are the only supported reconstructable types inside the deforming network code (in Delaunay vertices).
			if (topological_geometry_type &&
				topological_geometry_type.get() == GPlatesPropertyValues::GpmlTopologicalNetwork::STRUCTURAL_TYPE)
			{
				if (!can_use_as_topological_section_of_topological_network(feature_handle))
				{
					return boost::none;
				}
			}

			// The geometry type should be a point or a polyline.
			//
			// The topology build tools in GPlates will also accept a polygon (interpreted a polyline)
			// but we won't accept that here because it's pretty confusing to have a polygon form only part
			// of a topological boundary (intuitively you might except it could only form the entire boundary).
			const GPlatesMaths::GeometryType::Value geometry_type =
					GPlatesAppLogic::GeometryUtils::get_geometry_type(*feature_geometry.get());
			if (geometry_type == GPlatesMaths::GeometryType::POLYLINE)
			{
				const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type topological_section =
					GPlatesPropertyValues::GpmlTopologicalLineSection::create(
						GPlatesPropertyValues::GpmlPropertyDelegate::create(
							feature_handle->feature_id(),
							geometry_property_name.get(),
							GPlatesPropertyValues::GmlLineString::STRUCTURAL_TYPE),
						reverse_order);

				return topological_section;
			}
			else if (geometry_type == GPlatesMaths::GeometryType::POINT)
			{
				const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type topological_section =
					GPlatesPropertyValues::GpmlTopologicalPoint::create(
						GPlatesPropertyValues::GpmlPropertyDelegate::create(
							feature_handle->feature_id(),
							geometry_property_name.get(),
							GPlatesPropertyValues::GmlPoint::STRUCTURAL_TYPE));

				return topological_section;
			}
		}
		// else if a topological boundary or network was specified then we can also reference a topological line...
		else if (topological_geometry_type &&
				topological_geometry_type.get() != GPlatesPropertyValues::GpmlTopologicalLine::STRUCTURAL_TYPE)
		{
			// Find the topological geometry associated with the property name.
			//
			// Call python since Feature.get_topological_geometry is implemented in python code...
			boost::optional<topological_geometry_property_value_type> feature_topological_geometry =
					bp::extract< boost::optional<topological_geometry_property_value_type> >(
							feature_object.attr("get_topological_geometry")(geometry_property_name));
			if (feature_topological_geometry)
			{
				// It must be a topological *line*.
				if (boost::get<GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type>(&feature_topological_geometry.get()))
				{
					// Ideally if the topological *line* is used in a topological network then its sub-segments can only be
					// reconstructable by plate ID or half-stage rotation (since these are the only supported reconstructable types
					// inside the deforming network code - in Delaunay vertices).
					// However we only have the feature IDs of these sub-segment features (not the features themselves) and so
					// we cannot inspect the features to ensure they are reconstructable by plate ID or half-stage rotation.

					// Create a topological section referencing a topological line.
					const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type topological_section =
						GPlatesPropertyValues::GpmlTopologicalLineSection::create(
							GPlatesPropertyValues::GpmlPropertyDelegate::create(
								feature_handle->feature_id(),
								geometry_property_name.get(),
								GPlatesPropertyValues::GpmlTopologicalLine::STRUCTURAL_TYPE),
							reverse_order);

					return topological_section;
				}
			}
		}

		return boost::none;
	}

	/**
	 * Create a topological network interior referencing the geometry of a feature.
	 */
	boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
	gpml_topological_section_create_network_interior(
			GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle,
			boost::optional<GPlatesModel::PropertyName> geometry_property_name)
	{
		// If a geometry property name is not specified then use the default geometry property of the feature's type.
		if (!geometry_property_name)
		{
			geometry_property_name = get_default_geometry_property_name(feature_handle->feature_type());
		}

		bp::object feature_object(feature_handle);

		// Find the geometry associated with the property name.
		//
		// Call python since Feature.get_geometry is implemented in python code...
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> feature_geometry =
				bp::extract< boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> >(
						feature_object.attr("get_geometry")(geometry_property_name));
		if (feature_geometry)
		{
			// Sections in a topological network can only be reconstructable by plate ID or half-stage rotation.
			// These are the only supported reconstructable types inside the deforming network code (in Delaunay vertices).
			if (!can_use_as_topological_section_of_topological_network(feature_handle))
			{
				return boost::none;
			}

			// Any geometry type is allowed for a network interior (since only the points are used).
			//
			// However we still need to specify a property structural type that's appropriate for the geometry type.
			boost::optional<GPlatesPropertyValues::StructuralType> structural_type;
			const GPlatesMaths::GeometryType::Value geometry_type =
					GPlatesAppLogic::GeometryUtils::get_geometry_type(*feature_geometry.get());
			switch (geometry_type)
			{
			case GPlatesMaths::GeometryType::POLYLINE:
				structural_type = GPlatesPropertyValues::GmlLineString::STRUCTURAL_TYPE;
				break;
			case GPlatesMaths::GeometryType::MULTIPOINT:
				structural_type = GPlatesPropertyValues::GmlMultiPoint::STRUCTURAL_TYPE;
				break;
			case GPlatesMaths::GeometryType::POINT:
				structural_type = GPlatesPropertyValues::GmlPoint::STRUCTURAL_TYPE;
				break;
			case GPlatesMaths::GeometryType::POLYGON:
				{
					static const GPlatesPropertyValues::StructuralType GML_LINEAR_RING =
							GPlatesPropertyValues::StructuralType::create_gml("LinearRing");
					structural_type = GML_LINEAR_RING;
				}
				break;
			default:
				return boost::none;
			}

			// Create a property delegate referencing the network interior geometry.
			const GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type network_interior =
				GPlatesPropertyValues::GpmlPropertyDelegate::create(
					feature_handle->feature_id(),
					geometry_property_name.get(),
					structural_type.get());

			return network_interior;
		}
		// we can also reference a topological line...
		else
		{
			// Find the topological geometry associated with the property name.
			//
			// Call python since Feature.get_topological_geometry is implemented in python code...
			boost::optional<topological_geometry_property_value_type> feature_topological_geometry =
					bp::extract< boost::optional<topological_geometry_property_value_type> >(
							feature_object.attr("get_topological_geometry")(geometry_property_name));
			if (feature_topological_geometry)
			{
				// It must be a topological *line*.
				if (boost::get<GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type>(&feature_topological_geometry.get()))
				{
					// Ideally since the topological *line* is used in a topological network then its sub-segments can only be
					// reconstructable by plate ID or half-stage rotation (since these are the only supported reconstructable types
					// inside the deforming network code - in Delaunay vertices).
					// However we only have the feature IDs of these sub-segment features (not the features themselves) and so
					// we cannot inspect the features to ensure they are reconstructable by plate ID or half-stage rotation.

					// Create a property delegate referencing the network interior geometry (topological line).
					const GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type network_interior =
						GPlatesPropertyValues::GpmlPropertyDelegate::create(
							feature_handle->feature_id(),
							geometry_property_name.get(),
							GPlatesPropertyValues::GpmlTopologicalLine::STRUCTURAL_TYPE);

					return network_interior;
				}
			}
		}

		return boost::none;
	}
}

void
export_gpml_topological_section()
{
	// Use the 'non-const' overload so GpmlTopologicalSection can be modified via python...
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
			(GPlatesPropertyValues::GpmlTopologicalSection::*get_property_delegate)() =
					&GPlatesPropertyValues::GpmlTopologicalSection::get_source_geometry;

	/*
	 * GpmlTopologicalSection - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base class for topological section property values.
	 *
	 * Enables 'isinstance(obj, GpmlTopologicalSection)' in python - not that it's that useful.
	 */
	bp::class_<
			GPlatesPropertyValues::GpmlTopologicalSection,
			GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlTopologicalSection",
					"The base class inherited by all derived *topological section* property value classes.\n"
					"\n"
					"The list of derived topological section property value classes includes:\n"
					"\n"
					"* :class:`GpmlTopologicalPoint`\n"
					"* :class:`GpmlTopologicalLineSection`\n"
					"\n"
					"  .. versionadded:: 0.21\n",
					bp::no_init)
		.def("create",
			&GPlatesApi::gpml_topological_section_create,
			(bp::arg("feature"),
					bp::arg("geometry_property_name") = boost::optional<GPlatesModel::PropertyName>(),
					bp::arg("reverse_order") = false,
					bp::arg("topological_geometry_type") = boost::optional<GPlatesPropertyValues::StructuralType>()),
			"create(feature, [geometry_property_name], [reverse_order], [topological_geometry_type])\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Create a topological section referencing a feature geometry.\n"
			"\n"
			"  :param feature: the feature referenced by the returned topological section\n"
			"  :type feature: :class:`Feature`\n"
			"  :param geometry_property_name: the optional geometry property name used to find the geometry "
			"(topological or non-topological), if not specified then the default geometry property name associated "
			"with the feature's :class:`type<FeatureType>` is used instead\n"
			"  :type geometry_property_name: :class:`PropertyName`, or None\n"
			"  :param reverse_order: whether to reverse the topological section when it's used to resolve a "
			"topological geometry, this is ignored for *point* sections and only applies to a *line* section when "
			"it does not intersect both its neighbours (when resolving the parent topology) - defaults to False\n"
			"  :type reverse_order: bool\n"
			"  :param topological_geometry_type: optional type of topological geometry that the returned section "
			"will be used for (if specified, then used to determine what type of feature geometry can be used as a section)\n"
			"  :type topological_geometry_type: :class:`GpmlTopologicalLine` or :class:`GpmlTopologicalPolygon` or "
			":class:`GpmlTopologicalNetwork`, or None\n"
			"  :rtype: :class:`GpmlTopologicalSection` (:class:`GpmlTopologicalLineSection` or :class:`GpmlTopologicalPoint`), or None\n"
			"  :raises: ValueError if *topological_geometry_type* is specified but is not one of the accepted types "
			"(:class:`GpmlTopologicalLine` or :class:`GpmlTopologicalPolygon` or :class:`GpmlTopologicalNetwork`)\n"
			"\n"
			"  If *geometry_property_name* is not specified then the default geometry property name is determined from the feature's :class:`type<FeatureType>` - "
			"see :meth:`Feature.get_geometry` for more details.\n"
			"\n"
			"  A regular polyline or point can be referenced by any topological geometry (topological line, polygon or network). "
			"However a topological *line* can only be referenced by a topological polygon or network.\n"
			"\n"
			"  .. note:: It's fine to ignore *reverse_order* (leave it as the default) since it is not used when resolving the topological geometry "
			"provided it intersects both its neighbouring topological sections (in the topological geometry) - which applies only to line sections (not points). "
			"When a line section does not intersect both neighbouring sections then its reverse flag determines its orientation when rubber-banding the topology geometry.\n"
			"\n"
			"  Returns ``None`` if:\n"
			"\n"
			"  * there is not exactly one geometry (topological or non-topological) property named *geometry_property_name* (or default) in *feature*, or\n"
			"  * it's a regular geometry but it's not a point or polyline, or\n"
			"  * it's a regular point/polyline and *topological_geometry_type* is a topological network but *feature* is not reconstructable by "
			"plate ID or half-stage rotation (the only supported reconstructable types inside the deforming network Delaunay triangulation), or\n"
			"  * it's a topological polygon or network, or\n"
			"  * it's a topological line but *topological_geometry_type* is also a topological line (or not specified)\n"
			"\n"
			"  Create a topological section to be used in a :class:`GpmlTopologicalPolygon`:\n"
			"  ::\n"
			"\n"
			"    boundary_sections = []\n"
			"    \n"
			"    boundary_section = pygplates.GpmlTopologicalSection.create(referenced_feature, "
			"topological_geometry_type=pygplates.GpmlTopologicalPolygon)\n"
			"    if boundary_section:\n"
			"        boundary_sections.append(boundary_section)\n"
			"    \n"
			"    ...\n"
			"    \n"
			"    topological_boundary = pygplates.GpmlTopologicalPolygon(boundary_sections)\n"
			"    \n"
			"    topological_boundary_feature = pygplates.Feature(pygplates.FeatureType.gpml_topological_closed_plate_boundary)\n"
			"    topological_boundary_feature.set_topological_geometry(topological_boundary)\n"
			"\n"
			"  .. seealso:: :meth:`GpmlTopologicalLine.get_sections`, :meth:`GpmlTopologicalPolygon.get_boundary_sections` and "
			":meth:`GpmlTopologicalNetwork.get_boundary_sections`\n"
			"\n"
			"  .. versionadded:: 0.24\n")
		.staticmethod("create")
		.def("create_network_interior",
			&GPlatesApi::gpml_topological_section_create_network_interior,
			(bp::arg("feature"),
					bp::arg("geometry_property_name") = boost::optional<GPlatesModel::PropertyName>()),
			"create_network_interior(feature, [geometry_property_name])\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Create a topological network interior referencing a feature geometry.\n"
			"\n"
			"  :param feature: the feature referenced by the returned network interior\n"
			"  :type feature: :class:`Feature`\n"
			"  :param geometry_property_name: the optional geometry property name used to find the geometry "
			"(topological or non-topological), if not specified then the default geometry property name associated "
			"with the feature's :class:`type<FeatureType>` is used instead\n"
			"  :type geometry_property_name: :class:`PropertyName`, or None\n"
			"  :rtype: :class:`GpmlPropertyDelegate`, or None\n"
			"\n"
			"  If *geometry_property_name* is not specified then the default geometry property name is determined from the feature's :class:`type<FeatureType>` - "
			"see :meth:`Feature.get_geometry` for more details.\n"
			"\n"
			"  Any regular geometry (point, multipoint, polyline, polygon) or topological *line* can be referenced by a topological network interior.\n"
			"\n"
			"  .. note:: If a regular *polygon* geometry is referenced then it will be treated as a *rigid* interior block in the topological network and will not be "
			"part of the deforming region. Anything inside this interior polygon geometry will move rigidly using the plate ID of the referenced feature.\n"
			"\n"
			"  Returns ``None`` if:\n"
			"\n"
			"  * there is not exactly one geometry (topological or non-topological) property named *geometry_property_name* (or default) in *feature*, or\n"
			"  * it's a regular geometry but *feature* is not reconstructable by plate ID or half-stage rotation "
			"(the only supported reconstructable types inside the deforming network Delaunay triangulation), or\n"
			"  * it's a topological polygon or network\n"
			"\n"
			"  Create a topological network interior:\n"
			"  ::\n"
			"\n"
			"    network_interiors = []\n"
			"    \n"
			"    network_interior = pygplates.GpmlTopologicalSection.create_network_interior(referenced_interior_feature)\n"
			"    if network_interior:\n"
			"        network_interiors.append(network_interior)\n"
			"    \n"
			"    ...\n"
			"    network_boundaries = []\n"
			"    \n"
			"    network_boundary = pygplates.GpmlTopologicalSection.create(referenced_boundary_feature)\n"
			"    if network_boundary:\n"
			"        network_boundaries.append(network_boundary)\n"
			"    \n"
			"    ...\n"
			"    \n"
			"    topological_network = pygplates.GpmlTopologicalNetwork(network_boundaries, network_interiors)\n"
			"    \n"
			"    topological_network_feature = pygplates.Feature(pygplates.FeatureType.gpml_topological_network)\n"
			"    topological_network_feature.set_topological_geometry(topological_network)\n"
			"\n"
			"  .. seealso:: :meth:`GpmlTopologicalNetwork.get_interiors`\n"
			"\n"
			"  .. versionadded:: 0.24\n")
		.staticmethod("create_network_interior")
		.def("get_property_delegate",
			get_property_delegate,
			"get_property_delegate()\n"
			"  Returns the property value that references/delegates the source geometry.\n"
			"\n"
			"  :rtype: :class:`GpmlPropertyDelegate`\n")
		.def("get_reverse_orientation",
			&GPlatesPropertyValues::GpmlTopologicalSection::get_reverse_order,
			"get_reverse_orientation()\n"
			"  Returns ``True`` if this topological section is a :class:`line <GpmlTopologicalLineSection>` "
			"and it was reversed when contributing to the parent topology.\n"
			"\n"
			"  :rtype: bool\n"
			"\n"
			"  .. note:: If this topological section is a :class:`point <GpmlTopologicalPoint>` "
			"then ``False`` will always be returned.\n")
	;

	// Create a python class "GpmlTopologicalSectionList" for RevisionedVector<GpmlTopologicalSection> that behaves like a list of GpmlTopologicalSection.
	//
	// This enables the RevisionedVector<GpmlTopologicalSection> returned by 'GpmlTopologicalLine.get_sections()',
	// 'GpmlTopologicalPolygon.get_exterior_sections()' and 'GpmlTopologicalNetwork.get_boundary_sections()' to be treated as a list.
	GPlatesApi::create_python_class_as_revisioned_vector<GPlatesPropertyValues::GpmlTopologicalSection>("GpmlTopologicalSection");

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTopologicalSection>();
}


namespace GPlatesApi
{
	GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type
	gpml_topological_line_create(
			bp::object sections) // Any python sequence (eg, list, tuple).
	{
		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections_vector;
		PythonExtractUtils::extract_iterable(
				sections_vector,
				sections,
				"Expected a sequence of sections (GpmlTopologicalSection)");

		return GPlatesPropertyValues::GpmlTopologicalLine::create(sections_vector);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTopologicalSection>::non_null_ptr_type
	gpml_topological_line_get_sections(
			GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line)
	{
		return &gpml_topological_line.sections();
	}
}

void
export_gpml_topological_line()
{
	//
	// GpmlTopologicalLine - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
		GPlatesPropertyValues::GpmlTopologicalLine,
		GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type,
		bp::bases<GPlatesModel::PropertyValue>,
		boost::noncopyable>(
			"GpmlTopologicalLine",
			"A topological line geometry that is resolved from topological sections.\n"
			"\n"
			"  .. versionadded:: 0.21\n",
			// We need this (even though "__init__" is defined) since
			// there is no publicly-accessible default constructor...
			bp::no_init)
		.def("__init__",
			bp::make_constructor(
				&GPlatesApi::gpml_topological_line_create,
				bp::default_call_policies(),
				(bp::arg("sections"))),
			"__init__(sections)\n"
			"  Create a topological line made from topological sections.\n"
			"\n"
			"  :param sections: A sequence of :class:`GpmlTopologicalSection` elements\n"
			"  :type sections: Any sequence such as a ``list`` or a ``tuple``\n"
			"\n"
			"  ::\n"
			"\n"
			"    topological_line = pygplates.GpmlTopologicalLine(topological_sections)\n")
		.def("get_sections",
			&GPlatesApi::gpml_topological_line_get_sections,
			"get_sections()\n"
			"  Returns the :class:`sections<GpmlTopologicalSection>` in a sequence that behaves as a python ``list``.\n"
			"\n"
			"  :rtype: :class:`GpmlTopologicalSectionList`\n"
			"\n"
			"  Modifying the returned sequence will modify the internal state of the *GpmlTopologicalLine* instance:\n"
			"  ::\n"
			"\n"
			"    sections = topological_line.get_sections()\n"
			"\n"
			"    # Append a section\n"
			"    sections.append(pygplates.GpmlTopologicalPoint(...))\n")
		;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlTopologicalLine>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTopologicalLine>();
}

void
export_gpml_topological_line_section()
{
	// Use the 'non-const' overload so GpmlTopologicalLineSection can be modified via python...
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
			(GPlatesPropertyValues::GpmlTopologicalLineSection::*get_property_delegate)() =
					&GPlatesPropertyValues::GpmlTopologicalLineSection::get_source_geometry;

	//
	// GpmlTopologicalLineSection - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
		GPlatesPropertyValues::GpmlTopologicalLineSection,
		GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type,
		bp::bases<GPlatesPropertyValues::GpmlTopologicalSection>,
		boost::noncopyable>(
			"GpmlTopologicalLineSection",
			"A topological section referencing a line geometry.\n"
			"\n"
			"  .. versionadded:: 0.21\n",
			// We need this (even though "__init__" is defined) since
			// there is no publicly-accessible default constructor...
			bp::no_init)
		.def("__init__",
			bp::make_constructor(
				&GPlatesPropertyValues::GpmlTopologicalLineSection::create,
				bp::default_call_policies(),
				(bp::arg("gpml_property_delegate"),
					bp::arg("reverse_orientation"))),
			"__init__(gpml_property_delegate, reverse_orientation)\n"
			"  Create a topological point section property value that references a feature property containing a line geometry.\n"
			"\n"
			"  :param gpml_property_delegate: the line (polyline) property value\n"
			"  :type gpml_property_delegate: :class:`GpmlPropertyDelegate`\n"
			"  :param reverse_orientation: whether the line was reversed when contributing to the parent topology\n"
			"  :type reverse_orientation: bool\n"
			"\n"
			"  ::\n"
			"\n"
			"    topological_line_section = pygplates.GpmlTopologicalLineSection(line_property_delegate, false)\n")
		.def("get_property_delegate",
			get_property_delegate,
			"get_property_delegate()\n"
			"  Returns the property value that references/delegates the source line geometry.\n"
			"\n"
			"  :rtype: :class:`GpmlPropertyDelegate`\n")
		.def("set_property_delegate",
			&GPlatesPropertyValues::GpmlTopologicalLineSection::set_source_geometry,
			(bp::arg("gpml_property_delegate")),
			"set_property_delegate(gpml_property_delegate)\n"
			"  Sets the property value that references/delegates the source line geometry.\n"
			"\n"
			"  :param gpml_property_delegate: the line (polyline) delegate property value\n"
			"  :type gpml_property_delegate: :class:`GpmlPropertyDelegate`\n")
		.def("get_reverse_orientation",
			&GPlatesPropertyValues::GpmlTopologicalLineSection::get_reverse_order,
			"get_reverse_orientation()\n"
			"  Returns ``True`` if the line was reversed when contributing to the parent topology.\n"
			"\n"
			"  :rtype: bool\n")
		.def("set_reverse_orientation",
			&GPlatesPropertyValues::GpmlTopologicalLineSection::set_reverse_order,
			(bp::arg("reverse_orientation")),
			"set_reverse_orientation(reverse_orientation)\n"
			"  Sets the property value that references/delegates the source line geometry.\n"
			"\n"
			"  :param reverse_orientation: whether the line was reversed when contributing to the parent topology\n"
			"  :type reverse_orientation: bool\n")
		;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlTopologicalLineSection>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTopologicalLineSection>();
}


namespace GPlatesApi
{
	GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type
	gpml_topological_network_create(
			bp::object boundary_sections,   // Any python sequence (eg, list, tuple).
			bp::object interiors) // Any python sequence (eg, list, tuple).
	{
		// Copy boundary sections into a vector.
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> boundary_sections_vector;
		PythonExtractUtils::extract_iterable(
				boundary_sections_vector,
				boundary_sections,
				"Expected a sequence of boundary sections (GpmlTopologicalSection)");

		// Interior geometries are optional.
		std::vector<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> interiors_vector;
		if (interiors != bp::object()/*Py_None*/)
		{
			// Copy interior geometries into a vector.
			PythonExtractUtils::extract_iterable(
					interiors_vector,
					interiors,
					"Expected a sequence of interior geometries (GpmlPropertyDelegate)");
		}

		return GPlatesPropertyValues::GpmlTopologicalNetwork::create(boundary_sections_vector, interiors_vector);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTopologicalSection>::non_null_ptr_type
	gpml_topological_network_get_boundary_sections(
			GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network)
	{
		return &gpml_topological_network.boundary_sections();
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlPropertyDelegate>::non_null_ptr_type
	gpml_topological_network_get_interiors(
			GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network)
	{
		return &gpml_topological_network.interior_geometries();
	}
}

void
export_gpml_topological_network()
{
	//
	// GpmlTopologicalNetwork - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
		GPlatesPropertyValues::GpmlTopologicalNetwork,
		GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type,
		bp::bases<GPlatesModel::PropertyValue>,
		boost::noncopyable>(
			"GpmlTopologicalNetwork",
			"A topological deforming network that is resolved from boundary topological sections and interior geometries.\n"
			"\n"
			".. note:: If an interior geometry is a polygon then it becomes an interior rigid block.\n"
			"\n"
			"  .. versionadded:: 0.21\n",
			// We need this (even though "__init__" is defined) since
			// there is no publicly-accessible default constructor...
			bp::no_init)
		.def("__init__",
			bp::make_constructor(
				&GPlatesApi::gpml_topological_network_create,
				bp::default_call_policies(),
				(bp::arg("boundary_sections"),
					bp::arg("interiors") = bp::object()/*Py_None*/)),
			"__init__(boundary_sections, [interiors=None])\n"
			"  Create a topological network made from boundary topological sections and interior geometries.\n"
			"\n"
			"  :param boundary_sections: A sequence of :class:`GpmlTopologicalSection` elements\n"
			"  :type boundary_sections: Any sequence such as a ``list`` or a ``tuple``\n"
			"  :param interiors: A sequence of :class:`GpmlPropertyDelegate` elements\n"
			"  :type interiors: Any sequence such as a ``list`` or a ``tuple``\n"
			"\n"
			"  ::\n"
			"\n"
			"    topological_network = pygplates.GpmlTopologicalNetwork(boundary_sections, interiors)\n")
		.def("get_boundary_sections",
			&GPlatesApi::gpml_topological_network_get_boundary_sections,
			"get_boundary_sections()\n"
			"  Returns the :class:`boundary sections<GpmlTopologicalSection>` in a sequence that behaves as a python ``list``.\n"
			"\n"
			"  :rtype: :class:`GpmlTopologicalSectionList`\n"
			"\n"
			"  Modifying the returned sequence will modify the internal state of the *GpmlTopologicalNetwork* instance:\n"
			"  ::\n"
			"\n"
			"    boundary_sections = topological_network.get_boundary_sections()\n"
			"\n"
			"    # Append a section\n"
			"    boundary_sections.append(pygplates.GpmlTopologicalLineSection(...))\n")
		.def("get_interiors",
			&GPlatesApi::gpml_topological_network_get_interiors,
			"get_interiors()\n"
			"  Returns the :class:`interior geometries<GpmlPropertyDelegate>` in a sequence that behaves as a python ``list``.\n"
			"\n"
			"  :rtype: :class:`GpmlPropertyDelegateList`\n"
			"\n"
			"  Modifying the returned sequence will modify the internal state of the *GpmlTopologicalNetwork* instance:\n"
			"  ::\n"
			"\n"
			"    interiors = topological_network.get_interiors()\n"
			"\n"
			"    # Append an interior\n"
			"    interiors.append(pygplates.GpmlPropertyDelegate(...))\n")
		;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlTopologicalNetwork>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTopologicalNetwork>();
}

void
export_gpml_topological_point()
{
	// Use the 'non-const' overload so GpmlTopologicalPoint can be modified via python...
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
			(GPlatesPropertyValues::GpmlTopologicalPoint::*get_property_delegate)() =
					&GPlatesPropertyValues::GpmlTopologicalPoint::get_source_geometry;

	//
	// GpmlTopologicalPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
		GPlatesPropertyValues::GpmlTopologicalPoint,
		GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type,
		bp::bases<GPlatesPropertyValues::GpmlTopologicalSection>,
		boost::noncopyable>(
			"GpmlTopologicalPoint",
			"A topological section referencing a point geometry.\n"
			"\n"
			"  .. versionadded:: 0.21\n",
			// We need this (even though "__init__" is defined) since
			// there is no publicly-accessible default constructor...
			bp::no_init)
		.def("__init__",
			bp::make_constructor(
				&GPlatesPropertyValues::GpmlTopologicalPoint::create,
				bp::default_call_policies(),
				(bp::arg("gpml_property_delegate"))),
			"__init__(gpml_property_delegate)\n"
			"  Create a topological point section property value that references a feature property containing a point geometry.\n"
			"\n"
			"  :param gpml_property_delegate: the point geometry property value\n"
			"  :type gpml_property_delegate: :class:`GpmlPropertyDelegate`\n"
			"\n"
			"  ::\n"
			"\n"
			"    topological_point_section = pygplates.GpmlTopologicalPoint(point_property_delegate)\n")
		.def("get_property_delegate",
			get_property_delegate,
			"get_property_delegate()\n"
			"  Returns the property value that references/delegates the source point geometry.\n"
			"\n"
			"  :rtype: :class:`GpmlPropertyDelegate`\n")
		.def("set_property_delegate",
			&GPlatesPropertyValues::GpmlTopologicalPoint::set_source_geometry,
			(bp::arg("gpml_property_delegate")),
			"set_property_delegate(gpml_property_delegate)\n"
			"  Sets the property value that references/delegates the source point geometry.\n"
			"\n"
			"  :param gpml_property_delegate: the point geometry property value\n"
			"  :type gpml_property_delegate: :class:`GpmlPropertyDelegate`\n")
		;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlTopologicalPoint>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTopologicalPoint>();
}


namespace GPlatesApi
{
	GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type
	gpml_topological_polygon_create(
			bp::object exterior_sections) // Any python sequence (eg, list, tuple).
	{
		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> exterior_sections_vector;
		PythonExtractUtils::extract_iterable(
				exterior_sections_vector,
				exterior_sections,
				"Expected a sequence of exterior sections (GpmlTopologicalSection)");

		return GPlatesPropertyValues::GpmlTopologicalPolygon::create(exterior_sections_vector);
	}

	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTopologicalSection>::non_null_ptr_type
	gpml_topological_polygon_get_exterior_sections(
			GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
	{
		return &gpml_topological_polygon.exterior_sections();
	}
}

void
export_gpml_topological_polygon()
{
	//
	// GpmlTopologicalPolygon - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
		GPlatesPropertyValues::GpmlTopologicalPolygon,
		GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type,
		bp::bases<GPlatesModel::PropertyValue>,
		boost::noncopyable>(
			"GpmlTopologicalPolygon",
			"A topological polygon geometry that is resolved from topological sections.\n"
			"\n"
			"  .. versionadded:: 0.21\n",
			// We need this (even though "__init__" is defined) since
			// there is no publicly-accessible default constructor...
			bp::no_init)
		.def("__init__",
			bp::make_constructor(
				&GPlatesApi::gpml_topological_polygon_create,
				bp::default_call_policies(),
				(bp::arg("exterior_sections"))),
			"__init__(exterior_sections)\n"
			"  Create a topological polygon made from topological sections.\n"
			"\n"
			"  :param exterior_sections: A sequence of :class:`GpmlTopologicalSection` elements\n"
			"  :type exterior_sections: Any sequence such as a ``list`` or a ``tuple``\n"
			"\n"
			"  ::\n"
			"\n"
			"    topological_polygon = pygplates.GpmlTopologicalPolygon(topological_sections)\n")
		.def("get_exterior_sections",
			&GPlatesApi::gpml_topological_polygon_get_exterior_sections,
			"get_exterior_sections()\n"
			"  Same as :meth:`get_boundary_sections`.\n")
		.def("get_boundary_sections",
			&GPlatesApi::gpml_topological_polygon_get_exterior_sections,
			"get_boundary_sections()\n"
			"  Returns the :class:`boundary sections<GpmlTopologicalSection>` in a sequence that behaves as a python ``list``.\n"
			"\n"
			"  :rtype: :class:`GpmlTopologicalSectionList`\n"
			"\n"
			"  Modifying the returned sequence will modify the internal state of the *GpmlTopologicalPolygon* instance:\n"
			"  ::\n"
			"\n"
			"    boundary_sections = topological_polygon.get_boundary_sections()\n"
			"\n"
			"    # Append a section\n"
			"    boundary_sections.append(pygplates.GpmlTopologicalLineSection(...))\n"
			"\n"
			"  .. versionadded:: 0.24\n")
		;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::GpmlTopologicalPolygon>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::GpmlTopologicalPolygon>();
}

void
export_xs_boolean()
{
	//
	// XsBoolean - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsBoolean,
			GPlatesPropertyValues::XsBoolean::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsBoolean",
					"A property value that represents a boolean value. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsBoolean::create,
						bp::default_call_policies(),
						(bp::arg("boolean_value"))),
				"__init__(boolean_value)\n"
				"  Create a boolean property value from a boolean value.\n"
				"\n"
				"  :param boolean_value: the boolean value\n"
				"  :type boolean_value: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    boolean_property = pygplates.XsBoolean(boolean_value)\n")
		.def("get_boolean",
				&GPlatesPropertyValues::XsBoolean::get_value,
				"get_boolean()\n"
				"  Returns the boolean value.\n"
				"\n"
				"  :rtype: bool\n")
		.def("set_boolean",
				&GPlatesPropertyValues::XsBoolean::set_value,
				(bp::arg("boolean_value")),
				"set_boolean(boolean_value)\n"
				"  Sets the boolean value.\n"
				"\n"
				"  :param boolean_value: the boolean value\n"
				"  :type boolean_value: bool\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::XsBoolean::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::XsBoolean>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::XsBoolean>();
}


void
export_xs_double()
{
	//
	// XsDouble - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsDouble,
			GPlatesPropertyValues::XsDouble::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsDouble",
					"A property value that represents a *double*-precision floating-point number. "
					"Note that, in python, the ``float`` built-in type is double-precision. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsDouble::create,
						bp::default_call_policies(),
						(bp::arg("float_value"))),
				"__init__(float_value)\n"
				"  Create a floating-point property value from a ``float``.\n"
				"\n"
				"  :param float_value: the floating-point value\n"
				"  :type float_value: float\n"
				"\n"
				"  ::\n"
				"\n"
				"    float_property = pygplates.XsDouble(float_value)\n")
		.def("get_double",
				&GPlatesPropertyValues::XsDouble::get_value,
				"get_double()\n"
				"  Returns the floating-point value.\n"
				"\n"
				"  :rtype: float\n")
		.def("set_double",
				&GPlatesPropertyValues::XsDouble::set_value,
				(bp::arg("float_value")),
				"set_double(float_value)\n"
				"  Sets the floating-point value.\n"
				"\n"
				"  :param float_value: the floating-point value\n"
				"  :type float_value: float\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::XsDouble::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::XsDouble>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::XsDouble>();
}


void
export_xs_integer()
{
	//
	// XsInteger - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsInteger,
			GPlatesPropertyValues::XsInteger::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsInteger",
					"A property value that represents an integer number. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsInteger::create,
						bp::default_call_policies(),
						(bp::arg("integer_value"))),
				"__init__(integer_value)\n"
				"  Create an integer property value from an ``int``.\n"
				"\n"
				"  :param integer_value: the integer value\n"
				"  :type integer_value: int\n"
				"\n"
				"  ::\n"
				"\n"
				"    integer_property = pygplates.XsInteger(integer_value)\n")
		.def("get_integer",
				&GPlatesPropertyValues::XsInteger::get_value,
				"get_integer()\n"
				"  Returns the integer value.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_integer",
				&GPlatesPropertyValues::XsInteger::set_value,
				(bp::arg("integer_value")),
				"set_integer(integer_value)\n"
				"  Sets the integer value.\n"
				"\n"
				"  :param integer_value: the integer value\n"
				"  :type integer_value: int\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::XsInteger::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::XsInteger>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::XsInteger>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::XsString::non_null_ptr_type
	xs_string_create(
			const GPlatesPropertyValues::TextContent &string)
	{
		return GPlatesPropertyValues::XsString::create(string);
	}
}

void
export_xs_string()
{
	//
	// XsString - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsString,
			GPlatesPropertyValues::XsString::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsString",
					"A property value that represents a string. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::xs_string_create,
						bp::default_call_policies(),
						(bp::arg("string"))),
				"__init__(string)\n"
				"  Create a string property value from a string.\n"
				"\n"
				"  :param string: the string\n"
				"  :type string: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    string_property = pygplates.XsString(string)\n")
		.def("get_string",
				&GPlatesPropertyValues::XsString::get_value,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_string()\n"
				"  Returns the string.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_string",
				&GPlatesPropertyValues::XsString::set_value,
				(bp::arg("string")),
				"set_string(string)\n"
				"  Sets the string.\n"
				"\n"
				"  :param string: the string\n"
				"  :type string: string\n")
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesPropertyValues::XsString::non_null_ptr_type>())
	;

	// Register property value type as a structural type (GPlatesPropertyValues::StructuralType).
	GPlatesApi::register_structural_type<GPlatesPropertyValues::XsString>();

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesPropertyValues::XsString>();
}


void
export_property_values()
{
	// Since PropertyValue is the base class it must be registered first to avoid runtime error.
	export_property_value();

	//////////////////////////////////////////////////////////////////////////
	// NOTE: Please keep the property values alphabetically ordered.
	//       Unless there are inheritance dependencies.
	//////////////////////////////////////////////////////////////////////////

	export_enumeration();
	export_gml_data_block();
	export_gml_line_string();
	export_gml_multi_point();
	export_gml_orientable_curve();
	export_gml_point();
	export_gml_polygon();
	export_gml_time_instant();
	export_gml_time_period();

	export_gpml_array();
	export_gpml_constant_value();

	// GpmlInterpolationFunction and its derived classes.
	// Since GpmlInterpolationFunction is the base class it must be registered first to avoid runtime error.
	export_gpml_interpolation_function();
	export_gpml_finite_rotation_slerp();

	export_gpml_finite_rotation();
	export_gpml_hot_spot_trail_mark();
	export_gpml_irregular_sampling();
	export_gpml_key_value_dictionary();
	export_gpml_old_plates_header();
	export_gpml_piecewise_aggregation();
	export_gpml_plate_id();
	export_gpml_polarity_chron_id();
	export_gpml_property_delegate();

	export_gpml_time_sample(); // Not actually a property value.
	export_gpml_time_window(); // Not actually a property value.

	// GpmlTopologicalSection and its derived classes.
	// Since GpmlTopologicalSection is the base class it must be registered first to avoid runtime error.
	export_gpml_topological_section();
	export_gpml_topological_line_section();
	export_gpml_topological_point();

	// Topological line, polygon and network.
	export_gpml_topological_line();
	export_gpml_topological_polygon();
	export_gpml_topological_network();

	export_xs_boolean();
	export_xs_double();
	export_xs_integer();
	export_xs_string();
}

// This is here at the end of the layer because the problem appears to reside
// in a template being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")
