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

#ifndef GPLATES_FILE_IO_GPMLFEATUREREADERFACTORY_H
#define GPLATES_FILE_IO_GPMLFEATUREREADERFACTORY_H

#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GpmlFeatureReaderImpl.h"
#include "GpmlFeatureReaderInterface.h"
#include "GpmlPropertyReader.h"
#include "GpmlPropertyStructuralTypeReader.h"
#include "GpmlUpgradeReaderUtils.h"

#include "global/PointerTraits.h"

#include "model/FeatureType.h"
#include "model/GpgimVersion.h"


namespace GPlatesFileIO
{
	/**
	 * Handles generation of GPML feature reader structures that match the GPGIM version in a GPML file.
	 *
	 * The GPGIM version is stored in the GPML file as the 'gpml:version' attribute of the
	 * feature collection XML element.
	 *
	 * NOTE: All GPML files are loaded into the *latest* internal model supported by the
	 * current (latest) version of GPlates regardless of how old the version in the GPML is.
	 * So, in that sense, there's only one GPGIM version (the latest version supported by the
	 * GPlates source code being compiled) and this class knows how to parse older GPGIM version GPML files.
	 */
	class GpmlFeatureReaderFactory :
			private boost::noncopyable
	{
	public:

		/**
		 * Constructs a @a GpmlFeatureReaderFactory from a GPGIM.
		 *
		 * @a gpml_version is the GPGIM version stored in the GPML file.
		 */
		GpmlFeatureReaderFactory(
				const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version);


		/**
		 * Returns the feature reader associated with the specified feature type,
		 * and creates a new feature reader if one was not previously created.
		 *
		 * This creates each feature reader on demand to avoid creating readers for feature types
		 * that are never encountered in loaded GPML files.
		 *
		 * Note that any feature type can be read (even if it's not defined in the GPGIM).
		 *
		 * This can read any property defined in the GPGIM - they will be allowed.
		 * NOTE: This is because we no longer enforce which properties a feature type can load.
		 */
		GpmlFeatureReaderInterface
		get_feature_reader(
				const GPlatesModel::FeatureType &feature_type) const;

	private:

		//! Typedef for a map of feature types to feature reader impls.
		typedef std::map<GPlatesModel::FeatureType, GpmlFeatureReaderImpl::non_null_ptr_type>
				feature_reader_map_impl_type;

		//! Typedef for a sequence of property readers.
		typedef std::vector<GpmlPropertyReader::non_null_ptr_to_const_type> property_reader_seq_type;


		/**
		 * Used to read structural types from a GPML file.
		 */
		GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type d_property_structural_type_reader;

		/**
		 * The version of the GPGIM used to create the GPML file being read.
		 */
		GPlatesModel::GpgimVersion d_gpml_version;

		/**
		 * Used to read feature properties not allowed for a feature type, or when a feature type
		 * is not recognised (by the GPGIM).
		 *
		 * Currently any property (defined in the GPGIM) can be loaded for any feature type
		 * (defined or not defined in the GPGIM). Previously these unprocessed properties (or all
		 * properties for an unknown feature type) were read in as 'UninterpretedPropertyValue' property values.
		 */
		property_reader_seq_type d_unprocessed_property_readers;

		/**
		 * Map of feature types to feature reader impls.
		 *
		 * NOTE: Does not include the catch-all unprocessed feature reader impl at the head of each feature reader's chain.
		 *
		 * NOTE: Feature readers are created on demand as feature types are encountered in GPML files.
		 * This avoids creating feature readers for feature types that are never encountered.
		 */
		mutable feature_reader_map_impl_type d_feature_reader_impl_map;


		/**
		 * Gets the feature reader implementation for the specified feature type.
		 *
		 * NOTE: Does not include the catch-all unprocessed feature reader impl at the head of the reader chain.
		 */
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		get_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;


		/**
		 * Creates a feature reader implementation for the specified feature type.
		 *
		 * NOTE: Does not include the catch-all unprocessed feature reader impl at the head of the reader chain.
		 */
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;


		/**
		 * Creates a feature reader implementation for the specified feature class.
		 *
		 * NOTE: Does not include the catch-all unprocessed feature reader impl at the head of the reader chain.
		 */
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_feature_reader_impl(
				const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class) const;


		/**
		 * Gets the parent feature reader implementation for the specified gpgim feature class.
		 *
		 * If there is no parent GPGIM feature class then a feature creator is returned
		 * (the tail of the chain of all feature readers actually creates a feature)..
		 */
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		get_parent_feature_reader_impl(
				const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class) const;


		/**
		 * Creates a feature reader implementation for the specified feature type that can
		 * upgrade the feature (being read) from an older version GPGIM.
		 *
		 * Returns boost::none if the feature does not need upgrading.
		 */
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;


		//! Creates a feature reader that handles changes made in the GPGIM version (in the method's name).
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_1_6_318_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;

		//! Creates a feature reader that handles changes made in the GPGIM version (in the method's name).
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_1_6_319_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;

		//! Creates a feature reader that handles changes made in the GPGIM version (in the method's name).
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_1_6_320_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;

		//! Creates a feature reader that handles changes made in the GPGIM version (in the method's name).
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_1_6_338_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type) const;


		//! Creates a feature reader that renames a single property.
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_property_name_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type,
				const GPlatesModel::PropertyName &from_property_name,
				const GPlatesModel::PropertyName &to_property_name) const;


		//! Creates a feature reader that renames multiple properties.
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type>
		create_upgrade_property_name_feature_reader_impl(
				const GPlatesModel::FeatureType &feature_type,
				const std::vector<GpmlUpgradeReaderUtils::PropertyRename> &property_rename_pairs) const;

	};
}

#endif // GPLATES_FILE_IO_GPMLFEATUREREADERFACTORY_H
