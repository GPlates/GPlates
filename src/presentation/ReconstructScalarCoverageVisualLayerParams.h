/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_RECONSTRUCTSCALARCOVERAGEVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_RECONSTRUCTSCALARCOVERAGEVISUALLAYERPARAMS_H

#include <map>
#include <boost/optional.hpp>

#include "RemappedColourPaletteParameters.h"
#include "VisualLayerParams.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesPresentation
{
	class ReconstructScalarCoverageVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructScalarCoverageVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructScalarCoverageVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params)
		{
			return new ReconstructScalarCoverageVisualLayerParams(layer_params);
		}


		/**
		 * Returns the current colour palette (associated with the current scalar type).
		 */
		const RemappedColourPaletteParameters &
		get_current_colour_palette_parameters() const;

		/**
		 * Sets the current colour palette (associated with the current scalar type).
		 */
		void
		set_current_colour_palette_parameters(
				const RemappedColourPaletteParameters &colour_palette_parameters);


		/**
		 * Returns the colour palette associated with the specified scalar type.
		 */
		boost::optional<const RemappedColourPaletteParameters &>
		get_colour_palette_parameters(
				const GPlatesPropertyValues::ValueObjectType &scalar_type) const;

		/**
		 * Sets the colour palette associated with the specified scalar type.
		 */
		void
		set_colour_palette_parameters(
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const RemappedColourPaletteParameters &colour_palette_parameters);


		/**
		 * Returns the currently selected scalar type.
		 *
		 * Delegates to app-logic ReconstructScalarCoverageLayerParams::get_scalar_type().
		 */
		const GPlatesPropertyValues::ValueObjectType &
		get_current_scalar_type() const;

		/**
		 * Returns the list of scalar types available in the scalar coverage features.
		 *
		 * Delegates to app-logic ReconstructScalarCoverageLayerParams::get_scalar_types().
		 */
		const std::vector<GPlatesPropertyValues::ValueObjectType> &
		get_scalar_types() const;


		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_reconstruct_scalar_coverage_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruct_scalar_coverage_visual_layer_params(*this);
		}

		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);

	protected:

		explicit 
		ReconstructScalarCoverageVisualLayerParams( 
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params);

	private:

		/**
		 * Typedef for map from scalar type to colour palette parameters.
		 */
		typedef std::map<GPlatesPropertyValues::ValueObjectType, RemappedColourPaletteParameters>
				colour_palette_parameters_map_type;


		/**
		 * The colour palette(s) for this layer, whether set explicitly as loaded from a file,
		 * or auto-generated.
		 *
		 * These are mapped from the scalar type.
		 *
		 * Note: It's mutable since palettes are created on retrieval if they don't already exist.
		 */
		mutable colour_palette_parameters_map_type d_colour_palette_parameters_map;


		const RemappedColourPaletteParameters &
		create_colour_palette_parameters(
				const GPlatesPropertyValues::ValueObjectType &scalar_type) const;

		const RemappedColourPaletteParameters &
		get_or_create_colour_palette_parameters(
				const GPlatesPropertyValues::ValueObjectType &scalar_type) const;
	};
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTSCALARCOVERAGEVISUALLAYERPARAMS_H
