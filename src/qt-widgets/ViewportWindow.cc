/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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
 
#include <iostream>
#include <iterator>
#include <memory>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

#include <QFileDialog>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QMessageBox>
#include <QColorDialog>
#include <QColor>
#include <QInputDialog>
#include <QProgressBar>
#include <QDockWidget>

#include "ViewportWindow.h"
#include "InformationDialog.h"
#include "ShapefilePropertyMapper.h"
#include "TaskPanel.h"
#include "ActionButtonBox.h"
#include "CreateFeatureDialog.h"

#include "global/GPlatesException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "gui/CanvasToolAdapter.h"
#include "gui/CanvasToolChoice.h"
#include "gui/ChooseCanvasTool.h"
#include "gui/FeatureWeakRefSequence.h"
#include "gui/SvgExport.h"
#include "gui/PlatesColourTable.h"
#include "gui/SingleColourTable.h"
#include "gui/FeatureColourTable.h"
#include "gui/AgeColourTable.h"
#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/InvalidLatLonException.h"
#include "maths/Real.h"
#include "model/Model.h"
#include "model/types.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/DummyTransactionHandle.h"
#include "file-io/FeatureWriter.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/PlatesLineFormatReader.h"
#include "file-io/PlatesRotationFormatReader.h"
#include "file-io/FileInfo.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/RasterReader.h"
#include "file-io/ShapeFileReader.h"
#include "file-io/GpmlOnePointSixReader.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "view-operations/UndoRedo.h"
#include "feature-visitors/FeatureCollectionClassifier.h"


void
GPlatesQtWidgets::ViewportWindow::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	if ( ! GPlatesFileIO::is_writable(file_info)) {
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				file_info.get_qfileinfo().filePath());
	}

	if ( ! file_info.get_feature_collection()) {
		throw GPlatesGlobal::UnexpectedEmptyFeatureCollectionException(
				"Attempted to write an empty feature collection.");
	}

	boost::shared_ptr<GPlatesFileIO::FeatureWriter> writer =
		GPlatesFileIO::get_feature_collection_writer(
		file_info,
		feature_collection_write_format);

	GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
		*file_info.get_feature_collection();

	if (feature_collection.is_valid())
	{
		GPlatesModel::FeatureCollectionHandle::features_const_iterator
			iter = feature_collection->features_begin(), 
			end = feature_collection->features_end();

		for ( ; iter != end; ++iter)
		{
			const GPlatesModel::FeatureHandle& feature_handle = **iter;
			writer->write_feature(feature_handle);
		}
	}
}


void
GPlatesQtWidgets::ViewportWindow::save_file_as(
		const GPlatesFileIO::FileInfo &file_info,
		file_info_iterator features_to_save,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	GPlatesFileIO::FileInfo file_copy = save_file_copy(
		file_info,
		features_to_save,
		feature_collection_write_format);

	// Update iterator
	*features_to_save = file_copy;
}


GPlatesFileIO::FileInfo
GPlatesQtWidgets::ViewportWindow::save_file_copy(
		const GPlatesFileIO::FileInfo &file_info,
		file_info_iterator features_to_save,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	GPlatesFileIO::FileInfo file_copy(file_info.get_qfileinfo().filePath());
	if ( ! features_to_save->get_feature_collection()) {
		throw GPlatesGlobal::UnexpectedEmptyFeatureCollectionException(
				"Attempted to write an empty feature collection.");
	}
	file_copy.set_feature_collection(*(features_to_save->get_feature_collection()));
	save_file(file_copy, feature_collection_write_format);
	return file_copy;
}

void
GPlatesQtWidgets::ViewportWindow::load_files(
		const QStringList &file_names)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	QStringList::const_iterator iter = file_names.begin();
	QStringList::const_iterator end = file_names.end();

	bool have_loaded_new_rotation_file = false;

	for ( ; iter != end; ++iter) {

		GPlatesFileIO::FileInfo file(*iter);

		try
		{
			// Read the feature collection from file.
			GPlatesFileIO::read_feature_collection_file(file, *d_model_ptr, read_errors);

			switch ( GPlatesFileIO::get_feature_collection_file_format(file) )
			{
			case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

					// GPML format files can contain both reconstructable features and
					// reconstruction trees. This visitor lets us find out which.
					if (file.get_feature_collection()) {
						GPlatesFeatureVisitors::FeatureCollectionClassifier classifier;
						classifier.scan_feature_collection(
								GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
										*file.get_feature_collection())
								);
						// Check if the file contains reconstructable features.
						if (classifier.reconstructable_feature_count() > 0) {
							d_active_reconstructable_files.push_back(new_file);
						}
						// Check if the file contains reconstruction features.
						if (classifier.reconstruction_feature_count() > 0) {
							// We only want to make the first rotation file active.
							if ( ! have_loaded_new_rotation_file) 
							{
								d_active_reconstruction_files.clear();
								d_active_reconstruction_files.push_back(new_file);
								have_loaded_new_rotation_file = true;
							}
						}
					}
				}
				break;

			case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
				if (file.get_feature_collection())
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

					// Line format files are made active by default.
					d_active_reconstructable_files.push_back(new_file);
				}
				break;

			case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
				if (file.get_feature_collection())
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);				

					// We only want to make the first rotation file active.
					if ( ! have_loaded_new_rotation_file) 
					{
						d_active_reconstruction_files.clear();
						d_active_reconstruction_files.push_back(new_file);
						have_loaded_new_rotation_file = true;
					}
				}
				break;

			case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
				GPlatesFileIO::ShapeFileReader::set_property_mapper(
					boost::shared_ptr< ShapefilePropertyMapper >(new ShapefilePropertyMapper));
				GPlatesFileIO::ShapeFileReader::read_file(file,*d_model_ptr,read_errors);

				if (file.get_feature_collection())
				{
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);
					d_active_reconstructable_files.push_back(new_file);
				}
				break;

			default:
				break;
			}
		}
		catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
		{
			// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
			// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
			// QFileDialog support.
			// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
			boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
					new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
					new GPlatesFileIO::LineNumberInFile(0));
			read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
					GPlatesFileIO::ReadErrors::FileNotLoaded));
		}
		catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &e)
		{
			QString message = tr("GPlates was unable to use the '%1' program to read the file '%2'."
					" Please check that gzip is installed and in your PATH. You will still be able to open"
					" files which are not compressed.")
					.arg(e.command())
					.arg(e.filename());
			QMessageBox::critical(this, tr("Error Opening File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (GPlatesFileIO::FileFormatNotSupportedException &)
		{
			QString message = tr("Error: Loading files in this format is currently not supported.");
			QMessageBox::critical(this, tr("Error Opening File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (GPlatesGlobal::Exception &e)
		{
			std::cerr << "Caught exception: " << e << std::endl;
		}


	}

	// Internal state changed, make sure dialogs are up to date.
	d_read_errors_dialog.update();
	d_manage_feature_collections_dialog.update();
	d_shapefile_attribute_viewer_dialog.update();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}
}


void
GPlatesQtWidgets::ViewportWindow::reload_file(
		file_info_iterator file_it)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	// Now load the files in a similar way to 'load_files' above, but in this case
	// we don't need to worry about adding/removing from ApplicationState, or the
	// d_active_reconstructable_files and d_active_reconstruction_files lists.
	// The file should already belong to them.
	
	try
	{
		// FIXME: In fact, we are sharing plenty of exception-handling code with load_files as
		// well, though this might also change after the merge. A possible area for refactoring
		// if someone is bored?

		switch ( GPlatesFileIO::get_feature_collection_file_format(*file_it) )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			GPlatesFileIO::ShapeFileReader::set_property_mapper(
				boost::shared_ptr< ShapefilePropertyMapper >(new ShapefilePropertyMapper));
			break;

		default:
			break;
		}

		// Read the feature collection from file.
		GPlatesFileIO::read_feature_collection_file(*file_it, *d_model_ptr, read_errors);

	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
				new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumberInFile(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
	catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to read the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to open"
				" files which are not compressed.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesGlobal::Exception &e)
	{
		std::cerr << "Caught exception: " << e << std::endl;
	}

	
	// Internal state changed, make sure dialogs are up to date.
	d_read_errors_dialog.update();
	// We should be able to get by with just updating the MFCD's state buttons,
	// not rebuild the whole table. This avoids an ugly table redraw.
	d_manage_feature_collections_dialog.update_state();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}

	// Data may have changed, update the display.
	reconstruct();
}


GPlatesAppState::ApplicationState::file_info_iterator
GPlatesQtWidgets::ViewportWindow::create_empty_reconstructable_file()
{
	// Create an empty "file" - does not correspond to anything on disk yet.
	GPlatesFileIO::FileInfo file;
	file.set_feature_collection(d_model_ptr->create_feature_collection());

	GPlatesAppState::ApplicationState::file_info_iterator new_file =
	GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

	// Given this method's name, we are promised this new FeatureCollection will
	// be used for reconstructable data.
	d_active_reconstructable_files.push_back(new_file);

	// Internal state changed, make sure dialogs are up to date.
	d_manage_feature_collections_dialog.update();
	
	return new_file;
}


namespace
{
	void
	get_features_collection_from_file_info_collection(
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_files,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &features_collection) {

		GPlatesQtWidgets::ViewportWindow::active_files_collection_type::iterator
			iter = active_files.begin(),
			end = active_files.end();
		for ( ; iter != end; ++iter)
		{
			if ((*iter)->get_feature_collection())
			{
				features_collection.push_back(*((*iter)->get_feature_collection()));
			}
		}
	}

	GPlatesModel::Reconstruction::non_null_ptr_type 
	create_reconstruction(
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstructable_files,
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstruction_files,
			GPlatesModel::ModelInterface *model_ptr,
			double recon_time,
			GPlatesModel::integer_plate_id_type recon_root) {

		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
			reconstructable_features_collection,
			reconstruction_features_collection;

		get_features_collection_from_file_info_collection(
				active_reconstructable_files,
				reconstructable_features_collection);

		get_features_collection_from_file_info_collection(
				active_reconstruction_files,
				reconstruction_features_collection);

		return model_ptr->create_reconstruction(reconstructable_features_collection,
				reconstruction_features_collection, recon_time, recon_root);
	}


	void
	render_model(
			GPlatesModel::ModelInterface *model_ptr, 
			GPlatesModel::Reconstruction::non_null_ptr_type &reconstruction,
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstructable_files,
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstruction_files,
			double recon_time,
			GPlatesModel::integer_plate_id_type recon_root,
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
			GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
			GPlatesGui::ColourTable &colour_table)
	{
		// Delay any notification of changes to the rendered geometry collection
		// until end of current scope block. This is so we can do multiple changes
		// without redrawing canvas after each change.
		// This should ideally be located at the highest level to capture one
		// user GUI interaction - the user performs an action and we update canvas once.
		// But since these guards can be nested it's probably a good idea to have it here too.
		GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

		// Get the reconstruction rendered layer.
		GPlatesViewOperations::RenderedGeometryLayer *reconstruction_layer =
			rendered_geom_collection.get_main_rendered_layer(
					GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

		// Activate the layer.
		reconstruction_layer->set_active();

		// Clear all RenderedGeometry's before adding new ones.
		reconstruction_layer->clear_rendered_geometries();

		try {
			reconstruction = create_reconstruction(active_reconstructable_files, 
					active_reconstruction_files, model_ptr, recon_time, recon_root);

			GPlatesModel::Reconstruction::geometry_collection_type::iterator iter =
					reconstruction->geometries().begin();
			GPlatesModel::Reconstruction::geometry_collection_type::iterator end =
					reconstruction->geometries().end();
			for ( ; iter != end; ++iter) {
				GPlatesGui::ColourTable::const_iterator colour = colour_table.end();

				// We use a dynamic cast here (despite the fact that dynamic casts
				// are generally considered bad form) because we only care about
				// one specific derivation.  There's no "if ... else if ..." chain,
				// so I think it's not super-bad form.  (The "if ... else if ..."
				// chain would imply that we should be using polymorphism --
				// specifically, the double-dispatch of the Visitor pattern --
				// rather than updating the "if ... else if ..." chain each time a
				// new derivation is added.)
				GPlatesModel::ReconstructedFeatureGeometry *rfg =
						dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(iter->get());
				if (rfg) {
					// It's an RFG, so let's look at the feature it's
					// referencing.
					if (rfg->reconstruction_plate_id()) {
						colour = colour_table.lookup(*rfg);
					}
				}

				if (colour == colour_table.end()) {
					// Anything not in the table uses the 'Olive' colour.
					colour = &GPlatesGui::Colour::OLIVE;
				}

				// Create a RenderedGeometry using the reconstructed geometry.
				GPlatesViewOperations::RenderedGeometry rendered_geom =
					rendered_geom_factory.create_rendered_geometry_on_sphere(
							(*iter)->geometry(), *colour);

				// Add to the reconstruction rendered layer.
				// Updates to the canvas will be taken care of since canvas listens
				// to the update signal of RenderedGeometryCollection which in turn
				// listens to its rendered layers.
				reconstruction_layer->add_rendered_geometry(rendered_geom);
			}

			//render(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), &GPlatesQtWidgets::GlobeCanvas::draw_point, canvas_ptr);
			//for_each(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), render(canvas_ptr, &GlobeCanvas::draw_point, point_colour))
			// for_each(reconstruction->polyline_geometries().begin(), reconstruction->polyline_geometries().end(), polyline_point);
		} catch (GPlatesGlobal::Exception &e) {
			std::cerr << e << std::endl;
		}
	}

} // namespace


GPlatesGui::ColourTable *
GPlatesQtWidgets::ViewportWindow::get_colour_table()
{
	GPlatesGui::ColourTable *value;
	if (d_colour_table_ptr == NULL) {
		value = GPlatesGui::PlatesColourTable::Instance();
	} else {
		value = d_colour_table_ptr;
	}
	return value;
}

GPlatesQtWidgets::ViewportWindow::ViewportWindow() :
	d_model_ptr(new GPlatesModel::Model()),
	d_reconstruction_ptr(d_model_ptr->create_empty_reconstruction(0.0, 0)),
	d_recon_time(0.0),
	d_recon_root(0),
	d_reconstruction_view_widget(d_rendered_geom_collection, *this, this),
	d_feature_focus(),
	d_about_dialog(*this, this),
	d_animate_dialog(*this, this),
	d_total_reconstruction_poles_dialog(*this, this),
	d_feature_properties_dialog(*this, d_feature_focus, this),
	d_license_dialog(&d_about_dialog),
	d_manage_feature_collections_dialog(*this, this),
	d_read_errors_dialog(this),
	d_set_camera_viewpoint_dialog(*this, this),
	d_set_raster_surface_extent_dialog(*this, this),
	d_specify_fixed_plate_dialog(d_recon_root, this),
	d_animate_dialog_has_been_shown(false),
	d_geom_builder_tool_target(
			d_digitise_geometry_builder,
			d_focused_feature_geometry_builder,
			d_rendered_geom_collection,
			d_feature_focus),
	d_focused_feature_geom_manipulator(
			d_focused_feature_geometry_builder,
			d_feature_focus,
			*this),
	d_task_panel_ptr(NULL),
	d_shapefile_attribute_viewer_dialog(*this,this),
	d_feature_table_model_ptr(new GPlatesGui::FeatureTableModel(d_feature_focus)),
	d_segments_feature_table_model_ptr(new GPlatesGui::FeatureTableModel(d_feature_focus)),
	d_open_file_path(""),
	d_colour_table_ptr(NULL)
{
	setupUi(this);

	d_choose_canvas_tool.reset(new GPlatesGui::ChooseCanvasTool(*this));

	d_canvas_ptr = &(d_reconstruction_view_widget.globe_canvas());

	std::auto_ptr<TaskPanel> task_panel_auto_ptr(new TaskPanel(
			d_feature_focus,
			*d_model_ptr,
			d_rendered_geom_collection,
			d_canvas_ptr->get_rendered_geometry_factory(),
			d_digitise_geometry_builder,
			d_geom_builder_tool_target,
			*this,
			*d_choose_canvas_tool,
			this));
	d_task_panel_ptr = task_panel_auto_ptr.get();

	// Connect all the Signal/Slot relationships of ViewportWindow's
	// toolbar buttons and menu items.
	connect_menu_actions();
	
	// Set up an emergency context menu to control QDockWidgets even if
	// they're no longer behaving properly.
	set_up_dock_context_menus();
	
	// FIXME: Set up the Task Panel in a more detailed fashion here.
#if 1
	d_reconstruction_view_widget.insert_task_panel(task_panel_auto_ptr);
#else
	// Stretchable Task Panel hack for testing: Make the Task Panel
	// into a QDockWidget, undocked by default.
	QDockWidget *task_panel_dock = new QDockWidget(tr("Task Panel"), this);
	task_panel_dock->setWidget(task_panel_auto_ptr.release());
	task_panel_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, task_panel_dock);
	task_panel_dock->setFloating(true);
#endif
	set_up_task_panel_actions();
	
	// Disable the feature-specific Actions as there is no currently focused feature to act on.
	enable_or_disable_feature_actions(d_feature_focus.focused_feature());
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this, SLOT(enable_or_disable_feature_actions(GPlatesModel::FeatureHandle::weak_ref)));

	// Set up the Specify Fixed Plate dialog.
	QObject::connect(&d_specify_fixed_plate_dialog, SIGNAL(value_changed(unsigned long)),
			this, SLOT(reconstruct_with_root(unsigned long)));

	// Set up the Animate dialog.
	QObject::connect(&d_animate_dialog, SIGNAL(current_time_changed(double)),
			&d_reconstruction_view_widget, SLOT(set_reconstruction_time(double)));

	// Set up the Reconstruction View widget.
	setCentralWidget(&d_reconstruction_view_widget);

	QObject::connect(&d_reconstruction_view_widget, SIGNAL(reconstruction_time_changed(double)),
			this, SLOT(reconstruct_to_time(double)));
	QObject::connect(d_canvas_ptr, SIGNAL(mouse_pointer_position_changed(const GPlatesMaths::PointOnSphere &, bool)),
			&d_reconstruction_view_widget, SLOT(update_mouse_pointer_position(const GPlatesMaths::PointOnSphere &, bool)));

	// Connect the geometry-focus highlight to the feature focus.
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			&(d_canvas_ptr->geometry_focus_highlight()), SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	QObject::connect(&d_feature_focus, SIGNAL(focused_feature_modified(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			&(d_canvas_ptr->geometry_focus_highlight()), SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	// Connect the reconstruction pole widget to the feature focus.
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			&(d_task_panel_ptr->reconstruction_pole_widget()), SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	// The Reconstruction Pole widget needs to know when the reconstruction time changes.
	QObject::connect(this, SIGNAL(reconstruction_time_changed(
					double)),
			&(d_task_panel_ptr->reconstruction_pole_widget()), SLOT(handle_reconstruction_time_change(
					double)));

	// Connect the reconstruction pole widget to the feature focus.
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			&(d_task_panel_ptr->create_topology_widget()), SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	// Setup RenderedGeometryCollection.
	initialise_rendered_geom_collection();

	// Render everything on the screen in present-day positions.
	render_model(d_model_ptr, d_reconstruction_ptr, d_active_reconstructable_files, 
			d_active_reconstruction_files, 0.0, d_recon_root,
			d_rendered_geom_collection, get_rendered_geometry_factory(), *get_colour_table());

	// Set up the Clicked table.
	// FIXME: feature table model for this Qt widget and the Query Tool should be stored in ViewState.
	table_view_clicked_geometries->setModel(d_feature_table_model_ptr.get());
	table_view_clicked_geometries->verticalHeader()->hide();
	table_view_clicked_geometries->resizeColumnsToContents();
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*table_view_clicked_geometries->horizontalHeader());
	table_view_clicked_geometries->horizontalHeader()->setMinimumSectionSize(60);
	table_view_clicked_geometries->horizontalHeader()->setMovable(true);
	table_view_clicked_geometries->horizontalHeader()->setHighlightSections(false);
	// When the user selects a row of the table, we should focus that feature.
	QObject::connect(table_view_clicked_geometries->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			d_feature_table_model_ptr.get(),
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));


	// Set up the Platepolygon Segments table.
	// FIXME: feature table model for this Qt widget and the Query Tool should be stored in ViewState.
	table_view_platepolygon_segments->setModel(d_segments_feature_table_model_ptr.get());
	table_view_platepolygon_segments->verticalHeader()->hide();
	table_view_platepolygon_segments->resizeColumnsToContents();
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*table_view_platepolygon_segments->horizontalHeader());
	table_view_platepolygon_segments->horizontalHeader()->setMinimumSectionSize(60);
	table_view_platepolygon_segments->horizontalHeader()->setMovable(true);
	table_view_platepolygon_segments->horizontalHeader()->setHighlightSections(false);

	// When the user selects a row of the table, we should focus that feature.
	QObject::connect(table_view_platepolygon_segments->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			d_segments_feature_table_model_ptr.get(),
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));



	// If the focused feature is modified, we may need to reconstruct to update the view.
	// FIXME:  If the FeatureFocus emits the 'focused_feature_modified' signal, the view will
	// be reconstructed twice -- once here, and once as a result of the 'set_focus' slot in the
	// GeometryFocusHighlight below.
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this, SLOT(reconstruct()));

	// If the focused feature is modified, we may need to update the ShapefileAttributeViewerDialog.
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			&d_shapefile_attribute_viewer_dialog, SLOT(update()));

	// Set up the Canvas Tools.
	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_canvas_tool_choice_ptr.reset(
			new GPlatesGui::CanvasToolChoice(
					d_rendered_geom_collection,
					get_rendered_geometry_factory(),
					d_geom_builder_tool_target,
					d_geom_operation_render_parameters,
					*d_choose_canvas_tool,
					*d_canvas_ptr,
					d_canvas_ptr->globe(),
					*d_canvas_ptr,
					*this,
					*d_feature_table_model_ptr,
					*d_segments_feature_table_model_ptr,
					d_feature_properties_dialog,
					d_feature_focus,
					d_task_panel_ptr->reconstruction_pole_widget(),
					d_task_panel_ptr->create_topology_widget(),
					d_task_panel_ptr->plate_closure_widget(),
					d_canvas_ptr->geometry_focus_highlight()));

	// Set up the Canvas Tool Adapter for handling globe click and drag events.
	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_canvas_tool_adapter_ptr.reset(new GPlatesGui::CanvasToolAdapter(
			*d_canvas_tool_choice_ptr));

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_clicked(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)),
			d_canvas_tool_adapter_ptr.get(), SLOT(handle_click(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_dragged(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_canvas_tool_adapter_ptr.get(), SLOT(handle_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)));

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_released_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_canvas_tool_adapter_ptr.get(), SLOT(handle_release_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)));
	
	// If the user creates a new feature with the DigitisationWidget, we need to reconstruct to
	// make sure everything is displayed properly.
	QObject::connect(&(d_task_panel_ptr->digitisation_widget().get_create_feature_dialog()),
			SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
			this,
			SLOT(reconstruct()));


#if 0
#endif
	// If the user creates a new feature with the PlateClosureWidget, 
	// then we need to create and append property values to it
	QObject::connect(
		&(d_task_panel_ptr->plate_closure_widget().create_feature_dialog()),
		SIGNAL( feature_created(GPlatesModel::FeatureHandle::weak_ref) ),
		d_canvas_tool_adapter_ptr.get(),
		SLOT( handle_create_new_feature( GPlatesModel::FeatureHandle::weak_ref ) )
	);

	// Add a progress bar to the status bar (Hidden until needed).
	std::auto_ptr<QProgressBar> progress_bar(new QProgressBar(this));
	progress_bar->setMaximumWidth(100);
	progress_bar->hide();
	statusBar()->addPermanentWidget(progress_bar.release());
}


GPlatesQtWidgets::ViewportWindow::~ViewportWindow()
{
	// boost::scoped_ptr destructors needs complete type.
}


void	
GPlatesQtWidgets::ViewportWindow::connect_menu_actions()
{
	GlobeCanvas *canvas_ptr = &(d_reconstruction_view_widget.globe_canvas());
	// If you want to add a new menu action, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu.
	// 4. Add code for the triggered() signal your action generates here.
	//    Please keep this function sorted in the same order as menu items appear.

	// Main Tools:
	QObject::connect(action_Drag_Globe, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_drag_globe_tool()));
	QObject::connect(action_Zoom_Globe, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_zoom_globe_tool()));

	QObject::connect(action_Click_Geometry, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_click_geometry_tool()));

	QObject::connect(action_Digitise_New_Polyline, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_digitise_polyline_tool()));
	QObject::connect(action_Digitise_New_MultiPoint, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_digitise_multipoint_tool()));
	QObject::connect(action_Digitise_New_Polygon, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_digitise_polygon_tool()));

	QObject::connect(action_Move_Geometry, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_move_geometry_tool()));
	QObject::connect(action_Move_Vertex, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_move_vertex_tool()));
	// FIXME: The Move Geometry tool, although it has an awesome icon,
	// is to be disabled until it can be implemented.
	action_Move_Geometry->setVisible(false);

	QObject::connect(action_Manipulate_Pole, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_manipulate_pole_tool()));

	QObject::connect(action_Create_Topology, SIGNAL(triggered()),
			d_choose_canvas_tool.get(), SLOT(choose_create_topology_tool()));

	QObject::connect(action_Plate_Closure, SIGNAL(triggered()),
			this, SLOT(choose_plate_closure_platepolygon_tool()));


	// File Menu:
	QObject::connect(action_Open_Feature_Collection, SIGNAL(triggered()),
			&d_manage_feature_collections_dialog, SLOT(open_file()));
	QObject::connect(action_Open_Raster, SIGNAL(triggered()),
			this, SLOT(open_raster()));
	QObject::connect(action_Open_Time_Dependent_Raster_Sequence, SIGNAL(triggered()),
			this, SLOT(open_time_dependent_raster_sequence()));
	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			this, SLOT(pop_up_read_errors_dialog()));
	// ---
	QObject::connect(action_Manage_Feature_Collections, SIGNAL(triggered()),
			this, SLOT(pop_up_manage_feature_collections_dialog()));
	QObject::connect(action_View_Shapefile_Attributes, SIGNAL(triggered()),
			this, SLOT(pop_up_shapefile_attribute_viewer_dialog()));
	// ----
	QObject::connect(action_Quit, SIGNAL(triggered()),
			this, SLOT(close()));
	
	// Edit Menu:
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			&d_feature_properties_dialog, SLOT(choose_query_widget_and_open()));
	QObject::connect(action_Edit_Feature, SIGNAL(triggered()),
			&d_feature_properties_dialog, SLOT(choose_edit_widget_and_open()));
	// ----
	// Unfortunately, the Undo and Redo actions cannot be added in the Designer,
	// or at least, not nicely. We need to ask the QUndoGroup to create some
	// QActions for us, and add them programmatically. To follow the principle
	// of least surprise, placeholder actions are set up in the designer, which
	// this code can use to insert the actions in the correct place with the
	// correct shortcut.
	// The new actions will be linked to the QUndoGroup appropriately.
	QAction *undo_action_ptr =
		GPlatesViewOperations::UndoRedo::instance().get_undo_group().createUndoAction(this, tr("&Undo"));
	QAction *redo_action_ptr =
		GPlatesViewOperations::UndoRedo::instance().get_undo_group().createRedoAction(this, tr("&Redo"));
	undo_action_ptr->setShortcut(action_Undo_Placeholder->shortcut());
	redo_action_ptr->setShortcut(action_Redo_Placeholder->shortcut());
	menu_Edit->insertAction(action_Undo_Placeholder, undo_action_ptr);
	menu_Edit->insertAction(action_Redo_Placeholder, redo_action_ptr);
	menu_Edit->removeAction(action_Undo_Placeholder);
	menu_Edit->removeAction(action_Redo_Placeholder);
	// ----
#if 0		// Delete Feature is nontrivial to implement (in the model) properly.
	QObject::connect(action_Delete_Feature, SIGNAL(triggered()),
			this, SLOT(delete_focused_feature()));
#else
	action_Delete_Feature->setVisible(false);
#endif
	// ----
	QObject::connect(action_Clear_Selection, SIGNAL(triggered()),
			&d_feature_focus, SLOT(unset_focus()));

	// Reconstruction Menu:
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_time_spinbox()));
	QObject::connect(action_Increment_Reconstruction_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(increment_reconstruction_time()));
	QObject::connect(action_Decrement_Reconstruction_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(decrement_reconstruction_time()));
	QObject::connect(action_Animate, SIGNAL(triggered()),
			this, SLOT(pop_up_animate_dialog()));
	// ----
	QObject::connect(action_Specify_Fixed_Plate, SIGNAL(triggered()),
			this, SLOT(pop_up_specify_fixed_plate_dialog()));
	QObject::connect(action_View_Reconstruction_Poles, SIGNAL(triggered()),
			this, SLOT(pop_up_total_reconstruction_poles_dialog()));
	
	// View Menu:
	QObject::connect(action_Show_Raster, SIGNAL(triggered()),
			this, SLOT(enable_raster_display()));
	QObject::connect(action_Show_Points, SIGNAL(triggered()),
			this, SLOT(enable_point_display()));
	QObject::connect(action_Show_Lines, SIGNAL(triggered()),
			this, SLOT(enable_line_display()));
	QObject::connect(action_Show_Polygons, SIGNAL(triggered()),
			this, SLOT(enable_polygon_display()));
	QObject::connect(action_Show_Topologies, SIGNAL(triggered()),
			this, SLOT(enable_topology_display()));
	QObject::connect(action_Show_Multipoint, SIGNAL(triggered()),
			this, SLOT(enable_multipoint_display()));
	QObject::connect(action_Set_Raster_Surface_Extent, SIGNAL(triggered()),
			this, SLOT(pop_up_set_raster_surface_extent_dialog()));
	// ----
	QObject::connect(action_Colour_By_Plate_ID, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_plate_id()));
	QObject::connect(action_Colour_By_Single_Colour, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_single_colour()));
	QObject::connect(action_Colour_By_Feature_Type, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_feature_type()));
	QObject::connect(action_Colour_By_Age, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_age()));
	// ----
	QObject::connect(action_Set_Camera_Viewpoint, SIGNAL(triggered()),
			this, SLOT(pop_up_set_camera_viewpoint_dialog()));
	QObject::connect(action_Move_Camera_Up, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_up()));
	QObject::connect(action_Move_Camera_Down, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_down()));
	QObject::connect(action_Move_Camera_Left, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_left()));
	QObject::connect(action_Move_Camera_Right, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_right()));
	// ----
	QObject::connect(action_Rotate_Camera_Clockwise, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(rotate_camera_clockwise()));
	QObject::connect(action_Rotate_Camera_Anticlockwise, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(rotate_camera_anticlockwise()));
	QObject::connect(action_Reset_Camera_Orientation, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(orient_poles_vertically()));
	// ----
	QObject::connect(action_Set_Zoom, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_zoom_spinbox()));
	QObject::connect(action_Zoom_In, SIGNAL(triggered()),
			&(canvas_ptr->viewport_zoom()), SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			&(canvas_ptr->viewport_zoom()), SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			&(canvas_ptr->viewport_zoom()), SLOT(reset_zoom()));
	// ----
	QObject::connect(action_Export_Geometry_Snapshot, SIGNAL(triggered()),
			this, SLOT(pop_up_export_geometry_snapshot_dialog()));
	
	// Help Menu:
	QObject::connect(action_About, SIGNAL(triggered()),
			this, SLOT(pop_up_about_dialog()));
}


void	
GPlatesQtWidgets::ViewportWindow::set_up_task_panel_actions()
{
	ActionButtonBox &feature_actions = d_task_panel_ptr->feature_action_button_box();

	// If you want to add a new action button, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu (optional).
	// 4. Add code for the triggered() signal your action generates,
	//    see ViewportWindow::connect_menu_actions().
	// 5. Add a new line of code here adding the QAction to the ActionButtonBox.

	feature_actions.add_action(action_Query_Feature);
	feature_actions.add_action(action_Edit_Feature);
#if 0
	// Doesn't work - hidden for release.
	feature_actions.add_action(action_Delete_Feature);
#endif
	feature_actions.add_action(action_Clear_Selection);
}


void
GPlatesQtWidgets::ViewportWindow::set_up_dock_context_menus()
{
	// Search Results Dock:
	dock_search_results->addAction(action_Information_Dock_At_Top);
	dock_search_results->addAction(action_Information_Dock_At_Bottom);
	QObject::connect(action_Information_Dock_At_Top, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_top()));
	QObject::connect(action_Information_Dock_At_Bottom, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_bottom()));
}


void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_top()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::TopDockWidgetArea, dock_search_results);
}

void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_bottom()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::BottomDockWidgetArea, dock_search_results);
}


void
GPlatesQtWidgets::ViewportWindow::highlight_first_clicked_feature_table_row() const
{
	QModelIndex idx = d_feature_table_model_ptr->index(0, 0);
	
	if (idx.isValid()) {
		table_view_clicked_geometries->selectionModel()->clear();

		table_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	}
	table_view_clicked_geometries->scrollToTop();
}

void
GPlatesQtWidgets::ViewportWindow::highlight_segments_table_clear() const
{
	table_view_platepolygon_segments->selectionModel()->clear();
}

void
GPlatesQtWidgets::ViewportWindow::highlight_segments_table_row(int i, bool state) const
{
	QModelIndex idx = d_segments_feature_table_model_ptr->index(i, 0);
	
	if (idx.isValid()) 
	{
		if ( state )
		{
			table_view_platepolygon_segments->selectionModel()->select(idx,
					QItemSelectionModel::Select |
					QItemSelectionModel::Current |
					QItemSelectionModel::Rows);
			table_view_platepolygon_segments->scrollTo(idx);
		}
	}
}





void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time(
		double new_recon_time)
{
	// != does not work with doubles, so we must wrap them in Real.
	GPlatesMaths::Real original_recon_time(d_recon_time);
	if (original_recon_time != GPlatesMaths::Real(new_recon_time)) {
		d_recon_time = new_recon_time;
		// Reconstruct before we tell everyone that we've reconstructed!
		reconstruct();
		emit reconstruction_time_changed(d_recon_time);
	}
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_with_root(
		unsigned long new_recon_root)
{
	if (d_recon_root != new_recon_root) {
		d_recon_root = new_recon_root;
		// Does anyone care if the reconstruction root changed?
	}
	reconstruct();

	// The reconstruction time hasn't really changed, but emitting this signal will 
	// make sure that other parts of GPlates which are dependent on new geometry values 
	// will get updated.
	// FIXME: Create a suitable new slot, or maybe just rename the slot to 
	// something like "reconstruction_time_or_root_changed"
	emit reconstruction_time_changed(d_recon_time);
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time_with_root(
		double new_recon_time,
		unsigned long new_recon_root)
{
	// FIXME: This function is only called once, on application startup, for root=0 and time=0;
	// if we ever need to call this for other reasons, then we should be careful about the relative 
	// order of the reconstruction, and the emit signal. 

	// != does not work with doubles, so we must wrap them in Real.
	GPlatesMaths::Real original_recon_time(d_recon_time);
	if (original_recon_time != GPlatesMaths::Real(new_recon_time)) {
		d_recon_time = new_recon_time;
		emit reconstruction_time_changed(d_recon_time);
	}
	if (d_recon_root != new_recon_root) {
		d_recon_root = new_recon_root;
		// Does anyone care if the reconstruction root changed?
	}
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct()
{
	render_model(d_model_ptr, d_reconstruction_ptr, d_active_reconstructable_files, 
			d_active_reconstruction_files, d_recon_time, d_recon_root,
			d_rendered_geom_collection, get_rendered_geometry_factory(), *get_colour_table());

	if (d_total_reconstruction_poles_dialog.isVisible()) {
		d_total_reconstruction_poles_dialog.update();
	}
	if (action_Show_Raster->isChecked() && ! d_time_dependent_raster_map.isEmpty()) {	
		update_time_dependent_raster();
	}
	if (d_feature_focus.is_valid() && d_feature_focus.associated_rfg()) {
		// There's a focused feature and it has an associated RFG.  We need to update the
		// associated RFG for the new reconstruction.
		d_feature_focus.find_new_associated_rfg(*d_reconstruction_ptr);
	}
	d_canvas_ptr->update_canvas();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_specify_fixed_plate_dialog()
{
	d_specify_fixed_plate_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_set_camera_viewpoint_dialog()
{
	static const GPlatesMaths::PointOnSphere centre_of_canvas =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	GPlatesMaths::PointOnSphere oriented_centre = d_canvas_ptr->globe().Orient(centre_of_canvas);
	GPlatesMaths::LatLonPoint cur_llp = GPlatesMaths::make_lat_lon_point(oriented_centre);

	d_set_camera_viewpoint_dialog.set_lat_lon(cur_llp.latitude(), cur_llp.longitude());
	if (d_set_camera_viewpoint_dialog.exec())
	{
		try {
			GPlatesMaths::LatLonPoint desired_centre(
					d_set_camera_viewpoint_dialog.latitude(),
					d_set_camera_viewpoint_dialog.longitude());
			
			GPlatesMaths::PointOnSphere oriented_desired_centre = 
					d_canvas_ptr->globe().orientation().orient_point(
							GPlatesMaths::make_point_on_sphere(desired_centre));
			d_canvas_ptr->globe().SetNewHandlePos(oriented_desired_centre);
			d_canvas_ptr->globe().UpdateHandlePos(centre_of_canvas);
			
			d_canvas_ptr->globe().orientation().orient_poles_vertically();
			d_canvas_ptr->update_canvas();
		} catch (GPlatesMaths::InvalidLatLonException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.

			// User somehow managed to specify an invalid lat,lon. Pretend it didn't happen.
		}
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_total_reconstruction_poles_dialog()
{
	d_total_reconstruction_poles_dialog.update();

	d_total_reconstruction_poles_dialog.show();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_animate_dialog()
{
	if ( ! d_animate_dialog_has_been_shown) {
		d_animate_dialog.set_start_time_value_to_view_time();
		d_animate_dialog.set_current_time_value_to_view_time();
		d_animate_dialog_has_been_shown = true;
	}
	d_animate_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_about_dialog()
{
	d_about_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_license_dialog()
{
	d_license_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::choose_drag_globe_tool()
{
	uncheck_all_tools();
	action_Drag_Globe->setChecked(true);
	d_canvas_tool_choice_ptr->choose_reorient_globe_tool();
}


void
GPlatesQtWidgets::ViewportWindow::choose_zoom_globe_tool()
{
	uncheck_all_tools();
	action_Zoom_Globe->setChecked(true);
	d_canvas_tool_choice_ptr->choose_zoom_globe_tool();
}


void
GPlatesQtWidgets::ViewportWindow::choose_click_geometry_tool()
{
	uncheck_all_tools();
	action_Click_Geometry->setChecked(true);
	d_canvas_tool_choice_ptr->choose_click_geometry_tool();
	d_task_panel_ptr->choose_feature_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polyline_tool()
{
	uncheck_all_tools();
	action_Digitise_New_Polyline->setChecked(true);
	d_canvas_tool_choice_ptr->choose_digitise_polyline_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_multipoint_tool()
{
	uncheck_all_tools();
	action_Digitise_New_MultiPoint->setChecked(true);
	d_canvas_tool_choice_ptr->choose_digitise_multipoint_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polygon_tool()
{
	uncheck_all_tools();
	action_Digitise_New_Polygon->setChecked(true);
	d_canvas_tool_choice_ptr->choose_digitise_polygon_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}

void
GPlatesQtWidgets::ViewportWindow::choose_plate_closure_platepolygon_tool()
{
	uncheck_all_tools();
	action_Plate_Closure->setChecked(true);
	d_canvas_tool_choice_ptr->choose_plate_closure_platepolygon_tool();
	d_task_panel_ptr->choose_plate_closure_tab();
}

void
GPlatesQtWidgets::ViewportWindow::choose_move_geometry_tool()
{
	uncheck_all_tools();
	action_Move_Geometry->setChecked(true);
	d_canvas_tool_choice_ptr->choose_move_geometry_tool();
	d_task_panel_ptr->choose_feature_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_move_vertex_tool()
{
	uncheck_all_tools();
	action_Move_Vertex->setChecked(true);
	d_canvas_tool_choice_ptr->choose_move_vertex_tool();

	d_task_panel_ptr->choose_move_vertex_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_manipulate_pole_tool()
{
	uncheck_all_tools();
	action_Manipulate_Pole->setChecked(true);
	d_canvas_tool_choice_ptr->choose_manipulate_pole_tool();
	d_task_panel_ptr->choose_modify_pole_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_create_topology_tool()
{
	uncheck_all_tools();
	action_Create_Topology->setChecked(true);
	d_canvas_tool_choice_ptr->choose_create_topology_tool();
	d_task_panel_ptr->choose_create_topology_tab();
}




void
GPlatesQtWidgets::ViewportWindow::uncheck_all_tools()
{
	action_Drag_Globe->setChecked(false);
	action_Zoom_Globe->setChecked(false);
	action_Click_Geometry->setChecked(false);
	action_Digitise_New_Polyline->setChecked(false);
	action_Digitise_New_MultiPoint->setChecked(false);
	action_Digitise_New_Polygon->setChecked(false);
	action_Plate_Closure->setChecked(false);
	action_Move_Geometry->setChecked(false);
	action_Move_Vertex->setChecked(false);
	action_Manipulate_Pole->setChecked(false);
	action_Create_Topology->setChecked(false);
}


void
GPlatesQtWidgets::ViewportWindow::enable_or_disable_feature_actions(
		GPlatesModel::FeatureHandle::weak_ref focused_feature)
{
	bool enable = focused_feature.is_valid();
	action_Query_Feature->setEnabled(enable);
	action_Edit_Feature->setEnabled(enable);
	// FIXME: Move Geometry and Move Vertex could also be used for temporary
	// GeometryOnSphere manipulation, once we have a canonical location for them.
	action_Move_Geometry->setEnabled(enable);
	//action_Move_Vertex->setEnabled(enable);
	action_Manipulate_Pole->setEnabled(enable);
	action_Create_Topology->setEnabled(enable);
#if 0		// Delete Feature is nontrivial to implement (in the model) properly.
	action_Delete_Feature->setEnabled(enable);
#else
	action_Delete_Feature->setDisabled(true);
#endif
	action_Clear_Selection->setEnabled(enable);
#if 0
	// FIXME: Add to Selection is unimplemented and should stay disabled for now.
	// FIXME: To handle the "Remove from Selection", "Clear Selection" actions,
	// we may want to modify this method to also test for a nonempty selection of features.
	action_Add_Feature_To_Selection->setEnabled(enable);
#endif
}


void 
GPlatesQtWidgets::ViewportWindow::uncheck_all_colouring_tools() {
	action_Colour_By_Plate_ID->setChecked(false);
	action_Colour_By_Single_Colour->setChecked(false);
	action_Colour_By_Feature_Type->setChecked(false);
	action_Colour_By_Age->setChecked(false);
}

void		
GPlatesQtWidgets::ViewportWindow::choose_colour_by_plate_id() {
	d_colour_table_ptr = GPlatesGui::PlatesColourTable::Instance();
	uncheck_all_colouring_tools();
	action_Colour_By_Plate_ID->setChecked(true);
	reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::choose_colour_by_single_colour() {	
	QColor qcolor = QColorDialog::getColor();

	GPlatesGui::Colour colour(qcolor.redF(), qcolor.greenF(), qcolor.blueF(), qcolor.alphaF());

	GPlatesGui::SingleColourTable::Instance()->set_colour(colour);
	d_colour_table_ptr = GPlatesGui::SingleColourTable::Instance();

	uncheck_all_colouring_tools();
	action_Colour_By_Single_Colour->setChecked(true);
	reconstruct();
}

void	
GPlatesQtWidgets::ViewportWindow::choose_colour_by_feature_type() {
	d_colour_table_ptr = GPlatesGui::FeatureColourTable::Instance();

	uncheck_all_colouring_tools();
	action_Colour_By_Feature_Type->setChecked(true);
	reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::choose_colour_by_age() {
	GPlatesGui::AgeColourTable::Instance()->set_viewport_window(*this);
	d_colour_table_ptr = GPlatesGui::AgeColourTable::Instance();

	uncheck_all_colouring_tools();
	action_Colour_By_Age->setChecked(true);
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_read_errors_dialog()
{
	d_read_errors_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_manage_feature_collections_dialog()
{
	d_manage_feature_collections_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::deactivate_loaded_file(
		file_info_iterator loaded_file)
{
	// Don't bother checking whether 'loaded_file' is actually an element of
	// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
	// lists to remove the value if it *is* an element.

	// list<T>::remove(const T &val) -- remove all elements with value 'val'.
	// Will not throw (unless element comparisons can throw).
	// See Josuttis section 6.10.7 "Inserting and Removing Elements".
	d_active_reconstructable_files.remove(loaded_file);
	d_active_reconstruction_files.remove(loaded_file);

	// FIXME:  This should not happen here -- in fact, it should be removal of the loaded file
	// (using 'remove_loaded_file' in ApplicationState) which triggers *this*! -- but until we
	// have multiple view windows, it doesn't matter.
	GPlatesAppState::ApplicationState::instance()->remove_loaded_file(loaded_file);

	// Update the shapefile-attribute viewer dialog, which needs to know which files are loaded.
	d_shapefile_attribute_viewer_dialog.update();
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active(
		file_info_iterator loaded_file)
{
	return is_file_active_reconstructable(loaded_file) ||
			is_file_active_reconstruction(loaded_file);
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active_reconstructable(
		file_info_iterator loaded_file)
{
	active_files_iterator reconstructable_it = d_active_reconstructable_files.begin();
	active_files_iterator reconstructable_end = d_active_reconstructable_files.end();
	for (; reconstructable_it != reconstructable_end; ++reconstructable_it) {
		if (*reconstructable_it == loaded_file) {
			return true;
		}
	}
	return false;
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active_reconstruction(
		file_info_iterator loaded_file)
{
	active_files_iterator reconstruction_it = d_active_reconstruction_files.begin();
	active_files_iterator reconstruction_end = d_active_reconstruction_files.end();
	for (; reconstruction_it != reconstruction_end; ++reconstruction_it) {
		if (*reconstruction_it == loaded_file) {
			return true;
		}
	}
	return false;
}


void
GPlatesQtWidgets::ViewportWindow::set_file_active_reconstructable(
		file_info_iterator file_it,
		bool activate)
{
	if (activate) {
		// Add it to the list, if it's not there already.
		if ( ! is_file_active_reconstructable(file_it)) {
			d_active_reconstructable_files.push_back(file_it);
		}
	} else {
		// Don't bother checking whether 'loaded_file' is actually an element of
		// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
		// lists to remove the value if it *is* an element.
		
		// list<T>::remove(const T &val) -- remove all elements with value 'val'.
		// Will not throw (unless element comparisons can throw).
		// See Josuttis section 6.10.7 "Inserting and Removing Elements".
		d_active_reconstructable_files.remove(file_it);
	}
	// Active features changed, will need to reconstruct() to make RFGs for them.
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::set_file_active_reconstruction(
		file_info_iterator file_it,
		bool activate)
{
	if (activate) {
		// At the moment, we only want one active reconstruction tree
		// at a time. Deactivate the others, and update ManageFeatureCollectionsDialog
		// so that the other buttons get deselected appropriately.
		d_active_reconstruction_files.clear();
		d_active_reconstruction_files.push_back(file_it);
		// NOTE: in the current setup, the only place this set_file_active_xxxx()
		// method is called is by ManageFeatureCollectionsDialog itself, in response
		// to a button press. Therefore we can assume it is up-to-date already, except
		// for this one case where we have cleared all the other reconstruction files.
		// If this situation changes and other code will also be calling
		// set_file_active_xxxx() or otherwise messing with file 'active' status, you
		// will need to call ManageFeatureCollectionsDialog::update_state() at the end
		// of both these methods.
		d_manage_feature_collections_dialog.update_state();
	} else {
		// Don't bother checking whether 'loaded_file' is actually an element of
		// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
		// lists to remove the value if it *is* an element.
		
		// list<T>::remove(const T &val) -- remove all elements with value 'val'.
		// Will not throw (unless element comparisons can throw).
		// See Josuttis section 6.10.7 "Inserting and Removing Elements".
		d_active_reconstruction_files.remove(file_it);
	}
	// Active rotation changed, will need to reconstruct() to make make use of it.
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::create_svg_file()
{
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("SVG file (*.svg)"));

	if (filename.isEmpty()){
		return;
	}

	bool result = GPlatesGui::SvgExport::create_svg_output(filename,d_canvas_ptr);
	if (!result){
		std::cerr << "Error creating SVG output.." << std::endl;
	}

}


void
GPlatesQtWidgets::ViewportWindow::close_all_dialogs()
{
	d_about_dialog.reject();
	d_animate_dialog.reject();
	d_total_reconstruction_poles_dialog.reject();
	d_feature_properties_dialog.reject();
	d_license_dialog.reject();
	d_manage_feature_collections_dialog.reject();
	d_read_errors_dialog.reject();
	d_set_camera_viewpoint_dialog.reject();
	d_set_raster_surface_extent_dialog.reject();
	d_specify_fixed_plate_dialog.reject();
	d_shapefile_attribute_viewer_dialog.reject();
}

void
GPlatesQtWidgets::ViewportWindow::closeEvent(QCloseEvent *close_event)
{
	// For now, always accept the close event.
	// In the future, ->reject() can be used to postpone closure in the event of
	// unsaved files, etc.
	close_event->accept();
	// If we decide to accept the close event, we should also tidy up after ourselves.
	close_all_dialogs();
}

void
GPlatesQtWidgets::ViewportWindow::remap_shapefile_attributes(
	GPlatesFileIO::FileInfo &file_info)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	GPlatesFileIO::ShapeFileReader::remap_shapefile_attributes(file_info,*d_model_ptr,read_errors);

	d_read_errors_dialog.update();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}

	// Plate-ids may have changed, so update the reconstruction. 
	reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::enable_raster_display()
{
	if (action_Show_Raster->isChecked())
	{
		d_canvas_ptr->enable_raster_display();
	}
	else
	{
		d_canvas_ptr->disable_raster_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::enable_point_display()
{
	if (action_Show_Points->isChecked())
	{
		d_canvas_ptr->enable_point_display();
	}
	else
	{
		d_canvas_ptr->disable_point_display();
	}
}


void
GPlatesQtWidgets::ViewportWindow::enable_line_display()
{
	if (action_Show_Lines->isChecked())
	{
		d_canvas_ptr->enable_line_display();
	}
	else
	{
		d_canvas_ptr->disable_line_display();
	}
}


void
GPlatesQtWidgets::ViewportWindow::enable_polygon_display()
{
	if (action_Show_Polygons->isChecked())
	{
		d_canvas_ptr->enable_polygon_display();
	}
	else
	{
		d_canvas_ptr->disable_polygon_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::enable_topology_display()
{
	if (action_Show_Topologies->isChecked())
	{
		d_canvas_ptr->enable_topology_display();
	}
	else
	{
		d_canvas_ptr->disable_topology_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::enable_multipoint_display()
{
	if (action_Show_Multipoint->isChecked())
	{
		d_canvas_ptr->enable_multipoint_display();
	}
	else
	{
		d_canvas_ptr->disable_multipoint_display();
	}
}


void
GPlatesQtWidgets::ViewportWindow::open_raster()
{



	QString filename = QFileDialog::getOpenFileName(0,
		QObject::tr("Open File"), d_open_file_path, QObject::tr("Raster files (*.jpg *.jpeg)") );

	if ( filename.isEmpty()){
		return;
	}

	if (load_raster(filename))
	{
		// If we've successfully loaded a single raster, clear the raster_map.
		d_time_dependent_raster_map.clear();
	}
	QFileInfo last_opened_file(filename);
	d_open_file_path = last_opened_file.path();
}

bool
GPlatesQtWidgets::ViewportWindow::load_raster(
	QString filename)
{
	bool result = false;
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	
	GPlatesFileIO::FileInfo file_info(filename);

	try{

		GPlatesFileIO::RasterReader::read_file(file_info, d_canvas_ptr->globe().texture(), read_errors);
		action_Show_Raster->setChecked(true);
		result = true;
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
			new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
			new GPlatesFileIO::LineNumberInFile(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
			e_source,
			e_location,
			GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
			GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
	catch (GPlatesGlobal::Exception &e)
	{
		std::cerr << "Caught GPlates exception: " << e << std::endl;
	}
	catch (...)
	{
		std::cerr << "Caught exception loading raster file." << std::endl;
	}

	d_canvas_ptr->update_canvas();
	d_read_errors_dialog.update();

	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}
	return result;
}

void
GPlatesQtWidgets::ViewportWindow::open_time_dependent_raster_sequence()
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	QFileDialog file_dialog(this,QObject::tr("Choose Folder Containing Time-dependent Rasters"),d_open_file_path,NULL);
	file_dialog.setFileMode(QFileDialog::DirectoryOnly);

	if (file_dialog.exec())
	{
		
		QStringList directory_list = file_dialog.selectedFiles();
		QString directory = directory_list.at(0);

		GPlatesFileIO::RasterReader::populate_time_dependent_raster_map(d_time_dependent_raster_map,directory,read_errors);
		
		QFileInfo last_opened_file(file_dialog.directory().absoluteFilePath(directory_list.last()));
		d_open_file_path = last_opened_file.path();

		d_read_errors_dialog.update();
	
		// Pop up errors only if appropriate.
		GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
		if (num_initial_errors != num_final_errors) {
			d_read_errors_dialog.show();
		}
	}

	if (!d_time_dependent_raster_map.isEmpty())
	{
		action_Show_Raster->setChecked(true);
		update_time_dependent_raster();
	}



}

void
GPlatesQtWidgets::ViewportWindow::update_time_dependent_raster()
{
	QString filename = GPlatesFileIO::RasterReader::get_nearest_raster_filename(d_time_dependent_raster_map,d_recon_time);
	load_raster(filename);
}

// FIXME: Should be a ViewState operation, or /somewhere/ better than this.
void
GPlatesQtWidgets::ViewportWindow::delete_focused_feature()
{
	if (d_feature_focus.is_valid()) {
		GPlatesModel::FeatureHandle::weak_ref feature_ref = d_feature_focus.focused_feature();
#if 0		// Cannot call ModelInterface::remove_feature() as it is #if0'd out and not implemented in Model!
		// FIXME: figure out FeatureCollectionHandle::weak_ref that feature_ref belongs to.
		// Possibly implement that as part of ModelUtils.
		d_model_ptr->remove_feature(feature_ref, collection_ref);
#endif
		d_feature_focus.announce_deletion_of_focused_feature();
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_set_raster_surface_extent_dialog()
{
	d_set_raster_surface_extent_dialog.exec();
}
GPlatesViewOperations::RenderedGeometryFactory &
GPlatesQtWidgets::ViewportWindow::get_rendered_geometry_factory()
{
	return d_canvas_ptr->get_rendered_geometry_factory();
}

void
GPlatesQtWidgets::ViewportWindow::initialise_rendered_geom_collection()
{
	// Reconstruction rendered layer is always active.
	d_rendered_geom_collection.set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Specify which main rendered layers are orthogonal to each other - when
	// one is activated the others are automatically deactivated.
	GPlatesViewOperations::RenderedGeometryCollection::orthogonal_main_layers_type orthogonal_main_layers;
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::CREATE_TOPOLOGY_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::PLATE_CLOSURE_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_MANIPULATION_LAYER);

	d_rendered_geom_collection.set_orthogonal_main_layers(orthogonal_main_layers);
}
void
GPlatesQtWidgets::ViewportWindow::pop_up_shapefile_attribute_viewer_dialog()
{
	d_shapefile_attribute_viewer_dialog.show();
	d_shapefile_attribute_viewer_dialog.update();
}
