/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
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

#include <iostream>

#include "GpmlFeatureReference.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeIdTypeGenerator.h"
#include "model/TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlFeatureReference::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("FeatureReference");


void
GPlatesPropertyValues::GpmlFeatureReference::set_feature_id(
		const GPlatesModel::FeatureId &feature)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().feature = feature;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlFeatureReference::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	return os << revision.feature.get();
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlFeatureReference::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlFeatureReference> &gpml_feature_reference)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_feature_reference->get_feature_id(), "feature_id");
		scribe.save(TRANSCRIBE_SOURCE, gpml_feature_reference->get_value_type(), "value_type");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::FeatureId> feature_id_ =
				scribe.load<GPlatesModel::FeatureId>(TRANSCRIBE_SOURCE, "feature_id");
		if (!feature_id_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesModel::FeatureType> value_type_ =
				scribe.load<GPlatesModel::FeatureType>(TRANSCRIBE_SOURCE, "value_type");
		if (!value_type_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_feature_reference.construct_object(feature_id_, value_type_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlFeatureReference::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_feature_id(), "feature_id");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesModel::FeatureId> feature_id_ =
					scribe.load<GPlatesModel::FeatureId>(TRANSCRIBE_SOURCE, "feature_id");
			if (!feature_id_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			set_feature_id(feature_id_);
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_value_type, "value_type"))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlFeatureReference>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
