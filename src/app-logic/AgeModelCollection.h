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
#include <QFile>
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


typedef std::vector<AgeModel> age_model_container_type;

class AgeModelCollection
{
public:
	AgeModelCollection();

	boost::optional<const AgeModel &>
	get_active_age_model() const;

	void
	set_active_age_model(
			unsigned int index);

	void
	add_age_model(
			const AgeModel &age_model);

	const QString &
	get_filename() const
	{
		return d_filename;
	}

	void
	set_filename(
			const QString &filename)
	{
		d_filename = filename;
	}

	const age_model_container_type &
	get_age_models() const
	{
		return d_age_models;
	}


private:

	age_model_container_type d_age_models;

	/**
	 * @brief d_chron_comment - additional information relating to the chron - comments, references etc
	 * Ultimately we might have several metadata fields here; for now I'm bundling everything into one QString.
	 */
	std::map<QString,QString> d_chron_metadata;

	boost::optional<unsigned int> d_active_model_index;

	/**
	 * @brief filename of file containing the loaded model collection
	 */
	QString d_filename;

};


}
#endif // GPLATES_APP_LOGIC_AGEMODELCOLLECTION_H
