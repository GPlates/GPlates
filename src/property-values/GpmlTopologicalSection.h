/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-11 19:36:59 -0700 (Fri, 11 Jul 2008) $
 * 
 * Copyright (C) 2008, 2009, 2010 California Institute of Technology 
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H

#include "model/PropertyValue.h"


// This macro is used to define the virtual function 'deep_clone_as_topo_section' inside a class
// which derives from TopologicalSection.  The function definition is exactly identical in every
// TopologicalSection derivation, but the function must be defined in each derived class (rather
// than in the base) because it invokes the non-virtual member function 'deep_clone' of that
// specific derived class.
// (This function 'deep_clone' cannot be moved into the base class, because (i) its return type is
// the type of the derived class, and (ii) it must perform different actions in different classes.)
// To define the function, invoke the macro in the class definition.  The macro invocation will
// expand to a definition of the function.
#define DEFINE_FUNCTION_DEEP_CLONE_AS_TOPO_SECTION()  \
		virtual  \
		const GpmlTopologicalSection::non_null_ptr_type  \
		deep_clone_as_topo_section() const  \
		{  \
			return deep_clone();  \
		}


namespace GPlatesPropertyValues {

	/**
	 * This is an abstract class, because it derives from class PropertyValue, which contains
	 * the pure virtual member functions @a clone and @a accept_visitor, which this class does
	 * not override with non-pure-virtual definitions.
	 */
	class GpmlTopologicalSection:
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for boost::intrusive_ptr<GpmlTopologicalSection>.
		 */
		typedef boost::intrusive_ptr<GpmlTopologicalSection>
				maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const GpmlTopologicalSection>.
		 */
		typedef boost::intrusive_ptr<const GpmlTopologicalSection>
				maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalSection,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalSection,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalSection,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalSection,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * Construct a GpmlTopologicalSection instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		explicit
		GpmlTopologicalSection():
			PropertyValue()
		{  }

		/**
		 * Construct a GpmlTopologicalSection instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		GpmlTopologicalSection(
				const GpmlTopologicalSection &other) :
			PropertyValue()
		{  }

		virtual
		~GpmlTopologicalSection() {  }

		virtual
		const GpmlTopologicalSection::non_null_ptr_type
		deep_clone_as_topo_section() const = 0;

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const { }

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) { }


	private:
		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlTopologicalSection &
		operator=(const GpmlTopologicalSection &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H
