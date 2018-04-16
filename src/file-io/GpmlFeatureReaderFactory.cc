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

#include <QDebug>

#include "GpmlFeatureReaderFactory.h"

#include "GpmlFeatureReaderImpl.h"
#include "GpmlUpgradeReaderUtils.h"

#include "model/Gpgim.h"
#include "model/GpgimFeatureClass.h"


GPlatesFileIO::GpmlFeatureReaderFactory::GpmlFeatureReaderFactory(
		const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version) :
	d_property_structural_type_reader(property_structural_type_reader),
	d_gpml_version(gpml_version)
{
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderInterface>
GPlatesFileIO::GpmlFeatureReaderFactory::get_feature_reader(
		const GPlatesModel::FeatureType &feature_type) const
{
	// Get the feature reader implementation.
	boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
			get_feature_reader_impl(feature_type);
	if (!feature_reader_impl)
	{
		return boost::none;
	}

	// Create a final catch-all feature reader impl that reads all unprocessed feature
	// properties as 'UninterpretedPropertyValue'.
	GpmlUninterpretedFeatureReader::non_null_ptr_type uninterpreted_feature_reader_impl =
			GpmlUninterpretedFeatureReader::create(feature_reader_impl.get());

	// NOTE: We add the final catch-all uninterpreted feature reader here since this ensures we only
	// have one uninterpreted feature reader at the head of the feature reader's chain (ie, it avoids
	// a feature reader chain that alternates between regular and uninterpreted feature readers).

	return GpmlFeatureReaderInterface(uninterpreted_feature_reader_impl);
}


GPlatesFileIO::GpmlFeatureReaderInterface
GPlatesFileIO::GpmlFeatureReaderFactory::get_uninterpreted_feature_reader() const
{
	return GpmlFeatureReaderInterface(get_uninterpreted_feature_reader_impl());
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::get_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	// See if we've previously created a feature reader impl for the specified feature type.
	feature_reader_map_impl_type::const_iterator feature_reader_impl_iter =
			d_feature_reader_impl_map.find(feature_type);
	if (feature_reader_impl_iter != d_feature_reader_impl_map.end())
	{
		return feature_reader_impl_iter->second;
	}

	//
	// Create a feature reader implementation.
	//
	boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl;

	// First see if the GPML file was created using an older version GPGIM.
	// If so then it may need to be upgraded to the current GPGIM as it is being read.
	if (d_gpml_version < GPlatesModel::Gpgim::instance().get_version())
	{
		feature_reader_impl = create_upgrade_feature_reader_impl(feature_type);
	}

	// If:
	//    (1) GPML file version is the same as the current GPGIM version or a future version
	//        (if GPML file created by a future version of GPlates), or
	//    (2) if the feature reader for the current feature type does not need upgrading,
	//
	// ...then create the feature reader based on the current GPGIM.
	//
	if (!feature_reader_impl)
	{
		feature_reader_impl = create_feature_reader_impl(feature_type);
		if (!feature_reader_impl)
		{
			// The specified feature type is not recognised by the GPGIM.
			return boost::none;
		}
	}

	// Add the new feature reader impl to our map so subsequent queries can re-use it.
	d_feature_reader_impl_map.insert(
			feature_reader_map_impl_type::value_type(
					feature_type,
					feature_reader_impl.get()));

	// Return the new feature reader impl.
	return feature_reader_impl.get();
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	// Query the GPGIM for the GPGIM feature class associated with the specified feature type.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type>
			gpgim_feature_class = GPlatesModel::Gpgim::instance().get_feature_class(feature_type);
	if (!gpgim_feature_class)
	{
		// The specified feature type is not recognised by the GPGIM.
		//
		// If we're reading from a GPML file created from an earlier GPGIM version then it's possible
		// the feature type has since been renamed - so we'll log a warning to indicate that a
		// developer might need to implement an update handler to change the feature type.
		if (d_gpml_version < GPlatesModel::Gpgim::instance().get_version())
		{
			qWarning() << "GpmlFeatureReaderFactory: feature type '"
				<< convert_qualified_xml_name_to_qstring(feature_type)
				<< "' read from GPML file version '"
				<< d_gpml_version.get_version_string()
				<< "' is not recognised by the current GPGIM '"
				<< GPlatesModel::Gpgim::instance().get_version().get_version_string()
				<< "':";
			qWarning() << "...might need to implement an upgrade handler to change feature type.";
		}

		return boost::none;
	}

	return create_feature_reader_impl(gpgim_feature_class.get());
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_feature_reader_impl(
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class) const
{
	//
	// Create a new feature reader implementation.
	//

	// Reading of parent properties gets delegated to the parent reader.
	boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> parent_feature_reader_impl =
			get_parent_feature_reader_impl(gpgim_feature_class);
	if (!parent_feature_reader_impl)
	{
		// The parent feature type is not recognised by the GPGIM.
		// This would be unusual - for example, if 'gpml:TangibleFeature' was missing from the GPGIM.
		return boost::none;
	}

	// Create a feature reader to read only the feature properties associated with the current
	// GPGIM feature class (and delegate reading of parent properties to the parent reader).
	GpmlFeatureReaderImpl::non_null_ptr_type feature_reader =
			GpmlFeatureReader::create(
					gpgim_feature_class,
					parent_feature_reader_impl.get(),
					d_property_structural_type_reader,
					d_gpml_version);

	// NOTE: We don't add the final catch-all uninterpreted feature reader here since this reader
	// can be used as a parent reader by another feature reader and we only want one uninterpreted
	// feature reader at the head of each feature reader's chain (ie, we don't want a feature reader
	// chain that alternates between regular and uninterpreted feature readers).

	return feature_reader;
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::get_parent_feature_reader_impl(
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class) const
{
	// See if the GPGIM feature class inherits a parent feature class.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> parent_feature_class =
			gpgim_feature_class->get_parent_feature_class();
	if (!parent_feature_class)
	{
		// There is no parent GPGIM feature class which means we've reached the root of the
		// GPGIM feature class inheritance hierarchy.

		// Create the terminal feature reader impl that actually creates a new feature and reads in
		// the feature-id and revision-id that exist in every GPlates feature.
		return GpmlFeatureReaderImpl::non_null_ptr_type(GpmlFeatureCreator::create(d_gpml_version));
	}

	// The parent feature type.
	//
	// This may or may not represent a concrete feature type that can be instantiated in GPlates.
	// For example, it could be 'gpml:TangibleFeature' which is an abstract class that concrete
	// (or even abstract) feature types can inherit from.
	// In either case (concrete or abstract) we can still create a feature reader for the parent feature type.
	const GPlatesModel::FeatureType parent_feature_type = parent_feature_class.get()->get_feature_type();

	// Get the feature reader implementation associated with the parent GPGIM feature class.
	return get_feature_reader_impl(parent_feature_type);
}


GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type
GPlatesFileIO::GpmlFeatureReaderFactory::get_uninterpreted_feature_reader_impl() const
{
	if (!d_uninterpreted_feature_reader_impl)
	{
		// Create the terminal feature reader impl that actually creates a new feature and reads in
		// the feature-id and revision-id that exist in every GPlates feature.
		GpmlFeatureCreator::non_null_ptr_to_const_type feature_creator =
				GpmlFeatureCreator::create(d_gpml_version);

		// Create a feature reader impl that reads all feature properties as 'UninterpretedPropertyValue'
		// property values - this is used for feature types that are not recognised by the GPGIM.
		d_uninterpreted_feature_reader_impl = GpmlUninterpretedFeatureReader::create(feature_creator);
	}

	return d_uninterpreted_feature_reader_impl.get();
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	//
	// Check GPGIM versions starting with least recent (lowest version) and
	// ending with most recent (highest version).
	//
	// This order ensures less recent (older) GPML files are upgraded correctly.
	//
	// Note that each upgrade reader converts, the specified feature type, to the latest (current) GPGIM version.
	// Hence we don't need to chain them together, so if an upgrade reader is found it is returned
	// immediately, otherwise we look for the next upgrade reader.
	//

	static const GPlatesModel::GpgimVersion GPGIM_VERSION_1_6_318(1, 6, 318);
	if (d_gpml_version < GPGIM_VERSION_1_6_318)
	{
		boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
				create_upgrade_1_6_318_feature_reader_impl(feature_type);
		if (feature_reader_impl)
		{
			return feature_reader_impl.get();
		}
		// Fall through to check for a more recent GPML file...
	}

	static const GPlatesModel::GpgimVersion GPGIM_VERSION_1_6_319(1, 6, 319);
	if (d_gpml_version < GPGIM_VERSION_1_6_319)
	{
		boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
				create_upgrade_1_6_319_feature_reader_impl(feature_type);
		if (feature_reader_impl)
		{
			return feature_reader_impl.get();
		}
		// Fall through to check for a more recent GPML file...
	}

	static const GPlatesModel::GpgimVersion GPGIM_VERSION_1_6_320(1, 6, 320);
	if (d_gpml_version < GPGIM_VERSION_1_6_320)
	{
		boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
				create_upgrade_1_6_320_feature_reader_impl(feature_type);
		if (feature_reader_impl)
		{
			return feature_reader_impl.get();
		}
		// Fall through to check for a more recent GPML file...
	}

	static const GPlatesModel::GpgimVersion GPGIM_VERSION_1_6_338(1, 6, 338);
	if (d_gpml_version < GPGIM_VERSION_1_6_338)
	{
		boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
				create_upgrade_1_6_338_feature_reader_impl(feature_type);
		if (feature_reader_impl)
		{
			return feature_reader_impl.get();
		}
		// Fall through to check for a more recent GPML file...
	}

	// No upgrade necessary.
	return boost::none;
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_1_6_318_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	//
	// This upgrade handles changes made in GPGIM version 1.6.318
	//

	static const GPlatesModel::FeatureType ABSOLUTE_REFERENCE_FRAME_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("AbsoluteReferenceFrame");
	if (feature_type == ABSOLUTE_REFERENCE_FRAME_FEATURE_TYPE)
	{
		// Need to rename 'gpml:type' to 'gpml:absoluteReferenceFrame'.
		static const GPlatesModel::PropertyName FROM_ABSOLUTE_REFERENCE_FRAME_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("type");
		static const GPlatesModel::PropertyName TO_ABSOLUTE_REFERENCE_FRAME_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("absoluteReferenceFrame");

		return create_upgrade_property_name_feature_reader_impl(
				feature_type,
				FROM_ABSOLUTE_REFERENCE_FRAME_PROPERTY_NAME,
				TO_ABSOLUTE_REFERENCE_FRAME_PROPERTY_NAME);
	}

	static const GPlatesModel::FeatureType CONTINENTAL_CLOSED_BOUNDARY_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("ClosedContinentalBoundary");
	if (feature_type == CONTINENTAL_CLOSED_BOUNDARY_FEATURE_TYPE)
	{
		// Need to rename 'gpml:type' to 'gpml:crust'.
		static const GPlatesModel::PropertyName FROM_CRUST_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("type");
		static const GPlatesModel::PropertyName TO_CRUST_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("crust");

		return create_upgrade_property_name_feature_reader_impl(
				feature_type,
				FROM_CRUST_PROPERTY_NAME,
				TO_CRUST_PROPERTY_NAME);
	}

	return boost::none;
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_1_6_319_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	//
	// This upgrade handles changes made in GPGIM version 1.6.319
	//

	static const GPlatesModel::FeatureType TOPOLOGICAL_NETWORK_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("TopologicalNetwork");
	if (feature_type == TOPOLOGICAL_NETWORK_FEATURE_TYPE)
	{
		// Rename some properties.
		std::vector<GpmlUpgradeReaderUtils::PropertyRename> property_rename_pairs;

		// Rename 'gpml:shapeFactor' to 'gpml:networkShapeFactor'.
		static const GPlatesModel::PropertyName FROM_SHAPE_FACTOR_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("shapeFactor");
		static const GPlatesModel::PropertyName TO_SHAPE_FACTOR_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("networkShapeFactor");
		property_rename_pairs.push_back(GpmlUpgradeReaderUtils::PropertyRename(
				FROM_SHAPE_FACTOR_PROPERTY_NAME,
				TO_SHAPE_FACTOR_PROPERTY_NAME));

		// Rename 'gpml:maxEdge' to 'gpml:networkMaxEdge'.
		static const GPlatesModel::PropertyName FROM_MAX_EDGE_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("maxEdge");
		static const GPlatesModel::PropertyName TO_MAX_EDGE_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("networkMaxEdge");
		property_rename_pairs.push_back(GpmlUpgradeReaderUtils::PropertyRename(
				FROM_MAX_EDGE_PROPERTY_NAME,
				TO_MAX_EDGE_PROPERTY_NAME));

		// Query the GPGIM for the GPGIM feature class associated with the feature type.
		boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type>
				original_gpgim_feature_class =
						GPlatesModel::Gpgim::instance().get_feature_class(feature_type);
		if (!original_gpgim_feature_class)
		{
			return boost::none;
		}

		// Copy the GPGIM feature class but change the GPGIM property with the matching property name(s).
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type gpgim_feature_class =
				GpmlUpgradeReaderUtils::rename_gpgim_feature_class_properties(
						original_gpgim_feature_class.get(),
						property_rename_pairs);

		// Feature reader associated with the parent GPGIM feature class.
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> parent_feature_reader_impl =
				get_parent_feature_reader_impl(gpgim_feature_class);
		// Should always have a parent feature reader even if don't have a parent feature class.
		if (!parent_feature_reader_impl)
		{
			return boost::none;
		}

		// Create a feature reader that combines a single 'gpml:boundary' and
		// multiple 'gpml:interior' properties into single 'gpml:network' property.
		boost::optional<GpmlUpgradeReaderUtils::TopologicalNetworkFeatureReaderUpgrade_1_6_319::non_null_ptr_type>
				feature_reader_impl =
						GpmlUpgradeReaderUtils::TopologicalNetworkFeatureReaderUpgrade_1_6_319::create(
								gpgim_feature_class,
								parent_feature_reader_impl.get(),
								d_property_structural_type_reader,
								d_gpml_version);
		if (!feature_reader_impl)
		{
			// Can happen if can't find network feature property, for example, in GPGIM.
			return boost::none;
		}

		// For each property rename pair, chain a new rename property feature reader impl
		// onto the list of readers.
		return GpmlUpgradeReaderUtils::create_property_rename_feature_reader_impl(
				feature_reader_impl.get(),
				property_rename_pairs);
	}

	return boost::none;
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_1_6_320_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	//
	// This upgrade handles changes made in GPGIM version 1.6.320
	//

	static const GPlatesModel::FeatureType UNCLASSIFIED_TOPOLOGICAL_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("UnclassifiedTopologicalFeature");
	// The feature type was incorrectly spelled when it was first added !!
	static const GPlatesModel::FeatureType UNCLASSIFIED_TOPOLOGCIAL_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("UnclassifiedTopologcialFeature");
	if (feature_type == UNCLASSIFIED_TOPOLOGICAL_FEATURE_TYPE ||
		feature_type == UNCLASSIFIED_TOPOLOGCIAL_FEATURE_TYPE)
	{
		static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE_TYPE =
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");

		// Get the feature reader implementation that reads 'gpml:UnclassifiedFeature'.
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> unclassified_feature_reader_impl =
				get_feature_reader_impl(UNCLASSIFIED_FEATURE_TYPE);
		if (!unclassified_feature_reader_impl)
		{
			return boost::none;
		}

		// Wrap in a feature reader that changes the feature type (read in) to 'gpml:UnclassifiedFeature'. 
		return GpmlFeatureReaderImpl::non_null_ptr_type(
				GpmlUpgradeReaderUtils::ChangeFeatureTypeFeatureReaderImpl::create(
						UNCLASSIFIED_FEATURE_TYPE,
						unclassified_feature_reader_impl.get()));
	}

	return boost::none;
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_1_6_338_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type) const
{
	//
	// This upgrade handles changes made in GPGIM version 1.6.338
	//

	static const GPlatesModel::FeatureType SCALAR_COVERAGE_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("ScalarCoverage");
	if (feature_type == SCALAR_COVERAGE_FEATURE_TYPE)
	{
		// Create a feature reader implementation that reads scalar coverage feature types.
		boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
				create_feature_reader_impl(SCALAR_COVERAGE_FEATURE_TYPE);
		if (!feature_reader_impl)
		{
			return boost::none;
		}

		// Create a feature reader that updates any crustal thinning factors in scalar coverage.
		// The crustal thinning factors were incorrect in GPlates 2.0 (fixed in 2.1).
		return GpmlFeatureReaderImpl::non_null_ptr_type(
				GpmlUpgradeReaderUtils::CrustalThinningFactorUpgrade_1_6_338::create(
						feature_reader_impl.get()));
	}

	return boost::none;
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_property_name_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type,
		const GPlatesModel::PropertyName &from_property_name,
		const GPlatesModel::PropertyName &to_property_name) const
{
	const GpmlUpgradeReaderUtils::PropertyRename property_rename_pair(
			from_property_name,
			to_property_name);

	return create_upgrade_property_name_feature_reader_impl(
			feature_type,
			std::vector<GpmlUpgradeReaderUtils::PropertyRename>(1, property_rename_pair));
}


boost::optional<GPlatesFileIO::GpmlFeatureReaderImpl::non_null_ptr_type>
GPlatesFileIO::GpmlFeatureReaderFactory::create_upgrade_property_name_feature_reader_impl(
		const GPlatesModel::FeatureType &feature_type,
		const std::vector<GpmlUpgradeReaderUtils::PropertyRename> &property_rename_pairs) const
{
	// Query the GPGIM for the GPGIM feature class associated with the specified feature type.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> original_gpgim_feature_class =
			GPlatesModel::Gpgim::instance().get_feature_class(feature_type);
	if (!original_gpgim_feature_class)
	{
		return boost::none;
	}

	// Copy the GPGIM feature class but change the GPGIM property with the matching property name(s).
	// The returned GPGIM feature class will have the old-name GPGIM property(s) so that we can
	// read the old version GPML file.
	const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type gpgim_feature_class =
			GpmlUpgradeReaderUtils::rename_gpgim_feature_class_properties(
					original_gpgim_feature_class.get(),
					property_rename_pairs);

	// Create a feature reader implementation that reads the old version property name(s).
	boost::optional<GpmlFeatureReaderImpl::non_null_ptr_type> feature_reader_impl =
			create_feature_reader_impl(gpgim_feature_class);
	if (!feature_reader_impl)
	{
		return boost::none;
	}

	// For each property rename pair, chain a new rename property feature reader impl
	// onto the list of readers.
	return GpmlUpgradeReaderUtils::create_property_rename_feature_reader_impl(
			feature_reader_impl.get(),
			property_rename_pairs);
}
