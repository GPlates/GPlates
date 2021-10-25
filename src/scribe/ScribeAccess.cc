/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "ScribeAccess.h"


const GPlatesScribe::Access::export_registered_classes_type GPlatesScribe::Access::EXPORT_REGISTERED_CLASSES =
		GPlatesScribe::Access::export_register_classes();


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// NOTE: 'GPlatesScribe::Access::export_register_classes()' is defined as a macro in         //
// "ScribeExportRegistration.h" since it needs to register different classes for different   //
// programs (eg, gplates versus gplates-unit-test).                                          //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////
