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
