/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATES_OPENGL_OPENGLFWD_H
#define GPLATES_OPENGL_OPENGLFWD_H

//
// This should be one of the few non-system header includes because we're only declaring
// forward declarations in this file.
//
#include "global/PointerTraits.h"


namespace GPlatesOpenGL
{
	/*
	 * Forward declarations.
	 *
	 * The purpose of this file is to reduce header dependencies to speed up compilation and
	 * to also avoid cyclic header dependencies.
	 *
	 * Use of 'PointerTraits' avoids clients having to include the respective headers just to
	 * get the 'non_null_ptr_type' and 'non_null_ptr_to_const_type' nested typedefs.
	 * However, there are times when clients will need to include some respective headers
	 * where the 'non_null_ptr_type', for example, is copied/destroyed since then the
	 * non-NULL intrusive pointer will require the definition of the class it's referring to for this.
	 * This can be minimised by passing const references to 'non_null_ptr_type' instead of
	 * passing by value (ie, instead of copying).
	 *
	 * Any specialisations of PointerTraits that are needed can also be placed in this file
	 * (for most classes should not need any specialisation) - see PointerTraits for more details.
	 *
	 * NOTE: Please keep the PointerTraits declarations alphabetically ordered.
	 * NOTE: Please avoid 'using namespace GPlatesGlobal' as this will pollute the current namespace.
	 */

	template <bool CacheProjectionTransform, bool CacheBounds, bool CacheLooseBounds, bool CacheBoundingPolygon, bool CacheLooseBoundingPolygon>
	class GLCubeSubdivisionCache;
}

#endif // GPLATES_OPENGL_OPENGLFWD_H
