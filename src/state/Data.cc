/* $Id$ */

/**
 * @file 
 * This is not a file!!! WTG?
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

#include "Data.h"

GPlatesState::Data::GeoData_type *
GPlatesState::Data::_datagroup = NULL;

GPlatesState::Data::DrawableMap_type *
GPlatesState::Data::_drawable = NULL;

GPlatesState::Data::RotationMap_type *
GPlatesState::Data::_rot_hists = NULL;

GPlatesState::Data::DocumentMetaData_type *
GPlatesState::Data::_meta_data = NULL;
