/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GSMLPROPERTYHANDLERS_H
#define GPLATES_FILEIO_GSMLPROPERTYHANDLERS_H

#include <QString>
#include <QBuffer>

#include "model/FeatureHandle.h"

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;

	class GsmlPropertyHandlers
	{
	public:
		explicit
		GsmlPropertyHandlers(
				GPlatesModel::FeatureHandle::weak_ref fh);
		
		/*
		* Parse geometry data and create geometry property in GPlates model
		*/
		void
		handle_geometry_property(
				QBuffer&);

		/*
		* Parse observation method data.
		*/
		void
		handle_observation_method(
				QBuffer&);

		/*
		* Parse gml:name data.
		*/
		void
		handle_gml_name(
				QBuffer&);
		
		/*
		* Parse gml:description data.
		*/
		void
		handle_gml_desc(
				QBuffer&);

		/*
		* Parse occurrence property.
		*/
		void
		handle_occurrence_property(
				QBuffer&);

		/*
		* Copy the gml:validTime property.
		*/
		void
		handle_gml_valid_time(
				QBuffer&);

		void
		handle_gpml_valid_time_range(
				QBuffer&);

		void handle_gpml_rock_type(QBuffer&);

		void handle_gpml_rock_max_thick(QBuffer&);

		void handle_gpml_rock_min_thick(QBuffer&);

		void handle_gpml_fossil_diversity(QBuffer&);

	protected:
		void
		process_geometries(
				QBuffer&,
				const QString&);

	private:
		GPlatesModel::FeatureHandle::weak_ref d_feature;
		ReadErrorAccumulation* d_read_errors;
	};
}

#endif  // GPLATES_FILEIO_GSMLPROPERTYHANDLERS_H


