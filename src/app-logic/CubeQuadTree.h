/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_APP_LOGIC_CUBEQUADTREE_H
#define GPLATES_APP_LOGIC_CUBEQUADTREE_H

#include <vector>

#include "maths/UnitVector3D.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	template <typename NodeImplType>
	class CubeQuadTree :
			public GPlatesUtils::ReferenceCount<CubeQuadTree<NodeImplType> >
	{
	public:
		typedef CubeQuadTree<NodeImplType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create(
				unsigned int num_levels)
		{
			return non_null_ptr_type(new CubeQuadTree(num_levels));
		}


		enum CubeFaceType
		{
			POSITIVE_X,
			NEGATIVE_X,
			POSITIVE_Y,
			NEGATIVE_Y,
			POSITIVE_Z,
			NEGATIVE_Z
		};


		static
		const GPlatesMaths::UnitVector3D &
		get_u_direction_of_face(
				CubeFaceType cube_face)
		{
			return UV_FACE_DIRECTIONS[cube_face][0];
		}

		static
		const GPlatesMaths::UnitVector3D &
		get_v_direction_of_face(
				CubeFaceType cube_face)
		{
			return UV_FACE_DIRECTIONS[cube_face][1];
		}


		class QuadTreeNode
		{
		public:
		private:
			NodeImplType d_node_impl;
		};


		class QuadTreeLevel
		{
		public:
			QuadTreeLevel(
					unsigned int level);

			const QuadTreeNode &
			get_node(
					unsigned int node_u_index,
					unsigned int node_v_index) const;

			unsigned int
			get_level() const
			{
				return d_level;
			}

			unsigned int
			get_node_dimension() const
			{
				return 1 << d_level;
			}

		private:
			typedef std::vector<QuadTreeNode> node_seq_type;

			unsigned int d_level;

			node_seq_type d_nodes;
		};


		class QuadTree
		{
		public:
			QuadTree(
					unsigned int num_levels);

			const QuadTreeLevel &
			get_level_of_detail(
					unsigned int level_of_detail) const;

		private:
			typedef std::vector<QuadTreeLevel> level_seq_type;

			level_seq_type d_levels;
		};


		const QuadTree &
		get_quad_tree_of_face(
				CubeFaceType cube_face) const;

	private:
		typedef std::vector<QuadTree> quad_tree_seq_type;

		quad_tree_seq_type d_cube_faces[6];

		static const GPlatesMaths::UnitVector3D UV_FACE_DIRECTIONS[6][2];


		CubeQuadTree(
				unsigned int num_levels);
	};


	/**
	 * Standard directions used by 3D graphics APIs for cube map textures.
	 */
	template <typename NodeImplType>
	const GPlatesMaths::UnitVector3D
	CubeQuadTree<NodeImplType>::UV_FACE_DIRECTIONS[6][2] =
	{
		{ GPlatesMaths::UnitVector3D(0,0,-1), GPlatesMaths::UnitVector3D(0,-1,0) },
		{ GPlatesMaths::UnitVector3D(0,0,1), GPlatesMaths::UnitVector3D(0,-1,0) },
		{ GPlatesMaths::UnitVector3D(1,0,0), GPlatesMaths::UnitVector3D(0,0,1) },
		{ GPlatesMaths::UnitVector3D(1,0,0), GPlatesMaths::UnitVector3D(0,0,-1) },
		{ GPlatesMaths::UnitVector3D(1,0,0), GPlatesMaths::UnitVector3D(0,-1,0) },
		{ GPlatesMaths::UnitVector3D(-1,0,0), GPlatesMaths::UnitVector3D(0,-1,0) }
	};
}

#endif // GPLATES_APP_LOGIC_CUBEQUADTREE_H
