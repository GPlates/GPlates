/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_TEXTCONTENT_H
#define GPLATES_MODEL_TEXTCONTENT_H

#include "StringSetSingletons.h"
#include "StringContentTypeGenerator.h"

namespace GPlatesModel {

	class TextContentFactory {

	public:
		static
		GPlatesUtil::StringSet &
		instance()
		{
			return StringSetSingletons::text_content_instance();
		}

	private:
		TextContentFactory();

	};

	typedef StringContentTypeGenerator<TextContentFactory> TextContent;

}

#endif  // GPLATES_MODEL_TEXTCONTENT_H
