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

#ifndef GPLATES_MODEL_CONSTFEATUREVISITOR_H
#define GPLATES_MODEL_CONSTFEATUREVISITOR_H


namespace GPlatesModel {

	// Forward declarations for the member functions.
	class FeatureHandle;

	class ConstFeatureVisitor {

	public:

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions for convenience.
		virtual
		~ConstFeatureVisitor() = 0;

		virtual
		void
		visit_feature_handle(
				const FeatureHandle &feature_handle) {  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ConstFeatureVisitor &
		operator=(
				const ConstFeatureVisitor &);

	};

}

#endif  // GPLATES_MODEL_CONSTFEATUREVISITOR_H
