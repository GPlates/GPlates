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

#ifndef GPLATES_API_PYTHONVARIANTINPUTITERATOR_H
#define GPLATES_API_PYTHONVARIANTINPUTITERATOR_H

#include <iterator>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next.hpp>
#include <boost/variant.hpp>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * A STL input iterator that behaves like boost::python::stl_input_iterator but
	 * accepts sequence elements of *more than one* type (defined by the types in a boost::variant).
	 *
	 * This enables, for example, a python 'list' to contain mixed types such as feature collections
	 * and filenames (which can be loaded into feature collections).
	 *
	 * The template parameter 'VariantType' should be a boost::variant of the allowed sequence
	 * element types you've chosen (eg, boost::variant<int, QString>).
	 * Dereferencing this iterator will return an instance of 'VariantType' (boost::variant<...>).
	 * Each of the allowed element types should be convertible from python.
	 * Raises python TypeError if any element in the python sequence (iterated over) is not
	 * convertible to one of the types in the variant.
	 *
	 * For example:
	 *
	 *   boost::python::object sequence = ...;
	 *
	 *   VariantInputIterator< boost::variant<int, QString> >
	 *   		sequence_iter(sequence),
	 *   		sequence_end;
	 *
	 *   for ( ; sequence_iter != sequence_end; ++sequence_iter)
	 *   {
	 *      const boost::variant<int, QString> sequence_element = *sequence_iter;
	 *      if (const int *integer_element = boost::get<int>(&sequence_element))
	 *      {
	 *         ...
	 *      }
	 *      else // QString...
	 *      {
	 *         const QString &string_element = boost::get<QString>(sequence_element);
	 *         ...
	 *      }
	 *   }
	 */
	template <class VariantType>
	class VariantInputIterator :
			public boost::iterator_facade<
					VariantInputIterator<VariantType>,
					VariantType,
					std::input_iterator_tag,
					VariantType>
	{
	public:

		VariantInputIterator()
		{  }

		explicit
		VariantInputIterator(
				boost::python::object sequence) :
			d_iterator(sequence.attr("__iter__")())
		{
			increment();
		}

	private:

		boost::python::object d_iterator;
		boost::python::handle<> d_current_element;


		//! The boost::variant type.
		typedef VariantType variant_type;

		//! The MPL sequence of types in the variant.
		typedef typename variant_type::types variant_types;


		/**
		 * Returns the names of the types in the variant.
		 */
		template <typename VariantCurrentIterType, typename VariantEndIterType>
		struct GetVariantTypeNames;

		// Terminates visitation of variant types.
		template <typename VariantEndIterType>
		struct GetVariantTypeNames<VariantEndIterType, VariantEndIterType>
		{
			static
			void
			get(
					std::vector<const char *> &variant_type_names)
			{  }
		};

		// Primary template - defined after partial specialisation since latter is used by primate template.
		template <typename VariantCurrentIterType, typename VariantEndIterType>
		struct GetVariantTypeNames
		{
			static
			void
			get(
					std::vector<const char *> &variant_type_names)
			{
				// The current type being visited in the variant.
				typedef typename boost::mpl::deref<VariantCurrentIterType>::type variant_current_type;

				variant_type_names.push_back(typeid(variant_current_type).name());

				// Visit the next type in the variant.
				return GetVariantTypeNames<
						typename boost::mpl::next<VariantCurrentIterType>::type,
						VariantEndIterType>::get(variant_type_names);
			}
		};


		/**
		 * Attempts to extract (from boost) the currently visited type in the variant.
		 *
		 * If successful then returns, otherwise proceeds to the next type in the variant.
		 */
		template <typename VariantCurrentIterType, typename VariantEndIterType>
		struct ExtractVariant;

		// Terminates visitation of variant types.
		template <typename VariantEndIterType>
		struct ExtractVariant<VariantEndIterType, VariantEndIterType>
		{
			static
			variant_type
			get(
					PyObject *)
			{
				//
				// If we get here then we couldn't extract any variant type, so raise a python error.
				//

				// Get the names of all the types in the variant.
				std::vector<const char *> variant_type_names;
				GetVariantTypeNames<
						boost::mpl::begin<variant_types>::type,
						boost::mpl::end<variant_types>::type>::get(
								variant_type_names);
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						!variant_type_names.empty(),
						GPLATES_ASSERTION_SOURCE);

				std::ostringstream oss;
				oss << "Unable to convert sequence element to one of the following C++ types: ";
				for (unsigned int n = 0; n < variant_type_names.size() - 1; ++n)
				{
					oss << "'" << variant_type_names[n] << "', ";
				}
				oss << "'" << variant_type_names.back() << "'";

				PyErr_SetString(PyExc_TypeError, oss.str().c_str());
				boost::python::throw_error_already_set();

				// Should be able to get here.
				// This is to keep compiler happy since we're not returning a variant object
				// because it might not be default-constructible (if first element type isn't).
				throw GPlatesGlobal::AssertionFailureException(GPLATES_ASSERTION_SOURCE);
			}
		};

		// Primary template - defined after partial specialisation since latter is used by primate template.
		template <typename VariantCurrentIterType, typename VariantEndIterType>
		struct ExtractVariant
		{
			static
			variant_type
			get(
					PyObject *python_object)
			{
				// The current type being visited in the variant.
				typedef typename boost::mpl::deref<VariantCurrentIterType>::type variant_current_type;

				// Attempt to extract the current type in the variant.
				boost::python::extract<variant_current_type> extract_object(python_object);
				if (extract_object.check())
				{
					return variant_type(extract_object());
				}

				// Try to extract the next type in the variant.
				return ExtractVariant<
						typename boost::mpl::next<VariantCurrentIterType>::type,
						VariantEndIterType>::get(python_object);
			}
		};


		friend class boost::iterator_core_access;

		variant_type
		dereference() const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_current_element,
					GPLATES_ASSERTION_SOURCE);

			// Iterate over the types in the variant until we can extract the current sequence
			// element (python object) into one of the variant's C++ types.
			return ExtractVariant<
					boost::mpl::begin<variant_types>::type,
					boost::mpl::end<variant_types>::type>::get(
							d_current_element.get());
		}

		bool
		equal(
				const VariantInputIterator &other) const
		{
			// Same as in boost::python::stl_input_iterator...
			return !d_current_element == !other.d_current_element;
		}

		void
		increment()
		{
			// Same as in boost::python::stl_input_iterator...

			d_current_element = boost::python::handle<>(
					boost::python::allow_null(
							PyIter_Next(d_iterator.ptr())));

			if (PyErr_Occurred())
			{
				throw boost::python::error_already_set();
			}
		}

	};
}

#endif   //GPLATES_NO_PYTHON)

#endif // GPLATES_API_PYTHONVARIANTINPUTITERATOR_H
