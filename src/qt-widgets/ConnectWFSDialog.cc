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
#include <QDebug>
#include <QDir>
#include <QHttpRequestHeader>
#include <QPushButton>
#include <QMessageBox>
#include <QUrl>
#include <time.h>

#include "app-logic/ApplicationState.h"
#include "file-io/ArbitraryXmlReader.h"
#include "file-io/File.h"
#include "file-io/GeoscimlProfile.h"

#include "ConnectWFSDialog.h"

using namespace GPlatesQtWidgets;

ConnectWFSDialog::ConnectWFSDialog(
		GPlatesAppLogic::ApplicationState& app_state):
	d_current_request_id(0),
	d_app_state(app_state),
	d_canceled(false)
{
	setupUi(this);
	d_progress_dlg = new QProgressDialog(this);
	// ButtonBox signals.
	QObject::connect(
			buttonBox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_accept()));
	QObject::connect(
			buttonBox,
			SIGNAL(rejected()),
			this,
			SLOT(handle_reject()));
	QObject::connect(
			checkBox_proxy,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_proxy_state_change(int)));
	QObject::connect(
			d_progress_dlg,
			SIGNAL( canceled()),
			this,
			SLOT(handle_canceled()));
	
	checkBox_proxy->setChecked(false);
	handle_proxy_state_change(Qt::Unchecked);
	lineEdit_proxy->setText("www-cache.usyd.edu.au");
	lineEdit_name->setText("Untitled");
	lineEdit_url->setText("http://");

}


ConnectWFSDialog::~ConnectWFSDialog()
{
	delete d_progress_dlg;
}


void
ConnectWFSDialog::handle_accept()
{
	d_canceled = false;
	//TODO:
	//check the validation of url
	QUrl url = QUrl(lineEdit_url->text(),QUrl::TolerantMode);
	if(!url.isValid() || !lineEdit_url->text().startsWith("http://"))
	{
		QMessageBox msgBox;
		msgBox.setText(
				QApplication::translate(
						"QMessageBox", 
						"Invalid request url.", 
						0, 
						QApplication::UnicodeUTF8));
		msgBox.exec();
		return;
	}

	//set up progress bar
	QLabel* progress_label = 
		new QLabel(
				QApplication::translate(
						"QProgressDialog", 
						"Connecting to WFS server...", 
						0, 
						QApplication::UnicodeUTF8),
				d_progress_dlg);
	progress_label->setAlignment(Qt::AlignHCenter);
	d_progress_dlg->setMinimumSize(350,80);
	d_progress_dlg->setLabel(progress_label);
	d_progress_dlg->setRange(0,9);
	srand ( time(NULL) );
	d_progress_dlg->setValue(rand() % 10);
	d_progress_dlg->show();

	if(!d_http)
	{
		d_http.reset(new QHttp());
	}
	
	QString request = 
		QString("request=") + 
		plainTextEdit_request->toPlainText();
	QHttpRequestHeader header("POST", url.path());
	
	header.setValue("Content-type", "application/x-www-form-urlencoded");
	header.setValue("Accept", "text/plain");
	header.setValue("Host", url.host()); 

	d_http->setHost(url.host());

	if(checkBox_proxy->isChecked())
	{
		d_http->setProxy(lineEdit_proxy->text(), spinBox_proxy_port->value());
	}
	connect(d_http.get(), SIGNAL(requestFinished(int,bool)), this, SLOT(hanle_wfs_response(int,bool)));
	d_current_request_id = d_http->request(header,request.toUtf8());
	accept();
}

		
void
ConnectWFSDialog::handle_reject()
{
	reject();
}


void
ConnectWFSDialog::hanle_wfs_response(
		int id, 
		bool err)
{
	if(d_canceled)
	{
		qDebug() << "Http request has been canceled by user.";
		return;
	}
	if(err)
	{
		qWarning() << "Error happened during http request.";
		return;
	}
	if(d_current_request_id != id)
	{
		qWarning() << "http request id does not match.";
		return;
	}

	if(d_finished_request_id.find(id) != d_finished_request_id.end())
	{
		qWarning() << "This http request has been finished.";
		return;
	}
	d_finished_request_id.insert(id);
	
	d_progress_dlg->setValue(8);
	
	QByteArray xml_data = d_http->readAll();
	qDebug() << xml_data;
	//temp file path
	QString tmp_dir = QDir::tempPath();
	QString file_base_name = lineEdit_name->text();
	QString filename = tmp_dir + "/" + file_base_name;
	
	//create temp file
	QFile tmp_file(filename);
	tmp_file.open(QIODevice::ReadWrite | QIODevice::Text);
	tmp_file.close();

	const GPlatesFileIO::FileInfo file_info(filename);
	GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);
	
	GPlatesFileIO::ArbitraryXmlReader::read_xml_data(
			file->get_reference(), 
			boost::shared_ptr<GPlatesFileIO::ArbitraryXmlProfile>(new GPlatesFileIO::GeoscimlProfile()), 
			xml_data);
	d_app_state.get_feature_collection_file_state().add_file(file);
	d_http->clearPendingRequests();
	d_http->close();
	d_progress_dlg->hide();
}


void
ConnectWFSDialog::handle_proxy_state_change(int state)
{
	if(Qt::Unchecked == state)
	{
		lineEdit_proxy->setDisabled(true);
		spinBox_proxy_port->setDisabled(true);
	}
	else
	{
		lineEdit_proxy->setDisabled(false);
		spinBox_proxy_port->setDisabled(false);
	}
}


void
ConnectWFSDialog::handle_canceled()
{
	d_canceled = true;
	d_http->clearPendingRequests();
	d_http->close();
	d_progress_dlg->hide();
}


