/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GpmlFormatReconstructedScalarCoverageExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"
#include "GpmlOutputVisitor.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedScalarCoverage.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ScalarCoverageFeatureProperties.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsString.h"


namespace
{
	//! Convenience typedef for a sequence of reconstructed scalar coverages.
	typedef std::vector<const GPlatesAppLogic::ReconstructedScalarCoverage *> reconstructed_scalar_coverage_seq_type;


	boost::optional<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type>
	get_range_associated_with_reconstructed_scalar_coverage(
			const GPlatesAppLogic::ReconstructedScalarCoverage *reconstructed_scalar_coverage,
			const GPlatesModel::PropertyName &domain_property_name)
	{
		// Get all scalar coverages of the domain feature.
		std::vector<GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage> scalar_coverages;
		GPlatesAppLogic::ScalarCoverageFeatureProperties::get_coverages(
				scalar_coverages,
				reconstructed_scalar_coverage->get_feature_ref());

		// Iterate over all domain/range coverages until we find the one associated with our
		// reconstructed scalar coverage, then use it to get the range XML attributes.
		const unsigned int num_scalar_coverages = scalar_coverages.size();
		for (unsigned int c = 0; c < num_scalar_coverages; ++c)
		{
			const GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage &scalar_coverage = scalar_coverages[c];

			// See if property names match - only need to check domain name since range name is a
			// one-to-one mapping from domain name.
			if ((*scalar_coverage.domain_property)->property_name() == domain_property_name)
			{
				// Get the range property value from the range property iterator.
				boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> range_property_value_base =
						GPlatesModel::ModelUtils::get_property_value(**scalar_coverage.range_property);
				if (range_property_value_base)
				{
					boost::optional<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_to_const_type> range_property_value =
							GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlDataBlock>(*range_property_value_base.get());
					if (range_property_value)
					{
						// Iterate over the scalar types until we find a matching one.
						GPlatesPropertyValues::GmlDataBlock::tuple_list_type::const_iterator range_iter =
								range_property_value.get()->tuple_list_begin();
						GPlatesPropertyValues::GmlDataBlock::tuple_list_type::const_iterator range_end =
								range_property_value.get()->tuple_list_end();
						for ( ; range_iter != range_end; ++range_iter)
						{
							GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type range = *range_iter;
							if (range->value_object_type() == reconstructed_scalar_coverage->get_scalar_type())
							{
								return range;
							}
						}
					}
				}
			}
		}

		return boost::none;
	}


	void
	insert_reconstructed_scalar_coverage_into_feature_collection(
			GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			const GPlatesAppLogic::ReconstructedScalarCoverage *reconstructed_scalar_coverage,
			bool include_dilatation_strain_rate,
			bool include_second_invariant_strain_rate)
	{
		// Create a new feature.
		const GPlatesModel::FeatureHandle::non_null_ptr_type reconstructed_scalar_coverage_feature =
				GPlatesModel::FeatureHandle::create(
						reconstructed_scalar_coverage->get_feature_ref()->feature_type());
		const GPlatesModel::FeatureHandle::weak_ref reconstructed_scalar_coverage_feature_ref =
				reconstructed_scalar_coverage_feature->reference();

		// The domain/range property names.
		const GPlatesModel::PropertyName domain_property_name =
				(*reconstructed_scalar_coverage->get_domain_property())->property_name();
		const GPlatesModel::PropertyName range_property_name =
				(*reconstructed_scalar_coverage->get_range_property())->property_name();

		// Get the range in the domain feature associated with the reconstructed scalar coverage.
		boost::optional<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type>
				original_range = get_range_associated_with_reconstructed_scalar_coverage(
						reconstructed_scalar_coverage,
						domain_property_name);
		if (!original_range)
		{
			// Should have been able to find the domain/range.
			// Just return without adding feature to collection.
			return;
		}

		std::vector<double> scalar_values;
		reconstructed_scalar_coverage->get_reconstructed_point_scalar_values(scalar_values);

		// The reconstructed range (scalars) property.
		GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type reconstructed_range_property =
				GPlatesPropertyValues::GmlDataBlock::create();

		if (include_dilatation_strain_rate ||
			include_second_invariant_strain_rate)
		{
			boost::optional<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> dfg =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *>(
									reconstructed_scalar_coverage->get_reconstructed_feature_geometry().get());

			typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_strain_rate_seq_type
					point_deformation_strain_rate_seq_type;

			point_deformation_strain_rate_seq_type deformation_strain_rates;
			if (dfg)
			{
				// Get the current (per-point) geometry data.
				dfg.get()->get_geometry_data(
						boost::none/*points*/,
						deformation_strain_rates);
			}

			if (include_dilatation_strain_rate)
			{
				std::vector<double> dilatation_strain_rates;

				if (dfg)
				{
					dilatation_strain_rates.reserve(deformation_strain_rates.size());
					for (unsigned int d = 0; d < deformation_strain_rates.size(); ++d)
					{
						dilatation_strain_rates.push_back(deformation_strain_rates[d].get_strain_rate_dilatation());
					}
				}
				else
				{
					// The RFG is not a TopologyReconstructedFeatureGeometry so we have no deformation strain rate information.
					// Default to zero rate strain.
					dilatation_strain_rates.resize(scalar_values.size(), 0.0);
				}

				GPlatesPropertyValues::ValueObjectType dilatation_strain_rate_type =
						GPlatesPropertyValues::ValueObjectType::create_gpml("DilatationStrainRate");
				GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type dilatation_strain_rate_xml_attrs;
				GPlatesModel::XmlAttributeName uom = GPlatesModel::XmlAttributeName::create_gpml("uom");
				GPlatesModel::XmlAttributeValue per_second("urn:x-si:v1999:uom:per_second");
				dilatation_strain_rate_xml_attrs.insert(std::make_pair(uom, per_second));

				// Add the dilatation strain rate scalar values we're exporting.
				GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type dilatation_strain_rate_range =
						GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
								dilatation_strain_rate_type,
								dilatation_strain_rate_xml_attrs,
								dilatation_strain_rates.begin(),
								dilatation_strain_rates.end());
				reconstructed_range_property->tuple_list_push_back(dilatation_strain_rate_range);
			}

			if (include_second_invariant_strain_rate)
			{
				std::vector<double> second_invariant_strain_rates;

				if (dfg)
				{
					second_invariant_strain_rates.reserve(deformation_strain_rates.size());
					for (unsigned int d = 0; d < deformation_strain_rates.size(); ++d)
					{
						second_invariant_strain_rates.push_back(deformation_strain_rates[d].get_strain_rate_second_invariant());
					}
				}
				else
				{
					// The RFG is not a TopologyReconstructedFeatureGeometry so we have no deformation strain rate information.
					// Default to zero rate strain.
					second_invariant_strain_rates.resize(scalar_values.size(), 0.0);
				}

				GPlatesPropertyValues::ValueObjectType second_invariant_strain_rate_type =
						GPlatesPropertyValues::ValueObjectType::create_gpml("TotalStrainRate");
				GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type second_invariant_strain_rate_xml_attrs;
				GPlatesModel::XmlAttributeName uom = GPlatesModel::XmlAttributeName::create_gpml("uom");
				GPlatesModel::XmlAttributeValue per_second("urn:x-si:v1999:uom:per_second");
				second_invariant_strain_rate_xml_attrs.insert(std::make_pair(uom, per_second));

				// Add the second invariant strain rate scalar values we're exporting.
				GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type dilatation_range =
						GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
								second_invariant_strain_rate_type,
								second_invariant_strain_rate_xml_attrs,
								second_invariant_strain_rates.begin(),
								second_invariant_strain_rates.end());
				reconstructed_range_property->tuple_list_push_back(dilatation_range);
			}
		}

		// Add the reconstructed scalar values we're exporting.
		GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type reconstructed_range =
				GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
						original_range.get()->value_object_type(),
						original_range.get()->value_object_xml_attributes(),
						scalar_values.begin(),
						scalar_values.end());
		reconstructed_range_property->tuple_list_push_back(reconstructed_range);


		// The reconstructed domain (geometry) property.
		const GPlatesModel::PropertyValue::non_null_ptr_type reconstructed_domain_property =
				GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
						reconstructed_scalar_coverage->get_reconstructed_geometry());


		// Add the reconstructed domain/range properties.
		//
		// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
		// necessary time-dependent wrapper is added.
		GPlatesModel::ModelUtils::add_property(
				reconstructed_scalar_coverage_feature_ref,
				domain_property_name,
				reconstructed_domain_property);
		GPlatesModel::ModelUtils::add_property(
				reconstructed_scalar_coverage_feature_ref,
				range_property_name,
				reconstructed_range_property);

		// Finally add the feature to the feature collection.
		feature_collection->add(reconstructed_scalar_coverage_feature);
	}
}


void
GPlatesFileIO::GpmlFormatReconstructedScalarCoverageExport::export_reconstructed_scalar_coverages(
		const std::list<reconstructed_scalar_coverage_group_type> &reconstructed_scalar_coverage_group_seq,
		const QFileInfo& file_info,
		GPlatesModel::ModelInterface &model,
		bool include_dilatation_strain_rate,
		bool include_second_invariant_strain_rate)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	GPlatesModel::NotificationGuard model_notification_guard(*model.access_model());

	// NOTE: We don't add to the feature store otherwise it'll remain there but
	// we want to release it (and its memory) after export.
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create();
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref = feature_collection->reference();

	// Iterate through the reconstructed scalar coverages and write to output.
	std::list<reconstructed_scalar_coverage_group_type>::const_iterator feature_iter;
	for (feature_iter = reconstructed_scalar_coverage_group_seq.begin();
		feature_iter != reconstructed_scalar_coverage_group_seq.end();
		++feature_iter)
	{
		const reconstructed_scalar_coverage_group_type &feature_scalar_coverage_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = feature_scalar_coverage_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Iterate through the reconstructed scalar coverages of the current feature and write to output.
		reconstructed_scalar_coverage_seq_type::const_iterator rsc_iter;
		for (rsc_iter = feature_scalar_coverage_group.recon_geoms.begin();
			rsc_iter != feature_scalar_coverage_group.recon_geoms.end();
			++rsc_iter)
		{
			const GPlatesAppLogic::ReconstructedScalarCoverage *rsc = *rsc_iter;

			insert_reconstructed_scalar_coverage_into_feature_collection(
					feature_collection_ref,
					rsc,
					include_dilatation_strain_rate,
					include_second_invariant_strain_rate);
		}
	}

	// The output file info.
	FileInfo output_file(file_info.filePath());

	// Write the output file by visiting the feature collection with the new velocity fields.
	GpmlOutputVisitor gpml_writer(output_file, feature_collection_ref, false);
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(feature_collection_ref, gpml_writer);
}
