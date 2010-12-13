/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 

#include <iostream>

#include <algorithm>
#include <map>
#include "global/CompilerWarnings.h"
// Disable Visual Studio warning "qualifier applied to reference type; ignored" in boost 1.36.0
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
POP_MSVC_WARNINGS

#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <list>
#include <vector>
#include <QFileInfo>

#include "ExportVelocityAnimationStrategy.h"

#include "model/FeatureHandle.h"
#include "model/types.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/File.h"
#include "file-io/GpmlOnePointSixOutputVisitor.h"

#include "global/NotYetImplementedException.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"

#include "property-values/GmlDataBlock.h"
#include "property-values/GmlMultiPoint.h"


// All code in this namespace was copied from "file-io/ReconstructedFeatureGeometryExportImpl.h"
// and "file-io/ReconstructedFeatureGeometryExportImpl.cc".
namespace
{
	//! Typedef for sequence of feature collection files.
	typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;

	/**
	 * Typedef for a sequence of @a MultiPointVectorField pointers.
	 */
	typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *>
		vector_field_seq_type;


	/**
	 * Groups @a MultiPointVectorField objects with their feature.
	 */
	struct FeatureGeometryGroup
	{
		FeatureGeometryGroup(
				const GPlatesModel::FeatureHandle::const_weak_ref &_feature_ref) :
			feature_ref(_feature_ref)
		{  }

		GPlatesModel::FeatureHandle::const_weak_ref feature_ref;
		vector_field_seq_type recon_feature_geoms;
	};


	/**
	 * Typedef for a sequence of @a FeatureGeometryGroup objects.
	 */
	typedef std::list<FeatureGeometryGroup> feature_geometry_group_seq_type;

	//! Typedef for sequence of file references that reference a collection of geometries.
	typedef std::vector<const GPlatesFileIO::File::Reference *> referenced_files_collection_type;


	/**
	 * Typedef for a mapping from @a File::Reference to the vector fields calculated from the
	 * contents of that file.
	 */
	typedef std::map<const GPlatesFileIO::File::Reference *, vector_field_seq_type> file_to_vector_fields_map_type;


	/**
	 * Groups @a FeatureGeometryGroup objects with their feature collection.                                                                     
	 */
	struct FeatureCollectionFeatureGroup
	{
		FeatureCollectionFeatureGroup(
			const GPlatesFileIO::File::Reference *file_ptr_):
			file_ptr(file_ptr_)
		{}

		const GPlatesFileIO::File::Reference *file_ptr;
		feature_geometry_group_seq_type feature_geometry_groups;
	};


	/**
	 * Typedef for a sequence of @a FeatureCollectionFeatureGroup objects. 
	 */
	typedef std::list<FeatureCollectionFeatureGroup> feature_collection_feature_group_seq_type;


	/**
	 * Typedef for mapping from @a FeatureHandle to the feature collection file it came from.
	 */
	typedef std::map<const GPlatesModel::FeatureHandle *, const GPlatesFileIO::File::Reference *>
		feature_handle_to_collection_map_type;

	/**
	 * Returns a list of files that reference the RFGs.
	 * Result is stored in @a referenced_files.
	 */
	void
	get_files_referenced_by_geometries(
			referenced_files_collection_type &referenced_files,
			const vector_field_seq_type &reconstructed_feature_geometry_seq,
			const files_collection_type &reconstructable_files,
			feature_handle_to_collection_map_type &feature_to_collection_map);


	/**
	 * Returns a sequence of groups of RFGs (grouped by their feature).
	 * Result is stored in @a grouped_mpvfs_seq.
	 */
	void
	group_mpvfs_with_their_feature(
			feature_geometry_group_seq_type &grouped_mpvfs_seq,
			const vector_field_seq_type &reconstructed_feature_geometry_seq);


	/**
	 * Fills @a grouped_features_seq with the sorted contents of @a grouped_mpvfs_seq.
	 * 
	 * @a grouped_mpvfs_seq is sorted by feature collection. 
	 */
	void
	group_feature_geom_groups_with_their_collection(
			const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
			feature_collection_feature_group_seq_type &grouped_features_seq,
			const feature_geometry_group_seq_type &grouped_mpvfs_seq);



	//! Convenience typedef for referenced files.
	typedef referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for active reconstructable files.
	typedef files_collection_type
			files_collection_type;

	//! Convenience typedef for sequence of RFGs.
	typedef vector_field_seq_type
			vector_field_seq_type;

	//! Convenience typedef for feature-handle-to-collection map.
	typedef feature_handle_to_collection_map_type
			feature_handle_to_collection_map_type;

	//! Convenience typedef for a sequence of grouped RFGs.
	typedef feature_geometry_group_seq_type
			feature_geometry_group_seq_type;

	class ContainsSameFilePointerPredicate: public std::unary_function<
		FeatureCollectionFeatureGroup,bool>
	{
	public:
		bool 
		operator()(
			const FeatureCollectionFeatureGroup& elem) const
		{
			return elem.file_ptr == file_ptr;
		}


		explicit
		ContainsSameFilePointerPredicate(const GPlatesFileIO::File::Reference * file_ptr_):
			file_ptr(file_ptr_){}

	private:
		const GPlatesFileIO::File::Reference *file_ptr;
	};

	/**
	 * Populates mapping of feature handle to feature collection file.
	 */
	void
	populate_feature_handle_to_collection_map(
			feature_handle_to_collection_map_type &feature_handle_to_collection_map,
			const files_collection_type &reconstructable_files)
	{
		// Iterate through the feature collections of the active reconstructable files.
		files_collection_type::const_iterator reconstructable_files_iter;
		for (reconstructable_files_iter = reconstructable_files.begin();
			reconstructable_files_iter != reconstructable_files.end();
			++reconstructable_files_iter)
		{
			const GPlatesFileIO::File::Reference *recon_file = *reconstructable_files_iter;

			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_handle =
					recon_file->get_feature_collection();

			if (!feature_collection_handle.is_valid())
			{
				continue;
			}

			// Iterate through the feature handles in the current feature collection.
			GPlatesModel::FeatureCollectionHandle::const_iterator features_iter;
			for (features_iter = feature_collection_handle->begin();
				features_iter != feature_collection_handle->end();
				++features_iter)
			{
				const GPlatesModel::FeatureHandle *feature_handle_ptr = (*features_iter).get();

				// Add feature handle key to our mapping.
				feature_handle_to_collection_map[feature_handle_ptr] = recon_file;
			}
		}
	}


	/**
	 * Returns a unique list of files that reference the visible RFGs.
	 * Result is stored in @a referenced_files.
	 */
	void
	get_unique_list_of_referenced_files(
			referenced_files_collection_type &referenced_files,
			const vector_field_seq_type &reconstructed_feature_geometry_seq,
			const feature_handle_to_collection_map_type &feature_handle_to_collection_map)
	{
		// Iterate through the list of RFGs and build up a unique list of
		// feature collection files referenced by them.
		vector_field_seq_type::const_iterator mpvf_iter;
		for (mpvf_iter = reconstructed_feature_geometry_seq.begin();
			mpvf_iter != reconstructed_feature_geometry_seq.end();
			++mpvf_iter)
		{
			const GPlatesAppLogic::MultiPointVectorField *mpvf = *mpvf_iter;
			const GPlatesModel::FeatureHandle *feature_handle_ptr = mpvf->feature_handle_ptr();

			const feature_handle_to_collection_map_type::const_iterator map_iter =
					feature_handle_to_collection_map.find(feature_handle_ptr);
			if (map_iter == feature_handle_to_collection_map.end())
			{
				continue;
			}

			const GPlatesFileIO::File::Reference *file = map_iter->second;
			referenced_files.push_back(file);
		}

		// Sort in preparation for removing duplicates.
		// We end up sorting on 'const GPlatesFileIO::File::weak_ref' objects.
		std::sort(referenced_files.begin(), referenced_files.end(), boost::lambda::_1 < boost::lambda::_2);

		// Remove duplicates.
		referenced_files.erase(
				std::unique(referenced_files.begin(), referenced_files.end()),
				referenced_files.end());
	}


	void
	get_files_referenced_by_geometries(
			referenced_files_collection_type &referenced_files,
			const vector_field_seq_type &reconstructed_feature_geometry_seq,
			const files_collection_type &reconstructable_files,
			feature_handle_to_collection_map_type &feature_handle_to_collection_map)
	{

		populate_feature_handle_to_collection_map(
				feature_handle_to_collection_map,
				reconstructable_files);

		get_unique_list_of_referenced_files(
				referenced_files,
				reconstructed_feature_geometry_seq,
				feature_handle_to_collection_map);
	}


	void
	group_mpvfs_with_their_feature(
			feature_geometry_group_seq_type &grouped_mpvfs_seq,
			const vector_field_seq_type &reconstructed_feature_geometry_seq)
	{
		// Copy sequence so we can sort the RFGs by feature.
		vector_field_seq_type mpvfs_sorted_by_feature(
				reconstructed_feature_geometry_seq);

		// Sort in preparation for grouping RFGs by feature.
		std::sort(mpvfs_sorted_by_feature.begin(), mpvfs_sorted_by_feature.end(),
			boost::lambda::bind(&GPlatesAppLogic::MultiPointVectorField::feature_handle_ptr, boost::lambda::_1) <
						boost::lambda::bind(
								&GPlatesAppLogic::MultiPointVectorField::feature_handle_ptr, boost::lambda::_2));

		const GPlatesModel::FeatureHandle *current_feature_handle_ptr = NULL;

		// Iterate through the sorted sequence and put adjacent RFGs with the same feature
		// into a group.
		vector_field_seq_type::const_iterator sorted_mpvf_iter;
		for (sorted_mpvf_iter = mpvfs_sorted_by_feature.begin();
			sorted_mpvf_iter != mpvfs_sorted_by_feature.end();
			++sorted_mpvf_iter)
		{
			const GPlatesAppLogic::MultiPointVectorField *mpvf = *sorted_mpvf_iter;
			const GPlatesModel::FeatureHandle *feature_handle_ptr = mpvf->feature_handle_ptr();

			if (feature_handle_ptr != current_feature_handle_ptr)
			{
				// Start a new group.
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
						mpvf->get_feature_ref();

				grouped_mpvfs_seq.push_back(FeatureGeometryGroup(feature_ref));

				current_feature_handle_ptr = feature_handle_ptr;
			}

			// Add the current RFG to the current feature.
			grouped_mpvfs_seq.back().recon_feature_geoms.push_back(mpvf);
		}
	}

	void
	group_feature_geom_groups_with_their_collection(
		const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
		feature_collection_feature_group_seq_type &grouped_features_seq,
		const feature_geometry_group_seq_type &grouped_mpvfs_seq)
	{

		feature_geometry_group_seq_type::const_iterator feature_iter = grouped_mpvfs_seq.begin();
		for (; feature_iter != grouped_mpvfs_seq.end() ; ++feature_iter)
		{


			GPlatesModel::FeatureHandle::const_weak_ref handle_ref = feature_iter->feature_ref;

			// Need a pointer to use the map. 
			const GPlatesModel::FeatureHandle *handle_ptr = handle_ref.handle_ptr();
			const feature_handle_to_collection_map_type::const_iterator map_iter =
				feature_handle_to_collection_map.find(handle_ptr);
			if (map_iter != feature_handle_to_collection_map.end())
			{
				const GPlatesFileIO::File::Reference *file_ptr = map_iter->second;
				
				ContainsSameFilePointerPredicate predicate(file_ptr);
				feature_collection_feature_group_seq_type::iterator it =
						std::find_if(grouped_features_seq.begin(),grouped_features_seq.end(),predicate);
				if (it != grouped_features_seq.end())
				{
					// We found the file_ref in the FeatureCollectionFeatureGroup, so add this grouped_refs_seq to it.
					it->feature_geometry_groups.push_back(*feature_iter);
				}
				else
				{
					// We have found a new collection, so create an entry in the feature_collection_feature_group_seq
					FeatureCollectionFeatureGroup group_of_features(file_ptr);
					group_of_features.feature_geometry_groups.push_back(*feature_iter);
					grouped_features_seq.push_back(group_of_features);
				}
			}
		}

	}

}


namespace
{
	QString
	substitute_placeholder(
			const QString &output_filebasename,
			const QString &placeholder,
			const QString &placeholder_replacement)
	{
		return QString(output_filebasename).replace(placeholder, placeholder_replacement);
	}

	QString
	calculate_output_basename(
			const QString &output_filename_prefix,
			const QFileInfo &cap_qfileinfo)
	{
#if 0	
		//remove the cap file extension name 
		QString cap_filename = cap_qfileinfo.fileName();
		int idx = cap_filename.lastIndexOf(".gpml",-1);
		if(-1 != idx)
		{
			cap_filename = cap_filename.left(idx);
		}
#endif

		const QString output_basename = substitute_placeholder(
				output_filename_prefix,
				GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				cap_qfileinfo.fileName());
				//cap_filename);

		return output_basename;
	}
}

const QString 
GPlatesGui::ExportVelocityAnimationStrategy::DEFAULT_MESH_VELOCITIES_FILENAME_TEMPLATE
		="velocity_colat+lon_at_%u_%0.2fMa_on_mesh-%P.gpml";
const QString 
GPlatesGui::ExportVelocityAnimationStrategy::MESH_VELOCITIES_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;
const QString 
GPlatesGui::ExportVelocityAnimationStrategy::MESH_VELOCITIES_DESC
		="Export velocity data.";

const GPlatesGui::ExportVelocityAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportVelocityAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportVelocityAnimationStrategy * ptr=
			new ExportVelocityAnimationStrategy(export_animation_context,
			cfg.filename_template()); 
	
	ptr->d_class_id="MESH_VELOCITIES_GPML";
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportVelocityAnimationStrategy::ExportVelocityAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);

	// This code is copied from "gui/ExportReconstructedGeometryAnimationStrategy.cc".
	GPlatesAppLogic::FeatureCollectionFileState &file_state =
			d_export_animation_context_ptr->view_state().get_application_state()
					.get_feature_collection_file_state();

	// From the file state, obtain the list of all currently loaded files.
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();

	// Add them to our list of loaded files.
	BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref, loaded_files)
	{
		d_loaded_files.push_back(&file_ref.get_file());
	}
}

void
GPlatesGui::ExportVelocityAnimationStrategy::set_template_filename(
		const QString &filename)
{
// 	TODO: temporary thing. the template needs to be sorted out completely later.
// 						ExportAnimationStrategy::set_template_filename(
// 								filename.toStdString().substr(
// 										0,
// 										filename.toStdString().find_first_of("<")).c_str());
	ExportAnimationStrategy::set_template_filename(filename);
}


namespace
{
	void
	insert_velocity_field_into_feature_collection(
			GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			const GPlatesAppLogic::MultiPointVectorField *velocity_field)
	{
		static const GPlatesModel::FeatureType feature_type = 
				GPlatesModel::FeatureType::create_gpml("VelocityField");

		GPlatesModel::FeatureHandle::weak_ref feature = 
				GPlatesModel::FeatureHandle::create(
						feature_collection,
						feature_type);

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
		GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type gml_data_block =
				GPlatesPropertyValues::GmlDataBlock::create();

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

		GPlatesPropertyValues::ValueObjectType velocity_lon_type =
				GPlatesPropertyValues::ValueObjectType::create_gpml("VelocityLon");
		GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type xml_attrs_velocity_lon;
		xml_attrs_velocity_lon.insert(std::make_pair(uom, cm_per_year));

		GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type velocity_lon =
				GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
						velocity_lon_type, xml_attrs_velocity_lon,
						lon_velocity_components.begin(),
						lon_velocity_components.end());

		gml_data_block->tuple_list_push_back( velocity_colat );
		gml_data_block->tuple_list_push_back( velocity_lon );

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
GPlatesGui::ExportVelocityAnimationStrategy::export_velocity_fields_to_file(
		const vector_field_seq_type &velocity_fields,
		QString filename)
{
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create(
					d_export_animation_context_ptr->view_state()
							.get_application_state()
							.get_model_interface()->root());

	vector_field_seq_type::const_iterator velocity_fields_iter = velocity_fields.begin();
	vector_field_seq_type::const_iterator velocity_fields_end = velocity_fields.end();
	for ( ; velocity_fields_iter != velocity_fields_end; ++velocity_fields_iter)
	{
		insert_velocity_field_into_feature_collection(
				feature_collection, *velocity_fields_iter);
	}

	GPlatesFileIO::FileInfo export_file_info(filename);
	GPlatesFileIO::GpmlOnePointSixOutputVisitor gpml_writer(export_file_info, false);
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection, gpml_writer);
}


QString
GPlatesGui::ExportVelocityAnimationStrategy::get_file_name_from_feature_collection_handle(
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref)
{
	GPlatesAppLogic::FeatureCollectionFileState& file_state = 
		d_export_animation_context_ptr->view_state().get_application_state().get_feature_collection_file_state();
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_file_refs =
			file_state.get_loaded_files();
	
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &loaded_file_ref,
			loaded_file_refs)
	{
		if(feature_collection_ref == loaded_file_ref.get_file().get_feature_collection())
		{
			return loaded_file_ref.get_file().get_file_info().get_display_name(false);
		}
	}
	return QString();
}


namespace
{
	void
	populate_vector_field_seq(
			vector_field_seq_type &vector_field_seq,
			const GPlatesAppLogic::ApplicationState &application_state)
	{
		using namespace GPlatesAppLogic;

		const Reconstruction &reconstruction = application_state.get_current_reconstruction();

		// Find any MultiPointVectorField objects in the reconstruction.
		// The following code is copied from "ExportResolvedTopologyAnimationStrategy.cc".
		Reconstruction::reconstruction_tree_const_iterator recon_trees_iter =
				reconstruction.begin_reconstruction_trees();
		Reconstruction::reconstruction_tree_const_iterator recon_trees_end =
				reconstruction.end_reconstruction_trees();
		for ( ; recon_trees_iter != recon_trees_end; ++recon_trees_iter)
		{
			const ReconstructionTree::non_null_ptr_to_const_type &recon_tree =
					*recon_trees_iter;
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					reconstruction.begin_reconstruction_geometries(recon_tree),
					reconstruction.end_reconstruction_geometries(recon_tree),
					vector_field_seq);
		}
		// Temporarily patch a small bug relating to the 'd_reconstruction_tree_map' member
		// in class Reconstruction.  This bug results in duplicate pointers in
		// 'resolved_geom_seq'.
		std::set<const GPlatesAppLogic::MultiPointVectorField *> unique_vector_field_set(
				vector_field_seq.begin(), vector_field_seq.end());
		vector_field_seq.assign(unique_vector_field_set.begin(), unique_vector_field_set.end());

#if 0  // Just for testing/demonstration.
		vector_field_seq_type::const_iterator iter = vector_field_seq.begin();
		vector_field_seq_type::const_iterator end = vector_field_seq.end();
		for ( ; iter != end; ++iter) {
			std::cerr << *iter << ", FeatureHandle at " << (*iter)->feature_handle_ptr()
					<< ", feature type = "
					<< (*iter)->feature_handle_ptr()->feature_type().build_aliased_name()
					<< std::endl;
		}
#endif
	}


	void
	match_files_to_velocity_fields(
			file_to_vector_fields_map_type &file_to_vector_fields_map,
			const vector_field_seq_type &vector_field_seq,
			const files_collection_type &active_files)
	{
		// This code copied from "file-io/ReconstructedFeatureGeometryExportImpl.cc".
		feature_handle_to_collection_map_type feature_handle_to_collection_map;
		populate_feature_handle_to_collection_map(feature_handle_to_collection_map,
				active_files);

		feature_geometry_group_seq_type feature_geometry_group_seq;
		group_mpvfs_with_their_feature(feature_geometry_group_seq, vector_field_seq);

		feature_collection_feature_group_seq_type feature_collection_feature_group_seq;
		group_feature_geom_groups_with_their_collection(feature_handle_to_collection_map,
				feature_collection_feature_group_seq, feature_geometry_group_seq);

		feature_collection_feature_group_seq_type::const_iterator iter =
				feature_collection_feature_group_seq.begin();
		feature_collection_feature_group_seq_type::const_iterator end =
				feature_collection_feature_group_seq.end();
		for ( ; iter != end; ++iter)
		{
			const GPlatesFileIO::File::Reference *file_ptr = iter->file_ptr;
			vector_field_seq_type per_file_vector_field_seq;

			feature_geometry_group_seq_type::const_iterator iter2 =
					iter->feature_geometry_groups.begin();
			feature_geometry_group_seq_type::const_iterator end2 =
					iter->feature_geometry_groups.end();
			for ( ; iter2 != end2; ++iter2)
			{
				// Append all of 'iter2->recon_feature_geoms' onto the end of
				// 'per_file_vector_field_seq'.
				per_file_vector_field_seq.insert(
						per_file_vector_field_seq.end(),
						iter2->recon_feature_geoms.begin(),
						iter2->recon_feature_geoms.end());
			}

			file_to_vector_fields_map.insert(
					std::make_pair(file_ptr, per_file_vector_field_seq));
		}
	}
}


bool
GPlatesGui::ExportVelocityAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Assemble parts of this iteration's filename from the template filename sequence.
	QString output_filename_prefix = *filename_it++;
	QDir target_dir = d_export_animation_context_ptr->target_dir();

	// Get all the MultiPointVectorFields from the current reconstruction.
	vector_field_seq_type vector_field_seq;
	populate_vector_field_seq(vector_field_seq,
			d_export_animation_context_ptr->view_state().get_application_state());

	// Determine which velocity fields came from which mesh files.
	file_to_vector_fields_map_type file_to_vector_fields_map;
	match_files_to_velocity_fields(file_to_vector_fields_map, vector_field_seq, d_loaded_files);

	// For each mesh file, export the velocity fields calculated from that file.
	file_to_vector_fields_map_type::const_iterator iter = file_to_vector_fields_map.begin();
	file_to_vector_fields_map_type::const_iterator end = file_to_vector_fields_map.end();
	for ( ; iter != end; ++iter)
	{
		const QString &velocity_filename =
				iter->first->get_file_info().get_display_name(false);
		const vector_field_seq_type &vector_fields =
				iter->second;

		QString output_basename = calculate_output_basename(output_filename_prefix, velocity_filename);
		QString full_output_filename = target_dir.absoluteFilePath(output_basename);

		qDebug() << velocity_filename << "->" << full_output_filename << "<<" << vector_fields.front();

		// Next, the file writing. Update the dialog status message.
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Writing mesh velocities at frame %2 to file \"%1\"...")
				.arg(output_basename)
				.arg(frame_index) );

		export_velocity_fields_to_file(
				vector_fields,
				full_output_filename);
	}

	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportVelocityAnimationStrategy::wrap_up(
		bool export_successful)
{
	// If we need to do anything after writing a whole batch of velocity files,
	// here's the place to do it.
	// Of course, there's also the destructor, which should free up any resources
	// we acquired in the constructor; this method is intended for any "last step"
	// iteration operations that might need to occur. Perhaps all iterations end
	// up in the same file and we should close that file (if all steps completed
	// successfully).
}

const QString&
GPlatesGui::ExportVelocityAnimationStrategy::get_default_filename_template()
{
	return DEFAULT_MESH_VELOCITIES_FILENAME_TEMPLATE;
}

const QString&
GPlatesGui::ExportVelocityAnimationStrategy::get_filename_template_desc()
{
	return MESH_VELOCITIES_FILENAME_TEMPLATE_DESC;
}


// Quash warning C4503 ("decorated name length exceeded, name was truncated")
// on VS2008 with CGAL 3.6.1.
// It's here at the end of the file by a process of trial and error; the warning
// is probably being emitted here because the compiler is instantiating
// templates at the end of the file.
DISABLE_MSVC_WARNING(4503)
