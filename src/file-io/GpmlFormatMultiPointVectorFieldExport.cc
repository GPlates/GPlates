/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GpmlFormatMultiPointVectorFieldExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"
#include "GpmlOutputVisitor.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/MultiPointVectorField.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/CalculateVelocity.h"

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
	//! Convenience typedef for a sequence of MPVFs.
	typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *>
			multi_point_vector_field_seq_type;


	void
	insert_velocity_field_into_feature_collection(
			GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			const GPlatesAppLogic::MultiPointVectorField *velocity_field,
			const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
			const double &reconstruction_time)
	{
		// The domain feature used when generating the velocity field.
		const GPlatesModel::FeatureHandle::weak_ref domain_feature_ref =
				velocity_field->get_feature_ref();

		static const GPlatesModel::FeatureType feature_type = 
				GPlatesModel::FeatureType::create_gpml("VelocityField");

		GPlatesModel::FeatureHandle::weak_ref feature = 
				GPlatesModel::FeatureHandle::create(
						feature_collection,
						feature_type);

		//
		// Store the time instant at which the velocity field was generated.
		//

		static const GPlatesModel::PropertyName RECONSTRUCTED_TIME_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructedTime");

		GPlatesPropertyValues::GeoTimeInstant reconstructed_geo_time_instant(reconstruction_time);
		GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type reconstructed_gml_time_instant =
			GPlatesModel::ModelUtils::create_gml_time_instant(reconstructed_geo_time_instant);

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						RECONSTRUCTED_TIME_PROPERTY_NAME,
						reconstructed_gml_time_instant));

		//
		// Add the anchor plate id used for the reconstruction (and hence for the velocity calculations).
		//

		static const GPlatesModel::PropertyName ANCHORED_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("anchoredPlateId");

		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type anchored_gpml_plate_id =
				GPlatesPropertyValues::GpmlPlateId::create(reconstruction_anchor_plate_id);

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						ANCHORED_PLATE_ID_PROPERTY_NAME,
						anchored_gpml_plate_id));

		//
		// Store a feature snapshot reference to the domain feature.
		//
		// This is useful when the domain point is reconstructed - when no velocity surfaces
		// (like plate boundaries) were used to calculate velocities and, instead, the domain points
		// themselves are reconstructed to new positions and their plate ids used to calculate velocities.
		// In this case it can be useful to trace back to the original domain feature and hence
		// associate exported velocity fields at multiple time-steps with each other (via the
		// domain feature's feature id).
		//
		// Ultimately a time-dependent velocity property export (to a single file) might be a good idea.
		// That would require non-trivial changes to the export dialog since it currently exports
		// each time step to a separate export file.

		if (domain_feature_ref.is_valid())
		{
			static const GPlatesModel::PropertyName DOMAIN_DERIVED_FROM_PROPERTY_NAME =
					GPlatesModel::PropertyName::create_gpml("domainDerivedFrom");

			const GPlatesPropertyValues::GpmlFeatureSnapshotReference::non_null_ptr_type domain_derived_from =
					GPlatesPropertyValues::GpmlFeatureSnapshotReference::create(
							domain_feature_ref->feature_id(),
							GPlatesModel::RevisionId(),
							domain_feature_ref->feature_type());

			feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
							DOMAIN_DERIVED_FROM_PROPERTY_NAME,
							domain_derived_from));
		}

		//
		// Add the reconstruction plate id from the domain feature.
		//
		// This is a bit questionable since velocity fields can contain a different plate id at
		// each domain point (ie, one domain feature can have multiple domain points each with a
		// different plate id). This happens when surfaces (eg, plate polygons) are used in the
		// velocity layer, in which case each point's plate id is the plate id of the surface that
		// points falls within. MultiPointVectorField does store these plate ids but we currently
		// don't export them. However when no surfaces are used then the plate id of the domain
		// feature determines the velocity. So here we store the single plate id of the domain
		// feature for that particular situation (no surfaces). This plate id should be ignored
		// when surfaces are used - and is why the property has "domain" in its name.
		//

		if (domain_feature_ref.is_valid())
		{
			static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

			// Get the property value from the domain feature.
			const GPlatesPropertyValues::GpmlPlateId *domain_reconstruction_plate_id_property_value = NULL;
			if (GPlatesFeatureVisitors::get_property_value(
				domain_feature_ref,
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME,
				domain_reconstruction_plate_id_property_value))
			{
				static const GPlatesModel::PropertyName DOMAIN_RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
						GPlatesModel::PropertyName::create_gpml("domainReconstructionPlateId");

				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
								DOMAIN_RECONSTRUCTION_PLATE_ID_PROPERTY_NAME,
								domain_reconstruction_plate_id_property_value->clone()));
			}
		}

		//
		// Add the name of the domain feature.
		//
		// A perhaps questionable request since this information can be obtained via the domain
		// feature reference property added above.
		//

		if (domain_feature_ref.is_valid())
		{
			static const GPlatesModel::PropertyName NAME_PROPERTY_NAME =
					GPlatesModel::PropertyName::create_gml("name");

			// Get the property value from the domain feature.
			const GPlatesPropertyValues::XsString *name_property_value = NULL;
			if (GPlatesFeatureVisitors::get_property_value(
				domain_feature_ref,
				NAME_PROPERTY_NAME,
				name_property_value))
			{
				static const GPlatesModel::PropertyName DOMAIN_NAME_PROPERTY_NAME =
						GPlatesModel::PropertyName::create_gpml("domainName");

				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
								DOMAIN_NAME_PROPERTY_NAME,
								name_property_value->clone()));
			}
		}

		//
		// Create the "gml:domainSet" property of type GmlMultiPoint -
		// basically references "meshPoints" property in mesh node feature which
		// should be a GmlMultiPoint.
		//
		static const GPlatesModel::PropertyName domain_set_prop_name =
				GPlatesModel::PropertyName::create_gml("domainSet");

		GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type domain_set_gml_multi_point =
				GPlatesPropertyValues::GmlMultiPoint::create(
						velocity_field->multi_point());

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						domain_set_prop_name,
						domain_set_gml_multi_point));

		//
		// Set up GmlDataBlock 
		//
		GPlatesPropertyValues::GmlDataBlock::tuple_list_type gml_data_block_tuple_list;

		GPlatesPropertyValues::ValueObjectType velocity_colat_type =
				GPlatesPropertyValues::ValueObjectType::create_gpml("VelocityColat");
		GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type xml_attrs_velocity_colat;
		GPlatesModel::XmlAttributeName uom = GPlatesModel::XmlAttributeName::create_gpml("uom");
		GPlatesModel::XmlAttributeValue cm_per_year("urn:x-si:v1999:uom:cm_per_year");
		xml_attrs_velocity_colat.insert(std::make_pair(uom, cm_per_year));

		std::vector<double> colat_velocity_components;
		std::vector<double> lon_velocity_components;
		colat_velocity_components.reserve(velocity_field->domain_size());
		lon_velocity_components.reserve(velocity_field->domain_size());

		GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter =
				velocity_field->multi_point()->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator domain_end =
				velocity_field->multi_point()->end();
		GPlatesAppLogic::MultiPointVectorField::codomain_type::const_iterator codomain_iter =
				velocity_field->begin();
		for ( ; domain_iter != domain_end; ++domain_iter, ++codomain_iter) {
			if ( ! *codomain_iter) {
				// It's a "null" element.
				colat_velocity_components.push_back(0);
				lon_velocity_components.push_back(0);
				continue;
			}
			const GPlatesMaths::PointOnSphere &point = (*domain_iter);
			const GPlatesMaths::Vector3D &velocity_vector = (**codomain_iter).d_vector;

			GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon =
					GPlatesMaths::convert_vector_from_xyz_to_colat_lon(point, velocity_vector);
			colat_velocity_components.push_back(velocity_colat_lon.get_vector_colatitude().dval());
			lon_velocity_components.push_back(velocity_colat_lon.get_vector_longitude().dval());
		}

		GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type velocity_colat =
				GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
						velocity_colat_type, xml_attrs_velocity_colat,
						colat_velocity_components.begin(),
						colat_velocity_components.end());
		gml_data_block_tuple_list.push_back( velocity_colat );

		GPlatesPropertyValues::ValueObjectType velocity_lon_type =
				GPlatesPropertyValues::ValueObjectType::create_gpml("VelocityLon");
		GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type xml_attrs_velocity_lon;
		xml_attrs_velocity_lon.insert(std::make_pair(uom, cm_per_year));

		GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type velocity_lon =
				GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
						velocity_lon_type, xml_attrs_velocity_lon,
						lon_velocity_components.begin(),
						lon_velocity_components.end());
		gml_data_block_tuple_list.push_back( velocity_lon );

		//
		// Create the GmlDataBlock property
		//

		GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type gml_data_block =
				GPlatesPropertyValues::GmlDataBlock::create(gml_data_block_tuple_list);

		//
		// append the GmlDataBlock property 
		//

		GPlatesModel::PropertyName range_set_prop_name =
				GPlatesModel::PropertyName::create_gml("rangeSet");

		// add the new gml::rangeSet property
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						range_set_prop_name,
						gml_data_block));

	}
}


void
GPlatesFileIO::GpmlFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
		const std::list<multi_point_vector_field_group_type> &velocity_vector_field_group_seq,
		const QFileInfo& file_info,
		const GPlatesModel::Gpgim &gpgim,
		GPlatesModel::ModelInterface &model,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	GPlatesModel::NotificationGuard model_notification_guard(*model.access_model());

	// NOTE: We don't add to the feature store otherwise it'll remain there but
	// we want to release it (and its memory) after export.
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create();
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref = feature_collection->reference();

	// Iterate through the vector fields and write to output.
	std::list<multi_point_vector_field_group_type>::const_iterator feature_iter;
	for (feature_iter = velocity_vector_field_group_seq.begin();
		feature_iter != velocity_vector_field_group_seq.end();
		++feature_iter)
	{
		const multi_point_vector_field_group_type &feature_vector_field_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_vector_field_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Iterate through the vector fields of the current feature and write to output.
		multi_point_vector_field_seq_type::const_iterator mpvf_iter;
		for (mpvf_iter = feature_vector_field_group.recon_geoms.begin();
			mpvf_iter != feature_vector_field_group.recon_geoms.end();
			++mpvf_iter)
		{
			const GPlatesAppLogic::MultiPointVectorField *mpvf = *mpvf_iter;

			insert_velocity_field_into_feature_collection(
					feature_collection_ref,
					mpvf,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}
	}

	// The output file info.
	FileInfo output_file(file_info.filePath());

	// Write the output file by visiting the feature collection with the new velocity fields.
	GpmlOutputVisitor gpml_writer(output_file, feature_collection_ref, gpgim, false);
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(feature_collection_ref, gpml_writer);
}
