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

#ifndef GPLATES_APP_LOGIC_COREGISTRATIONLAYERPARAMS_H
#define GPLATES_APP_LOGIC_COREGISTRATIONLAYERPARAMS_H

#include <QObject>

#include "LayerParams.h"

#include "data-mining/CoRegConfigurationTable.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a co-registration layer.
	 */
	class CoRegistrationLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<CoRegistrationLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const CoRegistrationLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new CoRegistrationLayerParams());
		}


		/**
		 * Returns the 'const' configuration table.
		 */
		const GPlatesDataMining::CoRegConfigurationTable &
		get_cfg_table() const
		{
			return d_cfg_table;
		}

		/**
		 * Sets the configuration table.
		 *
		 * Emits signals 'modified_cfg_table' and 'modified' if a change detected.
		 */
		void
		set_cfg_table(
				const GPlatesDataMining::CoRegConfigurationTable &table);


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_co_registration_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_co_registration_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_cfg_table has been called (if a change detected).
		 */
		void
		modified_cfg_table(
				GPlatesAppLogic::CoRegistrationLayerParams &layer_params);

	private:

		GPlatesDataMining::CoRegConfigurationTable d_cfg_table;


		CoRegistrationLayerParams()
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_COREGISTRATIONLAYERPARAMS_H
