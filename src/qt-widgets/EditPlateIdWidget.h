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
 
#ifndef GPLATES_QTWIDGETS_EDITPLATEIDWIDGET_H
#define GPLATES_QTWIDGETS_EDITPLATEIDWIDGET_H

#include <boost/intrusive_ptr.hpp>
#include "AbstractEditWidget.h"
#include "property-values/GpmlPlateId.h"
#include "model/types.h"

#include "EditPlateIdWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditPlateIdWidget:
			public AbstractEditWidget, 
			protected Ui_EditPlateIdWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditPlateIdWidget(
				QWidget *parent_ = NULL);
		
		virtual
		void
		reset_widget_to_default_values();

		void
		update_widget_from_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;
		
		virtual
		GPlatesModel::integer_plate_id_type
		create_integer_plate_id_from_widget() const;

		virtual
		bool
		update_property_value_from_widget();


		/* Unique to EditPlateIdWidget is the ability to hold a 'None' or null value.
		 * This has been done for CreateFeatureDialog's sake, to prototype an interface
		 * which allows a spinbox for conjugate plate IDs to be present but not mandatory.
		 * This has possible applications for a more generic interface which could
		 * go in AbstractEditWidget, but we'd need to hammer out how it would look.
		 */
		
		virtual
		bool
		supports_null_value() const
		{
			return true;
		}
		
		virtual
		bool
		permits_null_value() const
		{
			return d_null_value_permitted;
		}
		
		virtual
		void
		set_null_value_permitted(
				bool null_permitted)
		{
			d_null_value_permitted = null_permitted;
			if (d_null_value_permitted) {
				spinbox_plate_id->setSpecialValueText(tr("None"));
				spinbox_plate_id->setMinimum(-1);
			} else {
				spinbox_plate_id->setSpecialValueText("");
				spinbox_plate_id->setMinimum(0);
			}
			button_set_to_null->setVisible(d_null_value_permitted);
		}
		
		virtual
		bool
		is_null() const;

		virtual
		void
		set_null(
				bool should_nullify);

	signals:
		
		void
		value_changed();
	
	private slots:
	
		/**
		 * Triggered from button.
		 */
		void
		nullify()
		{
			set_null(true);
		}

		void
		handle_value_changed();
		
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
		boost::intrusive_ptr<GPlatesPropertyValues::GpmlPlateId> d_plate_id_ptr;


		/**
		 * Whether we will allow the user to effectively select 'None' as the plate ID.
		 */
		bool d_null_value_permitted;
		
	};
}

#endif  // GPLATES_QTWIDGETS_EDITPLATEIDWIDGET_H

