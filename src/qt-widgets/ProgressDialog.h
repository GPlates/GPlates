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
#ifndef PROGRESS_DIALOG_H
#define PROGRESS_DIALOG_H

#include <QObject>
#include<QDialog>

#include <iostream>

#include "ui_ProgressDialogUi.h"

namespace GPlatesQtWidgets
{
	class ProgressDialog: 
		public QDialog,
		protected Ui_ProgressDialog 
	{
		Q_OBJECT

	public:
		explicit
		ProgressDialog(				
				QWidget *parent_ = NULL)
			:QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint),
			cancel_flag(false)
		{
			setupUi(this);
			
			QObject::connect(
					cancel_button, SIGNAL(clicked()),
					this, SLOT(handle_cancel()));
			// Handle user cancelling by pressing ESC key.
			QObject::connect(
					this, SIGNAL(rejected()),
					this, SLOT(handle_cancel()));
		}
		
		virtual
		~ProgressDialog()
		{	}


		bool
		canceled()
		{
			return cancel_flag;
		}

		void
		disable_cancel_button(
				bool flag)
		{
			cancel_button->setDisabled(flag);
		}


	public Q_SLOTS:

		void
		set_text(	
				const QString message)
		{
			info_label->setText(message);
		}

		void
		setRange(
				int min, 
				int max)
		{
			progress_bar->setRange(min,max);
		}
		
		void
		setValue(
				int val)
		{
			progress_bar->setValue(val);
		}
		
		void
		update_value(
				int val)
		{
			progress_bar->setValue(val);
			progress_bar->repaint();
			QCoreApplication::processEvents();
		}

		void
		update_progress(	
				int val, 
				const QString message)
		{
				progress_bar->setValue(val);
				info_label->setText(message);
				progress_bar->repaint();
				QCoreApplication::processEvents();
		}

	private Q_SLOTS:

		void 
		handle_cancel()
		{
			cancel_flag = true;
		}

	private:
		bool cancel_flag;

	};
}

#endif


