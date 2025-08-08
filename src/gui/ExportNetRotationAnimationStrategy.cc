/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015, 2016 Geological Survey of Norway
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

#include <map>
#include <vector>

#include <QDebug>

#include "app-logic/ApplicationState.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/NetRotationUtils.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTriangulationNetwork.h"
#include "app-logic/VelocityFieldCalculatorLayerProxy.h"

#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReconstructionGeometryExportImpl.h"

#include "gui/CsvExport.h"
#include "gui/ExportAnimationContext.h"

#include "presentation/ViewState.h"

#include "utils/Earth.h"

#include "view-operations/RenderedGeometryUtils.h"

#include "ExportNetRotationAnimationStrategy.h"


namespace
{
	typedef	std::vector<GPlatesGui::CsvExport::LineDataType> csv_data_type;

	/**
	 * Typedef for mapping from @a FeatureHandle to the feature collection file it came from and
	 * the order in which is occurs relative to other features in the feature collections.
	 */
	typedef std::map<
			const GPlatesModel::FeatureHandle *,
			std::pair<const GPlatesFileIO::File::Reference *, unsigned int/*feature order*/> >
					feature_handle_to_collection_map_type;

	// Conversion from steradians (square radians) to square kms.
	const double area_conversion_from_steradians_to_km2 =
			GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS;

	/**
	 * @brief get_older_and_younger_times - on return @time_older and @time_younger will hold
	 * the appropriate times for the velocity calculation at the @current_time.
	 */
	// TODO: unify this with method in KinematicGraphsDialog
	void
	get_older_and_younger_times(
			const GPlatesQtWidgets::VelocityMethodWidget::VelocityMethod &velocity_method,
			const double &delta_time,
			const double &current_time,
			double &time_older,
			double &time_younger,
			GPlatesAppLogic::VelocityDeltaTime::Type &velocity_delta_time_type)
	{
		switch(velocity_method)
		{
		case GPlatesQtWidgets::VelocityMethodWidget::T_TO_T_MINUS_DT:
			time_older = current_time;
			time_younger = current_time - delta_time;
			velocity_delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T;
			break;
		case GPlatesQtWidgets::VelocityMethodWidget::T_PLUS_DT_TO_T:
			time_older = current_time + delta_time;
			time_younger = current_time;
			velocity_delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T;
			break;
		case GPlatesQtWidgets::VelocityMethodWidget::T_PLUS_MINUS_HALF_DT:
			time_older = current_time + delta_time/2.;
			time_younger = current_time - delta_time/2.;
			velocity_delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_MINUS_HALF_DELTA_T;
			break;
		default:
			time_older = current_time;
			time_younger = current_time - delta_time;
			velocity_delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T;
		}
	}


#if 0
	/**
	 * Typedef for sequence of velocity field calculator layer proxies.
	 */
	typedef std::vector<GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::non_null_ptr_type>
	velocity_field_calculator_layer_proxy_seq_type;

	/**
	 * Typedef for a sequence of @a MultiPointVectorField pointers.
	 */
	typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *> vector_field_seq_type;


	/**
	 * @brief get_velocity_field_calculator_layer_proxies - this is used only when net-rotations are calculated using points and
	 * velocities from any existing velocity mesh. Default behaviour of the ExportNetRotationAnimationStrategy is to use a
	 * hard-coded 1-degree lat-lon grid, in which case this function is not used.
	 * @param velocity_field_outputs
	 * @param application_state
	 */
	void
	get_velocity_field_calculator_layer_proxies(
			velocity_field_calculator_layer_proxy_seq_type &velocity_field_outputs,
			const GPlatesAppLogic::ApplicationState &application_state)
	{
		using namespace GPlatesAppLogic;

		const Reconstruction &reconstruction = application_state.get_current_reconstruction();

		// Get the velocity field calculator layer outputs.
		// Note that an active layer does not necessarily mean a visible layer.
		reconstruction.get_active_layer_outputs<VelocityFieldCalculatorLayerProxy>(velocity_field_outputs);
	}


	/**
	 * @brief get_vector_field_seq - this is used only when net-rotations are calculated using points and
	 * velocities from any existing velocity mesh. Default behaviour of the ExportNetRotationAnimationStrategy is to use a
	 * hard-coded 1-degree lat-lon grid, in which case this function is not used.
	 * @param vector_field_seq
	 * @param multi_point_velocity_fields
	 */
	void
	get_vector_field_seq(
			vector_field_seq_type &vector_field_seq,
			const std::vector<GPlatesAppLogic::MultiPointVectorField::non_null_ptr_type> &multi_point_velocity_fields)
	{
		using namespace GPlatesAppLogic;

		// Convert sequence of non_null_ptr_type's to a sequence of raw pointers expected by the caller.
		BOOST_FOREACH(
					const GPlatesAppLogic::MultiPointVectorField::non_null_ptr_type &multi_point_velocity_field,
					multi_point_velocity_fields)
		{
			vector_field_seq.push_back(multi_point_velocity_field.get());
		}
	}

	/**
	 * @brief populate_vector_field_seq - - this is used only when net-rotations are calculated using points and
	 * velocities from any existing velocity mesh. Default behaviour of the ExportNetRotationAnimationStrategy is to use a
	 * hard-coded 1-degree lat-lon grid, in which case this function is not used.
	 * @param vector_field_seq
	 * @param application_state
	 * @param net_rotation_output
	 */
	void
	populate_vector_field_seq(
			vector_field_seq_type &vector_field_seq,
			const GPlatesAppLogic::ApplicationState &application_state,
			GPlatesAppLogic::NetRotationUtils::net_rotation_map_type &net_rotation_output)
	{
		using namespace GPlatesAppLogic;

		// Get the velocity field calculator layer outputs.
		std::vector<VelocityFieldCalculatorLayerProxy::non_null_ptr_type> velocity_field_outputs;

		get_velocity_field_calculator_layer_proxies(velocity_field_outputs, application_state);

		// Iterate over the layers that have velocity field calculator outputs.
		std::vector<GPlatesAppLogic::MultiPointVectorField::non_null_ptr_type> multi_point_velocity_fields;

		BOOST_FOREACH(
					const VelocityFieldCalculatorLayerProxy::non_null_ptr_type &velocity_field_output,
					velocity_field_outputs)
		{
			VelocityParams velocity_params = velocity_field_output->get_current_velocity_params();

			// This is where any smoothing option overrides might go.

			velocity_field_output->get_velocity_multi_point_vector_fields(
						multi_point_velocity_fields,
						velocity_params,
						net_rotation_output);
		}

		// Convert sequence of non_null_ptr_type's to a sequence of raw pointers expected by the caller.
		get_vector_field_seq(vector_field_seq, multi_point_velocity_fields);

	}
#endif

	void
	write_file_collection_to_csv_data(
			csv_data_type &csv_data,
			const GPlatesGui::ExportNetRotationAnimationStrategy::file_collection_type &files,
			const QString &description)
	{
		GPlatesGui::CsvExport::LineDataType data_line;

		data_line.clear();
		data_line.push_back(description);
		csv_data.push_back(data_line);
		data_line.clear();
		BOOST_FOREACH(const GPlatesFileIO::File::Reference *ref,files)
		{
			data_line.push_back(ref->get_file_info().get_display_name(false /*use-absolute-path = false */));
		}
		csv_data.push_back(data_line);
	}

	void
	write_reconstruction_info_to_csv_data(
			csv_data_type &csv_data,
			const GPlatesModel::integer_plate_id_type &anchor_plate,
			const GPlatesGui::ExportNetRotationAnimationStrategy::file_collection_type &referenced_files,
			const GPlatesGui::ExportNetRotationAnimationStrategy::file_collection_type &reconstruction_files)
	{
		GPlatesGui::CsvExport::LineDataType data_line;
		data_line.clear();
		data_line.push_back(QObject::tr("Anchor plate: ") + QString::number(anchor_plate));
		csv_data.push_back(data_line);

		write_file_collection_to_csv_data(csv_data,referenced_files,QObject::tr("Referenced files"));
		write_file_collection_to_csv_data(csv_data,reconstruction_files,QObject::tr("Reconstruction files"));
	}

	void
	write_header_to_csv_data(
			csv_data_type &csv_data,
			const double &time,
			const GPlatesModel::integer_plate_id_type &anchor_plate,
			const GPlatesGui::ExportNetRotationAnimationStrategy::file_collection_type &referenced_files,
			const GPlatesGui::ExportNetRotationAnimationStrategy::file_collection_type &reconstruction_files)
	{
		GPlatesGui::CsvExport::LineDataType data_line;

		data_line.push_back(QObject::tr("Time: ") + QString::number(time) + QObject::tr(" Ma"));
		csv_data.push_back(data_line);

		write_reconstruction_info_to_csv_data(csv_data,anchor_plate,referenced_files,reconstruction_files);

		data_line.clear();
		data_line.push_back(QObject::tr("PlateId"));
		data_line.push_back(QObject::tr("Lat (\302\260)")); // \302\260 is UTF8 for degree sign
		data_line.push_back(QObject::tr("Lon (\302\260)"));
		data_line.push_back(QObject::tr("Angular velocity (\302\260/Ma)"));
		data_line.push_back(QObject::tr("Area (km2)"));
		csv_data.push_back(data_line);
	}

	void
	write_net_rotation_to_csv_data(
			csv_data_type &csv_data,
			const GPlatesGui::ExportNetRotationAnimationStrategy::pole_type &net_rotation)
	{
		GPlatesGui::CsvExport::LineDataType data_line;
		data_line.clear();
		data_line.push_back(QObject::tr("Net rotation:"));
		csv_data.push_back(data_line);
		data_line.clear();
		data_line.push_back(QObject::tr("Lat (\302\260)")); // \302\260 is UTF8 for degree sign
		data_line.push_back(QObject::tr("Lon (\302\260)"));
		data_line.push_back(QObject::tr("Angular velocity (\302\260/Ma)"));
		csv_data.push_back(data_line);
		data_line.clear();
		data_line.push_back(QString::number(net_rotation.first.latitude()));
		data_line.push_back(QString::number(net_rotation.first.longitude()));
		data_line.push_back(QString::number(net_rotation.second));
		csv_data.push_back(data_line);

	}
}


GPlatesGui::ExportNetRotationAnimationStrategy::ExportNetRotationAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());

	// This code is copied from "gui/ExportReconstructedGeometryAnimationStrategy.cc".
	GPlatesAppLogic::FeatureCollectionFileState &file_state =
			d_export_animation_context_ptr->view_state().get_application_state()
			.get_feature_collection_file_state();

	// From the file state, obtain the list of all currently loaded files.
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();

	// Add them to our list of loaded files.
	BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref, loaded_files)
	{
		d_loaded_files.push_back(&file_ref.get_file());
	}


	const GPlatesAppLogic::ReconstructGraph &reconstruct_graph =
			d_export_animation_context_ptr->view_state().get_application_state().get_reconstruct_graph();

	// Check all the active reconstruction layers, and get their input files.
	GPlatesAppLogic::ReconstructGraph::const_iterator it = reconstruct_graph.begin(),
													end = reconstruct_graph.end();
	for (; it != end ; ++it)
	{
		if ((it->get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCTION) && it->is_active())
		{

			// The 'reconstruct geometries' layer has input feature collections on its main input channel.
			const GPlatesAppLogic::LayerInputChannelName::Type main_input_channel =
					it->get_main_input_feature_collection_channel();
			const std::vector<GPlatesAppLogic::Layer::InputConnection> main_inputs =
					it->get_channel_inputs(main_input_channel);

			// Loop over all input connections to get the files (feature collections) for the current target layer.
			BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& main_input_connection, main_inputs)
			{
				boost::optional<GPlatesAppLogic::Layer::InputFile> input_file =
						main_input_connection.get_input_file();
				// If it's not a file (ie, it's a layer) then continue to the next file.
				if(!input_file)
				{
					continue;
				}
				d_loaded_reconstruction_files.push_back(&(input_file->get_file().get_file()));
			}


		}
	}
	d_referenced_files_set.clear();
}


bool
GPlatesGui::ExportNetRotationAnimationStrategy::export_iteration(
		std::size_t frame_index)
{
	GPlatesAppLogic::ApplicationState &application_state =
		d_export_animation_context_ptr->view_state().get_application_state();

	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it =
			*d_filename_iterator_opt;

	// Figure out a filename from the template filename sequence.
	QString basename = *filename_it;
	// Add the target dir to that to figure out the absolute path + name.
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	// Write status message.
	d_export_animation_context_ptr->update_status_message(
				QObject::tr("Writing net rotations at frame %2 to file \"%1\"...")
				.arg(basename)
				.arg(frame_index) );

	csv_data_type data;
	GPlatesGui::CsvExport::LineDataType data_line;
	CsvExport::ExportOptions option;
	Q_UNUSED(option);
	switch(d_configuration->d_csv_export_type)
	{
	case Configuration::CSV_COMMA:
		option.delimiter = ',';
		break;
	case Configuration::CSV_TAB:
		option.delimiter = '\t';
		break;
	case Configuration::CSV_SEMICOLON:
		option.delimiter = ';';
		break;
	}
	double time = d_export_animation_context_ptr->view_time();
	d_anchor_plate_id = application_state.get_current_reconstruction().get_anchor_plate_id();

	double t_older;
	double t_younger;
	GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type;

	// Check the time settings required by the user through the configuration widget.
	get_older_and_younger_times(
				d_configuration->d_options.velocity_method,
				d_configuration->d_options.delta_time,
				time,
				t_older,
				t_younger,
				velocity_delta_time_type);
	const double velocity_delta_time = t_older - t_younger;

	// Skip times if if we get beyond the present day.
	if (t_younger < 0.)
	{
		d_export_animation_context_ptr->update_status_message(
					QObject::tr("Skipping net rotation file \"%1\": uses calculation time (\"%2\" Ma) younger than present day.").arg(full_filename).arg(t_younger));
		return true;
	}

	try
	{
		using namespace GPlatesViewOperations;

		RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
		RenderedGeometryUtils::get_unique_reconstruction_geometries(
					reconstruction_geom_seq,
					d_export_animation_context_ptr->view_state().get_rendered_geometry_collection(),
					// Don't want to export a duplicate reconstructed geometry if one is currently in focus...
					GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

		// Get any ReconstructionGeometry objects that are of type ResolvedTopologicalBoundary.
		std::vector<const GPlatesAppLogic::ResolvedTopologicalBoundary *> resolved_topological_boundary_seq;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					reconstruction_geom_seq.begin(),
					reconstruction_geom_seq.end(),
					resolved_topological_boundary_seq);

		// Get any ReconstructionGeometry objects that are of type ResolvedTopologicalNetwork.
		std::vector<const GPlatesAppLogic::ResolvedTopologicalNetwork *> resolved_topological_network_seq;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					reconstruction_geom_seq.begin(),
					reconstruction_geom_seq.end(),
					resolved_topological_network_seq);

		// Attempt to find files associated with our topological geometries and networks.
		feature_handle_to_collection_map_type feature_to_collection_map;
		std::vector<const GPlatesFileIO::File::Reference *> referenced_files;
		GPlatesFileIO::ReconstructionGeometryExportImpl::populate_feature_handle_to_collection_map(
				feature_to_collection_map,
				d_loaded_files);
		GPlatesFileIO::ReconstructionGeometryExportImpl::get_unique_list_of_referenced_files(
				referenced_files,
				resolved_topological_boundary_seq,
				feature_to_collection_map);
		GPlatesFileIO::ReconstructionGeometryExportImpl::get_unique_list_of_referenced_files(
				referenced_files,
				resolved_topological_network_seq,
				feature_to_collection_map);

		for (const GPlatesFileIO::File::Reference *ref : referenced_files)
		{
			qDebug() << ref->get_file_info().get_display_name(false /*use-absolute-path = false */);
			d_referenced_files_set.insert(ref);
		}


		write_header_to_csv_data(data,time,d_anchor_plate_id,referenced_files,d_loaded_reconstruction_files);

		std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type> resolved_topological_boundaries;
		for (auto resolved_topological_boundary : resolved_topological_boundary_seq)
		{
			resolved_topological_boundaries.push_back(resolved_topological_boundary);
		}

		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type> resolved_topological_networks;
		for (auto resolved_topological_network : resolved_topological_network_seq)
		{
			resolved_topological_networks.push_back(resolved_topological_network);
		}

		// Calculate the net rotation.
		const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator net_rotation(
				resolved_topological_boundaries,
				resolved_topological_networks,
				time,
				velocity_delta_time,
				velocity_delta_time_type,
				d_anchor_plate_id,
				180/*num_samples_along_meridian*/);  // use 180 x 360 uniform lat-lon samples

		// Go through the rotations plate-by-plate.
		for (const auto &net_rotation_plate_id_and_contribution : net_rotation.get_plate_id_net_rotation_map())
		{
			boost::optional<GPlatesModel::integer_plate_id_type> plate_id = net_rotation_plate_id_and_contribution.first;
			const GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator &net_rotation_contribution = net_rotation_plate_id_and_contribution.second;

			// Networks are no longer required to have a plate ID because it doesn't make sense
			// (network is deforming, not rigidly rotated by plate ID), in which case we'll use plate ID zero.
			//
			// TODO: We need to fix all this because currently all/most networks will get grouped under plate ID zero.
			if (!plate_id)
			{
				plate_id = 0;
			}

			boost::optional<std::pair<GPlatesMaths::LatLonPoint, double>> net_rotation_pole =
					net_rotation_contribution.get_net_rotation_lat_lon_pole_and_angle();
			if (!net_rotation_pole)
			{
				// There have not been any non-zero net rotation contributions.
				qDebug() << "Zero net rotation for plate " << plate_id.get() << ". Skipping plate.";
				continue;
			}

			// Force positive angle
			if (net_rotation_pole->second < 0)
			{
				net_rotation_pole->second = std::abs(net_rotation_pole->second);
				double lat = net_rotation_pole->first.latitude();
				double lon = net_rotation_pole->first.longitude();

				lat *= -1.;
				lon += 180.;
				lon = (lon > 360.)? (lon - 360.) : lon;
				net_rotation_pole->first = GPlatesMaths::LatLonPoint(lat,lon);
			}

			data_line.clear();

			data_line.push_back(QString::number(plate_id.get()));
			data_line.push_back(QString::number(net_rotation_pole->first.latitude()));
			data_line.push_back(QString::number(net_rotation_pole->first.longitude()));
			data_line.push_back(QString::number(net_rotation_pole->second));

			// Get the plate area.
			data_line.push_back(QString::number(net_rotation_contribution.get_area_in_steradians() * area_conversion_from_steradians_to_km2));

			data.push_back(data_line);
		}

		// Finally, export the net rotation
		data_line.clear();

		boost::optional<std::pair<GPlatesMaths::LatLonPoint, double>> total_net_rotation_pole =
				net_rotation.get_total_net_rotation().get_net_rotation_lat_lon_pole_and_angle();
		if (total_net_rotation_pole)
		{
			// Force positive angle
			if (total_net_rotation_pole->second < 0)
			{
				total_net_rotation_pole->second = std::abs(total_net_rotation_pole->second);
				double lat = total_net_rotation_pole->first.latitude();
				double lon = total_net_rotation_pole->first.longitude();

				lat *= -1.;
				lon += 180.;
				lon = (lon > 360.)? (lon - 360.) : lon;
				total_net_rotation_pole->first = GPlatesMaths::LatLonPoint(lat,lon);
			}
			data_line.clear();
			data.push_back(data_line);

			write_net_rotation_to_csv_data(data, total_net_rotation_pole.get());

			d_total_poles.push_back(std::make_pair(time, total_net_rotation_pole.get()));
		}

		CsvExport::export_data(
					QDir(d_export_animation_context_ptr->target_dir()).absoluteFilePath(
						*filename_it),
					option,
					data);
		filename_it++;

	}
	catch (std::exception &exc)
	{
		d_export_animation_context_ptr->update_status_message(
					QObject::tr("Error writing net rotation file \"%1\": %2")
					.arg(full_filename)
					.arg(exc.what()));
		return false;
	}
	catch (...)
	{
		d_export_animation_context_ptr->update_status_message(
					QObject::tr("Error writing net rotation file \"%1\": unknown error!").arg(full_filename));
		return false;
	}

	return true;
}

void
GPlatesGui::ExportNetRotationAnimationStrategy::set_template_filename(
		const QString &filename)
{
	ExportAnimationStrategy::set_template_filename(filename);
}


bool
GPlatesGui::ExportNetRotationAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	return export_iteration(frame_index);
}


void
GPlatesGui::ExportNetRotationAnimationStrategy::wrap_up(
		bool export_successful)
{
	// If we need to do anything after writing a whole batch of velocity files,
	// here's the place to do it.
	// Of course, there's also the destructor, which should free up any resources
	// we acquired in the constructor; this method is intended for any "last step"
	// iteration operations that might need to occur. Perhaps all iterations end
	// up in the same file and we should close that file (if all steps completed
	// successfully).

	// Export the total net-rotations for each time step to a single file.
	QString filename = d_export_animation_context_ptr->target_dir().absolutePath();
	filename.append(QDir::separator());
	filename.append("total-net-rotations.csv");

	std::vector<GPlatesGui::CsvExport::LineDataType> data;
	GPlatesGui::CsvExport::LineDataType data_line;
	CsvExport::ExportOptions option;
	switch(d_configuration->d_csv_export_type)
	{
	case Configuration::CSV_COMMA:
		option.delimiter = ',';
		break;
	case Configuration::CSV_TAB:
		option.delimiter = '\t';
		break;
	case Configuration::CSV_SEMICOLON:
		option.delimiter = ';';
		break;
	}

	file_collection_type referenced_files(d_referenced_files_set.begin(), d_referenced_files_set.end());

	// Write anchor plate, recon files.
	write_reconstruction_info_to_csv_data(data,d_anchor_plate_id,referenced_files,d_loaded_reconstruction_files);

	data_line.clear();
	data_line.push_back(QObject::tr("Time (Ma)"));
	data_line.push_back(QObject::tr("Lat (\302\260)")); // \302\260 is UTF8 for degree sign
	data_line.push_back(QObject::tr("Lon (\302\260)"));
	data_line.push_back(QObject::tr("Angular velocity (\302\260/Ma)"));
	data.push_back(data_line);


	BOOST_FOREACH(time_pole_pair_type time_pole_pair,d_total_poles)
	{
		data_line.clear();
		data_line.push_back(QString::number(time_pole_pair.first));	// time
		data_line.push_back(QString::number(time_pole_pair.second.first.latitude())); // pole lat
		data_line.push_back(QString::number(time_pole_pair.second.first.longitude())); // pole lon
		data_line.push_back(QString::number(time_pole_pair.second.second)); // omega
		data.push_back(data_line);
	}

	CsvExport::export_data(
				filename,
				option,
				data);
}
