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
#include <boost/ref.hpp>

#include "GmlDataBlockCoordinateList.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/types.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeQualifiedXmlName.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "scribe/Scribe.h"


namespace
{
	bool
	double_eq(
			double d1,
			double d2)
	{
		return GPlatesMaths::Real(d1) == GPlatesMaths::Real(d2);
	}
}


void
GPlatesPropertyValues::GmlDataBlockCoordinateList::set_value_object_xml_attributes(
		const xml_attributes_type &value_object_xml_attributes)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value_object_xml_attributes = value_object_xml_attributes;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlDataBlockCoordinateList::set_coordinates(
		const coordinates_type &coordinates)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().coordinates = coordinates;
	revision_handler.commit();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlDataBlockCoordinateList::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// We don't have any child revision references so there should be no children that
	// could bubble up a modification.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlDataBlockCoordinateList::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlDataBlockCoordinateList> &gml_data_block_coord_list)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_data_block_coord_list->get_value_object_type(), "value_object_type");
		scribe.save(TRANSCRIBE_SOURCE, gml_data_block_coord_list->get_value_object_xml_attributes(), "value_object_xml_attributes");
		scribe.save(TRANSCRIBE_SOURCE, gml_data_block_coord_list->get_coordinates(), "coordinates");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<ValueObjectType> value_object_type_ = scribe.load<ValueObjectType>(TRANSCRIBE_SOURCE, "value_object_type");
		if (!value_object_type_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		xml_attributes_type value_object_xml_attributes_;
		coordinates_type coordinates_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, value_object_xml_attributes_, "value_object_xml_attributes") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, coordinates_, "coordinates"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gml_data_block_coord_list.construct_object(
				boost::ref(transaction),  // non-const ref
				value_object_type_,
				value_object_xml_attributes_,
				coordinates_.begin(),
				coordinates_.end());
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlDataBlockCoordinateList::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_value_object_type(), "value_object_type");
			scribe.save(TRANSCRIBE_SOURCE, get_value_object_xml_attributes(), "value_object_xml_attributes");
			scribe.save(TRANSCRIBE_SOURCE, get_coordinates(), "coordinates");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<ValueObjectType> value_object_type_ = scribe.load<ValueObjectType>(TRANSCRIBE_SOURCE, "value_object_type");
			if (!value_object_type_.is_valid())
			{
				return scribe.get_transcribe_result();
			}
			d_value_object_type = value_object_type_;

			xml_attributes_type value_object_xml_attributes_;
			coordinates_type coordinates_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, value_object_xml_attributes_, "value_object_xml_attributes") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, coordinates_, "coordinates"))
			{
				return scribe.get_transcribe_result();
			}
			set_value_object_xml_attributes(value_object_xml_attributes_);
			set_coordinates(coordinates_);
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


bool
GPlatesPropertyValues::GmlDataBlockCoordinateList::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (value_object_xml_attributes != other_revision.value_object_xml_attributes ||
		coordinates.size() != other_revision.coordinates.size() ||
		!std::equal(coordinates.begin(), coordinates.end(), other_revision.coordinates.begin(), &double_eq) ||
		!GPlatesModel::Revision::equality(other_revision))
	{
		return false;
	}

	return true;
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GmlDataBlockCoordinateList &gml_data_block_coordinate_list)
{
	os << convert_qualified_xml_name_to_qstring(
			gml_data_block_coordinate_list.get_value_object_type()).toStdString();

	const GmlDataBlockCoordinateList::coordinates_type &coordinates =
			gml_data_block_coordinate_list.get_coordinates();

	os << " : [ ";

	bool first = true;
	GmlDataBlockCoordinateList::coordinates_type::const_iterator coordinates_iter = coordinates.begin();
	GmlDataBlockCoordinateList::coordinates_type::const_iterator coordinates_end = coordinates.end();
	for ( ; coordinates_iter != coordinates_end; ++coordinates_iter)
	{
		if (first)
		{
			first = false;
		}
		else
		{
			os << " , ";
		}
		os << *coordinates_iter;
	}

	return os << " ]";
}
