/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTANIMATIONREGISTRY_H
#define GPLATES_GUI_EXPORTANIMATIONREGISTRY_H

#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "ExportAnimationStrategy.h"
#include "ExportAnimationType.h"

// Forward declaration.
class QWidget;


namespace GPlatesQtWidgets
{
	class ExportOptionsWidget;
}

namespace GPlatesUtils
{
	class ExportFileNameTemplateValidator;
}

namespace GPlatesGui
{
	class ExportAnimationContext;

	/**
	 * Stores information required to create @a ExportAnimationStrategy objects.
	 */
	class ExportAnimationRegistry :
			public boost::noncopyable
	{
	public:
		/**
		 * Convenience typedef for a function that creates a @a ExportAnimationStrategy.
		 *
		 * The function takes the following arguments:
		 * - An @a ExportAnimationContext, and
		 * - An @a ExportAnimationStrategy::const_configuration_base_ptr.
		 *
		 * Returns the created @a ExportAnimationStrategy.
		 */
		typedef ExportAnimationStrategy::non_null_ptr_type
				create_export_animation_strategy_function_signature_type(
						ExportAnimationContext &,
						const ExportAnimationStrategy::const_configuration_base_ptr &);

		//! The boost::function typedef that creates a @a ExportAnimationStrategy.
		typedef boost::function<create_export_animation_strategy_function_signature_type>
				create_export_animation_strategy_function_type;


		/**
		 * Convenience typedef for a function that creates a @a ExportOptionsWidget.
		 *
		 * The function takes the following arguments:
		 * - An QWidget (the parent of the export options widget).
		 * - A @a ExportAnimationStrategy::const_configuration_base_ptr
		 *   (the value registered in @a register_exporter is what will get passed in here).
		 *
		 * Returns the created @a ExportOptionsWidget.
		 */
		typedef GPlatesQtWidgets::ExportOptionsWidget *
				create_export_options_widget_function_signature_type(
						QWidget *,
						ExportAnimationContext &,
						const ExportAnimationStrategy::const_configuration_base_ptr &);

		//! The boost::function typedef that creates a @a ExportOptionsWidget.
		typedef boost::function<create_export_options_widget_function_signature_type>
				create_export_options_widget_function_type;


		/**
		 * Convenience typedef for a function that validates a filename template.
		 *
		 * The function takes the following arguments:
		 * - A filename template,
		 * - A message regarding the validation,
		 * - A bool indicating whether to check for filename variation or not.
		 *
		 * Returns true if successfully validated.
		 */
		typedef bool
				validate_filename_template_function_signature_type(
						const QString &,
						QString &,
						bool);

		//! The boost::function typedef that validates a filename template.
		typedef boost::function<validate_filename_template_function_signature_type>
				validate_filename_template_function_type;


		/**
		 * Stores information about the given @a export_id_.
		 */
		void
		register_exporter(
				ExportAnimationType::ExportID export_id_,
				const ExportAnimationStrategy::const_configuration_base_ptr &export_configuration,
				//const QString &filename_template_description_,
				const create_export_animation_strategy_function_type &create_export_animation_strategy_function_,
				const create_export_options_widget_function_type &create_export_options_widget_function_,
				const validate_filename_template_function_type &validate_filename_template_function_);

		/**
		 * Unregisters the specified export ID.
		 */
		void
		unregister_exporter(
				ExportAnimationType::ExportID export_id);


		/**
		 * Returns a list of export IDs of all registered exporters.
		 */
		std::vector<ExportAnimationType::ExportID>
		get_registered_exporters() const;

		/**
		 * Returns the default export configuration for the specified export ID.
		 *
		 * NOTE: Returns NULL if the given export id has not been registered.
		 */
		ExportAnimationStrategy::const_configuration_base_ptr
		get_default_export_configuration(
				ExportAnimationType::ExportID export_id) const;

		/**
		 * Returns the default filename template for the specified export ID.
		 *
		 * Returns empty string if the given export id has not been registered.
		 */
		const QString &
		get_default_filename_template(
				ExportAnimationType::ExportID export_id) const;

#if 0
		/**
		 * Returns the filename template description for the specified export ID.
		 *
		 * Returns empty string if the given export id has not been registered.
		 */
		const QString &
		get_filename_template_description(
				ExportAnimationType::ExportID export_id) const;
#endif

		/**
		 * Causes a new export animation strategy of the given type to be created;
		 * the export ID must have been already registered.
		 */
		ExportAnimationStrategy::non_null_ptr_type
		create_export_animation_strategy(
				ExportAnimationType::ExportID export_id,
				ExportAnimationContext &export_animation_context,
				const ExportAnimationStrategy::const_configuration_base_ptr &export_configuration) const;

		/**
		 * Returns a widget to allow the user to specify export animation options for the
		 * specified export ID.
		 *
		 * If @a export_configuration is specified then it will be the configuration that's edited.
		 * Otherwise the default export configuration will be edited.
		 *
		 * @a parent is the widget used to parent the created export options widget.
		 *
		 * Returns boost::none if there is no widget for the specified export ID,
		 * or if the given ID has not been registered.
		 */
		boost::optional<GPlatesQtWidgets::ExportOptionsWidget *>
		create_export_options_widget(
				ExportAnimationType::ExportID export_id,
				QWidget *parent,
				ExportAnimationContext &export_animation_context,
				boost::optional<ExportAnimationStrategy::const_configuration_base_ptr> export_configuration = boost::none) const;

		/**
		 * Returns true if @a filename_template is valid for the specified export ID.
		 *
		 * @a filename_template_validation_message is set to the valid message, if any.
		 *
		 * If @a check_filename_variation is true then also checks that there is filename variation
		 * (varies with reconstruction time).
		 * This should normally be true except when a exporting for a single time instant.
		 */
		bool
		validate_filename_template(
				ExportAnimationType::ExportID export_id,
				const QString &filename_template,
				QString &filename_template_validation_message,
				bool check_filename_variation = true) const;


	private:
		struct ExporterInfo
		{
			ExporterInfo(
					const ExportAnimationStrategy::const_configuration_base_ptr &default_export_configuration_,
					const create_export_animation_strategy_function_type &create_export_animation_strategy_function_,
					const create_export_options_widget_function_type &create_export_options_widget_function_,
					const validate_filename_template_function_type &validate_filename_template_function_) :
				default_export_configuration(default_export_configuration_),
				create_export_animation_strategy_function(create_export_animation_strategy_function_),
				create_export_options_widget_function(create_export_options_widget_function_),
				validate_filename_template_function(validate_filename_template_function_)
			{  }

			ExportAnimationStrategy::const_configuration_base_ptr default_export_configuration;
			create_export_animation_strategy_function_type create_export_animation_strategy_function;
			create_export_options_widget_function_type create_export_options_widget_function;
			validate_filename_template_function_type validate_filename_template_function;
		};

		typedef std::map<ExportAnimationType::ExportID, ExporterInfo> exporter_info_map_type;


		/**
		 * Stores a struct of information for each export ID.
		 */
		exporter_info_map_type d_exporter_info_map;
	};

	/**
	 * Registers information about the default export animation types with the given @a registry.
	 */
	void
	register_default_export_animation_types(
			ExportAnimationRegistry &registry);
}

#endif // GPLATES_GUI_EXPORTANIMATIONREGISTRY_H
