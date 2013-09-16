/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "model/FeatureId.h"
#include "model/RevisionId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	const GPlatesModel::FeatureId
	feature_id_create_unique_id()
	{
		return GPlatesModel::FeatureId();
	}
}

void
export_feature_id()
{
	//
	// FeatureId - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// NOTE: Later we might wrap 'FeatureId::find_back_ref_targets()' to allow user to find feature
	// with the feature id (but for now it's probably not a good idea to expose this).
	//
	bp::class_<GPlatesModel::FeatureId>(
			"FeatureId",
			"A feature ID acts as a persistent unique identifier for a feature. "
			"Feature IDs are equality (``==``, ``!=``) comparable.\n"
			"\n"
			"The format of a feature ID is 'GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' where each "
			"*x* is a hexadecimal digit (0-9, a-f).\n",
			bp::no_init)
		.def("create_unique_id",
				&GPlatesApi::feature_id_create_unique_id,
				"create_unique_id() -> FeatureId\n"
				"  Create a unique *FeatureId* by generating a unique string identifier.\n"
				"  ::\n"
				"\n"
				"    feature_id = pygplates.FeatureId.create_unique_id()\n")
		.staticmethod("create_unique_id")
		.def("get_string",
				&GPlatesModel::FeatureId::get,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_string() -> string\n"
				"  Returns the feature identifier as a string.\n"
				"\n"
				"  :rtype: string\n")
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// For '__str__' return the string form...
		.def("__str__",
				&GPlatesModel::FeatureId::get,
				bp::return_value_policy<bp::copy_const_reference>())
	;

	// Enable boost::optional<FeatureId> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::FeatureId>();
}


namespace GPlatesApi
{
	const GPlatesModel::RevisionId
	revision_id_create_unique_id()
	{
		return GPlatesModel::RevisionId();
	}
}

void
export_revision_id()
{
	//
	// RevisionId - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesModel::RevisionId>(
			"RevisionId",
			"A revision ID acts as a persistent unique identifier for a feature. "
			"Revision IDs are equality (``==``, ``!=``) comparable.\n"
			"\n"
			"The format of a revision ID is 'GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' where each "
			"*x* is a hexadecimal digit (0-9, a-f).\n",
			bp::no_init)
		.def("create_unique_id",
				&GPlatesApi::revision_id_create_unique_id,
				"create_unique_id() -> RevisionId\n"
				"  Create a unique *RevisionId* by generating a unique string identifier.\n"
				"  ::\n"
				"\n"
				"    revision_id = pygplates.RevisionId.create_unique_id()\n")
		.staticmethod("create_unique_id")
		.def("get_string",
				&GPlatesModel::RevisionId::get,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_string() -> string\n"
				"  Returns the revision identifier as a string.\n"
				"\n"
				"  :rtype: string\n")
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// For '__str__' return the string form...
		.def("__str__",
				&GPlatesModel::RevisionId::get,
				bp::return_value_policy<bp::copy_const_reference>())
	;

	// Enable boost::optional<RevisionId> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::RevisionId>();
}

void
export_ids()
{
	export_feature_id();
	export_revision_id();
}

#endif // GPLATES_NO_PYTHON
