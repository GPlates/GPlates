/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GPMLSTRUCTURALTYPEREADERUTILS_H
#define GPLATES_FILEIO_GPMLSTRUCTURALTYPEREADERUTILS_H

#include <map>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>

#include "ReadErrors.h"

#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureId.h"
#include "model/PropertyValue.h"
#include "model/RevisionId.h"
#include "model/XmlElementName.h"
#include "model/XmlNode.h"

// Please keep these ordered alphabetically...
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlGridEnvelope.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalSection.h"


namespace GPlatesModel
{
	class GpgimVersion;
}	

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
	class GpmlPropertyStructuralTypeReader;

	/**
	 * This namespace contains utilities for reading structural types from GPML files.
	 *
	 * NOTE: It excludes structural types that can be feature properties.
	 * Utilities for those 'property' structural types can be found in 'GpmlPropertyStructuralTypeReaderUtils'.
	 */
	namespace GpmlStructuralTypeReaderUtils
	{
		//
		// NOTE: In the following functions, 'gpml_version' represents the GPGIM version
		// used when the GPML file (currently being read) was written.
		//
		// This allows us to modify the structural types in newer GPGIM revisions while still
		// allowing older GPML files to be read (into the current/latest GPGIM format).
		//


		//
		// Feature-id and revision-id.
		//


		GPlatesModel::FeatureId
		create_feature_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesModel::RevisionId
		create_revision_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		//
		// GML namespace.
		//
		// Please keep these ordered alphabetically...


		GPlatesPropertyValues::GmlGridEnvelope::non_null_ptr_type
		create_gml_grid_envelope(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		//
		// GPML namespace.
		//
		// Please keep these ordered alphabetically...


		GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type
		create_gpml_finite_rotation_slerp(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type
		create_gpml_interpolation_function(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlKeyValueDictionaryElement
		create_gpml_key_value_dictionary_element(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type
		create_gpml_property_delegate(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesModel::PropertyValue::non_null_ptr_type
		create_gpml_time_dependent_property_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTimeSample
		create_gpml_time_sample(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTimeWindow
		create_gpml_time_window(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalNetwork::Interior
		create_gpml_topological_network_interior(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
		create_gpml_topological_line_section(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
		create_gpml_topological_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type
		create_gpml_topological_section(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		/**
		 * This function will extract the single child of the given elem and return
		 * it.  If there is more than one child, or the type was not found,
		 * an exception is thrown.
		 */
		GPlatesModel::XmlElementNode::non_null_ptr_type
		get_structural_type_element(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::XmlElementName &xml_element_name);


		/**
		 * Retrieve the qualified template type.
		 *
		 * This is useful for checking types wrapped in time-dependent wrappers (like 'gpml:ConstantValue').
		 *
		 * For example, to get 'gml:Polygon' from a 'gpml:valueType' XML element (inside a 'gpml:ConstantValue').
		 */
		GPlatesPropertyValues::StructuralType 
		create_template_type_parameter_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
		get_xml_attributes_from_child(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::XmlElementName &xml_element_name);


		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
		find_optional(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::XmlElementName &xml_element_name,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);

		template< typename T >
		boost::optional<T>
		find_and_create_optional(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> target =
					find_optional(elem, xml_element_name, read_errors);
			if (!target)
			{
				// We didn't find the property, but that's okay here.
				return boost::optional<T>();
			}

			return (*creation_fn)(target.get(), gpml_version, read_errors);  // Can throw.
		}

		template< typename T >
		boost::optional<T>
		find_and_create_optional(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesFileIO::GpmlPropertyStructuralTypeReader &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> target =
					find_optional(elem, xml_element_name, read_errors);
			if (!target)
			{
				// We didn't find the property, but that's okay here.
				return boost::optional<T>();
			}

			return (*creation_fn)(target.get(), structural_type_reader, gpml_version, read_errors);  // Can throw.
		}


		GPlatesModel::XmlElementNode::non_null_ptr_type
		find_one(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::XmlElementName &xml_element_name,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);

		template< typename T >
		T
		find_and_create_one(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			GPlatesModel::XmlElementNode::non_null_ptr_type target =
					find_one(elem, xml_element_name, read_errors);

			return (*creation_fn)(target, gpml_version, read_errors);  // Can throw.
		}

		template< typename T >
		T
		find_and_create_one(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesFileIO::GpmlPropertyStructuralTypeReader &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			GPlatesModel::XmlElementNode::non_null_ptr_type target =
					find_one(elem, xml_element_name, read_errors);

			return (*creation_fn)(target, structural_type_reader, gpml_version, read_errors);  // Can throw.
		}


		void
		find_zero_or_more(
				std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> &targets,
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::XmlElementName &xml_element_name,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);

		template< typename T, typename CollectionOfT>
		void
		find_and_create_zero_or_more(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				CollectionOfT &destination,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> targets;
			find_zero_or_more(targets, elem, xml_element_name, read_errors);

			BOOST_FOREACH(const GPlatesModel::XmlElementNode::non_null_ptr_type &target, targets)
			{
				// Note: creation function can throw.
				destination.push_back((*creation_fn)(target, gpml_version, read_errors));
			}
		}

		template< typename T, typename CollectionOfT>
		void
		find_and_create_zero_or_more(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesFileIO::GpmlPropertyStructuralTypeReader &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				CollectionOfT &destination,
				const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> targets;
			find_zero_or_more(targets, elem, xml_element_name, read_errors);

			BOOST_FOREACH(const GPlatesModel::XmlElementNode::non_null_ptr_type &target, targets)
			{
				// Note: creation function can throw.
				destination.push_back((*creation_fn)(target, structural_type_reader, gpml_version, read_errors));
			}
		}


		void
		find_one_or_more(
				std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> &targets,
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::XmlElementName &xml_element_name,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);

		template< typename T, typename CollectionOfT>
		void
		find_and_create_one_or_more(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				CollectionOfT &destination,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> targets;
			find_one_or_more(targets, elem, xml_element_name, read_errors);

			BOOST_FOREACH(const GPlatesModel::XmlElementNode::non_null_ptr_type &target, targets)
			{
				// Note: creation function can throw.
				destination.push_back((*creation_fn)(target, gpml_version, read_errors));
			}
		}

		template< typename T, typename CollectionOfT>
		void
		find_and_create_one_or_more(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				T (*creation_fn)(
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesFileIO::GpmlPropertyStructuralTypeReader &,
						const GPlatesModel::GpgimVersion &,
						GPlatesFileIO::ReadErrorAccumulation &),
				const GPlatesModel::XmlElementName &xml_element_name,
				CollectionOfT &destination,
				const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			std::vector<GPlatesModel::XmlElementNode::non_null_ptr_type> targets;
			find_one_or_more(targets, elem, xml_element_name, read_errors);

			BOOST_FOREACH(const GPlatesModel::XmlElementNode::non_null_ptr_type &target, targets)
			{
				// Note: creation function can throw.
				destination.push_back((*creation_fn)(target, structural_type_reader, gpml_version, read_errors));
			}
		}


		GPlatesModel::PropertyValue::non_null_ptr_type
		find_and_create_from_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesPropertyValues::StructuralType &type,
				const GPlatesModel::XmlElementName &xml_element_name,
				const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		void
		find_and_create_one_or_more_from_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesPropertyValues::StructuralType &type,
				const GPlatesModel::XmlElementName &xml_element_name,
				std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members,
				const GPlatesFileIO::GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		QString
		create_string_without_trimming(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		QString
		create_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		QString
		create_nonempty_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		GPlatesUtils::UnicodeString
		create_unicode_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		bool
		create_boolean(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		double
		create_double(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		std::vector<double>
		create_double_list(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		unsigned long
		create_ulong(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		int
		create_int(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		std::vector<int>
		create_int_list(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		unsigned int
		create_uint(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		GPlatesMaths::PointOnSphere
		create_pos(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		// Similar to create_pos but returns it as (lon, lat) pair.
		// This is to ensure that the longitude doesn't get wiped when reading in a
		// point physically at the north pole.
		std::pair<double, double>
		create_lon_lat_pos(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * The same as @a create_pos, except that there's a comma between the two
		 * values instead of whitespace.
		 */
		GPlatesMaths::PointOnSphere
		create_coordinates(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);

		std::pair<double, double>
		create_lon_lat_coordinates(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		create_polyline(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		create_polygon(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * This function is used by create_gml_polygon to traverse the LinearRing intermediate junk.
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		create_linear_ring(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * This function is used by create_gml_point and create_gml_multi_point to do the
		 * common work of creating a GPlatesMaths::PointOnSphere.
		 */
		std::pair<GPlatesMaths::PointOnSphere, GPlatesPropertyValues::GmlPoint::GmlProperty>
		create_point_on_sphere(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		std::pair<std::pair<double, double>, GPlatesPropertyValues::GmlPoint::GmlProperty>
		create_lon_lat_point_on_sphere(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GeoTimeInstant
		create_geo_time_instant(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * This function is to traverse a 'gpml:TopologicalSections' intermediate XML element.
		 */
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
		create_topological_sections(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * Extracts out the value object template, i.e. the app:Temperature part of the
		 * example on p.253 of the GML book.
		 */
		GPlatesPropertyValues::GmlFile::value_component_type
		create_gml_file_value_component(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * Used by create_gml_file to create the gml:CompositeValue structural type inside
		 * a gml:File.
		 */
		GPlatesPropertyValues::GmlFile::composite_value_type
		create_gml_file_composite_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);
	}
}

#endif // GPLATES_FILEIO_GPMLSTRUCTURALTYPEREADERUTILS_H
