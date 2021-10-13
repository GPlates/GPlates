/* $Id$ */

/**
 * @file 
 * Contains the definition of class MultiPointVectorField.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_COREGISTRATIONDATA_H
#define GPLATES_APP_LOGIC_COREGISTRATIONDATA_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"
#include "data-mining/DataTable.h"
#include "model/FeatureHandle.h"
#include "model/WeakObserver.h"

namespace GPlatesAppLogic
{
	/**
	 * CoRegistrationData defines a derived class of ReconstructionGeometry. 
	 * The derived class is used to keep co-registration result data.
	 * TODO: more comments
	 */
	class CoRegistrationData:
			public ReconstructionGeometry
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<CoRegistrationData> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const CoRegistrationData> non_null_ptr_to_const_type;


		/**
		 * Creates a new @a CoRegistrationData object.
		 */
		static
		non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_)
		{
			return non_null_ptr_type(new CoRegistrationData(reconstruction_tree_));
		}


		virtual
		~CoRegistrationData()
		{  }


		const GPlatesDataMining::DataTable&
		data_table() const 
		{
			return d_table;
		}

		GPlatesDataMining::DataTable&
		data_table() 
		{
			return d_table;
		}

		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		
		non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		
		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		*/
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		
		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		*/
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

	private:
		GPlatesDataMining::DataTable d_table;
	

		/**
		 * Constructor is private so it cannot be created on the runtime stack which could cause
		 * problems if a client then tries to get a non_null_ptr_type from the stack object.
		 */
		explicit
		CoRegistrationData(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_):
			ReconstructionGeometry(reconstruction_tree_)
		{  }
	};
		
}

#endif  // GPLATES_APP_LOGIC_COREGISTRATIONDATA_H

