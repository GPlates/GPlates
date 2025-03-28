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

#include "GpmlPropertyDelegate.h"

#include "model/TranscribeQualifiedXmlName.h"
#include "model/TranscribeIdTypeGenerator.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlPropertyDelegate::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("PropertyDelegate");


std::ostream &
GPlatesPropertyValues::GpmlPropertyDelegate::print_to(
		std::ostream &os) const
{
	return os << d_feature.get() << ":" << d_property_name.build_aliased_name();
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlPropertyDelegate::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlPropertyDelegate> &gpml_property_delegate)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_property_delegate->get_feature_id(), "feature_id");
		scribe.save(TRANSCRIBE_SOURCE, gpml_property_delegate->get_target_property_name(), "target_property_name");
		scribe.save(TRANSCRIBE_SOURCE, gpml_property_delegate->get_value_type(), "value_type");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::FeatureId> feature_id_ =
				scribe.load<GPlatesModel::FeatureId>(TRANSCRIBE_SOURCE, "feature_id");
		if (!feature_id_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesModel::PropertyName> target_property_name_ =
				scribe.load<GPlatesModel::PropertyName>(TRANSCRIBE_SOURCE, "target_property_name");
		if (!target_property_name_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<StructuralType> value_type_ =
				scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
		if (!value_type_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_property_delegate.construct_object(feature_id_, target_property_name_, value_type_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlPropertyDelegate::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_feature, "feature_id") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_property_name, "target_property_name") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_value_type, "value_type"))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlPropertyDelegate>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
