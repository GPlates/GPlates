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
 */

#ifndef _GPLATES_STATE_STATE_H_
#define _GPLATES_STATE_STATE_H_

#include "geo/DataGroup.h"

namespace GPlatesState
{
	/**
	 * Provides access to the various data structures inside
	 * GPlates.
	 */
	class Data
	{
		public:
			/**
			 * Get a pointer to the root of the data group. 
			 */
			static GPlatesGeo::DataGroup*
			GetDataGroup()  { return _datagroup; }
	
			/**
			 * Set the pointer to the root of the DataGroup to the
			 * DataGroup pointed to by @a data.
			 */
			static void
			SetDataGroup(GPlatesGeo::DataGroup* data) { 
				_datagroup = data; 
			}

		private:
			/**
			 * The main data hierarchy.
			 */
			static GPlatesGeo::DataGroup* _datagroup;

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

#endif  /* _GPLATES_STATE_STATE_H_ */
