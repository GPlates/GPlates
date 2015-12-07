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

#ifndef GPLATES_API_PYTHONHASHDEFVISITOR_H
#define GPLATES_API_PYTHONHASHDEFVISITOR_H

#include <cstdlib> // For std::size_t
#include <boost/type_traits/is_polymorphic.hpp>

#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * A boost::python::class_ "def" visitor that sets '__hash__' to None to disable hashing
	 * and hence prevents the wrapped class from being used as a key in a python dict.
	 *
	 * When defining '__eq__' a compatible '__hash__' must be defined or made unhashable.
	 * This is because the default '__hash__' is based on 'id()' and would cause errors when used
	 * as key in a dictionary.
	 * Python 3 fixes this by automatically making unhashable if define '__eq__' only.
	 * This class, @a NoHashDefVisitor, makes a wrapped class unhashable and provides optional
	 * comparison operators based on (C++) object identity (see @a ObjectIdentityHashDefVisitor).
	 *
	 * Making a wrapped class unhashable is useful when it defines an equality operator that is not hashable
	 * such as comparing two floating-point numbers as equal if they are close enough to each other.
	 * This is unhashable because the hash is based on only one object (and hence closeness measures
	 * cannot be applied) but 'object1 == object1' requires 'hash(object1) == hash(object2)'.
	 * So all that can be done here is prevent hashing (make unhashable).
	 *
	 * An example usage is:
	 *
	 *	 bp::class_< boost::shared_ptr<GPlatesApi::GeoTimeInstant> >("GeoTimeInstant", ...)
	 *	 	...
	 *	 	.def(GPlatesApi::NoHashDefVisitor(false, false))
	 *	 	.def("__eq__", &GPlatesApi::geo_time_instant_eq)
	 *	 	.def("__ne__", &GPlatesApi::geo_time_instant_ne)
	 *	 	.def("__lt__", &GPlatesApi::geo_time_instant_lt)
	 *	 	.def("__le__", &GPlatesApi::geo_time_instant_le)
	 *	 	.def("__gt__", &GPlatesApi::geo_time_instant_gt)
	 *	 	.def("__ge__", &GPlatesApi::geo_time_instant_ge)
	 *   ;
	 */
	class NoHashDefVisitor :
			public boost::python::def_visitor<NoHashDefVisitor>
	{
	public:

		/**
		 * The parameters control whether to generate comparison operators (for object *identity*).
		 *
		 * Typically @a NoHashDefVisitor is used in combination with defining your own equality
		 * (and inequality) operator so not all of these will typically be needed.
		 *
		 * @param define_equality_and_inequality_operators define '__eq__' and '__ne__'.
		 * @param define_ordering_operators define '__lt__', '__le__', '__gt__' and '__ge__'.
		 */
		explicit
		NoHashDefVisitor(
				bool define_equality_and_inequality_operators = true,
				bool define_ordering_operators = true) :
			d_define_equality_and_inequality_operators(define_equality_and_inequality_operators),
			d_define_ordering_operators(define_ordering_operators)
		{  }

	private:

		friend class boost::python::def_visitor_access;

		template <class PythonClassType>
		void
		visit(
				PythonClassType &python_class) const;


		bool d_define_equality_and_inequality_operators;
		bool d_define_ordering_operators;
	};


	/**
	 * A boost::python::class_ "def" visitor that hashes and compares based on the object identity
	 * (address) of the wrapped C++ object instead of the python object (via python's 'id()').
	 *
	 * NOTE: If the HeldType of bp::class_ of the wrapped C++ class is boost::shared_ptr then
	 * this might not be needed (see special magic comment below).
	 *
	 * This visitor adds a '__hash__' method and associated comparison methods (such as '__eq__')
	 * to the wrapped class (see http://docs.python.org/2/reference/datamodel.html#object.__hash__).
	 *
	 * An example usage is:
	 *
	 *   boost::python::class_<
	 *   		GPlatesModel::FeatureHandle,
	 *   		GPlatesModel::FeatureHandle::non_null_ptr_type,
	 *   		boost::noncopyable>(
	 *   				"Feature",
	 *   				...)
	 *	 	...
	 * 	 	.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	 *	 	...
	 *   ;
	 *
	 *
	 * This avoids problems when using an object as a key in a python dict. This is especially
	 * important when wrapping a C++ smart pointer because then it is common for two python objects
	 * (with different ids) to reference the same C++ object - the following example shows how this
	 * can happen:
	 *
	 *   f = pygplates.Feature()
	 *   f_ref = f
	 *   f_cpp_ref = pygplates.FeatureCollection([f]).get(f.get_feature_id())
	 *
	 * ...where we know that both 'f' and 'f_cpp_ref' refer to the same 'Feature' (C++) object
	 * but they are different Python objects (and hence have different addresses or ids)
	 * because boost-python creates a new Python object when returning a C++ object to Python
	 * (note however that it does return the same *Python* object when using boost::shared_ptr -
	 * due to special magic it places in the shared pointer deleter to track the Python object
	 * it came from - although this would not work in cases where the C++ object, wrapped in a
	 * Python object, is created from C++ and not Python, eg, loading a feature collection from
	 * a file, in which case conversion of the same C++ feature to Python will always create a new
	 * Python object) . However GPlates uses boost intrusive_ptr extensively and boost python creates
	 * a new python object when returning this. And the default hash and equality behaviour is based
	 * on the python object address.
	 * So the following conditions hold...
	 *
	 *   assert(id(f) == id(f_ref))
	 *   assert(id(f) != id(f_cpp_ref))
	 *   assert(f != f_cpp_ref)
	 *   assert(hash(f) != hash(f_cpp_ref))
	 *   d = {f: 'f'}
	 *   assert(d[f] == 'f')
	 *   assertRaises(KeyError, d[f_cpp_ref])  # f_cpp_ref != f so cannot find key in dict
	 *
	 * ...but if we use 'ObjectIdentityHashDefVisitor' then the following conditions hold...
	 *
	 *   assert(id(f) == id(f_ref))
	 *   assert(id(f) != id(f_cpp_ref))
	 *   assert(f == f_cpp_ref)
	 *   assert(hash(f) == hash(f_cpp_ref))
	 *   d = {f: 'f'}
	 *   assert(d[f] == 'f')
	 *   assert(d[f_cpp_ref] == 'f')  # f_cpp_ref == f so can find key in dict
	 *
	 * ...the python object ids (addresses) are still different in both cases but in the latter case
	 * the two python objects compare equal and have the same hash value (based on the shared
	 * C++ object's address).
	 */
	class ObjectIdentityHashDefVisitor :
			public boost::python::def_visitor<ObjectIdentityHashDefVisitor>
	{
	public:

		/**
		 * @param define_equality_and_inequality_operators define '__eq__' and '__ne__'.
		 * @param define_ordering_operators define '__lt__', '__le__', '__gt__' and '__ge__'.
		 */
		explicit
		ObjectIdentityHashDefVisitor(
				bool define_equality_and_inequality_operators = true,
				bool define_ordering_operators = true) :
			d_define_equality_and_inequality_operators(define_equality_and_inequality_operators),
			d_define_ordering_operators(define_ordering_operators)
		{  }

	private:

		friend class boost::python::def_visitor_access;

		template <class PythonClassType>
		void
		visit(
				PythonClassType &python_class) const;

		template <class ClassType>
		static
		std::size_t
		hash(
				const ClassType &instance)
		{
			return std::size_t(
					// Make sure we get the address of the outermost object in cases where
					// 'ClassType' is a polymorphic base class of a multiply-inherited class...
					get_object_address(
							&instance,
							typename boost::is_polymorphic<ClassType>::type()));
		}

		//! Overload for polymorphic types - returns address of the entire (dynamic) object.
		template <class ClassType>
		static
		const void *
		get_object_address(
				const ClassType *object_address,
				boost::mpl::true_/*is_polymorphic*/)
		{
			return dynamic_cast<const void *>(object_address);
		}

		//! Overload for non-polymorphic types - just returns the address passed in.
		template <class ClassType>
		static
		const void *
		get_object_address(
				const ClassType *object_address,
				boost::mpl::false_/*is_polymorphic*/)
		{
			return static_cast<const void *>(object_address);
		}


		bool d_define_equality_and_inequality_operators;
		bool d_define_ordering_operators;
	};
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesApi
{
	namespace Implementation
	{
		template <class ClassType>
		bool
		object_identity_eq(
				const ClassType &instance,
				boost::python::object other)
		{
			namespace bp = boost::python;

			// Note that this also extracts from a Python object wrapping a C++ held-pointer to an
			// object derived from 'ClassType', and from a Python object wrapping a C++ held-pointer
			// to a polymorphic base of 'ClassType'. This is due to the Bases parameter of bp::class_.
			// This conversion to a 'ClassType' pointer/reference also nicely avoids the issue that
			// the address of a base class of a multiply-inherited class can be offset from the
			// derived class.
			bp::extract<const ClassType &> extract_other_instance(other);
			if (extract_other_instance.check())
			{
				return &instance == &extract_other_instance();
			}

			// If 'other' is not same type then cannot be same object (at same address).
			// Aside from the above-mentioned base and derived classes.
			return false;
		}

		template <class ClassType>
		bool
		object_identity_ne(
				const ClassType &instance,
				boost::python::object other)
		{
			// Invert the 'eq' result.
			return !object_identity_eq(instance, other);
		}

		template <class ClassType>
		boost::python::object
		object_identity_lt(
				const ClassType &instance,
				boost::python::object other)
		{
			namespace bp = boost::python;

			bp::extract<const ClassType &> extract_other_instance(other);
			if (extract_other_instance.check())
			{
				return bp::object(&instance < &extract_other_instance());
			}

			// Return NotImplemented so python can continue looking for a match
			// (eg, in case 'other' is a class that implements relational operators with ClassType).
			//
			// NOTE: This will most likely fall back to python's default handling which uses 'id()'
			// and hence will compare based on *python* object address rather than *C++* object address.
			return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
		}

		template <class ClassType>
		boost::python::object
		object_identity_le(
				const ClassType &instance,
				boost::python::object other)
		{
			namespace bp = boost::python;

			bp::extract<const ClassType &> extract_other_instance(other);
			if (extract_other_instance.check())
			{
				return bp::object(&instance <= &extract_other_instance());
			}

			// Return NotImplemented so python can continue looking for a match
			// (eg, in case 'other' is a class that implements relational operators with ClassType).
			//
			// NOTE: This will most likely fall back to python's default handling which uses 'id()'
			// and hence will compare based on *python* object address rather than *C++* object address.
			return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
		}

		template <class ClassType>
		boost::python::object
		object_identity_gt(
				const ClassType &instance,
				boost::python::object other)
		{
			namespace bp = boost::python;

			bp::object le_result = object_identity_le(instance, other);
			if (le_result.ptr() == Py_NotImplemented)
			{
				// Return NotImplemented.
				return le_result;
			}

			// Invert the result.
			return bp::object(!bp::extract<bool>(le_result));
		}

		template <class ClassType>
		boost::python::object
		object_identity_ge(
				const ClassType &instance,
				boost::python::object other)
		{
			namespace bp = boost::python;

			bp::object lt_result = object_identity_lt(instance, other);
			if (lt_result.ptr() == Py_NotImplemented)
			{
				// Return NotImplemented.
				return lt_result;
			}

			// Invert the result.
			return bp::object(!bp::extract<bool>(lt_result));
		}
	}


	template <class PythonClassType>
	void
	NoHashDefVisitor::visit(
			PythonClassType &python_class) const
	{
		// The type of the class being wrapped.
		typedef typename PythonClassType::wrapped_type class_type;

		// Make unhashable.
		python_class.setattr("__hash__", boost::python::object()/*None*/);

		if (d_define_equality_and_inequality_operators)
		{
			python_class
					.def("__eq__", &Implementation::object_identity_eq<class_type>)
					.def("__ne__", &Implementation::object_identity_ne<class_type>)
			;
		}

		if (d_define_ordering_operators)
		{
			python_class
				.def("__lt__", &Implementation::object_identity_lt<class_type>)
				.def("__le__", &Implementation::object_identity_le<class_type>)
				.def("__gt__", &Implementation::object_identity_gt<class_type>)
				.def("__ge__", &Implementation::object_identity_ge<class_type>)
			;
		}
	}


	template <class PythonClassType>
	void
	ObjectIdentityHashDefVisitor::visit(
			PythonClassType &python_class) const
	{
		// The type of the class being wrapped.
		typedef typename PythonClassType::wrapped_type class_type;

		python_class.def("__hash__", &hash<class_type>);

		if (d_define_equality_and_inequality_operators)
		{
			python_class
					.def("__eq__", &Implementation::object_identity_eq<class_type>)
					.def("__ne__", &Implementation::object_identity_ne<class_type>)
			;
		}

		if (d_define_ordering_operators)
		{
			python_class
				.def("__lt__", &Implementation::object_identity_lt<class_type>)
				.def("__le__", &Implementation::object_identity_le<class_type>)
				.def("__gt__", &Implementation::object_identity_gt<class_type>)
				.def("__ge__", &Implementation::object_identity_ge<class_type>)
			;
		}
	}
}

#endif   //GPLATES_NO_PYTHON)

#endif // GPLATES_API_PYTHONHASHDEFVISITOR_H
