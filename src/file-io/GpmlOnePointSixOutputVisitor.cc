/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "GpmlOnePointSixOutputVisitor.h"

#include <vector>
#include <utility>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QtCore/QUuid>
#include <QtGlobal> 
#include <QProcess>
#include <boost/bind.hpp>

#include "ErrorOpeningFileForWritingException.h"
#include "ErrorOpeningPipeToGzipException.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/XmlNamespaces.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/TemplateTypeParameterType.h"
#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPointConversions.h"


const GPlatesFileIO::ExternalProgram
		GPlatesFileIO::GpmlOnePointSixOutputVisitor::s_gzip_program("gzip", "gzip --version");


namespace
{
	/**
	 * Replace the q_file_ptr's filename with a unique temporary .gpml filename.
	 */
	void
	set_temporary_filename(
		boost::shared_ptr<QFile> q_file_ptr)
	{
		if (!q_file_ptr)
		{
			return;
		}
		// I'm using a uuid to generate a unique name... is this overkill?
		QUuid uuid = QUuid::createUuid();
		QString uuid_string = uuid.toString();

		// The uuid string has braces around it, so remove them. 
		uuid_string.remove(0,1);
		uuid_string.remove(uuid_string.length()-1,1);

		QFileInfo file_info(*q_file_ptr);

		// And don't forget to put ".gpml" at the end. 
		q_file_ptr->setFileName(file_info.absolutePath() + QDir::separator() + uuid_string + ".gpml");
	}


	/**
	 * If the filename of the QFile ends in ".gz", this will be stripped from the QFile's filename. 
	 */
	void
	remove_gz_from_filename(
		boost::shared_ptr<QFile> q_file_ptr)
	{
		if (!q_file_ptr){
			return;
		}

		QString file_name = q_file_ptr->fileName();

		QString gz_string(".gz");

		if (file_name.endsWith(gz_string))
		{
			file_name.remove(file_name.length()-gz_string.length(),gz_string.length());
			q_file_ptr->setFileName(file_name);
		}
	}

	typedef std::pair<
		GPlatesModel::XmlAttributeName, 
		GPlatesModel::XmlAttributeValue> 
			XmlAttribute;

	template< typename QualifiedNameType >
	void
	writeTemplateTypeParameterType(
			GPlatesFileIO::XmlWriter &writer,
			const QualifiedNameType &value_type)
	{
		boost::optional<const UnicodeString &> alias = 
			value_type.get_namespace_alias();

		UnicodeString prefix;

		if (alias) 
		{
			// XXX:  This namespace declaration is a hack to get around the
			// fact that we can't access the current namespace declarations
			// from QXmlStreamWriter.  It ensures that the namespace of the 
			// QualifiedNameType about to be written has been
			// declared.
			writer.writeNamespace(
					GPlatesUtils::make_qstring_from_icu_string(value_type.get_namespace()),
					GPlatesUtils::make_qstring_from_icu_string(*alias));

			prefix = *alias;
		}
		else 
		{
			prefix = writer.getAliasForNamespace(value_type.get_namespace_iterator());
		}

		writer.writeText(prefix + ":" + value_type.get_name());
	}
	
	
	/**
	 * Convenience function to help write GmlPolygon's exterior and interior rings.
	 */
	void
	write_gml_linear_ring(
			GPlatesFileIO::XmlWriter &xml_output,
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr)
	{
		xml_output.writeStartGmlElement("LinearRing");
	
		static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > 
			pos_list_xml_attrs;
	
		if (pos_list_xml_attrs.empty()) 
		{
			GPlatesModel::XmlAttributeName attr_name =
				GPlatesModel::XmlAttributeName::create_gml("dimension");
			GPlatesModel::XmlAttributeValue attr_value("2");
			pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
		}
		// FIXME: srsName?
		xml_output.writeStartGmlElement("posList");
		xml_output.writeAttributes(
				pos_list_xml_attrs.begin(),
				pos_list_xml_attrs.end());
	
		// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
		// were to create an iterator which performed the following transformation for us
		// automatically, but (i) that's probably not the most efficient use of our time right now;
		// (ii) it's file I/O, it's slow anyway; and (iii) we can cut it down to a single memory
		// allocation if we reserve the size of the vector in advance.
		std::vector<double> pos_list;
		// Reserve enough space for the coordinates, to avoid the need to reallocate.
		//
		// number of coords = 
		//   (one for each segment start-point, plus one for the final end-point
		//   (all other end-points are the start-point of the next segment, so are not counted)),
		//   times two, since each point is a (lat, lon) duple.
		pos_list.reserve((polygon_ptr->number_of_segments() + 1) * 2);
	
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator begin = polygon_ptr->vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = begin;
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon_ptr->vertex_end();
		for ( ; iter != end; ++iter) 
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
	
			// NOTE: We are assuming GPML is using (lat,lon) ordering.
			// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
			pos_list.push_back(llp.latitude());
			pos_list.push_back(llp.longitude());
		}
		// When writing gml:Polygons, the last point must be identical to the first point,
		// because the format wasn't verbose enough.
		GPlatesMaths::LatLonPoint begin_llp = GPlatesMaths::make_lat_lon_point(*begin);
		pos_list.push_back(begin_llp.latitude());
		pos_list.push_back(begin_llp.longitude());
		
		// Now that we have assembled the coordinates, write them into the XML.
		xml_output.writeNumericalSequence(pos_list.begin(), pos_list.end());
	
		// Don't forget to clear the vector when we're done with it!
		pos_list.clear();
	
		xml_output.writeEndElement(); // </gml:posList>
		xml_output.writeEndElement(); // </gml:LinearRing>
	}


	/**
	 * Convenience function to help write GmlPoint and GmlMultiPoint.
	 */
	void
	write_gml_point(
			GPlatesFileIO::XmlWriter &xml_output,
			const GPlatesMaths::PointOnSphere &point)
	{
		xml_output.writeStartGmlElement("Point");
			xml_output.writeStartGmlElement("pos");

				GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
				// NOTE: We are assuming GPML is using (lat,lon) ordering.
				// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
				xml_output.writeDecimalPair(llp.latitude(), llp.longitude());

			xml_output.writeEndElement();  // </gml:pos>
		xml_output.writeEndElement();  // </gml:Point>
	}


}


GPlatesFileIO::GpmlOnePointSixOutputVisitor::GpmlOnePointSixOutputVisitor(
		const FileInfo &file_info,
		bool use_gzip):
	d_qfile_ptr(new QFile(file_info.get_qfileinfo().filePath())),
	d_qprocess_ptr(new QProcess),
	d_gzip_afterwards(false),
	d_output_filename(file_info.get_qfileinfo().filePath())
{

// If we're on a Windows system, we have to treat the whole gzip thing differently. 
//	The approach under Windows is:
//	1. Produce uncompressed output.
//	2. Gzip it.
//
//  To prevent us from overwriting any existing uncompressed file which the user may want preserved,
//  we generate a temporary gpml file name. The desired output file name is stored in the member variable
//  "d_output_filename". 
//
//	To implement this we do the following:
//	1. If we're on Windows, AND compressed output is requested, then
//		a. Set the "d_gzip_afterwards" flag to true. (We need to check the status of this flag in the destructor
//			where we'll actually do the gzipping).
//		b. Set "use_gzip" to false, so that uncompressed output will be produced.
//		c. Replace the d_qfile_ptr's filename with a temporary .gpml filename. 
//
//	2. In the destructor, (i.e. after the uncompressed file has been written), check if the 
//		"d_gzip_afterwards" flag is set. If so, gzip the uncompressed file, and rename it to 
//		"d_output_filename". If not, do the normal wait-for-process-to-end stuff. 
//																	
#ifdef Q_WS_WIN
	if (use_gzip) {
		d_gzip_afterwards = true;
		use_gzip = false;
		set_temporary_filename(d_qfile_ptr);
	}
#endif

	if ( ! d_qfile_ptr->open(QIODevice::WriteOnly | QIODevice::Text)) {
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				file_info.get_qfileinfo().filePath());
	}

	if (use_gzip) {
		// Using gzip; We already opened the file using d_qfile_ptr, but that's okay,
		// since it verifies that we can actually write to that location.
		// Now we've verified that, we can close the file.
		d_qfile_ptr->close();
		
		// Set up the gzip process. Just like the QFile output, we need to keep it
		// as a shared_ptr belonging to this class.
		d_qprocess_ptr->setStandardOutputFile(file_info.get_qfileinfo().filePath());
		// FIXME: Assuming gzip is in a standard place on the path. Not true on MS/Win32. Not true at all.
		// In fact, it may need to be a user preference.
		d_qprocess_ptr->start(s_gzip_program.command());
		if ( ! d_qprocess_ptr->waitForStarted()) {
			throw ErrorOpeningPipeToGzipException(s_gzip_program.command(), file_info.get_qfileinfo().filePath());
		}
		// Use the newly-launched process as the device the XML writer writes to.
		d_output.setDevice(d_qprocess_ptr.get());
		
	} else {
		// Not using gzip, just write to the file as normal.
		d_output.setDevice(d_qfile_ptr.get());
		
	}
	
	start_writing_document(d_output);
}


GPlatesFileIO::GpmlOnePointSixOutputVisitor::GpmlOnePointSixOutputVisitor(
		QIODevice *target):
	d_qfile_ptr(),
	d_qprocess_ptr(),
	d_output(target),
	d_gzip_afterwards(false)
{
	start_writing_document(d_output);
}


void
	GPlatesFileIO::GpmlOnePointSixOutputVisitor::write_feature(
	const GPlatesModel::FeatureHandle& feature_handle)
{
	feature_handle.accept_visitor(*this);
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::start_writing_document(
		XmlWriter &writer)
{
	writer.writeStartDocument();

	writer.writeNamespace(
			GPlatesUtils::XmlNamespaces::GPML_NAMESPACE, 
			GPlatesUtils::XmlNamespaces::GPML_STANDARD_ALIAS);
	writer.writeNamespace(
			GPlatesUtils::XmlNamespaces::GML_NAMESPACE, 
			GPlatesUtils::XmlNamespaces::GML_STANDARD_ALIAS);
	writer.writeNamespace(
			GPlatesUtils::XmlNamespaces::XSI_NAMESPACE, 
			GPlatesUtils::XmlNamespaces::XSI_STANDARD_ALIAS);

	writer.writeStartGpmlElement("FeatureCollection");

	writer.writeGpmlAttribute("version", "1.6");
	writer.writeAttribute(
			GPlatesUtils::XmlNamespaces::XSI_NAMESPACE,
			"schemaLocation",
			"http://www.gplates.org/gplates ../xsd/gpml.xsd "\
			"http://www.opengis.net/gml ../../../gml/current/base");
}


GPlatesFileIO::GpmlOnePointSixOutputVisitor::~GpmlOnePointSixOutputVisitor()
{
	// Wrap the entire body of the function in a 'try {  } catch(...)' block so that no
	// exceptions can escape from the destructor.
	try {
		d_output.writeEndElement(); // </gpml:FeatureCollection>
		d_output.writeEndDocument();


		// d_gzip_aftewards should only be true if we're on Windows, and compressed output was requested. 
		if (d_gzip_afterwards && d_qfile_ptr)
		{
			// Do the zipping now, but close the file first.
			d_output.device()->close();

			QStringList args;

			args << d_qfile_ptr->fileName();

			// The temporary gpml filename, and hence the corresponding .gpml.gz name, should be unique, 
			// so we can just go ahead and compress the file. 
			QProcess::execute(s_gzip_program.command(),args);
			
			// The requested output file may exist. If that is the case, at this stage the user has already
			// confirmed that the file be overwritten. We can't rename a file if a file with the new name
			// already exists, so remove it. 
			if (QFile::exists(d_output_filename))
			{
				QFile::remove(d_output_filename);
			}
			QString gz_filename = d_qfile_ptr->fileName() + ".gz";
			QFile::rename(gz_filename,d_output_filename);
		}
		else
		{
			// If we were using gzip compression, we must wait for the process to finish.
			if (d_qprocess_ptr.get() != NULL) {
				// in fact, we need to forcibly close the input to gzip, because
				// if we wait for it to go out of scope to clean itself up, there seems
				// to be a bit of data left in a buffer somewhere - either ours, or gzip's.
				d_qprocess_ptr->closeWriteChannel();
				d_qprocess_ptr->waitForFinished();
			}
		}
	}
	catch (...) {
		// Nothing we can really do in here, I think... unless we want to log that we
		// smothered an exception.  However, if we DO want to log that we smothered an
		// exception, we need to wrap THAT code in a try-catch block also, to ensure that
		// THAT code can't throw an exception which escapes the destructor.
	}
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Every feature must be wrapped in a "gml:featureMember" element.
	d_output.writeStartGmlElement("featureMember");
		bool pop = d_output.writeStartElement(feature_handle.feature_type());

			d_output.writeStartGpmlElement("identity");
				d_output.writeText(feature_handle.feature_id().get());
			d_output.writeEndElement();

			d_output.writeStartGpmlElement("revision");
				d_output.writeText(feature_handle.revision_id().get());
			d_output.writeEndElement();

			// Now visit each of the properties in turn.
			visit_feature_properties(feature_handle);

		d_output.writeEndElement(pop); // </gpml:SomeFeature>
	d_output.writeEndElement(); // </gml:featureMember>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	d_last_property_seen = inline_property_container.property_name();

	bool pop = d_output.writeStartElement(inline_property_container.property_name());
		d_output.writeAttributes(
			inline_property_container.xml_attributes().begin(),
			inline_property_container.xml_attributes().end());

		visit_property_values(inline_property_container);
	d_output.writeEndElement(pop);
}



void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	d_output.writeText(enumeration.value().get());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_output.writeStartGmlElement("LineString");

	static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > 
		pos_list_xml_attrs;

	if (pos_list_xml_attrs.empty()) 
	{
		GPlatesModel::XmlAttributeName attr_name =
			GPlatesModel::XmlAttributeName::create_gml("dimension");
		GPlatesModel::XmlAttributeValue attr_value("2");
		pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
	}
	d_output.writeStartGmlElement("posList");
	d_output.writeAttributes(
			pos_list_xml_attrs.begin(),
			pos_list_xml_attrs.end());

	// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
	// were to create an iterator which performed the following transformation for us
	// automatically, but (i) that's probably not the most efficient use of our time right now;
	// (ii) it's file I/O, it's slow anyway; and (iii) we can cut it down to a single memory
	// allocation if we reserve the size of the vector in advance.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.polyline();
	std::vector<double> pos_list;
	// Reserve enough space for the coordinates, to avoid the need to reallocate.
	//
	// number of coords = 
	//   (one for each segment start-point, plus one for the final end-point
	//   (all other end-points are the start-point of the next segment, so are not counted)),
	//   times two, since each point is a (lat, lon) duple.
	pos_list.reserve((polyline_ptr->number_of_segments() + 1) * 2);

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_ptr->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_ptr->vertex_end();
	for ( ; iter != end; ++iter) 
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);

		// NOTE: We are assuming GPML is using (lat,lon) ordering.
		// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
		pos_list.push_back(llp.latitude());
		pos_list.push_back(llp.longitude());
	}
	d_output.writeNumericalSequence(pos_list.begin(), pos_list.end());

	// Don't forget to clear the vector when we're done with it!
	pos_list.clear();

	d_output.writeEndElement(); // </gml:posList>
	d_output.writeEndElement(); // </gml:LineString>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point) 
{
	d_output.writeStartGmlElement("MultiPoint");

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint_ptr =
			gml_multi_point.multipoint();

	GPlatesMaths::MultiPointOnSphere::const_iterator iter = multipoint_ptr->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multipoint_ptr->end();
	for ( ; iter != end; ++iter) 
	{
		d_output.writeStartGmlElement("pointMember");
		write_gml_point(d_output, *iter);
		d_output.writeEndElement(); // </gml:pointMember>
	}

	d_output.writeEndElement(); // </gml:MultiPoint>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve) 
{
	d_output.writeStartGmlElement("OrientableCurve");
		d_output.writeAttributes(
				gml_orientable_curve.xml_attributes().begin(),
				gml_orientable_curve.xml_attributes().end());

		d_output.writeStartGmlElement("baseCurve");
			gml_orientable_curve.base_curve()->accept_visitor(*this);
		d_output.writeEndElement();  // </gml:base_curve>

	d_output.writeEndElement();  // </gml:OrientableCurve>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point) 
{
	write_gml_point(d_output, *gml_point.point());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	d_output.writeStartGmlElement("Polygon");
	
	// GmlPolygon has exactly one exterior ring.
	d_output.writeStartGmlElement("exterior");
	write_gml_linear_ring(d_output, gml_polygon.exterior());
	d_output.writeEndElement(); // </gml:exterior>
	
	// GmlPolygon has zero or more interior rings.
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator it = gml_polygon.interiors_begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
	for ( ; it != end; ++it) {
		d_output.writeStartGmlElement("interior");
		write_gml_linear_ring(d_output, *it);
		d_output.writeEndElement(); // </gml:interior>
	}

	d_output.writeEndElement(); // </gml:Polygon>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant) 
{
	d_output.writeStartGmlElement("TimeInstant");
	d_output.writeStartGmlElement("timePosition");
		d_output.writeAttributes(
				gml_time_instant.time_position_xml_attributes().begin(),
				gml_time_instant.time_position_xml_attributes().end());

		const GPlatesPropertyValues::GeoTimeInstant &time_position = 
			gml_time_instant.time_position();
		if (time_position.is_real()) {
			d_output.writeDecimal(time_position.value());
		} else if (time_position.is_distant_past()) {
			d_output.writeText(QString("http://gplates.org/times/distantPast"));
		} else if (time_position.is_distant_future()) {
			d_output.writeText(QString("http://gplates.org/times/distantFuture"));
		}

	d_output.writeEndElement();  // </gml:timePosition>
	d_output.writeEndElement();  // </gml:TimeInstant>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period) 
{
	d_output.writeStartGmlElement("TimePeriod");
		d_output.writeStartGmlElement("begin");
			gml_time_period.begin()->accept_visitor(*this);
		d_output.writeEndElement();

		d_output.writeStartGmlElement("end");
			gml_time_period.end()->accept_visitor(*this);
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gml:TimePeriod>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_polarity_chron_id(
		const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id) 
{
	d_output.writeStartGpmlElement("PolarityChronId");
	if (gpml_polarity_chron_id.get_era()) {
		d_output.writeStartGpmlElement("era");
		d_output.writeText(*gpml_polarity_chron_id.get_era());
		d_output.writeEndElement();
	}
	if (gpml_polarity_chron_id.get_major_region()) {
		d_output.writeStartGpmlElement("major");
		d_output.writeInteger(*gpml_polarity_chron_id.get_major_region());
		d_output.writeEndElement();
	}
	if (gpml_polarity_chron_id.get_minor_region()) {
		d_output.writeStartGpmlElement("minor");
		d_output.writeText(*gpml_polarity_chron_id.get_minor_region());
		d_output.writeEndElement();
	}
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	d_output.writeStartGpmlElement("ConstantValue");
		d_output.writeStartGpmlElement("value");
			gpml_constant_value.value()->accept_visitor(*this);
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_constant_value.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_feature_reference(
		const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
{
	d_output.writeStartGpmlElement("FeatureReference");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_feature_reference.feature_id().get());
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_feature_reference.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_feature_snapshot_reference(
		const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference)
{
	d_output.writeStartGpmlElement("FeatureSnapshotReference");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_feature_snapshot_reference.feature_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("targetRevision");
			d_output.writeText(gpml_feature_snapshot_reference.revision_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_feature_snapshot_reference.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_property_delegate(
		const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
{
	d_output.writeStartGpmlElement("PropertyDelegate");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_property_delegate.feature_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("targetProperty");
			writeTemplateTypeParameterType(d_output, gpml_property_delegate.target_property());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_property_delegate.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation) 
{
	if (gpml_finite_rotation.is_zero_rotation()) {
		d_output.writeEmptyGpmlElement("ZeroFiniteRotation");
	} else {
		d_output.writeStartGpmlElement("AxisAngleFiniteRotation");

			d_output.writeStartGpmlElement("eulerPole");
				GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
						GPlatesPropertyValues::calculate_euler_pole(gpml_finite_rotation);
				visit_gml_point(*gml_point);
			d_output.writeEndElement();

			d_output.writeStartGpmlElement("angle");
				GPlatesMaths::real_t angle_in_degrees =
						GPlatesPropertyValues::calculate_angle(gpml_finite_rotation);
				d_output.writeDecimal(angle_in_degrees.dval());
			d_output.writeEndElement();

		d_output.writeEndElement();  // </gpml:AxisAngleFiniteRotation>
	}
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{
	d_output.writeStartGpmlElement("FiniteRotationSlerp");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_finite_rotation_slerp.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	d_output.writeStartGpmlElement("KeyValueDictionary");
		//d_output.writeStartGpmlElement("elements");
			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
				iter = gpml_key_value_dictionary.elements().begin(),
				end = gpml_key_value_dictionary.elements().end();
			for ( ; iter != end; ++iter) {
				d_output.writeStartGpmlElement("element");
				write_gpml_key_value_dictionary_element(*iter);
				d_output.writeEndElement();
			}
		//d_output.writeEndElement();
	d_output.writeEndElement();
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	d_output.writeStartGpmlElement("PiecewiseAggregation");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_piecewise_aggregation.value_type());
		d_output.writeEndElement();

		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
				gpml_piecewise_aggregation.time_windows().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
				gpml_piecewise_aggregation.time_windows().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeWindow");
				write_gpml_time_window(*iter);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:IrregularSampling>
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_topological_polygon(
	const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
	d_output.writeStartGpmlElement("TopologicalPolygon");
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::const_iterator iter;
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::const_iterator end;
	iter = gpml_toplogical_polygon.sections().begin();
	end = gpml_toplogical_polygon.sections().end();

	for ( ; iter != end; ++iter) 
	{
		d_output.writeStartGpmlElement("section");
		(*iter)->accept_visitor(*this);
		d_output.writeEndElement();
	}
	d_output.writeEndElement();  // </gpml:TopologicalPolygon>
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_topological_line_section(
	const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	d_output.writeStartGpmlElement("TopologicalLineSection");

		d_output.writeStartGpmlElement("sourceGeometry");
			// visit the delgate 
			( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 
		d_output.writeEndElement();

		// boost::optional<GpmlTopologicalIntersection>
		if ( gpml_toplogical_line_section.get_start_intersection() )
		{
			d_output.writeStartGpmlElement("startIntersection");
				gpml_toplogical_line_section.get_start_intersection()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		if ( gpml_toplogical_line_section.get_end_intersection() )
		{
			d_output.writeStartGpmlElement("endIntersection");
				gpml_toplogical_line_section.get_end_intersection()->accept_visitor(*this);
			d_output.writeEndElement();
		}
		
		d_output.writeStartGpmlElement("reverseOrder");
			d_output.writeBoolean( gpml_toplogical_line_section.get_reverse_order() );
		d_output.writeEndElement();

	d_output.writeEndElement();
}




void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_topological_intersection(
	const GPlatesPropertyValues::GpmlTopologicalIntersection &gpml_toplogical_intersection)
{
	d_output.writeStartGpmlElement("TopologicalIntersection");

		d_output.writeStartGpmlElement("intersectionGeometry");
			// visit the delegate
			gpml_toplogical_intersection.intersection_geometry()->accept_visitor(*this); 
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referencePoint");
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
					gpml_toplogical_intersection.reference_point();
			visit_gml_point(*gml_point);
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referencePointPlateId");
			// visit the delegate
			gpml_toplogical_intersection.reference_point_plate_id()->accept_visitor(*this);
		d_output.writeEndElement();

	d_output.writeEndElement();
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_topological_point(
	const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	d_output.writeStartGpmlElement("TopologicalPoint");
		d_output.writeStartGpmlElement("sourceGeometry");
			// visit the delegate
			( gpml_toplogical_point.get_source_geometry() )->accept_visitor(*this); 
		d_output.writeEndElement();
	d_output.writeEndElement();  
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_hot_spot_trail_mark(
		const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark)
{
	d_output.writeStartGpmlElement("HotSpotTrailMark");
		d_output.writeStartGpmlElement("position");
			gpml_hot_spot_trail_mark.position()->accept_visitor(*this);
		d_output.writeEndElement();

		if (gpml_hot_spot_trail_mark.trail_width()) {
			d_output.writeStartGpmlElement("trailWidth");
				(*gpml_hot_spot_trail_mark.trail_width())->accept_visitor(*this);
			d_output.writeEndElement();
		}
		if (gpml_hot_spot_trail_mark.measured_age()) {
			d_output.writeStartGpmlElement("measuredAge");
				(*gpml_hot_spot_trail_mark.measured_age())->accept_visitor(*this);
			d_output.writeEndElement();
		}
		if (gpml_hot_spot_trail_mark.measured_age_range()) {
			d_output.writeStartGpmlElement("measuredAgeRange");
				(*gpml_hot_spot_trail_mark.measured_age_range())->accept_visitor(*this);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:HotSpotTrailMark>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	d_output.writeAttributes(
			gpml_measure.quantity_xml_attributes().begin(),
			gpml_measure.quantity_xml_attributes().end());
	d_output.writeDecimal(gpml_measure.quantity());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::write_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	d_output.writeStartGpmlElement("TimeWindow");
		d_output.writeStartGpmlElement("timeDependentPropertyValue");
			gpml_time_window.time_dependent_value()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("validTime");
			gpml_time_window.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_window.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gpml:TimeWindow>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	d_output.writeStartGpmlElement("IrregularSampling");
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
				gpml_irregular_sampling.time_samples().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
				gpml_irregular_sampling.time_samples().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeSample");
				write_gpml_time_sample(*iter);
			d_output.writeEndElement();
		}

		// The interpolation function is optional.
		if (gpml_irregular_sampling.interpolation_function() != NULL)
		{
			d_output.writeStartGpmlElement("interpolationFunction");
				gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_irregular_sampling.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();  // </gpml:IrregularSampling>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_output.writeInteger(gpml_plate_id.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_revision_id(
		const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id)
{
	d_output.writeText(gpml_revision_id.value().get());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::write_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample) 
{
	d_output.writeStartGpmlElement("TimeSample");
		d_output.writeStartGpmlElement("value");
			gpml_time_sample.value()->accept_visitor(*this);
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("validTime");
			gpml_time_sample.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();

		// The description is optional.
		if (gpml_time_sample.description() != NULL) 
		{
			d_output.writeStartGmlElement("description");
				gpml_time_sample.description()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_sample.value_type());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:TimeSample>
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_output.writeStartGpmlElement("OldPlatesHeader");

		d_output.writeStartGpmlElement("regionNumber");
 		d_output.writeInteger(gpml_old_plates_header.region_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referenceNumber");
 		d_output.writeInteger(gpml_old_plates_header.reference_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("stringNumber");
 		d_output.writeInteger(gpml_old_plates_header.string_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("geographicDescription");
 		d_output.writeText(gpml_old_plates_header.geographic_description());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("plateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfAppearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_appearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfDisappearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_disappearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCode");
 		d_output.writeText(gpml_old_plates_header.data_type_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumber");
 		d_output.writeInteger(gpml_old_plates_header.data_type_code_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumberAdditional");
 		d_output.writeText(gpml_old_plates_header.data_type_code_number_additional());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("conjugatePlateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.conjugate_plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("colourCode");
 		d_output.writeInteger(gpml_old_plates_header.colour_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("numberOfPoints");
 		d_output.writeInteger(gpml_old_plates_header.number_of_points());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:OldPlatesHeader>
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	d_output.writeText(xs_string.value().get());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_uninterpreted_property_value(
		const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val)
{
	// XXX: Uncomment to indicate which property values weren't interpreted.
	//d_output.get_writer().writeEmptyElement("Uninterpreted");
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = 
		uninterpreted_prop_val.value();

	for_each(elem->children_begin(), elem->children_end(),
			boost::bind(&GPlatesModel::XmlNode::write_to, _1, 
				boost::ref(d_output.get_writer())));
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	d_output.writeBoolean(xs_boolean.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_double(
		const GPlatesPropertyValues::XsDouble &xs_double)
{
	d_output.writeDecimal(xs_double.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_integer(
		const GPlatesPropertyValues::XsInteger &xs_integer)
{
	d_output.writeInteger(xs_integer.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::write_gpml_key_value_dictionary_element(
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
{
	d_output.writeStartGpmlElement("KeyValueDictionaryElement");
		d_output.writeStartGpmlElement("key");
			element.key()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(
				d_output,
				element.value_type());
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("value");
			element.value()->accept_visitor(*this);
		d_output.writeEndElement();
	d_output.writeEndElement();
}
