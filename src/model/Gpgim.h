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

#ifndef GPLATES_MODEL_GPGIM_H
#define GPLATES_MODEL_GPGIM_H

#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>
#include <QXmlStreamReader>

#include "FeatureType.h"
#include "Gpgim.h"
#include "GpgimEnumerationType.h"
#include "GpgimFeatureClass.h"
#include "GpgimProperty.h"
#include "GpgimStructuralType.h"
#include "GpgimVersion.h"
#include "PropertyName.h"
#include "XmlNode.h"

#include "property-values/StructuralType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * The GPlates Geological Information Model (GPGIM) main query point.
	 *
	 * Only the current (latest) version of the GPGIM is available.
	 */
	class Gpgim :
			public GPlatesUtils::ReferenceCount<Gpgim>
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a Gpgim.
		typedef GPlatesUtils::non_null_intrusive_ptr<Gpgim> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a Gpgim.
		typedef GPlatesUtils::non_null_intrusive_ptr<const Gpgim> non_null_ptr_to_const_type;

		//! Typedef for a sequence of feature types.
		typedef std::vector<FeatureType> feature_type_seq_type;

		//! Typedef for a sequence of property structural types.
		typedef std::vector<GpgimStructuralType::non_null_ptr_to_const_type> property_structural_type_seq_type;

		//! Typedef for a sequence of property enumeration (structural) types.
		typedef std::vector<GpgimEnumerationType::non_null_ptr_to_const_type> property_enumeration_type_seq_type;

		//! Typedef for a sequence of properties.
		typedef std::vector<GpgimProperty::non_null_ptr_to_const_type> property_seq_type;


		/**
		 * The default filename for the GPGIM resource XML file.
		 *
		 * This is loaded into the GPlates executable as a Qt resource via the 'qt-resources' library.
		 */
		static const QString DEFAULT_GPGIM_RESOURCE_FILENAME;


		/**
		 * Creates a @a Gpgim from the specified GPGIM resource XML file.
		 *
		 * @throws @a ErrorOpeningFileForReadingException upon failure to open XML file for reading.
		 *
		 * @throws @a GpgimInitialisationException upon failure to properly initialise the GPGIM
		 * when reading/parsing the XML file.
		 */
		static
		non_null_ptr_type
		create(
				const QString &gpgim_resource_filename = DEFAULT_GPGIM_RESOURCE_FILENAME)
		{
			return non_null_ptr_type(new Gpgim(gpgim_resource_filename));
		}


		/**
		 * Returns the GPGIM version.
		 */
		const GpgimVersion &
		get_version() const;


		/**
		 * Returns a list of all feature types, available in the GPGIM, representing *concrete* features.
		 *
		 * Note that abstract feature types are not included.
		 * Concrete feature types are the only types ever instantiated as real features into the GPlates model.
		 * For example, 'gpml:TangibleFeature' is abstract and 'gpml:Isochron' is concrete.
		 */
		const feature_type_seq_type &
		get_concrete_feature_types() const
		{
			return d_concrete_feature_types;
		}


		/**
		 * Returns the feature class associated with the specified feature type.
		 *
		 * The feature class can represent an abstract or concrete feature.
		 * For example, 'gpml:TangibleFeature' is abstract and 'gpml:Isochron' is concrete.
		 *
		 * Returns boost::none if @a feature_type is not recognised by this GPGIM.
		 */
		boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type>
		get_feature_class(
				const FeatureType &feature_type) const;


		/**
		 * Convenience method returns the GPGIM property of the specified property name in the
		 * specified feature type.
		 *
		 * Returns boost::none if the specified feature type does not have the specified property name,
		 * or if the specified feature type is not recognised.
		 */
		boost::optional<GpgimProperty::non_null_ptr_to_const_type>
		get_feature_property(
				const FeatureType &feature_type,
				const PropertyName &property_name) const;


		/**
		 * Convenience method returns the GPGIM property(s) of the specified property type in the
		 * specified feature type.
		 *
		 * Returns false if the specified property type is not recognised for any properties
		 * of the specified feature type, or if the specified feature type is not recognised.
		 *
		 * Optionally returns the matching feature properties in @a feature_properties.
		 */
		bool
		get_feature_properties(
				const FeatureType &feature_type,
				const GPlatesPropertyValues::StructuralType &property_type,
				boost::optional<property_seq_type &> feature_properties = boost::none) const;


		/**
		 * Returns all properties supported by the GPGIM.
		 */
		const property_seq_type &
		get_properties() const
		{
			return d_properties;
		}


		/**
		 * Returns the property associated with the specified property name.
		 *
		 * Returns boost::none if @a property_name is not recognised by this GPGIM.
		 */
		boost::optional<GpgimProperty::non_null_ptr_to_const_type>
		get_property(
				const PropertyName &property_name) const;


		/**
		 * Returns all property structural types supported by the GPGIM.
		 */
		const property_structural_type_seq_type &
		get_property_structural_types() const
		{
			return d_property_structural_types;
		}


		/**
		 * Returns the property structural type associated with the specified structural type.
		 *
		 * Returns boost::none if @a structural_type is not recognised by this GPGIM.
		 *
		 * Note that the returned structural type could be an enumeration since enumerations are
		 * a subset of all property structural types.
		 */
		boost::optional<GpgimStructuralType::non_null_ptr_to_const_type>
		get_property_structural_type(
				const GPlatesPropertyValues::StructuralType &structural_type) const;


		/**
		 * Returns the subset of property structural types that are enumerations.
		 *
		 * This is a subset of the structural types returned by @a get_property_structural_types.
		 */
		const property_enumeration_type_seq_type &
		get_property_enumeration_types() const
		{
			return d_property_enumeration_types;
		}


		/**
		 * Returns the property enumeration (structural) type associated with the specified structural type.
		 *
		 * Returns boost::none if @a structural_type is not recognised as an enumeration type by this GPGIM.
		 *
		 * Note the enumerations are a subset structural types.
		 */
		boost::optional<GpgimEnumerationType::non_null_ptr_to_const_type>
		get_property_enumeration_type(
				const GPlatesPropertyValues::StructuralType &structural_type) const;

	private:

		//! Typedef for mapping from feature type to associated feature class XML element nodes.
		typedef std::map<FeatureType, XmlElementNode::non_null_ptr_type> feature_class_xml_element_node_map_type;

		//! Typedef for a map of feature type to feature class.
		typedef std::map<FeatureType, GpgimFeatureClass::non_null_ptr_to_const_type> feature_class_map_type;

		//! Typedef for a map of structural type to GPGIM structural type.
		typedef std::map<
				GPlatesPropertyValues::StructuralType,
				GpgimStructuralType::non_null_ptr_to_const_type>
						property_structural_type_map_type;

		//! Typedef for a map of enumeration (structural) type to GPGIM structural type.
		typedef std::map<
				GPlatesPropertyValues::StructuralType,
				GpgimEnumerationType::non_null_ptr_to_const_type>
						property_enumeration_type_map_type;

		//! Typedef for a map of property name to GPGIM property.
		typedef std::map<
				PropertyName,
				GpgimProperty::non_null_ptr_to_const_type>
						property_map_type;


		/**
		 * The GPGIM version.
		 *
		 * This is only optional because it is not known at the time of construction.
		 * But it is always valid (otherwise 'this' object will not exist).
		 */
		boost::optional<GpgimVersion> d_version;

		/**
		 * The list of all supported property structural types.
		 */
		property_structural_type_seq_type d_property_structural_types;

		/**
		 * Used to retrieve GPGIM structural type from structural type.
		 */
		property_structural_type_map_type d_property_structural_type_map;

		/**
		 * The list of all supported property *enumeration* types.
		 *
		 * This is a subset of @a d_property_structural_types.
		 */
		property_enumeration_type_seq_type d_property_enumeration_types;

		/**
		 * Used to retrieve GPGIM enumeration (structural) type from structural type.
		 */
		property_enumeration_type_map_type d_property_enumeration_type_map;

		/**
		 * The list of all supported properties.
		 */
		property_seq_type d_properties;

		/**
		 * Used to retrieve GPGIM property from property name.
		 */
		property_map_type d_property_map;

		/**
		 * Used to retrieve feature class from feature type.
		 */
		feature_class_map_type d_feature_class_map;

		/**
		 * Those subset of feature types that are concrete (not abstract).
		 */
		feature_type_seq_type d_concrete_feature_types;


		Gpgim(
				const QString &gpgim_resource_filename);

		/**
		 * Reads the root 'gpgim:GPGIM' element in the GPGIM XML document and returns the GPGIM version.
		 */
		GpgimVersion
		read_gpgim_element(
				boost::optional<XmlElementNode::non_null_ptr_type> &property_type_list_xml_element,
				boost::optional<XmlElementNode::non_null_ptr_type> &property_list_xml_element,
				boost::optional<XmlElementNode::non_null_ptr_type> &feature_class_list_xml_element,
				QXmlStreamReader &xml_reader,
				const QString &gpgim_resource_filename,
				boost::shared_ptr<XmlElementNode::AliasToNamespaceMap> alias_to_namespace_map);

		/**
		 * Compiles the property structural type definitions from their respective XML element nodes.
		 */
		void
		create_property_structural_types(
				const XmlElementNode::non_null_ptr_type &property_type_list_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Compiles a property structural type from the specified XML element node.
		 */
		GpgimStructuralType::non_null_ptr_to_const_type
		create_property_structural_type(
				const XmlElementNode::non_null_ptr_type &property_type_xml_element,
				bool is_enumeration,
				const QString &gpgim_resource_filename);

		/**
		 * Create the GPGIM feature properties listed in (children of) the specified XML element node.
		 */
		void
		create_properties(
				const XmlElementNode::non_null_ptr_type &property_list_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Create the GPGIM feature property associated with the specified XML element node.
		 */
		GpgimProperty::non_null_ptr_type
		create_property(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the feature property name from the specified property XML element.
		 */
		PropertyName
		read_feature_property_name(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the feature property user-friendly name from the specified property XML element.
		 */
		QString
		read_feature_property_user_friendly_name(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const PropertyName &property_name,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the feature property description from the specified property XML element.
		 */
		QString
		read_feature_property_description(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the feature property multiplicity from the specified property XML element.
		 */
		GpgimProperty::MultiplicityType
		read_feature_property_multiplicity(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the feature property structural types from the specified property XML element.
		 *
		 * Returns the default structural type as an iterator to the sequence @a gpgim_property_structural_types.
		 */
		GpgimProperty::structural_type_seq_type::size_type
		read_feature_property_structural_types(
				GpgimProperty::structural_type_seq_type &gpgim_property_structural_types,
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the default feature property structural type as an attribute of the specified property XML element.
		 */
		GPlatesPropertyValues::StructuralType
		read_default_feature_property_structural_type(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the feature property time-dependent types from the specified property XML element.
		 */
		GpgimProperty::time_dependent_flags_type
		read_feature_property_time_dependent_types(
				const XmlElementNode::non_null_ptr_type &property_xml_element,
				const QString &gpgim_resource_filename);

		boost::optional<GPlatesModel::PropertyName>
		read_feature_class_default_geometry_property_name(
				const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Reads the GPGIM feature class definitions in the GPGIM XML document.
		 */
		void
		read_feature_class_xml_elements(
				feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
				const XmlElementNode::non_null_ptr_type &feature_class_list_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Compiles the feature class definitions from their respective XML element nodes.
		 */
		void
		create_feature_classes(
				const feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
				const QString &gpgim_resource_filename);

		/**
		 * Creates the special-case feature class 'gpml:UnclassifiedFeature'.
		 */
		GpgimFeatureClass::non_null_ptr_to_const_type
		create_unclassified_feature_class(
				const QString &gpgim_resource_filename);

		/**
		 * Compiles a feature class definition from the specified XML element node, if it hasn't already been.
		 */
		GpgimFeatureClass::non_null_ptr_to_const_type
		create_feature_class_if_necessary(
				const FeatureType &feature_type,
				const XmlElementNode::non_null_ptr_type &feature_class_reference_xml_element,
				const feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
				const QString &gpgim_resource_filename);

		/**
		 * Compiles a feature class definition from the specified XML element node.
		 */
		GpgimFeatureClass::non_null_ptr_to_const_type
		create_feature_class(
				const FeatureType &feature_type,
				const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
				const feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
				const QString &gpgim_resource_filename);

		/**
		 * Create GPGIM feature properties for the feature class associated with the specified XML element node.
		 */
		void
		create_feature_properties(
				GpgimFeatureClass::gpgim_property_seq_type &gpgim_feature_properties,
				const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
				const QString &gpgim_resource_filename);

		/**
		 * Returns true if the feature class (associated with the specified XML element) is concrete.
		 */
		bool
		is_concrete_feature_class(
				const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
				const QString &gpgim_resource_filename) const;
	};
}

#endif // GPLATES_MODEL_GPGIM_H
