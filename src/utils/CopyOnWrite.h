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

#ifndef GPLATES_UTILS_COPYONWRITEPOINTER_H
#define GPLATES_UTILS_COPYONWRITEPOINTER_H

#include <utility>
#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_const.hpp>

#include "non_null_intrusive_ptr.h"
#include "ReferenceCount.h"


namespace GPlatesUtils
{
	/**
	 * Declaration of primary template for the default copy policy for copy-on-write.
	 *
	 * This primary template is intentionally undefined (partial or full specialisations are defined instead).
	 */
	template <class ValueType>
	class DefaultCopyOnWritePolicy;

	/**
	 * Primary template for copy-on-write value semantics.
	 *
	 * This primary template is intentionally undefined.
	 * Partial (or full) specialisations of this template define copy-on-write for specific types.
	 *
	 * Currently we support GPlatesUtils::non_null_intrusive_ptr, boost::shared_ptr and boost::intrusive_ptr
	 * since those types of smart pointers provide the reference counting that makes copy-on-write
	 * easier to implement.
	 *
	 * Template class 'CopyPolicy' should have a 'copy()' method:
	 *
	 *  static ValueType copy(const ValueType &value);
	 *
	 * A default copy policy is provided by 'DefaultCopyOnWritePolicy'.
	 *
	 *
	 * Usage:
	 *
	 *  A::non_null_ptr_type z(new A(1));
	 *
	 *  GPlatesUtils::CopyOnWrite<A::non_null_ptr_type> x(z);
	 *
	 *  // 'x' has *value* of '1'.
	 *  assert(x.get_const()->get_value() == 1);
	 *
	 *  GPlatesUtils::CopyOnWrite<A::non_null_ptr_type> y = x;
	 *
	 *  // 'y' and 'x' have same *value* (of '1').
	 *  assert(x.get_const()->get_value() == y.get_const()->get_value());
	 *
	 *  y.get_non_const()->set_value(2);
	 *
	 *  // 'y' now has value '2'.
	 *  assert(y.get_const()->get_value() == 2);
	 *
	 *  // But 'x' still has value '1', because 'y' was copied-on-write.
	 *  assert(x.get_const()->get_value() == 1);
	 */
	template <
			class ValueType,
			template <class> class CopyPolicy = DefaultCopyOnWritePolicy>
	class CopyOnWrite;

	//
	// Synopsis:
	//
	// 	template <
	// 			class ValueType,
	// 			template <class> class CopyPolicy = DefaultCopyOnWritePolicy>
	// 	class CopyOnWrite
	// 	{
	// 	public:
	// 
	// 		CopyOnWrite(
	// 				const ValueType &value) :
	// 			d_value(CopyPolicy<ValueType>::copy(value))
	// 		{  }
	// 
	// 		/**
	// 		 * Return 'const' reference to value.
	// 		 */
	// 		const ValueType &
	// 		get() const
	// 		{
	// 			return get_const();
	// 		}
	// 
	// 		/**
	// 		 * Return 'non-const' reference to value.
	// 		 */
	// 		ValueType &
	// 		get()
	// 		{
	// 			return get_non_const();
	// 		}
	// 
	// 		const ValueType &
	// 		get_const() const
	// 		{
	// 			return d_value;
	// 		}
	// 
	// 		ValueType &
	// 		get_non_const()
	// 		{
	//          d_value = CopyPolicy<ValueType>::copy(d_value);
	// 			return d_value;
	// 		}
	// 
	// 	private:
	// 		ValueType d_value;
	// 	};


	//
	// Partial and full specialisations.
	//


	/**
	 * Default copy policy for GPlatesUtils::non_null_intrusive_ptr assumes
	 * a 'clone()' method that returns a non_null_intrusive_ptr.
	 */
	template <class T, class H>
	class DefaultCopyOnWritePolicy<non_null_intrusive_ptr<T,H> >
	{
	public:
		static
		non_null_intrusive_ptr<T,H>
		copy(
				const non_null_intrusive_ptr<T,H> &value)
		{
			return value->clone();
		}
	};


	/**
	 * Partial specialisation of @a CopyOnWrite for GPlatesUtils::non_null_intrusive_ptr.
	 */
	template <class T, class H, template <class> class CopyPolicy>
	class CopyOnWrite<non_null_intrusive_ptr<T,H>, CopyPolicy>
	{
	public:

		typedef non_null_intrusive_ptr<T, H> non_null_ptr_type;
		typedef non_null_intrusive_ptr<const T, H> non_null_ptr_to_const_type;


		/**
		 * Constructor creates a copy of the referenced value.
		 */
		CopyOnWrite(
				const non_null_ptr_type &value) :
			// Clone since we cannot be sure there are no aliased references, to the value passed
			// to us, that could modify our value state without us knowing...
			d_value(CopyPolicy<non_null_ptr_type>::copy(value)),
			d_shareable(true) // There are no 'non-const' client references to our value.
		{  }

		/**
		 * Copy constructor copies/clones if @a other is not shareable (if client has non-const
		 * reference to the value of @a other).
		 */
		CopyOnWrite(
				const CopyOnWrite &other) :
			// Clone if not shareable to prevent non-const value references from modifying our value state...
			d_value(other.d_shareable ? other.d_value : CopyPolicy<non_null_ptr_type>::copy(other.d_value)),
			d_shareable(true) // There are no 'non-const' client references to our value.
		{  }

		CopyOnWrite &
		operator=(
				CopyOnWrite other)
		{
			// Copy-and-swap idiom.
			swap(d_value, other.d_value);
			std::swap(d_shareable, other.d_shareable);

			return *this;
		}

		/**
		 * Return 'const' reference to value.
		 */
		const non_null_ptr_to_const_type
		get() const
		{
			return get_const();
		}

		/**
		 * Return 'non-const' reference to value.
		 */
		const non_null_ptr_type
		get()
		{
			return get_non_const();
		}

		const non_null_ptr_to_const_type
		get_const() const
		{
			return d_value;
		}

		const non_null_ptr_type
		get_non_const()
		{
			// It's possible that 'T' is in fact const in which case there's no need for
			// copy-on-write in which case this class doesn't really do much except hand
			// back pointers to 'const' objects ('T') which cannot be written to.
			return get_non_const(typename boost::is_const<T>::type());
		}

	private:

		const non_null_ptr_type
		get_non_const(
				boost::mpl::true_/*'T' is const*/)
		{
			return get_const();
		}

		const non_null_ptr_type
		get_non_const(
				boost::mpl::false_/*'T' is not const*/)
		{
			// The returned non-const pointer can potentially write to our value, so clone the
			// value if it's shareable (ie, if no clients can write to it yet) and if the value
			// is currently shared by others. Otherwise either it's not shareable (ie, clients can
			// already write to it) or it's not currently shared by others, in which case the
			// current value can be returned (without cloning).
			if (d_shareable &&
				d_value->get_reference_count() > 1)
			{
				d_value = CopyPolicy<non_null_ptr_type>::copy(d_value);
			}

			// Clients can now modify the referenced value (through the returned non-const pointer)
			// so it can no longer be shared.
			d_shareable = false;

			return d_value;
		}

		non_null_ptr_type d_value;
		bool d_shareable;
	};


	/**
	 * Default copy policy for boost::intrusive_ptr assumes a 'clone()' method that
	 * returns either a boost::intrusive_ptr or a non_null_intrusive_ptr.
	 */
	template <class T>
	class DefaultCopyOnWritePolicy<boost::intrusive_ptr<T> >
	{
	public:
		static
		boost::intrusive_ptr<T>
		copy(
				const boost::intrusive_ptr<T> &value)
		{
			// Using 'get()' allows a return value of non_null_intrusive_ptr to work with intrusive_ptr.
			return value->clone().get();
		}
	};


	/**
	 * Partial specialisation of @a CopyOnWrite for boost::intrusive_ptr.
	 *
	 * For implementation details refer to the partial specialisation of 'non_null_intrusive_ptr'.
	 */
	template <class T, template <class> class CopyPolicy>
	class CopyOnWrite<boost::intrusive_ptr<T>, CopyPolicy>
	{
	public:

		typedef boost::intrusive_ptr<T> intrusive_ptr_type;
		typedef boost::intrusive_ptr<const T> intrusive_ptr_to_const_type;

		CopyOnWrite(
				const intrusive_ptr_type &value) :
			d_value(value ? CopyPolicy<intrusive_ptr_type>::copy(value) : intrusive_ptr_type()),
			d_shareable(true)
		{  }

		CopyOnWrite(
				const CopyOnWrite &other) :
			d_shareable(true)
		{
			if (other.d_shareable)
			{
				d_value = other.d_value;
			}
			else if (other.d_value)
			{
				d_value = CopyPolicy<intrusive_ptr_type>::copy(other.d_value);
			}
		}

		CopyOnWrite &
		operator=(
				CopyOnWrite other)
		{
			swap(d_value, other.d_value);
			std::swap(d_shareable, other.d_shareable);
			return *this;
		}

		const intrusive_ptr_to_const_type
		get() const
		{
			return get_const();
		}

		const intrusive_ptr_type
		get()
		{
			return get_non_const();
		}

		const intrusive_ptr_to_const_type
		get_const() const
		{
			return d_value;
		}

		const intrusive_ptr_type
		get_non_const()
		{
			return get_non_const(typename boost::is_const<T>::type());
		}

	private:

		const intrusive_ptr_type
		get_non_const(
				boost::mpl::true_)
		{
			return get_const();
		}

		const intrusive_ptr_type
		get_non_const(
				boost::mpl::false_)
		{
			if (d_shareable &&
				d_value &&
				d_value->get_reference_count() > 1)
			{
				d_value = CopyPolicy<intrusive_ptr_type>::copy(d_value);
			}
			d_shareable = false;
			return d_value;
		}

		intrusive_ptr_type d_value;
		bool d_shareable;
	};


	/**
	 * Default copy policy for boost::shared_ptr assumes a 'clone()' method that
	 * returns a boost::shared_ptr.
	 */
	template <class T>
	class DefaultCopyOnWritePolicy<boost::shared_ptr<T> >
	{
	public:
		static
		boost::shared_ptr<T>
		copy(
				const boost::shared_ptr<T> &value)
		{
			return value->clone();
		}
	};


	/**
	 * Partial specialisation of @a CopyOnWrite for boost::shared_ptr.
	 *
	 * For implementation details refer to the partial specialisation of 'non_null_intrusive_ptr'.
	 */
	template <class T, template <class> class CopyPolicy>
	class CopyOnWrite<boost::shared_ptr<T>, CopyPolicy>
	{
	public:

		typedef boost::shared_ptr<T> shared_ptr_type;
		typedef boost::shared_ptr<const T> shared_ptr_to_const_type;

		CopyOnWrite(
				const shared_ptr_type &value) :
			d_value(value ? CopyPolicy<shared_ptr_type>::copy(value) : shared_ptr_type()),
			d_shareable(true)
		{  }

		CopyOnWrite(
				const CopyOnWrite &other) :
			d_shareable(true)
		{
			if (other.d_shareable)
			{
				d_value = other.d_value;
			}
			else if (other.d_value)
			{
				d_value = CopyPolicy<shared_ptr_type>::copy(other.d_value);
			}
		}

		CopyOnWrite &
		operator=(
				CopyOnWrite other)
		{
			swap(d_value, other.d_value);
			std::swap(d_shareable, other.d_shareable);
			return *this;
		}

		const shared_ptr_to_const_type
		get() const
		{
			return get_const();
		}

		const shared_ptr_type
		get()
		{
			return get_non_const();
		}

		const shared_ptr_to_const_type
		get_const() const
		{
			return d_value;
		}

		const shared_ptr_type
		get_non_const()
		{
			return get_non_const(typename boost::is_const<T>::type());
		}

	private:

		const shared_ptr_type
		get_non_const(
				boost::mpl::true_)
		{
			return get_const();
		}

		const shared_ptr_type
		get_non_const(
				boost::mpl::false_)
		{
			if (d_shareable &&
				d_value &&
				!d_value->unique())
			{
				d_value = CopyPolicy<shared_ptr_type>::copy(d_value);
			}
			d_shareable = false;
			return d_value;
		}

		shared_ptr_type d_value;
		bool d_shareable;
	};
}

#endif // GPLATES_UTILS_COPYONWRITEPOINTER_H
