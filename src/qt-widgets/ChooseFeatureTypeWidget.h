/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_CHOOSEFEATURETYPEWIDGET_H
#define GPLATES_QTWIDGETS_CHOOSEFEATURETYPEWIDGET_H

#include <boost/optional.hpp>
#include <QWidget>
#include <QFocusEvent>

#include "SelectionWidget.h"

#include "model/FeatureType.h"


namespace GPlatesQtWidgets
{
	class ChooseFeatureTypeWidget :
			public QWidget
	{
		Q_OBJECT
		
	public:

		explicit
		ChooseFeatureTypeWidget(
				SelectionWidget::DisplayWidget display_widget,
				QWidget *parent_ = NULL);

		/**
		 * Initialises the list widget with feature types.
		 */
		void
		initialise(
				bool topological);

		/**
		 * Gets the currently selected feature type, or boost::none if no feature type
		 * is currently selected.
		 */
		boost::optional<GPlatesModel::FeatureType>
		get_feature_type() const;

	signals:

		void
		item_activated();

		void
		current_index_changed(
				boost::optional<GPlatesModel::FeatureType>);

	protected:

		virtual
		void
		focusInEvent(
				QFocusEvent *ev);

	private slots:

		void
		handle_item_activated(
				int index);

		void
		handle_current_index_changed(
				int index);

	private:

		void
		make_signal_slot_connections();

		SelectionWidget *d_selection_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONWIDGET_H
