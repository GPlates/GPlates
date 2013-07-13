/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2012-02-29 17:05:33 +0100 (Wed, 29 Feb 2012) $
 *
 * Copyright (C) 2012 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_PICKFILEREADER_H
#define GPLATES_FILEIO_PICKFILEREADER_H

#include "ReadErrorAccumulation.h"

namespace GPlatesQtWidgets
{
	class HellingerModel;
}

namespace GPlatesFileIO
{

class HellingerReader
{
public:
    HellingerReader();
#if 0
    /**
     * Provide a static read_file method for conformance with other file-io methods. Unsure
     * if we will need to instantiate a class here.
     *
     * All picks contained in the same pick file will share the same moving/fixed plate ids, and same
     * chron time.
     */
    static
    void
    read_pick_file(
            File::Reference &file,
            GPlatesModel::ModelInterface &model,
            ReadErrorAccumulation &read_errors,
            const GPlatesModel::integer_plate_id_type &moving_plate_id,
            const GPlatesModel::integer_plate_id_type &fixed_plate_id,
            const double &chron_time);
#endif


	static
	void
	read_pick_file(
			const QString &filename,
			GPlatesQtWidgets::HellingerModel &hellinger_model,
			ReadErrorAccumulation &read_errors);

    static
    void
    read_com_file(
            const QString &filename,
            GPlatesQtWidgets::HellingerModel& hellinger_model,
            ReadErrorAccumulation &read_errors);


	static
	void
	read_error_ellipse(
			const QString &filename,
			GPlatesQtWidgets::HellingerModel& hellinger_model);
};

}
#endif // GPLATES_FILEIO_PICKFILEREADER_H
