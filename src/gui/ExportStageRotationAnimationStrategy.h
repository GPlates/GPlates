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
 
#ifndef GPLATES_GUI_EXPORTSTAGEROTATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTSTAGEROTATIONSTRATEGY_H

#include <boost/none.hpp>

#include <QString>

#include "ExportAnimationStrategy.h"
#include "ExportOptionsUtils.h"

#include "maths/UnitQuaternion3D.h"

#include "model/types.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructionTree;
}

namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for writing
	 * *stage* (t + delta_t -> t) rotation poles at each timestep 't' for either:
	 *  (1) equivalent (to anchor plate), or
	 *  (2) relative (fixed/moving pairs).
	 */
	class ExportStageRotationAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportStageRotationAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportStageRotationAnimationStrategy> non_null_ptr_type;
	

		/**
		 * Configuration options.
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
		{
		public:
 			enum RotationType
 			{
				RELATIVE_COMMA,
				RELATIVE_SEMICOLON,
				RELATIVE_TAB,
				EQUIVALENT_COMMA,
				EQUIVALENT_SEMICOLON,
				EQUIVALENT_TAB
			};

			explicit
			Configuration(
					const QString& filename_template_,
					RotationType rotation_type_,
					const ExportOptionsUtils::ExportRotationOptions &rotation_options_,
					const ExportOptionsUtils::ExportStageRotationOptions &stage_rotation_options_) :
				ConfigurationBase(filename_template_),
				rotation_type(rotation_type_),
				rotation_options(rotation_options_),
				stage_rotation_options(stage_rotation_options_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			RotationType rotation_type;
			ExportOptionsUtils::ExportRotationOptions rotation_options;
			ExportOptionsUtils::ExportStageRotationOptions stage_rotation_options;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration)
		{
			return non_null_ptr_type(
					new ExportStageRotationAnimationStrategy(
							export_animation_context,
							export_configuration));
		}

		virtual
		~ExportStageRotationAnimationStrategy()
		{  }

		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);


	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportStageRotationAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration);
		
	private:

		//! Export configuration parameters.
		const_configuration_ptr d_configuration;


		/**
		 * Calculates the relative stage rotation for the specified fixed/moving plate pair from time t2 -> t1.
		 */
		GPlatesMaths::UnitQuaternion3D
		get_relative_stage_rotation(
				const GPlatesAppLogic::ReconstructionTree &tree1,
				const GPlatesAppLogic::ReconstructionTree &tree2,
				GPlatesModel::integer_plate_id_type moving_plate_id,
				GPlatesModel::integer_plate_id_type fixed_plate_id) const;

		/**
		 * Calculates the equivalent stage rotation for the specified plate id from time t2 -> t1.
		 */
		GPlatesMaths::UnitQuaternion3D
		get_equivalent_stage_rotation(
				const GPlatesAppLogic::ReconstructionTree &tree1,
				const GPlatesAppLogic::ReconstructionTree &tree2,
				GPlatesModel::integer_plate_id_type plate_id) const;
	};
}


#endif //GPLATES_GUI_EXPORTSTAGEROTATIONSTRATEGY_H


