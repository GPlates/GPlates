/* $Id: PlatesRotationFormatReader.cc 12376 2011-10-05 05:09:50Z jcannon $ */

/**
 * @file
 * Contains the definitions of the member functions of the class PlatesRotationFileProxy.
 *
 * Most recent change:
 *   $Date: 2011-10-05 16:09:50 +1100 (Wed, 05 Oct 2011) $
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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
#include <limits>
#include <boost/foreach.hpp>

#include <sstream>
#include <string>
#include <QBuffer>
#include <QFile>

#include "PlatesRotationFileProxy.h"

#include "global/LogException.h"
#include "model/ModelUtils.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlMetadata.h"
#include "property-values/GpmlTotalReconstructionPole.h"
#include "property-values/GpmlKeyValueDictionary.h"

const char GPlatesFileIO::RotationFileReaderV2::COMMENT_LEADING_CHARACTER = '#';
const char GPlatesFileIO::RotationFileReaderV2::ATTRIBUTE_LEADING_CHARACTER = '@';
const char GPlatesFileIO::RotationFileReaderV2::MPRS_HEADER_LEADING_CHARACTER = '>';
const char GPlatesFileIO::RotationFileReaderV2::ATTR_VALUE_SEPARATOR = '\"' ;
const char GPlatesFileIO::RotationFileReaderV2::SUB_ATTR_VALUE_SEPARATOR = '|' ;
const QString GPlatesFileIO::RotationFileReaderV2::ATTR_LONG_VALUE_SEPARATOR = "\"\"\"";

const QString GPlatesFileIO::RotationFileReaderV2::COMMENT_LINE_REGEXP = "^\\s*#";
const QString GPlatesFileIO::RotationFileReaderV2::ROTATION_POLE_REGEXP =
	"^\\s*#?\\s*(\\d+)\\s+(\\d+|\\d+\\.\\d*)\\s+(-?\\d+|-?\\d+\\.\\d*)\\s+(-?\\d+|-?\\d+\\.\\d*)\\s+(-?\\d+|-?\\d+\\.\\d*)\\s+(\\d+)";
const QString GPlatesFileIO::RotationFileReaderV2::ATTRIBUTE_LINE_REGEXP = "^\\s*@";
const QString GPlatesFileIO::RotationFileReaderV2::ATTRIBUTE_REGEXP = 	"@([^\"]+)\"([^\"]+)\"";
const QString GPlatesFileIO::RotationFileReaderV2::MULTI_LINE_ATTR_REGEXP = 	"@([^\"]+)\"{3}([^\"]+)\"{3}";
const QString GPlatesFileIO::RotationFileReaderV2::MPRS_HEADER_REGEXP = "^\\s*>";

namespace
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;
	using namespace GPlatesFileIO;

	class GetGpmlFiniteRotations :
		public GPlatesModel::ConstFeatureVisitor
	{
	public:
		void
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling)
		{
			BOOST_FOREACH(const GpmlTimeSample& sample, gpml_irregular_sampling.time_samples())
			{
				const GpmlFiniteRotation* fr = dynamic_cast<const GpmlFiniteRotation*>(sample.value().get());
				if(fr)
				{
					d_finite_rotations.push_back(fr);
				}
			}
		}
		
		std::vector<const GpmlFiniteRotation*>
		gpml_finite_rotations()
		{
			return d_finite_rotations;
		}
	private:
		std::vector<const GpmlFiniteRotation*> d_finite_rotations;;
	};


	std::vector<const GpmlFiniteRotation*>
	get_finite_rotations(	
			FeatureCollectionHandle::weak_ref fc)
	{
		GetGpmlFiniteRotations visitor;
		for(FeatureCollectionHandle::iterator it = fc->begin(); it != fc->end(); it++)
		{
			visitor.visit_feature(it);
		}
		return visitor.gpml_finite_rotations();
	}


	template<class SegmentType>
	std::vector<SegmentType*>
	filter(	
			const RotationFileSegmentContainer& segs)
	{
		std::vector<SegmentType*> ret;
		BOOST_FOREACH(boost::shared_ptr<RotationFileSegment> seg, segs)
		{
			SegmentType* pole = dynamic_cast<SegmentType*>(seg.get());
			if(pole)
			{
				ret.push_back(pole);
			}
		}
		return ret;
	}

	bool
	operator==(
			const RotationPoleSegment& pole_seg,
			const GpmlFiniteRotation& gpml_rot)
	{
		
		return true;
	}

	bool
	operator!=(
			const RotationPoleSegment& pole_seg,
			const GpmlFiniteRotation& gpml_rot)
	{
		return !(pole_seg == gpml_rot);
	}

	bool
	operator!=(
			const GpmlFiniteRotation& gpml_rot,
			const RotationPoleSegment& pole_seg)
	{
		return !(pole_seg == gpml_rot);
	}

	struct Modifications
	{
		std::vector<RotationPoleSegment*>  deleted;
		std::vector<const GpmlFiniteRotation*> added;
		std::vector<RotationPoleSegment*> modified;
	};


	int 
	find(
			const std::vector<RotationPoleSegment*>& segs,
			const GpmlFiniteRotation* gpml_fr)
	{
		int ret = -1, count = -1;
		BOOST_FOREACH(RotationPoleSegment* pole_seg, segs)
		{
			count++;
			if(pole_seg->finite_rotation() == gpml_fr)
			{
				ret = count;
				break;
			}
		}
		return ret;
	}

	int 
	find(
		const std::vector<const GpmlFiniteRotation*>& gpml_frs,
		const RotationPoleSegment* rotation_seg)
	{
		int ret = -1, count = -1;
		BOOST_FOREACH(const GpmlFiniteRotation* fr, gpml_frs)
		{
			count++;
			if(rotation_seg->finite_rotation() == fr)
			{
				ret = count;
				break;
			}
		}
		return ret;
	}


	Modifications
	check_modification(
			const RotationFileSegmentContainer& segs,
			FeatureCollectionHandle::weak_ref fc)
	{
		Modifications ret;
		std::vector<RotationPoleSegment*> poles = filter<RotationPoleSegment>(segs);
		std::vector<const GpmlFiniteRotation*> frs = get_finite_rotations(fc);
		BOOST_FOREACH(RotationPoleSegment* pole_seg, poles)
		{
			int idx = find(frs, pole_seg);
			if(-1 != idx)
			{
				if(*frs[idx] != *pole_seg)
				{
					ret.modified.push_back(pole_seg);
				}
				else
				{
					continue;
				}
			}
			else
			{
				ret.deleted.push_back(pole_seg);
			}
		}

		BOOST_FOREACH(const GpmlFiniteRotation* fr, frs)
		{
			int idx = find(poles, fr);
			if(-1 == idx)
			{
				ret.added.push_back(fr);
			}
		}

		return ret;
	}
}

const double GPlatesFileIO::PlatesRotationFileProxy::ROTATION_EPSILON = 1.0e-6;

void
GPlatesFileIO::PlatesRotationFileProxy::check_version()
{
	//qWarning() << "TODO: Check the rotation file format version.";
	d_version = TWO;
}


void
GPlatesFileIO::PlatesRotationFileProxy::init(
		File::Reference &file_ref)
{
	d_file_info = file_ref.get_file_info();
	d_feature_collection = file_ref.get_feature_collection();
	
	check_version();
	create_file_reader();
	d_reader_ptr->read(d_file_info.get_qfileinfo(), d_feature_collection);
}


GPlatesFileIO::RotationFileReaderV2::RotationFileReaderV2() :
	d_commnet_line_rx(COMMENT_LINE_REGEXP),
	d_pole_rx(ROTATION_POLE_REGEXP),
	d_attr_rx(ATTRIBUTE_LINE_REGEXP),
	d_multi_line_attr_rx(MULTI_LINE_ATTR_REGEXP),
	d_mprs_header_rx(MPRS_HEADER_REGEXP),
	d_last_moving_pid(0),
	d_processing_mprs(false)
{
	d_function_map[&d_commnet_line_rx] = &RotationFileReaderV2::process_comment;
	d_regexp_vec.push_back(&d_commnet_line_rx);
	d_function_map[&d_pole_rx] = &RotationFileReaderV2::process_rotation_pole_line;
	d_regexp_vec.push_back(&d_pole_rx);
	d_function_map[&d_attr_rx] = &RotationFileReaderV2::process_attribute_line;
	d_regexp_vec.push_back(&d_attr_rx);
	d_function_map[&d_mprs_header_rx] = &RotationFileReaderV2::process_mprs_header_line;
	d_regexp_vec.push_back(&d_mprs_header_rx);
}


void
GPlatesFileIO::RotationFileReader::read_file(
		File::Reference &file,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{

	const boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> & cfg = 
		file.get_file_configuration();

	if(!cfg)
	{
		return;
	}

	const FeatureCollectionFileFormat::RotationFileConfiguration* rot_file_cfg_const = 
		dynamic_cast<const FeatureCollectionFileFormat::RotationFileConfiguration*>((*cfg).get());
	FeatureCollectionFileFormat::RotationFileConfiguration* rot_file_cfg = 
		const_cast<FeatureCollectionFileFormat::RotationFileConfiguration*>(rot_file_cfg_const);

	PlatesRotationFileProxy& file_proxy = rot_file_cfg->get_rotation_file_proxy();

	file_proxy.init(file);
		
	RotationFileSegmentContainer file_segments = file_proxy.get_segments();
	PopulateReconstructionFeatureCollection visitor(file.get_feature_collection());
	BOOST_FOREACH(boost::shared_ptr<RotationFileSegment> seg, file_segments)
	{
		seg->accept_visitor(visitor);
		//std::cout << seg->to_qstring().toStdString();
	}
	visitor.finalize();

	//file_proxy.attach_callback();
}



void
GPlatesFileIO::RotationFileReaderV2::read(
			const QFileInfo& q_file_info,
			GPlatesModel::FeatureCollectionHandle::weak_ref fch)
{
	//qDebug() << "Enter GPlatesFileIO::RotationFileReaderV2::read()";
	QFile rot_file(q_file_info.absoluteFilePath());
	if (!rot_file.open(QFile::ReadOnly | QFile::Text))
	{
		qWarning() << "Failed to open file for reading -- " + q_file_info.absoluteFilePath();
	}
	QString next_line = peek_next_line(rot_file);

	while(!next_line.isEmpty())
	{
		RegExpVector::iterator begin = d_regexp_vec.begin(), end = d_regexp_vec.end();
		for(;begin != end; begin++)
		{
			if( -1 != (*begin)->indexIn(next_line))
			{
				(this->*d_function_map[*begin])(rot_file, d_segmetns);
				break;
			}
		}
		if(begin == end)
		{
			process_arbitrary_text(rot_file, d_segmetns);
		}
		next_line = peek_next_line(rot_file);
	}
	d_segmetns.push_back(
			boost::shared_ptr<RotationFileSegment>(
					new TextSegment("",true,false))); //We put a separator at the end of file.
}


void
GPlatesFileIO::RotationFileReaderV2::process_comment(
		QIODevice& file, 
		RotationFileSegmentContainer& container)
{
 	QString buf = peek_next_line(file);
	if(is_valid_rotation_pole_line(buf))
	{
		//This comment line is a disabled pole.
		process_rotation_pole_line(file, d_segmetns);
	}

	QString comment = file.readLine();
	container.push_back(boost::shared_ptr<RotationFileSegment>(new CommentSegment(comment)));
//	qDebug() <<"Comments: \n" + comment; 
	return;
}


void
GPlatesFileIO::RotationFileReaderV2::process_attribute_line(
		QIODevice& file, 
		RotationFileSegmentContainer& container)
{
	QString buf = file.readLine();
	
	//if the attribute is multi-line, read until reach the second separator.
	if(-1 != buf.indexOf(ATTR_LONG_VALUE_SEPARATOR))
	{
		QString line = file.readLine();
		buf += line;
		while(-1 == line.indexOf(ATTR_LONG_VALUE_SEPARATOR))
		{
			line = file.readLine();
			buf += line;
		}
	}

	//if the line ends with "\"(backslash), we treat it as a line continuation marker.
	while(buf.simplified().endsWith("\\"))
	{
		QByteArray tmp = file.readLine();
		if(tmp.isEmpty())
		{
			break;
		}
		else
		{
			buf += tmp;
		}
	}

	while(-1 != buf.indexOf(ATTRIBUTE_LEADING_CHARACTER))
	{
		QRegExp rx (ATTRIBUTE_REGEXP);
		bool is_multi_line_attr=false;
		int idx = rx.indexIn(buf);
		if(-1 == idx)// no simple attribute
		{
			rx = d_multi_line_attr_rx;
			idx = rx.indexIn(buf);
			if(-1 == idx) // no multiple line attribute either
				break;
			else
				is_multi_line_attr = true;
		}

		//get the matched strings
		QString attr_str = rx.cap(0);
		QString attr_name = rx.cap(1);
		QString attr_value = rx.cap(2);
		container.push_back(boost::shared_ptr<RotationFileSegment>(
				new AttributeSegment(attr_name, attr_value, is_multi_line_attr)));

		//looking for the index of next attribute. 
		int end_idx = idx+attr_str.length();
		int next_attr_idx = buf.indexOf(ATTRIBUTE_LEADING_CHARACTER, end_idx);
		
		if(-1 == next_attr_idx) // if there is no more attribute
		{
			next_attr_idx = buf.length();
		}

		//creat a TextSegment for the text between two attributes.
		if(next_attr_idx > end_idx)
		{
			container.push_back(boost::shared_ptr<RotationFileSegment>(
				new TextSegment(buf.mid(end_idx, next_attr_idx-end_idx))));
		}

		//set all processed text to blank spaces 
		if(next_attr_idx > idx)
		{
			buf.replace(idx, (next_attr_idx-idx),QChar(' '));
		}
	}
	
	//push back the last text segment if there is any
	if(!buf.simplified().isEmpty())
	{
		container.push_back(boost::shared_ptr<
			RotationFileSegment>(new TextSegment(buf.simplified())));
	}
	return;
}


void
GPlatesFileIO::RotationFileReaderV2::process_mprs_header_line(
		QIODevice& file, 
		RotationFileSegmentContainer& container)
{
	if(!d_processing_mprs)
	{
		container.push_back(
				boost::shared_ptr<RotationFileSegment>(
						new TextSegment("",true,false))); //insert an invisible separator before the MPRS header
	}
	d_processing_mprs = true;
	while('>' != file.read(1)[0]){}
	RotationFileSegmentContainer tmp;
	process_attribute_line(file, tmp);

	container.push_back(
			boost::shared_ptr<RotationFileSegment>(
					new MPRSHeaderLineSegment(tmp)));
	return;
}

void
GPlatesFileIO::RotationFileReaderV2::process_rotation_pole_line(
		QIODevice& file, 
		RotationFileSegmentContainer& container)
{
	d_processing_mprs = false;
	QString line = file.readLine();
	RotationPoleData data;
	parse_rotation_pole_line(line, data);

	line.remove(0, data.text.length());

	RotationFileSegmentContainer tmp;
	int attr_idx = line.indexOf(ATTRIBUTE_LEADING_CHARACTER);
	if((-1 != attr_idx) && (0 != attr_idx))
	{
		tmp.push_back(boost::shared_ptr<RotationFileSegment>(
			new TextSegment(line.left(attr_idx))));
		line.remove(0,attr_idx);
	}
	
	QByteArray attr_data = line.toUtf8();
	QBuffer buf(&attr_data);
	buf.open(QBuffer::ReadWrite);
	process_attribute_line(buf,tmp);
	tmp.insert( 
			tmp.begin(),
			boost::shared_ptr<RotationFileSegment>(
					new RotationPoleSegment(data)));

	container.push_back(
			boost::shared_ptr<RotationFileSegment>(
					new RotationPoleLine(tmp)));
	return;
}

void
GPlatesFileIO::RotationFileReaderV2::process_arbitrary_text(
		QIODevice& file, 
		RotationFileSegmentContainer& container)
{
	QString buf = file.readLine();
	
	container.push_back(boost::shared_ptr<RotationFileSegment>(new TextSegment(buf)));
	if(!buf.simplified().isEmpty()) 
		qWarning() <<"Unrecognized text found: \n" + buf; 
	return;
}


QString
GPlatesFileIO::RotationFileReaderV2::peek_next_line(
		QIODevice& file)
{
	qint64 current_pos = file.pos();
	QString ret = file.readLine();
	file.seek(current_pos);
	return QString(ret);
}


bool
GPlatesFileIO::RotationFileReaderV2::is_valid_rotation_pole_line(
		const QString& str)
{
	QRegExp rx(ROTATION_POLE_REGEXP);
	if(-1 == rx.indexIn(str))
		return false;
	else
		return true;
}

bool
GPlatesFileIO::RotationFileReaderV2::parse_rotation_pole_line(
		const QString& str,
		RotationPoleData& data)
{
	QString line = str.simplified();
	//if the line starts with 'COMMENT_LEADING_CHARACTER', the pole is disabled.
	if(line.startsWith(COMMENT_LEADING_CHARACTER))
		data.disabled = true;

	QRegExp rx(ROTATION_POLE_REGEXP);
	
	if(-1 == rx.indexIn(str))
		return false;
	
	data.text = rx.cap(0);
	data.moving_plate_id = rx.cap(1).toInt(); 
	data.time = rx.cap(2).toDouble();
	data.lat = rx.cap(3).toDouble();
	data.lon = rx.cap(4).toDouble();
	data.angle = rx.cap(5).toDouble();
	data.fix_plate_id = rx.cap(6).toInt();
	return true;
}


void
GPlatesFileIO::TextSegment::accept_visitor(
		RotationFileSegmentVisitor& v) 
{
	v.visit(*this);
}


void
GPlatesFileIO::CommentSegment::accept_visitor(
		RotationFileSegmentVisitor& v) 
{
	v.visit(*this);
	return;
}


void
GPlatesFileIO::AttributeSegment::accept_visitor(
		RotationFileSegmentVisitor& v) 
{
	v.visit(*this);
	return;
}


void
GPlatesFileIO::RotationPoleSegment::accept_visitor(
		RotationFileSegmentVisitor& v) 
{
	v.visit(*this);
	return;
}


void
GPlatesFileIO::LineSegment::accept_visitor(
		RotationFileSegmentVisitor& v)
{
	for(
		RotationFileSegmentContainer::iterator i = d_sub_segments.begin(); 
		i != d_sub_segments.end(); 
		i++)
		{
			(*i)->accept_visitor(v);
		}
	return;
}


void
GPlatesFileIO::MPRSHeaderLineSegment::accept_visitor(
		RotationFileSegmentVisitor& v)
{
	v.visit(*this);
	return;
}


const QString
GPlatesFileIO::MPRSHeaderLineSegment::to_qstring() 
{
	QString ret;
	QString attributes_str;
	
	BOOST_FOREACH(const RotationFileSegmentContainer::value_type s, d_sub_segs)
	{
		attributes_str += s->to_qstring();
	}
	if(!attributes_str.simplified().isEmpty())
	{
		ret = d_leading_char + " " + attributes_str;
	}
	if(d_end_with_new_line && !ret.simplified().isEmpty() && !ret.endsWith("\n"))
	{
		ret += "\n";
	}
	return ret;
}


int
GPlatesFileIO::MPRSHeaderLineSegment::get_pid() const
{
	BOOST_FOREACH(const RotationFileSegmentContainer::value_type s, d_sub_segs)
	{
		AttributeSegment *attr = dynamic_cast<AttributeSegment*>(s.get());
		if(attr)
		{
			if(attr->get_name() == "MPRS:pid")
			{
				return attr->get_value().toInt();
			}
		}
	}
	return 0;
}


void
GPlatesFileIO::PopulateReconstructionFeatureCollection::visit(
		RotationPoleSegment& seg) 
{
	const RotationPoleData& data = seg.data();
	//qDebug() << seg.to_qstring();
	if(!validate_pole(data))
	{
		return;
	}
	if(!d_current_feature.is_valid() || is_new_trs(d_last_pole,data))
	{
		create_new_trs_feature(data.moving_plate_id, data.fix_plate_id);
	}
	
	if(d_current_sampling)
	{
		d_current_sample = create_time_sample(data);
		GPlatesModel::PropertyValue::non_null_ptr_type fr = d_current_sample->value();
		seg.set_finite_rotation(fr.get());
	}
		
	d_last_pole =data;
}


void
GPlatesFileIO::PopulateReconstructionFeatureCollection::visit(
		RotationPoleLine& seg)
{
	if(d_current_sample)
	{
		GPlatesPropertyValues::GpmlTotalReconstructionPole* trp = 
			dynamic_cast<GPlatesPropertyValues::GpmlTotalReconstructionPole*>(d_current_sample->value().get());
		if(trp)
		{
			std::vector<boost::shared_ptr<GPlatesModel::Metadata> >& meta = trp->metadata();
			BOOST_FOREACH(const AttributeSegment&attr, d_attrs)
			{
				meta.push_back(
						boost::shared_ptr<GPlatesModel::Metadata>(
								new GPlatesModel::PoleMetadata(attr.get_name(), attr.get_value())));
			}
			d_attrs.clear();
		}
		(*d_current_sampling)->time_samples().push_back(*d_current_sample);
	}

}


void
GPlatesFileIO::PopulateReconstructionFeatureCollection::visit(
		AttributeSegment& s) 
{
	if(d_fc_metadata.set_metadata(s.get_name(),s.get_value()))
	{
		return;
	}
	else
	{
		d_attrs.push_back(s);
	}
}


void
GPlatesFileIO::PopulateReconstructionFeatureCollection::visit(
		MPRSHeaderLineSegment& s) 
{
	RotationFileSegmentContainer sub_segs = s.get_sub_segments();
	for(
		RotationFileSegmentContainer::iterator i = sub_segs.begin(); 
		i != sub_segs.end(); 
		i++)
	{
		if(AttributeSegment* attr = dynamic_cast<AttributeSegment*>((*i).get()))
		{
			GPlatesPropertyValues::XsString::non_null_ptr_type key = 
				GPlatesPropertyValues::XsString::create(
						GPlatesUtils::make_icu_string_from_qstring(attr->get_name()));

			GPlatesPropertyValues::XsString::non_null_ptr_type value = 
				GPlatesPropertyValues::XsString::create(
						GPlatesUtils::make_icu_string_from_qstring(attr->get_value()));
				
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
			d_mprs_attrs.push_back(element);
		}
	}
	return;
}


bool
GPlatesFileIO::PopulateReconstructionFeatureCollection::validate_pole(
		const RotationPoleData& current,
		boost::optional<const RotationPoleData&> pre)
{
	if(current.fix_plate_id == current.moving_plate_id)
	{
		qWarning() << "moving plate id equals fixed plate id. ignore this pole.";
		return false;
	}
	if ( ! GPlatesMaths::LatLonPoint::is_valid_latitude(current.lat))
	{
		qWarning() << "invalid latitude.";
		return false;
	}
	if ( ! GPlatesMaths::LatLonPoint::is_valid_longitude(current.lon))
	{
		qWarning() << "invalid longitude.";
		return false;
	}
	if(pre)
	{
		if(
			(current.moving_plate_id == pre->moving_plate_id) && 
			(current.fix_plate_id == pre->fix_plate_id)		&& 
			(current.time <= pre->time))
		{
			qWarning() << "overlap rotation poles. ignore this pole.";
			return false;
		}
	}
	return true;
}

GPlatesPropertyValues::GpmlTimeSample
GPlatesFileIO::PopulateReconstructionFeatureCollection::create_time_sample(
		const RotationPoleData& data)
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;

	std::pair<double, double> lon_lat_euler_pole(data.lon, data.lat);

	GpmlTotalReconstructionPole::non_null_ptr_type trp(
		new GpmlTotalReconstructionPole(GpmlFiniteRotation::create(
				lon_lat_euler_pole, 
				data.angle)->finite_rotation()));

	GeoTimeInstant geo_time_instant(data.time);
	GmlTimeInstant::non_null_ptr_type valid_time =
		ModelUtils::create_gml_time_instant(geo_time_instant);

	boost::intrusive_ptr<XsString> description;

// 	TemplateTypeParameterType value_type = 
// 		TemplateTypeParameterType::create_gpml("FiniteRotation");
	StructuralType value_type = 
		StructuralType::create_gpml("FiniteRotation");

	if (data.disabled) {
		return GpmlTimeSample(trp, valid_time, description, value_type, true);
	} else {
		return GpmlTimeSample(trp, valid_time, description, value_type);
	}
}

void
GPlatesFileIO::PopulateReconstructionFeatureCollection::create_new_trs_feature(
		const GPlatesModel::integer_plate_id_type &moving_plate_id,
		const GPlatesModel::integer_plate_id_type &fix_plate_id)
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;

	if(d_current_feature.is_valid())
	{
		//make sure the sampling for last feature is saved.
		d_current_feature->add(
				TopLevelPropertyInline::create(
						PropertyName::create_gpml("totalReconstructionPole"),
						*d_current_sampling));
	}

	GpmlTimeSample time_sample = create_time_sample(RotationPoleData());//dummy sample

	//create feature
	FeatureType feature_type = FeatureType::create_gpml("TotalReconstructionSequence");
	d_current_feature = GPlatesModel::FeatureHandle::create(d_fc, feature_type);

	//create GpmlIrregularSampling
	GpmlInterpolationFunction::non_null_ptr_type gpml_finite_rotation_slerp =
		GpmlFiniteRotationSlerp::create(
				time_sample.value_type());
	d_current_sampling =
		GpmlIrregularSampling::create(
				time_sample,
				GPlatesUtils::get_intrusive_ptr(gpml_finite_rotation_slerp),
				time_sample.value_type());
	
	//add fixed reference frame
	GpmlPlateId::non_null_ptr_type fixed_ref_frame =
		GpmlPlateId::create(fix_plate_id);
	d_current_feature->add(
			TopLevelPropertyInline::create(
			PropertyName::create_gpml("fixedReferenceFrame"),
			fixed_ref_frame));

	//add moving reference frame
	GpmlPlateId::non_null_ptr_type moving_ref_frame =
		GpmlPlateId::create(moving_plate_id);
	d_current_feature->add(
			TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("movingReferenceFrame"),
					moving_ref_frame));
	
	GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
		GpmlKeyValueDictionary::create();
	
	if(moving_plate_id == static_cast<unsigned long>(d_last_pole.moving_plate_id))
	{
		d_mprs_attrs = d_last_mprs;
	}
	if(d_mprs_attrs.empty())
	{
		XsString::non_null_ptr_type key = XsString::create(
				GPlatesUtils::make_icu_string_from_qstring("MPRS:pid"));

		XsString::non_null_ptr_type value = XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(
						QString().setNum(moving_plate_id)));

		GpmlKeyValueDictionaryElement element(
				key,
				value,
				StructuralType::create_xsi("string"));
		d_mprs_attrs.push_back(element);
	}
	dictionary->elements() = d_mprs_attrs;
	d_last_mprs = d_mprs_attrs;
	d_mprs_attrs.clear();
	if(dictionary->num_elements() > 0)
	{
		d_current_feature->add(
				TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("mprsAttributes"),
						dictionary));
	}
	
	(*d_current_sampling)->time_samples().clear(); //clear dummy sample
}


bool
GPlatesFileIO::PopulateReconstructionFeatureCollection::is_new_trs(
		const RotationPoleData& pre,
		const RotationPoleData& current)
{
	if (
		(pre.moving_plate_id != current.moving_plate_id) ||
		(pre.fix_plate_id != current.fix_plate_id)) 
	{
		return true;
	}
	return false;
}


void
GPlatesFileIO::PopulateReconstructionFeatureCollection::finalize()
{
	if(d_current_feature.is_valid())
	{
		d_current_feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("totalReconstructionPole"),
						*d_current_sampling));
		
		//reset the feature weak ref after the final clean up.
		d_current_feature = GPlatesModel::FeatureHandle::weak_ref();
	}
	//create a FeatureCollectionMetadata feature.
	FeatureType feature_type = FeatureType::create_gpml("FeatureCollectionMetadata");
	d_fc_metadata_feature = GPlatesModel::FeatureHandle::create(d_fc, feature_type);
	d_fc_metadata_feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("metadata"),
					GPlatesPropertyValues::GpmlMetadata::create(d_fc_metadata)));

}

void
GPlatesFileIO::GrotWriterWithCfg::finalise_post_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	qDebug() << "finalise_post_feature_properties in RotationFileWriter";
	const boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> cfg = 
		d_file_ref.get_file_configuration();
	if(cfg)
	{
		boost::shared_ptr<const FeatureCollectionFileFormat::RotationFileConfiguration> rotation_cfg_const =
			boost::dynamic_pointer_cast<const FeatureCollectionFileFormat::RotationFileConfiguration>(*cfg);
		boost::shared_ptr<FeatureCollectionFileFormat::RotationFileConfiguration> rotation_cfg =
			boost::const_pointer_cast< FeatureCollectionFileFormat::RotationFileConfiguration>(rotation_cfg_const);
		if(rotation_cfg)
		{
			rotation_cfg->get_rotation_file_proxy().save_feature(feature_handle,d_file_ref);
		}

	}
}


void
GPlatesFileIO::RotationPoleLine::accept_visitor(
		RotationFileSegmentVisitor& v) 
{
	BOOST_FOREACH(boost::shared_ptr<RotationFileSegment> seg, d_sub_segments)
	{
		seg->accept_visitor(v);
	}
	v.visit(*this);
}


void
GPlatesFileIO::GrotWriterWithoutCfg::visit_gpml_metadata(
		const GPlatesPropertyValues::GpmlMetadata &gpml_metadata)
{
	QString buf;
	gpml_metadata.get_data().serialize(buf);
	d_output->seekp(0);
	
	std::ifstream ifs(d_file_ref.get_file_info().get_qfileinfo().filePath().toStdString().c_str());
	std::string str(
			(std::istreambuf_iterator<char>(ifs)),
			std::istreambuf_iterator<char>());
	(*d_output) << buf.toUtf8().data();
	(*d_output) << str;
}


void
GPlatesFileIO::GrotWriterWithoutCfg::visit_gpml_key_value_dictionary(
	const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{

}


void
GPlatesFileIO::GrotWriterWithoutCfg::visit_gpml_total_reconstruction_pole(
	const GPlatesPropertyValues::GpmlTotalReconstructionPole &trs)
{
	const std::vector<boost::shared_ptr<GPlatesModel::Metadata> >& metadata = trs.metadata();
	for(std::size_t i=0; i<metadata.size(); i++)
	{
		d_accum.current_pole().metadata.push_back(metadata[i]);
	}
	visit_gpml_finite_rotation(trs);
}


bool
GPlatesFileIO::GrotWriterWithoutCfg::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	using namespace GPlatesModel;
	static const FeatureType
		gpmlTotalReconstructionSequence = FeatureType::create_gpml("TotalReconstructionSequence"),
		gpmlAbsoluteReferenceFrame = FeatureType::create_gpml("AbsoluteReferenceFrame"),
		metadata = FeatureType::create_gpml("FeatureCollectionMetadata");

	if ((feature_handle.feature_type() != gpmlTotalReconstructionSequence)
		&& (feature_handle.feature_type() != gpmlAbsoluteReferenceFrame)
		&& (feature_handle.feature_type() != metadata)) {
			// These are not the features you're looking for.
			return false;
	}

	//write out MPRS(Moving Plate Rotation Sequence) metadata
	try
	{
		using namespace GPlatesPropertyValues;
		GpmlKeyValueDictionary::non_null_ptr_type mprs_values = 
		ModelUtils::get_mprs_attributes(feature_handle.reference());
		BOOST_FOREACH(const GpmlKeyValueDictionaryElement& element, mprs_values->elements())
		{
			QString output_str;
			const XsString *key_val = element.key().get(),
				*val = dynamic_cast<const XsString*>(element.value().get());
			
			//Check if the MPRS metadata has already been written out. If so, skip this iteration.
			if("MPRS:pid" == key_val->value().get().qstring())
			{
				if(val->value().get().qstring().toUInt() == d_mprs_id)
				{
					return true;
				}
				else
				{
					d_mprs_id = val->value().get().qstring().toUInt();
				}
			}
			
			QString content = val->value().get().qstring(), sep = "\"";
			if(content.contains("\n"))
			{
				sep = "\"\"\"";
			}
			output_str += QString(QString("> @%1") + sep + "%2" + sep +"\n").
					arg(key_val->value().get().qstring()).
					arg(val->value().get().qstring());
			
			(*d_output) << output_str.toUtf8().data();
		}
	}
	catch(GPlatesGlobal::LogException& e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
	// Reset the accumulator.
	d_accum = PlatesRotationFormatAccumulator();

	return true;
}


GPlatesFileIO::RotationFileSegmentContainer&
GPlatesFileIO::PlatesRotationFileProxy::get_segments() 
{
	if(d_reader_ptr)
	{
		return d_reader_ptr->get_segments();
	}
	else
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			"Rotation file reader has not been initialized yet.");
	}
}


void
GPlatesFileIO::PlatesRotationFileProxy::update_header_metadata(
		GPlatesModel::FeatureCollectionMetadata fc_meta)
{
	using namespace GPlatesModel;
	std::multimap<QString, QString> meta_map = fc_meta.get_metadata_as_map(); //The new data set.
	std::multimap<QString, QString>::iterator 
		map_it = meta_map.begin(), map_it_end = meta_map.end();
	MetadataContainer tmp_container;
	for(; map_it != map_it_end; map_it++)
	{
		tmp_container.push_back(
				Metadata::shared_ptr_type(new Metadata(map_it->first, map_it->second)));
	}
	RotationFileSegmentContainer header_attrbutes;
	BOOST_FOREACH(RotationFileSegmentContainer::value_type seg, get_segments())
	{
		if(dynamic_cast<MPRSHeaderLineSegment *>(seg.get()) ||
			dynamic_cast<RotationPoleLine *>(seg.get()))
		{
			break;
		}
		header_attrbutes.push_back(seg);
	}
	try
	{
		tmp_container = update_attributes_and_return_new(tmp_container,header_attrbutes);
		if(!tmp_container.empty())
		{
			RotationFileSegmentContainer &segments = get_segments();
			BOOST_FOREACH(MetadataContainer::value_type meta, tmp_container) // for each new attribute
			{
				RotationFileSegmentContainer::iterator 
					it = segments.begin(), end = segments.end(), 
					pos_to_insert = end, last_same_attr = end, last_header_attr = segments.begin();
				// first, try to find the attributes with the same name.
				for(; it!=end; it++) 
				{
					AttributeSegment *a_ptr = dynamic_cast<AttributeSegment *>(it->get());
					if(a_ptr)
					{
						if(a_ptr->get_name() == meta->get_name())
						{
							last_same_attr = it;
						}
						else
						{
							last_header_attr = it;
						}
					}
					if(dynamic_cast<MPRSHeaderLineSegment *>(it->get()) ||
						dynamic_cast<RotationPoleLine *>(it->get()))
					{
						break; // this means we have searched the entire header metadata section, 
						// no need to process further.
					}
				}

				if(last_same_attr != end)
				{
					pos_to_insert = last_same_attr;
				}
				else
				{
					//if there is no entries with the same name,
					//put it at the end of header entries.
					pos_to_insert = last_header_attr;
				}
				while(pos_to_insert != end && pos_to_insert != segments.begin())
				{
					pos_to_insert++;
					if(!(*pos_to_insert)->to_qstring().simplified().isEmpty())
					{
						break;
					}
				}
				segments.insert(
						pos_to_insert,
						boost::shared_ptr<RotationFileSegment>(
								new AttributeSegment(
										meta->get_name(),
										meta->get_content(),
										false,true))); 
			}
		}
	}
	catch(GPlatesGlobal::LogException &e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


void
GPlatesFileIO::PlatesRotationFileProxy::update_MPRS_metadata(
		GPlatesModel::MetadataContainer mprs_only_data,
		GPlatesModel::MetadataContainer default_pole_data,
		const QString &moving_plate_id)
{
	using namespace GPlatesModel;
	try
	{
		RotationFileSegmentContainer::iterator 
			mprs_begin_iter, 
			mprs_end_iter;
		boost::tie(mprs_begin_iter, mprs_end_iter) = get_mprs_range(moving_plate_id);
		
		MetadataContainer	
			mprs_only_new_data(mprs_only_data), 
			default_pole_new_data(default_pole_data);

		if(std::distance(mprs_begin_iter,mprs_end_iter)>0) 
		{
			RotationFileSegmentContainer::iterator 
				iter = mprs_begin_iter, 
				fist_default_pole_attr = mprs_begin_iter ;
			const RotationMetadataRegistry &reg = PlatesRotationFileProxy::get_metadata_registry();
			bool mprs_only = true;
			for(; iter != mprs_end_iter; iter++)
			{
				MPRSHeaderLineSegment *ptr = dynamic_cast<MPRSHeaderLineSegment *>(iter->get());
				if(ptr)
				{
					BOOST_FOREACH(RotationFileSegmentContainer::value_type s,ptr->get_sub_segments())
					{
						AttributeSegment *attr_ptr = dynamic_cast<AttributeSegment *>(s.get()) ;
						if(NULL != attr_ptr )
						{
							if((reg.get(attr_ptr->get_name()).type_flag & MetadataType::POLE) &&
								(attr_ptr->get_name() != "C"))
							{
								mprs_only = false;
								fist_default_pole_attr = iter;
							}
							if(mprs_only)
							{
								update_or_delete_attribute(mprs_only_new_data,*attr_ptr);
							}
							else
							{
								update_or_delete_attribute(default_pole_new_data,*attr_ptr);
							}
						}
					}
				}
			}

			if(!mprs_only_new_data.empty())
			{
				RotationFileSegmentContainer tmp;
				BOOST_FOREACH(MetadataContainer::value_type v, mprs_only_new_data)
				{
					RotationFileSegmentContainer t;
					t.push_back(
							boost::shared_ptr<RotationFileSegment>(
									new AttributeSegment(
											v->get_name(),
											v->get_content())));
					tmp.push_back(
							boost::shared_ptr<RotationFileSegment>(
									new MPRSHeaderLineSegment(t,true)));
				}
				get_segments().insert(fist_default_pole_attr,tmp.begin(),tmp.end());
			}
			
			if(!default_pole_new_data.empty())
			{
				RotationFileSegmentContainer tmp;
				BOOST_FOREACH(MetadataContainer::value_type v, default_pole_new_data)
				{
					RotationFileSegmentContainer t;
					t.push_back(
							boost::shared_ptr<RotationFileSegment>(
									new AttributeSegment(
											v->get_name(),
											v->get_content())));
					tmp.push_back(
							boost::shared_ptr<RotationFileSegment>(
									new MPRSHeaderLineSegment(t,true)));
				}
				get_segments().insert(mprs_end_iter,tmp.begin(),tmp.end());
			}
		}
		else //(std::distance(mprs_begin_iter,mprs_end_iter) == 0) 
		{
			MetadataContainer tmp;
			tmp.reserve(mprs_only_data.size() + default_pole_data.size());
			tmp.insert(tmp.begin(), mprs_only_data.begin(), mprs_only_data.end());
			tmp.insert(tmp.end(),default_pole_data.begin(), default_pole_data.end());
			RotationFileSegmentContainer tmp_s;
			BOOST_FOREACH(MetadataContainer::value_type v, tmp)
			{
				RotationFileSegmentContainer t;
				t.push_back(
						boost::shared_ptr<RotationFileSegment>(
								new AttributeSegment(
										v->get_name(),
										v->get_content())));
				tmp_s.push_back(
						boost::shared_ptr<RotationFileSegment>(
								new MPRSHeaderLineSegment(t,true)));
			}
			get_segments().insert(mprs_end_iter,tmp_s.begin(),tmp_s.end());
		}
	}
	catch(GPlatesGlobal::LogException &e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


boost::tuple<
		RotationFileSegmentContainer::iterator,
		RotationFileSegmentContainer::iterator>
GPlatesFileIO::PlatesRotationFileProxy::get_mprs_range(
				const QString& moving_plate_id)
{
	using namespace GPlatesModel;
	RotationFileSegmentContainer& segments = get_segments();
	RotationFileSegmentContainer::iterator 
		it = segments.begin(),
		end = segments.end(),
		mprs_begin_iter = end,
		mprs_end_iter = end;
	bool found_flag = false, inside_mprs = false;
	for(; it !=end; it++)
	{
		MPRSHeaderLineSegment *mprs_line_ptr = 
			dynamic_cast<MPRSHeaderLineSegment *>(it->get());
			
		if(mprs_line_ptr && !inside_mprs)
		{
			mprs_begin_iter = it;
			inside_mprs = true;
			continue;
		}
		RotationPoleLine *pole_line_ptr = 
			dynamic_cast<RotationPoleLine *>(it->get());
		if(pole_line_ptr)
		{
			inside_mprs = false;
			if(pole_line_ptr->get_rotation_pole_data().moving_plate_id 
				== moving_plate_id.toInt())
			{
				mprs_end_iter = it;
				found_flag = true;
				break;
			}
			mprs_begin_iter = end;
		}
	}
	if(!found_flag)
	{
		mprs_begin_iter = mprs_end_iter = end;
	}
	else if(mprs_begin_iter == end)
	{
		mprs_begin_iter = mprs_end_iter;
	}
	return boost::make_tuple(mprs_begin_iter,mprs_end_iter);
}


void
GPlatesFileIO::PlatesRotationFileProxy::update_pole_metadata(
		const MetadataContainer &metadata,
		const RotationPoleData &pole_data)
{
	using namespace GPlatesModel;
	try
	{
		bool done = false;
		RotationFileSegmentContainer& segments = get_segments();
		RotationFileSegmentContainer::iterator 
			it = segments.begin(),
			end = segments.end(),
			pole_line_iter = end,
			pole_attr_begin = segments.begin();
		for(; it !=end; it++)
		{
			if(dynamic_cast<MPRSHeaderLineSegment *>(it->get()))
			{
				pole_attr_begin = it;
				continue;
			}
			RotationPoleLine *pole_line_ptr = dynamic_cast<RotationPoleLine *>(it->get()) ;

			if(pole_line_ptr)
			{
				if(pole_data == pole_line_ptr->get_rotation_pole_data())
				{
					pole_line_iter = it;
					MetadataContainer new_meta = pole_line_ptr->update_attributes(metadata);
					
					if(std::distance(pole_attr_begin, it) > 0)
					{
						new_meta = update_attributes_and_return_new(
								new_meta, 
								RotationFileSegmentContainer(pole_attr_begin, it));
					}
					
					RotationFileSegmentContainer tmp;
					BOOST_FOREACH(MetadataContainer::value_type meta, new_meta)
					{
						if(!meta->get_content().isEmpty())
						{
							tmp.push_back(
									boost::shared_ptr<RotationFileSegment>(
											new AttributeSegment(
													meta->get_name(), 
													meta->get_content(),
													false,
													true)));
						}
					}
										
					segments.insert(pole_line_iter, tmp.begin(), tmp.end());
					done =true;
					break;
				}
				else
				{
					pole_attr_begin = it;
				}
			}
		}
		if(!done)
		{
			qWarning()<<"Unable to find the reconstruction pole to update metadata.";
		}
	}catch(GPlatesGlobal::LogException &e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


GPlatesFileIO::RotationPoleData
GPlatesFileIO::RotationPoleLine::get_rotation_pole_data() const
{
	RotationFileSegmentContainer::const_iterator 
		i = d_sub_segments.begin(),
		end = d_sub_segments.end(); 
	for(; i != end; i++)
	{
		boost::shared_ptr<const RotationPoleSegment> pole_seg = 
			boost::dynamic_pointer_cast<const RotationPoleSegment>(*i);
		if(NULL != pole_seg)
		{
			return pole_seg->data();
		}
	}
	throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			"Unable to find rotation pole in rotation pole line.");
}


GPlatesFileIO::RotationPoleData&
GPlatesFileIO::RotationPoleLine::get_rotation_pole_data()
{
	RotationFileSegmentContainer::iterator 
		i = d_sub_segments.begin(),
		end = d_sub_segments.end(); 
	for(; i != end; i++)
	{
		boost::shared_ptr<RotationPoleSegment> pole_seg = 
			boost::dynamic_pointer_cast<RotationPoleSegment>(*i);
		if(NULL != pole_seg)
		{
			return pole_seg->data();
		}
	}
	throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,
			"Unable to find rotation pole in rotation pole line. ");
}


GPlatesModel::MetadataContainer
GPlatesFileIO::RotationPoleLine::update_attributes(
		const GPlatesModel::MetadataContainer &metadata)
		
{
	return update_attributes_and_return_new(metadata, d_sub_segments);
}


void
GPlatesFileIO::PlatesRotationFileProxy::insert_pole(
		const RotationPoleData& new_data)
{
	using namespace GPlatesModel;
	try
	{
		RotationFileSegmentContainer& segments = get_segments();
		RotationFileSegmentContainer::iterator 
			it = segments.begin(),
			end = segments.end(),
			mprs_iter = segments.end(),
			first_pole_with_greater_pid = segments.end(),
			position_to_insert = segments.end();
			bool brand_new_seq = false;
		for(; it !=end; it++)
		{
			RotationPoleLine *pole_line_ptr = dynamic_cast<RotationPoleLine*>(it->get()) ;

			if(pole_line_ptr)
			{
				if(new_data.moving_plate_id == pole_line_ptr->get_rotation_pole_data().moving_plate_id)
				{
					if(pole_line_ptr->get_rotation_pole_data().time > new_data.time)
					{
						position_to_insert = it;
						break;
					}
					else
					{
						position_to_insert = it + 1;
					}
				}
				else
				{
					if(new_data.moving_plate_id < pole_line_ptr->get_rotation_pole_data().moving_plate_id)
					{
						if(first_pole_with_greater_pid == end)
						{
							first_pole_with_greater_pid = it; 
							//We reached first pole whose pid is greater than new pole.
						}
					}
				}
			}
			else // (!pole_line_ptr)
			{
				MPRSHeaderLineSegment *mprs_line = dynamic_cast<MPRSHeaderLineSegment*>(it->get());
				if(mprs_line && (mprs_line->get_pid() == new_data.moving_plate_id))
				{
					mprs_iter = it; //We found the mprs header for the new rotation pole.
				}
			}
		} //for loop
		if(position_to_insert == end)
		{
			if(mprs_iter != end)// This means we have dangling mprs header, use it.
			{
				mprs_iter++;
				for(;mprs_iter != end; mprs_iter++)
				{
					TextSegment *ts = dynamic_cast<TextSegment*>(mprs_iter->get());
					if(ts && ts->is_separator())
					{
						position_to_insert = mprs_iter;
						break;
					}
				}
			}
			else if(first_pole_with_greater_pid != end)
			{
				first_pole_with_greater_pid--;
				for(;first_pole_with_greater_pid != segments.begin(); first_pole_with_greater_pid--)
				{
					TextSegment *ts2 = dynamic_cast<TextSegment*>(first_pole_with_greater_pid->get());
					if(ts2 && ts2->is_separator())
					{
						position_to_insert = first_pole_with_greater_pid;
						brand_new_seq = true;
						break;
					}
				}
			}
		}
		std::vector<boost::shared_ptr<RotationFileSegment> > tmp;
		if(brand_new_seq)
		{
			tmp.push_back(
					boost::shared_ptr<RotationFileSegment>(
							new TextSegment("",true,false))); //do not forget the magical separator.
		}
		tmp.push_back(
				boost::shared_ptr<RotationFileSegment>(
						new RotationPoleSegment(new_data)));
		tmp.push_back(
				boost::shared_ptr<TextSegment>(
						new TextSegment("\n")));
		boost::shared_ptr<RotationPoleLine> line(new RotationPoleLine(tmp));
		segments.insert(position_to_insert, line);

	}catch(GPlatesGlobal::LogException &e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


void
GPlatesFileIO::PlatesRotationFileProxy::update_pole(
		const RotationPoleData& old_pole,
		const RotationPoleData& new_pole)
{
	using namespace GPlatesModel;
	try
	{
		RotationFileSegmentContainer& segments = get_segments();
		RotationFileSegmentContainer::iterator 
			it = segments.begin(),
			end = segments.end();
		for(; it !=end; it++)
		{
			boost::shared_ptr<RotationPoleLine> pole_line_ptr =  
				boost::dynamic_pointer_cast<RotationPoleLine>(*it) ;

			if(pole_line_ptr)
			{
				if(old_pole.moving_plate_id == pole_line_ptr->get_rotation_pole_data().moving_plate_id)
				{
					if(std::fabs(pole_line_ptr->get_rotation_pole_data().time - old_pole.time) 
						< ROTATION_EPSILON)
					{
						pole_line_ptr->get_rotation_pole_data() = new_pole;
						break;
					}
				}
			}
		}
	}catch(GPlatesGlobal::LogException &e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


void
GPlatesFileIO::PlatesRotationFileProxy::delete_pole(
		const RotationPoleData& pole)
{
	using namespace GPlatesModel;
	try
	{
		RotationFileSegmentContainer& segments = get_segments();
		RotationFileSegmentContainer::iterator 
			it = segments.begin(),
			end = segments.end(),
			pre_pole= segments.end();
		for(; it !=end; it++)
		{
			if(boost::dynamic_pointer_cast<MPRSHeaderLineSegment>(*it))
			{
				//for the first pole in a sequence, use the MPRS header line as previous role line.
				pre_pole = it;
				continue;
			}

			boost::shared_ptr<RotationPoleLine> pole_line_ptr =  
				boost::dynamic_pointer_cast<RotationPoleLine>(*it) ;

			if(pole_line_ptr)
			{
				if((pole.moving_plate_id == pole_line_ptr->get_rotation_pole_data().moving_plate_id) &&
					(pole.fix_plate_id == pole_line_ptr->get_rotation_pole_data().fix_plate_id))
				{
					if(std::fabs(pole_line_ptr->get_rotation_pole_data().time - pole.time) 
						< ROTATION_EPSILON)
					{
						if( std::fabs(pole_line_ptr->get_rotation_pole_data().lat - pole.lat)
							> ROTATION_EPSILON  ||
							std::fabs(pole_line_ptr->get_rotation_pole_data().lon - pole.lon)
							> ROTATION_EPSILON ||
							std::fabs(pole_line_ptr->get_rotation_pole_data().angle - pole.angle) 
							> ROTATION_EPSILON)
							{
								qWarning() << "The pole about to be deleted does not match the given pole data, although it should.";
								qWarning() << "To be deleted: " << pole_line_ptr->get_rotation_pole_data().to_string();
								qWarning() << "The given one: " << pole.to_string();
							}
						break;
					}
				}
				pre_pole = it;
			}
		}
		
		if((it != end) && std::distance(pre_pole, it) > 0)
		{
			pre_pole++;
			it++;
			segments.erase(pre_pole, it); //erase all segments between previous pole and current one.
		}
		else
		{
			if(it != end)
			{
				qWarning() << "Unable to find previous pole line. Delete the current one.";
				segments.erase(it);
			}
		}
	}catch(GPlatesGlobal::LogException &e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


void
GPlatesFileIO::PlatesRotationFileProxy::remove_dangling_mprs_header()
{
	RotationFileSegmentContainer tmp_buf_for_rotation_seq, result_buf;
	RotationFileSegmentContainer& segments = get_segments();
	RotationFileSegmentContainer::iterator 
		it = segments.begin(),
		end = segments.end();
	bool inside_rotation_sequence = false;
	bool found_pole_line = false;
	for(; it != end; it++)
	{
		boost::shared_ptr<TextSegment> text_line =  
			boost::dynamic_pointer_cast<TextSegment>(*it);
		if(text_line && text_line->is_separator())
		{
			if(inside_rotation_sequence)
			{
				if(found_pole_line)
				{
					//Found pole data lines between two MPRS header. 
					//The sequence is good. Copy it to result buffer. 
					result_buf.insert(
							result_buf.end(),
							tmp_buf_for_rotation_seq.begin(),
							tmp_buf_for_rotation_seq.end());
				}
				tmp_buf_for_rotation_seq.clear();
				result_buf.insert(result_buf.end(), *it);
				found_pole_line = false;//reset flags.
			}
			else // not inside a rotation sequence.
			{
				// The separator indicates the beginning of a new sequence.
				inside_rotation_sequence = true;
				found_pole_line = false;
				tmp_buf_for_rotation_seq.insert(tmp_buf_for_rotation_seq.end(),*it);
			}
		}
		else // if it is not a separator
		{
			if(inside_rotation_sequence)
			{
				boost::shared_ptr<RotationPoleLine> pole_line =  
					boost::dynamic_pointer_cast<RotationPoleLine>(*it);
				if(pole_line)
				{
					found_pole_line = true;
				}
				tmp_buf_for_rotation_seq.insert(tmp_buf_for_rotation_seq.end(),*it);
			}
			else
			{
				//If it is not inside a rotation sequence, copy it around.
				result_buf.insert(result_buf.end(), *it);
			}
		}
	}
	if(found_pole_line)
	{
		result_buf.insert(
				result_buf.end(),
				tmp_buf_for_rotation_seq.begin(),
				tmp_buf_for_rotation_seq.end());
	}
	segments = result_buf;
	return;
}

GPlatesModel::MetadataContainer
GPlatesFileIO::update_attributes_and_return_new(
		const GPlatesModel::MetadataContainer &new_data,
		const RotationFileSegmentContainer    &file_segs)

{
	using namespace GPlatesModel;
	GPlatesModel::MetadataContainer tmp_new_data(new_data);

	BOOST_FOREACH(RotationFileSegmentContainer::value_type seg, file_segs)
	{
		AttributeSegment *ptr =  dynamic_cast<AttributeSegment *>(seg.get());
		if(ptr)
		{
			update_or_delete_attribute(tmp_new_data,*ptr);
		}
	}
	return tmp_new_data;
}


void
GPlatesFileIO::update_or_delete_attribute(
		GPlatesModel::MetadataContainer &new_data, 
		AttributeSegment &attr)
{
	using namespace GPlatesModel;
	MetadataContainer::iterator iter = find_first_of(attr.get_name(), new_data);

	if(iter != new_data.end())
	{
		attr.get_value() = (*iter)->get_content();
		new_data.erase(iter);
	}
	else
	{
		attr.get_value()="";attr.get_name() = "";
	}
}
