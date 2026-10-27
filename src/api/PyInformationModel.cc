/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "PyInformationModel.h"

#include "global/python.h"


namespace bp = boost::python;


void
export_information_model()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesApi::VerifyInformationModel::Value>(
			"VerifyInformationModel",
			"Whether to check feature types, property names and property values against the GPlates Geological Information Model (GPGIM).\n"
			"\n"
			"  Accepted by the :class:`Feature` methods that create or set properties (for example :meth:`Feature.__init__`, :meth:`Feature.add`, :meth:`Feature.set` and :meth:`Feature.set_geometry`) and by the ``Feature.create_*`` functions.\n"
			"\n"
			"  ========================== ==============\n"
			"  Value                      Description\n"
			"  ========================== ==============\n"
			"  VerifyInformationModel.yes Verify (the default). :class:`InformationModelError` is raised if a feature type is not recognised, a property name is not supported by the feature type, or a property value is not of the type the property expects.\n"
			"  VerifyInformationModel.no  Do not verify. Any property can then be added to any feature, but GPlates may not recognise the result.\n"
			"  ========================== ==============\n"
			"\n"
			"  .. seealso:: The `GPGIM <http://www.gplates.org/docs/gpgim/>`_ documentation for the feature types and their properties.\n")
			.value("yes", GPlatesApi::VerifyInformationModel::YES)
			.value("no", GPlatesApi::VerifyInformationModel::NO);
}
