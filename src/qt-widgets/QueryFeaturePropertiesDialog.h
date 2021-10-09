/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESDIALOG_H
#define GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESDIALOG_H

#include <QDialog>
#include "QueryFeaturePropertiesDialogUi.h"
#include "model/FeatureHandle.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class QueryFeaturePropertiesDialog: 
			public QDialog,
			protected Ui_QueryFeaturePropertiesDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		QueryFeaturePropertiesDialog(
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~QueryFeaturePropertiesDialog()
		{  }

		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}

		void
		set_feature_type(
				const QString &feature_type);

		/**
		 * The parameter is a QString to enable us to pass the string "indeterminate".
		 */
		void
		set_euler_pole(
				const QString &point_position);

		void
		set_angle(
				const double &angle);

		void
		set_plate_id(
				unsigned long plate_id);

		void
		set_root_plate_id(
				unsigned long plate_id);

		void
		set_reconstruction_time(
				const double &recon_time);

		QTreeWidget &
		property_tree() const;

	public slots:

		void
		display_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);

	signals:

	private:
		/**
		 * This is the view state which is used to obtain the reconstruction root.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;
		
	};
}

#endif  // GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESDIALOG_H
