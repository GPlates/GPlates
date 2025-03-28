/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
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

#include "GpmlPolarityChronId.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlPolarityChronId::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("PolarityChronId");


void
GPlatesPropertyValues::GpmlPolarityChronId::set_era(
		const QString &era)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().era = era;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlPolarityChronId::set_major_region(
		unsigned int major_region)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().major_region = major_region;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlPolarityChronId::set_minor_region(
		const QString &minor_region)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().minor_region = minor_region;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlPolarityChronId::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	if (revision.era)
	{
		os << revision.era->toStdString() << " ";
	}
	if (revision.major_region)
	{
		os << *revision.major_region << " ";
	}
	if (revision.minor_region)
	{
		os << revision.minor_region->toStdString();
	}

	return os;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlPolarityChronId::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlPolarityChronId> &gpml_polarity_chron_id)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_polarity_chron_id->get_era(), "era");
		scribe.save(TRANSCRIBE_SOURCE, gpml_polarity_chron_id->get_major_region(), "major_region");
		scribe.save(TRANSCRIBE_SOURCE, gpml_polarity_chron_id->get_minor_region(), "minor_region");
	}
	else // loading
	{
		boost::optional<QString> era_;
		boost::optional<unsigned int> major_region_;
		boost::optional<QString> minor_region_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, era_, "era") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, major_region_, "major_region") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, minor_region_, "minor_region"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_polarity_chron_id.construct_object(era_, major_region_, minor_region_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlPolarityChronId::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_era(), "era");
			scribe.save(TRANSCRIBE_SOURCE, get_major_region(), "major_region");
			scribe.save(TRANSCRIBE_SOURCE, get_minor_region(), "minor_region");
		}
		else // loading
		{
			boost::optional<QString> era_;
			boost::optional<unsigned int> major_region_;
			boost::optional<QString> minor_region_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, era_, "era") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, major_region_, "major_region") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, minor_region_, "minor_region"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.era = era_;
			revision.major_region = major_region_;
			revision.minor_region = minor_region_;
			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlPolarityChronId>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
