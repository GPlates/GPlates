/* $Id$ */

/**
 * \file Convenience functions for @a ReconstructionGeometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H
#define GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H

#include <vector>

#include "model/ReconstructionGeometry.h"


namespace GPlatesModel
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesAppLogic
{
	namespace ReconstructionGeometryUtils
	{
		/**
		 * Typedef for a sequence of @a ReconstructedFeatureGeometry pointers.
		 */
		typedef std::vector<GPlatesModel::ReconstructedFeatureGeometry *>
				reconstructed_feature_geom_seq_type;


		/**
		 * Returns @a ReconstructedFeatureGeometry if @a reconstruction_geom is an
		 * object of derived type @a ReconstructedFeatureGeometry.
		 */
		bool
		get_reconstructed_feature_geometry(
				GPlatesModel::ReconstructionGeometry *reconstruction_geom,
				GPlatesModel::ReconstructedFeatureGeometry *&reconstructed_feature_geom);

		/**
		 * Returns @a ReconstructedFeatureGeometry if @a reconstruction_geom is an
		 * object of derived type @a ReconstructedFeatureGeometry.
		 */
		inline
		bool
		get_reconstructed_feature_geometry(
				GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geom,
				GPlatesModel::ReconstructedFeatureGeometry *&reconstructed_feature_geom)
		{
			return get_reconstructed_feature_geometry(
					reconstruction_geom.get(), reconstructed_feature_geom);
		}


		/**
		 * Searches a sequence of @a ReconstructionGeometry objects for
		 * derived @a ReconstructedFeatureGeometry types and returns any found.
		 * Returns true if any found.
		 */
		template <typename ReconstructionGeometryForwardIter>
		bool
		get_reconstructed_feature_geometries(
				ReconstructionGeometryForwardIter reconstruction_geoms_begin,
				ReconstructionGeometryForwardIter reconstruction_geoms_end,
				reconstructed_feature_geom_seq_type &reconstructed_feature_geom_seq)
		{
			bool found = false;
			ReconstructionGeometryForwardIter recon_geom_iter;
			for (recon_geom_iter = reconstruction_geoms_begin;
				recon_geom_iter != reconstruction_geoms_end;
				++recon_geom_iter)
			{
				GPlatesModel::ReconstructedFeatureGeometry *rfg;
				if (get_reconstructed_feature_geometry(recon_geom_iter->get(), &rfg))
				{
					reconstructed_feature_geom_seq.push_back(rfg);
					found = true;
				}
			}

			return found;
		}
	}
}

#endif // GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H
