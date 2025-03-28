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

#include "GpmlMetadata.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlMetadata::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("GpmlMetadata");


void
GPlatesPropertyValues::GpmlMetadata::set_data(
		const GPlatesModel::FeatureCollectionMetadata &metadata)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().metadata = metadata;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlMetadata::print_to(
		std::ostream &os) const
{
	qWarning() << "TODO: implement this function.";
	os << "TODO: implement this function.";
	return  os;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlMetadata::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlMetadata> &gpml_metadata)
{
	if (scribe.is_saving())
	{
		save_construct_data(scribe, gpml_metadata.get_object());
	}
	else // loading
	{
		GPlatesModel::FeatureCollectionMetadata metadata_;
		if (!load_construct_data(scribe, metadata_))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_metadata.construct_object(metadata_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlMetadata::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			save_construct_data(scribe, *this);
		}
		else // loading
		{
			GPlatesModel::FeatureCollectionMetadata metadata_;
			if (!load_construct_data(scribe, metadata_))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			{
				GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
				Revision &revision = revision_handler.get_revision<Revision>();
				revision.metadata = metadata_;
				revision_handler.commit();
			}
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlMetadata>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesPropertyValues::GpmlMetadata::save_construct_data(
		GPlatesScribe::Scribe &scribe,
		const GpmlMetadata &gpml_metadata)
{
	// Save the metadata.
	const GPlatesModel::FeatureCollectionMetadata &metadata = gpml_metadata.get_data();
	scribe.save(TRANSCRIBE_SOURCE, metadata.get_metadata_as_map(), "metadata");
}


bool
GPlatesPropertyValues::GpmlMetadata::load_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesModel::FeatureCollectionMetadata &metadata_)
{
	// Load the metadata.
	std::multimap<QString, QString> metadata_map;
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, metadata_map, "metadata"))
	{
		return false;
	}

	for (const auto &data : metadata_map)
	{
		metadata_.set_metadata(data.first, data.second);
	}

	return true;
}
