/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2014 Geological Survey of Norway
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

#include <map>

#include "AgeModelCollection.h"

GPlatesAppLogic::AgeModelCollection::AgeModelCollection()
{
}

boost::optional<const GPlatesAppLogic::AgeModel &>
GPlatesAppLogic::AgeModelCollection::get_active_age_model() const
{
	if (d_active_model_index)
	{
		if (static_cast<std::size_t>(*d_active_model_index) < d_age_models.size())
		{
			return boost::optional<const GPlatesAppLogic::AgeModel &>(d_age_models.at(*d_active_model_index));
		}
	}

	return boost::none;
}

void
GPlatesAppLogic::AgeModelCollection::set_active_age_model(
		unsigned int index)
{
	if (static_cast<std::size_t>(index) < d_age_models.size())
	{
		d_active_model_index.reset(index);
	}
}

void
GPlatesAppLogic::AgeModelCollection::add_age_model(
		const GPlatesAppLogic::AgeModel &age_model)
{
	d_age_models.push_back(age_model);
}

void
GPlatesAppLogic::AgeModelCollection::add_chron_to_model(
		const QString &model_id,
		const QString &chron,
		double age)
{

}

void
GPlatesAppLogic::AgeModelCollection::add_chron_to_model(
		int index,
		const QString &chron,
		double age)
{
	if (index > static_cast<int>(d_age_models.size()))
	{
		return;
	}
	AgeModel &age_model = d_age_models.at(index);
	age_model.d_model.insert(std::make_pair(chron,age));
}

void
GPlatesAppLogic::AgeModelCollection::add_chron_metadata(
		const QString &chron,
		const QString &chron_metadata)
{
	d_chron_comments.insert(std::make_pair(chron,chron_metadata));
}


void
GPlatesAppLogic::AgeModelCollection::clear()
{
	d_age_models.clear();
	d_chron_comments.clear();
	d_filename = QString();
	d_active_model_index.reset();
	d_ordered_chrons.clear();
}

QString
GPlatesAppLogic::AgeModelCollection::get_model_id(
		int index) const
{
	if (index > static_cast<int>(d_age_models.size()))
	{
		return QString();
	}
	return d_age_models.at(index).d_identifier;
}

void
GPlatesAppLogic::AgeModelCollection::add_next_ordered_chron(
		const QString &chron)
{
	d_ordered_chrons.push_back(chron);
}

const GPlatesAppLogic::chron_comment_map_type &
GPlatesAppLogic::AgeModelCollection::get_chron_comment_map() const
{
	return d_chron_comments;
}

const GPlatesAppLogic::ordered_chron_container_type &
GPlatesAppLogic::AgeModelCollection::get_ordered_chrons() const
{
	return d_ordered_chrons;
}
