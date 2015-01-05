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
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QValidator>

#include "GenerateVelocityDomainTerraDialog.h"

#include "ProgressDialog.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/GenerateVelocityDomainTerra.h"
#include "app-logic/ReconstructGraph.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FileIOFeedback.h"

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
			"<p>The following Terra parameters, related to gridding, are:</p>"
			"<ul>"
			"<li>mt - Number of grid intervals along icosahedral diamond edge (must be a power-of-two).</li>"
			"<li>nt - Number of grid intervals along edge of local subdomain (must be a power-of-two).</li>"
			"<li>nd - Number of diamonds mapped to a local process (this must be either 5 or 10).</li>"
			"</ul>"
			"<p>The number of processors is determined by the above three parameters according to "
			"'(mt/nt) * (mt/nt) * (10/nd)' and hence 'mt' must be greater or equal to 'nt'.</p>"
			"</body></html>");

	const char *HELP_DIALOG_TITLE_OUTPUT = QT_TR_NOOP("Setting output directory and file name template");

	const char *HELP_DIALOG_TEXT_OUTPUT = QT_TR_NOOP(
			"<html><body>"
			"<p/>"
			"<p>Generated GPML files will be saved to the specifed output directory.</p>"
			"<p>The filename template enables Terra parameters to be specified in the output filenames "
			"using the following template parameters:</p>"
			"<ul>"
			"<li>%mt - gets replaced with the Terra 'mt' parameter.<li>"
			"<li>%nt - gets replaced with the Terra 'nt' parameter.<li>"
			"<li>%nd - gets replaced with the Terra 'nd' parameter.<li>"
			"<li>%np - gets replaced with the Terra processor number of the current output file.<li>"
			"</ul>"
			"<p><b>Note that '%np' must appear at least once since it's the only parameter "
			"that varies across the output files.</b></p>"
			"<p>An example template filename is 'TerraMesh.%mt.%nt.%nd.%np'.</p>"
			"</body></html>\n");


	/**
	 * Replace all occurrences of a place holder substring with its replacement substring.
	 */
	void
	replace_place_holder(
			std::string &str,
			const std::string &place_holder,
			const std::string &replacement)
	{
		while (true)
		{
			const std::string::size_type str_index = str.find(place_holder);
			if (str_index == std::string::npos)
			{
				break;
			}
			str.replace(str_index, place_holder.size(), replacement);
		}
	}


	/**
	 * A QSpinBox that only allows power-of-two values.
	 */
	class PowerOfTwoSpinBox :
			public QSpinBox
	{
	public:

		explicit
		PowerOfTwoSpinBox(
				QWidget *parent_) :
			QSpinBox(parent_)
		{  }

		virtual
		void
		stepBy(
				int steps)
		{
			int current_value = value();

			for ( ; steps > 0; --steps)
			{
				// The '+1' ensures we get the next power-of-two instead of current power-of-two.
				const int next_value = GPlatesUtils::Base2::next_power_of_two(current_value + 1);
				if (next_value > maximum())
				{
					break;
				}

				current_value = next_value;
			}

			for ( ; steps < 0; ++steps)
			{
				// We can't go to a lower power-of-two than 1.
				if (current_value == 1)
				{
					break;
				}

				// The '-1' ensures we get the previous power-of-two instead of current power-of-two.
				const int prev_value = GPlatesUtils::Base2::previous_power_of_two(current_value - 1);
				if (prev_value < minimum())
				{
					break;
				}

				current_value = prev_value;
			}

			setValue(current_value);
		}

		virtual
		QValidator::State
		validate(
				QString &input_,
				int &pos_) const
		{
			bool ok;
			const int value_ = locale().toInt(input_, &ok);

			if (ok)
			{
				return GPlatesUtils::Base2::is_power_of_two(value_)
						? QValidator::Acceptable
						: QValidator::Intermediate;
			}

			return QValidator::Invalid;
		}

		virtual
		void
		fixup(
				QString &input_) const
		{
			QLocale locale_ = locale();

			bool ok;
			int value_ = locale_.toInt(input_, &ok);

			if (ok)
			{
				// Set to the next power-of-two if value is not already a power-of-two.
				if (!GPlatesUtils::Base2::is_power_of_two(value_))
				{
					int prev_value = GPlatesUtils::Base2::previous_power_of_two(value_);
					if (prev_value < minimum())
					{
						prev_value = minimum();
					}
					int next_value = GPlatesUtils::Base2::next_power_of_two(value_);
					if (next_value > maximum())
					{
						next_value = maximum();
					}

					// Round the actual value to the nearest of the previous and next power-of-two.
					if (value_ - prev_value < next_value - value_)
					{
						input_ = locale_.toString(prev_value);
					}
					else
					{
						input_ = locale_.toString(next_value);
					}
				}
			}
		}
	};


	/**
	 * A QSpinBox for the Terra 'nd' parameter which can only be 5 or 10.
	 */
	class NdSpinBox :
			public QSpinBox
	{
	public:

		explicit
		NdSpinBox(
				QWidget *parent_) :
			QSpinBox(parent_)
		{  }

		virtual
		QValidator::State
		validate(
				QString &input_,
				int &pos_) const
		{
			bool ok;
			const int value_ = locale().toInt(input_, &ok);

			if (ok)
			{
				if (value_ == 5 || value_ == 10)
				{
					return QValidator::Acceptable;
				}
				else if (value_ == 1)
				{
					// "1" is intermediate for "10".
					return QValidator::Intermediate;
				}
			}

			return QValidator::Invalid;
		}
	};
}


GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::GenerateVelocityDomainTerraDialog(
		ViewportWindow &main_window_,
		QWidget *parent_ ) :
	GPlatesDialog(
			parent_,
			Qt::CustomizeWindowHint |
			Qt::WindowTitleHint |
			Qt::WindowSystemMenuHint |
			Qt::MSWindowsFixedSizeDialogHint),
	d_main_window(main_window_),
	d_mt(32),
	d_nt(16),
	d_nd(5),
	d_num_processors(GPlatesAppLogic::GenerateVelocityDomainTerra::calculate_num_processors(d_mt, d_nt, d_nd)),
	d_file_name_template(
			"TerraMesh." + MT_PLACE_HOLDER + "." + NT_PLACE_HOLDER + "." + ND_PLACE_HOLDER + "." + NP_PLACE_HOLDER),
	d_mt_spinbox(NULL),
	d_nt_spinbox(NULL),
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
			main_window_.get_view_state())
{
	setupUi(this);

	d_mt_spinbox = new PowerOfTwoSpinBox(this);
	d_mt_spinbox->setRange(1, 1024);
	QtWidgetUtils::add_widget_to_placeholder(
			d_mt_spinbox, mt_spinbox_placeholder);

	d_nt_spinbox = new PowerOfTwoSpinBox(this);
	d_nt_spinbox->setRange(1, 1024);
	QtWidgetUtils::add_widget_to_placeholder(
			d_nt_spinbox, nt_spinbox_placeholder);

	NdSpinBox *nd_spinbox = new NdSpinBox(this);
	nd_spinbox->setRange(5, 10);
	nd_spinbox->setSingleStep(5);
	QtWidgetUtils::add_widget_to_placeholder(
			nd_spinbox, nd_spinbox_placeholder);

	QObject::connect(
			d_mt_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_mt_value_changed(int)));
	QObject::connect(
			d_nt_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_nt_value_changed(int)));
	QObject::connect(
			nd_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_nd_value_changed(int)));

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

	d_mt_spinbox->setValue(d_mt);
	d_nt_spinbox->setValue(d_nt);
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

	// Must constrain mt >= nt.
	d_nt_spinbox->setMaximum(mt);
}


void
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::handle_nt_value_changed(
		int nt)
{
	d_nt = nt;

	// Update the number of processors.
	set_num_processors();

	// Must constrain mt >= nt.
	d_mt_spinbox->setMinimum(nt);
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
	d_num_processors = GPlatesAppLogic::GenerateVelocityDomainTerra::calculate_num_processors(d_mt, d_nt, d_nd);

	num_processors_line_edit->setText(QString("%1").arg(d_num_processors));
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
	const std::size_t np_pos = text.toStdString().find(NP_PLACE_HOLDER);

	// Must have at least one occurrence of the 'processor number' placeholder so that
	// each output filename will be different.
	if (text.isEmpty() || np_pos == std::string::npos)
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
	GPlatesModel::ModelInterface model = d_main_window.get_application_state().get_model_interface();

	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(model.access_model());

	// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			d_main_window.get_application_state(), true/*reconstruct_on_scope_exit*/);

	// Loading files will trigger layer additions.
	// As an optimisation (ie, not required), put all layer additions in a single add layers group.
	// It dramatically improves the speed of the Visual Layers dialog when there's many layers.
	GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(
			d_main_window.get_application_state().get_reconstruct_graph());
	add_layers_group.begin_add_or_remove_layers();

	main_buttonbox->setDisabled(true);

	ProgressDialog *progress_dlg = new ProgressDialog(this);
	progress_dlg->setRange(0, d_num_processors);
	progress_dlg->setValue(0);
	progress_dlg->show();

	// Generate the complete Terra grid in memory through recursive subdivision.
	progress_dlg->update_progress(0, tr("Generating Terra grid..."));
	GPlatesAppLogic::GenerateVelocityDomainTerra::Grid grid(d_mt, d_nt, d_nd);

	// Iterate over the Terra processors.
	for (int np = 0; np < d_num_processors; ++np)
	{
		std::stringstream stream;
		stream << tr("Generating domain for Terra processor # ").toStdString() << np << " ...";
		progress_dlg->update_progress(np, stream.str().c_str());

		// Generate the sub-domain points for the current local processor.
		const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_sub_domain =
				grid.get_processor_sub_domain(np);

		// Save to a new file.
		if (!save_velocity_domain_file(velocity_sub_domain, np))
		{
			progress_dlg->close();
			main_buttonbox->setDisabled(false);
			close();
			return;
		}

		if (progress_dlg->canceled())
		{
			progress_dlg->close();
			main_buttonbox->setDisabled(false);
			close();
			return;
		}
	}
	progress_dlg->disable_cancel_button(true);

	// Even with this optimisation, if we are adding say 512 files then the layers dialog
	// can still take a few minutes to update. So just before we that happens we will
	// change the progress bar message to reflect this.
	progress_dlg->update_progress(
			d_num_processors,
			tr("Updating layers dialog - this can take a few minutes if there's more than a hundred files..."));
	add_layers_group.end_add_or_remove_layers();

	main_buttonbox->setDisabled(false);
	progress_dlg->reject();

	accept();
}


bool
GPlatesQtWidgets::GenerateVelocityDomainTerraDialog::save_velocity_domain_file(
		const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_sub_domain,
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

	// Create the geometry property and append to feature.
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("meshPoints"),
				GPlatesPropertyValues::GmlMultiPoint::create(velocity_sub_domain)));

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
	replace_place_holder(file_name, MT_PLACE_HOLDER, mt_stream.str());
	replace_place_holder(file_name, NT_PLACE_HOLDER, nt_stream.str());
	replace_place_holder(file_name, ND_PLACE_HOLDER, nd_stream.str());
	replace_place_holder(file_name, NP_PLACE_HOLDER, np_stream.str());
	file_name = d_path.toStdString() + file_name;

	// Make a new FileInfo object for saving to a new file.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo(file_name.c_str());
	GPlatesFileIO::File::non_null_ptr_type new_file =
			GPlatesFileIO::File::create_file(new_fileinfo, feature_collection);

	// Save the feature collection to a file that is registered with
	// FeatureCollectionFileState (maintains list of all loaded files).
	// This will pop up an error dialog if there's an error.
	return d_main_window.file_io_feedback().create_file(new_file);
}
