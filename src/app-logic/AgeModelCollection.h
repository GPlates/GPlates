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
#include "boost/noncopyable.hpp"
#include "boost/optional.hpp"
#include <QFile>
#include <QString>

namespace GPlatesAppLogic
{

typedef std::map<QString,double> age_model_map_type;
//typedef std::vector<std::pair<QString,double> > age_model_map_type;
typedef std::pair<QString,double> age_model_pair_type;
typedef std::vector<QString> ordered_chron_container_type;

struct AgeModel
{
	AgeModel()
	{};

	AgeModel(
			const QString &model_id):
		d_identifier(model_id)
	{}

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

class AgeModelCollection:
		public boost::noncopyable
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

	void
	add_chron_to_model(
			const QString &model_id,
			const QString &chron,
			double age);


	void
	add_chron_to_model(
			int index,
			const QString &chron,
			double age);

	void
	add_chron_metadata(
			const QString &chron,
			const QString &chron_metadata);

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

	void
	set_age_models(
			const age_model_container_type &models)
	{
		d_age_models = models;
	}

	int
	number_of_age_models() const
	{
		return static_cast<unsigned int>(d_age_models.size());
	}

	void
	clear();

	QString
	get_model_id(
			int index) const;

	void
	add_next_ordered_chron(
			const QString &chron);

	const std::vector<QString> &
	get_ordered_chrons() const;

private:

	age_model_container_type d_age_models;

	/**
	 * @brief d_chron_comment - additional information relating to the chron - comments, references etc
	 * Ultimately we might have several metadata fields here; for now I'm bundling everything into one QString.
	 */
	std::map<QString,QString> d_chron_metadata;

	boost::optional<unsigned int> d_active_model_index;

	/**
	 * @brief Name of file from which the age models were imported.
	 */
	QString d_filename;

	/**
	 * @brief An ordered vector of chrons, from youngest to oldest.
	 *
	 * The chrons are stored as QStrings, and their default sorted order
	 * would not be chronological (e.g. 2ny would come after 2An.1ny...)
	 *
	 * While it might be possible to set up some sort of customised sort so that
	 * they're displayed in chronological order, here I'm taking a more brute
	 * force approach and storing the sorted order here.
	 *
	 * We assume that the order provided in the age model text file is chronological;
	 * for each chron line in the file, we add a new chron to d_ordered_chrons.
	 *
	 */
	std::vector<QString> d_ordered_chrons;

};


}
#endif // GPLATES_APP_LOGIC_AGEMODELCOLLECTION_H
