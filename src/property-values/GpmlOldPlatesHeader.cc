/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GpmlOldPlatesHeader.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlOldPlatesHeader::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("OldPlatesHeader");


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_region_number(
		const unsigned int &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().region_number = i;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_reference_number(
		const unsigned int &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().reference_number = i;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_string_number(
		const unsigned int &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().string_number = i;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_geographic_description(
		const GPlatesUtils::UnicodeString &us)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().geographic_description = us;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_plate_id_number(
		const GPlatesModel::integer_plate_id_type &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().plate_id_number = i;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_age_of_appearance(
		const double &d)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().age_of_appearance = d;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_age_of_disappearance(
		const double &d)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().age_of_disappearance = d;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_data_type_code(
		const GPlatesUtils::UnicodeString &us)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().data_type_code = TextContent(us);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_data_type_code_number(
		const unsigned int &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().data_type_code_number = i;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_data_type_code_number_additional(
		const GPlatesUtils::UnicodeString &us)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().data_type_code_number_additional = TextContent(us);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_conjugate_plate_id_number(
		const GPlatesModel::integer_plate_id_type &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().conjugate_plate_id_number = i;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_colour_code(
		const unsigned int &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().colour_code = i;
	revision_handler.commit();
}

	
void
GPlatesPropertyValues::GpmlOldPlatesHeader::set_number_of_points(
		const unsigned int &i)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().number_of_points = i;
	revision_handler.commit();
}


GPlatesUtils::UnicodeString
GPlatesPropertyValues::GpmlOldPlatesHeader::old_feature_id() const
{
	const Revision &revision = get_current_revision<Revision>();

	std::ostringstream oss;
	oss << "gplates";
	oss << "_";

	oss.width(2);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << revision.region_number;
	oss << "_";

	oss.width(2);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << revision.reference_number;
	oss << "_";

	oss.width(4);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << revision.string_number;
	oss << "_";

	// NOTE: this string may have spaces in it; that's acceptable 
	oss << GPlatesUtils::make_qstring( revision.geographic_description ).toStdString();
	oss << "_";

	oss.width(3);
	oss.fill('0'); // NOTE: pad the field with ZERO in case the plate id is single digit
	oss << revision.plate_id_number;
	oss << "_";

	oss.setf(std::ios::showpoint);
	oss.setf(std::ios::fixed);
	oss.precision(1); // old_id's only have 1 decimal of precision
	oss.width(6);
	oss.fill(' '); // NOTE: DO NOT pad the field ; old_id's have spaces around ages
	oss << revision.age_of_appearance;
	oss << "_";

	oss.setf(std::ios::fixed);
	oss.setf(std::ios::showpoint);
	oss.precision(1); // old_id's only have 1 decimal of precision
	oss.width(6);
	oss.fill(' '); // NOTE: pad the field with SPACE ; old_id's have spaces around ages
	oss << revision.age_of_disappearance;
	oss << "_";

	oss << GPlatesUtils::make_qstring( revision.data_type_code ).toStdString();
	oss << "_";

	oss.width(4);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << revision.data_type_code_number;
	oss << "_";

	oss.width(3);
	oss.fill('0'); // NOTE: pad the field with ZERO in case the plate id is single digit
	oss << revision.conjugate_plate_id_number;

	oss << "_";

	return GPlatesUtils::UnicodeString(oss.str().c_str());
}


std::ostream &
GPlatesPropertyValues::GpmlOldPlatesHeader::print_to(
		std::ostream &os) const
{
	return os << old_feature_id();
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlOldPlatesHeader::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlOldPlatesHeader> &gpml_old_plates_header)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_region_number(), "region_number");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_reference_number(), "reference_number");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_string_number(), "string_number");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_geographic_description(), "geographic_description");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_plate_id_number(), "plate_id_number");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_age_of_appearance(), "age_of_appearance");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_age_of_disappearance(), "age_of_disappearance");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_data_type_code(), "data_type_code");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_data_type_code_number(), "data_type_code_number");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_data_type_code_number_additional(), "data_type_code_number_additional");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_conjugate_plate_id_number(), "conjugate_plate_id_number");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_colour_code(), "colour_code");
		scribe.save(TRANSCRIBE_SOURCE, gpml_old_plates_header->get_number_of_points(), "number_of_points");
	}
	else // loading
	{
		unsigned int region_number_;
		unsigned int reference_number_;
		unsigned int string_number_;
		GPlatesUtils::UnicodeString geographic_description_;
		GPlatesModel::integer_plate_id_type plate_id_number_;
		double age_of_appearance_;
		double age_of_disappearance_;
		GPlatesUtils::UnicodeString data_type_code_;
		unsigned int data_type_code_number_;
		GPlatesUtils::UnicodeString data_type_code_number_additional_;
		GPlatesModel::integer_plate_id_type conjugate_plate_id_number_;
		unsigned int colour_code_;
		unsigned int number_of_points_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, region_number_, "region_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, reference_number_, "reference_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, string_number_, "string_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geographic_description_, "geographic_description") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, plate_id_number_, "plate_id_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, age_of_appearance_, "age_of_appearance") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, age_of_disappearance_, "age_of_disappearance") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, data_type_code_, "data_type_code") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, data_type_code_number_, "data_type_code_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, data_type_code_number_additional_, "data_type_code_number_additional") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, conjugate_plate_id_number_, "conjugate_plate_id_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, colour_code_, "colour_code") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, number_of_points_, "number_of_points"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_old_plates_header.construct_object(
				region_number_,
				reference_number_,
				string_number_,
				geographic_description_,
				plate_id_number_,
				age_of_appearance_,
				age_of_disappearance_,
				data_type_code_,
				data_type_code_number_,
				data_type_code_number_additional_,
				conjugate_plate_id_number_,
				colour_code_,
				number_of_points_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlOldPlatesHeader::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_region_number(), "region_number");
			scribe.save(TRANSCRIBE_SOURCE, get_reference_number(), "reference_number");
			scribe.save(TRANSCRIBE_SOURCE, get_string_number(), "string_number");
			scribe.save(TRANSCRIBE_SOURCE, get_geographic_description(), "geographic_description");
			scribe.save(TRANSCRIBE_SOURCE, get_plate_id_number(), "plate_id_number");
			scribe.save(TRANSCRIBE_SOURCE, get_age_of_appearance(), "age_of_appearance");
			scribe.save(TRANSCRIBE_SOURCE, get_age_of_disappearance(), "age_of_disappearance");
			scribe.save(TRANSCRIBE_SOURCE, get_data_type_code(), "data_type_code");
			scribe.save(TRANSCRIBE_SOURCE, get_data_type_code_number(), "data_type_code_number");
			scribe.save(TRANSCRIBE_SOURCE, get_data_type_code_number_additional(), "data_type_code_number_additional");
			scribe.save(TRANSCRIBE_SOURCE, get_conjugate_plate_id_number(), "conjugate_plate_id_number");
			scribe.save(TRANSCRIBE_SOURCE, get_colour_code(), "colour_code");
			scribe.save(TRANSCRIBE_SOURCE, get_number_of_points(), "number_of_points");
		}
		else // loading
		{
			unsigned int region_number_;
			unsigned int reference_number_;
			unsigned int string_number_;
			GPlatesUtils::UnicodeString geographic_description_;
			GPlatesModel::integer_plate_id_type plate_id_number_;
			double age_of_appearance_;
			double age_of_disappearance_;
			GPlatesUtils::UnicodeString data_type_code_;
			unsigned int data_type_code_number_;
			GPlatesUtils::UnicodeString data_type_code_number_additional_;
			GPlatesModel::integer_plate_id_type conjugate_plate_id_number_;
			unsigned int colour_code_;
			unsigned int number_of_points_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, region_number_, "region_number") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, reference_number_, "reference_number") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, string_number_, "string_number") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, geographic_description_, "geographic_description") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, plate_id_number_, "plate_id_number") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, age_of_appearance_, "age_of_appearance") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, age_of_disappearance_, "age_of_disappearance") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, data_type_code_, "data_type_code") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, data_type_code_number_, "data_type_code_number") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, data_type_code_number_additional_, "data_type_code_number_additional") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, conjugate_plate_id_number_, "conjugate_plate_id_number") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, colour_code_, "colour_code") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, number_of_points_, "number_of_points"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.region_number = region_number_;
			revision.reference_number = reference_number_;
			revision.string_number = string_number_;
			revision.geographic_description = geographic_description_;
			revision.plate_id_number = plate_id_number_;
			revision.age_of_appearance = age_of_appearance_;
			revision.age_of_disappearance = age_of_disappearance_;
			revision.data_type_code = data_type_code_;
			revision.data_type_code_number = data_type_code_number_;
			revision.data_type_code_number_additional = data_type_code_number_additional_;
			revision.conjugate_plate_id_number = conjugate_plate_id_number_;
			revision.colour_code = colour_code_;
			revision.number_of_points = number_of_points_;
			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlOldPlatesHeader>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
