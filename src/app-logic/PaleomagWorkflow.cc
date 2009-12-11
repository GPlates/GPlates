/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#include "model/ConstFeatureVisitor.h"
#include "model/ModelInterface.h"


bool
GPlatesAppLogic::PaleomagWorkflow::add_file(
		FeatureCollectionFileState::file_iterator file_iter,
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
		FeatureCollectionFileState::file_iterator file_iter)
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
		FeatureCollectionFileState::file_iterator file_iter,
		GPlatesFileIO::File &old_file,
		const ClassifyFeatureCollection::classifications_type &new_classification)
{
	// Only interested in feature collections with paleomag features.
	return PaleomagUtils::detect_paleomag_features(file_iter->get_feature_collection());
}


void
GPlatesAppLogic::PaleomagWorkflow::set_file_active(
		FeatureCollectionFileState::file_iterator file_iter,
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
		GPlatesModel::Reconstruction &reconstruction,
		const double &reconstruction_time,
		const GPlatesGui::ColourTable &colour_table)
{
	//FIXME: make this extract the a95 and/or dm and dp properties from
	// the paleomag feature(s), and render this on the globe as a circle/ellipse. 
	
	// Later we may also want to render the sample site and/or vgp in particular
	// styles (e.g. stick an arrow on the sample site; stick a box around the vgp,
	// make sites only visible, poles only visible etc.) 
	// In that case we would override the default reconstructed geometry rendering.
	
	
	// Return if there are no paleomag feature collections.
	if (d_paleomag_feature_collection_infos.empty())
	{
		return;
	}

	d_paleomag_layer->set_active();	
	d_paleomag_layer->clear_rendered_geometries();
	
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

			GPlatesModel::FeatureCollectionHandle::features_iterator iter =
				paleomag_feature_collection->features_begin();	

			GPlatesModel::FeatureCollectionHandle::features_iterator end =
				paleomag_feature_collection->features_end();								

			for ( ; iter != end; ++iter)
			{
				// Check that it's ok to dereference the iterators.
				if (iter.is_valid())
				{

					PaleomagUtils::PaleomagRenderer paleomag_renderer(
						reconstruction,
						reconstruction_time,
						d_paleomag_layer,
						colour_table,
						false /* render_as_ellipse = false */);

					paleomag_renderer.visit_feature(iter);
				}
			}
		}
		
	}
	
}
