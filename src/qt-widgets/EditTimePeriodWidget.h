/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EDITTIMEPERIODWIDGET_H
#define GPLATES_QTWIDGETS_EDITTIMEPERIODWIDGET_H

#include <boost/intrusive_ptr.hpp>

#include "ui_EditTimePeriodWidgetUi.h"

#include "AbstractEditWidget.h"
#include "InformationDialog.h"

#include "property-values/GmlTimePeriod.h"


namespace GPlatesQtWidgets
{
	class EditTimePeriodWidget:
			public AbstractEditWidget, 
			protected Ui_EditTimePeriodWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditTimePeriodWidget(
				QWidget *parent_ = NULL);
		
		virtual
		void
		reset_widget_to_default_values();

		void
		update_widget_from_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

		virtual
		bool
		update_property_value_from_widget();

		// easy access method
		double
		get_time_period_begin()
		{
			double b = 0.0;
			if ( d_time_period_ptr->begin()->get_time_position().is_distant_past() )
			{
				b = 1000.0;
			}
			else if ( d_time_period_ptr->begin()->get_time_position().is_distant_past() )
			{
				b = 0.0;
			}
			else {
				b = d_time_period_ptr->begin()->get_time_position().value();
			}
			return b;
		}

		double
		get_time_period_end()
		{
			double e = 0.0;
			if ( d_time_period_ptr->end()->get_time_position().is_distant_past() )
			{
				e = 1000.0;
			}
			else if ( d_time_period_ptr->end()->get_time_position().is_distant_past() )
			{
				e = 0.0;
			}
			else {
				e = d_time_period_ptr->end()->get_time_position().value();
			}
			return e;
		}

		/**
		 * Accessor for the '&Begin' label. As we have more than one main
		 * label for this widget, we cannot simply rely on the label()
		 * accessor provided by AbstractEditWidget.
		 */
		QLabel *
		label_begin()
		{
			return label_begin_time;
		}

		/**
		 * Accessor for the '&End' label. As we have more than one main
		 * label for this widget, we cannot simply rely on the label()
		 * accessor provided by AbstractEditWidget.
		 */
		QLabel *
		label_end()
		{
			return label_end_time;
		}

		bool
		valid();

	private Q_SLOTS:
	
		void
		handle_appearance_is_distant_past_check();
	
		void
		handle_appearance_is_distant_future_check();

		void
		handle_disappearance_is_distant_past_check();
	
		void
		handle_disappearance_is_distant_future_check();

	private:
		
		/**
		 * This boost::intrusive_ptr is used to remember the property value which
		 * was last loaded into this editing widget. This is done so that the
		 * edit widget can directly update the property value later.
		 *
		 * We need to use a reference-counting pointer to make sure the property
		 * value doesn't disappear while this edit widget is active; however, since
		 * the property value is not known at the time the widget is created,
		 * the pointer may be NULL and this must be checked for.
		 *
		 * The pointer will also be NULL when the edit widget is being used for
		 * adding brand new properties to the model.
		 */
		boost::intrusive_ptr<GPlatesPropertyValues::GmlTimePeriod> d_time_period_ptr;

		/**
		 * "What does this mean?" blue question mark help dialog.
		 * Memory managed by Qt.
		 */
		InformationDialog *d_help_dialog;

		static const QString s_help_dialog_text;
		static const QString s_help_dialog_title;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITTIMEPERIODWIDGET_H
