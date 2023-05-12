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

#include <boost/ref.hpp>

#include "GmlOrientableCurve.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlOrientableCurve::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("OrientableCurve");


const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesPropertyValues::GmlOrientableCurve::create(
		GmlLineString::non_null_ptr_type base_curve_,
		const xml_attributes_type &xml_attributes_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GmlOrientableCurve(transaction, base_curve_, xml_attributes_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GmlOrientableCurve::set_base_curve(
		GmlLineString::non_null_ptr_type bc)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().base_curve.change(
			revision_handler.get_model_transaction(), bc);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlOrientableCurve::set_xml_attributes(
		const xml_attributes_type &xml_attributes)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().xml_attributes = xml_attributes;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlOrientableCurve::print_to(
		std::ostream &os) const
{
	return get_current_revision<Revision>().base_curve.get_revisionable()->print_to(os);
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlOrientableCurve::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// There's only one nested property value so it must be that.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revisionable == revision.base_curve.get_revisionable(),
			GPLATES_ASSERTION_SOURCE);

	// Create a new revision for the child property value.
	return revision.base_curve.clone_revision(transaction);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlOrientableCurve::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlOrientableCurve> &gml_orientable_curve)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_orientable_curve->base_curve(), "base_curve");
		scribe.save(TRANSCRIBE_SOURCE, gml_orientable_curve->get_xml_attributes(), "xml_attributes");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GmlLineString::non_null_ptr_type> base_curve_ =
				scribe.load<GmlLineString::non_null_ptr_type>(TRANSCRIBE_SOURCE, "base_curve");
		if (!base_curve_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		xml_attributes_type xml_attributes;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, xml_attributes, "xml_attributes"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gml_orientable_curve.construct_object(
				boost::ref(transaction),  // non-const ref
				base_curve_,
				xml_attributes);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlOrientableCurve::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, base_curve(), "base_curve");
			scribe.save(TRANSCRIBE_SOURCE, get_xml_attributes(), "xml_attributes");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GmlLineString::non_null_ptr_type> base_curve_ =
					scribe.load<GmlLineString::non_null_ptr_type>(TRANSCRIBE_SOURCE, "base_curve");
			if (!base_curve_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			xml_attributes_type xml_attributes_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, xml_attributes_, "xml_attributes"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			set_base_curve(base_curve_);
			set_xml_attributes(xml_attributes_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlOrientableCurve>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
