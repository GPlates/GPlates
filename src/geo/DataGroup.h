/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
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
			DataGroup(const DataType_t&, const RotationGroupId_t&,
				const Attributes_t&);
			
			/**
			 * Create a DataGroup containing the given elements.
			 * The DataGroup is initialised with a copy of each of 
			 * the elements from \a children.
			 */
			DataGroup(const DataType_t&, const RotationGroupId_t&,
				const Attributes_t&, const Children_t&);
			
			/**
			 * @role ConcreteElement::Accept(Visitor) in the Visitor
			 *   design pattern (p331).
			 */
			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }
			
			/** 
			 * Add an element to the container of children.
			 * Does nothing if @a child is NULL.
			 * @param child The element to be added to the group. 
			 * @pre If @a child is non-NULL, then it must point to a valid
			 *   piece of memory.
			 * @role Composite::Add(Component) in the Composite design pattern 
			 *   (p163).
			 */
			virtual void
			Add(GeologicalData* child) throw();

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
			ChildrenIterator() { return _children.begin(); }

			/** 
			 * Restricted enumerative access to the children.
			 */
			virtual Children_t::const_iterator
			ChildrenIterator() const { return _children.begin(); }
			
		private:
			/** 
			 * The children of this node in the 'data tree'.
			 * DataGroup is responsible for the cleanup of the @a _children
			 * container, but not the cleanup of its elements.
			 */
			Children_t _children;
	};
}

#endif  // _GPLATES_DATAGROUP_H_
