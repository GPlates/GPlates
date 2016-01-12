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
