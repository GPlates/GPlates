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

#ifndef GPLATES_MODEL_TOPLEVELPROPERTYREVISION_H
#define GPLATES_MODEL_TOPLEVELPROPERTYREVISION_H

#include <map>
#include <boost/optional.hpp>

#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class FeatureHandle;
	template <class TopLevelPropertyType> class TopLevelPropertyRevisionedReference;

	/**
	 * Base class inherited by derived revision classes (in derived top-level properties) where
	 * mutable/revisionable top-level property state is stored so it can be revisioned.
	 */
	class TopLevelPropertyRevision :
			public GPlatesUtils::ReferenceCount<TopLevelPropertyRevision>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopLevelPropertyRevision> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopLevelPropertyRevision> non_null_ptr_to_const_type;


		/**
		 * The type of the container of XML attributes.
		 */
		typedef std::map<XmlAttributeName, XmlAttributeValue> xml_attributes_type;


		virtual
		~TopLevelPropertyRevision()
		{  }


		/**
		 * A shallow clone that deep copies everything except nested property value revision references.
		 *
		 * @a context is the optional (parent) feature handle within which this revision is nested.
		 * A top-level property value (revision) that is not attached to a parent has no context.
		 *
		 * Since property values nested within this top-level property are already revisioned
		 * we don't need to deep copy them. In other words two parent top-level property revisions
		 * can share the same nested property value revision.
		 */
		virtual
		non_null_ptr_type
		clone_revision(
				boost::optional<FeatureHandle &> context = boost::none) const = 0;


		/**
		 * Determine if two Revision instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'other'.
		 */
		virtual
		bool
		equality(
				const TopLevelPropertyRevision &other) const
		{
			return xml_attributes == other.xml_attributes;
		}


		/**
		 * Returns the (parent) context of this revision, if any.
		 *
		 * Note: There's no set method since it should not be possible to alter the context
		 * after a revision has been created.
		 */
		boost::optional<FeatureHandle &>
		get_context() const
		{
			return d_context;
		}


		/**
		 * XML attributes.
		 */
		xml_attributes_type xml_attributes;

	protected:

		/**
		 * Constructor specified optional (parent) context in which this top-level property (revision) is nested.
		 */
		explicit
		TopLevelPropertyRevision(
				const xml_attributes_type &xml_attributes_,
				boost::optional<FeatureHandle &> context = boost::none) :
			xml_attributes(xml_attributes_),
			d_context(context),
			d_revision_reference_ref_count(0)
		{  }

		/**
		 * Construct a TopLevelPropertyRevision instance using another TopLevelPropertyRevision.
		 */
		TopLevelPropertyRevision(
				const TopLevelPropertyRevision &other,
				boost::optional<FeatureHandle &> context) :
			xml_attributes(other.xml_attributes),
			d_context(context),
			d_revision_reference_ref_count(0)
		{  }

		/**
		 * Copy constructor - calls default constructor of ReferenceCount (non-copyable) base class.
		 *
		 * Note: This also copies the revision context.
		 */
		TopLevelPropertyRevision(
				const TopLevelPropertyRevision &other) :
			xml_attributes(other.xml_attributes),
			d_context(other.d_context),
			d_revision_reference_ref_count(0)
		{  }

	private:

		/**
		 * The bubble up callback to the parent feature handle, if any, that is called just prior
		 * to making a modification to 'this' top-level property.
		 */
		boost::optional<FeatureHandle &> d_context;


		/**
		 * The reference-count of this instance used by @a TopLevelPropertyRevisionedReference.
		 *
		 * This is used to detach 'this' top-level property revision from its revision context when
		 * the last @a TopLevelPropertyRevisionedReference referencing 'this' is destroyed.
		 */
		mutable int d_revision_reference_ref_count;


		template <class TopLevelPropertyType>
		friend class TopLevelPropertyRevisionedReference;
	};
}

#endif // GPLATES_MODEL_TOPLEVELPROPERTYREVISION_H
