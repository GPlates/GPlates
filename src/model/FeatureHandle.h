/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREHANDLE_H
#define GPLATES_MODEL_FEATUREHANDLE_H

#include <boost/intrusive_ptr.hpp>
#include "FeatureRevision.h"
#include "FeatureId.h"
#include "FeatureType.h"
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"

namespace GPlatesModel {

	/**
	 * A feature handle is the part of a GPML feature which does not change with revisions.
	 *
	 * This class represents the part of a GPML feature which does not change with revisions.
	 * That is, it contains the components of a feature which never change (the feature type
	 * and the feature ID), as well as acting as an everlasting handle to the FeatureRevision
	 * which contains the rest of the feature.
	 *
	 * A FeatureHandle instance is referenced by a FeatureCollection.
	 */
	class FeatureHandle {

	public:
		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_ and
		 * feature ID @a feature_id_.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_):
			d_current_revision(NULL),
			d_feature_type(feature_type_),
			d_feature_id(feature_id_)
		{  }

		/**
		 * Return the feature type of this feature.
		 *
		 * Note that no "setter" is provided:  The feature type of a feature should never
		 * be changed.
		 */
		const FeatureType &
		feature_type() const {
			return d_feature_type;
		}

		/**
		 * Return the feature ID of this feature.
		 *
		 * Note that no "setter" is provided:  The feature ID of a feature should never be
		 * changed.
		 */
		const FeatureId &
		feature_id() const {
			return d_feature_id;
		}

		/**
		 * Access the current revision of this feature.
		 *
		 * This is the overloading of this function for const FeatureHandle instances; it
		 * returns a pointer to a const FeatureRevision instance.
		 */
		const boost::intrusive_ptr<const FeatureRevision>
		current_revision() const {
			return d_current_revision;
		}

		/**
		 * Access the current revision of this feature.
		 *
		 * This is the overloading of this function for non-const FeatureHandle instances;
		 * it returns a C++ reference to a pointer to a non-const FeatureRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureRevision is private,
		 * the FeatureRevision referenced by the return-value of this function cannot be
		 * assigned-to, which means that this function does not provide a means to directly
		 * switch the FeatureRevision within this FeatureHandle instance.  (This
		 * restriction is intentional.)
		 *
		 * To switch the FeatureRevision within this FeatureHandle instance, use the
		 * function @a set_current_revision below.
		 */
		const boost::intrusive_ptr<FeatureRevision>
		current_revision() {
			return d_current_revision;
		}

		/**
		 * Set the current revision of this feature to @a rev.
		 */
		void
		set_current_revision(
				boost::intrusive_ptr<FeatureRevision> rev) {
			d_current_revision = rev;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_feature_handle(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				FeatureVisitor &visitor) {
			visitor.visit_feature_handle(*this);
		}

	private:

		boost::intrusive_ptr<FeatureRevision> d_current_revision;
		FeatureType d_feature_type;
		FeatureId d_feature_id;

	};

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
