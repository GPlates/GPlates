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

#include "GpmlFormatDeformationExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"
#include "GpmlOutputVisitor.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ScalarCoverageFeatureProperties.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureCollectionHandle.h"
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
	//! Convenience typedef for a sequence of deformed feature geometries.
	typedef std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> deformed_feature_geometry_seq_type;


	void
	insert_deformed_feature_geometry_into_feature_collection(
			GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *deformed_feature_geometry,
			bool include_dilatation_rate,
			bool include_dilatation)
	{
		typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_strain_rate_seq_type
				point_deformation_strain_rate_seq_type;
		typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_total_strain_seq_type
				point_deformation_total_strain_seq_type;

		point_deformation_strain_rate_seq_type deformation_strain_rates;
		point_deformation_total_strain_seq_type deformation_strains;

		// Only retrieve strain rates and total strains if needed.
		boost::optional<point_deformation_strain_rate_seq_type &> deformation_strain_rates_option;
		boost::optional<point_deformation_total_strain_seq_type &> deformation_strains_option;
		if (include_dilatation_rate)
		{
			deformation_strain_rates_option = deformation_strain_rates;
		}
		if (include_dilatation)
		{
			deformation_strains_option = deformation_strains;
		}

		// Get the current (per-point) geometry data.
		deformed_feature_geometry->get_geometry_data(
				boost::none/*points*/,
				deformation_strain_rates_option,
				deformation_strains_option);

		// Create a new feature.
		const GPlatesModel::FeatureHandle::non_null_ptr_type deformed_feature_geometry_feature =
				GPlatesModel::FeatureHandle::create(
						deformed_feature_geometry->get_feature_ref()->feature_type());
		const GPlatesModel::FeatureHandle::weak_ref deformed_feature_geometry_feature_ref =
				deformed_feature_geometry_feature->reference();

		// The domain property name.
		GPlatesModel::PropertyName domain_property_name = (*deformed_feature_geometry->property())->property_name();
		// Find the range property name associated with the domain property name.
		boost::optional<GPlatesModel::PropertyName> range_property_name =
				GPlatesAppLogic::ScalarCoverageFeatureProperties::get_range_property_name_from_domain(
						domain_property_name);
		if (!range_property_name)
		{
			// There's no range associated with the geometry domain then use the default domain/range
			// property names. This can happen because we have a regular reconstructed feature geometry
			// and not a reconstructed scalar coverage (the latter should have a proper domain/range
			// coverage pairing). However it's possible that the feature type won't support the default names
			// in which case we won't output the feature.
			static const std::pair<GPlatesModel::PropertyName/*domain*/, GPlatesModel::PropertyName/*range*/>
					default_domain_range_property_names =
							GPlatesAppLogic::ScalarCoverageFeatureProperties::get_default_domain_range_property_names();

			domain_property_name = default_domain_range_property_names.first;
			range_property_name = default_domain_range_property_names.second;
		}

		// The reconstructed range (scalars) property.
		GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type reconstructed_range_property =
				GPlatesPropertyValues::GmlDataBlock::create();

		// Include dilatation rate if requested.
		if (include_dilatation_rate)
		{
			std::vector<double> dilatation_rates;
			dilatation_rates.reserve(deformation_strain_rates.size());
			for (unsigned int d = 0; d < deformation_strain_rates.size(); ++d)
			{
				dilatation_rates.push_back(deformation_strain_rates[d].get_dilatation());
			}

			GPlatesPropertyValues::ValueObjectType dilatation_rate_type =
					GPlatesPropertyValues::ValueObjectType::create_gpml("DilatationRate");
			GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type dilatation_rate_xml_attrs;
			GPlatesModel::XmlAttributeName uom = GPlatesModel::XmlAttributeName::create_gpml("uom");
			GPlatesModel::XmlAttributeValue per_second("urn:x-si:v1999:uom:per_second");
			dilatation_rate_xml_attrs.insert(std::make_pair(uom, per_second));

			// Add the dilatation rate scalar values we're exporting.
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type dilatation_rate_range =
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
							dilatation_rate_type,
							dilatation_rate_xml_attrs,
							dilatation_rates.begin(),
							dilatation_rates.end());

			reconstructed_range_property->tuple_list_push_back(dilatation_rate_range);
		}

		// Include accumulated dilatation if requested.
		if (include_dilatation)
		{
			std::vector<double> dilatations;
			dilatations.reserve(deformation_strains.size());
			for (unsigned int d = 0; d < deformation_strains.size(); ++d)
			{
				dilatations.push_back(deformation_strains[d].get_dilatation());
			}

			GPlatesPropertyValues::ValueObjectType dilatation_type =
					GPlatesPropertyValues::ValueObjectType::create_gpml("Dilatation");
			GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type dilatation_xml_attrs;
			// Total dilatation has no units so don't add any "uom" XML attribute.

			// Add the dilatation scalar values we're exporting.
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type dilatation_range =
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
							dilatation_type,
							dilatation_xml_attrs,
							dilatations.begin(),
							dilatations.end());

			reconstructed_range_property->tuple_list_push_back(dilatation_range);
		}


		// The reconstructed domain (geometry) property.
		const GPlatesModel::PropertyValue::non_null_ptr_type reconstructed_domain_property =
				GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
						deformed_feature_geometry->reconstructed_geometry());


		// Add the reconstructed domain/range properties.
		//
		// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
		// necessary time-dependent wrapper is added.
		if (!GPlatesModel::ModelUtils::add_property(
				deformed_feature_geometry_feature_ref,
				domain_property_name,
				reconstructed_domain_property))
		{
			// We probably changed the domain/range property names so we could output a scalar coverage,
			// but the feature type doesn't support them. Return without outputting the feature.
			return;
		}

		if (include_dilatation_rate ||
			include_dilatation)
		{
			if (!GPlatesModel::ModelUtils::add_property(
					deformed_feature_geometry_feature_ref,
					range_property_name.get(),
					reconstructed_range_property))
			{
				// We probably changed the domain/range property names so we could output a scalar coverage,
				// but the feature type doesn't support them. Return without outputting the feature.
				return;
			}
		}

		// Finally add the feature to the feature collection.
		feature_collection->add(deformed_feature_geometry_feature);
	}
}


void
GPlatesFileIO::GpmlFormatDeformationExport::export_deformation(
		const std::list<deformed_feature_geometry_group_type> &deformed_feature_geometry_group_seq,
		const QFileInfo& file_info,
		GPlatesModel::ModelInterface &model,
		bool include_dilatation_rate,
		bool include_dilatation)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	GPlatesModel::NotificationGuard model_notification_guard(*model.access_model());

	// NOTE: We don't add to the feature store otherwise it'll remain there but
	// we want to release it (and its memory) after export.
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create();
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref = feature_collection->reference();

	// Iterate through the deformed feature geometries and write to output.
	std::list<deformed_feature_geometry_group_type>::const_iterator feature_iter;
	for (feature_iter = deformed_feature_geometry_group_seq.begin();
		feature_iter != deformed_feature_geometry_group_seq.end();
		++feature_iter)
	{
		const deformed_feature_geometry_group_type &deformed_feature_geometry_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = deformed_feature_geometry_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Iterate through the deformed feature geometries of the current feature and write to output.
		deformed_feature_geometry_seq_type::const_iterator dfg_iter;
		for (dfg_iter = deformed_feature_geometry_group.recon_geoms.begin();
			dfg_iter != deformed_feature_geometry_group.recon_geoms.end();
			++dfg_iter)
		{
			const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *dfg = *dfg_iter;

			insert_deformed_feature_geometry_into_feature_collection(
					feature_collection_ref,
					dfg,
					include_dilatation_rate,
					include_dilatation);
		}
	}

	// The output file info.
	FileInfo output_file(file_info.filePath());

	// Write the output file by visiting the feature collection with the new velocity fields.
	GpmlOutputVisitor gpml_writer(output_file, feature_collection_ref, false);
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(feature_collection_ref, gpml_writer);
}
