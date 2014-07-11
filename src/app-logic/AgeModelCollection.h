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

#ifndef GPLATES_APP_LOGIC_AGEMODELCOLLECTION_H
#define GPLATES_APP_LOGIC_AGEMODELCOLLECTION_H

#include <map>
#include <vector>
#include "boost/optional.hpp"
#include <QString>

namespace GPlatesAppLogic
{

	typedef std::map<QString,double> age_model_map_type;

struct AgeModel
{
	/**
	 * @brief d_identifier - A brief name for the model, for example CandeKent95
	 */
	QString d_identifier;

	/**
	 * @brief d_model - a map of chron (e.g. "2An.1ny") to time (Ma)
	 */
	age_model_map_type d_model;
};

class AgeModelCollection
{
public:
	AgeModelCollection()
	{};

	const boost::optional<AgeModel &>
	get_active_age_model() const
	{
		return boost::none;
	}

private:

	std::vector<AgeModel> d_age_models;

	/**
	 * @brief d_chron_comment - additional information relating to the chron - comments, references etc
	 * Ultimately we might have several metadata fields here; for now I'm bundling everything into one QString.
	 */
	std::map<QString,QString> d_chron_metadata;


};


}
#endif // GPLATES_APP_LOGIC_AGEMODELCOLLECTION_H
