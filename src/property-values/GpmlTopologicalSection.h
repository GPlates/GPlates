/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-11 19:36:59 -0700 (Fri, 11 Jul 2008) $
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
