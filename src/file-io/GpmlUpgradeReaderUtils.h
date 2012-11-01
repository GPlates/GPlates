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

#ifndef GPLATES_FILE_IO_GPMLUPGRADEREADERUTILS_H
#define GPLATES_FILE_IO_GPMLUPGRADEREADERUTILS_H

#include <vector>
#include <boost/optional.hpp>

#include "GpmlFeatureReaderImpl.h"
#include "GpmlPropertyReader.h"
#include "GpmlPropertyStructuralTypeReader.h"

#include "global/NotYetImplementedException.h"

#include "model/GpgimFeatureClass.h"
#include "model/GpgimProperty.h"
#include "model/GpgimVersion.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesFileIO
{
	namespace GpmlUpgradeReaderUtils
	{
		/**
		 * Structure used when renaming a GPGIM property.
		 */
		struct PropertyRename
		{
			PropertyRename(
					const GPlatesModel::PropertyName &old_property_name_,
					const GPlatesModel::PropertyName &new_property_name_);

			GPlatesModel::PropertyName old_property_name;
			GPlatesModel::PropertyName new_property_name;
		};


		/**
		 * Copy the specified GPGIM feature class, but change the specified property names.
		 *
		 * This enables reading of an old version GPML file where the property names correspond
		 * to the *new* property names.
		 */
		GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
		rename_gpgim_feature_class_properties(
				const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &feature_class,
				const std::vector<PropertyRename> &property_renames);


		/**
		 * Creates a feature reader impl that reads a feature using @a feature_read_impl and then
		 * renames feature properties with matching property names to the *new* property names.
		 */
		GpmlFeatureReaderImpl::non_null_ptr_type
		create_property_rename_feature_reader_impl(
				const GpmlFeatureReaderImpl::non_null_ptr_type &feature_reader_impl,
				const std::vector<PropertyRename> &property_renames,
				const GPlatesModel::Gpgim &gpgim);


		/**
		 * Copy the specified GPGIM feature class, but remove GPGIM properties matching
		 * the specified property names.
		 */
		GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
		remove_gpgim_feature_class_properties(
				const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &feature_class,
				const std::vector<GPlatesModel::PropertyName> &property_names);


		/**
		 * A feature reader that delegates feature reading to another reader and then renames
		 * properties, in the read feature, matching a specified property name.
		 *
		 * This is useful when a property of a feature type has been renamed in the GPGIM and an
		 * older version GPML file is being read in (and hence needs to have its property(s) renamed).
		 */
		class RenamePropertyFeatureReaderImpl :
				public GpmlFeatureReaderImpl
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a RenamePropertyFeatureReaderImpl.
			typedef GPlatesUtils::non_null_intrusive_ptr<RenamePropertyFeatureReaderImpl> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a RenamePropertyFeatureReaderImpl.
			typedef GPlatesUtils::non_null_intrusive_ptr<const RenamePropertyFeatureReaderImpl> non_null_ptr_to_const_type;


			/**
			 * Creates a @a RenamePropertyFeatureReaderImpl.
			 *
			 * Properties of features created by @a feature_reader, with property names matching
			 * @a from_property_name, are renamed to @a to_property_name.
			 */
			static
			non_null_ptr_type
			create(
					const GPlatesModel::PropertyName &from_property_name,
					const GPlatesModel::PropertyName &to_property_name,
					const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader,
					const GPlatesModel::Gpgim &gpgim)
			{
				return non_null_ptr_type(
						new RenamePropertyFeatureReaderImpl(
								from_property_name,
								to_property_name,
								feature_reader,
								gpgim));
			}


			virtual
			GPlatesModel::FeatureHandle::non_null_ptr_type
			read_feature(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
					xml_node_seq_type &unprocessed_feature_property_xml_nodes,
					GpmlReaderUtils::ReaderParams &reader_params) const;

		private:

			const GPlatesModel::Gpgim &d_gpgim;

			/**
			 * The feature reader that we delegate all property reading to.
			 */
			GpmlFeatureReaderImpl::non_null_ptr_to_const_type d_feature_reader;

			GPlatesModel::PropertyName d_from_property_name;
			GPlatesModel::PropertyName d_to_property_name;


			RenamePropertyFeatureReaderImpl(
					const GPlatesModel::PropertyName &from_property_name,
					const GPlatesModel::PropertyName &to_property_name,
					const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader,
					const GPlatesModel::Gpgim &gpgim);

		};


		/**
		 * A feature reader that delegates feature reading to another reader and then changes the feature type.
		 *
		 * This is useful when a feature type has been renamed in the GPGIM and an older version
		 * GPML file is being read in (and hence needs to have its feature type changed).
		 */
		class ChangeFeatureTypeFeatureReaderImpl :
				public GpmlFeatureReaderImpl
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a ChangeFeatureTypeFeatureReaderImpl.
			typedef GPlatesUtils::non_null_intrusive_ptr<ChangeFeatureTypeFeatureReaderImpl> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a ChangeFeatureTypeFeatureReaderImpl.
			typedef GPlatesUtils::non_null_intrusive_ptr<const ChangeFeatureTypeFeatureReaderImpl> non_null_ptr_to_const_type;


			/**
			 * Creates a @a ChangeFeatureTypeFeatureReaderImpl.
			 */
			static
			non_null_ptr_type
			create(
					const GPlatesModel::FeatureType &new_feature_type,
					const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader,
					const GPlatesModel::Gpgim &gpgim)
			{
				return non_null_ptr_type(
						new ChangeFeatureTypeFeatureReaderImpl(
								new_feature_type,
								feature_reader,
								gpgim));
			}


			virtual
			GPlatesModel::FeatureHandle::non_null_ptr_type
			read_feature(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
					xml_node_seq_type &unprocessed_feature_property_xml_nodes,
					GpmlReaderUtils::ReaderParams &reader_params) const;

		private:

			const GPlatesModel::Gpgim &d_gpgim;

			/**
			 * The feature reader that we delegate all property reading to.
			 */
			GpmlFeatureReaderImpl::non_null_ptr_to_const_type d_feature_reader;

			GPlatesModel::FeatureType d_new_feature_type;


			ChangeFeatureTypeFeatureReaderImpl(
					const GPlatesModel::FeatureType &new_feature_type,
					const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader,
					const GPlatesModel::Gpgim &gpgim);

		};


		/**
		 * This feature reader handles changes to 'gpml:TopologicalNetwork' made in GPGIM version 1.6.319.
		 *
		 * Combines a single 'gpml:boundary' and multiple 'gpml:interior' properties into single network property.
		 */
		class TopologicalNetworkFeatureReaderUpgrade_1_6_319 :
				public GpmlFeatureReaderImpl
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a TopologicalNetworkFeatureReaderUpgrade_1_6_319.
			typedef GPlatesUtils::non_null_intrusive_ptr<TopologicalNetworkFeatureReaderUpgrade_1_6_319> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a TopologicalNetworkFeatureReaderUpgrade_1_6_319.
			typedef GPlatesUtils::non_null_intrusive_ptr<const TopologicalNetworkFeatureReaderUpgrade_1_6_319> non_null_ptr_to_const_type;


			static
			boost::optional<non_null_ptr_type>
			create(
					const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class,
					const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &parent_feature_reader,
					const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
					const GPlatesModel::Gpgim &gpgim,
					const GPlatesModel::GpgimVersion &gpml_version);


			virtual
			GPlatesModel::FeatureHandle::non_null_ptr_type
			read_feature(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
					xml_node_seq_type &unprocessed_feature_property_xml_nodes,
					GpmlReaderUtils::ReaderParams &reader_params) const;

		private:

			const GPlatesModel::Gpgim &d_gpgim;

			//! The feature reader associated with the parent GPGIM feature class.
			GpmlFeatureReaderImpl::non_null_ptr_to_const_type d_parent_feature_reader;

			//! Reads the 'gpml:boundary' property.
			GpmlPropertyReader::non_null_ptr_to_const_type d_boundary_property_reader;

			//! Reads the 'gpml:interior' property.
			GpmlPropertyReader::non_null_ptr_to_const_type d_interior_property_reader;

			//! The network property name or whatever it currently is in the GPGIM.
			GPlatesModel::PropertyName d_network_property_name;


			TopologicalNetworkFeatureReaderUpgrade_1_6_319(
					const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &parent_feature_reader,
					const GpmlPropertyReader::non_null_ptr_to_const_type &boundary_property_reader,
					const GpmlPropertyReader::non_null_ptr_to_const_type &interior_property_reader,
					const GPlatesModel::PropertyName &network_property_name,
					const GPlatesModel::Gpgim &gpgim) :
				d_gpgim(gpgim),
				d_parent_feature_reader(parent_feature_reader),
				d_boundary_property_reader(boundary_property_reader),
				d_interior_property_reader(interior_property_reader),
				d_network_property_name(network_property_name)
			{  }

		};
	}
}

#endif // GPLATES_FILE_IO_GPMLUPGRADEREADERUTILS_H
