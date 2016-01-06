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
#include <ostream>
#include <set>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "PyFeature.h"

#include "PyGPlatesModule.h"
#include "PyInformationModel.h"
#include "PyPropertyValues.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructParams.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ScalarCoverageFeatureProperties.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"
#include "global/python.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "model/FeatureType.h"
#include "model/Gpgim.h"
#include "model/GpgimFeatureClass.h"
#include "model/GpgimProperty.h"
#include "model/ModelUtils.h"
#include "model/RevisionId.h"
#include "model/TopLevelProperty.h"

#include "property-values/Enumeration.h"
#include "property-values/EnumerationContent.h"
#include "property-values/EnumerationType.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlIrregularSampling.h"

#include "utils/UnicodeString.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// Forward declaration.
	bp::object
	feature_handle_set_property(
			GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			bp::object property_value_object,
			VerifyInformationModel::Value verify_information_model);


	namespace
	{
		/**
		 * Returns the default geometry property name associated with the specified feature type.
		 */
		boost::optional<GPlatesModel::PropertyName>
		get_default_geometry_property_name(
				const GPlatesModel::FeatureType &feature_type)
		{
			const GPlatesModel::Gpgim &gpgim = GPlatesModel::Gpgim::instance();

			// Get the GPGIM feature class.
			boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
					gpgim.get_feature_class(feature_type);
			if (!gpgim_feature_class)
			{
				return boost::none;
			}

			// Get the feature's default geometry property.
			boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> default_geometry_feature_property =
					gpgim_feature_class.get()->get_default_geometry_feature_property();
			if (!default_geometry_feature_property)
			{
				return boost::none;
			}

			return default_geometry_feature_property.get()->get_property_name();
		}

		/**
		 * Returns true if the specified property name supports the type of the specified geometry.
		 */
		bool
		is_geometry_type_supported_by_property(
				const GPlatesMaths::GeometryOnSphere &geometry,
				const GPlatesModel::PropertyName &property_name)
		{
			const GPlatesMaths::GeometryType::Value geometry_type =
					GPlatesAppLogic::GeometryUtils::get_geometry_type(geometry);

			// Get the property value structural type associated with the geometry type.
			boost::optional<GPlatesPropertyValues::StructuralType> geometry_structural_type;
			switch (geometry_type)
			{
			case GPlatesMaths::GeometryType::POINT:
				geometry_structural_type = GPlatesPropertyValues::StructuralType::create_gml("Point");
				break;

			case GPlatesMaths::GeometryType::MULTIPOINT:
				geometry_structural_type = GPlatesPropertyValues::StructuralType::create_gml("MultiPoint");
				break;

			case GPlatesMaths::GeometryType::POLYLINE:
				geometry_structural_type = GPlatesPropertyValues::StructuralType::create_gml("LineString");
				break;

			case GPlatesMaths::GeometryType::POLYGON:
				geometry_structural_type = GPlatesPropertyValues::StructuralType::create_gml("Polygon");
				break;

			case GPlatesMaths::GeometryType::NONE:
			default:
				break;
			}

			if (!geometry_structural_type)
			{
				return false;
			}

			const GPlatesModel::Gpgim &gpgim = GPlatesModel::Gpgim::instance();

			// Get the GPGIM property using the property name.
			boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_property =
					gpgim.get_property(property_name);
			if (!gpgim_property)
			{
				return false;
			}

			const GPlatesModel::GpgimProperty::structural_type_seq_type &gpgim_structural_types =
					gpgim_property.get()->get_structural_types();

			// If any allowed structural type matches then the geometry type is supported.
			BOOST_FOREACH(
					GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type gpgim_structural_type,
					gpgim_structural_types)
			{
				if (geometry_structural_type == gpgim_structural_type->get_structural_type())
				{
					return true;
				}
			}

			return false;
		}

		/**
		 * Return derived geometry type as a string.
		 */
		QString
		get_geometry_type_as_string(
				const GPlatesMaths::GeometryOnSphere &geometry)
		{
			const GPlatesMaths::GeometryType::Value geometry_type =
					GPlatesAppLogic::GeometryUtils::get_geometry_type(geometry);

			switch (geometry_type)
			{
			case GPlatesMaths::GeometryType::POINT:
				return QString("PointOnSphere");

			case GPlatesMaths::GeometryType::MULTIPOINT:
				return QString("MultiPointOnSphere");

			case GPlatesMaths::GeometryType::POLYLINE:
				return QString("PolylineOnSphere");

			case GPlatesMaths::GeometryType::POLYGON:
				return QString("PolygonOnSphere");

			case GPlatesMaths::GeometryType::NONE:
			default:
				break;
			}

			// Should not be able to get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			return QString(); // Keep compiler happy.
		}

		/**
		 * Throws @a InformationModelException if specified property name does not support the type
		 * of the specified geometry (and @a verify_information_model requested checking).
		 */
		void
		verify_geometry_type_supported_by_property(
				const GPlatesMaths::GeometryOnSphere &geometry,
				const GPlatesModel::PropertyName &property_name,
				VerifyInformationModel::Value verify_information_model)
		{
			// Make sure geometry type is supported by property (if requested to check).
			if (verify_information_model == VerifyInformationModel::YES &&
				!is_geometry_type_supported_by_property(geometry, property_name))
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The geometry type '") +
								get_geometry_type_as_string(geometry) +
								"' is not supported by property name '" +
								convert_qualified_xml_name_to_qstring(property_name) + "'");
			}
		}

		/**
		 * Throws InformationModelException if @a feature_type does not inherit directly or indirectly
		 * from @a ancestor_feature_type.
		 */
		void
		verify_feature_type_inherits(
				const GPlatesModel::FeatureType &feature_type,
				const GPlatesModel::FeatureType &ancestor_feature_type)
		{
			boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
					GPlatesModel::Gpgim::instance().get_feature_class(feature_type);

			if (!gpgim_feature_class)
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The feature type '") +
								convert_qualified_xml_name_to_qstring(feature_type) +
								"' was not recognised as a valid type by the GPGIM");
			}

			if (!gpgim_feature_class.get()->does_inherit_from(ancestor_feature_type))
			{
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The feature type '") +
								convert_qualified_xml_name_to_qstring(feature_type) +
								"' is not a reconstructable feature (does not inherit '" +
								convert_qualified_xml_name_to_qstring(ancestor_feature_type) +
								"')");
			}
		}

		// FIXME: Avoid duplicating same function from "PyPropertyValues.cc".
		void
		verify_enumeration_type_and_content(
				const GPlatesPropertyValues::EnumerationType &type,
				const QString &content)
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
				if (content == enum_content.value)
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
								content +
								"' is not supported by enumeration type '" +
								convert_qualified_xml_name_to_qstring(type) + "'");
			}
		}

		/**
		 * Extract the (begin, end) times from a tuple and set the valid time on the specified feature.
		 */
		void
		set_valid_time_from_tuple(
				bp::object feature_object,
				bp::object valid_time,
				VerifyInformationModel::Value verify_information_model)
		{
			bp::extract<bp::tuple> extract_tuple(valid_time);
			if (!extract_tuple.check())
			{
				PyErr_SetString(PyExc_TypeError, "Expecting a (begin, end) tuple for 'valid_time'");
				bp::throw_error_already_set();
			}

			bp::tuple valid_time_tuple = extract_tuple();
			if (bp::len(valid_time_tuple) != 2)
			{
				PyErr_SetString(PyExc_TypeError, "Expecting a (begin, end) tuple for 'valid_time'");
				bp::throw_error_already_set();
			}

			bp::extract<GPlatesPropertyValues::GeoTimeInstant> extract_begin_time(valid_time[0]);
			bp::extract<GPlatesPropertyValues::GeoTimeInstant> extract_end_time(valid_time[1]);
			if (!extract_begin_time.check() ||
				!extract_end_time.check())
			{
				PyErr_SetString(PyExc_TypeError, "Expecting float or GeoTimeInstant for 'valid_time' tuple values");
				bp::throw_error_already_set();
			}

			// Call python since Feature.set_valid_time is implemented in python code...
			feature_object.attr("set_valid_time")(extract_begin_time(), extract_end_time(), verify_information_model);
		}

		/**
		 * Get the reverse-reconstruct rotation model (and reconstruction and anchor plate id).
		 */
		std::pair<
				GPlatesAppLogic::ReconstructionTreeCreator,
				double/*reconstruction_time*/>
		extract_reverse_reconstruct_parameters(
				bp::object reverse_reconstruct_object)
		{
			const char *type_error_string =
					"Expecting a (rotation model, reconstruction time [, anchor plate id]) "
					"tuple for 'reverse_reconstruct'";

			bp::extract<bp::tuple> extract_tuple(reverse_reconstruct_object);
			if (!extract_tuple.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}

			bp::tuple reverse_reconstruct_tuple = extract_tuple();
			const unsigned int tuple_len = bp::len(reverse_reconstruct_tuple);
			if (tuple_len != 3 && tuple_len != 2)
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}

			bp::extract<GPlatesApi::RotationModel::non_null_ptr_type> extract_rotation_model(reverse_reconstruct_tuple[0]);
			bp::extract<GPlatesPropertyValues::GeoTimeInstant> extract_reconstruction_geo_time_instant(reverse_reconstruct_tuple[1]);
			if (!extract_rotation_model.check() ||
				!extract_reconstruction_geo_time_instant.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}

			const GPlatesApi::RotationModel::non_null_ptr_type rotation_model = extract_rotation_model();

			// Time must not be distant past/future.
			const GPlatesPropertyValues::GeoTimeInstant reconstruction_geo_time_instant = extract_reconstruction_geo_time_instant();
			if (!reconstruction_geo_time_instant.is_real())
			{
				PyErr_SetString(PyExc_ValueError,
						"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
				bp::throw_error_already_set();
			}
			const double reconstruction_time = reconstruction_geo_time_instant.value();

			GPlatesModel::integer_plate_id_type anchor_plate_id = 0;
			if (tuple_len == 3)
			{
				bp::extract<GPlatesModel::integer_plate_id_type> extract_anchor_plate_id(reverse_reconstruct_tuple[2]);
				if (!extract_anchor_plate_id.check())
				{
					PyErr_SetString(PyExc_TypeError, type_error_string);
					bp::throw_error_already_set();
				}

				anchor_plate_id = extract_anchor_plate_id();
			}

			// Adapt the reconstruction tree creator to a new one that has 'anchor_plate_id' as its default.
			// This ensures we will reverse reconstruct using the correct anchor plate.
			GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
					GPlatesAppLogic::create_cached_reconstruction_tree_adaptor(
							rotation_model->get_reconstruction_tree_creator(),
							anchor_plate_id);

			return std::make_pair(reconstruction_tree_creator, reconstruction_time);
		}

		/**
		 * Reverse reconstruct the specified geometry using the specified feature (properties) and reverse reconstruct parameters.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reverse_reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				GPlatesModel::FeatureHandle &feature_handle,
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruction_method_registry,
				const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator,
				const double &reconstruction_time)
		{
			return GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
					geometry,
					reconstruction_method_registry,
					feature_handle.reference(),
					reconstruction_tree_creator,
					GPlatesAppLogic::ReconstructParams(),
					reconstruction_time,
					true/*reverse_reconstruct*/);
		}

		/**
		 * Set the geometry as a property on the feature and check information model if requested
		 * (and reverse reconstruct if requested).
		 *
		 * Also optionally set the range (GmlDataBlock) as a property on the feature.
		 *
		 * Returns the feature property containing the geometry, or a tuple of properties containing
		 * the geometry (coverage domain) and the coverage range.
		 *
		 * Note: The range property name is obtained from the domain (geometry) property name (if needed).
		 */
		bp::object
		set_geometry(
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry,
				const GPlatesModel::PropertyName &geometry_property_name,
				bp::object reverse_reconstruct_object,
				VerifyInformationModel::Value verify_information_model,
				boost::optional<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type> coverage_range_property_value = boost::none)
		{
			//
			// Set the geometry property.
			//

			// Make sure geometry type is supported by property (if requested to check).
			verify_geometry_type_supported_by_property(*geometry, geometry_property_name, verify_information_model);

			// If we need to reverse reconstruct the geometry.
			if (reverse_reconstruct_object != bp::object()/*Py_None*/)
			{
				std::pair<GPlatesAppLogic::ReconstructionTreeCreator, double/*reconstruction_time*/>
						reverse_reconstruct_parameters = extract_reverse_reconstruct_parameters(
								reverse_reconstruct_object);

				GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
				geometry = reverse_reconstruct_geometry(
						geometry,
						feature_handle,
						reconstruct_method_registry,
						reverse_reconstruct_parameters.first,
						reverse_reconstruct_parameters.second);
			}

			// Wrap the geometry in a property value.
			GPlatesModel::PropertyValue::non_null_ptr_type geometry_property_value =
					GPlatesAppLogic::GeometryUtils::create_geometry_property_value(geometry);

			// Set the geometry property value in the feature.
			const bp::object geometry_property_object = feature_handle_set_property(
					feature_handle,
					geometry_property_name,
					bp::object(geometry_property_value),
					verify_information_model);

			// Get the coverage range property name associated with the domain property name (if any).
			boost::optional<GPlatesModel::PropertyName> range_property_name =
					GPlatesAppLogic::ScalarCoverageFeatureProperties::get_range_property_name_from_domain(
							geometry_property_name);

			// If we're just setting a geometry (and not a coverage).
			if (!coverage_range_property_value)
			{
				// We still remove any coverages associated with the geometry so that the geometry
				// is not interpreted as a coverage domain.
				//
				// It's not an error if a coverage is not supported for the geometry property name
				// because the caller was not trying to set a coverage (only setting a geometry).
				if (range_property_name)
				{
					feature_handle.remove_properties_by_name(range_property_name.get());
				}

				return geometry_property_object;
			}

			//
			// We're also setting the coverage range (where coverage domain is the geometry).
			//

			// If the geometry property name does not support a coverage then this is an error
			// because the caller is trying to set a coverage (and not just a geometry).
			if (!range_property_name)
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("Geometry property name '" +
								convert_qualified_xml_name_to_qstring(geometry_property_name) +
								"' does not support coverages"));
			}

			// Number of points in domain must match number of scalar values in range.
			const unsigned int num_domain_geometry_points =
					GPlatesAppLogic::GeometryUtils::get_num_geometry_points(*geometry);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!coverage_range_property_value.get()->tuple_list().empty(),
					GPLATES_ASSERTION_SOURCE);
			// Just test the scalar values length for the first scalar type (all types should already have the same length).
			if (num_domain_geometry_points != coverage_range_property_value.get()->tuple_list().front().get()->get_coordinates().size())
			{
				PyErr_SetString(PyExc_ValueError, "Number of scalar values in coverage must match number of points in geometry");
				bp::throw_error_already_set();
			}

			// Set the coverage range property in the feature.
			const bp::object coverage_range_property_object = feature_handle_set_property(
					feature_handle,
					range_property_name.get(),
					bp::object(coverage_range_property_value.get()),
					verify_information_model);

			return bp::make_tuple(geometry_property_object, coverage_range_property_object);
		}

		/**
		 * Set geometries as properties on the feature and check information model if requested
		 * (and reverse reconstruct if requested).
		 *
		 * Also optionally set ranges (GmlDataBlock's) as properties on the feature.
		 *
		 * Returns a list of the feature properties containing the geometries, or a list of 2-tuples
		 * with each 2-tuple containing a geometry (domain) property and a coverage range property.
		 *
		 * Note: The range property name is obtained from the domain (geometry) property name (if needed).
		 */
		bp::object
		set_geometries(
				GPlatesModel::FeatureHandle &feature_handle,
				const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &geometries,
				const GPlatesModel::PropertyName &geometry_property_name,
				bp::object reverse_reconstruct_object,
				VerifyInformationModel::Value verify_information_model,
				boost::optional<const std::vector<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type> &>
						coverage_range_property_values = boost::none)
		{
			//
			// Set the geometry properties.
			//

			// Get reverse reconstruct parameters if we're going to reverse reconstruct geometries.
			boost::optional< std::pair<GPlatesAppLogic::ReconstructionTreeCreator, double/*reconstruction_time*/> >
					reverse_reconstruct_parameters;
			GPlatesAppLogic::ReconstructMethodRegistry reconstruction_method_registry;
			if (reverse_reconstruct_object != bp::object()/*Py_None*/)
			{
				reverse_reconstruct_parameters = extract_reverse_reconstruct_parameters(reverse_reconstruct_object);
			}

			// Wrap the geometries in property values.
			bp::list geometry_property_values;

			BOOST_FOREACH(GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry, geometries)
			{
				// Make sure geometry type is supported by property (if requested to check).
				verify_geometry_type_supported_by_property(*geometry, geometry_property_name, verify_information_model);

				// If we need to reverse reconstruct the geometry.
				if (reverse_reconstruct_parameters)
				{
					geometry = reverse_reconstruct_geometry(
							geometry,
							feature_handle,
							reconstruction_method_registry,
							reverse_reconstruct_parameters->first,
							reverse_reconstruct_parameters->second);
				}

				// Wrap the current geometry in a property value.
				GPlatesModel::PropertyValue::non_null_ptr_type geometry_property_value =
						GPlatesAppLogic::GeometryUtils::create_geometry_property_value(geometry);

				geometry_property_values.append(geometry_property_value);
			}

			// Set the geometry property values in the feature.
			const bp::object geometry_property_list_object = feature_handle_set_property(
					feature_handle,
					geometry_property_name,
					geometry_property_values,
					verify_information_model);

			// Get the coverage range property name associated with the domain property name (if any).
			boost::optional<GPlatesModel::PropertyName> range_property_name =
					GPlatesAppLogic::ScalarCoverageFeatureProperties::get_range_property_name_from_domain(
							geometry_property_name);

			// If we're just setting geometries (and not coverages).
			if (!coverage_range_property_values)
			{
				// We still remove any coverages associated with the geometries so that the geometries
				// are not interpreted as coverage domains.
				//
				// It's not an error if coverages are not supported for the geometry property name
				// because the caller was not trying to set coverages (only setting geometries).
				if (range_property_name)
				{
					feature_handle.remove_properties_by_name(range_property_name.get());
				}

				return geometry_property_list_object;
			}

			//
			// We're also setting coverage ranges (where coverage domains are the geometries).
			//

			// If the geometry property name does not support coverages then this is an error
			// because the caller is trying to set coverages (and not just geometries).
			if (!range_property_name)
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("Geometry property name '" +
								convert_qualified_xml_name_to_qstring(geometry_property_name) +
								"' does not support coverages"));
			}

			// Both coverage domains and ranges should be the same length.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					geometries.size() == coverage_range_property_values->size(),
					GPLATES_ASSERTION_SOURCE);

			const unsigned int num_coverages = geometries.size();

			// Make sure the number of points in each domain matches number of scalar values in associated range.
			// Also make sure no two domains have the same number of points (otherwise it's ambiguous
			// which range belongs to which domain since they use the same domain/range property name).
			std::set<unsigned int> num_domain_points_set;
			for (unsigned int c = 0; c < num_coverages; ++c)
			{
				// Number of points in domain must match number of scalar values in range.
				const unsigned int num_domain_geometry_points =
						GPlatesAppLogic::GeometryUtils::get_num_geometry_points(*geometries[c]);

				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						!coverage_range_property_values.get()[c]->tuple_list().empty(),
						GPLATES_ASSERTION_SOURCE);
				// Just test the scalar values length for the first scalar type (all types should already have the same length).
				if (num_domain_geometry_points != coverage_range_property_values.get()[c]->tuple_list().front().get()->get_coordinates().size())
				{
					PyErr_SetString(PyExc_ValueError, "Number of scalar values in coverage must match number of points in geometry");
					bp::throw_error_already_set();
				}

				// Each coverage should have a different number of points (ie, should get inserted into std::set).
				GPlatesGlobal::Assert<AmbiguousGeometryCoverageException>(
						num_domain_points_set.insert(num_domain_geometry_points).second,
						GPLATES_ASSERTION_SOURCE,
						geometry_property_name);
			}

			// Wrap the coverage ranges in Python property values.
			bp::list coverage_range_property_values_list;

			BOOST_FOREACH(
					GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type coverage_range,
					coverage_range_property_values.get())
			{
				coverage_range_property_values_list.append(coverage_range);
			}

			// Set the coverage range property values in the feature.
			const bp::object coverage_range_property_list_object = feature_handle_set_property(
					feature_handle,
					range_property_name.get(),
					coverage_range_property_values_list,
					verify_information_model);
			
			bp::list coverage_domain_range_property_list_object;

			// Both coverage domain and range property lists should be the same length.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					bp::len(coverage_range_property_list_object) == bp::ssize_t(num_coverages) &&
						bp::len(geometry_property_list_object) == bp::ssize_t(num_coverages),
					GPLATES_ASSERTION_SOURCE);

			// Return a list of tuples (rather than a tuple of lists) since we want to mirror the
			// input which was a sequence of (GeometryOnSphere, coverage-range) tuples.
			for (unsigned int n = 0; n < num_coverages; ++n)
			{
				coverage_domain_range_property_list_object.append(
						bp::make_tuple(
								geometry_property_list_object[n],
								coverage_range_property_list_object[n]));
			}

			return coverage_domain_range_property_list_object;
		}
	}


	/**
	 * Enumeration to determine how properties are returned.
	 */
	namespace PropertyReturn
	{
		enum Value
		{
			EXACTLY_ONE, // Returns a single element only if there's one match to the query.
			FIRST,       // Returns the first element that matches the query.
			ALL          // Returns all elements that matches the query.
		};
	};


	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create(
			boost::optional<GPlatesModel::FeatureType> feature_type,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			VerifyInformationModel::Value verify_information_model)
	{
		// Default to unclassified feature - since that supports any combination of properties.
		if (!feature_type)
		{
			feature_type = GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
		}
		else if (verify_information_model == VerifyInformationModel::YES)
		{
			if (!GPlatesModel::Gpgim::instance().get_feature_class(feature_type.get()))
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("The feature type '") +
								convert_qualified_xml_name_to_qstring(feature_type.get()) +
								"' was not recognised as a valid type by the GPGIM");
			}
		}

		// Create a unique feature id if none specified.
		if (!feature_id)
		{
			feature_id = GPlatesModel::FeatureId();
		}

		return GPlatesModel::FeatureHandle::create(feature_type.get(), feature_id.get());
	}

	/**
	 * Clone an existing feature.
	 *
	 * NOTE: We don't use FeatureHandle::clone() because it currently does a shallow copy
	 * instead of a deep copy.
	 * FIXME: Once FeatureHandle has been updated to use the same revisioning system as
	 * TopLevelProperty and PropertyValue then just delegate directly to FeatureHandle::clone().
	 */
	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_clone(
			GPlatesModel::FeatureHandle &feature_handle)
	{
		GPlatesModel::FeatureHandle::non_null_ptr_type cloned_feature =
				GPlatesModel::FeatureHandle::create(feature_handle.feature_type());

		// Iterate over the properties of the feature and clone them.
		GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
		GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
		for ( ; properties_iter != properties_end; ++properties_iter)
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

			cloned_feature->add(feature_property->clone());
		}

		return cloned_feature;
	}

	bp::object
	feature_handle_add_property_internal(
			GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			bp::object property_value_object,
			VerifyInformationModel::Value verify_information_model,
			const char *type_error_string)
	{
		// 'property_value_object' is either a property value or a sequence of property values.
		bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type> extract_property_value(property_value_object);
		if (extract_property_value.check())
		{
			GPlatesModel::PropertyValue::non_null_ptr_type property_value = extract_property_value();

			if (verify_information_model == VerifyInformationModel::NO)
			{
				// Just create a top-level property without checking information model.
				GPlatesModel::TopLevelProperty::non_null_ptr_type property =
						GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);

				GPlatesModel::FeatureHandle::iterator property_iter = feature_handle.add(property);

				// Return the newly added property.
				return bp::object(*property_iter);
			}

			// Only add property if valid property name for the feature's type.
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
			boost::optional<GPlatesModel::FeatureHandle::iterator> feature_property_iter =
					GPlatesModel::ModelUtils::add_property(
							feature_handle.reference(),
							property_name,
							property_value,
							true/*check_property_name_allowed_for_feature_type*/,
							true/*check_property_multiplicity*/,
							true/*check_property_value_type*/,
							&add_property_error_code);
			if (!feature_property_iter)
			{
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)));
			}

			// Return the newly added property.
			return bp::object(*feature_property_iter.get());
		}
		// ...else a sequence of property values.

		// Attempt to extract a sequence of property values.
		typedef std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> property_value_seq_type;
		property_value_seq_type property_values;
		PythonExtractUtils::extract_iterable(property_values, property_value_object, type_error_string);

		if (verify_information_model == VerifyInformationModel::NO)
		{
			bp::list properties;

			// Just create top-level properties without checking information model.
			property_value_seq_type::const_iterator property_values_iter = property_values.begin();
			property_value_seq_type::const_iterator property_values_end = property_values.end();
			for ( ; property_values_iter != property_values_end; ++property_values_iter)
			{
				GPlatesModel::PropertyValue::non_null_ptr_type property_value = *property_values_iter;

				GPlatesModel::TopLevelProperty::non_null_ptr_type property =
						GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);

				GPlatesModel::FeatureHandle::iterator feature_property_iter = feature_handle.add(property);

				properties.append(*feature_property_iter);
			}

			// Return the property list.
			return properties;
		}

		bp::list properties;

		property_value_seq_type::const_iterator property_values_iter = property_values.begin();
		property_value_seq_type::const_iterator property_values_end = property_values.end();
		for ( ; property_values_iter != property_values_end; ++property_values_iter)
		{
			GPlatesModel::PropertyValue::non_null_ptr_type property_value = *property_values_iter;

			// Only add property if valid property name for the feature's type.
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
			boost::optional<GPlatesModel::FeatureHandle::iterator> feature_property_iter =
					GPlatesModel::ModelUtils::add_property(
							feature_handle.reference(),
							property_name,
							property_value,
							true/*check_property_name_allowed_for_feature_type*/,
							true/*check_property_multiplicity*/,
							true/*check_property_value_type*/,
							&add_property_error_code);
			if (!feature_property_iter)
			{
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)));
			}

			properties.append(*feature_property_iter.get());
		}

		// Return the property list.
		return properties;
	}

	bp::object
	feature_handle_add_property(
			GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			bp::object property_value_object,
			VerifyInformationModel::Value verify_information_model)
	{
		return feature_handle_add_property_internal(
				feature_handle,
				property_name,
				property_value_object,
				verify_information_model,
				"Expected a PropertyName and PropertyValue, or PropertyName and sequence of PropertyValue");
	}

	bp::list
	feature_handle_add_properties(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object properties_object,
			VerifyInformationModel::Value verify_information_model)
	{
		bp::list properties_list;

		const char *type_error_string = "Expected a sequence of (PropertyName, PropertyValue(s))";

		std::vector<bp::object> properties;
		PythonExtractUtils::extract_iterable(properties, properties_object, type_error_string);

		// Retrieve the (PropertyName, PropertyValue) pairs.
		std::vector<bp::object>::const_iterator properties_iter = properties.begin();
		std::vector<bp::object>::const_iterator properties_end = properties.end();
		for ( ; properties_iter != properties_end; ++properties_iter)
		{
			// Attempt to extract the property name and value.
			std::vector<bp::object> name_value_vector;
			PythonExtractUtils::extract_iterable(name_value_vector, *properties_iter, type_error_string);

			if (name_value_vector.size() != 2)   // (PropertyName, PropertyValue(s))
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}
			// A two-element sequence containing property name and property value(s).
			const bp::object property_name_object = name_value_vector[0];
			const bp::object property_value_object = name_value_vector[1];

			// Make sure we can extract PropertyName.
			// The PropertyValue(s) is handled by 'feature_handle_add_property()'.
			bp::extract<GPlatesModel::PropertyName> extract_property_name(property_name_object);
			if (!extract_property_name.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}

			bp::object property =
					feature_handle_add_property_internal(
							feature_handle,
							extract_property_name(),
							property_value_object,
							verify_information_model,
							type_error_string);

			// It could be a list of properties if we passed in a sequence of property values.
			bp::extract<bp::list> extract_property_list(property);
			if (extract_property_list.check())
			{
				properties_list.extend(extract_property_list());
			}
			else
			{
				properties_list.append(property);
			}
		}

		return properties_list;
	}

	void
	feature_handle_remove(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object property_query_object)
	{
		// See if a single property name.
		bp::extract<GPlatesModel::PropertyName> extract_property_name(property_query_object);
		if (extract_property_name.check())
		{
			const GPlatesModel::PropertyName property_name = extract_property_name();

			// Search for the property name.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				if (property_name == feature_property->get_property_name())
				{
					// Note that removing a property does not prevent us from incrementing to the next property.
					feature_handle.remove(properties_iter);
				}
			}

			return;
		}

		// See if a single property.
		bp::extract<GPlatesModel::TopLevelProperty::non_null_ptr_type> extract_property(property_query_object);
		if (extract_property.check())
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type property = extract_property();

			// Search for the property.
			// Note: This searches for the same property *instance* - it does not compare values of
			// two different property instances.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// Compare pointers not pointed-to-objects.
				if (property == feature_property)
				{
					feature_handle.remove(properties_iter);
					return;
				}
			}

			// Raise the 'ValueError' python exception if the property was not found.
			PyErr_SetString(PyExc_ValueError, "Property instance not found");
			bp::throw_error_already_set();
		}

		// See if a single predicate callable.
		if (PyObject_HasAttrString(property_query_object.ptr(), "__call__"))
		{
			// Search for the property using a predicate callable.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// See if current property matches the query.
				// Property query is a callable predicate...
				if (bp::extract<bool>(property_query_object(feature_property)))
				{
					// Note that removing a property does not prevent us from incrementing to the next property.
					feature_handle.remove(properties_iter);
				}
			}

			return;
		}

		const char *type_error_string = "Expected PropertyName, or Property, or predicate, "
				"or a sequence of any combination of them";

		// Try an iterable sequence next.
		typedef std::vector<bp::object> property_queries_seq_type;
		property_queries_seq_type property_queries_seq;
		PythonExtractUtils::extract_iterable(property_queries_seq, property_query_object, type_error_string);

		typedef std::vector<GPlatesModel::PropertyName> property_names_seq_type;
		property_names_seq_type property_names_seq;

		typedef std::vector<GPlatesModel::TopLevelProperty::non_null_ptr_type> properties_seq_type;
		properties_seq_type properties_seq;

		typedef std::vector<bp::object> predicates_seq_type;
		predicates_seq_type predicates_seq;

		// Extract the different property query types into their own arrays.
		property_queries_seq_type::const_iterator property_queries_iter = property_queries_seq.begin();
		property_queries_seq_type::const_iterator property_queries_end = property_queries_seq.end();
		for ( ; property_queries_iter != property_queries_end; ++property_queries_iter)
		{
			const bp::object property_query = *property_queries_iter;

			// See if a property name.
			bp::extract<GPlatesModel::PropertyName> extract_property_name_element(property_query);
			if (extract_property_name_element.check())
			{
				const GPlatesModel::PropertyName property_name = extract_property_name_element();
				property_names_seq.push_back(property_name);
				continue;
			}

			// See if a property.
			bp::extract<GPlatesModel::TopLevelProperty::non_null_ptr_type> extract_property_element(property_query);
			if (extract_property_element.check())
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type property = extract_property_element();
				properties_seq.push_back(property);
				continue;
			}

			// See if a predicate callable.
			if (PyObject_HasAttrString(property_query.ptr(), "__call__"))
			{
				predicates_seq.push_back(property_query);
				continue;
			}

			// Unexpected property query type so raise an error.
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}

		//
		// Process properties first to avoid unnecessarily throwing ValueError exception.
		//

		// Remove duplicate property pointers.
		properties_seq.erase(
				std::unique(properties_seq.begin(), properties_seq.end()),
				properties_seq.end());

		if (!properties_seq.empty())
		{
			// Search for the properties.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// Compare pointers not pointed-to-objects.
				properties_seq_type::iterator properties_seq_iter =
						std::find(properties_seq.begin(), properties_seq.end(), feature_property);
				if (properties_seq_iter != properties_seq.end())
				{
					// Remove the property from the feature.
					// Note that removing a property does not prevent us from incrementing to the next property.
					feature_handle.remove(properties_iter);
					// Record that we have removed this property.
					properties_seq.erase(properties_seq_iter);
				}
			}

			// Raise the 'ValueError' python exception if not all properties were found.
			if (!properties_seq.empty())
			{
				PyErr_SetString(PyExc_ValueError, "Not all property instances were found");
				bp::throw_error_already_set();
			}
		}

		//
		// Process property names next.
		//

		// Remove duplicate property names.
		property_names_seq.erase(
				std::unique(property_names_seq.begin(), property_names_seq.end()),
				property_names_seq.end());

		if (!property_names_seq.empty())
		{
			// Search for the property names.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				property_names_seq_type::iterator property_names_seq_iter = std::find(
						property_names_seq.begin(),
						property_names_seq.end(),
						feature_property->get_property_name());
				if (property_names_seq_iter != property_names_seq.end())
				{
					// Remove the property from the feature.
					// Note that removing a property does not prevent us from incrementing to the next property.
					feature_handle.remove(properties_iter);
				}
			}
		}

		//
		// Process predicate callables next.
		//

		if (!predicates_seq.empty())
		{
			// Search for matching predicate callables.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// Test each predicate callable.
				predicates_seq_type::const_iterator predicates_seq_iter = predicates_seq.begin();
				predicates_seq_type::const_iterator predicates_seq_end = predicates_seq.end();
				for ( ; predicates_seq_iter != predicates_seq_end; ++predicates_seq_iter)
				{
					bp::object predicate = *predicates_seq_iter;

					// See if current property matches the query.
					// Property query is a callable predicate...
					if (bp::extract<bool>(predicate(feature_property)))
					{
						// Note that removing a property does not prevent us from incrementing to the next property.
						feature_handle.remove(properties_iter);
						break;
					}
				}
			}
		}
	}

	bp::object
	feature_handle_set_property(
			GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			bp::object property_value_object,
			VerifyInformationModel::Value verify_information_model)
	{
		const char *type_error_string = "Expected a PropertyValue, or sequence of PropertyValue";

		// 'property_value_object' is either a property value or a sequence of property values.
		bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type> extract_property_value(property_value_object);
		if (extract_property_value.check())
		{
			GPlatesModel::PropertyValue::non_null_ptr_type property_value = extract_property_value();

			if (verify_information_model == VerifyInformationModel::NO)
			{
				// Just create a top-level property without checking information model.
				GPlatesModel::TopLevelProperty::non_null_ptr_type property =
						GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);

				// Search for an existing property with the same name.
				GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
				GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
				for ( ; properties_iter != properties_end; ++properties_iter)
				{
					GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

					if (property_name == feature_property->get_property_name())
					{
						// Change the property.
						feature_handle.set(properties_iter, property);

						// Remove any remaining properties with same name.
						for (++properties_iter ; properties_iter != properties_end; ++properties_iter)
						{
							if (property_name == (*properties_iter)->get_property_name())
							{
								feature_handle.remove(properties_iter);
							}
						}

						// Return the property.
						return bp::object(property);
					}
				}

				// Existing property with same name not found so just add property.
				GPlatesModel::FeatureHandle::iterator feature_property_iter = feature_handle.add(property);

				// Return the newly added property.
				return bp::object(*feature_property_iter);
			}

			// Only add property if valid property name for the feature's type.
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type set_property_error_code;
			boost::optional<GPlatesModel::FeatureHandle::iterator> feature_property_iter =
					GPlatesModel::ModelUtils::set_property(
							feature_handle.reference(),
							property_name,
							property_value,
							true/*check_property_name_allowed_for_feature_type*/,
							true/*check_property_value_type*/,
							&set_property_error_code);
			if (!feature_property_iter)
			{
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString(GPlatesModel::ModelUtils::get_error_message(set_property_error_code)));
			}

			// Return the newly added property.
			return bp::object(*feature_property_iter.get());
		}
		// ...else a sequence of property values.

		// Attempt to extract a sequence of property values.
		typedef std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> property_value_seq_type;
		property_value_seq_type property_values;
		PythonExtractUtils::extract_iterable(property_values, property_value_object, type_error_string);

		if (verify_information_model == VerifyInformationModel::NO)
		{
			bp::list properties;

			property_value_seq_type::const_iterator property_value_seq_iter = property_values.begin();
			property_value_seq_type::const_iterator property_value_seq_end = property_values.end();

			// Search for an existing property with the same name.
			// We will override existing properties with new property values where possible.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				if (property_name == feature_property->get_property_name())
				{
					// If we have a property value to set...
					if (property_value_seq_iter != property_value_seq_end)
					{
						// Get the next property value to set.
						GPlatesModel::PropertyValue::non_null_ptr_type property_value = *property_value_seq_iter;
						++property_value_seq_iter;

						// Just create a top-level property without checking information model.
						GPlatesModel::TopLevelProperty::non_null_ptr_type property =
								GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);

						// Change the property.
						feature_handle.set(properties_iter, property);

						properties.append(property);
					}
					else
					{
						// Remove remaining properties with same name.
						feature_handle.remove(properties_iter);
					}
				}
			}

			// If there are any remaining properties then just add them.
			for ( ; property_value_seq_iter != property_value_seq_end; ++property_value_seq_iter)
			{
				// Get the next property value to set.
				GPlatesModel::PropertyValue::non_null_ptr_type property_value = *property_value_seq_iter;

				// Just create a top-level property without checking information model.
				GPlatesModel::TopLevelProperty::non_null_ptr_type property =
						GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);

				GPlatesModel::FeatureHandle::iterator feature_property_iter = feature_handle.add(property);

				properties.append(*feature_property_iter);
			}

			// Return the property list.
			return properties;
		}

		// Only add properties if valid property name for the feature's type.
		GPlatesModel::ModelUtils::TopLevelPropertyError::Type set_property_error_code;
		std::vector<GPlatesModel::FeatureHandle::iterator> feature_properties;
		if (!GPlatesModel::ModelUtils::set_properties(
				feature_properties,
				feature_handle.reference(),
				property_name,
				std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>(
						property_values.begin(),
						property_values.end()),
				true/*check_property_name_allowed_for_feature_type*/,
				true/*check_property_multiplicity*/,
				true/*check_property_value_type*/,
				&set_property_error_code))
		{
			throw InformationModelException(
					GPLATES_EXCEPTION_SOURCE,
					QString(GPlatesModel::ModelUtils::get_error_message(set_property_error_code)));
		}

		bp::list properties;

		BOOST_FOREACH(GPlatesModel::FeatureHandle::iterator feature_property_iter, feature_properties)
		{
			properties.append(*feature_property_iter);
		}

		// Return the property list.
		return properties;
	}

	bp::object
	feature_handle_get_property(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object property_query_object,
			PropertyReturn::Value property_return)
	{
		// See if property query is a property name.
		boost::optional<GPlatesModel::PropertyName> property_name;
		bp::extract<GPlatesModel::PropertyName> extract_property_name(property_query_object);
		if (extract_property_name.check())
		{
			property_name = extract_property_name();
		}

		if (property_return == PropertyReturn::EXACTLY_ONE)
		{
			boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> property;

			// Search for the property.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// See if current property matches the query.
				const bool property_query_result = property_name
					? (property_name.get() == feature_property->get_property_name())
					// Property query is a callable predicate...
					: bp::extract<bool>(property_query_object(feature_property));

				if (property_query_result)
				{
					if (property)
					{
						// Found two properties matching same query but client expecting only one.
						return bp::object()/*Py_None*/;
					}

					property = feature_property;
				}
			}

			// Return exactly one found property (if found).
			if (property)
			{
				return bp::object(property.get());
			}
		}
		else if (property_return == PropertyReturn::FIRST)
		{
			// Search for the property.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// See if current property matches the query.
				const bool property_query_result = property_name
					? (property_name.get() == feature_property->get_property_name())
					// Property query is a callable predicate...
					: bp::extract<bool>(property_query_object(feature_property));

				if (property_query_result)
				{
					// Return first found.
					return bp::object(feature_property);
				}
			}
		}
		else
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					property_return == PropertyReturn::ALL,
					GPLATES_ASSERTION_SOURCE);

			bp::list properties;

			// Search for the properties.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// See if current property matches the query.
				const bool property_query_result = property_name
					? (property_name.get() == feature_property->get_property_name())
					// Property query is a callable predicate...
					: bp::extract<bool>(property_query_object(feature_property));

				if (property_query_result)
				{
					properties.append(feature_property);
				}
			}

			// Returned list could be empty if no properties matched.
			return properties;
		}

		return bp::object()/*Py_None*/;
	}

	bp::object
	feature_handle_get_property_value(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object property_query_object,
			const GPlatesPropertyValues::GeoTimeInstant &time,
			PropertyReturn::Value property_return)
	{
		bp::object properties_object =
				feature_handle_get_property(feature_handle, property_query_object, property_return);
		if (properties_object == bp::object()/*Py_None*/)
		{
			return bp::object()/*Py_None*/;
		}

		if (property_return == PropertyReturn::ALL)
		{
			// We're expecting a list for 'PropertyReturn::ALL'.
			bp::list property_values;

			const unsigned int num_properties = bp::len(properties_object);
			for (unsigned int n = 0; n < num_properties; ++n)
			{
				// Call python since Property.get_value is implemented in python code...
				bp::object property_value = properties_object[n].attr("get_value")(time);
				// Only append to list of property values if not Py_None;
				if (property_value != bp::object())
				{
					property_values.append(property_value);
				}
			}

			// Returned list could be empty if no properties matched, or 'time' outside
			// range of time-dependent properties.
			return property_values;
		}
		else
		{
			// Call python since Property.get_value is implemented in python code...
			bp::object property_value = properties_object.attr("get_value")(time);
			// This could be Py_None...
			return property_value;
		}
	}

	bp::object
	feature_handle_set_geometry(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object geometry_object,
			boost::optional<GPlatesModel::PropertyName> property_name,
			bp::object reverse_reconstruct_object,
			VerifyInformationModel::Value verify_information_model)
	{
		// If a property name wasn't specified then determine the
		// default geometry property name via the GPGIM.
		if (!property_name)
		{
			property_name = get_default_geometry_property_name(feature_handle.feature_type());

			if (!property_name)
			{
				// This exception will get converted to python 'InformationModelError'.
				throw InformationModelException(
						GPLATES_EXCEPTION_SOURCE,
						QString("Unable to determine the default geometry property name from the feature type '") +
								convert_qualified_xml_name_to_qstring(feature_handle.feature_type()) + "'");
			}
		}
		const GPlatesModel::PropertyName &geometry_property_name = property_name.get();

		//
		// 'geometry_object' is either:
		//   1) a GeometryOnSphere, or
		//   2) a sequence of GeometryOnSphere's, or
		//   3) a coverage, or
		//   4) a sequence of coverages.
		//
		// ...where a 'coverage' is a (geometry-domain, geometry-range) sequence (eg, 2-tuple)
		// and 'geometry-domain' is GeometryOnSphere and 'geometry-range' is a 'dict', or a sequence,
		// of (scalar type, sequence of scalar values) 2-tuples.
		//

		const char *type_error_string = "Expected a GeometryOnSphere, or a sequence of GeometryOnSphere, "
				"or a coverage, or a sequence of coverages - where a coverage is a "
				"(GeometryOnSphere, scalar-values-dictionary) tuple and a scalar-values-dictionary is "
				"a 'dict' or a sequence of (scalar type, sequence of scalar values) tuples";

		bp::extract<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> extract_geometry(geometry_object);
		if (extract_geometry.check())
		{
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry = extract_geometry();

			return set_geometry(
					feature_handle,
					geometry,
					geometry_property_name,
					reverse_reconstruct_object,
					verify_information_model);
		}

		// Attempt to extract a sequence of objects.
		// All the following are sequences - including the tuple in (3)...
		//
		//   2) a sequence of GeometryOnSphere's, or
		//   3) a (GeometryOnSphere, coverage-range) tuple, or
		//   4) a sequence of (GeometryOnSphere, coverage-range) tuples.
		//
		std::vector<bp::object> sequence_of_objects;
		PythonExtractUtils::extract_iterable(sequence_of_objects, geometry_object, type_error_string);

		// It's possible we were given an empty sequence - which means we should remove all
		// matching geometries (domains) and coverage ranges.
		if (sequence_of_objects.empty())
		{
			// Remove any geometry properties with the geometry property name.
			feature_handle.remove_properties_by_name(geometry_property_name);

			boost::optional<GPlatesModel::PropertyName> coverage_range_property_name =
					GPlatesAppLogic::ScalarCoverageFeatureProperties::get_range_property_name_from_domain(
							geometry_property_name);
			if (coverage_range_property_name)
			{
				// Remove any coverage range properties associated with the geometry property name (if any).
				feature_handle.remove_properties_by_name(coverage_range_property_name.get());
			}

			// Return an empty list since we didn't set any properties - only (potentially) removed some.
			return bp::list();
		}

		// If the first object in the sequence is a geometry then we've narrowed things down to:
		//   2) a sequence of GeometryOnSphere's, or
		//   3) a (GeometryOnSphere, coverage-range) tuple.
		//
		// Ie, we've ruled out:
		//   4) a sequence of (GeometryOnSphere, coverage-range) tuples.
		//
		// ...because it's first object is a tuple (not a geometry).
		bp::extract<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
				extract_first_geometry(sequence_of_objects[0]);
		if (extract_first_geometry.check())
		{
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_geometry = extract_first_geometry();

			// If there's exactly two objects then we *could* be looking at a (GeometryOnSphere, coverage-range) tuple.
			// Otherwise it has to be a sequence of GeometryOnSphere's.
			if (sequence_of_objects.size() == 2)
			{
				// See if the second object is also a geometry.
				bp::extract<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
						extract_second_geometry(sequence_of_objects[1]);
				if (!extract_second_geometry.check())
				{
					// If we get here then we've narrowed things down to:
					//   3) a (GeometryOnSphere, coverage-range) tuple.
					//

					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type coverage_domain_geometry = first_geometry;

					// Extract the coverage range.
					GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type gml_data_block =
							create_gml_data_block(sequence_of_objects[1], type_error_string);

					return set_geometry(
							feature_handle,
							coverage_domain_geometry,
							geometry_property_name,
							reverse_reconstruct_object,
							verify_information_model,
							gml_data_block);
				}
				// else second object is a geometry so we must have a sequence of geometries.
			}

			// If we get here then we've narrowed things down to:
			//   2) a sequence of GeometryOnSphere's.
			//

			std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometries;

			// We've already extracted the first geometry.
			geometries.push_back(first_geometry);

			// Extract the remaining geometries.
			for (unsigned int n = 1; n < sequence_of_objects.size(); ++n)
			{
				bp::extract<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
						extract_geometry_n(sequence_of_objects[n]);
				if (!extract_geometry_n.check())
				{
					PyErr_SetString(PyExc_TypeError, type_error_string);
					bp::throw_error_already_set();
				}
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_n = extract_geometry_n();

				geometries.push_back(geometry_n);
			}

			return set_geometries(
					feature_handle,
					geometries,
					geometry_property_name,
					reverse_reconstruct_object,
					verify_information_model);
		}

		// If we get here then we've narrowed things down to:
		//   4) a sequence of (GeometryOnSphere, coverage-range) tuples.
		//

		std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> coverage_domains;
		std::vector<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type> coverage_ranges;

		// Extract the sequence of coverages (domains/ranges).
		std::vector<bp::object>::const_iterator sequence_of_objects_iter = sequence_of_objects.begin();
		std::vector<bp::object>::const_iterator sequence_of_objects_end = sequence_of_objects.end();
		for ( ; sequence_of_objects_iter != sequence_of_objects_end; ++sequence_of_objects_iter)
		{
			const bp::object &coverage_object = *sequence_of_objects_iter;

			// Extract the domain/range tuple.
			std::vector<bp::object> coverage_domain_range;
			PythonExtractUtils::extract_iterable(coverage_domain_range, coverage_object, type_error_string);

			if (coverage_domain_range.size() != 2)
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}

			// Extract the coverage domain.
			bp::extract<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
					extract_coverage_domain(coverage_domain_range[0]);
			if (!extract_coverage_domain.check())
			{
				PyErr_SetString(PyExc_TypeError, type_error_string);
				bp::throw_error_already_set();
			}
			coverage_domains.push_back(extract_coverage_domain());

			// Extract the coverage range.
			GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type coverage_range =
					create_gml_data_block(coverage_domain_range[1], type_error_string);
			coverage_ranges.push_back(coverage_range);
		}

		return set_geometries(
				feature_handle,
				coverage_domains,
				geometry_property_name,
				reverse_reconstruct_object,
				verify_information_model,
				coverage_ranges);
	}

	bp::object
	feature_handle_get_geometry(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object property_query_object,
			PropertyReturn::Value property_return,
			GPlatesApi::CoverageReturn::Value coverage_return)
	{
		// If a property name or predicate wasn't specified then determine the
		// default geometry property name via the GPGIM.
		if (property_query_object == bp::object()/*Py_None*/)
		{
			boost::optional<GPlatesModel::PropertyName> default_geometry_property_name =
					get_default_geometry_property_name(feature_handle.feature_type());
			if (!default_geometry_property_name)
			{
				return (property_return == PropertyReturn::ALL)
						? bp::list() /*empty list*/
						: bp::object()/*Py_None*/;
			}

			property_query_object = bp::object(default_geometry_property_name.get());
		}

		// Get the geometry property(s).
		//
		// Note that we're querying all matching properties, not the number of (geometry)
		// properties requested by our caller, because the property query might match non-geometry
		// properties (which we'll later filter out the geometry properties and test the number of those).
		bp::object property_list_object =
				feature_handle_get_property(
						feature_handle,
						property_query_object,
						// Query all matching property values (ie, not what user requested)...
						PropertyReturn::ALL);

		// If caller is only interested in geometries (not coverages).
		if (coverage_return == CoverageReturn::GEOMETRY_ONLY)
		{
			std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometries;

			const unsigned int num_properties = bp::len(property_list_object);
			for (unsigned int n = 0; n < num_properties; ++n)
			{
				// Call python since Property.get_value is implemented in python code...
				bp::object property_value_object = property_list_object[n].attr("get_value")(0.0/*time*/);
				// Ignore property values that are Py_None.
				if (property_value_object == bp::object()/*Py_None*/)
				{
					continue;
				}

				// Get the current property value.
				GPlatesModel::PropertyValue::non_null_ptr_type property_value =
						bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type>(property_value_object);

				// Extract the geometry from the property value.
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry =
						GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(*property_value);
				if (!geometry)
				{
					continue;
				}

				// Optimisations - to return early.
				if (property_return == PropertyReturn::FIRST)
				{
					// Return first object immediately.
					return bp::object(geometry.get());
				}
				else if (property_return == PropertyReturn::EXACTLY_ONE)
				{
					// If we've already found one geometry (and now we'll have two) then return Py_None.
					if (geometries.size() == 1)
					{
						return bp::object()/*Py_None*/;
					}
				}

				geometries.push_back(geometry.get());
			}

			if (property_return == PropertyReturn::ALL)
			{
				bp::list geometries_list;

				BOOST_FOREACH(GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry, geometries)
				{
					geometries_list.append(geometry);
				}

				return geometries_list;
			}

			if (property_return == PropertyReturn::EXACTLY_ONE)
			{
				return (geometries.size() == 1) ? bp::object(geometries.front()) : bp::object()/*Py_None*/;
			}

			// ...else PropertyReturn::FIRST
			return !geometries.empty() ? bp::object(geometries.front()) : bp::object()/*Py_None*/;
		}

		//
		// Coverages (geometry domain + scalar values range).
		//

		// Get all coverages for the feature.
		typedef std::vector<GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage> coverage_seq_type;
		coverage_seq_type all_coverages;
		GPlatesAppLogic::ScalarCoverageFeatureProperties::get_coverages(
				all_coverages,
				feature_handle.reference(),
				0.0/*reconstruction_time*/);

		// The coverages with domains that match 'property_query_object'.
		coverage_seq_type coverages;

		const unsigned int num_properties = bp::len(property_list_object);
		for (unsigned int n = 0; n < num_properties; ++n)
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type property =
					bp::extract<GPlatesModel::TopLevelProperty::non_null_ptr_type>(
							property_list_object[n]);

			// Iterate over all coverages to see if the current property is a coverage 'domain'.
			coverage_seq_type::const_iterator coverage_iter = all_coverages.begin();
			const coverage_seq_type::const_iterator coverage_end = all_coverages.end();
			for ( ; coverage_iter != coverage_end; ++coverage_iter)
			{
				if (property == *coverage_iter->domain_property)
				{
					break;
				}
			}

			// Skip current property if it's not the domain of a coverage.
			if (coverage_iter == coverage_end)
			{
				continue;
			}
			const GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage &coverage = *coverage_iter;

			// Optimisations - to return early.
			if (property_return == PropertyReturn::FIRST)
			{
				// Return first coverage (domain, range) object immediately.
				return bp::make_tuple(
						bp::object(coverage.domain),
						create_dict_from_gml_data_block_coordinate_lists(
								coverage.range.begin(),
								coverage.range.end()));
			}
			else if (property_return == PropertyReturn::EXACTLY_ONE)
			{
				// If we've already found one coverage (and now we'll have two) then return Py_None.
				if (coverages.size() == 1)
				{
					return bp::object()/*Py_None*/;
				}
			}

			coverages.push_back(coverage);
		}

		if (property_return == PropertyReturn::ALL)
		{
			bp::list coverages_list;

			BOOST_FOREACH(
					const GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage &coverage,
					coverages)
			{
				const bp::object coverage_object =
						bp::make_tuple(
								bp::object(coverage.domain),
								create_dict_from_gml_data_block_coordinate_lists(
										coverage.range.begin(),
										coverage.range.end()));

				coverages_list.append(coverage_object);
			}

			return coverages_list;
		}

		if (property_return == PropertyReturn::EXACTLY_ONE)
		{
			if (coverages.size() != 1)
			{
				return bp::object()/*Py_None*/;
			}

			// Return coverage (domain, range) object.
			const GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage &coverage = coverages.front();
			return bp::make_tuple(
					bp::object(coverage.domain),
					create_dict_from_gml_data_block_coordinate_lists(
							coverage.range.begin(),
							coverage.range.end()));
		}

		// ...else PropertyReturn::FIRST

		if (coverages.empty())
		{
			return bp::object()/*Py_None*/;
		}

		// Return coverage (domain, range) object.
		const GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage &coverage = coverages.front();
		return bp::make_tuple(
				bp::object(coverage.domain),
				create_dict_from_gml_data_block_coordinate_lists(
						coverage.range.begin(),
						coverage.range.end()));
	}

	bp::object
	feature_handle_get_geometries(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object property_query_object,
			GPlatesApi::CoverageReturn::Value coverage_return)
	{
		// The returned object will be a list.
		return feature_handle_get_geometry(
				feature_handle,
				property_query_object,
				PropertyReturn::ALL,
				coverage_return);
	}

	bp::list
	feature_handle_get_all_geometries(
			GPlatesModel::FeatureHandle &feature_handle,
			GPlatesApi::CoverageReturn::Value coverage_return)
	{
		if (coverage_return == CoverageReturn::GEOMETRY_ONLY)
		{
			bp::list geometry_properties;

			// Search for the geometry properties.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// Extract the geometry from the property value.
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry =
						GPlatesAppLogic::GeometryUtils::get_geometry_from_property(feature_property);
				if (geometry)
				{
					geometry_properties.append(geometry.get());
				}
			}

			// Returned list could be empty if there were no geometry properties for some reason.
			return geometry_properties;
		}

		//
		// Coverages (geometry domain + scalar values range).
		//

		// Get all coverages for the feature.
		std::vector<GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage> all_coverages;
		GPlatesAppLogic::ScalarCoverageFeatureProperties::get_coverages(
				all_coverages,
				feature_handle.reference(),
				0.0/*reconstruction_time*/);

		bp::list coverages_list;

		BOOST_FOREACH(
				const GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage &coverage,
				all_coverages)
		{
			const bp::object coverage_object =
					bp::make_tuple(
							bp::object(coverage.domain),
							create_dict_from_gml_data_block_coordinate_lists(
									coverage.range.begin(),
									coverage.range.end()));

			coverages_list.append(coverage_object);
		}

		return coverages_list;
	}

	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create_total_reconstruction_sequence(
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type total_reconstruction_pole,
			bp::object name,
			boost::optional<QString> description,
			bp::object other_properties,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			VerifyInformationModel::Value verify_information_model)
	{
		static const GPlatesModel::FeatureType TOTAL_RECONSTRUCTION_SEQUENCE_FEATURE_TYPE =
				GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence");

		GPlatesModel::FeatureHandle::non_null_ptr_type feature = feature_handle_create(
				TOTAL_RECONSTRUCTION_SEQUENCE_FEATURE_TYPE, feature_id, verify_information_model);
		bp::object feature_object(feature);

		if (name != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_name is implemented in python code...
			feature_object.attr("set_name")(name, verify_information_model);
		}

		if (description)
		{
			// Call python since Feature.set_description is implemented in python code...
			feature_object.attr("set_description")(description.get(), verify_information_model);
		}

		// Call python since Feature.set_total_reconstruction_pole is implemented in python code...
		feature_object.attr("set_total_reconstruction_pole")(
				fixed_plate_id,
				moving_plate_id,
				total_reconstruction_pole,
				verify_information_model);

		// If there are other properties then add them.
		if (other_properties != bp::object()/*Py_None*/)
		{
			feature_handle_add_properties(*feature, other_properties, verify_information_model);
		}

		return feature;
	}

	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create_reconstructable_feature(
			const GPlatesModel::FeatureType &feature_type,
			bp::object geometry,
			bp::object name,
			boost::optional<QString> description,
			bp::object valid_time,
			boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
			bp::object conjugate_plate_id,
			bp::object other_properties,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			bp::object reverse_reconstruct_object,
			VerifyInformationModel::Value verify_information_model)
	{
		GPlatesModel::FeatureHandle::non_null_ptr_type feature =
				feature_handle_create(feature_type, feature_id, verify_information_model);
		bp::object feature_object(feature);

		// Make sure 'feature_type' inherits directly or indirectly from 'gpml:ReconstructableFeature'.
		if (verify_information_model == VerifyInformationModel::YES)
		{
			static const GPlatesModel::FeatureType RECONSTRUCTABLE_FEATURE_TYPE =
					GPlatesModel::FeatureType::create_gpml("ReconstructableFeature");

			verify_feature_type_inherits(feature_type, RECONSTRUCTABLE_FEATURE_TYPE);
		}

		if (name != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_name is implemented in python code...
			feature_object.attr("set_name")(name, verify_information_model);
		}

		if (description)
		{
			// Call python since Feature.set_description is implemented in python code...
			feature_object.attr("set_description")(description.get(), verify_information_model);
		}

		if (valid_time != bp::object()/*Py_None*/)
		{
			set_valid_time_from_tuple(feature_object, valid_time, verify_information_model);
		}

		if (reconstruction_plate_id)
		{
			// Call python since Feature.set_reconstruction_plate_id is implemented in python code...
			feature_object.attr("set_reconstruction_plate_id")(reconstruction_plate_id.get(), verify_information_model);
		}

		if (conjugate_plate_id != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_conjugate_plate_id is implemented in python code...
			feature_object.attr("set_conjugate_plate_id")(conjugate_plate_id, verify_information_model);
		}

		// If there are other properties then add them.
		if (other_properties != bp::object()/*Py_None*/)
		{
			feature_handle_add_properties(*feature, other_properties, verify_information_model);
		}

		// Set the geometry (or geometries).
		// NOTE: We *must* set the geometry after all other properties have been set since
		// reverse reconstructing uses those properties.
		feature_handle_set_geometry(
				*feature, geometry, boost::none, reverse_reconstruct_object, verify_information_model);

		return feature;
	}

	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create_tectonic_section(
			const GPlatesModel::FeatureType &feature_type,
			bp::object geometry,
			bp::object name,
			boost::optional<QString> description,
			bp::object valid_time,
			boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
			bp::object conjugate_plate_id,
			boost::optional<GPlatesModel::integer_plate_id_type> left_plate,
			boost::optional<GPlatesModel::integer_plate_id_type> right_plate,
			boost::optional<GPlatesUtils::UnicodeString> reconstruction_method,
			bp::object other_properties,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			bp::object reverse_reconstruct_object,
			VerifyInformationModel::Value verify_information_model)
	{
		GPlatesModel::FeatureHandle::non_null_ptr_type feature =
				feature_handle_create(feature_type, feature_id, verify_information_model);
		bp::object feature_object(feature);

		// Make sure 'feature_type' inherits directly or indirectly from 'gpml:TectonicSection'.
		if (verify_information_model == VerifyInformationModel::YES)
		{
			static const GPlatesModel::FeatureType TECTONIC_SECTION_FEATURE_TYPE =
					GPlatesModel::FeatureType::create_gpml("TectonicSection");

			verify_feature_type_inherits(feature_type, TECTONIC_SECTION_FEATURE_TYPE);
		}

		if (name != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_name is implemented in python code...
			feature_object.attr("set_name")(name, verify_information_model);
		}

		if (description)
		{
			// Call python since Feature.set_description is implemented in python code...
			feature_object.attr("set_description")(description.get(), verify_information_model);
		}

		if (valid_time != bp::object()/*Py_None*/)
		{
			set_valid_time_from_tuple(feature_object, valid_time, verify_information_model);
		}

		if (reconstruction_plate_id)
		{
			// Call python since Feature.set_reconstruction_plate_id is implemented in python code...
			feature_object.attr("set_reconstruction_plate_id")(reconstruction_plate_id.get(), verify_information_model);
		}

		if (conjugate_plate_id != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_conjugate_plate_id is implemented in python code...
			feature_object.attr("set_conjugate_plate_id")(conjugate_plate_id, verify_information_model);
		}

		if (left_plate)
		{
			// Call python since Feature.set_left_plate is implemented in python code...
			feature_object.attr("set_left_plate")(left_plate.get(), verify_information_model);
		}

		if (right_plate)
		{
			// Call python since Feature.set_right_plate is implemented in python code...
			feature_object.attr("set_right_plate")(right_plate.get(), verify_information_model);
		}

		if (reconstruction_method)
		{
			static const GPlatesModel::PropertyName RECONSTRUCTION_METHOD_PROPERTY_NAME =
					GPlatesModel::PropertyName::create_gpml("reconstructionMethod");
			static const GPlatesPropertyValues::EnumerationType RECONSTRUCTION_METHOD_ENUMERATION_TYPE =
					GPlatesPropertyValues::EnumerationType::create_gpml("ReconstructionMethodEnumeration");

			if (verify_information_model == VerifyInformationModel::YES)
			{
				verify_enumeration_type_and_content(
						RECONSTRUCTION_METHOD_ENUMERATION_TYPE,
						reconstruction_method->qstring());
			}

			feature_handle_add_property(
					*feature,
					RECONSTRUCTION_METHOD_PROPERTY_NAME,
					bp::object(
							GPlatesPropertyValues::Enumeration::create(
									RECONSTRUCTION_METHOD_ENUMERATION_TYPE,
									reconstruction_method.get())),
					verify_information_model);
		}

		// If there are other properties then add them.
		if (other_properties != bp::object()/*Py_None*/)
		{
			feature_handle_add_properties(*feature, other_properties, verify_information_model);
		}

		// Set the geometry (or geometries).
		// NOTE: We *must* set the geometry after all other properties have been set since
		// reverse reconstructing uses those properties.
		feature_handle_set_geometry(
				*feature, geometry, boost::none, reverse_reconstruct_object, verify_information_model);

		return feature;
	}

	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create_flowline(
			bp::object seed_geometry,
			bp::object times,
			bp::object name,
			boost::optional<QString> description,
			bp::object valid_time,
			boost::optional<GPlatesModel::integer_plate_id_type> left_plate,
			boost::optional<GPlatesModel::integer_plate_id_type> right_plate,
			bp::object other_properties,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			bp::object reverse_reconstruct_object,
			VerifyInformationModel::Value verify_information_model)
	{
		static const GPlatesModel::FeatureType FLOWLINE_FEATURE_TYPE =
				GPlatesModel::FeatureType::create_gpml("Flowline");

		GPlatesModel::FeatureHandle::non_null_ptr_type feature = feature_handle_create(
				FLOWLINE_FEATURE_TYPE, feature_id, verify_information_model);
		bp::object feature_object(feature);

		// Set the times.
		// Call python since Feature.set_times is implemented in python code...
		feature_object.attr("set_times")(times, verify_information_model);

		// Set the reconstruction method to half-stage rotation.
		// Call python since Feature.set_reconstruction_method is implemented in python code...
		feature_object.attr("set_reconstruction_method")("HalfStageRotationVersion2", verify_information_model);

		if (name != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_name is implemented in python code...
			feature_object.attr("set_name")(name, verify_information_model);
		}

		if (description)
		{
			// Call python since Feature.set_description is implemented in python code...
			feature_object.attr("set_description")(description.get(), verify_information_model);
		}

		if (valid_time != bp::object()/*Py_None*/)
		{
			set_valid_time_from_tuple(feature_object, valid_time, verify_information_model);
		}

		if (left_plate)
		{
			// Call python since Feature.set_left_plate is implemented in python code...
			feature_object.attr("set_left_plate")(left_plate.get(), verify_information_model);
		}

		if (right_plate)
		{
			// Call python since Feature.set_right_plate is implemented in python code...
			feature_object.attr("set_right_plate")(right_plate.get(), verify_information_model);
		}

		// If there are other properties then add them.
		if (other_properties != bp::object()/*Py_None*/)
		{
			feature_handle_add_properties(*feature, other_properties, verify_information_model);
		}

		// Set the seed geometry.
		// NOTE: We *must* set the geometry after all other properties have been set since
		// reverse reconstructing uses those properties.
		feature_handle_set_geometry(
				*feature, seed_geometry, boost::none, reverse_reconstruct_object, verify_information_model);

		return feature;
	}

	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create_motion_path(
			bp::object seed_geometry,
			bp::object times,
			bp::object name,
			boost::optional<QString> description,
			bp::object valid_time,
			boost::optional<GPlatesModel::integer_plate_id_type> relative_plate,
			boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
			bp::object other_properties,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			bp::object reverse_reconstruct_object,
			VerifyInformationModel::Value verify_information_model)
	{
		static const GPlatesModel::FeatureType MOTIONPATH_FEATURE_TYPE =
				GPlatesModel::FeatureType::create_gpml("MotionPath");

		GPlatesModel::FeatureHandle::non_null_ptr_type feature = feature_handle_create(
				MOTIONPATH_FEATURE_TYPE, feature_id, verify_information_model);
		bp::object feature_object(feature);

		// Set the times.
		// Call python since Feature.set_times is implemented in python code...
		feature_object.attr("set_times")(times, verify_information_model);

		// Set the reconstruction method to by-plate-id.
		// Call python since Feature.set_reconstruction_method is implemented in python code...
		feature_object.attr("set_reconstruction_method")("ByPlateId", verify_information_model);

		if (name != bp::object()/*Py_None*/)
		{
			// Call python since Feature.set_name is implemented in python code...
			feature_object.attr("set_name")(name, verify_information_model);
		}

		if (description)
		{
			// Call python since Feature.set_description is implemented in python code...
			feature_object.attr("set_description")(description.get(), verify_information_model);
		}

		if (valid_time != bp::object()/*Py_None*/)
		{
			set_valid_time_from_tuple(feature_object, valid_time, verify_information_model);
		}

		if (relative_plate)
		{
			// Call python since Feature.set_relative_plate is implemented in python code...
			feature_object.attr("set_relative_plate")(relative_plate.get(), verify_information_model);
		}

		if (reconstruction_plate_id)
		{
			// Call python since Feature.set_reconstruction_plate_id is implemented in python code...
			feature_object.attr("set_reconstruction_plate_id")(reconstruction_plate_id.get(), verify_information_model);
		}

		// If there are other properties then add them.
		if (other_properties != bp::object()/*Py_None*/)
		{
			feature_handle_add_properties(*feature, other_properties, verify_information_model);
		}

		// Set the seed geometry.
		// NOTE: We *must* set the geometry after all other properties have been set since
		// reverse reconstructing uses those properties.
		feature_handle_set_geometry(
				*feature, seed_geometry, boost::none, reverse_reconstruct_object, verify_information_model);

		return feature;
	}
}


void
GPlatesApi::AmbiguousGeometryCoverageException::write_message(
		std::ostream &os) const
{
	os << "more than one coverage *geometry* named '"
		<< convert_qualified_xml_name_to_qstring(d_domain_property_name).toStdString()
		<< "' with same number of points (or same number of scalar values).";
}


void
export_feature()
{
	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesApi::PropertyReturn::Value>("PropertyReturn")
			.value("exactly_one", GPlatesApi::PropertyReturn::EXACTLY_ONE)
			.value("first", GPlatesApi::PropertyReturn::FIRST)
			.value("all", GPlatesApi::PropertyReturn::ALL);

	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesApi::CoverageReturn::Value>("CoverageReturn")
			.value("geometry_only", GPlatesApi::CoverageReturn::GEOMETRY_ONLY)
			.value("geometry_and_scalars", GPlatesApi::CoverageReturn::GEOMETRY_AND_SCALARS);

	//
	// Feature - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesModel::FeatureHandle,
			GPlatesModel::FeatureHandle::non_null_ptr_type,
			boost::noncopyable>(
					"Feature",
					"The feature is an abstract model of some geological or plate-tectonic object or "
					"concept of interest. It consists of a collection of :class:`properties<Property>`, "
					"a :class:`feature type<FeatureType>` and a :class:`feature id<FeatureId>`.\n"
					"\n"
					"The following operations for iterating over the properties in a feature are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(f)``                  number of properties in feature *f*\n"
					"``for p in f``              iterates over the properties *p* in feature *f*\n"
					"=========================== ==========================================================\n"
					"\n"
					"For example:\n"
					"::\n"
					"\n"
					"  num_properties = len(feature)\n"
					"  properties_in_feature = [property for property in feature]\n"
					"  assert(num_properties == len(properties_in_feature))\n"
					"\n"
					"The following methods provide convenient ways to create :class:`features<Feature>`:\n"
					"\n"
					"* :meth:`create_reconstructable_feature`\n"
					"* :meth:`create_tectonic_section`\n"
					"* :meth:`create_flowline`\n"
					"* :meth:`create_motion_path`\n"
					"* :meth:`create_total_reconstruction_sequence`\n"
					"\n"
					"The following methods return the :class:`feature type<FeatureType>` and :class:`feature id<FeatureId>`:\n"
					"\n"
					"* :meth:`get_feature_type`\n"
					"* :meth:`get_feature_id`\n"
					"\n"
					"The following methods provide *generic* support for adding, removing, setting and getting properties:\n"
					"\n"
					"* :meth:`add`\n"
					"* :meth:`remove`\n"
					"* :meth:`set`\n"
					"* :meth:`get`\n"
					"* :meth:`get_value`\n"
					"\n"
					"The following methods provide a convenient way to set and get feature :class:`geometry<GeometryOnSphere>`:\n"
					"\n"
					"* :meth:`set_geometry`\n"
					"* :meth:`get_geometry`\n"
					"* :meth:`get_geometries`\n"
					"* :meth:`get_all_geometries`\n"
					"\n"
					"The following methods provide a convenient way to set and get attributes imported from a Shapefile:\n"
					"\n"
					"* :meth:`set_shapefile_attribute`\n"
					"* :meth:`set_shapefile_attributes`\n"
					"* :meth:`get_shapefile_attribute`\n"
					"* :meth:`get_shapefile_attributes`\n"
					"\n"
					"The following methods provide a convenient way to set and get some of the properties "
					"that are common to many feature types:\n"
					"\n"
					"* :meth:`set_name`\n"
					"* :meth:`get_name`\n"
					"* :meth:`set_description`\n"
					"* :meth:`get_description`\n"
					"* :meth:`set_valid_time`\n"
					"* :meth:`get_valid_time`\n"
					"* :meth:`set_reconstruction_plate_id`\n"
					"* :meth:`get_reconstruction_plate_id`\n"
					"* :meth:`set_conjugate_plate_id`\n"
					"* :meth:`get_conjugate_plate_id`\n"
					"* :meth:`set_left_plate`\n"
					"* :meth:`get_left_plate`\n"
					"* :meth:`set_right_plate`\n"
					"* :meth:`get_right_plate`\n"
					"* :meth:`set_relative_plate`\n"
					"* :meth:`get_relative_plate`\n"
					"* :meth:`set_times`\n"
					"* :meth:`get_times`\n"
					"* :meth:`set_reconstruction_method`\n"
					"* :meth:`get_reconstruction_method`\n"
					"* :meth:`set_total_reconstruction_pole`\n"
					"* :meth:`get_total_reconstruction_pole`\n"
					"\n"
					"For other properties the generic :meth:`set`, :meth:`get` and :meth:`get_value` "
					"methods will still need to be used.\n"
					"\n"
					"A feature can be deep copied using :meth:`clone`.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::feature_handle_create,
						bp::default_call_policies(),
						(bp::arg("feature_type") = boost::optional<GPlatesModel::FeatureType>(),
							bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
							bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES)),
				"__init__([feature_type], [feature_id], [verify_information_model=VerifyInformationModel.yes])\n"
				"  Create a new feature instance that is (initially) empty (has no properties).\n"
				"\n"
				"  :param feature_type: the type of feature\n"
				"  :type feature_type: :class:`FeatureType`\n"
				"  :param feature_id: the feature identifier\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param verify_information_model: whether to check *feature_type* with the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *feature_type* is not a recognised feature type\n"
				"\n"
				"  *feature_type* defaults to *gpml:UnclassifiedFeature* if not specified. "
				"There are no restrictions on the types and number of properties that can be added "
				"to features of type *gpml:UnclassifiedFeature* provided their property names are recognised by the "
				"GPlates Geological Information Model (GPGIM) (see http://www.gplates.org/gpml.html). "
				"However all other feature types are restricted to a subset of recognised properties. "
				"The restriction is apparent when the features are created explicitly (see :meth:`add`) "
				"and when features are *read* from a GPML format file (there are no restrictions "
				"when the features are *written* to a GPML format file).\n"
				"\n"
				"  If *feature_id* is not specified then a unique feature identifier is created. In most cases "
				"a specific *feature_id* should not be specified because it avoids the possibility of "
				"accidentally having two feature instances with the same identifier which can cause "
				"problems with *topological* geometries.\n"
				"  ::\n"
				"\n"
				"    unclassified_feature = pygplates.Feature()\n"
				"\n"
				"    # This does the same thing as the code above.\n"
				"    unclassified_feature = pygplates.Feature(\n"
				"        pygplates.FeatureType.create_gpml('UnclassifiedFeature'))\n")
		.def("__iter__", bp::iterator<GPlatesModel::FeatureHandle>())
		.def("__len__", &GPlatesModel::FeatureHandle::size)
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())

		.def("clone",
				&GPlatesApi::feature_handle_clone,
				"clone()\n"
				"  Create a duplicate of this feature instance.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  This creates a new :class:`Feature` instance with cloned versions of this feature's properties. "
				"And the cloned feature is created with its own unique :class:`FeatureId`.\n")
		.def("create_total_reconstruction_sequence",
				&GPlatesApi::feature_handle_create_total_reconstruction_sequence,
				(bp::arg("fixed_plate_id"),
						bp::arg("moving_plate_id"),
						bp::arg("total_reconstruction_pole"),
						bp::arg("name") = bp::object()/*Py_None*/,
						bp::arg("description") = boost::optional<QString>(),
						bp::arg("other_properties") = bp::object()/*Py_None*/,
						bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"create_total_reconstruction_sequence(fixed_plate_id, moving_plate_id, total_reconstruction_pole, "
				"[name], [description], [other_properties], [feature_id], "
				"[verify_information_model=VerifyInformationModel.yes])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a rotation feature for a fixed/moving plate pair.\n"
				"\n"
				"  :param fixed_plate_id: the fixed plate id\n"
				"  :type fixed_plate_id: int\n"
				"  :param moving_plate_id: the moving plate id\n"
				"  :type moving_plate_id: int\n"
				"  :param total_reconstruction_pole: the time-sequence of rotations\n"
				"  :type total_reconstruction_pole: :class:`GpmlIrregularSampling` of :class:`GpmlFiniteRotation`\n"
				"  :param name: the name or names, if not specified then no 'gml:name' properties are added\n"
				"  :type name: string, or sequence of string\n"
				"  :param description: the description, if not specified then a 'gml:description' property is not added\n"
				"  :type description: string\n"
				"  :param other_properties: any extra property name/value pairs to add, these can alternatively "
				"be added later with :meth:`add`\n"
				"  :type other_properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, "
				":class:`PropertyValue` or sequence of :class:`PropertyValue`)\n"
				"  :param feature_id: the feature identifier, if not specified then a unique feature identifier is created\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param verify_information_model: whether to check the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  This function creates a rotation feature containing a "
				":meth:`total reconstruction pole<get_total_reconstruction_pole>` (a time sequence of "
				":class:`finite rotations<GpmlFiniteRotation>`) for a fixed/moving plate pair. "
				"The :class:`feature type<FeatureType>` is a `total reconstruction sequence "
				"<http://www.earthbyte.org/Resources/GPGIM/public/#TotalReconstructionSequence>`_.\n"
				"\n"
				"  This function calls :meth:`set_name`, :meth:`set_description`, "
				":meth:`set_total_reconstruction_pole` and :meth:`add`.\n"
				"\n"
				"  Create a rotation feature:\n"
				"  ::\n"
				"\n"
				"    rotation_feature = pygplates.Feature.create_total_reconstruction_sequence(\n"
				"        550,\n"
				"        801,\n"
				"        total_reconstruction_pole_801_rel_550,\n"
				"        name='INA-AUS Muller et.al 2000')\n"
				"\n"
				"  The previous example is the equivalent of the following:\n"
				"  ::\n"
				"\n"
				"    rotation_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('TotalReconstructionSequence'))\n"
				"    rotation_feature.set_name('INA-AUS Muller et.al 2000')\n"
				"    rotation_feature.set_total_reconstruction_pole(550, 801, total_reconstruction_pole_801_rel_550)\n")
		.staticmethod("create_total_reconstruction_sequence")
		.def("create_reconstructable_feature",
				&GPlatesApi::feature_handle_create_reconstructable_feature,
				(bp::arg("feature_type"),
						bp::arg("geometry"),
						bp::arg("name") = bp::object()/*Py_None*/,
						bp::arg("description") = boost::optional<QString>(),
						bp::arg("valid_time") = bp::object()/*Py_None*/,
						bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("conjugate_plate_id") = bp::object()/*Py_None*/,
						bp::arg("other_properties") = bp::object()/*Py_None*/,
						bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
						bp::arg("reverse_reconstruct") = bp::object()/*Py_None*/,
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"create_reconstructable_feature(feature_type, geometry, [name], [description], [valid_time], "
				"[reconstruction_plate_id], [conjugate_plate_id], [other_properties], [feature_id], [reverse_reconstruct], "
				"[verify_information_model=VerifyInformationModel.yes])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a reconstructable feature.\n"
				"\n"
				"  :param feature_type: the type of feature to create\n"
				"  :type feature_type: :class:`FeatureType`\n"
				"  :param geometry: the geometry (or geometries, or a coverage or a sequence of coverages) - "
				"if geometry is not present-day geometry then the created feature will need to be reverse "
				"reconstructed to present day (using either the *reverse_reconstruct* parameter or "
				":func:`reverse_reconstruct`) before the feature can be reconstructed to an arbitrary reconstruction time\n"
				"  :type geometry: :class:`GeometryOnSphere`, or sequence (eg, ``list`` or ``tuple``) "
				"of :class:`GeometryOnSphere` (or a coverage or a sequence of coverages - :meth:`set_geometry`)\n"
				"  :param name: the name or names, if not specified then no 'gml:name' properties are added\n"
				"  :type name: string, or sequence of string\n"
				"  :param description: the description, if not specified then a 'gml:description' property is not added\n"
				"  :type description: string\n"
				"  :param valid_time: the (begin_time, end_time) tuple, if not specified then a 'gml:validTime' "
				"property is not added\n"
				"  :type valid_time: a tuple of (float or :class:`GeoTimeInstant`, float or :class:`GeoTimeInstant`)\n"
				"  :param reconstruction_plate_id: the reconstruction plate id, if not specified then a "
				"'gpml:reconstructionPlateId' property is not added\n"
				"  :type reconstruction_plate_id: int\n"
				"  :param conjugate_plate_id: the conjugate plate ID or plate IDs, if not specified then no "
				"'gpml:conjugatePlateId' properties are added - **note** that not all `reconstructable features "
				"<http://www.earthbyte.org/Resources/GPGIM/public/#ReconstructableFeature>`_ have a conjugate "
				"plate id (*conjugate_plate_id* is provided to support the "
				"`Isochron feature type <http://www.earthbyte.org/Resources/GPGIM/public/#Isochron>`_)\n"
				"  :type conjugate_plate_id: int, or sequence of int\n"
				"  :param other_properties: any extra property name/value pairs to add, these can alternatively "
				"be added later with :meth:`add`\n"
				"  :type other_properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, "
				":class:`PropertyValue` or sequence of :class:`PropertyValue`)\n"
				"  :param feature_id: the feature identifier, if not specified then a unique feature identifier is created\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param reverse_reconstruct: the tuple (rotation model, geometry reconstruction time [, anchor plate id]) "
				"where the anchor plate is optional - if this tuple of reverse reconstruct parameters is specified "
				"then *geometry* is reverse reconstructed using those parameters and any specified feature properties "
				"(eg, *reconstruction_plate_id*) - this is only required if *geometry* is not present day - "
				"alternatively you can subsequently call :func:`reverse_reconstruct`\n"
				"  :type reverse_reconstruct: tuple (:class:`RotationModel`, float or :class:`GeoTimeInstant` [, int])\n"
				"  :param verify_information_model: whether to check the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :rtype: :class:`Feature`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *feature_type* is not a `reconstructable feature "
				"<http://www.earthbyte.org/Resources/GPGIM/public/#ReconstructableFeature>`_.\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if *valid_time* has begin time later than end time\n"
				"\n"
				"  This function creates a feature of :class:`type<FeatureType>` that falls in the category "
				"of a `reconstructable feature <http://www.earthbyte.org/Resources/GPGIM/public/#ReconstructableFeature>`_ - "
				"note that there are multiple :class:`feature types<FeatureType>` that fall into this category.\n"
				"\n"
				"  This function calls :meth:`set_geometry`, :meth:`set_name`, :meth:`set_description`, "
				":meth:`set_valid_time`, :meth:`set_reconstruction_plate_id`, :meth:`set_conjugate_plate_id` "
				"and :meth:`add`.\n"
				"\n"
				"  Create a coastline feature:\n"
				"  ::\n"
				"\n"
				"    present_day_coastline_geometry = pygplates.PolylineOnSphere([...])\n"
				"    east_antarctica_coastline_feature = pygplates.Feature.create_reconstructable_feature(\n"
				"        pygplates.FeatureType.create_gpml('Coastline'),\n"
				"        present_day_coastline_geometry,\n"
				"        name='East Antarctica',\n"
				"        valid_time=(600, pygplates.GeoTimeInstant.create_distant_future()),\n"
				"        reconstruction_plate_id=802)\n"
				"\n"
				"  If *geometry* is not present-day geometry (see isochron example below) then the created "
				"feature will need to be reverse reconstructed to present day using (using either the "
				"*reverse_reconstruct* parameter or :func:`reverse_reconstruct`) before the feature can "
				"be reconstructed to an arbitrary reconstruction time - this is because a feature is not "
				"complete until its geometry is *present day* geometry.\n"
				"\n"
				"  Create an isochron feature (note that it must also be reverse reconstructed since "
				"the specified geometry is not present day geometry but instead the geometry of the "
				"mid-ocean ridge that the isochron came from at the isochron's time of appearance):\n"
				"  ::\n"
				"\n"
				"    time_of_appearance = 600\n"
				"    geometry_at_time_of_appearance = pygplates.PolylineOnSphere([...])\n"
				"    isochron_feature = pygplates.Feature.create_reconstructable_feature(\n"
				"        pygplates.FeatureType.create_gpml('Isochron'),\n"
				"        geometry_at_time_of_appearance,\n"
				"        name='SOUTH ATLANTIC, SOUTH AMERICA-AFRICA ANOMALY 13 ISOCHRON',\n"
				"        valid_time=(time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),\n"
				"        reconstruction_plate_id=201,\n"
				"        conjugate_plate_id=701,\n"
				"        reverse_reconstruct=(rotation_model, time_of_appearance))\n"
				"    \n"
				"    # ...or...\n"
				"    \n"
				"    isochron_feature = pygplates.Feature.create_reconstructable_feature(\n"
				"        pygplates.FeatureType.create_gpml('Isochron'),\n"
				"        geometry_at_time_of_appearance,\n"
				"        name='SOUTH ATLANTIC, SOUTH AMERICA-AFRICA ANOMALY 13 ISOCHRON',\n"
				"        valid_time=(time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),\n"
				"        reconstruction_plate_id=201,\n"
				"        conjugate_plate_id=701)\n"
				"    pygplates.reverse_reconstruct(isochron_feature, rotation_model, time_of_appearance)\n"
				"\n"
				"  The previous example is the equivalent of the following (note that the "
				":func:`reverse reconstruction<reverse_reconstruct>` is done *after* the properties have "
				"been set on the feature - this is necessary because reverse reconstruction looks at these "
				"properties to determine how to reverse reconstruct):\n"
				"  ::\n"
				"\n"
				"    isochron_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('Isochron'))\n"
				"    isochron_feature.set_geometry(geometry_at_time_of_appearance)\n"
				"    isochron_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA ANOMALY 13 ISOCHRON')\n"
				"    isochron_feature.set_valid_time(time_of_appearance, pygplates.GeoTimeInstant.create_distant_future())\n"
				"    isochron_feature.set_reconstruction_plate_id(201)\n"
				"    isochron_feature.set_conjugate_plate_id(701)\n"
				"    pygplates.reverse_reconstruct(isochron_feature, rotation_model, time_of_appearance)\n"
				"    \n"
				"    # ...or...\n"
				"    \n"
				"    isochron_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('Isochron'))\n"
				"    isochron_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA ANOMALY 13 ISOCHRON')\n"
				"    isochron_feature.set_valid_time(time_of_appearance, pygplates.GeoTimeInstant.create_distant_future())\n"
				"    isochron_feature.set_reconstruction_plate_id(201)\n"
				"    isochron_feature.set_conjugate_plate_id(701)\n"
				"    # Set geometry and reverse reconstruct *after* other feature properties have been set.\n"
				"    isochron_feature.set_geometry(\n"
				"        geometry_at_time_of_appearance,\n"
				"        reverse_reconstruct=(rotation_model, time_of_appearance))\n")
		.staticmethod("create_reconstructable_feature")
		.def("create_tectonic_section",
				&GPlatesApi::feature_handle_create_tectonic_section,
				(bp::arg("feature_type"),
						bp::arg("geometry"),
						bp::arg("name") = bp::object()/*Py_None*/,
						bp::arg("description") = boost::optional<QString>(),
						bp::arg("valid_time") = bp::object()/*Py_None*/,
						bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("conjugate_plate_id") = bp::object()/*Py_None*/,
						bp::arg("left_plate") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("right_plate") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("reconstruction_method") = boost::optional<GPlatesUtils::UnicodeString>(),
						bp::arg("other_properties") = bp::object()/*Py_None*/,
						bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
						bp::arg("reverse_reconstruct") = bp::object()/*Py_None*/,
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"create_tectonic_section(feature_type, geometry, [name], [description], [valid_time], "
				"[reconstruction_plate_id], [conjugate_plate_id], [left_plate], [right_plate], [reconstruction_method], "
				"[other_properties], [feature_id], [reverse_reconstruct], "
				"[verify_information_model=VerifyInformationModel.yes])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a tectonic section feature.\n"
				"\n"
				"  :param feature_type: the type of feature to create\n"
				"  :type feature_type: :class:`FeatureType`\n"
				"  :param geometry: the geometry (or geometries, or a coverage or a sequence of coverages) - "
				"if geometry is not present-day geometry then the created feature will need to be reverse "
				"reconstructed to present day (using either the *reverse_reconstruct* parameter or "
				":func:`reverse_reconstruct`) before the feature can be reconstructed to an arbitrary "
				"reconstruction time\n"
				"  :type geometry: :class:`GeometryOnSphere`, or sequence (eg, ``list`` or ``tuple``) "
				"of :class:`GeometryOnSphere` (or a coverage or a sequence of coverages - :meth:`set_geometry`)\n"
				"  :param name: the name or names, if not specified then no 'gml:name' properties are added\n"
				"  :type name: string, or sequence of string\n"
				"  :param description: the description, if not specified then a 'gml:description' property is not added\n"
				"  :type description: string\n"
				"  :param valid_time: the (begin_time, end_time) tuple, if not specified then a 'gml:validTime' "
				"property is not added\n"
				"  :type valid_time: a tuple of (float or :class:`GeoTimeInstant`, float or :class:`GeoTimeInstant`)\n"
				"  :param reconstruction_plate_id: the reconstruction plate id, if not specified then a "
				"'gpml:reconstructionPlateId' property is not added\n"
				"  :type reconstruction_plate_id: int\n"
				"  :param conjugate_plate_id: the conjugate plate ID or plate IDs, if not specified then no "
				"'gpml:conjugatePlateId' properties are added\n"
				"  :type conjugate_plate_id: int, or sequence of int\n"
				"  :param left_plate: the left plate id, if not specified then a 'gpml:leftPlate' property is not added\n"
				"  :type left_plate: int\n"
				"  :param right_plate: the right plate id, if not specified then a 'gpml:rightPlate' property is not added\n"
				"  :type right_plate: int\n"
				"  :param reconstruction_method: the reconstruction method, if not specified then a 'gpml:reconstructionMethod' "
				"property is not added (note that a missing property essentially defaults to 'ByPlateId' behaviour) "
				"- note that 'HalfStageRotationVersion2' is the latest and most accurate half-stage method and should "
				"generally be used unless backward compatibility with old GPlates versions is required\n"
				"  :type reconstruction_method: string ('ByPlateId', 'HalfStageRotation' or 'HalfStageRotationVersion2')\n"
				"  :param other_properties: any extra property name/value pairs to add, these can alternatively "
				"be added later with :meth:`add`\n"
				"  :type other_properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, "
				":class:`PropertyValue` or sequence of :class:`PropertyValue`)\n"
				"  :param feature_id: the feature identifier, if not specified then a unique feature identifier is created\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param reverse_reconstruct: the tuple (rotation model, geometry reconstruction time [, anchor plate id]) "
				"where the anchor plate is optional - if this tuple of reverse reconstruct parameters is specified "
				"then *geometry* is reverse reconstructed using those parameters and any specified feature properties "
				"(eg, *reconstruction_plate_id*) - this is only required if *geometry* is not present day - "
				"alternatively you can subsequently call :func:`reverse_reconstruct`\n"
				"  :type reverse_reconstruct: tuple (:class:`RotationModel`, float or :class:`GeoTimeInstant` [, int])\n"
				"  :param verify_information_model: whether to check the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :rtype: :class:`Feature`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *feature_type* is not a `tectonic section "
				"<http://www.earthbyte.org/Resources/GPGIM/public/#TectonicSection>`_.\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if *valid_time* has begin time later than end time\n"
				"\n"
				"  This function creates a feature of :class:`type<FeatureType>` that falls in the category "
				"of a `tectonic section <http://www.earthbyte.org/Resources/GPGIM/public/#TectonicSection>`_ - "
				"note that there are multiple :class:`feature types<FeatureType>` that fall into this category.\n"
				"\n"
				"  This function calls :meth:`set_geometry`, :meth:`set_name`, :meth:`set_description`, "
				":meth:`set_valid_time`, :meth:`set_reconstruction_plate_id`, :meth:`set_conjugate_plate_id`, "
				":meth:`set_left_plate`, :meth:`set_right_plate` and :meth:`add`.\n"
				"\n"
				"  If *geometry* is not present-day geometry then the created feature will need to be "
				"reverse reconstructed to present day using (using either the *reverse_reconstruct* "
				"parameter or :func:`reverse_reconstruct`) before the feature can be reconstructed to an "
				"arbitrary reconstruction time - this is because a feature is not complete until its "
				"geometry is *present day* geometry. This is usually the case for features that are "
				"reconstructed using half-stage rotations (see *reconstruction_method*) since it is "
				"typically much easier to specify the geometry at the time of appearance (as opposed "
				"to present-day). The mid-ocean ridge example below demonstrates this.\n"
				"\n"
				"  Create a mid-ocean ridge feature (note that it must also be reverse reconstructed since "
				"the specified geometry is not present day geometry but instead the geometry of the "
				"mid-ocean ridge at its time of appearance):\n"
				"  ::\n"
				"\n"
				"    time_of_appearance = 55.9\n"
				"    time_of_disappearance = 48\n"
				"    geometry_at_time_of_appearance = pygplates.PolylineOnSphere([...])\n"
				"    mid_ocean_ridge_feature = pygplates.Feature.create_tectonic_section(\n"
				"        pygplates.FeatureType.create_gpml('MidOceanRidge'),\n"
				"        geometry_at_time_of_appearance,\n"
				"        name='SOUTH ATLANTIC, SOUTH AMERICA-AFRICA',\n"
				"        valid_time=(time_of_appearance, time_of_disappearance),\n"
				"        left_plate=201,\n"
				"        right_plate=701,\n"
				"        reconstruction_method='HalfStageRotationVersion2',\n"
				"        reverse_reconstruct=(rotation_model, time_of_appearance))\n"
				"\n"
				"  The previous example is the equivalent of the following (note that the "
				":func:`reverse reconstruction<reverse_reconstruct>` is done *after* the properties have "
				"been set on the feature - this is necessary because reverse reconstruction looks at these "
				"properties to determine how to reverse reconstruct):\n"
				"  ::\n"
				"\n"
				"    mid_ocean_ridge_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('MidOceanRidge'))\n"
				"    mid_ocean_ridge_feature.set_geometry(geometry_at_time_of_appearance)\n"
				"    mid_ocean_ridge_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA')\n"
				"    mid_ocean_ridge_feature.set_valid_time(time_of_appearance, time_of_disappearance)\n"
				"    mid_ocean_ridge_feature.set_left_plate(201)\n"
				"    mid_ocean_ridge_feature.set_right_plate(701)\n"
				"    mid_ocean_ridge_feature.set_reconstruction_method('HalfStageRotationVersion2')\n"
				"    pygplates.reverse_reconstruct(mid_ocean_ridge_feature, rotation_model, time_of_appearance)\n"
				"    \n"
				"    # ...or...\n"
				"    \n"
				"    mid_ocean_ridge_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('MidOceanRidge'))\n"
				"    mid_ocean_ridge_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA')\n"
				"    mid_ocean_ridge_feature.set_valid_time(time_of_appearance, time_of_disappearance)\n"
				"    mid_ocean_ridge_feature.set_left_plate(201)\n"
				"    mid_ocean_ridge_feature.set_right_plate(701)\n"
				"    mid_ocean_ridge_feature.set_reconstruction_method('HalfStageRotationVersion2')\n"
				"    # Set geometry and reverse reconstruct *after* other feature properties have been set.\n"
				"    mid_ocean_ridge_feature.set_geometry(\n"
				"        geometry_at_time_of_appearance,\n"
				"        reverse_reconstruct=(rotation_model, time_of_appearance))\n")
		.staticmethod("create_tectonic_section")
		.def("create_flowline",
				&GPlatesApi::feature_handle_create_flowline,
				(bp::arg("seed_geometry"),
						bp::arg("times"),
						bp::arg("name") = bp::object()/*Py_None*/,
						bp::arg("description") = boost::optional<QString>(),
						bp::arg("valid_time") = bp::object()/*Py_None*/,
						bp::arg("left_plate") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("right_plate") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("other_properties") = bp::object()/*Py_None*/,
						bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
						bp::arg("reverse_reconstruct") = bp::object()/*Py_None*/,
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"create_flowline(seed_geometry, times, [name], [description], [valid_time], "
				"[left_plate], [right_plate], [other_properties], [feature_id], [reverse_reconstruct], "
				"[verify_information_model=VerifyInformationModel.yes])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a flowline feature.\n"
				"\n"
				"  :param seed_geometry: the seed point (or points) - see :meth:`set_geometry` - if geometry "
				"is not present-day geometry then the created feature will need to be reverse reconstructed "
				"to present day (using either the *reverse_reconstruct* parameter or :func:`reverse_reconstruct`) "
				"before the feature can be reconstructed to an arbitrary reconstruction time\n"
				"  :type seed_geometry: :class:`PointOnSphere` or :class:`MultiPointOnSphere`\n"
				"  :param times: the list of times\n"
				"  :type times: sequence (eg, ``list`` or ``tuple``) of float or :class:`GeoTimeInstant`\n"
				"  :param name: the name or names, if not specified then no 'gml:name' properties are added\n"
				"  :type name: string, or sequence of string\n"
				"  :param description: the description, if not specified then a 'gml:description' property is not added\n"
				"  :type description: string\n"
				"  :param valid_time: the (begin_time, end_time) tuple, if not specified then a 'gml:validTime' "
				"property is not added\n"
				"  :type valid_time: a tuple of (float or :class:`GeoTimeInstant`, float or :class:`GeoTimeInstant`)\n"
				"  :param left_plate: the left plate id, if not specified then a 'gpml:leftPlate' property is not added\n"
				"  :type left_plate: int\n"
				"  :param right_plate: the right plate id, if not specified then a 'gpml:rightPlate' property is not added\n"
				"  :type right_plate: int\n"
				"  :param other_properties: any extra property name/value pairs to add, these can alternatively "
				"be added later with :meth:`add`\n"
				"  :type other_properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, "
				":class:`PropertyValue` or sequence of :class:`PropertyValue`)\n"
				"  :param feature_id: the feature identifier, if not specified then a unique feature identifier is created\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param reverse_reconstruct: the tuple (rotation model, seed geometry reconstruction time [, anchor plate id]) "
				"where the anchor plate is optional - if this tuple of reverse reconstruct parameters is specified "
				"then *seed_geometry* is reverse reconstructed using those parameters and any specified feature properties "
				"(eg, *left_plate*) - this is only required if *seed_geometry* is not present day - "
				"alternatively you can subsequently call :func:`reverse_reconstruct`\n"
				"  :type reverse_reconstruct: tuple (:class:`RotationModel`, float or :class:`GeoTimeInstant` [, int])\n"
				"  :param verify_information_model: whether to check the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :rtype: :class:`Feature`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *seed_geometry* is not a :class:`PointOnSphere` or a :class:`MultiPointOnSphere`.\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if *valid_time* has begin time later than end time\n"
				"\n"
				"  This function calls :meth:`set_geometry`, :meth:`set_times`, :meth:`set_name`, "
				":meth:`set_description`, :meth:`set_valid_time`, :meth:`set_left_plate`, "
				":meth:`set_right_plate`, :meth:`set_reconstruction_method` and :meth:`add`.\n"
				"\n"
				"  Create a flowline feature:\n"
				"  ::\n"
				"\n"
				"    present_day_seed_geometry = pygplates.MultiPointOnSphere([...])\n"
				"    flowline_feature = pygplates.Feature.create_flowline(\n"
				"        present_day_seed_geometry,\n"
				"        [0, 10, 20, 30, 40],\n"
				"        valid_time=(50, 0),\n"
				"        left_plate=201,\n"
				"        right_plate=701)\n"
				"\n"
				"  If *seed_geometry* is not present-day geometry then the created feature will need to "
				"be reverse reconstructed to present day using (using either the *reverse_reconstruct* "
				"parameter or :func:`reverse_reconstruct`) before the feature can be reconstructed to an "
				"arbitrary reconstruction time - this is because a feature is not complete until its "
				"geometry is *present day* geometry.\n"
				"\n"
				"  Create a flowline feature (note that it must also be reverse reconstructed since "
				"the specified geometry is not present day geometry):\n"
				"  ::\n"
				"\n"
				"    seed_geometry_at_50Ma = pygplates.MultiPointOnSphere([...])\n"
				"    flowline_feature = pygplates.Feature.create_flowline(\n"
				"        seed_geometry_at_50Ma,\n"
				"        valid_time=(50, 0),\n"
				"        left_plate=201,\n"
				"        right_plate=701,\n"
				"        reverse_reconstruct=(rotation_model, 50))\n"
				"\n"
				"  The previous example is the equivalent of the following (note that the "
				":func:`reverse reconstruction<reverse_reconstruct>` is done *after* the properties have "
				"been set on the feature - this is necessary because reverse reconstruction looks at these "
				"properties to determine how to reverse reconstruct):\n"
				"  ::\n"
				"\n"
				"    flowline_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('Flowline'))\n"
				"    flowline_feature.set_geometry(seed_geometry_at_50Ma)\n"
				"    flowline_feature.set_valid_time(50, 0)\n"
				"    flowline_feature.set_left_plate(201)\n"
				"    flowline_feature.set_right_plate(701)\n"
				"    pygplates.reverse_reconstruct(flowline_feature, rotation_model, 50)\n"
				"    \n"
				"    # ...or...\n"
				"    \n"
				"    flowline_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('Flowline'))\n"
				"    flowline_feature.set_valid_time(50, 0)\n"
				"    flowline_feature.set_left_plate(201)\n"
				"    flowline_feature.set_right_plate(701)\n"
				"    # Set geometry and reverse reconstruct *after* other feature properties have been set.\n"
				"    flowline_feature.set_geometry(\n"
				"        seed_geometry_at_50Ma,\n"
				"        reverse_reconstruct=(rotation_model, 50))\n")
		.staticmethod("create_flowline")
		.def("create_motion_path",
				&GPlatesApi::feature_handle_create_motion_path,
				(bp::arg("seed_geometry"),
						bp::arg("times"),
						bp::arg("name") = bp::object()/*Py_None*/,
						bp::arg("description") = boost::optional<QString>(),
						bp::arg("valid_time") = bp::object()/*Py_None*/,
						bp::arg("relative_plate") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
						bp::arg("other_properties") = bp::object()/*Py_None*/,
						bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
						bp::arg("reverse_reconstruct") = bp::object()/*Py_None*/,
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"create_motion_path(seed_geometry, times, [name], [description], [valid_time], "
				"[relative_plate], [reconstruction_plate_id], [other_properties], [feature_id], "
				"[reverse_reconstruct], [verify_information_model=VerifyInformationModel.yes])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a motion path feature.\n"
				"\n"
				"  :param seed_geometry: the seed point (or points) - see :meth:`set_geometry` - if geometry "
				"is not present-day geometry then the created feature will need to be reverse reconstructed "
				"to present day (using either the *reverse_reconstruct* parameter or :func:`reverse_reconstruct`) "
				"before the feature can be reconstructed to an arbitrary reconstruction time\n"
				"  :type seed_geometry: :class:`PointOnSphere` or :class:`MultiPointOnSphere`\n"
				"  :param times: the list of times\n"
				"  :type times: sequence (eg, ``list`` or ``tuple``) of float or :class:`GeoTimeInstant`\n"
				"  :param name: the name or names, if not specified then no 'gml:name' properties are added\n"
				"  :type name: string, or sequence of string\n"
				"  :param description: the description, if not specified then a 'gml:description' property is not added\n"
				"  :type description: string\n"
				"  :param valid_time: the (begin_time, end_time) tuple, if not specified then a 'gml:validTime' "
				"property is not added\n"
				"  :type valid_time: a tuple of (float or :class:`GeoTimeInstant`, float or :class:`GeoTimeInstant`)\n"
				"  :param relative_plate: the relative plate id, if not specified then a 'gpml:relativePlate' property is not added\n"
				"  :type relative_plate: int\n"
				"  :param reconstruction_plate_id: the reconstruction plate id, if not specified then a "
				"'gpml:reconstructionPlateId' property is not added\n"
				"  :type reconstruction_plate_id: int\n"
				"  :param other_properties: any extra property name/value pairs to add, these can alternatively "
				"be added later with :meth:`add`\n"
				"  :type other_properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, "
				":class:`PropertyValue` or sequence of :class:`PropertyValue`)\n"
				"  :param feature_id: the feature identifier, if not specified then a unique feature identifier is created\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param reverse_reconstruct: the tuple (rotation model, seed geometry reconstruction time [, anchor plate id]) "
				"where the anchor plate is optional - if this tuple of reverse reconstruct parameters is specified "
				"then *seed_geometry* is reverse reconstructed using those parameters and any specified feature properties "
				"(eg, *reconstruction_plate_id*) - this is only required if *seed_geometry* is not present day - "
				"alternatively you can subsequently call :func:`reverse_reconstruct`\n"
				"  :type reverse_reconstruct: tuple (:class:`RotationModel`, float or :class:`GeoTimeInstant` [, int])\n"
				"  :param verify_information_model: whether to check the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :rtype: :class:`Feature`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *seed_geometry* is not a :class:`PointOnSphere` or a :class:`MultiPointOnSphere`.\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if *valid_time* has begin time later than end time\n"
				"\n"
				"  This function calls :meth:`set_geometry`, :meth:`set_times`, :meth:`set_name`, "
				":meth:`set_description`, :meth:`set_valid_time`, :meth:`set_relative_plate`, "
				":meth:`set_reconstruction_plate_id`, :meth:`set_reconstruction_method` and :meth:`add`.\n"
				"\n"
				"  Create a motion path feature:\n"
				"  ::\n"
				"\n"
				"    present_day_seed_geometry = pygplates.MultiPointOnSphere([...])\n"
				"    motion_path_feature = pygplates.Feature.create_motion_path(\n"
				"        present_day_seed_geometry,\n"
				"        [0, 10, 20, 30, 40],\n"
				"        valid_time=(50, 0),\n"
				"        relative_plate=201,\n"
				"        reconstruction_plate_id=701)\n"
				"\n"
				"  If *seed_geometry* is not present-day geometry then the created feature will need to "
				"be reverse reconstructed to present day using (using either the *reverse_reconstruct* "
				"parameter or :func:`reverse_reconstruct`) before the feature can be reconstructed to an "
				"arbitrary reconstruction time - this is because a feature is not complete until its "
				"geometry is *present day* geometry.\n"
				"\n"
				"  Create a motion path feature (note that it must also be reverse reconstructed since "
				"the specified geometry is not present day geometry):\n"
				"  ::\n"
				"\n"
				"    seed_geometry_at_50Ma = pygplates.MultiPointOnSphere([...])\n"
				"    motion_path_feature = pygplates.Feature.create_motion_path(\n"
				"        seed_geometry_at_50Ma,\n"
				"        valid_time=(50, 0),\n"
				"        relative_plate=201,\n"
				"        reconstruction_plate_id=701,\n"
				"        reverse_reconstruct=(rotation_model, 50))\n"
				"\n"
				"  The previous example is the equivalent of the following (note that the "
				":func:`reverse reconstruction<reverse_reconstruct>` is done *after* the properties have "
				"been set on the feature - this is necessary because reverse reconstruction looks at these "
				"properties to determine how to reverse reconstruct):\n"
				"  ::\n"
				"\n"
				"    motion_path_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('MotionPath'))\n"
				"    motion_path_feature.set_geometry(seed_geometry_at_50Ma)\n"
				"    motion_path_feature.set_valid_time(50, 0)\n"
				"    motion_path_feature.set_relative_plate(201)\n"
				"    motion_path_feature.set_reconstruction_plate_id(701)\n"
				"    pygplates.reverse_reconstruct(motion_path_feature, rotation_model, 50)\n"
				"    \n"
				"    # ...or...\n"
				"    \n"
				"    motion_path_feature = pygplates.Feature(pygplates.FeatureType.create_gpml('MotionPath'))\n"
				"    motion_path_feature.set_valid_time(50, 0)\n"
				"    motion_path_feature.set_relative_plate(201)\n"
				"    motion_path_feature.set_reconstruction_plate_id(701)\n"
				"    # Set geometry and reverse reconstruct *after* other feature properties have been set.\n"
				"    motion_path_feature.set_geometry(\n"
				"        seed_geometry_at_50Ma,\n"
				"        reverse_reconstruct=(rotation_model, 50))\n")
		.staticmethod("create_motion_path")
		.def("add",
				&GPlatesApi::feature_handle_add_property,
				(bp::arg("property_name"),
						bp::arg("property_value"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"add(property_name, property_value, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Adds a property (or properties) to this feature.\n"
				"\n"
				"  :param property_name: the name of the property (or properties) to add\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_value: the value (or values) of the property (or properties) to add\n"
				"  :type property_value: :class:`PropertyValue`, or sequence (eg, ``list`` or ``tuple``) "
				"of :class:`PropertyValue`\n"
				"  :param verify_information_model: whether to check the information model before adding (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :returns: the property (or properties) added to the feature\n"
				"  :rtype: :class:`Property`, or list of :class:`Property` depending on whether *property_value* "
				"is a :class:`PropertyValue` or sequence of :class:`PropertyValue`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *property_name* is not a recognised property name or is not supported by the feature type, "
				"or if *property_value* does not have a property value type supported by *property_name*\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_added = feature.add(property_name, property_value)\n"
				"    properties_added = feature.add(property_name, [property_value1, property_value2])\n"
				"    assert(len(properties_added) == 2)\n"
				"\n"
				"  A feature is an *unordered* collection of properties so there is no concept of "
				"where a property is inserted in the sequence of properties.\n"
				"\n"
				"  Note that even a feature of :class:`type<FeatureType>` *gpml:UnclassifiedFeature* will raise "
				"*InformationModelError* if *verify_information_model* is *VerifyInformationModel.yes* and "
				"*property_name* is not recognised by the GPlates Geological Information Model (GPGIM).\n")
		.def("add",
				&GPlatesApi::feature_handle_add_properties,
				(bp::arg("properties"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"add(properties, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Adds properties to this feature.\n"
				"\n"
				"  :param properties: the property name/value pairs to add\n"
				"  :type properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, "
				":class:`PropertyValue` or sequence of :class:`PropertyValue`)\n"
				"  :param verify_information_model: whether to check the information model before adding (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :returns: the list of properties added to the feature\n"
				"  :rtype: ``list`` of :class:`Property`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and any of the property names are not recognised property names or not supported by the feature type, "
				"or if any property value type is not supported by its associated property name\n"
				"\n"
				"  ::\n"
				"\n"
				"    properties_added = feature.add([\n"
				"        (property_name1, property_value1),\n"
				"        (property_name2, property_value2)])\n"
				"    assert(len(properties_added) == 2)\n"
				"    \n"
				"    properties_added = feature.add([\n"
				"        (property_name3, (property_value3a, property_value3b, property_value3c)),\n"
				"        (property_name4, [property_value4a, property_value4b])\n"
				"        (property_name5, property_value5)\n"
				"        ])\n"
				"    assert(len(properties_added) == 6)\n"
				"\n"
				"  A feature is an *unordered* collection of properties so there is no concept of where "
				"a property is inserted in the sequence of properties.\n"
				"\n"
				"  Note that even a feature of :class:`type<FeatureType>` *gpml:UnclassifiedFeature* will raise "
				"*InformationModelError* if *verify_information_model* is *VerifyInformationModel.yes* and "
				"*property_name* is not recognised by the GPlates Geological Information Model (GPGIM).\n")
		.def("remove",
				&GPlatesApi::feature_handle_remove,
				(bp::arg("property_query")),
				"remove(property_query)\n"
				"  Removes properties from this feature.\n"
				"\n"
				"  :param property_query: one or more property names, property instances or predicate functions "
				"that determine which properties to remove\n"
				"  :type property_query: :class:`PropertyName`, or :class:`Property`, or callable "
				"(accepting single :class:`Property` argument), or a sequence (eg, ``list`` or ``tuple``) "
				"of any combination of them\n"
				"  :raises: ValueError if any specified :class:`Property` is not currently a property in this feature\n"
				"\n"
				"  All feature properties matching any :class:`PropertyName` or predicate callable (if any specified) will "
				"be removed. Any specified :class:`PropertyName` or predicate callable that does not match a property "
				"in this feature is ignored. However if any specified :class:`Property` is not currently a property "
				"in this feature then the ``ValueError`` exception is raised - note that the same :class:`Property` *instance* must "
				"have previously been added (in other words the property *values* are not compared - "
				"it actually looks for the same property *instance*).\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature.remove(pygplates.PropertyName.create_gpml('leftPlate'))\n"
				"    feature.remove([\n"
				"        pygplates.PropertyName.create_gpml('leftPlate'),\n"
				"        pygplates.PropertyName.create_gpml('rightPlate')])\n"
				"    \n"
				"    for property in feature:\n"
				"        if predicate(property):\n"
				"            feature.remove(property)\n"
				"    feature.remove(predicate)\n"
				"    feature.remove([property for property in feature if predicate(property)])\n"
				"    # Specifying just an iterator also works...\n"
				"    feature.remove(property for property in feature if predicate(property))\n"
				"    \n"
				"    # Mix different query types.\n"
				"    # Remove a specific 'property' instance and any 'gpml:leftPlate' properties...\n"
				"    feature.remove([property, pygplates.PropertyName.create_gpml('leftPlate')])\n"
				"    \n"
				"    # Remove 'gpml:leftPlate' properties with plate IDs less than 700...\n"
				"    feature.remove(\n"
				"        lambda property: property.get_name() == pygplates.PropertyName.create_gpml('leftPlate') and\n"
				"                         property.get_value().get_plate_id() < 700)\n"
				"    \n"
				"    # Remove 'gpml:leftPlate' and 'gpml:rightPlate' properties...\n"
				"    feature.remove([\n"
				"        lambda property: property.get_name() == pygplates.PropertyName.create_gpml('leftPlate'),\n"
				"        pygplates.PropertyName.create_gpml('rightPlate')])\n"
				"    feature.remove(\n"
				"        lambda property: property.get_name() == pygplates.PropertyName.create_gpml('leftPlate') or\n"
				"                         property.get_name() == pygplates.PropertyName.create_gpml('rightPlate'))\n")
		.def("set",
				&GPlatesApi::feature_handle_set_property,
				(bp::arg("property_name"),
						bp::arg("property_value"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"set(property_name, property_value, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Sets a property (or properties) to this feature.\n"
				"\n"
				"  :param property_name: the name of the property (or properties) to set\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_value: the value (or values) of the property (or properties) to set\n"
				"  :type property_value: :class:`PropertyValue`, or sequence (eg, ``list`` or ``tuple``) "
				"of :class:`PropertyValue`\n"
				"  :param verify_information_model: whether to check the information model before setting (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :returns: the property (or properties) set in the feature\n"
				"  :rtype: :class:`Property`, or list of :class:`Property` depending on whether *property_value* "
				"is a :class:`PropertyValue` or sequence of :class:`PropertyValue`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *property_name* is not a recognised property name or is not supported by the feature type, "
				"or if *property_value* does not have a property value type supported by *property_name*\n"
				"\n"
				"  ::\n"
				"\n"
				"    property = feature.set(property_name, property_value)\n"
				"    properties = feature.set(property_name, [property_value1, property_value2])\n"
				"    assert(len(properties) == 2)\n"
				"\n"
				"  This method essentially has the same effect as calling :meth:`remove` followed by :meth:`add`:\n"
				"  ::\n"
				"\n"
				"    def set(feature, property_name, property_value, verify_information_model):\n"
				"        feature.remove(property_name)\n"
				"        return feature.add(property_name, property_value, verify_information_model)\n"
				"\n"
				"  Note that even a feature of :class:`type<FeatureType>` *gpml:UnclassifiedFeature* will raise "
				"*InformationModelError* if *verify_information_model* is *VerifyInformationModel.yes* and "
				"*property_name* is not recognised by the GPlates Geological Information Model (GPGIM).\n")
		.def("get",
				&GPlatesApi::feature_handle_get_property,
				(bp::arg("property_query"),
						bp::arg("property_return") = GPlatesApi::PropertyReturn::EXACTLY_ONE),
				"get(property_query, [property_return=PropertyReturn.exactly_one])\n"
				"  Returns one or more properties matching a property name or predicate.\n"
				"\n"
				"  :param property_query: the property name (or predicate function) that matches the property "
				"(or properties) to get\n"
				"  :type property_query: :class:`PropertyName`, or callable (accepting single :class:`Property` argument)\n"
				"  :param property_return: whether to return exactly one property, the first property or "
				"all matching properties\n"
				"  :type property_return: *PropertyReturn.exactly_one*, *PropertyReturn.first* or *PropertyReturn.all*\n"
				"  :rtype: :class:`Property`, or ``list`` of :class:`Property`, or None\n"
				"\n"
				"  This method is similar to :meth:`get_value` except it returns properties instead "
				"of property *values*.\n"
				"\n"
				"  The following table maps *property_return* values to return values:\n"
				"\n"
				"  ======================================= ==============\n"
				"  PropertyReturn Value                     Description\n"
				"  ======================================= ==============\n"
				"  exactly_one                             Returns a :class:`Property` only if "
				"*property_query* matches exactly one property, otherwise ``None`` is returned.\n"
				"  first                                   Returns the first :class:`Property` that matches "
				"*property_query* - however note that a feature is an *unordered* collection of "
				"properties. If no properties match then ``None`` is returned.\n"
				"  all                                     Returns a ``list`` of :class:`properties<Property>` "
				"matching *property_query*. If no properties match then the returned list will be empty.\n"
				"  ======================================= ==============\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_gml('validTime')\n"
				"    exactly_one_property = feature.get(property_name)\n"
				"    first_property = feature.get(property_name, pygplates.PropertyReturn.first)\n"
				"    all_properties = feature.get(property_name, pygplates.PropertyReturn.all)\n"
				"    \n"
				"    # A predicate function that returns true if property is 'gpml:reconstructionPlateId' "
				"with value less than 700.\n"
				"    def recon_plate_id_less_700(property):\n"
				"      return property.get_name() == pygplates.PropertyName.create_gpml('reconstructionPlateId') and \\\n"
				"             property.get_value().get_plate_id() < 700\n"
				"    \n"
				"    recon_plate_id_less_700_property = feature.get(recon_plate_id_less_700)\n"
				"    assert(recon_plate_id_less_700_property.get_value().get_plate_id() < 700)\n")
		.def("get_value",
				&GPlatesApi::feature_handle_get_property_value,
				(bp::arg("property_query"),
						bp::arg("time") = GPlatesPropertyValues::GeoTimeInstant(0),
						bp::arg("property_return") = GPlatesApi::PropertyReturn::EXACTLY_ONE),
				"get_value(property_query, [time=0], [property_return=PropertyReturn.exactly_one])\n"
				"  Returns one or more values of properties matching a property name or predicate.\n"
				"\n"
				"  :param property_query: the property name (or predicate function) that matches the property "
				"(or properties) to get\n"
				"  :type property_query: :class:`PropertyName`, or callable (accepting single :class:`Property` argument)\n"
				"  :param time: the time to extract value (defaults to present day)\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :param property_return: whether to return exactly one property, the first property or "
				"all matching properties\n"
				"  :type property_return: *PropertyReturn.exactly_one*, *PropertyReturn.first* or *PropertyReturn.all*\n"
				"  :rtype: :class:`PropertyValue`, or ``list`` of :class:`PropertyValue`, or None\n"
				"\n"
				"  This method is essentially the same as :meth:`get` except it also calls "
				":meth:`Property.get_value` on each property.\n"
				"\n"
				"  The following table maps *property_return* values to return values:\n"
				"\n"
				"  ======================================= ==============\n"
				"  PropertyReturn Value                     Description\n"
				"  ======================================= ==============\n"
				"  exactly_one                             Returns a :class:`value<PropertyValue>` only if "
				"*property_query* matches exactly one property, otherwise ``None`` is returned. "
				"Note that ``None`` can still be returned, even if exactly one property matches, "
				"due to :meth:`Property.get_value` returning ``None``.\n"
				"  first                                   Returns the :class:`value<PropertyValue>` "
				"of the first property matching *property_query* - however note that a feature "
				"is an *unordered* collection of properties. If no properties match then ``None`` is returned. "
				"Note that ``None`` can still be returned for the first matching property due to "
				":meth:`Property.get_value` returning ``None``.\n"
				"  all                                     Returns a ``list`` of :class:`values<PropertyValue>` "
				"of properties matching *property_query*. If no properties match then the returned list will be empty. "
				"Any matching properties where :meth:`Property.get_value` returns ``None`` will not be added to the list.\n"
				"  ======================================= ==============\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_gml('validTime')\n"
				"    exactly_one_property_value = feature.get_value(property_name)\n"
				"    first_property_value = feature.get_value(property_name, property_return=pygplates.PropertyReturn.first)\n"
				"    all_property_values = feature.get_value(property_name, property_return=pygplates.PropertyReturn.all)\n"
				"    \n"
				"    # Using a predicate function that returns true if property is 'gpml:reconstructionPlateId' "
				"with value less than 700.\n"
				"    recon_plate_id_less_700_property_value = feature.get_value(\n"
				"        lambda property: property.get_name() == pygplates.PropertyName.create_gpml('reconstructionPlateId') and\n"
				"                         property.get_value().get_plate_id() < 700)\n"
				"    assert(recon_plate_id_less_700_property_value.get_plate_id() < 700)\n")
		.def("set_geometry",
				&GPlatesApi::feature_handle_set_geometry,
				(bp::arg("geometry"),
						bp::arg("property_name") = boost::optional<GPlatesModel::PropertyName>(),
						bp::arg("reverse_reconstruct") = bp::object()/*Py_None*/,
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"set_geometry(geometry, [property_name], [reverse_reconstruct], "
				"[verify_information_model=VerifyInformationModel.yes])\n"
				"  Set the geometry (or geometries) of this feature.\n"
				"\n"
				"  :param geometry: the geometry or geometries (or coverage or coverages - see below) to set "
				"- if the geometry(s) is not present-day geometry then this feature will need to be "
				"reverse reconstructed to present day using (using either the *reverse_reconstruct* "
				"parameter or :func:`reverse_reconstruct`) before this feature can be reconstructed to "
				"an arbitrary reconstruction time\n"
				"  :type geometry: :class:`GeometryOnSphere`, or sequence (eg, ``list`` or ``tuple``) "
				"of :class:`GeometryOnSphere` (or a coverage or a sequence of coverages - see below)\n"
				"  :param property_name: the optional property name of the geometry property or properties to set, "
				"if not specified then the default geometry property name associated with this feature's "
				":class:`type<FeatureType>` is used instead\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param reverse_reconstruct: the tuple (rotation model, geometry reconstruction time [, anchor plate id]) "
				"where the anchor plate is optional - if this tuple of reverse reconstruct parameters is specified "
				"then *geometry* is reverse reconstructed using those parameters and this feature's existing properties "
				"(eg, reconstruction plate id) - this is only required if *geometry* is not present day - "
				"alternatively you can subsequently call :func:`reverse_reconstruct`\n"
				"  :type reverse_reconstruct: tuple (:class:`RotationModel`, float or :class:`GeoTimeInstant` [, int])\n"
				"  :param verify_information_model: whether to check the information model before setting (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :returns: the geometry property (or properties) set in the feature\n"
				"  :rtype: :class:`Property`, or list of :class:`Property` depending on whether *geometry* "
				"is a :class:`GeometryOnSphere` or sequence of :class:`GeometryOnSphere`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *property_name* is specified but is not a recognised property name or is not supported by "
				"this feature's :class:`type<FeatureType>`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and multiple geometries (if specified in *geometry*) are not supported by *property_name* "
				"(or the default geometry property name if *property_name* not specified)\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and any :class:`geometry type<GeometryOnSphere>` in *geometry* is not supported for "
				"*property_name* (or the default geometry property name if *property_name* not specified)\n"
				"  :raises: AmbiguousGeometryCoverageError if multiple coverages are specified (in *geometry*) "
				"and more than one has the same number of points (or scalar values) - the ambiguity is due to not being able to "
				"subsequently determine which coverage range property is associated with which coverage domain property\n"
				"  :raises: ValueError if *geometry* is one or more coverages where the number of points in a coverage "
				"geometry is not equal to the number of scalar values associated with it\n"
				"  :raises: ValueError if *geometry* is one or more coverages where the scalar values are "
				"incorrectly specified - see :meth:`GmlDataBlock.__init__` for details\n"
				"\n"
				"  This is a convenience method to make setting geometry easier.\n"
				"\n"
				"  Usually a :class:`feature type<FeatureType>` supports *geometry* properties with more than "
				"one property name (eg, 'gpml:centerLineOf' and 'gpml:unclassifiedGeometry'). "
				"But only one of them is the default (eg, the default property that geometry data is imported into). "
				"If *property_name* is not specified then this method determines the default property name "
				"from this feature's :class:`type<FeatureType>` and sets the geometry in one or more "
				"properties of that :class:`PropertyName`.\n"
				"\n"
				"  The question of how many distinct geometries are allowed per feature is a little more tricky. "
				"Some geometry properties, such as 'gpml:centerLineOf', support multiple properties per "
				"feature and support any :class:`geometry type<GeometryOnSphere>`. Other geometry "
				"properties, such as 'gpml:boundary', tend to support only one property per feature "
				"and only some :class:`geometry types<GeometryOnSphere>` (eg, only :class:`PolylineOnSphere` "
				"and :class:`PolygonOnSphere`). However the :class:`geometry type<GeometryOnSphere>` is usually "
				"apparent given the feature type. For example a 'gpml:Isochron' feature typically contains "
				"a :class:`polyline<PolylineOnSphere>` whereas a 'gpml:HotSpot' feature contains a "
				":class:`point<PointOnSphere>`.\n"
				"\n"
				"  Set the default geometry:\n"
				"  ::\n"
				"\n"
				"    feature.set_geometry(default_geometry)\n"
				"\n"
				"  Set the list of default geometries:\n"
				"  ::\n"
				"\n"
				"    default_geometries = []\n"
				"    ...\n"
				"    feature.set_geometry(default_geometries)\n"
				"\n"
				"  Set the geometry associated with a property named 'gpml:averageSampleSitePosition':\n"
				"  ::\n"
				"\n"
				"    feature.set_geometry(\n"
				"        average_sample_site_position,\n"
				"        pygplates.PropertyName.create_gpml('averageSampleSitePosition'))\n"
				"\n"
				"  Set the list of geometries associated with the property name 'gpml:unclassifiedGeometry':\n"
				"  ::\n"
				"\n"
				"    unclassified_geometries = []\n"
				"    ...\n"
				"    feature.set_geometry(\n"
				"        unclassified_geometries,\n"
				"        pygplates.PropertyName.create_gpml('unclassifiedGeometry'))\n"
				"\n"
				"  If *geometry* is not present-day geometry then the created feature will need to be "
				"reverse reconstructed to present day using (using either the *reverse_reconstruct* "
				"parameter or :func:`reverse_reconstruct`) before the feature can be reconstructed to "
				"an arbitrary reconstruction time - this is because a feature is not complete until its "
				"geometry is *present day* geometry. This is usually the case for features that are "
				"reconstructed using half-stage rotations since it is typically much easier to specify "
				"the geometry at the geological time at which the feature is digitised (as opposed to "
				"present-day) as the following example demonstrates:\n"
				"  ::\n"
				"\n"
				"    time_of_digitisation = 50\n"
				"    ridge_geometry_at_digitisation_time = pygplates.PolylineOnSphere([...])\n"
				"    mid_ocean_ridge_feature.set_geometry(\n"
				"        ridge_geometry_at_digitisation_time,\n"
				"        reverse_reconstruct=(rotation_model, time_of_digitisation))\n"
				"\n"
				"  The previous example is the equivalent of the following:\n"
				"  ::\n"
				"\n"
				"    time_of_digitisation = 50\n"
				"    ridge_geometry_at_digitisation_time = pygplates.PolylineOnSphere([...])\n"
				"    mid_ocean_ridge_feature.set_geometry(ridge_geometry_at_digitisation_time)\n"
				"    pygplates.reverse_reconstruct(mid_ocean_ridge_feature, rotation_model, time_of_digitisation)\n"
				"\n"
				"  .. note:: *geometry* can also be a coverage or sequence of coverages - where a coverage essentially "
				"maps each point in a geometry to one or more scalar values. A coverage is specified in *geometry* "
				"as a (:class:`GeometryOnSphere`, *scalar-values-dictionary*) tuple (or a sequence of tuples) "
				"where *scalar-values-dictionary* is a ``dict`` that maps :class:`scalar types<ScalarType>` to "
				"lists of scalar values. This is the same as the sole argument to :meth:`GmlDataBlock.__init__`. "
				"The number of scalar values, associated with each :class:`ScalarType` should be equal to the "
				"number of points in the geometry.\n"
				"\n"
				"     Set the velocity coverage on the default geometry:\n"
				"     ::\n"
				"\n"
				"       coverage_geometry = pygplates.MultiPointOnSphere([(0,0), (0,10), (0,20)])\n"
				"       coverage_scalars = {\n"
				"           pygplates.ScalarType.create_gpml('VelocityColat') : [-1.5, -1.6, -1.55],\n"
				"           pygplates.ScalarType.create_gpml('VelocityLon') : [0.36, 0.37, 0.376]}\n"
				"       feature.set_geometry((coverage_geometry, coverage_scalars))\n"
				"\n"
				"  .. warning:: If more than one coverage geometry is specified in *geometry* then the "
				"number of points in each coverage geometry should be different otherwise "
				"*AmbiguousGeometryCoverageError* will be raised. Due to this restriction it's better "
				"to set only a single coverage (per geometry property name) - but that single coverage "
				"can still have more than one list of scalars.\n")
		.def("get_geometry",
				&GPlatesApi::feature_handle_get_geometry,
				(bp::arg("property_query") = bp::object()/*Py_None*/,
						bp::arg("property_return") = GPlatesApi::PropertyReturn::EXACTLY_ONE,
						bp::arg("coverage_return") = GPlatesApi::CoverageReturn::GEOMETRY_ONLY),
				"get_geometry([property_query], [property_return=PropertyReturn.exactly_one], "
				"[coverage_return=CoverageReturn.geometry_only])\n"
				"  Return the *present day* geometry (or geometries) of this feature.\n"
				"\n"
				"  :param property_query: the optional property name or predicate function used to find "
				"the geometry property or properties, if not specified then the default geometry property "
				"name associated with this feature's :class:`type<FeatureType>` is used instead\n"
				"  :type property_query: :class:`PropertyName`, or callable (accepting single :class:`Property` argument)\n"
				"  :param property_return: whether to return exactly one geometry, the first geometry or all geometries\n"
				"  :type property_return: *PropertyReturn.exactly_one*, *PropertyReturn.first* or *PropertyReturn.all*\n"
				"  :param coverage_return: whether to return geometry(s) only (the default), or coverage(s) "
				"(where a coverage is a geometry and associated per-point scalar values)\n"
				"  :type coverage_return: *CoverageReturn.geometry_only* or *CoverageReturn.geometry_and_scalars*\n"
				"  :rtype: :class:`GeometryOnSphere`, or list of :class:`GeometryOnSphere`, or None\n"
				"\n"
				"  This is a convenience method to make geometry retrieval easier.\n"
				"\n"
				"  Usually a :class:`feature type<FeatureType>` supports *geometry* properties with more than "
				"one property name (eg, 'gpml:centerLineOf' and 'gpml:unclassifiedGeometry'). "
				"But only one of them is the default (eg, the default property that geometry data is imported into). "
				"If *property_query* is not specified then this method determines the default property name "
				"from this feature's :class:`type<FeatureType>` and retrieves the geometry from one or more "
				"properties of that :class:`PropertyName`.\n"
				"\n"
				"  The question of how many distinct geometries are allowed per feature is a little more tricky. "
				"Some geometry properties, such as 'gpml:centerLineOf', support multiple properties per "
				"feature and support any :class:`geometry type<GeometryOnSphere>`. Other geometry "
				"properties, such as 'gpml:boundary', tend to support only one property per feature "
				"and only some :class:`geometry types<GeometryOnSphere>` (eg, only :class:`PolylineOnSphere` "
				"and :class:`PolygonOnSphere`). However the :class:`geometry type<GeometryOnSphere>` is usually "
				"apparent given the feature type. For example a 'gpml:Isochron' feature typically contains "
				"a :class:`polyline<PolylineOnSphere>` whereas a 'gpml:HotSpot' feature contains a "
				":class:`point<PointOnSphere>`.\n"
				"\n"
				"  The following table maps *property_return* values to return values:\n"
				"\n"
				"  ===================== ==============\n"
				"  PropertyReturn Value   Description\n"
				"  ===================== ==============\n"
				"  exactly_one           Returns the geometry if exactly one matching geometry property is found, "
				"otherwise ``None`` is returned.\n"
				"  first                 Returns the geometry of the first matching geometry property - "
				"however note that a feature is an *unordered* collection of properties. Returns ``none`` "
				"if there are no matching geometry properties.\n"
				"  all                   Returns a ``list`` of geometries of matching geometry properties. "
				"Returns an empty list if there are no matching geometry properties.\n"
				"  ===================== ==============\n"
				"\n"
				"  Return the default geometry (returns ``None`` if not exactly one default geometry property found):\n"
				"  ::\n"
				"\n"
				"    default_geometry = feature.get_geometry()\n"
				"    if default_geometry:\n"
				"        ...\n"
				"\n"
				"  Return the list of default geometries (defaults to an empty list if no default geometry properties are found):\n"
				"  ::\n"
				"\n"
				"    default_geometries = feature.get_geometry(property_return=pygplates.PropertyReturn.all)\n"
				"\n"
				"    # ...or more conveniently...\n"
				"\n"
				"    default_geometries = feature.get_geometries()\n"
				"\n"
				"  Return the geometry associated with the property named 'gpml:averageSampleSitePosition':\n"
				"  ::\n"
				"\n"
				"    average_sample_site_position = feature.get_geometry(\n"
				"        pygplates.PropertyName.create_gpml('averageSampleSitePosition'))\n"
				"\n"
				"  Return the list of all geometries (regardless of which properties they came from):\n"
				"  ::\n"
				"\n"
				"    all_geometries = feature.get_geometry(\n"
				"        lambda property: True,\n"
				"        pygplates.PropertyReturn.all)\n"
				"\n"
				"    # ...or more conveniently...\n"
				"\n"
				"    all_geometries = feature.get_all_geometries()\n"
				"\n"
				"  Return the geometry (regardless of which property it came from) - returns ``None`` "
				"if not exactly one geometry property found:\n"
				"  ::\n"
				"\n"
				"    geometry = feature.get_geometry(lambda property: True)\n"
				"    if geometry:\n"
				"        ...\n"
				"\n"
				"  .. note:: If *CoverageReturn.geometry_and_scalars* is specified for *coverage_return* "
				"then a coverage (or sequence of coverages) is returned - where a coverage essentially "
				"maps each point in a geometry to one or more scalar values. A coverage is returned "
				"as a (:class:`GeometryOnSphere`, *scalar-values-dictionary*) tuple "
				"where *scalar-values-dictionary* is a ``dict`` that maps :class:`scalar types<ScalarType>` to "
				"lists of scalar values. This is the same as the sole argument to :meth:`GmlDataBlock.__init__`. "
				"The number of scalar values, associated with each :class:`ScalarType` should be equal to the "
				"number of points in the geometry.\n"
				"\n"
				"     Get the velocity coverage on the default geometry:\n"
				"     ::\n"
				"\n"
				"       default_coverage = feature.get_geometry(coverage_return=pygplates.CoverageReturn.geometry_and_scalars)\n"
				"       if default_coverage:\n"
				"           coverage_geometry, coverage_scalars = default_coverage\n"
				"           coverage_points = coverage_geometry.get_points()\n"
				"           velocity_colat_scalars = coverage_scalars.get(\n"
				"               pygplates.ScalarType.create_gpml('VelocityColat'))\n"
				"           velocity_lon_scalars = coverage_scalars.get(\n"
				"               pygplates.ScalarType.create_gpml('VelocityLon'))\n")
		.def("get_geometries",
				&GPlatesApi::feature_handle_get_geometries,
				(bp::arg("property_query") = bp::object()/*Py_None*/,
						bp::arg("coverage_return") = GPlatesApi::CoverageReturn::GEOMETRY_ONLY),
				"get_geometries([property_query], [coverage_return=CoverageReturn.geometry_only])\n"
				"  Return a list of the *present day* geometries of this feature.\n"
				"\n"
				"  :param property_query: the optional property name or predicate function used to find "
				"the geometry properties, if not specified then the default geometry property "
				"name associated with this feature's :class:`type<FeatureType>` is used instead\n"
				"  :type property_query: :class:`PropertyName`, or callable (accepting single :class:`Property` argument)\n"
				"  :param coverage_return: whether to return geometries only (the default), or coverages "
				"(where a coverage is a geometry and associated per-point scalar values)\n"
				"  :type coverage_return: *CoverageReturn.geometry_only* or *CoverageReturn.geometry_and_scalars*\n"
				"  :rtype: list of :class:`GeometryOnSphere`\n"
				"\n"
				"  | This is a convenient alternative to :meth:`get_geometry` that returns a ``list`` "
				"of matching geometries without having to specify ``pygplates.PropertyReturn.all``.\n"
				"  | This method is essentially equivalent to:\n"
				"\n"
				"  ::\n"
				"\n"
				"    def get_geometries(feature, property_query, coverage_return):\n"
				"        return feature.get_geometry(property_query, pygplates.PropertyReturn.all, coverage_return)\n"
				"\n"
				"  See :meth:`get_geometry` for more details.\n")
		.def("get_all_geometries",
				&GPlatesApi::feature_handle_get_all_geometries,
				(bp::arg("coverage_return") = GPlatesApi::CoverageReturn::GEOMETRY_ONLY),
				"get_all_geometries([coverage_return=CoverageReturn.geometry_only])\n"
				"  Return a list of all *present day* geometries of this feature (regardless of their property names).\n"
				"\n"
				"  :param coverage_return: whether to return geometries only (the default), or coverages "
				"(where a coverage is a geometry and associated per-point scalar values)\n"
				"  :type coverage_return: *CoverageReturn.geometry_only* or *CoverageReturn.geometry_and_scalars*\n"
				"  :rtype: list of :class:`GeometryOnSphere`\n"
				"\n"
				"  | This is a convenient alternative to :meth:`get_geometries` that returns a ``list`` "
				"of *all* geometries regardless of their :class:`property names<PropertyName>`.\n"
				"  | This method is equivalent to:\n"
				"\n"
				"  ::\n"
				"\n"
				"    def get_all_geometries(feature, coverage_return):\n"
				"        return feature.get_geometries(\n"
				"            lambda property: True,\n"
				"            coverage_return)\n"
				"\n"
				"  See :meth:`get_geometries` for more details.\n")
		.def("get_feature_type",
				&GPlatesModel::FeatureHandle::feature_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_feature_type()\n"
				"  Returns the feature type.\n"
				"\n"
				"  :rtype: :class:`FeatureType`\n")
		.def("get_feature_id",
				&GPlatesModel::FeatureHandle::feature_id,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_feature_id()\n"
				"  Returns the feature identifier.\n"
				"\n"
				"  :rtype: :class:`FeatureId`\n")
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesModel::FeatureHandle>();
}

// This is here at the end of the layer because the problem appears to reside
// in a template being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

#endif // GPLATES_NO_PYTHON
