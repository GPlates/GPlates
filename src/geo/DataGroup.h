/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_DATAGROUP_H_
#define _GPLATES_DATAGROUP_H_

#include <vector>
#include "GeologicalData.h"

namespace GPlatesGeo
{
	/** 
	 * Represents a group of GeologicalData.
	 * While the job of DataGroup is to represent a logical group
	 * of GeologicalData, it is itself a piece of GeologicalData and
	 * so can be composed of other objects of type DataGroup. 
	 * @role Composite in the Composite design pattern (p163).
	 */
	class DataGroup : public GeologicalData
	{
		public:
			/** 
			 * Convenience typedef for refering to the DataGroup's 
			 * children.  The elements of the group are called children
			 * because the Composite role provides a tree object 
			 * structure: 'Children' is taken from tree nomenclature.
			 */
			typedef std::vector<GeologicalData*> Children_t;
			
			/** 
			 * Create a DataGroup with no elements.
			 * The parameters are passed directly to the superclass.
			 */
			DataGroup(const DataType_t&,
					  const RotationGroupId_t&,
					  const TimeWindow&,
					  const Attributes_t&);
			
			/**
			 * Create a DataGroup containing the given elements.
			 * The DataGroup is initialised with a copy of each of 
			 * the elements from @a children.
			 */
			DataGroup(const DataType_t&, 
					  const RotationGroupId_t&,
					  const TimeWindow&,
					  const Attributes_t&, 
					  const Children_t&);
			
			/**
			 * Delete each of the elements of _children.
			 */
			~DataGroup();

			/**
			 * Calls Accept() on all of its (immutable) children.
			 * @role ConcreteElement::Accept(Visitor) in the Visitor
			 *   design pattern (p331).
			 */
			virtual void
			Accept(Visitor& visitor) const;

			/**
			 * As above but the children are mutable.
			 */
			virtual void
			Accept(Visitor& visitor);
			
			/** 
			 * Add an element to the container of children.
			 * Does nothing if @a child is NULL.
			 * @param child The element to be added to the group. 
			 * @role Composite::Add(Component) in the Composite design pattern 
			 *   (p163).
			 */
			virtual void
			Add(GeologicalData* child);

			/**
			 * Remove an element from the container of children.
			 * Does nothing if @a child is NULL.
			 * @param child The element to be removed from the group.
			 * @role Composite::Remove(Component) in the Composite design 
			 *   pattern (p163).
			 */
			virtual void
			Remove(GeologicalData* child);

			/**
			 * Enumerative access to the children.
			 */
			virtual Children_t::iterator
			ChildrenBegin() { return _children.begin(); }

			virtual Children_t::iterator
			ChildrenEnd() { return _children.end(); }

			/** 
			 * Restricted enumerative access to the children.
			 */
			virtual Children_t::const_iterator
			ChildrenBegin() const { return _children.begin(); }
			
			virtual Children_t::const_iterator
			ChildrenEnd() const { return _children.end(); }

		private:
			/** 
			 * The children of this node in the 'data tree'.
			 */
			Children_t _children;
	};
}

#endif  // _GPLATES_DATAGROUP_H_
