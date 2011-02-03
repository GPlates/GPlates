/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute file_iter and/or modify file_iter under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that file_iter will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <boost/cast.hpp>
#include <boost/assign.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QString>

#include "DataAssociationDialog.h"
#include "ResultTableDialog.h"
#include "ProgressDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "data-mining/OpaqueDataToQString.h"
#include "data-mining/PopulateShapeFileAttributesVisitor.h"

#include "file-io/File.h"
#include "file-io/GpmlOnePointSixOutputVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FeatureFocus.h"

#include "presentation/ViewState.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlKeyValueDictionary.h"

#include "maths/Real.h"
#include "model/ModelUtils.h"

using namespace GPlatesQtWidgets;

namespace
{
	void
	find_attr_by_name(
			GPlatesModel::FeatureHandle::weak_ref fh,
			GPlatesModel::PropertyName &name,
			std::vector< const GPlatesModel::TopLevelProperty* >& attrs)
	{
		GPlatesModel::FeatureHandle::iterator it_begin = 
			fh->begin();
		GPlatesModel::FeatureHandle::iterator it_end = 
			fh->end();

		for( ;it_begin != it_end; it_begin++)
		{
			if((*it_begin)->property_name()==name)
			{
				attrs.push_back((*it_begin).get());
			}
		}
	}

	class FeatureCollectionListItem :
		public QListWidgetItem
	{
	public:
		FeatureCollectionListItem(
				const QString &str_):  
			QListWidgetItem(str_)
		{ }
		const GPlatesFileIO::File::Reference* file;
	};

	class FeatureCollectionTableItem : 
		public QTableWidgetItem
	{
	public:
		FeatureCollectionTableItem(
				const QString &str_):
			QTableWidgetItem(str_)
		{ }
		const GPlatesFileIO::File::Reference *file;

	};

	class AttributeItem :
		public QListWidgetItem
	{
	public:
		AttributeItem(
				const QString &str_,
				const GPlatesModel::PropertyName& name_):  
			QListWidgetItem(str_),
			name(name_)
		{ }
		const GPlatesModel::PropertyName name;
	};

	class AttributeTableItem :
		public QTableWidgetItem
	{
	public:
		AttributeTableItem(
				const QString &str_,
				const GPlatesModel::PropertyName& name_):  
			QTableWidgetItem(str_),
			name(name_)
		{ }
		const GPlatesModel::PropertyName name;
	};

};

const QString DataAssociationDialog::DISTANCE = "Distance";
const QString DataAssociationDialog::PRESENCE = "Presence";
const QString DataAssociationDialog::NUM_ROI  = "Number in Region";

CoRegConfigurationTable DataAssociationDialog::CoRegCfgTable;

const DefaultAttributeList 
DataAssociationDialog::d_default_attribute_list = 
	boost::assign::list_of
			(DataAssociationDialog::DISTANCE)
			(DataAssociationDialog::PRESENCE)
			(DataAssociationDialog::NUM_ROI);

GPlatesQtWidgets::DataAssociationDialog::DataAssociationDialog(
		  GPlatesAppLogic::ApplicationState &application_state,
		  GPlatesPresentation::ViewState &view_state,
		  QWidget *parent_):
	QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint | 
			Qt::MSWindowsFixedSizeDialogHint),
	d_progress_dialog(NULL),
	d_button_create(NULL),
	d_feature_collection_file_state(application_state.get_feature_collection_file_state()),
	d_application_state(view_state.get_application_state()),
	d_view_state(view_state),
	d_feature_focus(view_state.get_feature_focus()),
	d_default_ROI_range(0),
	d_start_time(0.0),
	d_end_time(0.0),
	d_time_inc(0.0)
{
	setupUi(this);
	// NOTE: This needs to be done first thing after setupUi() is called.
	d_seed_file_state_seq.table_widget = table_seed_files;

	table_feature_collect_attr->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
	table_seed_files->horizontalHeader()->setResizeMode(0,QHeaderView::Stretch);

	set_up_button_box();
	set_up_seed_files_page();
	set_up_select_attr_page();
	set_up_general_options_page();

	// When the current page is changed, we need to enable and disable some buttons.
	QObject::connect(
			stack_widget, SIGNAL(currentChanged(int)),
			this, SLOT(handle_page_change(int)));

	// Send a fake page change event to ensure buttons are set up properly at start.
	handle_page_change(0);
	react_association_operator_changed(0);
}

void
GPlatesQtWidgets::DataAssociationDialog::pop_up_dialog()
{

	const file_ptr_seq_type loaded_files = get_loaded_files();

	// Setup the partitioning and partitioned file lists in the widget.
	initialise_file_list(d_seed_file_state_seq, loaded_files);
	init_target_collection_list_widget();
	//initialise_file_list(d_selected_file_state_seq, loaded_files);

	// Set the stack back to the first page.
	stack_widget->setCurrentIndex(0);

	show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	raise();
}

void
GPlatesQtWidgets::DataAssociationDialog::init_target_collection_list_widget()
{
	listWidget_target_collection->clear();
	table_feature_collect_attr->clearContents(); 
	table_feature_collect_attr->setRowCount(0);
	
	const file_ptr_seq_type loaded_files = get_loaded_files();

	file_ptr_seq_type::const_iterator it = loaded_files.begin();
	file_ptr_seq_type::const_iterator it_end = loaded_files.end();
	for(; it != it_end; it++)
	{
		QString display_name;

		if (GPlatesFileIO::file_exists((*it)->get_file_info()))
		{
			display_name = (*it)->get_file_info().get_display_name(false);
		}
		else
		{
			// The file doesn't exist so give it a filename to indicate this.
			display_name = "New Feature Collection";
		}

		FeatureCollectionListItem* item = 
			new  FeatureCollectionListItem(display_name);
		listWidget_target_collection->addItem(item); 
	}
}


GPlatesQtWidgets::DataAssociationDialog::file_ptr_seq_type
GPlatesQtWidgets::DataAssociationDialog::get_loaded_files()
{
	//
	// Get a list of all loaded files.
	//
	file_ptr_seq_type loaded_files;

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_file_refs =
			d_feature_collection_file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &loaded_file_ref,
			loaded_file_refs)
	{
		const GPlatesFileIO::File::Reference &loaded_file = loaded_file_ref.get_file();
		loaded_files.push_back(&loaded_file);
	}

	return loaded_files;
}


GPlatesQtWidgets::DataAssociationDialog::feature_collection_seq_type
GPlatesQtWidgets::DataAssociationDialog::get_selected_feature_collections(
		FileStateCollection &file_state_collection)
{
	feature_collection_seq_type selected_feature_collections;

	// Iterate through the files accepted by the user.
	file_state_seq_type::const_iterator file_state_iter =
			file_state_collection.file_state_seq.begin();
	file_state_seq_type::const_iterator file_state_end =
			file_state_collection.file_state_seq.end();
	for ( ; file_state_iter != file_state_end; ++file_state_iter)
	{
		const FileState &file_state = *file_state_iter;

		if (file_state.enabled)
		{
			selected_feature_collections.push_back(
					file_state.file->get_feature_collection());
		}
	}

	return selected_feature_collections;
}

void
GPlatesQtWidgets::DataAssociationDialog::set_up_button_box()
{
	// Default 'OK' button should read 'Apply'.
	d_button_create = buttonbox->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
	d_button_create->setDefault(true);

	QObject::connect(buttonbox, SIGNAL(accepted()),
		this, SLOT(apply()));
	QObject::connect(buttonbox, SIGNAL(rejected()),
		this, SLOT(reject()));

	// Extra buttons for switching between the pages.
	QObject::connect(button_prev, SIGNAL(clicked()),
		this, SLOT(handle_prev()));
	QObject::connect(button_next, SIGNAL(clicked()),
		this, SLOT(handle_next()));

	QObject::connect(apply_layer_configuration_button, SIGNAL(clicked()),
		this, SLOT(apply_layer_configuration()));
}

void
GPlatesQtWidgets::DataAssociationDialog::set_up_seed_files_page()
{
	
	QObject::connect(table_seed_files, SIGNAL(cellChanged(int, int)),
		this, SLOT(react_cell_changed_partitioning_files(int, int)));

	QObject::connect(comboBox_association_op, SIGNAL(activated(int)),
		this, SLOT(react_association_operator_changed(int)));

	groupBox_ROI->setVisible(false);
}

void
GPlatesQtWidgets::DataAssociationDialog::set_up_select_attr_page()
{
	
	QObject::connect(listWidget_target_collection, SIGNAL(itemSelectionChanged()),
		this, SLOT(react_feature_collection_changed()));

	QObject::connect(listWidget_target_collection, SIGNAL(itemClicked(QListWidgetItem *)),
		this, SLOT(react_feature_collection_changed()));

	QObject::connect(pushButton_add, SIGNAL(clicked()),
		this, SLOT(react_add_button_clicked()));

	QObject::connect(comboBox_association_type, SIGNAL(currentIndexChanged(int)),
		this, SLOT(react_combox_association_type_current_index_changed(int)));
}

void
GPlatesQtWidgets::DataAssociationDialog::react_association_operator_changed(
		int index)
{
	switch(index)
	{
	case 1://Feature ID List
		groupBox_ROI->setVisible(false);
		groupBox_FeatureList->setVisible(true);
		d_association_operator = GPlatesDataMining::FEATURE_ID_LIST;
		break;

	case 0://ROI
	default:
		groupBox_ROI->setVisible(false);
		groupBox_FeatureList->setVisible(false);
		d_association_operator = GPlatesDataMining::REGION_OF_INTEREST;
		break;
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::set_up_general_options_page()
{
}

void
GPlatesQtWidgets::DataAssociationDialog::handle_prev()
{
	const int prev_index = stack_widget->currentIndex() - 1;
	if (prev_index >= 0)
	{
		stack_widget->setCurrentIndex(prev_index);
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::handle_next()
{
	const int next_index = stack_widget->currentIndex() + 1;
	if (next_index < stack_widget->count())
	{
		stack_widget->setCurrentIndex(next_index);
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::handle_page_change(
	int page)
{
	// Enable all buttons and then disable buttons appropriately.
	button_prev->setEnabled(true);
	button_next->setEnabled(true);
	d_button_create->setEnabled(false);

	// Disable buttons which are not valid for the page,
	// and focus the first widget.
	switch (page)
	{
	case 0:
		partitioning_files->setFocus();
		button_prev->setEnabled(false);
		d_button_create->setEnabled(false);
		break;

	case 1:
		partitioned_files->setFocus();
		button_next->setEnabled(false);
		if(table_feature_collect_attr->rowCount() > 0)
		{
			d_button_create->setEnabled(true);
		}
		listWidget_attributes->clear();
		//d_default_ROI_range = spinBox_ROI_range->value();
		break;
	
	default:
		break;
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::initialise_file_list(
		FileStateCollection &file_state_collection,
		const file_ptr_seq_type &files)
{
	clear_rows(file_state_collection);

	for (file_ptr_seq_type::const_iterator file_iter = files.begin();
		file_iter != files.end();
		++file_iter)
	{
		add_row(file_state_collection, **file_iter);
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::clear_rows(
		FileStateCollection &file_state_collection)
{
	file_state_collection.file_state_seq.clear();
	file_state_collection.table_widget->clearContents(); // Do not clear the header items as well.
	file_state_collection.table_widget->setRowCount(0);  // Do remove the newly blanked rows.
}

void
GPlatesQtWidgets::DataAssociationDialog::add_row(
		FileStateCollection &file_state_collection,
		const GPlatesFileIO::File::Reference &file)
{
	const GPlatesFileIO::FileInfo &file_info = file.get_file_info();

	// Obtain information from the FileInfo
	const QFileInfo &qfileinfo = file_info.get_qfileinfo();

	// Some files might not actually exist yet if the user created a new
	// feature collection internally and hasn't saved it to file yet.
	QString display_name;
	if (GPlatesFileIO::file_exists(file_info))
	{
		display_name = file_info.get_display_name(false);
	}
	else
	{
		// The file doesn't exist so give it a filename to indicate this.
		display_name = "New Feature Collection";
	}

	const QString filepath_str = qfileinfo.path();

	// The rows in the QTableWidget and our internal file sequence should be in sync.
	const int row = file_state_collection.table_widget->rowCount();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		row == boost::numeric_cast<int>(file_state_collection.file_state_seq.size()),
		GPLATES_ASSERTION_SOURCE);

	// Add a row.
	file_state_collection.table_widget->insertRow(row);
	file_state_collection.file_state_seq.push_back(FileState(file));
	const FileState &row_file_state = file_state_collection.file_state_seq.back();

	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	file_state_collection.table_widget->setItem(row, FILENAME_COLUMN, filename_item);

	//Add checkbox item to enable/disable the file.
	QTableWidgetItem *file_enabled_item = new QTableWidgetItem();
	file_enabled_item->setToolTip(tr("Select to enable file for partitioning"));
	file_enabled_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	file_enabled_item->setCheckState(row_file_state.enabled ? Qt::Checked : Qt::Unchecked);
	file_state_collection.table_widget->setItem(row, ENABLE_FILE_COLUMN, file_enabled_item);
}


void
GPlatesQtWidgets::DataAssociationDialog::clear()
{
	clear_rows(d_seed_file_state_seq);
	d_seed_file_state_seq.file_state_seq.clear();
}


void
GPlatesQtWidgets::DataAssociationDialog::react_cell_changed_partitioning_files(
		int row,
		int column)
{
	react_cell_changed(d_seed_file_state_seq, row, column);
}

void
GPlatesQtWidgets::DataAssociationDialog::react_cell_changed(
		FileStateCollection &file_state_collection,
		int row,
		int column)
{
	if (row < 0 ||
		boost::numeric_cast<file_state_seq_type::size_type>(row) >
		file_state_collection.file_state_seq.size())
	{
		return;
	}

	// It should be the enable file checkbox column as that's the only
	// cell that's editable.
	if (column != ENABLE_FILE_COLUMN)
	{
		return;
	}

	// Set the enable flag in our internal file sequence.
	file_state_collection.file_state_seq[row].enabled =
		file_state_collection.table_widget->item(row, column)->checkState() == Qt::Checked;
}

void
GPlatesQtWidgets::DataAssociationDialog::react_clear_all(
		FileStateCollection &file_state_collection)
{
	for (int row = 0; row < file_state_collection.table_widget->rowCount(); ++row)
	{
		file_state_collection.table_widget->item(row, ENABLE_FILE_COLUMN)
			->setCheckState(Qt::Unchecked);
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::react_select_all(
		FileStateCollection &file_state_collection)
{
	for (int row = 0; row < file_state_collection.table_widget->rowCount(); ++row)
	{
		file_state_collection.table_widget->item(row, ENABLE_FILE_COLUMN)
			->setCheckState(Qt::Checked);
	}
}

void 
GPlatesQtWidgets::DataAssociationDialog::populate_input_table(
		GPlatesDataMining::CoRegConfigurationTable& input_table)
{
	using namespace GPlatesDataMining;

	int row_num = table_feature_collect_attr->rowCount();

	for(int i = 0; i < row_num; i++)
	{
		FeatureCollectionTableItem* feature_collection_item = 
			dynamic_cast< FeatureCollectionTableItem* > (
					table_feature_collect_attr->item(i, FeatureCollectionName) );
		AttributeTableItem* attr_item = 
			dynamic_cast< AttributeTableItem* > (
					table_feature_collect_attr->item(i, AttributesFunctions) );
		QComboBox* data_operator = 
			dynamic_cast< QComboBox* >(
					table_feature_collect_attr->cellWidget(i, DataOperatorCombo) );
		QDoubleSpinBox* spinbox_ROI_range = 
			dynamic_cast< QDoubleSpinBox* >(
					table_feature_collect_attr->cellWidget(i,RegionOfInterestRange) );
		
		if( !(	feature_collection_item &&
				attr_item				&&
				data_operator			&&
				spinbox_ROI_range) )
		{
			qWarning() << "Invalid input table item found! Skip this iteration";
			continue;
		}
		
		QString operator_name = data_operator->currentText();
		DataOperator::DataOperatorNameMap::iterator it = 
			DataOperator::d_data_operator_name_map.find( operator_name );
		
		if(it == DataOperator::d_data_operator_name_map.end())
		{
			qWarning() << "Invalid operator found in input table.";
			continue;
		}
		
		DataOperatorType op = it->second;

		ConfigurationTableRow row;

		row.target_feature_collection_handle = 
				feature_collection_item->file->get_feature_collection();
 
		row.association_operator_type = d_association_operator;
		row.association_parameters.d_ROI_range = spinbox_ROI_range->value();
		row.association_parameters.d_associator_type = d_association_operator;
		row.attribute_name = attr_item->text();
		row.data_operator_type = op;
		//crack code for shapefileAttributes
		if(attr_item->name.get_name() == "shapefileAttributes")
		{
			row.data_operator_parameters.d_is_shape_file_attr = true;
		}
		input_table.push_back(row);
	}
	sort_input_table(input_table);
}

namespace
{
	using namespace GPlatesDataMining;
	bool
	sort_input_table_compare_function(
			ConfigurationTableRow i,
			ConfigurationTableRow j)
	{
		return i.association_parameters.d_ROI_range > j.association_parameters.d_ROI_range;
	}
}

void
DataAssociationDialog::sort_input_table(
		GPlatesDataMining::CoRegConfigurationTable& input_table)
{
	std::sort(
			input_table.begin(), 
			input_table.end(),
			sort_input_table_compare_function);           
}

void
GPlatesQtWidgets::DataAssociationDialog::update_progress_bar(
		time_t time)
{
	//TODO: the progress bar needs more work...
	if(0 == time)
	{
		return;
	}
	d_progress_dialog_counter++;
	unsigned seconds_left = time * (d_progress_dialog_range - d_progress_dialog_counter);
	char tmp[50];
	sprintf(
			tmp,
			"%d hours %d minutes %d seconds left.",
			seconds_left/3600,
			(seconds_left%3600)/60,
			(seconds_left%3600)%60);
	d_progress_dialog->update_progress(
			d_progress_dialog_counter,
			QString(tmp));
}

void
GPlatesQtWidgets::DataAssociationDialog::set_progress_bar_range()
{
	//TODO: the progress bar needs more work...
	unsigned time_slice = 1;
	if((d_start_time > d_end_time) && (d_time_inc > 0))
	{
		time_slice = (d_start_time - d_end_time ) / d_time_inc;
		time_slice++;
	}
	d_progress_dialog_range = time_slice ;
	d_progress_dialog->setRange(0, d_progress_dialog_range);
}

void
GPlatesQtWidgets::DataAssociationDialog::initialise_progress_dialog()
{
	//TODO: the progress bar needs more work...
	d_progress_dialog = new ProgressDialog(this);
	d_progress_dialog_counter = 0;
	d_progress_dialog->update_progress(
			0,
			"Processing....");
	set_progress_bar_range();
	d_progress_dialog->show();
}

void
GPlatesQtWidgets::DataAssociationDialog::destroy_progress_dialog()
{
	d_progress_dialog->reject();
	delete d_progress_dialog;
	d_progress_dialog = NULL;
	d_progress_dialog_counter = 0;
	d_progress_dialog_range = 0;
}

const QString filter(QObject::tr("* (*.*)"));
const QString filter_ext(QObject::tr("*"));

void
GPlatesQtWidgets::DataAssociationDialog::create_data_association_feature()
{
	//TODO: To be finished....
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create(
					d_application_state.get_model_interface()->root());

	static const GPlatesModel::FeatureType feature_type = 
				GPlatesModel::FeatureType::create_gpml("CoRegistration");

	GPlatesModel::FeatureHandle::weak_ref feature = 
			GPlatesModel::FeatureHandle::create(
					feature_collection,
					feature_type);

	GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gml("DataAssociationParameters");
	GPlatesPropertyValues::XsString::non_null_ptr_type data_association_parameters =
			GPlatesPropertyValues::XsString::create("dummy");

	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
					prop_name,
					data_association_parameters));

	GPlatesQtWidgets::SaveFileDialog::filter_list_type filters;
	filters.push_back(FileDialogFilter(filter, filter_ext));

	GPlatesQtWidgets::SaveFileDialog save_dialog(
			this,
			"Save",
			filters,
			d_view_state);
	
	boost::optional<QString> filename_opt = save_dialog.get_file_name();
	if(!filename_opt)
	{
		qDebug() << "No file name.";
		return;
	}
	
	GPlatesFileIO::FileInfo export_file_info(*filename_opt);
	GPlatesFileIO::GpmlOnePointSixOutputVisitor gpml_writer(export_file_info, false);
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection, gpml_writer);
}

void
GPlatesQtWidgets::DataAssociationDialog::apply_layer_configuration()
{
	populate_input_table(CoRegCfgTable);
	clear();
	done(QDialog::Accepted);
}

void
GPlatesQtWidgets::DataAssociationDialog::apply()
{
	using namespace GPlatesDataMining;
	
	d_start_time = doubleSpinBox_start_time->value();
	d_end_time = doubleSpinBox_end_time->value();
	d_time_inc = doubleSpinBox_time_increment->value();

	if(0.0 == GPlatesMaths::Real(d_time_inc))
	{
		d_time_inc = 1;
	}

	feature_collection_seq_type seed_feature_collections = 
			get_selected_feature_collections( 
					d_seed_file_state_seq );

	std::vector< DataTable > result_collection;
	CoRegConfigurationTable matrix;
	populate_input_table(matrix);
	double reconstruct_time = d_start_time;
	initialise_progress_dialog();
	int count = 0;

	for(; reconstruct_time >= d_end_time; reconstruct_time -= d_time_inc, ++count )
	{
		time_t seconds = time(NULL);
		DataTable t_result;
		t_result.set_reconstruction_time(reconstruct_time);
		feature_collection_seq_type::const_iterator it = seed_feature_collections.begin();
		feature_collection_seq_type::const_iterator it_end = seed_feature_collections.end();
		for(; it != it_end; it++)
		{
			const GPlatesAppLogic::Reconstruction* reconstruction = NULL;
			
			if(reconstruct_time > 0)
			{
				d_application_state.set_reconstruction_time( reconstruct_time );
				reconstruction = &(d_application_state.get_current_reconstruction());
			}
			boost::scoped_ptr< DataSelector > selector( DataSelector::create(matrix) );
			selector->select(
					*it,
					reconstruction,
					t_result);
			if(d_progress_dialog->canceled())
			{
				destroy_progress_dialog();
				return;
			}
		}
		result_collection.push_back(t_result);
		update_progress_bar(time(NULL) - seconds);
#ifdef _DEBUG
		std::cout << t_result << std::endl;
#endif
	}
	destroy_progress_dialog();

	clear();
	done(QDialog::Accepted);
	
	d_result_dialog.reset(
			new ResultTableDialog(
				result_collection,
				d_view_state,
				this));
	d_result_dialog->show();
 	d_result_dialog->activateWindow();
 	d_result_dialog->raise();
}

void
GPlatesQtWidgets::DataAssociationDialog::reject()
{
	clear();

	done(QDialog::Rejected);
}

boost::optional< GPlatesModel::FeatureCollectionHandle::const_weak_ref >
GPlatesQtWidgets::DataAssociationDialog::get_feature_collection_by_name(
		QString& name)
{
	const file_ptr_seq_type loaded_files = get_loaded_files();
	file_ptr_seq_type::const_iterator it = loaded_files.begin();
	file_ptr_seq_type::const_iterator it_end = loaded_files.end();
	for(; it != it_end; it++)
	{
		if((*it)->get_file_info().get_display_name(false) == name)
		{
			return boost::optional< GPlatesModel::FeatureCollectionHandle::const_weak_ref > (
					(*it)->get_feature_collection());
		}
	}
	return boost::none;
}

const GPlatesFileIO::File::Reference *
GPlatesQtWidgets::DataAssociationDialog::get_file_by_name(
		QString& name)
{
	const file_ptr_seq_type loaded_files = get_loaded_files();
	file_ptr_seq_type::const_iterator it = loaded_files.begin();
	file_ptr_seq_type::const_iterator it_end = loaded_files.end();
	for(; it != it_end; it++)
	{
		const GPlatesFileIO::File::Reference* const_file = *it;
		if(const_file->get_file_info().get_display_name(false) == name)
		{
			return (*it);
		}
	}
	return NULL;
}

void
GPlatesQtWidgets::DataAssociationDialog::add_shape_file_attrs( 
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection,
		const GPlatesModel::PropertyName& _property_name,
		QListWidget *_listWidget_attributes)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator it = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator it_end = feature_collection->end();
	std::set< QString > shape_attr_set;
	for(; it != it_end; it++)
	{
		GPlatesDataMining::PopulateShapeFileAttributesVisitor visitor;
		visitor.visit_feature((*it)->reference());
		std::vector< QString > attr_names =
				visitor.get_shape_file_attr_names();
		std::vector< QString >::const_iterator inner_it = attr_names.begin();
		std::vector< QString >::const_iterator inner_it_end = attr_names.end();
		for(; inner_it != inner_it_end; inner_it++)
		{
			shape_attr_set.insert(*inner_it);
		}
	}

	std::set< QString >::const_iterator set_it = shape_attr_set.begin();
	std::set< QString >::const_iterator set_it_end = shape_attr_set.end();
	for(; set_it != set_it_end; set_it++)
	{
		QListWidgetItem *item = 
				new AttributeItem(
						*set_it,
						_property_name);
		_listWidget_attributes->addItem(item);
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::add_default_attributes()
{
	using namespace GPlatesDataMining;
	DefaultAttributeList::const_iterator it = 
		d_default_attribute_list.begin();
	DefaultAttributeList::const_iterator it_end = 
		d_default_attribute_list.end();
	for(; it != it_end; it++)
	{
		QListWidgetItem *item = 
			new AttributeItem(
					*it, 
					GPlatesModel::PropertyName::create_gpml(*it));
		listWidget_attributes->addItem(item);
	}
}

void
GPlatesQtWidgets::DataAssociationDialog::react_feature_collection_changed()
{
	listWidget_attributes->clear();
	react_combox_association_type_current_index_changed(0);
	comboBox_association_type->setCurrentIndex(0);
}

void
GPlatesQtWidgets::DataAssociationDialog::setup_operator_combobox(
		const QString& attribute_name,	
		QComboBox* combo)
{
	//the following code needs to be refined in the future
	if(attribute_name == DISTANCE)
	{
		combo->addItem("Min Distance");
		combo->addItem("Max Distance");
		combo->addItem("Mean Distance");
		return;
	}
	else if(attribute_name == PRESENCE)
	{
		combo->addItem("Presence");
		return;
	}
	else if(attribute_name == NUM_ROI)
	{
		combo->addItem("NumberInROI");
		return;
	}
	AttributeTypeEnum a_type = Unknown_Type;
	if(d_attr_name_type_map.find(attribute_name) != d_attr_name_type_map.end())
	{
		a_type = d_attr_name_type_map.find(attribute_name)->second;
	}

	if(String_Attribute == a_type)
	{
		combo->addItem("Lookup");
		combo->addItem("Vote");
		return;
	}

	if(Number_Attribute == a_type)
	{
		combo->addItem("Lookup");
		combo->addItem("Min");
		combo->addItem("Max");
		combo->addItem("Mean");
		combo->addItem("Median");
		return;
	}

	GPlatesDataMining::DataOperator::DataOperatorNameMap::const_iterator it =
		GPlatesDataMining::DataOperator::d_data_operator_name_map.begin(); 
	GPlatesDataMining::DataOperator::DataOperatorNameMap::const_iterator it_end =
		GPlatesDataMining::DataOperator::d_data_operator_name_map.end(); 
	for(; it != it_end; it++)
	{
		combo->addItem(it->first);
	}

	return;
}


void
GPlatesQtWidgets::DataAssociationDialog::react_add_button_clicked()
{
	if( !(listWidget_target_collection->currentItem() && listWidget_attributes->currentItem()))
	{
		return;
	}
	int row_num=table_feature_collect_attr->rowCount();
	table_feature_collect_attr->insertRow(row_num);

	//attributes/function 
	AttributeItem* attr_item = 
			dynamic_cast< AttributeItem* >( listWidget_attributes->currentItem() );
	if(attr_item)
	{
		AttributeTableItem* item = 
				new AttributeTableItem(
						listWidget_attributes->currentItem()->text(),
						attr_item->name);
		table_feature_collect_attr->setItem(
				row_num, 
				AttributesFunctions, 
				item);
	}

	//operator combobox
	QComboBox* combo = new QComboBox();       
    table_feature_collect_attr->setCellWidget(
			row_num, 
			DataOperatorCombo, 
			combo);
	setup_operator_combobox(
			listWidget_attributes->currentItem()->text(),
			combo);

	//feature collection name
	QString fc_name = listWidget_target_collection->currentItem()->text();
	FeatureCollectionTableItem *fc_item = 
			new FeatureCollectionTableItem( fc_name );
	fc_item->file = get_file_by_name(fc_name);
	table_feature_collect_attr->setItem(
			row_num,
			FeatureCollectionName,
			fc_item);

	//Association Type
	QTableWidgetItem* association_type_item = new QTableWidgetItem("Association Type");
	table_feature_collect_attr->setItem(
			row_num,
			AssociationType,
			association_type_item);

	//Region of interest range
	QDoubleSpinBox* ROI_range_spinbox = new QDoubleSpinBox(); 
	QObject::connect(
			ROI_range_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_default_ROI_range_changed(double)));
	ROI_range_spinbox->setRange(0,25000);
	ROI_range_spinbox->setValue(d_default_ROI_range);
    table_feature_collect_attr->setCellWidget(
			row_num, 
			RegionOfInterestRange, 
			ROI_range_spinbox);
	
	d_button_create->setEnabled(true);
}

void
DataAssociationDialog::react_combox_association_type_current_index_changed(
				int idx)
{
	listWidget_attributes->clear();
	QString fc_name = listWidget_target_collection->currentItem()->text();
	boost::optional< GPlatesModel::FeatureCollectionHandle::const_weak_ref > feature_collection_ref = 
		get_feature_collection_by_name(fc_name);
	if(!feature_collection_ref)
	{
		return;
	}
	if(Relational == idx)
	{
		add_default_attributes();
	}
	else if(Coregistration == idx)
	{
		std::set<GPlatesModel::PropertyName> attr_names;
		get_unique_attribute_names(
				*feature_collection_ref, 
				attr_names);
		
		std::set< GPlatesModel::PropertyName >::const_iterator it = attr_names.begin();
		std::set< GPlatesModel::PropertyName >::const_iterator it_end = attr_names.end();

		for(; it != it_end; it++)
		{
			QListWidgetItem *item = 
				new AttributeItem(
						GPlatesUtils::make_qstring_from_icu_string((*it).get_name()),
						*it);

			if(GPlatesUtils::make_qstring_from_icu_string((*it).get_name()) == "shapefileAttributes")
			{
				add_shape_file_attrs(
						*feature_collection_ref,
						*it,
						listWidget_attributes);
			}
			else
			{
				listWidget_attributes->addItem(item);
			}
		}
	}
	else
	{
		return;
	}
}

void
DataAssociationDialog::get_unique_attribute_names(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref,
		std::set< GPlatesModel::PropertyName > &names)
{
	using namespace GPlatesDataMining;
	GPlatesModel::FeatureCollectionHandle::const_iterator it_begin = 
		feature_collection_ref->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator it_end = 
		feature_collection_ref->end();

	for( ; it_begin != it_end; it_begin++)
	{
		GPlatesModel::FeatureHandle::const_iterator fh_it_begin = 
			(*it_begin)->begin();
		GPlatesModel::FeatureHandle::const_iterator fh_it_end = 
			(*it_begin)->end();

		for( ; fh_it_begin != fh_it_end; fh_it_begin++)
		{
			GPlatesModel::PropertyName name = (*fh_it_begin)->property_name();
			names.insert( name );

			CheckAttrTypeVisitor visitor;
			(*fh_it_begin)->accept_visitor(visitor);
			//hacking code for shape file.
			using namespace GPlatesPropertyValues;
			if(GPlatesUtils::make_qstring_from_icu_string(name.get_name()) == "shapefileAttributes")
			{
				d_attr_name_type_map.insert(
						visitor.shape_map().begin(),
						visitor.shape_map().end());
				
			}
			else
			{
				QString q_name = GPlatesUtils::make_qstring_from_icu_string(name.get_name());
				d_attr_name_type_map.insert(
						std::pair< QString, AttributeTypeEnum >(
								q_name,visitor.type() ) );
			}
		}
	}
}



