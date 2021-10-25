/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CONFIGVALUEDELEGATE_H
#define GPLATES_GUI_CONFIGVALUEDELEGATE_H

#include <QItemDelegate>



namespace GPlatesGui
{
	/**
	 * Qt Delegate for use in TableViews created for ConfigBundles and UserPreferences.
	 *
	 * This lets us have finer control over the editing widgets that get created in table cells.
	 * One ConfigValueDelegate gets created for one ConfigModel.
	 *
	 * I'm basing this off the Spin Box Delegate tutorial for now, until I've got a better handle
	 * on exactly what we need for our delegates. http://doc.trolltech.com/4.5/itemviews-spinboxdelegate.html
	 */
	class ConfigValueDelegate :
			public QItemDelegate
	{
		Q_OBJECT
	public:
		
		explicit
		ConfigValueDelegate(
				QObject *parent_ = NULL);
		
		virtual
		~ConfigValueDelegate()
		{  }
		
		
		/**
		 * The Delegate is used to create an editor widget whenever the user triggers an edit
		 * event. This is what gets called.
		 */
		QWidget *
		createEditor(
				QWidget *parent_widget,
				const QStyleOptionViewItem &option,
				const QModelIndex &idx) const;
		
		
		/**
		 * Reads data from the Qt model, converting it as appropriate, and writes it to the
		 * editor widget.
		 */
		void
		setEditorData(
				QWidget *editor,
				const QModelIndex &idx) const;
		
		
		/**
		 * Reads data from the edit widget, converting it as appropriate, and writes it to the
		 * config model.
		 */
		void
		setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &idx) const;
		
		
		void
		updateEditorGeometry(
				QWidget *editor,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;
	
	private Q_SLOTS:
		
		void
		commit_and_close(
				QWidget *editor)
		{
			commitData(editor);
			closeEditor(editor);
		}
		
	};
}



#endif //GPLATES_GUI_CONFIGVALUEDELEGATE_H


