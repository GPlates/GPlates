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

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ExportAnimationRegistry.h"

#include "ExportCitcomsResolvedTopologyAnimationStrategy.h"
#include "ExportCoRegistrationAnimationStrategy.h"
#include "ExportFileNameTemplateValidationUtils.h"
#include "ExportFlowlineAnimationStrategy.h"
#include "ExportMotionPathAnimationStrategy.h"
#include "ExportRasterAnimationStrategy.h"
#include "ExportReconstructedGeometryAnimationStrategy.h"
#include "ExportResolvedTopologyAnimationStrategy.h"
#include "ExportStageRotationAnimationStrategy.h"
#include "ExportSvgAnimationStrategy.h"
#include "ExportTotalRotationAnimationStrategy.h"
#include "ExportVelocityAnimationStrategy.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "qt-widgets/ExportCitcomsResolvedTopologyOptionsWidget.h"
#include "qt-widgets/ExportFlowlineOptionsWidget.h"
#include "qt-widgets/ExportMotionPathOptionsWidget.h"
#include "qt-widgets/ExportReconstructedGeometryOptionsWidget.h"
#include "qt-widgets/ExportResolvedTopologyOptionsWidget.h"
#include "qt-widgets/ExportStageRotationOptionsWidget.h"
#include "qt-widgets/ExportTotalRotationOptionsWidget.h"

namespace GPlatesGui
{
	namespace
	{
		/**
		 * Convenience function to cast a ExportAnimationStrategy::const_configuration_base_ptr into
		 * a derived class 'ExportAnimationStrategyType::const_configuration_ptr'.
		 */
		template <class ExportAnimationStrategyType>
		typename ExportAnimationStrategyType::const_configuration_ptr
		dynamic_cast_export_configuration(
				const ExportAnimationStrategy::const_configuration_base_ptr &export_configuration)
		{
			// Cast the 'ExportAnimationStrategy::ConfigurationBase'
			// to a 'ExportAnimationStrategyType::Configuration'.
			typename ExportAnimationStrategyType::const_configuration_ptr
					derived_export_configuration =
							boost::dynamic_pointer_cast<
									const typename ExportAnimationStrategyType::Configuration>(
											export_configuration);

			// The cast failed - this shouldn't happen - assert so programmer can fix bug.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					derived_export_configuration,
					GPLATES_ASSERTION_SOURCE);

			return derived_export_configuration;
		}


		/**
		 * Function to create an @a ExportAnimationStrategy derived type ExportAnimationStrategyType.
		 *
		 * Expects derived type ExportAnimationStrategyType to contain:
		 * - a nested type called 'const_configuration_base_ptr' that is implicitly convertible
		 *   to a ExportAnimationStrategy::const_configuration_base_ptr (ie, a boost::shared_ptr), and
		 * - a nested type called 'Configuration' that is derived from
		 *   ExportAnimationStrategy::ConfigurationBase.
		 */
		template <class ExportAnimationStrategyType>
		ExportAnimationStrategy::non_null_ptr_type
		create_animation_strategy(
				ExportAnimationContext &export_animation_context,
				const ExportAnimationStrategy::const_configuration_base_ptr &export_configuration)
		{
			return ExportAnimationStrategyType::create(
					export_animation_context,
					dynamic_cast_export_configuration<ExportAnimationStrategyType>(export_configuration));
		}


		/**
		 * Function to create an @a ExportOptionsWidget derived type ExportOptionsWidgetType
		 * passing in a default export configuration.
		 *
		 * Expects derived type ExportOptionsWidgetType to contain a static method 'create'
		 * that accepts the following parameters:
		 * - a 'QWidget' pointer (the parent widget), and
		 * - an export configuration of type ExportAnimationStrategyType::const_configuration_ptr (see below).
		 *
		 * Expects derived type ExportAnimationStrategyType to contain:
		 * - a nested type called 'const_configuration_base_ptr' that is implicitly convertible
		 *   to a ExportAnimationStrategy::const_configuration_base_ptr (ie, a boost::shared_ptr), and
		 * - a nested type called 'Configuration' that is derived from
		 *   ExportAnimationStrategy::ConfigurationBase.
		 */
		template <class ExportOptionsWidgetType, class ExportAnimationStrategyType>
		GPlatesQtWidgets::ExportOptionsWidget *
		create_export_options_widget(
				QWidget *parent,
				const ExportAnimationStrategy::const_configuration_base_ptr &default_export_configuration)
		{
			return ExportOptionsWidgetType::create(
					parent,
					dynamic_cast_export_configuration<ExportAnimationStrategyType>(default_export_configuration));
		}

		/**
		 * Same as the other overload of @a create_export_options_widget but has an extra parameter.
		 */
		template <class ExportOptionsWidgetType, class ExportAnimationStrategyType, typename A1>
		GPlatesQtWidgets::ExportOptionsWidget *
		create_export_options_widget(
				QWidget *parent,
				const ExportAnimationStrategy::const_configuration_base_ptr &default_export_configuration,
				const A1 &arg1)
		{
			return ExportOptionsWidgetType::create(
					parent,
					dynamic_cast_export_configuration<ExportAnimationStrategyType>(default_export_configuration),
					arg1);
		}

		/**
		 * A convenience typedef because a static_cast is needed on some compilers to help determine
		 * the correct overload of 'create_export_options_widget()' to use with boost::bind.
		 *
		 * Note: It seems the static_cast is only needed for the overload with no extra arguments.
		 */
		typedef GPlatesQtWidgets::ExportOptionsWidget *
				(*create_export_options_widget_function_pointer_type)(
						QWidget *,
						const ExportAnimationStrategy::const_configuration_base_ptr &);


		/**
		 * A function that returns a NULL @a ExportOptionsWidget.
		 *
		 * Used for those exporters that don't need an export options widget to configure
		 * their export parameters.
		 */
		GPlatesQtWidgets::ExportOptionsWidget *
		create_null_export_options_widget(
				QWidget *,
				const ExportAnimationStrategy::const_configuration_base_ptr &)
		{
			return NULL;
		}
	}
}


void
GPlatesGui::ExportAnimationRegistry::register_exporter(
		ExportAnimationType::ExportID export_id_,
		const ExportAnimationStrategy::const_configuration_base_ptr &default_export_configuration,
		const create_export_animation_strategy_function_type &create_export_animation_strategy_function_,
		const create_export_options_widget_function_type &create_export_options_widget_function_,
		const validate_filename_template_function_type &validate_filename_template_function_)
{
	d_exporter_info_map.insert(
			std::make_pair(
				export_id_,
				ExporterInfo(
					default_export_configuration,
					create_export_animation_strategy_function_,
					create_export_options_widget_function_,
					validate_filename_template_function_)));
}


void
GPlatesGui::ExportAnimationRegistry::unregister_exporter(
		ExportAnimationType::ExportID export_id)
{
	d_exporter_info_map.erase(export_id);
}


std::vector<GPlatesGui::ExportAnimationType::ExportID>
GPlatesGui::ExportAnimationRegistry::get_registered_exporters() const
{
	std::vector<ExportAnimationType::ExportID> export_ids;

	BOOST_FOREACH(
			const exporter_info_map_type::value_type &exporter_entry,
			d_exporter_info_map)
	{
		export_ids.push_back(exporter_entry.first);
	}

	return export_ids;
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesGui::ExportAnimationRegistry::get_default_export_configuration(
		ExportAnimationType::ExportID export_id) const
{
	exporter_info_map_type::const_iterator iter = d_exporter_info_map.find(export_id);
	if (iter == d_exporter_info_map.end())
	{
		return ExportAnimationStrategy::const_configuration_base_ptr();
	}

	const ExporterInfo &exporter_info = iter->second;

	return exporter_info.default_export_configuration;
}


const QString &
GPlatesGui::ExportAnimationRegistry::get_default_filename_template(
		ExportAnimationType::ExportID export_id) const
{
	const ExportAnimationStrategy::const_configuration_base_ptr export_configuration =
			get_default_export_configuration(export_id);

	if (!export_configuration)
	{
		static const QString empty_filename_template;
		return empty_filename_template;
	}

	return export_configuration->get_filename_template();
}


GPlatesGui::ExportAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportAnimationRegistry::create_export_animation_strategy(
		ExportAnimationType::ExportID export_id,
		ExportAnimationContext &export_animation_context,
		const ExportAnimationStrategy::const_configuration_base_ptr &export_configuration) const
{
	exporter_info_map_type::const_iterator iter = d_exporter_info_map.find(export_id);
	if (iter == d_exporter_info_map.end())
	{
		return ExportAnimationStrategy::create(export_animation_context);
	}

	const ExporterInfo &exporter_info = iter->second;

	return exporter_info.create_export_animation_strategy_function(
			export_animation_context,
			export_configuration);
}


boost::optional<GPlatesQtWidgets::ExportOptionsWidget *>
GPlatesGui::ExportAnimationRegistry::create_export_options_widget(
		ExportAnimationType::ExportID export_id,
		QWidget *parent) const
{
	exporter_info_map_type::const_iterator iter = d_exporter_info_map.find(export_id);
	if (iter == d_exporter_info_map.end())
	{
		return boost::none;
	}

	const ExporterInfo &exporter_info = iter->second;

	GPlatesQtWidgets::ExportOptionsWidget *export_options_widget =
			exporter_info.create_export_options_widget_function(
					parent,
					exporter_info.default_export_configuration);

	if (export_options_widget == NULL)
	{
		return boost::none;
	}

	return export_options_widget;
}


bool
GPlatesGui::ExportAnimationRegistry::validate_filename_template(
		ExportAnimationType::ExportID export_id,
		const QString &filename_template,
		QString &filename_template_validation_message) const
{
	exporter_info_map_type::const_iterator iter = d_exporter_info_map.find(export_id);
	if (iter == d_exporter_info_map.end())
	{
		return false;
	}

	const ExporterInfo &exporter_info = iter->second;

	return exporter_info.validate_filename_template_function(
			filename_template,
			filename_template_validation_message);
}


void
GPlatesGui::register_default_export_animation_types(
		ExportAnimationRegistry &registry)
{
	//
	// Export reconstructed geometries
	//

	// By default only export to multiple files (one output file per input file) as this
	// is the most requested output.
	const ExportOptionsUtils::ExportFileOptions default_reconstructed_geometry_file_export_options(
			/*export_to_a_single_file_*/true,
			/*export_to_multiple_files_*/true);
	const bool default_reconstructed_geometry_wrap_to_dateline = false;

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RECONSTRUCTED_GEOMETRIES,
					ExportAnimationType::GMT),
			ExportReconstructedGeometryAnimationStrategy::const_configuration_ptr(
					new ExportReconstructedGeometryAnimationStrategy::Configuration(
							"reconstructed_%0.2fMa.xy",
							ExportReconstructedGeometryAnimationStrategy::Configuration::GMT,
							default_reconstructed_geometry_file_export_options,
							default_reconstructed_geometry_wrap_to_dateline)),
			&create_animation_strategy<ExportReconstructedGeometryAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportReconstructedGeometryOptionsWidget,
							ExportReconstructedGeometryAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RECONSTRUCTED_GEOMETRIES,
					ExportAnimationType::SHAPEFILE),
			ExportReconstructedGeometryAnimationStrategy::const_configuration_ptr(
					new ExportReconstructedGeometryAnimationStrategy::Configuration(
							"reconstructed_%0.2fMa.shp",
							ExportReconstructedGeometryAnimationStrategy::Configuration::SHAPEFILE,
							default_reconstructed_geometry_file_export_options,
							default_reconstructed_geometry_wrap_to_dateline)),
			&create_animation_strategy<ExportReconstructedGeometryAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportReconstructedGeometryOptionsWidget,
							ExportReconstructedGeometryAnimationStrategy,
							bool>,
					// The 'true' allows user to turn on/off dateline wrapping of geometries...
					_1, _2, true),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RECONSTRUCTED_GEOMETRIES,
					ExportAnimationType::OGRGMT),
			ExportReconstructedGeometryAnimationStrategy::const_configuration_ptr(
				new ExportReconstructedGeometryAnimationStrategy::Configuration(
						"reconstructed_%0.2fMa.gmt",
						ExportReconstructedGeometryAnimationStrategy::Configuration::SHAPEFILE,
						default_reconstructed_geometry_file_export_options,
						default_reconstructed_geometry_wrap_to_dateline)),
			&create_animation_strategy<ExportReconstructedGeometryAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportReconstructedGeometryOptionsWidget,
							ExportReconstructedGeometryAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export projected geometries
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::PROJECTED_GEOMETRIES,
					ExportAnimationType::SVG),
			ExportSvgAnimationStrategy::const_configuration_ptr(
					new ExportSvgAnimationStrategy::Configuration(
							"snapshot_%0.2fMa.svg")),
			&create_animation_strategy<ExportSvgAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export velocities
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::MESH_VELOCITIES,
					ExportAnimationType::GPML),
			ExportVelocityAnimationStrategy::const_configuration_ptr(
					new ExportVelocityAnimationStrategy::Configuration(
							"velocity_colat_lon_on_mesh_%P_at_%0.2fMa.gpml")),
			&create_animation_strategy<ExportVelocityAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_with_percent_P);

	//
	// Export resolved topologies (general)
	//

	const ExportOptionsUtils::ExportFileOptions default_resolved_topology_file_export_options(
			/*export_to_a_single_file_*/true,
			/*export_to_multiple_files_*/true);
	const bool default_resolved_topology_export_lines = true;
	const bool default_resolved_topology_export_polygons = true;
	const bool default_resolved_topology_wrap_to_dateline = false;

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RESOLVED_TOPOLOGIES,
					ExportAnimationType::GMT),
			ExportResolvedTopologyAnimationStrategy::const_configuration_ptr(
					new ExportResolvedTopologyAnimationStrategy::Configuration(
							"topology_%0.2fMa.xy",
							ExportResolvedTopologyAnimationStrategy::Configuration::GMT,
							default_resolved_topology_file_export_options,
							default_resolved_topology_export_lines,
							default_resolved_topology_export_polygons,
							default_resolved_topology_wrap_to_dateline)),
			&create_animation_strategy<ExportResolvedTopologyAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget,
							ExportResolvedTopologyAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RESOLVED_TOPOLOGIES,
					ExportAnimationType::SHAPEFILE),
			ExportResolvedTopologyAnimationStrategy::const_configuration_ptr(
					new ExportResolvedTopologyAnimationStrategy::Configuration(
							"topology_%0.2fMa.shp",
							ExportResolvedTopologyAnimationStrategy::Configuration::SHAPEFILE,
							default_resolved_topology_file_export_options,
							default_resolved_topology_export_lines,
							default_resolved_topology_export_polygons,
							default_resolved_topology_wrap_to_dateline)),
			&create_animation_strategy<ExportResolvedTopologyAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget,
							ExportResolvedTopologyAnimationStrategy,
							bool>,
					// The 'true' allows the user to turn on/off dateline wrapping of geometries...
					_1, _2, true),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
				ExportAnimationType::RESOLVED_TOPOLOGIES,
					ExportAnimationType::OGRGMT),
			ExportResolvedTopologyAnimationStrategy::const_configuration_ptr(
					new ExportResolvedTopologyAnimationStrategy::Configuration(
							"topology_%02fMa.gmt",
							ExportResolvedTopologyAnimationStrategy::Configuration::OGRGMT,
							default_resolved_topology_file_export_options,
							default_resolved_topology_export_lines,
							default_resolved_topology_export_polygons,
							default_resolved_topology_wrap_to_dateline)),
			&create_animation_strategy<ExportResolvedTopologyAnimationStrategy>,
            boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget,
							ExportResolvedTopologyAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
                    _1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);


	//
	// Export resolved topologies (CitcomS-specific)
	//

	// Set defaults 
	const GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::OutputOptions
			default_citcoms_resolved_topology_export_options(
					/*wrap_geometries_to_the_dateline*/false,

					/*export_individual_plate_polygon_files*/false,
					/*export_all_plate_polygons_to_a_single_file*/true,
					/*export_plate_polygon_subsegments_to_lines*/false,

					// NOTE: all of these must be set to true to enable the check box on the gui: 
					// checkBox_export_plate_polygon_subsegments_to_type_files 

					/*export_ridge_transforms*/true,
					/*export_subductions*/true,
					/*export_left_subductions*/true,
					/*export_right_subductions*/true,

					/*export_individual_network_boundary_files*/false,
					/*export_all_network_boundaries_to_a_single_file*/true,
					/*export_network_polygon_subsegments_to_lines*/false,

					// NOTE: all of these must be set to true to enable the check box on the gui: 
					// checkBox_export_networks_polygon_subsegments_to_type_files 

					/*export_network_ridge_transforms*/true,
					/*export_network_subductions*/true,
					/*export_network_left_subductions*/true,
					/*export_network_right_subductions*/true,

					/*export_individual_slab_polygon_files*/false,
					/*export_all_slab_polygons_to_a_single_file*/true,
					/*export_slab_polygon_subsegments_to_lines*/false,

					// NOTE: all of these must be set to true to enable the check box on the gui: 
					// checkBox_export_slab_polygon_subsegments_to_type_files 

					/*export_slab_edge_leading*/true,
					/*export_slab_edge_leading_left*/true,
					/*export_slab_edge_leading_right*/true,
					/*export_slab_edge_trench*/true,
					/*export_slab_edge_side*/true
				);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RESOLVED_TOPOLOGIES_CITCOMS,
					ExportAnimationType::GMT),
			ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr(
					new ExportCitcomsResolvedTopologyAnimationStrategy::Configuration(
							"topology_%P_%0.2fMa.xy",
							ExportCitcomsResolvedTopologyAnimationStrategy::Configuration::GMT,
							default_citcoms_resolved_topology_export_options)),
			&create_animation_strategy<ExportCitcomsResolvedTopologyAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget,
							ExportCitcomsResolvedTopologyAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_with_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RESOLVED_TOPOLOGIES_CITCOMS,
					ExportAnimationType::SHAPEFILE),
			ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr(
					new ExportCitcomsResolvedTopologyAnimationStrategy::Configuration(
							"topology_%P_%0.2fMa.shp",
							ExportCitcomsResolvedTopologyAnimationStrategy::Configuration::SHAPEFILE,
							default_citcoms_resolved_topology_export_options)),
			&create_animation_strategy<ExportCitcomsResolvedTopologyAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget,
							ExportCitcomsResolvedTopologyAnimationStrategy,
							bool>,
					// The 'true' allows the user to turn on/off dateline wrapping of geometries...
					_1, _2, true),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_with_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
				ExportAnimationType::RESOLVED_TOPOLOGIES_CITCOMS,
					ExportAnimationType::OGRGMT),
			ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr(
					new ExportCitcomsResolvedTopologyAnimationStrategy::Configuration(
							"topology_%P_%02fMa.gmt",
							ExportCitcomsResolvedTopologyAnimationStrategy::Configuration::OGRGMT,
							default_citcoms_resolved_topology_export_options)),
			&create_animation_strategy<ExportCitcomsResolvedTopologyAnimationStrategy>,
            boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget,
							ExportCitcomsResolvedTopologyAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
                    _1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_with_percent_P);


	//
	// Export rotations
	//

	// By default write out identity rotations as "Indeterminate".
	const ExportOptionsUtils::ExportRotationOptions default_rotation_export_options(
			ExportOptionsUtils::ExportRotationOptions::WRITE_IDENTITY_AS_INDETERMINATE,
			ExportOptionsUtils::ExportRotationOptions::WRITE_EULER_POLE_AS_LATITUDE_LONGITUDE);

	//
	// Export relative total rotations
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RELATIVE_TOTAL_ROTATION,
					ExportAnimationType::CSV_COMMA),
			ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new ExportTotalRotationAnimationStrategy::Configuration(
							"relative_total_rotation_comma_%0.2fMa.csv",
							ExportTotalRotationAnimationStrategy::Configuration::RELATIVE_COMMA,
							default_rotation_export_options)),
			&create_animation_strategy<ExportTotalRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportTotalRotationOptionsWidget,
									ExportTotalRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RELATIVE_TOTAL_ROTATION,
					ExportAnimationType::CSV_SEMICOLON),
			ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new ExportTotalRotationAnimationStrategy::Configuration(
							"relative_total_rotation_semicolon_%0.2fMa.csv",
							ExportTotalRotationAnimationStrategy::Configuration::RELATIVE_SEMICOLON,
							default_rotation_export_options)),
			&create_animation_strategy<ExportTotalRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportTotalRotationOptionsWidget,
									ExportTotalRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RELATIVE_TOTAL_ROTATION,
					ExportAnimationType::CSV_TAB),
			ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new ExportTotalRotationAnimationStrategy::Configuration(
							"relative_total_rotation_tab_%0.2fMa.csv",
							ExportTotalRotationAnimationStrategy::Configuration::RELATIVE_TAB,
							default_rotation_export_options)),
			&create_animation_strategy<ExportTotalRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportTotalRotationOptionsWidget,
									ExportTotalRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export equivalent total rotations
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::EQUIVALENT_TOTAL_ROTATION,
					ExportAnimationType::CSV_COMMA),
			ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new ExportTotalRotationAnimationStrategy::Configuration(
							"equivalent_total_rotation_comma_%0.2fMa.csv",
							ExportTotalRotationAnimationStrategy::Configuration::EQUIVALENT_COMMA,
							default_rotation_export_options)),
			&create_animation_strategy<ExportTotalRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportTotalRotationOptionsWidget,
									ExportTotalRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::EQUIVALENT_TOTAL_ROTATION,
					ExportAnimationType::CSV_SEMICOLON),
			ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new ExportTotalRotationAnimationStrategy::Configuration(
							"equivalent_total_rotation_semicolon_%0.2fMa.csv",
							ExportTotalRotationAnimationStrategy::Configuration::EQUIVALENT_SEMICOLON,
							default_rotation_export_options)),
			&create_animation_strategy<ExportTotalRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportTotalRotationOptionsWidget,
									ExportTotalRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::EQUIVALENT_TOTAL_ROTATION,
					ExportAnimationType::CSV_TAB),
			ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new ExportTotalRotationAnimationStrategy::Configuration(
							"equivalent_total_rotation_tab_%0.2fMa.csv",
							ExportTotalRotationAnimationStrategy::Configuration::EQUIVALENT_TAB,
							default_rotation_export_options)),
			&create_animation_strategy<ExportTotalRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportTotalRotationOptionsWidget,
									ExportTotalRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export relative stage rotations
	//

	// Default *stage* rotation export options.
	const ExportOptionsUtils::ExportStageRotationOptions default_stage_rotation_export_options(
			// Default stage rotation time interval is 1.0 My...
			1.0);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RELATIVE_STAGE_ROTATION,
					ExportAnimationType::CSV_SEMICOLON),
			ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new ExportStageRotationAnimationStrategy::Configuration(
							"relative_stage_rotation_semicolon_%0.2fMa.csv",
							ExportStageRotationAnimationStrategy::Configuration::RELATIVE_SEMICOLON,
							default_rotation_export_options,
							default_stage_rotation_export_options)),
			&create_animation_strategy<ExportStageRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportStageRotationOptionsWidget,
									ExportStageRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RELATIVE_STAGE_ROTATION,
					ExportAnimationType::CSV_COMMA),
			ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new ExportStageRotationAnimationStrategy::Configuration(
							"relative_stage_rotation_comma_%0.2fMa.csv",
							ExportStageRotationAnimationStrategy::Configuration::RELATIVE_COMMA,
							default_rotation_export_options,
							default_stage_rotation_export_options)),
			&create_animation_strategy<ExportStageRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportStageRotationOptionsWidget,
									ExportStageRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RELATIVE_STAGE_ROTATION,
					ExportAnimationType::CSV_TAB),
			ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new ExportStageRotationAnimationStrategy::Configuration(
							"relative_stage_rotation_tab_%0.2fMa.csv",
							ExportStageRotationAnimationStrategy::Configuration::RELATIVE_TAB,
							default_rotation_export_options,
							default_stage_rotation_export_options)),
			&create_animation_strategy<ExportStageRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportStageRotationOptionsWidget,
									ExportStageRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export equivalent stage rotations
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::EQUIVALENT_STAGE_ROTATION,
					ExportAnimationType::CSV_SEMICOLON),
			ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new ExportStageRotationAnimationStrategy::Configuration(
							"equivalent_stage_rotation_semicolon_%0.2fMa.csv",
							ExportStageRotationAnimationStrategy::Configuration::EQUIVALENT_SEMICOLON,
							default_rotation_export_options,
							default_stage_rotation_export_options)),
			&create_animation_strategy<ExportStageRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportStageRotationOptionsWidget,
									ExportStageRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::EQUIVALENT_STAGE_ROTATION,
					ExportAnimationType::CSV_COMMA),
			ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new ExportStageRotationAnimationStrategy::Configuration(
							"equivalent_stage_rotation_comma_%0.2fMa.csv",
							ExportStageRotationAnimationStrategy::Configuration::EQUIVALENT_COMMA,
							default_rotation_export_options,
							default_stage_rotation_export_options)),
			&create_animation_strategy<ExportStageRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportStageRotationOptionsWidget,
									ExportStageRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::EQUIVALENT_STAGE_ROTATION,
					ExportAnimationType::CSV_TAB),
			ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new ExportStageRotationAnimationStrategy::Configuration(
							"equivalent_stage_rotation_tab_%0.2fMa.csv",
							ExportStageRotationAnimationStrategy::Configuration::EQUIVALENT_TAB,
							default_rotation_export_options,
							default_stage_rotation_export_options)),
			&create_animation_strategy<ExportStageRotationAnimationStrategy>,
			boost::bind(
					// 'static_cast' is because some compilers have trouble determining
					// which overload of 'create_export_options_widget()' to use...
					static_cast<create_export_options_widget_function_pointer_type>(
							&create_export_options_widget<
									GPlatesQtWidgets::ExportStageRotationOptionsWidget,
									ExportStageRotationAnimationStrategy>),
					_1, _2),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export rasters
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::BMP),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.bmp",
							ExportRasterAnimationStrategy::Configuration::BMP)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::JPG),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.jpg",
							ExportRasterAnimationStrategy::Configuration::JPG)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::JPEG),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.jpeg",
							ExportRasterAnimationStrategy::Configuration::JPEG)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::PNG),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.png",
							ExportRasterAnimationStrategy::Configuration::PNG)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::PPM),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.ppm",
							ExportRasterAnimationStrategy::Configuration::PPM)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::TIFF),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.tiff",
							ExportRasterAnimationStrategy::Configuration::TIFF)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::XBM),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.xbm",
							ExportRasterAnimationStrategy::Configuration::XBM)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::RASTER,
					ExportAnimationType::XPM),
			ExportRasterAnimationStrategy::const_configuration_ptr(
					new ExportRasterAnimationStrategy::Configuration(
							"raster_%0.2fMa.xpm",
							ExportRasterAnimationStrategy::Configuration::XPM)),
			&create_animation_strategy<ExportRasterAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export flowlines
	//

	// By default only export to multiple files (one output file per input file) as this
	// is the most requested output.
	const ExportOptionsUtils::ExportFileOptions default_flowline_file_export_options(
			/*export_to_a_single_file_*/false,
			/*export_to_multiple_files_*/true);
	const bool default_flowline_wrap_to_dateline = false;

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::FLOWLINES,
					ExportAnimationType::GMT),
			ExportFlowlineAnimationStrategy::const_configuration_ptr(
					new ExportFlowlineAnimationStrategy::Configuration(
							"flowline_output_%0.2fMa.xy",
							ExportFlowlineAnimationStrategy::Configuration::GMT,
							default_flowline_file_export_options,
							default_flowline_wrap_to_dateline)),
			&create_animation_strategy<ExportFlowlineAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportFlowlineOptionsWidget,
							ExportFlowlineAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::FLOWLINES,
					ExportAnimationType::SHAPEFILE),
			ExportFlowlineAnimationStrategy::const_configuration_ptr(
					new ExportFlowlineAnimationStrategy::Configuration(
							"flowline_output_%0.2fMa.shp",
							ExportFlowlineAnimationStrategy::Configuration::SHAPEFILE,
							default_flowline_file_export_options,
							default_flowline_wrap_to_dateline)),
			&create_animation_strategy<ExportFlowlineAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportFlowlineOptionsWidget,
							ExportFlowlineAnimationStrategy,
							bool>,
					// The 'true' allows user to turn on/off dateline wrapping of geometries...
					_1, _2, true),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);


	registry.register_exporter(
			ExportAnimationType::get_export_id(
				ExportAnimationType::FLOWLINES,
				ExportAnimationType::OGRGMT),
		ExportFlowlineAnimationStrategy::const_configuration_ptr(
			new ExportFlowlineAnimationStrategy::Configuration(
					"flowline_output_%0.2fMa.gmt",
					ExportFlowlineAnimationStrategy::Configuration::OGRGMT,
					default_flowline_file_export_options,
					default_flowline_wrap_to_dateline)),
		&create_animation_strategy<ExportFlowlineAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportFlowlineOptionsWidget,
							ExportFlowlineAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
		&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export motion paths
	//

	// By default only export to multiple files (one output file per input file) as this
	// is the most requested output.
	const ExportOptionsUtils::ExportFileOptions default_motion_path_file_export_options(
			/*export_to_a_single_file_*/false,
			/*export_to_multiple_files_*/true);
	const bool default_motion_path_wrap_to_dateline = false;

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::MOTION_PATHS,
					ExportAnimationType::GMT),
			ExportMotionPathAnimationStrategy::const_configuration_ptr(
					new ExportMotionPathAnimationStrategy::Configuration(
							"motion_path_output_%0.2fMa.xy",
							ExportMotionPathAnimationStrategy::Configuration::GMT,
							default_motion_path_file_export_options,
							default_motion_path_wrap_to_dateline)),
			&create_animation_strategy<ExportMotionPathAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportMotionPathOptionsWidget,
							ExportMotionPathAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::MOTION_PATHS,
					ExportAnimationType::SHAPEFILE),
			ExportMotionPathAnimationStrategy::const_configuration_ptr(
					new ExportMotionPathAnimationStrategy::Configuration(
							"motion_path_output_%0.2fMa.shp",
							ExportMotionPathAnimationStrategy::Configuration::SHAPEFILE,
							default_motion_path_file_export_options,
							default_motion_path_wrap_to_dateline)),
			&create_animation_strategy<ExportMotionPathAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportMotionPathOptionsWidget,
							ExportMotionPathAnimationStrategy,
							bool>,
					// The 'true' allows user to turn on/off dateline wrapping of geometries...
					_1, _2, true),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);


	registry.register_exporter(
			ExportAnimationType::get_export_id(
				ExportAnimationType::MOTION_PATHS,
				ExportAnimationType::OGRGMT),
			ExportMotionPathAnimationStrategy::const_configuration_ptr(
				new ExportMotionPathAnimationStrategy::Configuration(
					"motion_path_output_%0.2fMa.gmt",
					ExportMotionPathAnimationStrategy::Configuration::OGRGMT,
					default_motion_path_file_export_options,
					default_motion_path_wrap_to_dateline)),
			&create_animation_strategy<ExportMotionPathAnimationStrategy>,
			boost::bind(
					&create_export_options_widget<
							GPlatesQtWidgets::ExportMotionPathOptionsWidget,
							ExportMotionPathAnimationStrategy,
							bool>,
					// The 'false' prevents user from turning on/off dateline wrapping of geometries...
					_1, _2, false),
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P);

	//
	// Export co-registration
	//

	registry.register_exporter(
			ExportAnimationType::get_export_id(
					ExportAnimationType::CO_REGISTRATION,
					ExportAnimationType::CSV_COMMA),
			ExportCoRegistrationAnimationStrategy::const_configuration_ptr(
					new ExportCoRegistrationAnimationStrategy::Configuration(
							"co_registration_data%P_%0.2fMa.csv")),
			&create_animation_strategy<ExportCoRegistrationAnimationStrategy>,
			&create_null_export_options_widget,
			&ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_with_percent_P);
}
