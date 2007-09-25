/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_STATE_DATA_H_
#define _GPLATES_STATE_DATA_H_

#include <list>
#include <map>

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
			typedef void* GeoData_type;

			typedef std::list< void* > DrawableDataSet;

			typedef std::map< GPlatesGlobal::rid_t,
			 DrawableDataSet > DrawableMap_type;

			typedef std::map< GPlatesGlobal::rid_t,
			 GPlatesMaths::RotationHistory > RotationMap_type;

			typedef std::map< std::string, std::string >
			 DocumentMetaData_type;

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
			 * Get a pointer to the map of meta data.
			 */
			static DocumentMetaData_type *
			GetDocumentMetaData() { return _meta_data; }

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

			/**
			 * Set the pointer to the map of the document's meta
			 * data to the map pointed to by @a meta_data.
			 */
			static void
			SetDocumentMetaData(DocumentMetaData_type *meta_data) {

				if (_meta_data)
					delete _meta_data;
				_meta_data = meta_data;
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
			 * The map of meta data associated with the current
			 * data set.
			 */ 
			static DocumentMetaData_type *_meta_data;

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
