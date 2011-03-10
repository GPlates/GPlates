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

#ifndef GPLATES_FILEIO_ARBITRARYXMLREADER_H
#define GPLATES_FILEIO_ARBITRARYXMLREADER_H

#include <boost/shared_ptr.hpp>

#include "ArbitraryXmlProfile.h"
#include "File.h"
#include "FileInfo.h"
#include "ReadErrorAccumulation.h"

#include "model/ModelInterface.h"

namespace GPlatesFileIO
{

	class ArbitraryXmlReader
	{
	public:
		static
		void
		read_file(
				const File::Reference &file,
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);

		static
		void
		read_xml_data(
				const File::Reference &file,
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				QByteArray& data);
		
	private:
		
	};
}

#endif  // GPLATES_FILEIO_ARBITRARYXMLREADER_H
