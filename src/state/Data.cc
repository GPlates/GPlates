/* $Id$ */

/**
 * @file 
 * This is not a file!!! WTG?
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

#include "Data.h"

GPlatesState::Data::GeoData_type *
GPlatesState::Data::_datagroup = NULL;

GPlatesState::Data::DrawableMap_type *
GPlatesState::Data::_drawable = NULL;

GPlatesState::Data::RotationMap_type *
GPlatesState::Data::_rot_hists = NULL;
