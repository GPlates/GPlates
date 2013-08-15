/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVariant>

#include "app-logic/TRSUtils.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"
#include "model/TopLevelProperty.h"
#include "model/TopLevelPropertyInline.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlFiniteRotationSlerp.h"

#include "TotalReconstructionSequencesDialog.h"
#include "EditTableActionWidget.h"
#include "EditTotalReconstructionSequenceWidget.h"

Q_DECLARE_METATYPE( boost::optional<GPlatesPropertyValues::GpmlTimeSample> )

namespace ColumnNames
{
	enum ColumnName
	{
		TIME = 0,
		LATITUDE,
		LONGITUDE,
		ANGLE,
		COMMENT,
		ACTIONS,

		NUMCOLS // Should always be last.
	};
}

namespace
{

	/**
	 * Borrowed from the TopologySectionsTable. Will try using it for itemChanged here. 
	 *
	 * Tiny convenience class to help suppress the @a QTableWidget::cellChanged()
	 * notification in situations where we are updating the table data
	 * programatically. This allows @a react_cell_changed() to differentiate
	 * between changes made by us, and changes made by the user.
	 *
	 * For it to work properly, you must declare one in any TopologySectionsTable
	 * method that directly mucks with table cell data.
	 */
	struct TableUpdateGuard :
			public boost::noncopyable
	{
		TableUpdateGuard(
				bool &guard_flag_ref):
			d_guard_flag_ptr(&guard_flag_ref)
		{
			// Nesting these guards is an error.
			Q_ASSERT(*d_guard_flag_ptr == false);
			*d_guard_flag_ptr = true;
		}
		
		~TableUpdateGuard()
		{
			*d_guard_flag_ptr = false;
		}
		
		bool *d_guard_flag_ptr;
	};

	

	void
	fill_table_with_comment(
		QTableWidget *table,
		unsigned int row_count,
		const QString &comment)
	{
		QTableWidgetItem *comment_item = new QTableWidgetItem;
		comment_item->setText(comment);

		comment_item->setFlags(comment_item->flags() | Qt::ItemIsEditable);
		table->setItem(row_count,ColumnNames::COMMENT,comment_item);
	}

	void
	fill_table_with_finite_rotation(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesPropertyValues::GpmlFiniteRotation &finite_rotation,
		const QLocale &locale_)
	{
		QTableWidgetItem *lat_item = new QTableWidgetItem();
		QTableWidgetItem *lon_item = new QTableWidgetItem();
		QTableWidgetItem *angle_item = new QTableWidgetItem();

		const GPlatesMaths::FiniteRotation &fr = finite_rotation.finite_rotation();
		const GPlatesMaths::UnitQuaternion3D &uq = fr.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			// It's an identity rotation (ie, a rotation of angle == 0.0), so there's
			// no determinate axis of rotation.
			static const double zero_angle = 0.0;

			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString indeterm_tr_str =
				QObject::tr(
				"indet");

			lat_item->setText(indeterm_tr_str);
			lon_item->setText(indeterm_tr_str);
			angle_item->setText(locale_.toString(zero_angle));
		} else {
			// There is a well-defined axis of rotation and a non-zero angle.
			using namespace GPlatesMaths;

			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());
			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);
			double angle = convert_rad_to_deg(params.angle).dval();

			lat_item->setText(locale_.toString(llp.latitude()));
			lon_item->setText(locale_.toString(llp.longitude()));
			angle_item->setText(locale_.toString(angle));
		}

		lat_item->setFlags(lat_item->flags() | Qt::ItemIsEditable);
		lon_item->setFlags(lon_item->flags() | Qt::ItemIsEditable);
		angle_item->setFlags(angle_item->flags() | Qt::ItemIsEditable);

		table->setItem(row_count,ColumnNames::LATITUDE,lat_item);
		table->setItem(row_count,ColumnNames::LONGITUDE,lon_item);			
		table->setItem(row_count,ColumnNames::ANGLE,angle_item);
	}

	void
	fill_table_with_pole(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type &time_sample_value,
		const QLocale &locale_)
	{
		using namespace GPlatesPropertyValues;

		const GpmlFiniteRotation *finite_rotation =
			dynamic_cast<const GpmlFiniteRotation *>(time_sample_value.get());
		if (finite_rotation) {
			// OK, so we definitely have a FiniteRotation.  Now we have to determine
			// whether it's an identity rotation or a rotation with a well-defined axis.
			fill_table_with_finite_rotation(table, row_count, *finite_rotation, locale_);
		} else {
			// The value of the TimeSample was NOT a FiniteRotation as it should
			// have been.  Hence, we can only display an error message in place of
			// the rotation.
			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString not_found =
				QObject::tr(
				"x");
			QTableWidgetItem *lat_item = new QTableWidgetItem();
			QTableWidgetItem *lon_item = new QTableWidgetItem();
			QTableWidgetItem *angle_item = new QTableWidgetItem();
			lat_item->setText(not_found);
			lon_item->setText(not_found);
			angle_item->setText(not_found);
			
			table->setItem(row_count,ColumnNames::LATITUDE,lat_item);
			table->setItem(row_count,ColumnNames::LONGITUDE,lon_item);			
			table->setItem(row_count,ColumnNames::ANGLE,angle_item);
		}
	}

	void
	fill_table_with_time_instant(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant,
		const QLocale &locale_)
	{
		QTableWidgetItem *item = new QTableWidgetItem();
		if (geo_time_instant.is_real()) {
			// Use setData here so that the table can be sorted numerically by the time column. 
			item->setData(Qt::DisplayRole,geo_time_instant.value());
		} else {
			// This is a string to display if the geo-time instant is in either
			// the distant past or distant future (which it should not be).
			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString invalid_time =
				QObject::tr(
				"invalid time");
			item->setData(Qt::DisplayRole,invalid_time);

		}
		table->setItem(row_count,ColumnNames::TIME,item);
	}

	/**
	 * Fill row @a row_count in the QTableWidget @a table with the time,lat,lon,angle and comment from the
	 * GpmlTimeSample @a time_sample.
	 */
	void
	insert_table_row(
			QTableWidget *table,
			unsigned int row_count,
			const GPlatesPropertyValues::GpmlTimeSample &time_sample,
			const QLocale &locale_)
	{
		table->insertRow(row_count);
		fill_table_with_time_instant(table,row_count,time_sample.valid_time()->time_position(),locale_);
		
		fill_table_with_pole(table,row_count,time_sample.value(),locale_);

		QString comment;
		if (time_sample.description())
		{
			comment = GPlatesUtils::make_qstring_from_icu_string(
				time_sample.description()->value().get());
		}
		fill_table_with_comment(table, row_count, comment);
		
		if(time_sample.is_disabled())
		{
			for (int i = 0; i < table->horizontalHeader()->count()-1; i++)
			{
				table->item(row_count, i)->setData(Qt::BackgroundRole, Qt::gray);
			}
		}
		QVariant qv;
		qv.setValue(boost::optional<GPlatesPropertyValues::GpmlTimeSample>(time_sample));
		table->item(row_count, ColumnNames::TIME)->setData(Qt::UserRole,qv);
	}


	/**
	 * Set appropriate limits for the spinbox according to its column - e.g. -90 to 90 for latitude.                                                                     
	 */
	void
	set_spinbox_properties(
		QDoubleSpinBox *spinbox,
		int column)
	{
		switch(column){
			case ColumnNames::TIME:
				spinbox->setMinimum(0.);
				spinbox->setMaximum(1000.);
				break;
			case ColumnNames::LATITUDE:
				spinbox->setMinimum(-90.);
				spinbox->setMaximum(90.);
				break;
			case ColumnNames::LONGITUDE:
				spinbox->setMinimum(-360.);
				spinbox->setMaximum(360.);
				break;
			case ColumnNames::ANGLE:
				spinbox->setMinimum(-360.);
				spinbox->setMaximum(360.);
				break;
		}
        spinbox->setDecimals(4);
	}

	/**
	 * Commit any spinbox widget value from the most recently spinbox-ified cell to the table.                                                                    
	 */
	void
	update_table_from_last_active_cell(
		QTableWidget *table)
	{
		int row = table->currentRow();
		int column = table->currentColumn();
		
		if ((column >= ColumnNames::TIME) && (column <= ColumnNames::ANGLE))
		{
			QWidget *widget = table->cellWidget(row,column);
			if (widget)
			{
				QTableWidgetItem *item = new QTableWidgetItem();
				QVariant variant = static_cast<QDoubleSpinBox*>(widget)->value();
				item->setData(Qt::DisplayRole,variant.toDouble());
				table->setItem(row,column,item);
			}

		}
	}

	void
	fill_row_with_defaults(
		QTableWidget *table,
		int row)
	{

		QTableWidgetItem *time_item = new QTableWidgetItem();
		time_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *lat_item = new QTableWidgetItem();
		lat_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *lon_item = new QTableWidgetItem();
		lon_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *angle_item = new QTableWidgetItem();
		angle_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *comment_item = new QTableWidgetItem();
		comment_item->setText(QString());

		table->setItem(row,ColumnNames::TIME,time_item);
		table->setItem(row,ColumnNames::LATITUDE,lat_item);
		table->setItem(row,ColumnNames::LONGITUDE,lon_item);
		table->setItem(row,ColumnNames::ANGLE,angle_item);
		table->setItem(row,ColumnNames::COMMENT,comment_item);
	}

	/**
	 * Returns true if the time values (i.e. values in ColumnNames::Time of @a table):
	 *   1) are not empty AND
	 *   2) do not contain duplicate times.
	 */ 
	bool
	table_times_are_valid(
		QTableWidget *table)
	{
		std::vector<double> times;
		for (int i=0; i < table->rowCount(); ++i)
		{
			bool ok;
			QTableWidgetItem *item = table->item(i,ColumnNames::TIME);
			if (!item)
			{
				continue;
			}

			//The disabled poles should not count.
			QVariant qv = item->data(Qt::UserRole);
			using namespace GPlatesPropertyValues;
			boost::optional<GpmlTimeSample> sample = qv.value<boost::optional<GpmlTimeSample> >();
			if(sample && sample->is_disabled())
			{
				continue;
			}

			// The item text should have been derived from a spinbox, but check we
			// have a double anyway.
			double time = table->item(i,ColumnNames::TIME)->text().toDouble(&ok);
			if (!ok)
			{
				return false;
			}
			if (std::find(times.begin(),times.end(),time) != times.end())
			{
				return false;
			}
			times.push_back(time);
		}
		return (!times.empty());
	}

	/**
	 * Changes any of the lat/lon fields in row @a row to "indet" if their corresponding angle field is zero.                                                                     
	 */
	void
	set_indeterminate_fields_for_row(
		QTableWidget *table,
		int row)
	{
		// Make sure we have a valid QTableWidgetItem first.
		QTableWidgetItem *item = table->item(row,ColumnNames::ANGLE);
		if (!item)
		{
			return;
		}
		double angle = table->item(row,ColumnNames::ANGLE)->text().toDouble();
		if (GPlatesMaths::are_almost_exactly_equal(angle,0.))
		{
			QTableWidgetItem *indet_lat_item = new QTableWidgetItem();
			indet_lat_item->setText(QObject::tr("indet"));

			QTableWidgetItem *indet_lon_item = new QTableWidgetItem();
			indet_lon_item->setText(QObject::tr("indet"));

			table->setItem(row,ColumnNames::LATITUDE,indet_lat_item);
			table->setItem(row,ColumnNames::LONGITUDE,indet_lon_item);			
		}
	}

	/**
	 * Changes any of the lat/lon fields in @a table to "indet" if their corresponding angle fields are zero.                                                                     
	 */
	void
	set_indeterminate_fields_for_table(
		QTableWidget *table)
	{
		for (int i = 0; i < table->rowCount() ; ++i)
		{
			set_indeterminate_fields_for_row(table,i);
		}
	}
	
}


GPlatesQtWidgets::EditPoleActionWidget::EditPoleActionWidget(
		EditTableWidget *table_widget,
		bool enable_is_on,
		QWidget *parent_ ):
	EditTableActionWidget(table_widget,parent_),
	d_enable_is_on(enable_is_on)
{
	resize(144, 34);
	disable_button = new QPushButton(this);
	disable_button->setObjectName(QString::fromUtf8("button_disable"));
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/disable_22.png"), QSize(), QIcon::Normal, QIcon::Off);
	disable_button->setIcon(icon);
	disable_button->setIconSize(QSize(22, 22));
	disable_button->setFlat(false);
	hboxLayout->addWidget(disable_button);

	enable_button = new QPushButton(this);
	enable_button->setObjectName(QString::fromUtf8("button_enable"));
	icon.addFile(QString::fromUtf8(":/enable_22.png"), QSize(), QIcon::Normal, QIcon::Off);
	enable_button->setIcon(icon);
	enable_button->setIconSize(QSize(22, 22));
	enable_button->setFlat(false);
	hboxLayout->addWidget(enable_button);

	icon.addFile(QString::fromUtf8(":/gnome_edit_delete_22.png"), QSize(), QIcon::Normal, QIcon::Off);
	button_delete->setIcon(icon);

#ifndef QT_NO_TOOLTIP
	disable_button->setToolTip(
			QApplication::translate(
					"EditPoleActionWidget", 
					"Disable the pole", 
					0, 
					QApplication::UnicodeUTF8));
	enable_button->setToolTip(
			QApplication::translate(
					"EditPoleActionWidget", 
					"Enable the pole", 
					0, 
					QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
	disable_button->setText(QString());
	enable_button->setText(QString());
	refresh_buttons();
	QObject::connect(enable_button, SIGNAL(clicked()), this, SLOT(enable_pole()));
	QObject::connect(disable_button, SIGNAL(clicked()), this, SLOT(disable_pole()));
}


void
GPlatesQtWidgets::EditPoleActionWidget::enable_pole()
{
	EditTotalReconstructionSequenceWidget *edit_widget = 
		dynamic_cast<EditTotalReconstructionSequenceWidget *>(d_table_widget_ptr);
	if(edit_widget)
	{
		edit_widget->handle_disable_pole(this, false);
	}
}


void
GPlatesQtWidgets::EditPoleActionWidget::disable_pole()
{
	EditTotalReconstructionSequenceWidget *edit_widget = 
		dynamic_cast<EditTotalReconstructionSequenceWidget *>(d_table_widget_ptr);
	if(edit_widget)
	{
		edit_widget->handle_disable_pole(this, true);
	}
}


GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::EditTotalReconstructionSequenceWidget(
	QWidget *parent_):
		QWidget(parent_),
		EditTableWidget(),
		d_suppress_update_notification_guard(false),
		d_spinbox_row(0),
		d_spinbox_column(0),
		d_moving_plate_changed(false),
		d_fixed_plate_changed(false),
		d_is_grot(false)
{
	setupUi(this);

	// For setting minimum sizes.
	EditPoleActionWidget dummy(this,NULL);
	table_sequences->horizontalHeader()->setResizeMode(ColumnNames::COMMENT,QHeaderView::Stretch);
	table_sequences->horizontalHeader()->setResizeMode(ColumnNames::ACTIONS,QHeaderView::Fixed);
	table_sequences->horizontalHeader()->resizeSection(ColumnNames::ACTIONS,dummy.width());
	table_sequences->verticalHeader()->setDefaultSectionSize(dummy.height());

	// FIXME: In addition to any text in label_validation, 
	// consider displaying some kind of warning icon as well. 
	label_validation->setText("");

	// Experiment with signals from cells. 
	// FIXME: remember to remove any experimental / unneeded signal connections. 
	QObject::connect(
		table_sequences,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(handle_item_changed(QTableWidgetItem*)));
	QObject::connect(
		button_insert,SIGNAL(pressed()),this,SLOT(handle_insert_new_pole()));
	QObject::connect(
		table_sequences,SIGNAL(currentCellChanged(int,int,int,int)),
		this,SLOT(handle_current_cell_changed(int,int,int,int)));
	QObject::connect(
		spinbox_moving,SIGNAL(valueChanged(int)),
		this,SLOT(handle_plate_ids_changed()));


	QObject::connect(
		spinbox_fixed,SIGNAL(valueChanged(int)),
		this,SLOT(handle_plate_ids_changed()));

	table_sequences->setRowCount(0);

	
	label_validation->setStyleSheet("QLabel {color: red;}");
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::update_table_widget_from_property(
	GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type irreg_sampling)
{
	TableUpdateGuard guard(d_suppress_update_notification_guard);

	// We use this to express floating-point values (the TimeSample time positions)
	// in the correct format for this locale.
	QLocale locale_;

	using namespace GPlatesPropertyValues;

	// Note that this is clearContents() and not clear() - calling clear() will also clear the header text (which has
	// been set up in QtDesigner) resulting in only numerical headers appearing. 
	table_sequences->clearContents();
	std::vector<GpmlTimeSample>::const_iterator iter =
		irreg_sampling->time_samples().begin();
	std::vector<GpmlTimeSample>::const_iterator end =
		irreg_sampling->time_samples().end();

	for ( ; iter != end; ++iter) 
	{
		if(dynamic_cast<const GpmlTotalReconstructionPole*>(iter->value().get()))
		{
			break;
		}
	}
	if(iter != end)
	{
		d_is_grot = true;
		table_sequences->hideColumn(ColumnNames::COMMENT);
	}
	iter = irreg_sampling->time_samples().begin();
	table_sequences->setRowCount(0);
	unsigned int row_count = 0;

	for ( ; iter != end; ++iter, ++row_count) {
		insert_table_row(table_sequences,row_count,*iter,locale_);
	}
	table_sequences->setRowCount(row_count);

    set_indeterminate_fields_for_table(table_sequences);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_item_changed(
	QTableWidgetItem *item)
{
	if (d_suppress_update_notification_guard)
	{
		return;
	}

    validate();
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_insert_row_above(
	const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_row(row);
	}
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_insert_row_below(
	const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_row(row+1);
	}
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_delete_row(
	const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		delete_row(row);
		set_action_widget_in_row(row);
	}
	validate();
}

GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::get_irregular_sampling_property_value_from_table_widget()
{
	update_table_from_last_active_cell(table_sequences);
	return make_irregular_sampling_from_table();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_insert_new_pole()
{
	using namespace GPlatesPropertyValues;
	using namespace GPlatesModel;

	static QLocale locale_;
	ModelUtils::TotalReconstructionPole trs_pole;
	trs_pole.time =  spinbox_time->value();; 
	trs_pole.lat_of_euler_pole = spinbox_lat->value();
	trs_pole.lon_of_euler_pole = spinbox_lon->value();
	trs_pole.rotation_angle =spinbox_angle->value();
	trs_pole.comment = lineedit_comment->text();
	
	GpmlTimeSample time_sample = ModelUtils::create_gml_time_sample(trs_pole, d_is_grot); 

	insert_table_row(table_sequences,table_sequences->rowCount(),time_sample,locale_);
	if (table_sequences->rowCount() > 0)
	{
		set_action_widget_in_row(table_sequences->rowCount()-1);
	}

	table_sequences->sortItems(ColumnNames::TIME);
    set_indeterminate_fields_for_table(table_sequences);
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_current_cell_changed(
		int current_row, int current_column, int previous_row, int previous_column)
{

	static QLocale locale_;

	// Move the action widget to the current row.
	if ((current_row != previous_row) && current_row >=0)
	{
		set_action_widget_in_row(current_row);
	}

	// Remove the spinbox from the previous cell. The value from the previous cell should
	// have been committed to the table in the "editingFinished()" method.
	if ((previous_column >= ColumnNames::TIME) && (previous_column <= ColumnNames::ANGLE))
	{
		table_sequences->removeCellWidget(previous_row,previous_column);
	}

	// Put a new spinbox in the current cell, and set it up with the value in the cell. 
	// The table will take ownership of the spinbox widget.
	if ((current_column >= ColumnNames::TIME) && (current_column <= ColumnNames::ANGLE))
	{
		QDoubleSpinBox *spinbox = new QDoubleSpinBox();
		table_sequences->setCellWidget(current_row,current_column,spinbox);
		d_spinbox_column = current_column;
		d_spinbox_row = current_row;
		QTableWidgetItem *current_item = table_sequences->item(current_row,current_column);
		if (current_item)
		{
			bool ok;
			double cell_value;
			current_item->data(Qt::DisplayRole).toDouble(&ok);
			if (ok)
			{
				cell_value = (current_item->data(Qt::DisplayRole)).toDouble();
			}
			else
			{
				cell_value = 0.;
			}

			set_spinbox_properties(spinbox,current_column);
			spinbox->setValue(cell_value);

			QObject::connect(spinbox,SIGNAL(editingFinished()),
				this,SLOT(handle_editing_finished()));
        }
	}

}

int
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::get_row_for_action_widget(
	const EditTableActionWidget *action_widget)
{
	for (int i = 0; i < table_sequences->rowCount(); ++i)
	{
		if (table_sequences->cellWidget(i, ColumnNames::ACTIONS) == action_widget) {
			return i;
		}
	}
	return -1;
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::insert_blank_row(
		int row)
{
	// Insert a new blank row.
	table_sequences->insertRow(row);

	fill_row_with_defaults(table_sequences,row);

	// Not yet sure if the work-around used in other Edit... widgets which involve tables 
	// is necessary in the EditTRSWidget....

	// Open up an editor for the first time field.
	QTableWidgetItem *time_item = table_sequences->item(row, ColumnNames::TIME);
	QVariant qv;
	GPlatesModel::ModelUtils::TotalReconstructionPole trs_pole;
	trs_pole.time = 0; 
	trs_pole.lat_of_euler_pole = 0; 
	trs_pole.lon_of_euler_pole = 0; 
	trs_pole.rotation_angle = 0;
	qv.setValue( 
			boost::optional<GPlatesPropertyValues::GpmlTimeSample>(
					GPlatesModel::ModelUtils::create_gml_time_sample(trs_pole, d_is_grot)));
	time_item->setData(Qt::UserRole, qv);
	if (time_item != NULL) {
		table_sequences->setCurrentItem(time_item);
		table_sequences->editItem(time_item);
	}
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::delete_row(
	int row)
{
	// Before we delete the row, delete the action widget. removeRow() messes with the previous/current
	// row indices, and then calls handle_current_cell_changed, which cannot delete the old action widget, 
	// the upshot being that we end up with a surplus action widget which we can't get rid of. 
	table_sequences->removeCellWidget(row,ColumnNames::ACTIONS);
	// Delete the given row.
	table_sequences->removeRow(row);

	// May need the glitch work-around here too.
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::sort_table_by_time()
{
	update_table_from_last_active_cell(table_sequences);
	table_sequences->sortItems(ColumnNames::TIME);
}

bool
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::validate()
{
	// Until we have a mechanism for enabling/disabling poles and sequences, disallow editing/creation of
	// sequences with 999 plate-ids. 

    bool times_valid = false;
    bool plates_valid = false;

	if (table_sequences->rowCount() == 0)
	{
        times_valid = false;
		label_validation->setText(QObject::tr("No values in table.\nTo delete a sequence use the Delete Sequence button in the Total Reconstruction Seqeunce Dialog."));
	}
	else
	{
        times_valid = table_times_are_valid(table_sequences);
        if (times_valid)
		{
			label_validation->setText("");
		}
		else
		{
			label_validation->setText(QObject::tr("Table contains samples with equal time instants."));
		}
	}
    plates_valid = ((spinbox_moving->value() != 999) && (spinbox_fixed->value() != 999));
    if (!plates_valid)
    {
        // This will over-write any time-related feedback. But once any plate-id related issues are fixed,
        // the table goes through validation again, and so any time-related feedback will appear.
        label_validation->setText(QObject::tr("Plate ids of 999 not currently supported in creation/editing."));
    }

	// This signal can be picked up for example by the parent Edit... and Create... 
	// dialogs to update their Apply/Create buttons. 
    Q_EMIT table_validity_changed(times_valid && plates_valid);

    return (times_valid && plates_valid);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_editing_finished()
{
#if 0
	qDebug();
	qDebug() << "spinbox editing finished";
	qDebug() << "Current table row: " << table_sequences->currentRow();
	qDebug() << "Current table column: " << table_sequences->currentColumn();
	qDebug() << "Spinbox row " << d_spinbox_row;
	qDebug() << "Spinbox column" << d_spinbox_column;
	qDebug();
#endif
	QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(table_sequences->cellWidget(d_spinbox_row,d_spinbox_column));
	if (spinbox)
	{
		double spinbox_value = spinbox->value();
		QTableWidgetItem *item = table_sequences->item(d_spinbox_row,d_spinbox_column);
		item->setData(Qt::DisplayRole,spinbox_value);

		if (d_spinbox_column == ColumnNames::TIME)
		{
			table_sequences->sortItems(ColumnNames::TIME);
			validate();
		}

		if (d_spinbox_column == ColumnNames::ANGLE)
		{
			set_indeterminate_fields_for_row(table_sequences,d_spinbox_row);
		}
	}
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::set_fixed_plate_id(
        const GPlatesModel::integer_plate_id_type &fixed_plate_id_)
{
        spinbox_fixed->setValue(fixed_plate_id_);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::set_moving_plate_id(
        const GPlatesModel::integer_plate_id_type &moving_plate_id_)
{
        spinbox_moving->setValue(moving_plate_id_);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_plate_ids_changed()
{
    if (validate())
    {
        Q_EMIT plate_ids_have_changed();
    }
}

GPlatesModel::integer_plate_id_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::moving_plate_id() const
{
	return spinbox_moving->value();
}

GPlatesModel::integer_plate_id_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::fixed_plate_id() const
{
	return spinbox_fixed->value();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::initialise()
{
	TableUpdateGuard guard(d_suppress_update_notification_guard);

    table_sequences->clearContents();
    table_sequences->setRowCount(0);
    insert_blank_row(0);
    spinbox_moving->setValue(0);
    spinbox_fixed->setValue(0);
	validate();
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::set_action_widget_in_row(
		int row)
{
	if (row < 0 )
	{
		return;
	}

	if (row >= table_sequences->rowCount())
	{
		row = table_sequences->rowCount() - 1;
	}

	// Remove any existing action widget.
	for (int i = 0; i < table_sequences->rowCount() ; ++i)
	{
		if (table_sequences->cellWidget(i,ColumnNames::ACTIONS))
		{
			table_sequences->removeCellWidget(i,ColumnNames::ACTIONS);
		}
	}

	// Insert action widget in desired row.
	bool enable_flag = false;
	QVariant qv = table_sequences->item(row,ColumnNames::TIME)->data(Qt::UserRole);
	using namespace GPlatesPropertyValues;
	boost::optional<GpmlTimeSample> sample = qv.value<boost::optional<GpmlTimeSample> >();
	if(sample && sample->is_disabled())
	{
		enable_flag = true;
	}
	EditPoleActionWidget *action_widget = new EditPoleActionWidget(this, enable_flag, this);
	table_sequences->setCellWidget(row, ColumnNames::ACTIONS, action_widget);
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_disable_pole(
		GPlatesQtWidgets::EditPoleActionWidget* action_widget,
		bool disable_flag)
{
	int row = get_row_for_action_widget(action_widget);
	QVariant qv = table_sequences->item(row,ColumnNames::TIME)->data(Qt::UserRole);
	using namespace GPlatesPropertyValues;
	boost::optional<GpmlTimeSample> sample = qv.value<boost::optional<GpmlTimeSample> >();
	if(sample)
	{
		sample->set_disabled(disable_flag);
	}
	qv.setValue(sample);
	table_sequences->item(row,ColumnNames::TIME)->setData(Qt::UserRole, qv);
	Qt::GlobalColor bg_color;
	if(disable_flag)
	{
		bg_color = Qt::gray;
	}
	else
	{
		bg_color = Qt::white;
	}
	
	table_sequences->removeCellWidget(row,ColumnNames::ACTIONS);
	for (int i = 0; i < table_sequences->horizontalHeader()->count()-1; i++)
	{
		table_sequences->item(row, i)->setData(Qt::BackgroundRole, bg_color);
	}
}


GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::make_irregular_sampling_from_table()
{
	static QLocale locale_;
	using namespace GPlatesPropertyValues;
	using namespace GPlatesModel;
	std::vector<GpmlTimeSample> time_samples;
		
	for (int i = 0; i < table_sequences->rowCount(); ++i)
	{
		static QString indet_string = QObject::tr("indet");
        // FIXME: handle bad "ok"s
		bool ok;
		double time = locale_.toDouble(table_sequences->item(i,ColumnNames::TIME)->text(),&ok);

        QString lat_string = table_sequences->item(i,ColumnNames::LATITUDE)->text();
        QString lon_string = table_sequences->item(i,ColumnNames::LONGITUDE)->text();
        double lat = (lat_string == indet_string) ? 0.0 : locale_.toDouble(lat_string,&ok);
		double lon = (lon_string == indet_string) ? 0.0 : locale_.toDouble(lon_string,&ok);  
		double angle = locale_.toDouble(table_sequences->item(i,ColumnNames::ANGLE)->text(),&ok);
		QString comment = table_sequences->item(i,ColumnNames::COMMENT)->text();
        ModelUtils::TotalReconstructionPole pole_data = {time,lat,lon,angle,comment};
		QVariant qv = table_sequences->item(i, ColumnNames::TIME)->data(Qt::UserRole);
		boost::optional<GpmlTimeSample> original_sample = qv.value<boost::optional<GpmlTimeSample> >();
		GpmlTimeSample new_time_sample = ModelUtils::create_gml_time_sample(pole_data, d_is_grot);
		if(original_sample)
		{
			if(original_sample->is_disabled())
			{
				new_time_sample.set_disabled(true);
			}
			GpmlTotalReconstructionPole 
				*new_pole =	dynamic_cast<GpmlTotalReconstructionPole*>(new_time_sample.value().get()),
				*old_pole = dynamic_cast<GpmlTotalReconstructionPole*>(original_sample->value().get());			
			if(new_pole && old_pole)
			{
				new_pole->metadata() = old_pole->metadata();
			}
		}
		time_samples.push_back(new_time_sample);
	}
		
	StructuralType value_type = d_is_grot ?
			StructuralType::create_gpml("TotalReconstructionPole"):
			StructuralType::create_gpml("FiniteRotation");

	PropertyValue::non_null_ptr_type gpml_irregular_sampling =
		GpmlIrregularSampling::create(
				time_samples,
				GPlatesUtils::get_intrusive_ptr(
						GpmlFiniteRotationSlerp::create(value_type)), 
				value_type);

	TopLevelProperty::non_null_ptr_type top_level_property_inline =
		TopLevelPropertyInline::create(
				PropertyName::create_gpml("totalReconstructionPole"),
				gpml_irregular_sampling, 
				std::map<XmlAttributeName, XmlAttributeValue>());

	return top_level_property_inline;
}


