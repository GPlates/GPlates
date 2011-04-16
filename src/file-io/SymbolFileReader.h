/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SYMBOLFILEREADER_H
#define GPLATES_FILEIO_SYMBOLFILEREADER_H

#include "gui/Symbol.h"

namespace GPlatesFileIO
{


    /**
     * Class for reading a simple symbol file, and using
     * the content to
     * fill the @a symbol_map
     * as appropriate.
     *
     */
    class SymbolFileReader
    {
    public:

       static
       void
       read_file(
	   const QString &filename,
	   GPlatesGui::symbol_map_type &symbol_map);
    };

}

#endif // GPLATES_FILEIO_SYMBOLFILEREADER_H
