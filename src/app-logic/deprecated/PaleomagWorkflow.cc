/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <algorithm>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "PaleomagWorkflow.h"

#include "AppLogicUtils.h"
#include "PaleomagUtils.h"
#include "ReconstructedFeatureGeometry.h"

#include "feature-visitors/GeometryFinder.h"
#include "gui/ColourProxy.h"
#include "model/FeatureVisitor.h"
#include "model/ModelInterface.h"
#include "property-values/GpmlPlateId.h"


namespace
{
	const GPlatesGui::ColourProxy
	get_colour_from_feature(
		const GPlatesModel::FeatureCollectionHandle::iterator feature_iterator)
	{
		GPlatesModel::PropertyName vgp_name = 
			GPlatesModel::PropertyName::create_gpml("polePosition");
		GPlatesFeatureVisitors::GeometryFinder finder(vgp_name);

		finder.visit_feature(feature_iterator);

		// FIXME: This is a hack to get the same colour as the rendered geometry of this feature. To access
		// the colour via the ColourTable->lookup functions we need a ReconstructionGeometry, so I'm creating
		// a temporary reconstruction geometry just for the purpose of grabbing the appropriate colour.
		//
		// If we later take control of the site and vgp rendering in this class, then we'll have to go through
		// this RFG creation process anyway.
		//
		// If instead we later have a separate RFG->RenderedGeometry style Workflow, then we'd also be able to 
		// access the RFG's colour there.

		if (!finder.has_found_geometries())
		{
			return GPlatesGui::ColourProxy(boost::none);
		}
		
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
			*(finder.found_geometries_begin());

		const GPlatesPropertyValues::GpmlPlateId *plate_id = NULL;
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		boost::optional<GPlatesModel::integer_plate_id_type> optional_plate_id;
		if (GPlatesFeatureVisitors::get_property_value((*feature_iterator)->reference(), plate_id_property_name,plate_id))
		{
			optional_plate_id.reset(plate_id->value());
		}

		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type rfg = 
			GPlatesAppLogic::ReconstructedFeatureGeometry::create(
				geometry,
				**feature_iterator,
				(*feature_iterator)->begin(),
				optional_plate_id,
				boost::none);

		return GPlatesGui::ColourProxy(rfg);
	}
}

int
GPlatesAppLogic::PaleomagWorkflow::s_instance_number = 0;

GPlatesAppLogic::FeatureCollectionWorkflow::tag_type
GPlatesAppLogic::PaleomagWorkflow::get_tag() const
{
	// FIXME: We're only doing this because each ViewState wants to have its own workflow instances
	QString id;
	id.setNum(d_instance_number);
	return QString("PaleomagWorkflow").append(id);
}

bool
GPlatesAppLogic::PaleomagWorkflow::add_file(
		FeatureCollectionFileState::file_reference file_iter,
		const ClassifyFeatureCollection::classifications_type &classification,
		bool /*used_by_higher_priority_workflow*/)
{
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file_iter->get_feature_collection();

	// Only interested in feature collections with paloemag features
	if (!PaleomagUtils::detect_paleomag_features(feature_collection))
	{
		return false;
	}
	
	d_paleomag_feature_collection_infos.push_back(
			PaleomagFeatureCollectionInfo(file_iter));

	return true;
}


void
GPlatesAppLogic::PaleomagWorkflow::remove_file(
		FeatureCollectionFileState::file_reference file_iter)
{
	using boost::lambda::_1;

	// Try removing it from the velocity feature collections.
	d_paleomag_feature_collection_infos.erase(
			std::remove_if(
					d_paleomag_feature_collection_infos.begin(),
					d_paleomag_feature_collection_infos.end(),
					boost::lambda::bind(&PaleomagFeatureCollectionInfo::d_file_iterator, _1)
							== file_iter),
			d_paleomag_feature_collection_infos.end());
}


bool
GPlatesAppLogic::PaleomagWorkflow::changed_file(
		FeatureCollectionFileState::file_reference file_iter,
		GPlatesFileIO::File &old_file,
		const ClassifyFeatureCollection::classifications_type &new_classification)
{
	// Only interested in feature collections with paleomag features.
	return PaleomagUtils::detect_paleomag_features(file_iter->get_feature_collection());
}


void
GPlatesAppLogic::PaleomagWorkflow::set_file_active(
		FeatureCollectionFileState::file_reference file_iter,
		bool activate)
{
	using boost::lambda::_1;

	paleomag_feature_collection_info_seq_type::iterator iter = std::find_if(
			d_paleomag_feature_collection_infos.begin(),
			d_paleomag_feature_collection_infos.end(),
			boost::lambda::bind(&PaleomagFeatureCollectionInfo::d_file_iterator, _1)
					== file_iter);

	if (iter != d_paleomag_feature_collection_infos.end())
	{
		iter->d_active = activate;
	}
}


void
GPlatesAppLogic::PaleomagWorkflow::draw_paleomag_features(
		Reconstruction &reconstruction,
		const double &reconstruction_time)
{
	
	// Later we may also want to render the sample site and/or vgp in particular
	// styles (e.g. stick an arrow on the sample site; stick a box around the vgp,
	// make sites only visible, poles only visible etc.) 
	// In that case we would override the default reconstructed geometry rendering.
	
	d_paleomag_layer->set_active();	
	d_paleomag_layer->clear_rendered_geometries();	
	
	// Return if there are no paleomag feature collections.
	if (d_paleomag_feature_collection_infos.empty())
	{
		return;
	}

	// Iterate over all our velocity field feature collections and solve velocities.
	paleomag_feature_collection_info_seq_type::iterator paleomag_feature_collection_iter;
	for (paleomag_feature_collection_iter = d_paleomag_feature_collection_infos.begin();
		paleomag_feature_collection_iter != d_paleomag_feature_collection_infos.end();
		++paleomag_feature_collection_iter)
	{
		// Only interested in active files.
		if (!paleomag_feature_collection_iter->d_active)
		{
			continue;
		}	
		
		const GPlatesModel::FeatureCollectionHandle::weak_ref paleomag_feature_collection =
			paleomag_feature_collection_iter->d_file_iterator->get_feature_collection();
		
		if (paleomag_feature_collection.is_valid())
		{

			GPlatesModel::FeatureCollectionHandle::iterator iter =
				paleomag_feature_collection->begin();	

			GPlatesModel::FeatureCollectionHandle::iterator end =
				paleomag_feature_collection->end();								




			for ( ; iter != end; ++iter)
			{
				boost::optional<GPlatesMaths::Rotation> additional_rotation;

				const GPlatesGui::ColourProxy colour = get_colour_from_feature(iter);

				PaleomagUtils::VgpRenderer vgp_renderer(
					reconstruction,
					additional_rotation, 
					d_paleomag_layer,
					colour,
					d_view_state_ptr,
					true /* should add geometries to reconstruction */
					);

				vgp_renderer.visit_feature(iter);
			}
		
		}
	}
	
}
