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

#ifndef GPLATES_FILEIO_PICKFILEWRITER_H
#define GPLATES_FILEIO_PICKFILEWRITER_H


namespace GPlatesQtWidgets
{
	class HellingerModel;
}

namespace GPlatesFileIO
{

class HellingerWriter
{
public:
    HellingerWriter();

    /**
	 *
     */
    static
    void
    write_pick_file(
			const QString &filename,
			GPlatesQtWidgets::HellingerModel& hellinger_model,
			bool export_disabled_picks = true);


};

}
#endif // GPLATES_FILEIO_PICKFILEWRITER_H
