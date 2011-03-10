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
 
#ifndef GPLATES_QTWIDGETS_CONNECTWFSDIALOG_H
#define GPLATES_QTWIDGETS_CONNECTWFSDIALOG_H
#include <set>
#include <boost/shared_ptr.hpp>
#include <QHttp>
#include <QProgressDialog>

#include "ConnectWFSDialogUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	
	class ConnectWFSDialog : 
			public QDialog, 
			protected Ui_ConnectWFSDialog
	{
		Q_OBJECT

	public:

		/**
		*
		*/
		ConnectWFSDialog(
				GPlatesAppLogic::ApplicationState& app_state);

		~ConnectWFSDialog();

	private slots:
		void
		handle_accept();
		
		void
		handle_reject();

		void
		hanle_wfs_response(
				int id, 
				bool err);

		void
		handle_proxy_state_change(int);

		void
		handle_canceled();

	private:
		boost::shared_ptr<QHttp> d_http;
		int d_current_request_id;
		GPlatesAppLogic::ApplicationState& d_app_state;
		QProgressDialog* d_progress_dlg;
		bool d_canceled;
		std::set<int> d_finished_request_id;

	private:
		ConnectWFSDialog();
		ConnectWFSDialog(const ConnectWFSDialog&);
	};
}

#endif  // GPLATES_QTWIDGETS_CONNECTWFSDIALOG_H
