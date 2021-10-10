/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_STRUCTUREPROPERTYCREATORMAP_H
#define GPLATES_FILEIO_STRUCTUREPROPERTYCREATORMAP_H

#include <map>
#include "property-values/TemplateTypeParameterType.h"
#include "PropertyCreationUtils.h"

namespace GPlatesFileIO {

	/**
	 * This class encapsulates a mapping from a (fully qualified) structural
	 * type name to a creation function for it.
	 *
	 *   structural type name  ----->  creation_function
	 */
	class StructurePropertyCreatorMap
	{
			typedef std::map<
					GPlatesPropertyValues::TemplateTypeParameterType, 
					PropertyCreationUtils::PropertyCreator> 
				StructurePropertyCreatorMapType;

		public:
			typedef StructurePropertyCreatorMapType::iterator iterator;

			static
			StructurePropertyCreatorMap *
			instance();


			iterator
			find(
					const StructurePropertyCreatorMapType::key_type &key) {
				return d_map.find(key);
			}
			

			iterator
			end() {
				return d_map.end();
			}


		private:
			StructurePropertyCreatorMapType d_map;
			static StructurePropertyCreatorMap *d_instance;

			StructurePropertyCreatorMap();
			StructurePropertyCreatorMap(
					const StructurePropertyCreatorMap &);
			StructurePropertyCreatorMap &
			operator=(const StructurePropertyCreatorMap &);
	};
}

#endif // GPLATES_FILEIO_STRUCTUREPROPERTYCREATORMAP_H
