/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_FEATUREWEAKREFSEQUENCE_H
#define GPLATES_GUI_FEATUREWEAKREFSEQUENCE_H

#include <vector>
#include <QObject>

#include "model/FeatureHandle.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	/**
	 * This class is used for a sequence of feature weak-refs in the GUI.
	 *
	 * For example: it might be used to contain the collection of weak-refs to the features
	 * "hit" by a mouse-click on the globe; it might be used to contain the collection of
	 * weak-refs to the features which are currently selected in the GUI.
	 *
	 * It is referenced by intrusive-pointer, so it can be shared between objects of differing
	 * lifetimes.
	 *
	 * Sometime in the future, it might become smart enough to purge weak-refs automatically
	 * when their features are removed and the Undo history is flushed.
	 *
	 * Note that there is no guarantee that the weak-refs contained in a FeatureWeakRefSequence
	 * instance are valid to be dereferenced.
	 */
	class FeatureWeakRefSequence:
			public QObject,
			public GPlatesUtils::ReferenceCount<FeatureWeakRefSequence>
	{
		Q_OBJECT

	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<FeatureWeakRefSequence,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureWeakRefSequence,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * The type used to contain the sequence of feature weak-refs.
		 */
		typedef std::vector<GPlatesModel::FeatureHandle::weak_ref> sequence_type;

		/**
		 * The type used for the size of the sequence of feature weak-refs.
		 */
		typedef sequence_type::size_type size_type;

		/**
		 * The type used to const-iterate over the sequence of feature weak-refs.
		 */
		typedef sequence_type::const_iterator const_iterator;

		~FeatureWeakRefSequence()
		{  }

		/**
		 * Create a new FeatureWeakRefSequence instance.
		 */
		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(new FeatureWeakRefSequence,
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		size_type
		size() const
		{
			return d_sequence.size();
		}

		const_iterator
		begin() const
		{
			return d_sequence.begin();
		}

		const_iterator
		end() const
		{
			return d_sequence.end();
		}
		
		GPlatesModel::FeatureHandle::weak_ref
		at(size_type index) const
		{
			return d_sequence.at(index);
		}
		
		void
		clear()
		{
			d_sequence.clear();
		}

		void
		push_back(
				const GPlatesModel::FeatureHandle::weak_ref &new_elem)
		{
			d_sequence.push_back(new_elem);
		}
	
	private:
		/**
		 * The sequence of feature weak-refs.
		 */
		sequence_type d_sequence;

		/**
		 * Construct a FeatureWeakRefSequence instance.
		 */
		FeatureWeakRefSequence()
		{  }

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		FeatureWeakRefSequence(
				const FeatureWeakRefSequence &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		FeatureWeakRefSequence &
		operator=(
				const FeatureWeakRefSequence &);
	};
}

#endif  // GPLATES_GUI_FEATUREWEAKREFSEQUENCE_H
