/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GPMLPROPERTYREADER_H
#define GPLATES_FILE_IO_GPMLPROPERTYREADER_H

#include <list>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "GpmlPropertyStructuralTypeReader.h"

#include "model/GpgimProperty.h"
#include "model/GpgimVersion.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "model/XmlElementName.h"
#include "model/XmlNode.h"

#include "property-values/StructuralType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesFileIO
{
	namespace GpmlReaderUtils
	{
		struct ReaderParams;
	}

	/**
	 * Reads feature properties by referring to the GPGIM (specifically a @a GpgimProperty).
	 *
	 * The GPGIM determines the property name, allowed structural types, time-dependent types (if any)
	 * and how many times the property can appear in a feature.
	 */
	class GpmlPropertyReader :
			public GPlatesUtils::ReferenceCount<GpmlPropertyReader>
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlPropertyReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPropertyReader> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlPropertyReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPropertyReader> non_null_ptr_to_const_type;

		//! Typedef for a sequence of @a XmlNode objects.
		typedef std::list<GPlatesModel::XmlNode::non_null_ptr_type> xml_node_seq_type;


		/**
		 * Creates a @a GpmlPropertyReader from the specified GPGIM property.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_property,
				const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version)
		{
			return non_null_ptr_type(new GpmlPropertyReader(gpgim_property, property_structural_type_reader, gpml_version));
		}


		/**
		 * Returns the property name.
		 */
		const GPlatesModel::PropertyName &
		get_property_name() const
		{
			return d_gpgim_property->get_property_name();
		}


		/**
		 * Creates and reads feature properties that match the GPGIM feature property specification
		 * passed into constructor.
		 *
		 * The specified feature XML element node is searched to find all properties that match
		 * the GPGIM property's requirements.
		 *
		 * Multiple property values are possible if the GPGIM property allows a multiplicity greater than one.
		 *
		 * NOTE: A property value is generated for each property name allowed by the GPGIM.
		 * If a property name is accepted but it's structural type is rejected (by the GPGIM) then
		 * the property will be wrapped in an 'UninterpretedPropertyValue' property value.
		 * A property name that is rejected (by the GPGIM) will have no property value generated.
		 * Thus a property value is generated for each recognised property (name).
		 *
		 * XML property nodes that are processed will be removed from the unprocessed property node list.
		 * This happens even if a 'UninterpretedPropertyValue' is created for an XML node.
		 */
		void
		read_properties(
				std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &property_values,
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				xml_node_seq_type &unprocessed_feature_property_xml_nodes,
				GpmlReaderUtils::ReaderParams &reader_params) const;

	private:

		//! Associates a structural type with its structural reader function.
		struct StructuralReaderType
		{
			StructuralReaderType(
					const GPlatesPropertyValues::StructuralType &structural_type_,
					const GpmlPropertyStructuralTypeReader::structural_type_reader_function_type &structural_reader_function_);

			// NOTE: Structural type stored as XmlElementName since we use it to query XmlElementNode
			// and XmlElementNode uses XmlElementName for this.
			GPlatesModel::XmlElementName structural_type;

			GpmlPropertyStructuralTypeReader::structural_type_reader_function_type structural_reader_function;
		};

		//! Typedef for a sequence of structural reader types.
		typedef std::vector<StructuralReaderType> structural_reader_type_seq_type;


		//! The GPGIM property.
		GPlatesModel::GpgimProperty::non_null_ptr_to_const_type d_gpgim_property;

		/**
		 * Used to read property structural types from a GPML file.
		 */
		GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type d_property_structural_type_reader;

		/**
		 * The version of the GPGIM used to create the GPML file being read.
		 */
		GPlatesModel::GpgimVersion d_gpml_version;

		/**
		 * Sequence of allowed structural types (and associated reader functions).
		 *
		 * NOTE: There should be at least one structural reader type (enforced by the GPGIM property).
		 */
		structural_reader_type_seq_type d_structural_reader_types;

		//! Structural reader function for 'gpml:ConstantValue'.
		GpmlPropertyStructuralTypeReader::structural_type_reader_function_type d_constant_value_structural_reader_function;

		//! Structural reader function for 'gpml:IrregularSampling'.
		GpmlPropertyStructuralTypeReader::structural_type_reader_function_type d_irregular_sampling_structural_reader_function;

		//! Structural reader function for 'gpml:PiecewiseAggregation'.
		GpmlPropertyStructuralTypeReader::structural_type_reader_function_type d_piecewise_aggregation_structural_reader_function;


		GpmlPropertyReader(
				const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_property,
				const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version);

		/**
		 * Attempt to read the property if it matches the GPGIM.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		read_property(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params) const;

		/**
		 * Attempt to read the property structural type if it matches the GPGIM.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		read_property_structural_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
				boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> structural_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params) const;

		/**
		 * Attempt to convert an unwrapped (structural) type to a time-dependent wrapped structural type.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		convert_unwrapped_to_time_dependent_wrapped_structural_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
				boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> structural_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params) const;

		/**
		 * Attempt to convert a time-dependent wrapped structural type to an unwrapped structural type.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		convert_time_dependent_wrapped_to_unwrapped_structural_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
				const GPlatesModel::XmlElementNode::non_null_ptr_type &structural_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params) const;

		/**
		 * Returns the structural type, wrapped in time-dependent structure, if accepted by the GPGIM.
		 */
		boost::optional<GPlatesPropertyValues::StructuralType>
		get_time_dependent_wrapped_structural_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &structural_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params) const;

		/**
		 * Returns the structural reader function for the specified (non-time-dependent) structural element.
		 */
		boost::optional<GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
		get_structural_creation_function(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &structural_xml_element) const;

		/**
		 * Attempt to read (non-time-dependent) structural type with the specified structural reader function.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		read_structural_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
				const GpmlPropertyStructuralTypeReader::structural_type_reader_function_type &structural_creation_function,
				GpmlReaderUtils::ReaderParams &reader_params) const;

		/**
		 * Attempt to read (non-time-dependent) structural type that has no structural type
		 * specified in the GPML file (as a structural XML element).
		 *
		 * Also returns the index into @a d_structural_reader_types for the successful structural type.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		read_unspecified_structural_type(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params,
				boost::optional<unsigned int &> structural_creation_type_index = boost::none) const;

	};
}

#endif // GPLATES_FILE_IO_GPMLPROPERTYREADER_H
