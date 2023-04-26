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
//The "global/python.h" must stay at the top. 
//Otherwise, there will be strange compiling errors on Mac.
#include "global/python.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"
#include "PythonPickle.h"

#include "model/FeatureId.h"
#include "model/RevisionId.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	const GPlatesModel::FeatureId
	feature_id_create_unique_id()
	{
		return GPlatesModel::FeatureId();
	}

	bp::object
	feature_id_hash(
			const GPlatesModel::FeatureId &feature_id)
	{
		// Use the Python built-in 'hash()' function on the string form of the feature ID.
		return bp::object(PyObject_Hash(bp::object(feature_id.get()).ptr()));
	}

	typedef boost::shared_ptr<GPlatesModel::FeatureId> feature_id_pickle_type;

	namespace PythonPickle
	{
		/**
		 * We specialize transcribing (for pickling) of FeatureId to avoid implementing a general transcribe
		 * of FeatureId (since not sure the best way to do that with FeatureId back-refs).
		 */
		template <>
		class Transcribe<feature_id_pickle_type>
		{
		public:

			static
			void
			pickle(
					GPlatesScribe::Scribe &scribe,
					const feature_id_pickle_type &feature_id)
			{
				scribe.save(TRANSCRIBE_SOURCE, feature_id->get(), "feature_id");
			}

			static
			feature_id_pickle_type
			unpickle(
					GPlatesScribe::Scribe &scribe)
			{
				GPlatesScribe::LoadRef<GPlatesUtils::UnicodeString> feature_id =
						scribe.load<GPlatesUtils::UnicodeString>(TRANSCRIBE_SOURCE, "feature_id");
				GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnsupportedVersion>(
						feature_id.is_valid(),
						GPLATES_ASSERTION_SOURCE);

				return feature_id_pickle_type(new GPlatesModel::FeatureId(feature_id));
			}
		};
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
			"A feature ID acts as a persistent unique identifier for a feature.\n"
			"\n"
			"Feature IDs are equality (``==``, ``!=``) comparable and "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"The format of a feature ID is 'GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' where each "
			"*x* is a hexadecimal digit (0-9, a-f).\n",
			bp::no_init)
		.def("create_unique_id",
				&GPlatesApi::feature_id_create_unique_id,
				"create_unique_id()\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a unique *FeatureId* by generating a unique string identifier.\n"
				"\n"
				"  :rtype: :class:`FeatureId`\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature_id = pygplates.FeatureId.create_unique_id()\n"
				"\n"
				"  .. note:: You cannot generate a feature ID using a specific string - it must be "
				"randomly generated. You can however compare an existing feature ID with a specific "
				"string - see :meth:`get_string`.\n")
		.staticmethod("create_unique_id")
		.def("get_string",
				&GPlatesModel::FeatureId::get,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_string()\n"
				"  Returns the feature identifier as a string.\n"
				"\n"
				"  :rtype: string\n"
				"\n"
				"  To determine if a feature has a particular feature ID:\n"
				"  ::\n"
				"\n"
				"    if feature.get_feature_id().get_string() == 'GPlates-63b81b91-b7a0-4ad7-908d-16db3c70e6ed':\n"
				"      ...\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.def("__hash__", &GPlatesApi::feature_id_hash)
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// For '__str__' return the string form...
		.def("__str__",
				&GPlatesModel::FeatureId::get,
				bp::return_value_policy<bp::copy_const_reference>())
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::feature_id_pickle_type>())
	;

	// Enable boost::optional<FeatureId> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesModel::FeatureId>();
}


namespace GPlatesApi
{
	const GPlatesModel::RevisionId
	revision_id_create_unique_id()
	{
		return GPlatesModel::RevisionId();
	}

	bp::object
	revision_id_hash(
			const GPlatesModel::RevisionId &revision_id)
	{
		// Use the Python built-in 'hash()' function on the string form of the revision ID.
		return bp::object(PyObject_Hash(bp::object(revision_id.get()).ptr()));
	}
}

void
export_revision_id()
{
	// Not including RevisionId yet since it is not really needed in the python API user (we can add it later though)...
#if 0
	//
	// RevisionId - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesModel::RevisionId>(
			"RevisionId",
			"A revision ID acts as a persistent unique identifier for a feature.\n"
			"\n"
			"Revision IDs are equality (``==``, ``!=``) comparable and "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"The format of a revision ID is 'GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' where each "
			"*x* is a hexadecimal digit (0-9, a-f).\n",
			bp::no_init)
		.def("create_unique_id",
				&GPlatesApi::revision_id_create_unique_id,
				"create_unique_id()\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a unique *RevisionId* by generating a unique string identifier.\n"
				"\n"
				"  :rtype: :class:`RevisionId`\n"
				"\n"
				"  ::\n"
				"\n"
				"    revision_id = pygplates.RevisionId.create_unique_id()\n")
		.staticmethod("create_unique_id")
		.def("get_string",
				&GPlatesModel::RevisionId::get,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_string()\n"
				"  Returns the revision identifier as a string.\n"
				"\n"
				"  :rtype: string\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.def("__hash__", &GPlatesApi::revision_id_hash)
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// For '__str__' return the string form...
		.def("__str__",
				&GPlatesModel::RevisionId::get,
				bp::return_value_policy<bp::copy_const_reference>())
	;

	// Enable boost::optional<RevisionId> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesModel::RevisionId>();
#endif
}

void
export_ids()
{
	export_feature_id();
	export_revision_id();
}
