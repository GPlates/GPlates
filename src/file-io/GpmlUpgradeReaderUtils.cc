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

#include <vector>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "GpmlUpgradeReaderUtils.h"

#include "GpmlReaderUtils.h"
#include "GpmlStructuralTypeReaderUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/Gpgim.h"
#include "model/GpgimProperty.h"
#include "model/GpgimStructuralType.h"
#include "model/ModelUtils.h"
#include "model/PropertyValue.h"
#include "model/XmlNodeUtils.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/OldVersionPropertyValue.h"
#include "property-values/StructuralType.h"

namespace GPlatesFileIO
{
	namespace
	{
		//! Typedef for a sequence of topological sections.
		typedef std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
				topological_sections_seq_type;

		/**
		 * Reads a list of topological sections from an old version 'gpml:TopologicalPolygon' or
		 * 'gpml:TopologicalInterior'.
		 */
		GPlatesPropertyValues::OldVersionPropertyValue::non_null_ptr_type
		create_topological_section_list(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &parent,
				const GPlatesPropertyValues::StructuralType &structural_type,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors)
		{
			static const GPlatesModel::XmlElementName SECTION =
					GPlatesModel::XmlElementName::create_gpml("section");

			GPlatesModel::XmlElementNode::non_null_ptr_type
				elem = GpmlStructuralTypeReaderUtils::get_structural_type_element(
						parent,
						GPlatesModel::XmlElementName(structural_type));

			std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> sections;

			GpmlStructuralTypeReaderUtils::find_and_create_one_or_more(
					elem, &GpmlStructuralTypeReaderUtils::create_gpml_topological_section,
					SECTION, sections, gpml_version, read_errors);

			return GPlatesPropertyValues::OldVersionPropertyValue::create(
					structural_type,
					GPlatesPropertyValues::OldVersionPropertyValue::value_type(sections));
		}


		void
		append_reader_errors(
				GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code,
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params)
		{
			if (error_code == GPlatesModel::ModelUtils::TopLevelPropertyError::PROPERTY_NAME_NOT_RECOGNISED)
			{
				// The new property name is not allowed, by the GPGIM, for the feature type.
				append_warning(feature_xml_element, reader_params,
						GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
						GPlatesFileIO::ReadErrors::ElementNotNameChanged);
			}
			else if (error_code == GPlatesModel::ModelUtils::TopLevelPropertyError::PROPERTY_NAME_NOT_SUPPORTED_BY_FEATURE_TYPE)
			{
				append_warning(feature_xml_element, reader_params,
						GPlatesFileIO::ReadErrors::PropertyNameNotRecognisedInFeatureType,
						GPlatesFileIO::ReadErrors::ElementNotNameChanged);
			}
			else if (error_code == GPlatesModel::ModelUtils::TopLevelPropertyError::COULD_NOT_WRAP_INTO_A_TIME_DEPENDENT_PROPERTY)
			{
				append_warning(feature_xml_element, reader_params,
						GPlatesFileIO::ReadErrors::TimeDependentPropertyStructuralElementNotFound,
						GPlatesFileIO::ReadErrors::ElementNotNameChanged);
			}
			else if (error_code == GPlatesModel::ModelUtils::TopLevelPropertyError::COULD_NOT_UNWRAP_EXISTING_TIME_DEPENDENT_PROPERTY)
			{
				append_warning(feature_xml_element, reader_params,
						GPlatesFileIO::ReadErrors::TimeDependentPropertyStructuralElementFound,
						GPlatesFileIO::ReadErrors::ElementNotNameChanged);
			}
			else if (error_code == GPlatesModel::ModelUtils::TopLevelPropertyError::COULD_NOT_CONVERT_FROM_ONE_TIME_DEPENDENT_WRAPPER_TO_ANOTHER)
			{
				append_warning(feature_xml_element, reader_params,
						GPlatesFileIO::ReadErrors::IncorrectTimeDependentPropertyStructuralElementFound,
						GPlatesFileIO::ReadErrors::ElementNotNameChanged);
			}
			else
			{
				// We won't generate a read error warning for the other errors since they are top-level
				// structural errors that should never occur.
				qWarning() << "Top-level property is not inline or does not contain exactly one property value.";
			}
		}


		/**
		 * Find a @a OldVersionPropertyValue property value given a @a PropertyValue.
		 *
		 * This is used instead of the general-purpose 'get_property_value()' function in
		 * "PropertyValueFind.h" because the property value could be a piecewise aggregation
		 * with a limited time range. There will only be one time window but we don't know what
		 * reconstruction time to specify to get that window. Instead we just search for the
		 * first time window using this class.
		 */
		class OldVersionPropertyValueFinder:
				public GPlatesModel::ConstFeatureVisitor
		{
		public:

			boost::optional<const GPlatesPropertyValues::OldVersionPropertyValue &>
			get_old_version_property_value(
					const GPlatesModel::PropertyValue &property_value)
			{
				d_old_version_property_value = boost::none;

				property_value.accept_visitor(*this);

				return d_old_version_property_value;
			}

		private:

			virtual
			void
			visit_gpml_constant_value(
					const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

			virtual
			void
			visit_gpml_piecewise_aggregation(
					const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
			{
				if (gpml_piecewise_aggregation.get_time_windows().empty())
				{
					return;
				}

				// Just visit the first time window - there should only be one window.
				gpml_piecewise_aggregation.get_time_windows().front().get_time_dependent_value()->accept_visitor(*this);
			}

			virtual
			void
			visit_old_version_property_value(
					const GPlatesPropertyValues::OldVersionPropertyValue &old_version_prop_value)
			{
				d_old_version_property_value = old_version_prop_value;
			}

		private:

			boost::optional<const GPlatesPropertyValues::OldVersionPropertyValue &> d_old_version_property_value;
		};
	}
}


GPlatesFileIO::GpmlUpgradeReaderUtils::PropertyRename::PropertyRename(
		const GPlatesModel::PropertyName &old_property_name_,
		const GPlatesModel::PropertyName &new_property_name_) :
	old_property_name(old_property_name_),
	new_property_name(new_property_name_)
{
}


GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
GPlatesFileIO::GpmlUpgradeReaderUtils::rename_gpgim_feature_class_properties(
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &original_gpgim_feature_class,
		const std::vector<PropertyRename> &property_renames)
{
	//
	// Copy the GPGIM feature class but change the GPGIM properties with matching property names.
	//

	// Get the GPGIM feature properties associated with our feature class (and not its ancestors).
	// The ancestor properties are taken care of by the parent feature class.
	GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type gpgim_feature_properties =
			original_gpgim_feature_class.get()->get_feature_properties_excluding_ancestor_classes();

	// Find GPGIM feature property(s) with matching property name(s).
	GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type::iterator gpgim_feature_property_iter =
			gpgim_feature_properties.begin();
	const GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type::iterator gpgim_feature_property_end =
			gpgim_feature_properties.end();
	for ( ; gpgim_feature_property_iter != gpgim_feature_property_end; ++gpgim_feature_property_iter)
	{
		const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property =
				*gpgim_feature_property_iter;

		// If we've found a matching property name then rename the property so that we read the old property name.
		BOOST_FOREACH(const PropertyRename &property_rename, property_renames)
		{
			if (gpgim_feature_property->get_property_name() == property_rename.new_property_name)
			{
				// Replace the GPGIM property, in the sequence, with a cloned version
				// (but with the old property name).
				GPlatesModel::GpgimProperty::non_null_ptr_type old_gpgim_feature_property =
						gpgim_feature_property->clone();
				old_gpgim_feature_property->set_property_name(property_rename.old_property_name);
				*gpgim_feature_property_iter = old_gpgim_feature_property;
				break;
			}
		}
	}

	// Create the GPGIM feature class with the old-name GPGIM property(s).
	return GPlatesModel::GpgimFeatureClass::create(
			original_gpgim_feature_class.get()->get_feature_type(),
			original_gpgim_feature_class.get()->get_feature_description(),
			gpgim_feature_properties.begin(),
			gpgim_feature_properties.end(),
			original_gpgim_feature_class.get()->get_parent_feature_class());
}


GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type
GPlatesFileIO::GpmlUpgradeReaderUtils::create_property_rename_feature_reader_impl(
		const GpmlFeatureReaderImpl::non_null_ptr_type &parent_feature_reader_impl,
		const std::vector<PropertyRename> &property_renames,
		const GPlatesModel::Gpgim::non_null_ptr_to_const_type &gpgim)
{
	GpmlFeatureReaderImpl::non_null_ptr_type feature_reader_impl = parent_feature_reader_impl;

	// For each property rename, chain a new rename property feature reader impl into the list of readers.
	BOOST_FOREACH(const PropertyRename &property_rename, property_renames)
	{
		// Create a feature reader impl that renames the current old property name to the new one.
		// This also builds a chain of feature readers - one link for each property rename.
		feature_reader_impl = RenamePropertyFeatureReaderImpl::create(
				property_rename.old_property_name,
				property_rename.new_property_name,
				feature_reader_impl,
				gpgim);
	}

	return feature_reader_impl;
}


GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
GPlatesFileIO::GpmlUpgradeReaderUtils::remove_gpgim_feature_class_properties(
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &original_gpgim_feature_class,
		const std::vector<GPlatesModel::PropertyName> &property_names)
{
	//
	// Copy the GPGIM feature class but remove the GPGIM properties with matching property names.
	//

	// Get the GPGIM feature properties associated with our feature class (and not its ancestors).
	// The ancestor properties are taken care of by the parent feature class.
	GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type gpgim_feature_properties =
			original_gpgim_feature_class.get()->get_feature_properties_excluding_ancestor_classes();

	// Find GPGIM feature property(s) with matching property name(s).
	GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type::iterator gpgim_feature_property_iter =
			gpgim_feature_properties.begin();
	while (gpgim_feature_property_iter !=
		// NOTE: Don't cache this in a variable - we're iterating over a std::vector and calling 'erase()'...
		gpgim_feature_properties.end())
	{
		const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property =
				*gpgim_feature_property_iter;

		// If we've found a matching property name then remove the GPGIM property.
		bool removed_gpgim_property = false;
		BOOST_FOREACH(const GPlatesModel::PropertyName &property_name, property_names)
		{
			if (gpgim_feature_property->get_property_name() == property_name)
			{
				gpgim_feature_property_iter = gpgim_feature_properties.erase(gpgim_feature_property_iter);
				removed_gpgim_property = true;
				break;
			}
		}

		if (!removed_gpgim_property)
		{
			++gpgim_feature_property_iter;
		}
	}

	// Create the GPGIM feature class with the removed GPGIM property(s).
	return GPlatesModel::GpgimFeatureClass::create(
			original_gpgim_feature_class.get()->get_feature_type(),
			original_gpgim_feature_class.get()->get_feature_description(),
			gpgim_feature_properties.begin(),
			gpgim_feature_properties.end(),
			original_gpgim_feature_class.get()->get_parent_feature_class());
}


GPlatesFileIO::GpmlUpgradeReaderUtils::RenamePropertyFeatureReaderImpl::RenamePropertyFeatureReaderImpl(
		const GPlatesModel::PropertyName &from_property_name,
		const GPlatesModel::PropertyName &to_property_name,
		const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader,
		const GPlatesModel::Gpgim::non_null_ptr_to_const_type &gpgim) :
	d_gpgim(gpgim),
	d_feature_reader(feature_reader),
	d_from_property_name(from_property_name),
	d_to_property_name(to_property_name)
{
}


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlUpgradeReaderUtils::RenamePropertyFeatureReaderImpl::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Read the feature.
	GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			d_feature_reader->read_feature(
					feature_xml_element,
					unprocessed_feature_property_xml_nodes,
					reader_params);

	// Rename all properties matching our property name.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;
	if (!GPlatesModel::ModelUtils::rename_feature_properties(
			*feature,
			d_from_property_name,
			d_to_property_name,
			*d_gpgim,
			true/*check_new_property_name_allowed_for_feature_type*/,
			&error_code))
	{
		append_reader_errors(error_code, feature_xml_element, reader_params);
	}

	return feature;
}


GPlatesFileIO::GpmlUpgradeReaderUtils::ChangeFeatureTypeFeatureReaderImpl::ChangeFeatureTypeFeatureReaderImpl(
		const GPlatesModel::FeatureType &new_feature_type,
		const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader,
		const GPlatesModel::Gpgim::non_null_ptr_to_const_type &gpgim) :
	d_gpgim(gpgim),
	d_feature_reader(feature_reader),
	d_new_feature_type(new_feature_type)
{
}


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlUpgradeReaderUtils::ChangeFeatureTypeFeatureReaderImpl::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Read the feature.
	GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			d_feature_reader->read_feature(
					feature_xml_element,
					unprocessed_feature_property_xml_nodes,
					reader_params);

	// Change the feature type.
	feature->set_feature_type(d_new_feature_type);

	return feature;
}


boost::optional<GPlatesFileIO::GpmlUpgradeReaderUtils::TopologicalNetworkFeatureReaderUpgrade_1_6_319::non_null_ptr_type>
GPlatesFileIO::GpmlUpgradeReaderUtils::TopologicalNetworkFeatureReaderUpgrade_1_6_319::create(
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &original_gpgim_feature_class,
		const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &parent_feature_reader,
		const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
		const GPlatesModel::Gpgim::non_null_ptr_to_const_type &gpgim,
		const GPlatesModel::GpgimVersion &gpml_version)
{
	//
	// Find the 'gpml:network' property name or whatever it currently is in the GPGIM.
	//

	boost::optional<GPlatesModel::PropertyName> network_property_name;

	// Search over the structural types to find a 'gpml:TopologicalNetwork' property type.
	// We could have searched for a 'gpml:network' property name instead but property names
	// are far more likely to change across GPGIM revisions than property types.
	const GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type &original_gpgim_feature_properties =
			original_gpgim_feature_class.get()->get_feature_properties_excluding_ancestor_classes();
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &original_gpgim_feature_property,
			original_gpgim_feature_properties)
	{
		static const GPlatesPropertyValues::StructuralType NETWORK_PROPERTY_TYPE =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");

		if (original_gpgim_feature_property->get_default_structural_type()->get_structural_type() ==
			NETWORK_PROPERTY_TYPE)
		{
			network_property_name = original_gpgim_feature_property->get_property_name();
			break;
		}
	}

	if (!network_property_name)
	{
		return boost::none;
	}

	//
	// Create a new parent feature reader, minus the network property.
	//
	// NOTE: We remove the network property from the parent reader because we are going
	// to handle creation of a network property by combining a 'gpml:boundary' property and
	// a 'gpml:interior' property.

	// Use the feature class, minus the network property...
	GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type gpgim_feature_class = 
			GpmlUpgradeReaderUtils::remove_gpgim_feature_class_properties(
					original_gpgim_feature_class,
					std::vector<GPlatesModel::PropertyName>(1, network_property_name.get()));

	// Create a new feature reader, minus the network property.
	const GpmlFeatureReaderImpl::non_null_ptr_to_const_type feature_reader =
			GpmlFeatureReader::create(
					gpgim_feature_class,
					parent_feature_reader,
					property_structural_type_reader,
					gpml_version);

	//
	// Create a structural type reader for the old 'gpml:boundary' and 'gpml:interior' properties.
	//

	static const GPlatesModel::PropertyName BOUNDARY_PROPERTY_NAME =
			GPlatesModel::PropertyName::create_gpml("boundary");
	static const GPlatesPropertyValues::StructuralType BOUNDARY_PROPERTY_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");

	static const GPlatesModel::PropertyName INTERIOR_PROPERTY_NAME =
			GPlatesModel::PropertyName::create_gpml("interior");
	static const GPlatesPropertyValues::StructuralType INTERIOR_PROPERTY_TYPE =
			GPlatesPropertyValues::StructuralType::create_gpml("TopologicalInterior");

	// We only need to add the property structural types we expect to encounter.
	GpmlPropertyStructuralTypeReader::non_null_ptr_type old_version_property_structural_type_reader =
			GpmlPropertyStructuralTypeReader::create_empty(gpgim);

	old_version_property_structural_type_reader->add_time_dependent_wrapper_structural_types();

	// Add our specialised reader function for old version type 'gpml:TopologicalPolygon'.
	old_version_property_structural_type_reader->add_structural_type(
			BOUNDARY_PROPERTY_TYPE,
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&create_topological_section_list, _1, BOUNDARY_PROPERTY_TYPE, _2, _3));

	// Add our specialised reader function for old version type 'gpml:TopologicalInterior'.
	old_version_property_structural_type_reader->add_structural_type(
			INTERIOR_PROPERTY_TYPE,
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&create_topological_section_list, _1, INTERIOR_PROPERTY_TYPE, _2, _3));

	//
	// Create a property reader for the 'gpml:boundary' property.
	//

	// We're expecting a piecewise-aggregation but throw the constant-value in just in case.
	GPlatesModel::GpgimProperty::time_dependent_flags_type gpgim_time_dependent_flags;
	gpgim_time_dependent_flags.set(GPlatesModel::GpgimProperty::CONSTANT_VALUE);
	gpgim_time_dependent_flags.set(GPlatesModel::GpgimProperty::PIECEWISE_AGGREGATION);

	GPlatesModel::GpgimProperty::non_null_ptr_to_const_type boundary_gpgim_property =
			GPlatesModel::GpgimProperty::create(
					BOUNDARY_PROPERTY_NAME,
					GPlatesUtils::make_qstring_from_icu_string(BOUNDARY_PROPERTY_NAME.get_name()),
					"",
					GPlatesModel::GpgimProperty::ONE, // Exactly one property must be present.
					GPlatesModel::GpgimStructuralType::create(BOUNDARY_PROPERTY_TYPE, ""),
					gpgim_time_dependent_flags);

	// Create a GPML property reader based on the GPGIM property just created.
	const GpmlPropertyReader::non_null_ptr_to_const_type boundary_property_reader =
			GpmlPropertyReader::create(
					boundary_gpgim_property,
					old_version_property_structural_type_reader,
					gpml_version);

	//
	// Create a property reader for the 'gpml:interior' property.
	//

	GPlatesModel::GpgimProperty::non_null_ptr_to_const_type interior_gpgim_property =
			GPlatesModel::GpgimProperty::create(
					INTERIOR_PROPERTY_NAME,
					GPlatesUtils::make_qstring_from_icu_string(INTERIOR_PROPERTY_NAME.get_name()),
					"",
					GPlatesModel::GpgimProperty::ZERO_OR_ONE, // The property is optional.
					GPlatesModel::GpgimStructuralType::create(INTERIOR_PROPERTY_TYPE, ""),
					gpgim_time_dependent_flags);

	// Create a GPML property reader based on the GPGIM property just created.
	const GpmlPropertyReader::non_null_ptr_to_const_type interior_property_reader =
			GpmlPropertyReader::create(
					interior_gpgim_property,
					old_version_property_structural_type_reader,
					gpml_version);

	return non_null_ptr_type(
			new TopologicalNetworkFeatureReaderUpgrade_1_6_319(
					feature_reader,
					boundary_property_reader,
					interior_property_reader,
					network_property_name.get(),
					gpgim));
}


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlUpgradeReaderUtils::TopologicalNetworkFeatureReaderUpgrade_1_6_319::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Read the feature - minus the network property.
	GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			d_parent_feature_reader->read_feature(
					feature_xml_element,
					unprocessed_feature_property_xml_nodes,
					reader_params);

	//
	// Read the 'gpml:boundary' property.
	//

	boost::optional<topological_sections_seq_type> boundary_topological_sections;

	// Read the 'gpml:boundary' property into a 'gpml:OldVersionPropertyValue'.
	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> boundary_property_values;
	d_boundary_property_reader->read_properties(
			boundary_property_values,
			feature_xml_element,
			unprocessed_feature_property_xml_nodes,
			reader_params);

	// Our GPGIM property specifies exactly one occurrence of 'gpml:boundary'.
	if (boundary_property_values.size() == 1)
	{
		// Make sure it's an 'OldVersionPropertyValue' as expected rather than 'UninterpretedPropertyValue's.
		OldVersionPropertyValueFinder old_version_property_value_finder;
		boost::optional<const GPlatesPropertyValues::OldVersionPropertyValue &> old_version_property_value =
				old_version_property_value_finder.get_old_version_property_value(*boundary_property_values.front());
		if (old_version_property_value)
		{
			// Retrieve the topological sections from the old version property value.
			boundary_topological_sections =
					boost::any_cast<const topological_sections_seq_type &>(
							old_version_property_value->get_value());
		}
	}

	//
	// Read the 'gpml:interior' property.
	//

	//! Typedef for a sequence of topological interiors.
	typedef std::vector<GPlatesPropertyValues::GpmlTopologicalNetwork::Interior>
			topological_interiors_seq_type;

	boost::optional<topological_interiors_seq_type> topological_interiors;

	// Read the 'gpml:interior' property into a 'gpml:OldVersionPropertyValue'.
	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> interior_property_values;
	d_interior_property_reader->read_properties(
			interior_property_values,
			feature_xml_element,
			unprocessed_feature_property_xml_nodes,
			reader_params);

	// Our GPGIM property specifies zero or one occurrence of 'gpml:interior'.
	if (interior_property_values.size() == 1)
	{
		// Make sure it's an 'OldVersionPropertyValue' as expected rather than 'UninterpretedPropertyValue's.
		OldVersionPropertyValueFinder old_version_property_value_finder;
		boost::optional<const GPlatesPropertyValues::OldVersionPropertyValue &> old_version_property_value =
				old_version_property_value_finder.get_old_version_property_value(*interior_property_values.front());
		if (old_version_property_value)
		{
			// Retrieve the topological sections from the old version property value.
			const topological_sections_seq_type &interior_topological_sections =
					boost::any_cast<const topological_sections_seq_type &>(
							old_version_property_value->get_value());

			// Convert the topological sections to topological interiors by only retaining the
			// source geometry property delegate - the other section information was never needed.
			topological_interiors = topological_interiors_seq_type();
			BOOST_FOREACH(
					const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type &interior_topological_section,
					interior_topological_sections)
			{
				const GPlatesPropertyValues::GpmlTopologicalLineSection *topological_line_section = NULL;
				const GPlatesPropertyValues::GpmlTopologicalPoint *topological_point = NULL;
				if (GPlatesFeatureVisitors::get_property_value(*interior_topological_section, topological_line_section))
				{
					topological_interiors->push_back(
							GPlatesPropertyValues::GpmlTopologicalNetwork::Interior(
									topological_line_section->source_geometry()->clone()));
				}
				else if (GPlatesFeatureVisitors::get_property_value(*interior_topological_section, topological_point))
				{
					topological_interiors->push_back(
							GPlatesPropertyValues::GpmlTopologicalNetwork::Interior(
									topological_point->source_geometry()->clone()));
				}
			}
		}
	}


	//
	// Combine a 'gpml:boundary' property and a 'gpml:interior' property into a network property.
	//

	// We need at least a boundary topological polygon to create a topological network.
	if (!boundary_topological_sections)
	{
		return feature;
	}

	// Create the network property and add it to the feature.
	GPlatesModel::PropertyValue::non_null_ptr_type network_property_value =
			topological_interiors
			? GPlatesPropertyValues::GpmlTopologicalNetwork::create(
					boundary_topological_sections->begin(),
					boundary_topological_sections->end(),
					topological_interiors->begin(),
					topological_interiors->end())
			: GPlatesPropertyValues::GpmlTopologicalNetwork::create(
					boundary_topological_sections->begin(),
					boundary_topological_sections->end());

	//
	// Wrap the network property in a time-dependent wrapper if necessary.
	//

	GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
	if (!GPlatesModel::ModelUtils::add_property(
		feature->reference(),
		d_network_property_name,
		network_property_value,
		*d_gpgim,
		true/*check_property_name_allowed_for_feature_type*/,
		&add_property_error_code))
	{
		append_reader_errors(add_property_error_code, feature_xml_element, reader_params);
	}

	return feature;
}
