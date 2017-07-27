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

#include <QDialog>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QProgressDialog>

#include "ConnectWFSDialogUi.h"

#include "EditTimePeriodWidget.h"
#include "GPlatesDialog.h"

#include "maths/GeometryOnSphere.h"


class QDialogButtonBox;
class QFile;
class QLabel;
class QLineEdit;
class QProgressDialog;
class QPushButton;
class QSslError;
class QAuthenticator;
class QNetworkReply;

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	
	class ConnectWFSDialog : 
			public GPlatesDialog, 
			protected Ui_ConnectWFSDialog
	{
		Q_OBJECT

	public:

		explicit
		ConnectWFSDialog(
				GPlatesAppLogic::ApplicationState& app_state,
				QWidget *parent_ = NULL);

		~ConnectWFSDialog();

		void
		set_request_geometry( GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ );

		void
		startRequest(QUrl url);

		void
		process_xml();

	private Q_SLOTS:

		void downloadFile(); // will call startRequest(url);

		void cancelDownload();

		// These slots are connected to d_reply within startRequest()
		void httpFinished();
		void httpReadyRead();
		void updateDataReadProgress(qint64 bytesRead, qint64 totalBytes);

		// These slots are connected to widgets on the dialog 
		void handle_apply_valid_time();
		void handle_proxy_state_change(int);

	private:
	
		/**
		 * Update the GPlates-wide proxy details based on what
		 * the proxy widgets are set to.
		 */
		void
		update_global_proxy();
		

		GPlatesAppLogic::ApplicationState& d_app_state;

		QProgressDialog* d_progress_dlg;

		QUrl d_url;
		QNetworkAccessManager d_qnam;
		QNetworkReply *d_reply;

		QFile *d_xml_file; // to save the xml results for debugging

		int d_request_id;

		bool d_httpRequestAborted;

	private:
		ConnectWFSDialog();
		ConnectWFSDialog(const ConnectWFSDialog&);

		QString d_request_geom_string;
		QString d_request_time_string;
		QString d_request_type_string;

		QByteArray d_xml_data;
	};
}

#endif  // GPLATES_QTWIDGETS_CONNECTWFSDIALOG_H
