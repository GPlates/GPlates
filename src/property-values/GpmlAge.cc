/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "GpmlAge.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "scribe/Scribe.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	boost::optional<GPlatesPropertyValues::TimescaleBand>
	convert_to_band_maybe(
			const QString &str)
	{
		if (str.isNull()) {
			return boost::none;
		}
		GPlatesPropertyValues::TimescaleBand band(GPlatesUtils::make_icu_string_from_qstring(str));
		return band;
	}

	boost::optional<GPlatesPropertyValues::TimescaleBand>
	convert_to_band_maybe(
			const boost::optional<QString> &str_maybe)
	{
		if ( ! str_maybe) {
			return boost::none;
		}
		return convert_to_band_maybe(*str_maybe);
	}
	
	boost::optional<GPlatesPropertyValues::TimescaleName>
	convert_to_name_maybe(
			const QString &str)
	{
		if (str.isNull()) {
			return boost::none;
		}
		GPlatesPropertyValues::TimescaleName name(GPlatesUtils::make_icu_string_from_qstring(str));
		return name;
	}

	boost::optional<GPlatesPropertyValues::TimescaleName>
	convert_to_name_maybe(
			const boost::optional<QString> &str_maybe)
	{
		if ( ! str_maybe) {
			return boost::none;
		}
		return convert_to_name_maybe(*str_maybe);
	}
}


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlAge::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("Age");


const GPlatesPropertyValues::GpmlAge::non_null_ptr_type
GPlatesPropertyValues::GpmlAge::create(
		boost::optional<double> age_absolute,
		boost::optional<TimescaleBand> age_named,
		boost::optional<TimescaleName> timescale,
		boost::optional<double> uncertainty_plusminus,
		boost::optional<double> uncertainty_youngest_absolute,
		boost::optional<TimescaleBand> uncertainty_youngest_named,
		boost::optional<double> uncertainty_oldest_absolute,
		boost::optional<TimescaleBand> uncertainty_oldest_named)
{
	return non_null_ptr_type(
			new GpmlAge(age_absolute, age_named, timescale, uncertainty_plusminus, 
					uncertainty_youngest_absolute, uncertainty_youngest_named,
					uncertainty_oldest_absolute, uncertainty_oldest_named));
}


const GPlatesPropertyValues::GpmlAge::non_null_ptr_type
GPlatesPropertyValues::GpmlAge::create(
		boost::optional<double> age_absolute,
		boost::optional<QString> age_named,
		boost::optional<QString> timescale,
		boost::optional<double> uncertainty_plusminus,
		boost::optional<double> uncertainty_youngest_absolute,
		boost::optional<QString> uncertainty_youngest_named,
		boost::optional<double> uncertainty_oldest_absolute,
		boost::optional<QString> uncertainty_oldest_named)
{
	return non_null_ptr_type(
			new GpmlAge(age_absolute, convert_to_band_maybe(age_named), convert_to_name_maybe(timescale), uncertainty_plusminus, 
					uncertainty_youngest_absolute, convert_to_band_maybe(uncertainty_youngest_named),
					uncertainty_oldest_absolute, convert_to_band_maybe(uncertainty_oldest_named)));
}


const GPlatesPropertyValues::GpmlAge::non_null_ptr_type
GPlatesPropertyValues::GpmlAge::create()
{
	return non_null_ptr_type(
			new GpmlAge(boost::none, boost::none, boost::none, boost::none, boost::none, boost::none, boost::none, boost::none));
}


void
GPlatesPropertyValues::GpmlAge::set_age_absolute(
		boost::optional<double> age_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().age_absolute = age_maybe;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlAge::set_age_named(
		boost::optional<GPlatesPropertyValues::TimescaleBand> age_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().age_named = age_maybe;
	revision_handler.commit();
}

void
GPlatesPropertyValues::GpmlAge::set_age_named(
		const QString &age)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().age_named = convert_to_band_maybe(age);
	revision_handler.commit();
}


GPlatesPropertyValues::GpmlAge::AgeDefinition::AgeDefinitionType
GPlatesPropertyValues::GpmlAge::age_type() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (revision.age_named)
	{
		if (revision.age_absolute)
		{
			return AgeDefinition::AGE_BOTH;
		}
		else
		{
			return AgeDefinition::AGE_NAMED;
		}
	}
	else
	{
		if (revision.age_absolute)
		{
			return AgeDefinition::AGE_ABSOLUTE;
		}
		else
		{
			return AgeDefinition::AGE_NONE;
		}
	}
}


void
GPlatesPropertyValues::GpmlAge::set_timescale(
		boost::optional<GPlatesPropertyValues::TimescaleName> timescale_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().timescale = timescale_maybe;
	revision_handler.commit();
}

void
GPlatesPropertyValues::GpmlAge::set_timescale(
		const QString &timescale)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().timescale = convert_to_name_maybe(timescale);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlAge::set_uncertainty_plusminus(
		boost::optional<double> uncertainty_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_plusminus = uncertainty_maybe;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlAge::set_uncertainty_youngest_absolute(
		boost::optional<double> uncertainty_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_youngest_absolute = uncertainty_maybe;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlAge::set_uncertainty_youngest_named(
		boost::optional<GPlatesPropertyValues::TimescaleBand> uncertainty_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_youngest_named = uncertainty_maybe;
	revision_handler.commit();
}

void
GPlatesPropertyValues::GpmlAge::set_uncertainty_youngest_named(
		const QString &uncertainty)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_youngest_named = convert_to_band_maybe(uncertainty);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlAge::set_uncertainty_oldest_absolute(
		boost::optional<double> uncertainty_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_oldest_absolute = uncertainty_maybe;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlAge::set_uncertainty_oldest_named(
		boost::optional<GPlatesPropertyValues::TimescaleBand> uncertainty_maybe)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_oldest_named = uncertainty_maybe;
	revision_handler.commit();
}

void
GPlatesPropertyValues::GpmlAge::set_uncertainty_oldest_named(
		const QString &uncertainty)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().uncertainty_oldest_named = convert_to_band_maybe(uncertainty);
	revision_handler.commit();
}


GPlatesPropertyValues::GpmlAge::UncertaintyDefinition::UncertaintyDefinitionType
GPlatesPropertyValues::GpmlAge::uncertainty_type() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (revision.uncertainty_plusminus)
	{
		return UncertaintyDefinition::UNC_PLUS_OR_MINUS;
	}
	else if (revision.uncertainty_oldest_absolute || revision.uncertainty_oldest_named ||
			revision.uncertainty_youngest_absolute || revision.uncertainty_youngest_named)
	{
		return UncertaintyDefinition::UNC_RANGE;
	}
	else
	{
		return UncertaintyDefinition::UNC_NONE;
	}
}


std::ostream &
GPlatesPropertyValues::GpmlAge::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	if (revision.age_absolute)
	{
		os << revision.age_absolute.get() << " ";
	}
	if (revision.age_named)
	{
		os << "(" << revision.age_named.get().get().qstring().toStdString() << ") ";
	}
	if (revision.uncertainty_plusminus)
	{
		os << "±" << revision.uncertainty_plusminus.get();
	}
	if (revision.uncertainty_youngest_absolute)
	{
		os << "[" << revision.uncertainty_youngest_absolute.get() << "-";
	}
	if (revision.uncertainty_youngest_named)
	{
		os << "[" << revision.uncertainty_youngest_named.get().get().qstring().toStdString() << "-";
	}
	if (revision.uncertainty_oldest_absolute)
	{
		os << revision.uncertainty_oldest_absolute.get() << "]";
	}
	if (revision.uncertainty_oldest_named)
	{
		os << revision.uncertainty_oldest_named.get().get().qstring().toStdString() << "]";
	}

	return os;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlAge::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlAge> &gpml_age)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_age_absolute(), "age_absolute");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_age_named(), "age_named");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_timescale(), "timescale");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_uncertainty_plusminus(), "uncertainty_plusminus");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_uncertainty_youngest_absolute(), "uncertainty_youngest_absolute");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_uncertainty_youngest_named(), "uncertainty_youngest_named");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_uncertainty_oldest_absolute(), "uncertainty_oldest_absolute");
		scribe.save(TRANSCRIBE_SOURCE, gpml_age->get_uncertainty_oldest_named(), "uncertainty_oldest_named");
	}
	else // loading
	{
		boost::optional<double> age_absolute_;
		boost::optional<TimescaleBand> age_named_;
		boost::optional<TimescaleName> timescale_;
		boost::optional<double> uncertainty_plusminus_;
		boost::optional<double> uncertainty_youngest_absolute_;
		boost::optional<TimescaleBand> uncertainty_youngest_named_;
		boost::optional<double> uncertainty_oldest_absolute_;
		boost::optional<TimescaleBand> uncertainty_oldest_named_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, age_absolute_, "age_absolute") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, age_named_, "age_named") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, timescale_, "timescale") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_plusminus_, "uncertainty_plusminus") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_youngest_absolute_, "uncertainty_youngest_absolute") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_youngest_named_, "uncertainty_youngest_named") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_oldest_absolute_, "uncertainty_oldest_absolute") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_oldest_named_, "uncertainty_oldest_named"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_age.construct_object(
				age_absolute_,
				age_named_,
				timescale_,
				uncertainty_plusminus_,
				uncertainty_youngest_absolute_,
				uncertainty_youngest_named_,
				uncertainty_oldest_absolute_,
				uncertainty_oldest_named_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlAge::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_age_absolute(), "age_absolute");
			scribe.save(TRANSCRIBE_SOURCE, get_age_named(), "age_named");
			scribe.save(TRANSCRIBE_SOURCE, get_timescale(), "timescale");
			scribe.save(TRANSCRIBE_SOURCE, get_uncertainty_plusminus(), "uncertainty_plusminus");
			scribe.save(TRANSCRIBE_SOURCE, get_uncertainty_youngest_absolute(), "uncertainty_youngest_absolute");
			scribe.save(TRANSCRIBE_SOURCE, get_uncertainty_youngest_named(), "uncertainty_youngest_named");
			scribe.save(TRANSCRIBE_SOURCE, get_uncertainty_oldest_absolute(), "uncertainty_oldest_absolute");
			scribe.save(TRANSCRIBE_SOURCE, get_uncertainty_oldest_named(), "uncertainty_oldest_named");
		}
		else // loading
		{
			boost::optional<double> age_absolute_;
			boost::optional<TimescaleBand> age_named_;
			boost::optional<TimescaleName> timescale_;
			boost::optional<double> uncertainty_plusminus_;
			boost::optional<double> uncertainty_youngest_absolute_;
			boost::optional<TimescaleBand> uncertainty_youngest_named_;
			boost::optional<double> uncertainty_oldest_absolute_;
			boost::optional<TimescaleBand> uncertainty_oldest_named_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, age_absolute_, "age_absolute") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, age_named_, "age_named") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, timescale_, "timescale") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_plusminus_, "uncertainty_plusminus") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_youngest_absolute_, "uncertainty_youngest_absolute") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_youngest_named_, "uncertainty_youngest_named") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_oldest_absolute_, "uncertainty_oldest_absolute") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, uncertainty_oldest_named_, "uncertainty_oldest_named"))
			{
				return scribe.get_transcribe_result();
			}

			set_age_absolute(age_absolute_);
			set_age_named(age_named_);
			set_timescale(timescale_);
			set_uncertainty_plusminus(uncertainty_plusminus_);
			set_uncertainty_youngest_absolute(uncertainty_youngest_absolute_);
			set_uncertainty_youngest_named(uncertainty_youngest_named_);
			set_uncertainty_oldest_absolute(uncertainty_oldest_absolute_);
			set_uncertainty_oldest_named(uncertainty_oldest_named_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlAge>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
