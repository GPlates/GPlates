/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
#include <QPushButton>
#include <QMessageBox>
#include <QString>
#include <QErrorMessage>

#include <QtGui>
#include <QtNetwork>
#include <QMessageBox>

#include <time.h>

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/UserPreferences.h"

#include "file-io/ArbitraryXmlReader.h"
#include "file-io/File.h"
#include "file-io/GeoscimlProfile.h"

#include "maths/LatLonPoint.h"

#include "utils/NetworkUtils.h"

#include "ConnectWFSDialog.h"


GPlatesQtWidgets::ConnectWFSDialog::ConnectWFSDialog(
		GPlatesAppLogic::ApplicationState& app_state,
		QWidget *parent_) :
	GPlatesDialog(
			parent_, 
			Qt::Window),
	d_app_state(app_state),
	d_request_id(0),
	d_httpRequestAborted(false)
{
	setupUi(this);

    // ButtonBox signals.
    QObject::connect(
            buttonBox,
            SIGNAL(accepted()),
            this,
            SLOT( downloadFile() ));

    QObject::connect(
            buttonBox,
            SIGNAL(rejected()),
            this,
            SLOT( close() ));

	// set up progress dialog 
	d_progress_dlg = new QProgressDialog(this);
	QObject::connect( 
		d_progress_dlg, 
		SIGNAL(canceled()), 
		this, 
		SLOT(cancelDownload()));

	// Proxy :-

	// Fill in defaults from UserPreferences (which themselves might be able to get info from system)
	checkBox_proxy->setChecked(app_state.get_user_preferences().get_value("net/proxy/enabled").toBool());
	lineEdit_proxy->setText(app_state.get_user_preferences().get_value("net/proxy/url").toString());
	// FIXME: We can later move this to a GUI prefs config widget, and with it set the
	//        GPlates-wide proxy that changes whenever the preference changes. For now, we're
	//        only using it for WFS, so we only need to update the proxy details prior to a
	//        WFS request.
	// Update disabled/enabled checkboxes.
	QObject::connect(checkBox_proxy, SIGNAL(stateChanged(int)),
			this, SLOT(handle_proxy_state_change(int)));
	handle_proxy_state_change(checkBox_proxy->checkState());

	
	// Query name 
	QString s = "Untitled-";
	s.append ( QString::number( d_request_id ) );
	lineEdit_name->setText( s );

	// set default URL
	// FIXME: comboBox_url->setItem(0);

	// Time controls do not need any adjustments

	// apply valid time button
	QObject::connect(
			pushButton_apply,
			SIGNAL(clicked()),
			this,
			SLOT(handle_apply_valid_time()));
	

	// FIXME:  remove default requests
	// plainTextEdit_request->setPlainText("?&polygon=-121.9 46.73, -121.6 44.26, -116 44.7, -115.8 46.76, -121.9 46.73&age_bottom=1&age_top=0");
	// plainTextEdit_request->setPlainText("?&polygon=-109.5 41.5, -109.7 36.55, -99.22 36.19, -99.77 41.4, -109.5 41.5&age_bottom=100&age_top=0");
	// good for paleo db
	// plainTextEdit_request->setPlainText("?&polygon=-103.3 37.22, -103.2 36.17, -99.78 36.11, -99.67 37.31, -103.3 37.22&age_bottom=100&age_top=0");

	plainTextEdit_request->setPlainText("?&polygon=-104.3 37.88, -104.2 35.77, -99.38 35.6, -99.23 37.82, -104.3 37.88&age_bottom=200&age_top=0");

	d_request_geom_string = "?&polygon=-104.3 37.88, -104.2 35.77, -99.38 35.6, -99.23 37.82, -104.3 37.88";
}

GPlatesQtWidgets::ConnectWFSDialog::~ConnectWFSDialog()
{
	delete d_progress_dlg;
}


void
GPlatesQtWidgets::ConnectWFSDialog::set_request_geometry( 
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr)
{
	QString geom_str = "";

	//qDebug() << "ConnectWFSDialog::set_request_geometry(): ";
	// double check on geom type 
	const GPlatesMaths::GeometryType::Value geometry_type =
			GPlatesAppLogic::GeometryUtils::get_geometry_type(*geometry_ptr);

	if (geometry_type == GPlatesMaths::GeometryType::POLYGON)
	{
		geom_str.append("?&polygon=");

		std::vector<GPlatesMaths::PointOnSphere> points;
		GPlatesAppLogic::GeometryUtils::get_geometry_exterior_points(*geometry_ptr, points);

		std::vector<GPlatesMaths::PointOnSphere>::iterator itr = points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::iterator end = points.end();
		std::vector<GPlatesMaths::PointOnSphere>::iterator last = points.end();
		--last;

		for ( ; itr != end; ++itr)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point( *itr );
			double lat = llp.latitude();
			double lon = llp.longitude();
			geom_str.append( QString::number(lon, 'g', 4) );
			geom_str.append( " ");
			geom_str.append( QString::number(lat, 'g', 4) );

			if ( itr != (last) )
			{
				 geom_str.append(", ");
			}
		}
	}
	else
	{
		QErrorMessage *e = new QErrorMessage( this );
		e->showMessage("Please use the Polygon digitization tool for WFS queries");
		return;
	}
	
	// Set the data 
	d_request_geom_string = geom_str;

	// pretend the user clicked apply with the default time values
	handle_apply_valid_time();
}

void 
GPlatesQtWidgets::ConnectWFSDialog::startRequest(QUrl url)
{
// qDebug() << "ConnectWFSDialog::startRequest: url=" << url;

	d_reply = d_qnam.get( QNetworkRequest(url) );

	// Set proxy info if needed 
	update_global_proxy();

	QObject::connect(
		d_reply, 
		SIGNAL( finished()),
		this, 
		SLOT( httpFinished()));

	QObject::connect(
		d_reply, 
		SIGNAL( readyRead()),
		this, 
		SLOT( httpReadyRead()));

	QObject::connect(
		d_reply, 
		SIGNAL( downloadProgress(qint64,qint64)),
		this, 
		SLOT(updateDataReadProgress(qint64,qint64)));
}

void
GPlatesQtWidgets::ConnectWFSDialog::downloadFile()
{
	// Get the base URL
	QString url_str = comboBox_url->currentText();

	// Add the request string to the URL 
	QString request_str = plainTextEdit_request->toPlainText();
	url_str.append( request_str );

	// Form the total URL
	d_url = QUrl(url_str, QUrl::TolerantMode);

	// check url 
	if( !d_url.isValid() || !comboBox_url->currentText().startsWith("http://") )
	{
		QMessageBox msgBox;
		msgBox.setText(
				QApplication::translate(
						"QMessageBox", 
						"Invalid request url.", 
						0));
		msgBox.exec();
		return;
	}

	QFileInfo fileInfo(d_url.path());
	QString fileName = fileInfo.fileName();
	if (fileName.isEmpty())
	{
		fileName = "TEST.xml";
	}
	// fix test filename
	fileName = "TEST.xml";

	if (QFile::exists(fileName)) 
	{
#if 0
		if (
			QMessageBox::question(this, tr("HTTP"),
				tr("There already exists a file called %1 in "
				"the current directory. Overwrite?").arg(fileName),
				QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
			== QMessageBox::No)
             return;
#endif
         QFile::remove(fileName);
	}

	d_xml_file = new QFile(fileName);

	if (!d_xml_file->open(QIODevice::WriteOnly)) 
	{
		QMessageBox::information(
			this, tr("HTTP"),
			tr("Unable to save the file %1: %2.")
			.arg(fileName).arg(d_xml_file->errorString()));
		delete d_xml_file;
		d_xml_file = 0;
		return;
	}

	// Set up progress bar
	QLabel* progress_label = 
		new QLabel(
			QApplication::translate(
				"QProgressDialog", 
				"Connecting to WFS server ...    ", 
				0),
		d_progress_dlg);

	d_progress_dlg->setLabel(progress_label);
	d_progress_dlg->setMinimumSize(350,80);
	// FIXME: upper value on range is very arbitrary:
	// NOTE : with the Macrostrat rocktype tests,
	// a coverage of the US, from 0Ma to 200 Ma is about 6178735 bytes
	// NOTE : with the Macrostrat paleodb tests,
	// a coverage of the US, from 0Ma to 500 Ma is about 25595412 bytes
	d_progress_dlg->setRange(0, 50000000); 
	d_progress_dlg->setValue(0);
	d_progress_dlg->show();

	// set the flags 
	d_httpRequestAborted = false;

	// schedule the request
	startRequest( d_url );
}

void 
GPlatesQtWidgets::ConnectWFSDialog::cancelDownload()
{
	d_httpRequestAborted = true;
	d_reply->abort();
	d_progress_dlg->hide();
}   

void 
GPlatesQtWidgets::ConnectWFSDialog::httpFinished()
{
// qDebug() << "GPlatesQtWidgets::ConnectWFSDialog::httpFinished()";

	if ( d_httpRequestAborted ) 
	{
		if (d_xml_file) 
		{
             d_xml_file->close();
             d_xml_file->remove();
             delete d_xml_file;
             d_xml_file = 0;
		}
		d_reply->deleteLater();

		d_progress_dlg->hide();
		return;
	}

	d_httpRequestAborted = false;
     
	d_progress_dlg->hide();
	d_xml_file->flush();
	d_xml_file->close();

	QVariant redirectionTarget = d_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
     
	// increment id  
	++d_request_id;

	if (d_reply->error()) 
	{
		d_xml_file->remove();
		QMessageBox::information(
			this, tr("HTTP"),
			tr("Download failed: %1.").arg(d_reply->errorString()));
	} 
	else if (!redirectionTarget.isNull()) 
	{
		QUrl newUrl = d_url.resolved(redirectionTarget.toUrl());
		if (QMessageBox::question(
				this, tr("HTTP"),
				tr("Redirect to %1 ?").arg(newUrl.toString()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) 
		{
             d_url = newUrl;
             d_reply->deleteLater();
             d_xml_file->open(QIODevice::WriteOnly);
             d_xml_file->resize(0);
             startRequest(d_url);
             return;
		}
	} 
	else 
	{
		QString fileName = QFileInfo(
			QUrl( comboBox_url->currentText() ).path()).fileName();
	}

	d_reply->deleteLater();
	d_reply = 0;
	delete d_xml_file;
	d_xml_file = 0;

	// translate the xml into gpml
	process_xml();

	// Update widgets for next request
	QString s = "Untitled-";
	s.append ( QString::number( d_request_id ) );
	lineEdit_name->setText( s );

	// close the dialog
	close();
}


void 
GPlatesQtWidgets::ConnectWFSDialog::httpReadyRead()
{
	// this slot gets called every time the QNetworkReply has new data.
	// We read all of its new data and write it into the d_xml_file.
	// That way we use less RAM than when reading it at the finished()
	// signal of the QNetworkReply
	QByteArray data_array = d_reply->readAll();

	d_xml_data.append( data_array );

	if (d_xml_file)
	{
		d_xml_file->write( data_array );
	}
}


void 
GPlatesQtWidgets::ConnectWFSDialog::updateDataReadProgress(
	qint64 bytesRead, qint64 totalBytes)
{
	if (d_httpRequestAborted) { return; }
	d_progress_dlg->setValue(bytesRead);

	QString label = "Transfering XML (";
	label.append( QString::number( bytesRead ) );
	label.append( " bytes)" );
	d_progress_dlg->setLabelText( label );
}


void 
GPlatesQtWidgets::ConnectWFSDialog::process_xml()
{
// qDebug() << "GPlatesQtWidgets::ConnectWFSDialog::process_xml(): d_xml_data.size() = " << d_xml_data.size();

	// Double check on return
	if ( !d_xml_data.startsWith("<?xml") )
	{
		QErrorMessage *e = new QErrorMessage( this );
		e->showMessage("Error with query or returned XML; Please Cancel;");
		return;
	}
	
	// Get temp file path
	QString tmp_dir = QDir::tempPath();
	QString file_base_name = lineEdit_name->text();
	QString filename = tmp_dir + "/" + file_base_name;
	
	// First step: check for number of features 
	int count = d_app_state.get_feature_collection_file_io().count_features_in_xml_data(d_xml_data);

	// Ask to proceed
	bool process = false;
	QMessageBox msgBox;
	QString msg = QString("Web Feature Service query returned %1 features").arg( count );
	msgBox.setText( msg );
 	msgBox.setInformativeText("Do you want to load features into a new, temporary layer?");
 	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
 	msgBox.setDefaultButton(QMessageBox::Yes);
 	int ret = msgBox.exec();

 	switch (ret) 
	{
		case QMessageBox::No:
			break;
		case QMessageBox::Yes:
			process = true;
			break;
		default:
			break;
	}

	if ( process ) 
	{
		d_app_state.get_feature_collection_file_io().load_xml_data(filename, d_xml_data);
	}

	// clear out old data for next query 
	d_xml_data.clear();
}





void
GPlatesQtWidgets::ConnectWFSDialog::handle_apply_valid_time()
{
//qDebug() << "ConnectWFSDialog::handle_apply_valid_time():";

	// double check on geom string 
	if (d_request_geom_string == "")
	{
		QErrorMessage *e = new QErrorMessage( this );
		e->showMessage( "Please use the Polygon Digitization Tool to start WFS queries.\nDefine a bounding box for the query.\nSet the Valid time and click Apply.");
		return;
	}
	// Get info from widget
	double b = spinbox_begin->value();
	double e = spinbox_end->value();

//qDebug() << "ConnectWFSDialog::handle_apply_valid_time(): b =" << QString::number(b, 'g', 4);
//qDebug() << "ConnectWFSDialog::handle_apply_valid_time(): e =" << QString::number(e, 'g', 4);

	// build time string 
	d_request_time_string = "";
	d_request_time_string.append("&age_bottom=");
	d_request_time_string.append( QString::number(b, 'g', 4) );
	d_request_time_string.append("&age_top=");
	d_request_time_string.append( QString::number(e, 'g', 4) );

	// build total request string
	QString request_string = "";
	request_string.append( d_request_geom_string );
	request_string.append( d_request_time_string );

	// set the dialog's info
	//qDebug() << "ConnectWFSDialog::set_request_geometry(): request_string = " << request_string;
	plainTextEdit_request->setPlainText( request_string );
}



void
GPlatesQtWidgets::ConnectWFSDialog::handle_proxy_state_change(int state)
{
	lineEdit_proxy->setDisabled(state == Qt::Unchecked);
}


void
GPlatesQtWidgets::ConnectWFSDialog::update_global_proxy()
{
	if (checkBox_proxy->isChecked()) {
		QNetworkProxy proxy = GPlatesUtils::NetworkUtils::get_proxy_for_url(lineEdit_proxy->text());
		qDebug() << "WFS: Using proxy: " << GPlatesUtils::NetworkUtils::get_url_for_proxy(proxy).toString();
		d_qnam.setProxy(proxy);
	} else {
		qDebug() << "WFS: Proxy is disabled.";
		d_qnam.setProxy( QNetworkProxy(QNetworkProxy::NoProxy) );
	}
}

