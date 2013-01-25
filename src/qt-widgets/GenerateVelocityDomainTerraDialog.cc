/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <sstream>
#include <string>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

#include "GenerateVelocityDomainTerraDialog.h"

#include "ProgressDialog.h"
#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/GenerateVelocityDomainCitcoms.h"
#include "app-logic/ReconstructGraph.h"

#include "feature-visitors/GeometryFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FeatureFocus.h"

#include "maths/MultiPointOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/ModelInterface.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "presentation/ViewState.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GpmlPlateId.h"

#include "utils/Base2Utils.h"


namespace
{
	const std::string MT_PLACE_HOLDER = "%mt"; // Terra 'mt' parameter.
	const std::string NT_PLACE_HOLDER = "%nt"; // Terra 'nt' parameter.
	const std::string ND_PLACE_HOLDER = "%nd"; // Terra 'nd' parameter.
	const std::string NP_PLACE_HOLDER = "%np"; // Number of processors.

	const char *HELP_DIALOG_TITLE_CONFIGURATION = QT_TR_NOOP("Configuration parameters");

	const char *HELP_DIALOG_TEXT_CONFIGURATION = QT_TR_NOOP(
		"<html><body>"
		"<p/>"
		"<p>The nodex and nodey parameters specify the number of nodes in each edge of cap diamonds.</p>"
		"<p>These	nodes are used to divide the diamonds into smaller ones evenly.</p>"
		"<p>For the global mesh, the nodex always equals nodey.</p>" 
		"<p>In current release, we only support global mesh. The regional mesh might come in the future.</p>"
		"</body></html>");

	const char *HELP_DIALOG_TITLE_OUTPUT = QT_TR_NOOP("Setting output directory and file name template");

	const char *HELP_DIALOG_TEXT_OUTPUT = QT_TR_NOOP(
		"<html><body>"
		"<p/>"
		"<p>12 files will be generated in the specifed output directory.</p>"
		"<p>The file name template can be specified as something like %d.mesh.%c "
		"where the '%d' represents the mesh point density and '%c' represents the cap number.</p>"
		"<p>%d and %c must appear in the template once and only once.</p>"
		"</body></html>\n");


	/**
	 * Calculates the number of processors given the Terra parameters 'mt', 'nt' and 'nd'.
	 */
	int
	calculate_num_processors(
			int mt,
			int nt,
			int nd)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				mt > 0 && GPlatesUtils::Base2::is_power_of_two(mt) &&
					nt > 0 && GPlatesUtils::Base2::is_power_of_two(nt) &&
					(nd == 5 || nd == 10),
				GPLATES_ASSERTION_SOURCE);

		const int ldiv = mt / nt;
		const int sp = ldiv * ldiv;

		return 10 * sp / nd;
	}
}


GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::GenerateVelocityDomainTerraDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_ ) :
	GPlatesDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint | 
			Qt::MSWindowsFixedSizeDialogHint),
	d_view_state(view_state),
	d_help_dialog_configuration(
			new InformationDialog(
					tr(HELP_DIALOG_TEXT_CONFIGURATION), 
					tr(HELP_DIALOG_TITLE_CONFIGURATION), 
					this)),
	d_help_dialog_output(
			new InformationDialog(
					tr(HELP_DIALOG_TEXT_OUTPUT), 
					tr(HELP_DIALOG_TITLE_OUTPUT), 
					this)),
	d_open_directory_dialog(
			this,
			tr("Select Path"),
			view_state),
	d_mt(32),
	d_nt(16),
	d_nd(5),
	d_num_processors(calculate_num_processors(d_mt, d_nt, d_nd)),
	d_file_name_template(
			"TerraMesh." + MT_PLACE_HOLDER + "." + NT_PLACE_HOLDER + "." + ND_PLACE_HOLDER + "." + NP_PLACE_HOLDER)
{
	setupUi(this);
	
	QObject::connect(
			button_path,
			SIGNAL(clicked()),
			this,
			SLOT(select_path()));
	QObject::connect(
			lineEdit_path,
			SIGNAL(editingFinished()),
			this,
			SLOT(set_path()));
	QObject::connect(
			lineEdit_file_template,
			SIGNAL(editingFinished()),
			this,
			SLOT(set_file_name_template()));
	QObject::connect(
			mt_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_mt_value_changed(int)));
	QObject::connect(
			nt_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_nt_value_changed(int)));
	QObject::connect(
			nd_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_nd_value_changed(int)));
	QObject::connect(
			pushButton_info_output,
			SIGNAL(clicked()),
			d_help_dialog_output,
			SLOT(show()));
	QObject::connect(
			pushButton_info_configuration,
			SIGNAL(clicked()),
			d_help_dialog_configuration,
			SLOT(show()));

	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(generate_velocity_domain()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	QtWidgetUtils::resize_based_on_size_hint(this);

	//
	// Initialise the GUI from the initial parameter values.
	//

	mt_spinbox->setValue(d_mt);
	nt_spinbox->setValue(d_nt);
	nd_spinbox->setValue(d_nd);
	
	lineEdit_path->setText(QDir::toNativeSeparators(QDir::currentPath()));

	lineEdit_file_template->setText(d_file_name_template.c_str());
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::handle_mt_value_changed(
		int mt)
{
	d_mt = mt;

	// Update the number of processors.
	set_num_processors();
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::handle_nt_value_changed(
		int nt)
{
	d_nt = nt;

	// Update the number of processors.
	set_num_processors();
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::handle_nd_value_changed(
		int nd)
{
	d_nd = nd;

	// Update the number of processors.
	set_num_processors();
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::set_num_processors()
{
	d_num_processors = calculate_num_processors(d_mt, d_nt, d_nd);

	num_processors_label->setText(tr("Number of processors: %1").arg(d_num_processors));
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::set_path()
{
	QString new_path = lineEdit_path->text();
	
	QFileInfo new_path_info(new_path);

	if (new_path_info.exists() && 
		new_path_info.isDir() && 
		new_path_info.isWritable()) 
	{
		d_path = new_path;
		
		// Make sure the path ends with a directory separator
		if (!d_path.endsWith(QDir::separator()))
		{
			d_path = d_path + QDir::separator();
		}
	}
	else
	{
		// If the new path is invalid, we don't allow the path change.
		lineEdit_path->setText(QDir::toNativeSeparators(d_path));
	}
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::select_path()
{
	d_open_directory_dialog.select_directory(lineEdit_path->text());
	QString pathname = d_open_directory_dialog.get_existing_directory();

	if (!pathname.isEmpty())
	{
		lineEdit_path->setText(QDir::toNativeSeparators(pathname));
		set_path();
	}
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::set_file_name_template()
{
	const QString text = lineEdit_file_template->text();
	std::size_t c_pos = text.toStdString().find(MT_PLACE_HOLDER);
	std::size_t d_pos = text.toStdString().find(ND_PLACE_HOLDER);
	
	if(text.isEmpty() ||
		c_pos == std::string::npos ||
		d_pos == std::string::npos ||
		text.toStdString().find(MT_PLACE_HOLDER, c_pos+1) != std::string::npos  ||
		text.toStdString().find(ND_PLACE_HOLDER, d_pos+1) != std::string::npos )
	{
		QMessageBox::warning(
				this, 
				tr("Invalid template"), 
				tr("The file name template is not valid. "),
				QMessageBox::Ok, QMessageBox::Ok);
		lineEdit_file_template->setText(d_file_name_template.c_str());
		return;
	}

	d_file_name_template = text.toStdString();
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::generate_velocity_domain()
{
	GPlatesModel::ModelInterface model = d_view_state.get_application_state().get_model_interface();

	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(model.access_model());

	// Loading files will trigger layer additions.
	// As an optimisation (ie, not required), put all layer additions in a single add layers group.
	// It dramatically improves the speed of the Visual Layers dialog when there's many layers.
	GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(
			d_view_state.get_application_state().get_reconstruct_graph());
	add_layers_group.begin_add_or_remove_layers();

	main_buttonbox->setDisabled(true);

	std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> geometries;

	ProgressDialog *progress_dlg = new ProgressDialog(this);
	progress_dlg->setRange(0, d_num_processors);
	progress_dlg->setValue(0);
	progress_dlg->show();

	// Iterate over the Terra processors.
	for (int np = 0; np < d_num_processors; ++np)
	{
		std::stringstream stream;
		stream << tr("Generating domain for Terra processor # ").toStdString() << np << " ...";
		progress_dlg->update_progress(np, stream.str().c_str());

// 		geometries.push_back(
// 				GPlatesAppLogic::GenerateVelocityDomainCitcoms::generate_mesh_geometry(d_node_x, i));

		save_velocity_domain_file(np);

		if (progress_dlg->canceled())
		{
			progress_dlg->close();
			main_buttonbox->setDisabled(false);
			close();
			return;
		}
	}
	progress_dlg->disable_cancel_button(true);

	add_layers_group.end_add_or_remove_layers();

	main_buttonbox->setDisabled(false);
	progress_dlg->reject();

	accept();
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::save_velocity_domain_file(
		int processor_number)
{
	// Create a feature collection that is not added to the model.
	const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create();

	// Get a weak reference so we can add features to the feature collection.
	const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
			feature_collection->reference();

	static const GPlatesModel::FeatureType mesh_node_feature_type = 
			GPlatesModel::FeatureType::create_gpml("MeshNode");

	GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(
			feature_collection_ref,
			mesh_node_feature_type);

	//create the geometry property and append to feature
// 	feature->add(
// 			GPlatesModel::TopLevelPropertyInline::create(
// 				GPlatesModel::PropertyName::create_gpml("meshPoints"),
// 				GPlatesPropertyValues::GmlMultiPoint::create(geometries[i])));

	//
	// Add 'reconstructionPlateId' and 'validTime' to mesh points feature
	// These two properties are needed to show mesh points on globe.
	//

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
		GPlatesPropertyValues::GpmlPlateId::create(0);
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
				GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id)));


	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
		GPlatesModel::ModelUtils::create_gml_time_period(
		GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
		GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gml("validTime"),
				gml_valid_time));

	// Get the integer parameters in string form.
	std::stringstream mt_stream;
	mt_stream << d_mt;
	std::stringstream nt_stream;
	nt_stream << d_nt;
	std::stringstream nd_stream;
	nd_stream << d_nd;
	std::stringstream np_stream;
	np_stream << processor_number;

	// Generate the filename from the template by replacing the place holders with parameter values.
	std::string file_name = d_file_name_template;	
	file_name.append(".gpml");
	file_name.replace(file_name.find(MT_PLACE_HOLDER), MT_PLACE_HOLDER.size(), mt_stream.str());
	file_name.replace(file_name.find(NT_PLACE_HOLDER), NT_PLACE_HOLDER.size(), nt_stream.str());
	file_name.replace(file_name.find(ND_PLACE_HOLDER), ND_PLACE_HOLDER.size(), nd_stream.str());
	file_name.replace(file_name.find(NP_PLACE_HOLDER), NP_PLACE_HOLDER.size(), np_stream.str());
	file_name = d_path.toStdString() + file_name;

	// Make a new FileInfo object for saving to a new file.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo(file_name.c_str());

	// Save the feature collection to a file that is registered with
	// FeatureCollectionFileState (maintains list of all loaded files).
	d_view_state.get_application_state().get_feature_collection_file_io().create_file(
			new_fileinfo, feature_collection);
}
