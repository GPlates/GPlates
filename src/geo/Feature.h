/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_GEO_FEATURE_H
#define GPLATES_GEO_FEATURE_H

#include <list>
#include "ReconstructedFeature.h"
#include "Property.h"
#include "Decoration.h"
#include "maths/FiniteRotationSnapshotTable.h"

namespace GPlatesGeo {

	class Feature {

		public:

			virtual
			~Feature();

			/**
			 * Create a reconstruction of this Feature.
			 *
			 * @return NULL if this Feature does not exist at the 
			 *  time of @a table, or else return the newly created 
			 *  representation for this Feature at the time of 
			 *  @a table.  The @b caller is responsible for the deletion
			 *  of this object.
			 */
			virtual
			ReconstructedFeature *
			reconstruct(
			 const GPlatesMaths::FiniteRotationSnapshotTable &table) = 0;

		private:

			typedef std::list< Property * > PropertyCollection;

			/**
			 * The collection of properties associated with this feature.
			 */
			PropertyCollection m_properties;

			/**
			 * An iterator into @a m_properties which refers to the 
			 * Property currently used for the decoration of the display 
			 * of this Feature.
			 */
			PropertyCollection::const_iterator m_selected_property;

			/**
			 * The way we wish to represent this Feature on the Globe.
			 */
			Decoration m_decoration;

	};
}

#endif
