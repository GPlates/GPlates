/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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
 
#include <unistd.h>
#include <boost/format.hpp>

#include "maths/LatLonPointConversions.h"
#include "model/ModelUtility.h"
#include "model/DummyTransactionHandle.h"

#include "Document.h"
#include "InformationDialog.h"

namespace {

	void
	reconstruct_fake_model(
		GPlatesModel::Model &model,
		GPlatesModel::FeatureCollectionHandle::weak_ref isochrons,
		GPlatesModel::FeatureCollectionHandle::weak_ref total_recon_seqs, 
		double time)
	{
		GPlatesModel::Reconstruction::non_null_ptr_type reconstruction =
				model.create_reconstruction(isochrons, total_recon_seqs, time, 0);
	}

#if 0
	template<class render_functor_type>
	class render
	{
	private:
		render_functor_type d_render_functor;
		typedef typename render_functor_type::argument_type::element_type geometry_type;
		GPlatesGui::GlobeCanvas *d_canvas_ptr;
		GPlatesGui::Colour d_colour;
	public:
		render(
				GPlatesGui::GlobeCanvas *canvas_ptr,
				render_functor_type render_functor,
				GPlatesGui::Colour colour) :
			d_render_functor(render_functor),
			d_canvas_ptr(canvas_ptr),
			d_colour(colour)
		{  }

		void
		operator()(
				std::vector<GPlatesModel::ReconstructedFeatureGeometry<geometry_type> >::iterator rfg)
		{
			render_functor(rfg->geometry(), d_colour);
		}
	}
#endif
	template<typename In, typename F>
	void
	render(
			In start, 
			In end, 
			F functor, 
			GPlatesGui::GlobeCanvas *canvas_ptr)
	{
		for (In i = start; i != end; ++i)
			canvas_ptr->*functor(i->geometry(), GPlatesGui::Colour::RED);
	}

	void
	render_model(
			GPlatesGui::GlobeCanvas *canvas_ptr, 
			GPlatesModel::Model *model_ptr, 
			GPlatesModel::FeatureCollectionHandle::weak_ref isochrons,
			GPlatesModel::FeatureCollectionHandle::weak_ref total_recon_seqs,
			double time)
	{
		GPlatesModel::Reconstruction::non_null_ptr_type reconstruction =
				model_ptr->create_reconstruction(isochrons, total_recon_seqs, time,
						501);

		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator iter =
				reconstruction->point_geometries().begin();
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator finish =
				reconstruction->point_geometries().end();
		while (iter != finish) {
			canvas_ptr->draw_point(*iter->geometry());
			++iter;
		}
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator iter2 =
				reconstruction->polyline_geometries().begin();
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator finish2 =
				reconstruction->polyline_geometries().end();
		while (iter2 != finish2) {
			canvas_ptr->draw_polyline(*iter2->geometry());
			++iter2;
		}
		//render(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), &GPlatesGui::GlobeCanvas::draw_point, canvas_ptr);
		//for_each(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), render(canvas_ptr, &GlobeCanvas::draw_point, point_colour))
		// for_each(reconstruction->polyline_geometries().begin(), reconstruction->polyline_geometries().end(), polyline_point);
	}
	
	void
	draw_sample_points_and_lines(
			GPlatesGui::GlobeCanvas *canvas_ptr)
	{
		using namespace GPlatesMaths;
	
		std::list< LatLonPoint > points;
	
		points.push_back(LatLonPoint(30, 40));
		points.push_back(LatLonPoint(43, 41));
		points.push_back(LatLonPoint(47, 42));
	
		PolylineOnSphere line = 
			LatLonPointConversions::convertLatLonPointListToPolylineOnSphere(points);
	
		canvas_ptr->draw_polyline(line);
	
		canvas_ptr->draw_point(PointOnSphere(UnitVector3D(1, 0, 0)));
		canvas_ptr->draw_point(PointOnSphere(UnitVector3D(0, 1, 0)));
		canvas_ptr->draw_point(PointOnSphere(UnitVector3D(0, 0, 1)));
	
		canvas_ptr->update_canvas();
	}

	const GPlatesModel::FeatureHandle::weak_ref
	create_isochron(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref &target_collection,
			const unsigned long &plate_id,
			const double *points,
			unsigned num_points,
			const GPlatesModel::GeoTimeInstant &geo_time_instant_begin,
			const GPlatesModel::GeoTimeInstant &geo_time_instant_end,
			const UnicodeString &description,
			const UnicodeString &name,
			const UnicodeString &codespace_of_name)
	{
		using namespace GPlatesModel;
		using namespace GPlatesModel::ModelUtility;

		UnicodeString feature_type_string("gpml:Isochron");
		FeatureType feature_type(feature_type_string);
		FeatureHandle::weak_ref feature_handle =
				model.create_feature(feature_type, target_collection);
	
		PropertyContainer::non_null_ptr_type reconstruction_plate_id_container =
				create_reconstruction_plate_id(plate_id);
		PropertyContainer::non_null_ptr_type centre_line_of_container =
				create_centre_line_of(points, num_points);
		PropertyContainer::non_null_ptr_type valid_time_container =
				create_valid_time(geo_time_instant_begin, geo_time_instant_end);
		PropertyContainer::non_null_ptr_type description_container =
				create_description(description);
		PropertyContainer::non_null_ptr_type name_container =
				create_name(name, codespace_of_name);
	
		DummyTransactionHandle pc1(__FILE__, __LINE__);
		feature_handle->append_property_container(reconstruction_plate_id_container, pc1);
		pc1.commit();
	
		DummyTransactionHandle pc2(__FILE__, __LINE__);
		feature_handle->append_property_container(centre_line_of_container, pc2);
		pc2.commit();
	
		DummyTransactionHandle pc3(__FILE__, __LINE__);
		feature_handle->append_property_container(valid_time_container, pc3);
		pc3.commit();
	
		DummyTransactionHandle pc4(__FILE__, __LINE__);
		feature_handle->append_property_container(description_container, pc4);
		pc4.commit();
	
		DummyTransactionHandle pc5(__FILE__, __LINE__);
		feature_handle->append_property_container(name_container, pc5);
		pc5.commit();
	
		return feature_handle;
	}

	void 
	create_everything(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &isochrons,
			GPlatesModel::FeatureCollectionHandle::weak_ref &total_recon_seqs)
	{
		using namespace GPlatesModel;
		using namespace GPlatesModel::ModelUtility;
	
		isochrons = model.create_feature_collection();
		total_recon_seqs = model.create_feature_collection();
	
		static const unsigned long plate_id1 = 501;
		// lon, lat, lon, lat... is how GML likes it.
		static const double points1[] = {
			69.2877, -5.5765,
			69.1323, -4.8556,
			69.6092, -4.3841,
			69.2748, -3.9554,
			69.7079, -3.3680,
			69.4119, -3.0486,
			69.5999, -2.6304,
			68.9400, -1.8446,
		};
		static const unsigned num_points1 = sizeof(points1) / sizeof(points1[0]);
		GeoTimeInstant geo_time_instant_begin1(10.9);
		GeoTimeInstant geo_time_instant_end1 =
				GeoTimeInstant::create_distant_future();
		UnicodeString description1("CARLSBERG RIDGE, INDIA-AFRICA ANOMALY 5 ISOCHRON");
		UnicodeString name1("Izzy the Isochron");
		UnicodeString codespace_of_name1("EarthByte");
	
		FeatureHandle::weak_ref isochron1 =
				create_isochron(model, isochrons, plate_id1, points1, num_points1,
						geo_time_instant_begin1, geo_time_instant_end1,
						description1, name1, codespace_of_name1);
	
		static const unsigned long plate_id2 = 702;
		// lon, lat, lon, lat... is how GML likes it.
		static const double points2[] = {
			41.9242, -34.9340,
			42.7035, -33.4482,
			44.8065, -33.5645,
			44.9613, -33.0805,
			45.6552, -33.2601,
			46.3758, -31.6947,
		};
		static const unsigned num_points2 = sizeof(points2) / sizeof(points2[0]);
		GeoTimeInstant geo_time_instant_begin2(83.5);
		GeoTimeInstant geo_time_instant_end2 =
				GeoTimeInstant::create_distant_future();
		UnicodeString description2("SOUTHWEST INDIAN RIDGE, MADAGASCAR-ANTARCTICA ANOMALY 34 ISOCHRON");
		UnicodeString name2("Ozzy the Isochron");
		UnicodeString codespace_of_name2("EarthByte");
	
		FeatureHandle::weak_ref isochron2 =
				create_isochron(model, isochrons, plate_id2, points2, num_points2,
						geo_time_instant_begin2, geo_time_instant_end2,
						description2, name2, codespace_of_name2);
	
		static const unsigned long plate_id3 = 511;
		// lon, lat, lon, lat... is how GML likes it.
		static const double points3[] = {
			76.6320, -18.1374,
			77.9538, -19.1216,
			77.7709, -19.4055,
			80.1582, -20.6289,
			80.3237, -20.3765,
			81.1422, -20.7506,
			80.9199, -21.2669,
			81.8522, -21.9828,
		};
		static const unsigned num_points3 = sizeof(points3) / sizeof(points3[0]);
		GeoTimeInstant geo_time_instant_begin3(40.1);
		GeoTimeInstant geo_time_instant_end3 =
				GeoTimeInstant::create_distant_future();
		UnicodeString description3("SEIR CROZET AND CIB, CENTRAL INDIAN BASIN-ANTARCTICA ANOMALY 18 ISOCHRON");
		UnicodeString name3("Uzi the Isochron");
		UnicodeString codespace_of_name3("EarthByte");
	
		FeatureHandle::weak_ref isochron3 =
				create_isochron(model, isochrons, plate_id3, points3, num_points3,
						geo_time_instant_begin3, geo_time_instant_end3,
						description3, name3, codespace_of_name3);
	
		static const unsigned long fixed_plate_id1 = 511;
		static const unsigned long moving_plate_id1 = 501;
		static const TotalReconstructionPoleData five_tuples1[] = {
			//	time	e.lat	e.lon	angle	comment
			{	0.0,	90.0,	0.0,	0.0,	"IND-CIB India-Central Indian Basin"	},
			{	9.9,	-8.7,	76.9,	2.75,	"IND-CIB AN 5 JYR 7/4/89"	},
			{	20.2,	-5.2,	74.3,	5.93,	"IND-CIB Royer & Chang 1991"	},
			{	83.5,	-5.2,	74.3,	5.93,	"IND-CIB switchover"	},
		};
		static const unsigned num_five_tuples1 = sizeof(five_tuples1) / sizeof(five_tuples1[0]);
	
		FeatureHandle::weak_ref total_recon_seq1 =
				create_total_recon_seq(model, total_recon_seqs, fixed_plate_id1,
						moving_plate_id1, five_tuples1, num_five_tuples1);
	
		static const unsigned long fixed_plate_id2 = 702;
		static const unsigned long moving_plate_id2 = 501;
		static const TotalReconstructionPoleData five_tuples2[] = {
			//	time	e.lat	e.lon	angle	comment
			{	83.5,	22.8,	19.1,	-51.28,	"IND-MAD"	},
			{	88.0,	19.8,	27.2,	-59.16,	" RDM/chris 30/11/2001"	},
			{	120.4,	24.02,	32.04,	-53.01,	"IND-MAD M0 RDM 21/01/02"	},
		};
		static const unsigned num_five_tuples2 = sizeof(five_tuples2) / sizeof(five_tuples2[0]);
	
		FeatureHandle::weak_ref total_recon_seq2 =
				create_total_recon_seq(model, total_recon_seqs, fixed_plate_id2,
						moving_plate_id2, five_tuples2, num_five_tuples2);
	
		static const unsigned long fixed_plate_id3 = 501;
		static const unsigned long moving_plate_id3 = 502;
		static const TotalReconstructionPoleData five_tuples3[] = {
			//	time	e.lat	e.lon	angle	comment
			{	0.0,	0.0,	0.0,	0.0,	"SLK-IND Sri Lanka-India"	},
			{	75.0,	0.0,	0.0,	0.0,	"SLK-ANT Sri Lanka-Ant"	},
			{	90.0,	21.97,	72.79,	-10.13,	"SLK-IND M9 FIT CG01/04-"	},
			{	129.5,	21.97,	72.79,	-10.13,	"SLK-IND M9 FIT CG01/04-for sfs in Enderby"	},
		};
		static const unsigned num_five_tuples3 = sizeof(five_tuples3) / sizeof(five_tuples3[0]);
	
		FeatureHandle::weak_ref total_recon_seq3 =
				create_total_recon_seq(model, total_recon_seqs, fixed_plate_id3,
						moving_plate_id3, five_tuples3, num_five_tuples3);
	}
}

GPlatesGui::Document::Document()
{
	setupUi(this);

	d_timer_ptr = new QTimer(this);
	connect(d_timer_ptr, SIGNAL(timeout()), this, SLOT(animation_step()));

	d_model_ptr = new GPlatesModel::Model();
	create_everything(*d_model_ptr, d_isochrons, d_total_recon_seqs);
	//reconstruct_fake_model(model, d_isochrons, d_total_recon_seqs, 0);
	
	d_canvas_ptr = new GlobeCanvas(this);
	QObject::connect(d_canvas_ptr, SIGNAL(items_selected()), this, SLOT(selection_handler()));
	QObject::connect(d_canvas_ptr, SIGNAL(left_mouse_button_clicked()), this, SLOT(mouse_click_handler()));
	
	centralwidget = d_canvas_ptr;
	setCentralWidget(centralwidget);
}

void 
GPlatesGui::Document::selection_handler(
		std::vector< GlobeCanvas::line_header_type > &items)
{
	// Do nothing here for this demo: see the method below.
	// Or: Use InformationDialog here.
}

void
GPlatesGui::Document::mouse_click_handler()
{
	d_time = 100.0;
	if (d_timer_ptr->isActive()) {
		return;
	}
	const int timerInterval = 25;
	d_timer_ptr->start(timerInterval);
}

void
GPlatesGui::Document::animation_step()
{
	// Move the animation on by a fraction
	if (d_time <= 80) {
		d_timer_ptr->stop();
		statusbar->clearMessage();
		return;
	}
	d_time -= 0.1;
	const QString message(boost::str(boost::format("%1% MYA") % d_time).c_str());
	statusbar->showMessage(message);
	d_canvas_ptr->clear_data();
	render_model(d_canvas_ptr, d_model_ptr, d_isochrons, d_total_recon_seqs, d_time);
	d_canvas_ptr->update_canvas();
}
