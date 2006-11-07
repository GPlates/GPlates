/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_FEATUREREVISION_H
#define GPLATES_MODEL_FEATUREREVISION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include "RevisionId.h"
#include "PropertyContainer.h"
#include "ConstFeatureVisitor.h"


namespace GPlatesModel {

	class FeatureRevision {

	public:
		typedef long ref_count_type;

		~FeatureRevision()
		{  }

		static
		boost::intrusive_ptr<FeatureRevision>
		create() {
			boost::intrusive_ptr<FeatureRevision> ptr(new FeatureRevision());
			return ptr;
		}

		static
		boost::intrusive_ptr<FeatureRevision>
		create(
				const RevisionId &revision_id) {
			boost::intrusive_ptr<FeatureRevision> ptr(new FeatureRevision(revision_id));
			return ptr;
		}

		boost::intrusive_ptr<FeatureRevision>
		clone() const {
			boost::intrusive_ptr<FeatureRevision> dup(new FeatureRevision(*this));
			return dup;
		}

		const RevisionId &
		revision_id() const {
			return d_revision_id;
		}

		// No non-const 'revision_id':  The revision-ID should never be changed.

		const std::vector<boost::intrusive_ptr<PropertyContainer> > &
		properties() const {
			return d_properties;
		}

		std::vector<boost::intrusive_ptr<PropertyContainer> > &
		properties() {
			return d_properties;
		}

		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_feature_revision(*this);
		}

		void
		increment_ref_count() const {
			++d_ref_count;
		}

		ref_count_type
		decrement_ref_count() const {
			return --d_ref_count;
		}

	private:

		mutable ref_count_type d_ref_count;
		RevisionId d_revision_id;
		std::vector<boost::intrusive_ptr<PropertyContainer> > d_properties;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		FeatureRevision() :
			d_ref_count(0),
			d_revision_id()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		FeatureRevision(
				const RevisionId &revision_id_) :
			d_ref_count(0),
			d_revision_id(revision_id_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		FeatureRevision(
				const FeatureRevision &other) :
			d_ref_count(other.d_ref_count),
			d_revision_id(other.d_revision_id),
			d_properties(other.d_properties)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureRevision &
		operator=(
				const FeatureRevision &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureRevision *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureRevision *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATUREREVISION_H
