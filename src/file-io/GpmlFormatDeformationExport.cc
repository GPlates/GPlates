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

#include "maths/MathsUtils.h"

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
			boost::optional<GPlatesFileIO::DeformationExport::PrincipalStrainOptions> include_principal_strain,
			bool include_dilatation_strain,
			bool include_dilatation_strain_rate,
			bool include_second_invariant_strain_rate)
	{
		typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_strain_rate_seq_type
				point_deformation_strain_rate_seq_type;
		typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_total_strain_seq_type
				point_deformation_total_strain_seq_type;

		point_deformation_strain_rate_seq_type deformation_strain_rates;
		point_deformation_total_strain_seq_type deformation_strains;

		// Only retrieve strain rates if needed.
		boost::optional<point_deformation_strain_rate_seq_type &> deformation_strain_rates_option;
		if (include_dilatation_strain_rate ||
			include_second_invariant_strain_rate)
		{
			deformation_strain_rates_option = deformation_strain_rates;
		}

		// Only retrieve strain if needed.
		boost::optional<point_deformation_total_strain_seq_type &> deformation_strains_option;
		if (include_principal_strain ||
			include_dilatation_strain)
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

		// Include principal strain if requested.
		if (include_principal_strain)
		{
			std::vector<double> principal_majors;
			std::vector<double> principal_minors;
			std::vector<double> principal_angles;
			principal_majors.reserve(deformation_strains.size());
			principal_minors.reserve(deformation_strains.size());
			principal_angles.reserve(deformation_strains.size());
			for (unsigned int d = 0; d < deformation_strains.size(); ++d)
			{
				const GPlatesAppLogic::DeformationStrain::StrainPrincipal principal_strain =
						deformation_strains[d].get_strain_principal();

				if (include_principal_strain->output == GPlatesFileIO::DeformationExport::PrincipalStrainOptions::STRAIN)
				{
					// Output strain.
					principal_majors.push_back(principal_strain.principal1);
					principal_minors.push_back(principal_strain.principal2);
				}
				else // PrincipalStrainOptions::STRETCH...
				{
					// Output stretch (1.0 + strain).
					principal_majors.push_back(1.0 + principal_strain.principal1);
					principal_minors.push_back(1.0 + principal_strain.principal2);
				}

				principal_angles.push_back(
						include_principal_strain->get_principal_angle_or_azimuth_in_degrees(principal_strain));
			}

			// Add the principal angle scalar values we're exporting.
			GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type principal_angle_xml_attrs;
			principal_angle_xml_attrs.insert(std::make_pair(
					GPlatesModel::XmlAttributeName::create_gpml("uom"),
					GPlatesModel::XmlAttributeValue("urn:x-epsg:v0.1:uom:degree")));
			reconstructed_range_property->tuple_list_push_back(
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
					include_principal_strain->output == GPlatesFileIO::DeformationExport::PrincipalStrainOptions::STRAIN
							? (include_principal_strain->format == GPlatesFileIO::DeformationExport::PrincipalStrainOptions::ANGLE_MAJOR_MINOR
									? GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStrainMajorAngle")
									: GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStrainMajorAzimuth"))
							: (include_principal_strain->format == GPlatesFileIO::DeformationExport::PrincipalStrainOptions::ANGLE_MAJOR_MINOR
									? GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStretchMajorAngle")
									: GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStretchMajorAzimuth")),
					principal_angle_xml_attrs,
					principal_angles.begin(),
					principal_angles.end()));

			// Add the principal major scalar values we're exporting.
			reconstructed_range_property->tuple_list_push_back(
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
					include_principal_strain->output == GPlatesFileIO::DeformationExport::PrincipalStrainOptions::STRAIN ?
							GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStrainMajorAxis") :
							GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStretchMajorAxis"),
					// Strain/stretch has no units so don't add any "uom" XML attribute...
					GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type(),
					principal_majors.begin(),
					principal_majors.end()));

			// Add the principal minor scalar values we're exporting.
			reconstructed_range_property->tuple_list_push_back(
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
					include_principal_strain->output == GPlatesFileIO::DeformationExport::PrincipalStrainOptions::STRAIN ?
							GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStrainMinorAxis") :
							GPlatesPropertyValues::ValueObjectType::create_gpml("PrincipalStretchMinorAxis"),
					// Strain/stretch has no units so don't add any "uom" XML attribute...
					GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type(),
					principal_minors.begin(),
					principal_minors.end()));
		}

		// Include dilatation strain if requested.
		if (include_dilatation_strain)
		{
			std::vector<double> dilatation_strains;
			dilatation_strains.reserve(deformation_strains.size());
			for (unsigned int d = 0; d < deformation_strains.size(); ++d)
			{
				dilatation_strains.push_back(deformation_strains[d].get_strain_dilatation());
			}

			GPlatesPropertyValues::ValueObjectType dilatation_strain_type =
					GPlatesPropertyValues::ValueObjectType::create_gpml("DilatationStrain");
			GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type dilatation_strain_xml_attrs;
			// Dilatation has no units so don't add any "uom" XML attribute.

			// Add the dilatation strain scalar values we're exporting.
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type dilatation_strain_range =
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
							dilatation_strain_type,
							dilatation_strain_xml_attrs,
							dilatation_strains.begin(),
							dilatation_strains.end());

			reconstructed_range_property->tuple_list_push_back(dilatation_strain_range);
		}

		// Include dilatation strain rate if requested.
		if (include_dilatation_strain_rate)
		{
			std::vector<double> dilatation_strain_rates;
			dilatation_strain_rates.reserve(deformation_strain_rates.size());
			for (unsigned int d = 0; d < deformation_strain_rates.size(); ++d)
			{
				dilatation_strain_rates.push_back(deformation_strain_rates[d].get_strain_rate_dilatation());
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

		// Include second invariant strain rate if requested.
		if (include_second_invariant_strain_rate)
		{
			std::vector<double> second_invariant_strain_rates;
			second_invariant_strain_rates.reserve(deformation_strain_rates.size());
			for (unsigned int d = 0; d < deformation_strain_rates.size(); ++d)
			{
				second_invariant_strain_rates.push_back(deformation_strain_rates[d].get_strain_rate_second_invariant());
			}

			GPlatesPropertyValues::ValueObjectType second_invariant_strain_rate_type =
					GPlatesPropertyValues::ValueObjectType::create_gpml("TotalStrainRate");
			GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type second_invariant_strain_rate_xml_attrs;
			GPlatesModel::XmlAttributeName uom = GPlatesModel::XmlAttributeName::create_gpml("uom");
			GPlatesModel::XmlAttributeValue per_second("urn:x-si:v1999:uom:per_second");
			second_invariant_strain_rate_xml_attrs.insert(std::make_pair(uom, per_second));

			// Add the second invariant strain rate scalar values we're exporting.
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type second_invariant_strain_rate_range =
					GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
							second_invariant_strain_rate_type,
							second_invariant_strain_rate_xml_attrs,
							second_invariant_strain_rates.begin(),
							second_invariant_strain_rates.end());

			reconstructed_range_property->tuple_list_push_back(second_invariant_strain_rate_range);
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

		if (!GPlatesModel::ModelUtils::add_property(
				deformed_feature_geometry_feature_ref,
				range_property_name.get(),
				reconstructed_range_property))
		{
			// We probably changed the domain/range property names so we could output a scalar coverage,
			// but the feature type doesn't support them. Return without outputting the feature.
			return;
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
		boost::optional<DeformationExport::PrincipalStrainOptions> include_principal_strain,
		bool include_dilatation_strain,
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
					include_principal_strain,
					include_dilatation_strain,
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
