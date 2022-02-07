/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPARAMS_H
#define GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPARAMS_H

#include <QObject>

#include "LayerParams.h"
#include "VelocityParams.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a velocity layer.
	 */
	class VelocityFieldCalculatorLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<VelocityFieldCalculatorLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const VelocityFieldCalculatorLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new VelocityFieldCalculatorLayerParams());
		}


		/**
		 * Returns the 'const' velocity parameters.
		 */
		const VelocityParams &
		get_velocity_params() const
		{
			return d_velocity_params;
		}

		/**
		 * Sets the velocity parameters.
		 *
		 * Emits signals 'modified_velocity_params' and 'modified' if a change detected.
		 */
		void
		set_velocity_params(
				const VelocityParams &velocity_params);


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_velocity_field_calculator_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_velocity_field_calculator_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_velocity_params has been called (if a change detected).
		 */
		void
		modified_velocity_params(
				GPlatesAppLogic::VelocityFieldCalculatorLayerParams &layer_params);

	private:

		VelocityParams d_velocity_params;


		VelocityFieldCalculatorLayerParams()
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPARAMS_H
