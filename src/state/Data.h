/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_STATE_DATA_H_
#define _GPLATES_STATE_DATA_H_

#include <list>
#include <map>

#include "geo/DataGroup.h"
#include "geo/DrawableData.h"
#include "global/types.h"
#include "maths/RotationHistory.h"


namespace GPlatesState
{
	/**
	 * Provides access to the various data structures inside
	 * GPlates.
	 */
	class Data
	{
		public:
			typedef GPlatesGeo::DataGroup GeoData_type;

			typedef std::map< GPlatesGlobal::rid_t,
			 std::list< GPlatesGeo::DrawableData * > >
			 DrawableMap_type;

			typedef std::map< GPlatesGlobal::rid_t,
			 GPlatesMaths::RotationHistory > RotationMap_type;

			/**
			 * Get a pointer to the root of the data group. 
			 */
			static GeoData_type *
			GetDataGroup() { return _datagroup; }

			/**
			 * Get a pointer to the map of drawable data.
			 */
			static DrawableMap_type *
			GetDrawableData() { return _drawable; }

			/**
			 * Get a pointer to the map of rotation histories.
			 */
			static RotationMap_type *
			GetRotationHistories() { return _rot_hists; }

			/**
			 * Set the pointer to the root of the DataGroup to the
			 * DataGroup pointed to by @a data.
			 */
			static void
			SetDataGroup(GeoData_type *data) { 

				if (_datagroup) delete _datagroup;
				_datagroup = data; 
			}

			/**
			 * Set the pointer to the map of drawable data
			 * to the map pointer to by @a drawable.
			 */
			static void
			SetDrawableData(DrawableMap_type *drawable) {

				if (_drawable) delete _drawable;
				_drawable = drawable;
			}

			/**
			 * Set the pointer to the map of the rotation histories
			 * to the map pointed to by @a rot_hists.
			 */
			static void
			SetRotationHistories(RotationMap_type *rot_hists) {

				if (_rot_hists) delete _rot_hists;
				_rot_hists = rot_hists;
			}

		private:
			/**
			 * The main data hierarchy.
			 */
			static GeoData_type *_datagroup;

			/**
			 * The map of drawable data for each plate.
			 */
			static DrawableMap_type *_drawable;

			/**
			 * The map of rotation histories for each plate.
			 */
			static RotationMap_type *_rot_hists;

			/**
			 * Prohibit construction of this class.
			 */
			Data();
			
			/**
			 * Prohibit copying of this class.
			 */
			Data(const Data&);
	};
}

#endif  /* _GPLATES_STATE_DATA_H_ */
