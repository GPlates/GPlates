/* $Id$ */

/**
 * \file 
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
 
#ifndef GPLATES_GUI_EXPORTANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/shared_ptr.hpp>

#include <QString>

#include "file-io/ExportTemplateFilenameSequence.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesGui
{

	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Base class for the different Export Animation strategies.
	 * ExportAnimationStrategy serves as the (abstract) Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportAnimationStrategy:
			public GPlatesUtils::ReferenceCount<ExportAnimationStrategy>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportAnimationStrategy> non_null_ptr_type;
		
		virtual
		~ExportAnimationStrategy()
		{  }


		class ConfigurationBase;

		//! Typedef for a shared pointer to const @a ConfigurationBase.
		typedef boost::shared_ptr<const ConfigurationBase> const_configuration_base_ptr;
		//! Typedef for a shared pointer to @a ConfigurationBase.
		typedef boost::shared_ptr<ConfigurationBase> configuration_base_ptr;

		/**
		 * Configuration parameters for an @a ExportAnimationStrategy.
		 *
		 * Contains filename template (common to all @a ExportAnimationStrategy derived types).
		 *
		 * If a derived @a ExportAnimationStrategy type requires specialised configuration
		 * parameters then create a derived @a Configuration class.
		 */
		class ConfigurationBase
		{
		public:
			virtual
			~ConfigurationBase()
			{  }

			explicit
			ConfigurationBase(
					const QString& _filename_template)
			{
				d_filename_template = _filename_template;
			}

			const QString&
			get_filename_template() const
			{
				return d_filename_template;
			}

			void
			set_filename_template(
					const QString &filename_template)
			{
				d_filename_template = filename_template;
			}

			//! Clones derived configuration object.
			virtual
			configuration_base_ptr
			clone() const = 0;

		private:
			QString d_filename_template;
		};


		/**
		 * Creates an export animation strategy that doesn't do anything.
		 *
		 * Only created when an export ID is requested of export animation registry that doesn't exist.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context)
		{
			return non_null_ptr_type(new ExportAnimationStrategy(export_animation_context));
		}


		/**
		 * Sets the internal ExportTemplateFilenameSequence.
		 */
		virtual
		void
		set_template_filename(
				const QString &filename);

		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual		
		bool
		do_export_iteration(
				std::size_t frame_index)
		{
			return false;
		}


		/**
		 * Allows Strategy objects to do any housekeeping that might be necessary
		 * after all export iterations are completed.
		 *
		 * Of course, there's also the destructor, which should free up any resources
		 * we acquired in the constructor; this method is intended for any "last step"
		 * iteration operations that might need to occur. Perhaps all iterations end
		 * up in the same file and we should close that file (if all steps completed
		 * successfully). This is the place to do those final steps.
		 *
		 * @param export_successful is true if all iterations were performed successfully,
		 *        false if there was any kind of interruption.
		 *
		 * Called by ExportAnimationContext.
		 */
		virtual
		void
		wrap_up(
				bool export_successful)
		{  }

		virtual
		bool
		check_filename_sequence();

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context);
		
		/**
		 * Pointer back to the Context, as an easy way to get at all kinds of state.
		 * See Gamma et al, Strategy pattern, p319 point 1.
		 */
		GPlatesGui::ExportAnimationContext *d_export_animation_context_ptr;
		
		/**
		 * The filename sequence to use when exporting.
		 */
		boost::optional<GPlatesFileIO::ExportTemplateFilenameSequence> d_filename_sequence_opt;
		boost::optional<GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator> d_filename_iterator_opt;
	};
}
#endif //GPLATES_GUI_EXPORTANIMATIONSTRATEGY_H
