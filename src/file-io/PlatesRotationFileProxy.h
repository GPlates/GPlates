/* $Id: PlatesRotationFormatReader.h 12148 2011-08-18 12:01:47Z jcannon $ */

/**
 * @file
 * Contains the definition of the class PlatesRotationFileProxy.
 *
 * Most recent change:
 *   $Date: 2011-08-18 22:01:47 +1000 (Thu, 18 Aug 2011) $
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

#ifndef GPLATES_FILEIO_PLATESROTATIONFILEPROXY_H
#define GPLATES_FILEIO_PLATESROTATIONFILEPROXY_H

#include <boost/tuple/tuple.hpp>

#include <QTextStream>

#include "File.h"
#include "FileInfo.h"
#include "ErrorOpeningFileForReadingException.h"
#include "PlatesRotationFormatWriter.h"
#include "ReadErrorAccumulation.h"
#include "RotationAttributesRegistry.h"
#include "maths/FiniteRotation.h"
#include "model/FeatureCollectionHandle.h"
#include "model/Metadata.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlMetadata.h"

namespace GPlatesFileIO
{
	class RotationFileSegmentVisitor;

	struct RotationPoleData
	{
		RotationPoleData():
			moving_plate_id(0),
			fix_plate_id(0),
			time(0),
			lat(0),
			lon(0),
			angle(0),
			disabled(false),
			text("")
		{ }

		RotationPoleData(
				const GPlatesMaths::FiniteRotation &fr,
				int m_plate_id,
				int f_plate_id,
				double time_,
				bool disabled_ = false,
				const QString& comment=""):
			moving_plate_id(m_plate_id),
			fix_plate_id(f_plate_id),
			time(time_),
			disabled(disabled_),
			text(comment)
		{
			const GPlatesMaths::UnitQuaternion3D &quat =fr.unit_quat();
			if (GPlatesMaths::represents_identity_rotation(quat)) 
			{
				lat = 0;
				lon = 0;
				angle = 0;
			} 
			else 
			{
				GPlatesMaths::UnitQuaternion3D::RotationParams rot_params =
					quat.get_rotation_params(fr.axis_hint());

				GPlatesMaths::LatLonPoint pole = 
					GPlatesMaths::make_lat_lon_point(GPlatesMaths::PointOnSphere(rot_params.axis));
				lat = pole.latitude();
				lon = pole.longitude();
				angle = GPlatesMaths::convert_rad_to_deg(rot_params.angle.dval());
			}
		}

		bool
		operator==(
				const RotationPoleData& r_data) const 
		{
			return (to_string() == r_data.to_string());
		}

		const QString
		to_string() const 
		{
			QString ret;
			if(disabled)
			{
				ret += "#";
			}
			ret += QString("%1\t").arg(moving_plate_id,3,10,QChar('0'));
			ret += QString("%1\t").arg(time,0,'g',4,QChar('0'));
			ret += QString("%1\t").arg(lat,0,'g',4,QChar('0'));
			ret += QString("%1\t").arg(lon,0,'g',4,QChar('0'));
			ret += QString("%1\t").arg(angle,0,'g',4,QChar('0'));
			ret += QString("%1\t\n").arg(fix_plate_id,3,10,QChar('0'));
			return ret ;
		}

		int moving_plate_id, fix_plate_id;
		double time, lat, lon, angle;
		bool disabled;
		QString text;
	};

	
	class RotationFileSegment
	{
	public:
		virtual
		void
		accept_visitor(
				RotationFileSegmentVisitor&) = 0;

		virtual
		const QString
		to_qstring() = 0;

		virtual
		~RotationFileSegment() { }
	};


	typedef std::vector<boost::shared_ptr<RotationFileSegment> > RotationFileSegmentContainer;
	
	class LineSegment: 
		public RotationFileSegment
	{
	public:
		
		explicit
		LineSegment(
				const RotationFileSegmentContainer& segs) :
			d_sub_segments(segs)
		{	}

		
		explicit
		LineSegment(
				boost::shared_ptr<RotationFileSegment> seg) 
		{
			d_sub_segments.push_back(seg);
		}

		void
		accept_visitor(
				RotationFileSegmentVisitor& v);

		const QString
		to_qstring() 
		{
			
			QString ret;
			for(
				RotationFileSegmentContainer::iterator i = d_sub_segments.begin(); 
				i != d_sub_segments.end(); 
				i++)
			{
				ret += (*i)->to_qstring();
			}

			if(ret.endsWith("\n"))
				return ret;
			else
				return ret + "\n";
		}
	protected:
		RotationFileSegmentContainer d_sub_segments;
	};

	
	class TextSegment: 
		public RotationFileSegment
	{
	public:
		TextSegment(
				const QString& txt,
				bool is_sep = false,
				bool visible = true) :
			d_txt(txt),
			d_is_separator(is_sep),
			d_visible(visible)
		{	}

		void
		accept_visitor(
				RotationFileSegmentVisitor& v);

		bool
		is_separator()
		{
			return d_is_separator;
		}


		const QString
		to_qstring() 
		{
			if(d_visible)
				return d_txt;
			else
				return "";
				//return "---------------------------------------------\n";
		}
	private:
		QString d_txt;
		bool d_is_separator, d_visible;
	};

	
	class CommentSegment : 
		public RotationFileSegment
	{
	public:
		CommentSegment(
				const QString& str) :
			d_text(str)
		{	}

		void
		accept_visitor(
				RotationFileSegmentVisitor& v);

		const QString
		to_qstring() 
		{
			return d_text;
		}

	protected:
		QString d_text;
	};


	class AttributeSegment: 
		public RotationFileSegment
	{
	public:
		AttributeSegment(
				const QString& name,
				const QString& value,
				bool is_multiple_lines = false,
				bool end_with_new_line = false,
				const QString& leading_char = "@",
				const QString& name_sep = ":",
				const QString& value_sep = "\"",
				const QString& multilines_sep = "\"\"\"",
				const QString& sub_value_sep = "|"):
			d_name(name),
			d_value(value),
			d_is_multilines(is_multiple_lines),
			d_end_with_new_line(end_with_new_line),
			d_leading_char(leading_char),
			d_name_sep(name_sep),
			d_value_sep(value_sep),
			d_multilines_sep(multilines_sep),
			d_sub_value_sep(sub_value_sep)			
		{
			d_sub_names = name.split(name_sep);
			d_sub_values = value.split(sub_value_sep);
		}

		
		void
		accept_visitor(
				RotationFileSegmentVisitor& v) ;


		const QString
		to_qstring() 
		{
			QString ret;
			if(!d_value.isEmpty())
			{
				ret = d_leading_char + d_sub_names.join(":");
				if(d_is_multilines)
				{
					ret += d_multilines_sep + d_value + d_multilines_sep;
				}
				else
				{
					ret += d_value_sep + d_value + d_value_sep;
				}
			}
			if(!ret.simplified().isEmpty() && d_end_with_new_line && !ret.endsWith("\n"))
			{
				ret += "\n";
			}
			return ret;
		}

		QString
		get_name() const 
		{
			return d_name;
		}

		QString
		get_value() const
		{
			return d_value;
		}

		QString&
		get_name() 
		{
			return d_name;
		}

		QString&
		get_value() 
		{
			return d_value;
		}

	private:
		QStringList d_sub_names, d_sub_values;
		QString d_name, d_value;
		bool d_is_multilines, d_end_with_new_line;
		QString d_leading_char,	d_name_sep,	d_value_sep, d_multilines_sep, d_sub_value_sep ;
	};


	class MPRSHeaderLineSegment:
		public RotationFileSegment
	{
	public:
		MPRSHeaderLineSegment(
				const RotationFileSegmentContainer& segs,
				bool end_with_new_line = false,
				const QString& leading_char = ">") :
			d_end_with_new_line(end_with_new_line),
			d_leading_char(leading_char),
			d_sub_segs(segs)
		{	}

		void
		accept_visitor(
				RotationFileSegmentVisitor&);

		const QString
		to_qstring();

		RotationFileSegmentContainer&
		get_sub_segments()
		{
			return d_sub_segs;
		}

		int
		get_pid() const;

	private:
		bool d_end_with_new_line;
		const QString d_leading_char;
		RotationFileSegmentContainer d_sub_segs;
	};


	class RotationPoleSegment:
		public RotationFileSegment
	{
	public:
		RotationPoleSegment(
				const RotationPoleData& _data,
				int plate_id_len = 3,
				int double_precision = 4):
			d_data(_data),
			d_plate_id_len(plate_id_len),
			d_double_precision(double_precision)
		{ }


		void
		accept_visitor(
				RotationFileSegmentVisitor& v) ;


		const QString
		to_qstring() 
		{
			QString moving_plate_id = QString::number(d_data.moving_plate_id),
					fix_plate_id    = QString::number(d_data.fix_plate_id);
			QString ret;
			if(d_data.disabled)
			{
				ret += "#";
			}
			ret += pad_string(moving_plate_id,'0', d_plate_id_len, false)	+ "  " +
				pad_string(QString::number(d_data.time,'f',4),  ' ', 8)		+ "  " +
				pad_string(QString::number(d_data.lat,'f',4),   ' ', 8)		+ "  " +
				pad_string(QString::number(d_data.lon,'f',4),   ' ', 8)		+ "  " +
				pad_string(QString::number(d_data.angle,'f',4), ' ', 8)		+ "  " +
				pad_string(fix_plate_id,'0', d_plate_id_len, false)			+ "  " ;
			return ret;
		}

		RotationPoleData
		data() const 
		{
			return d_data;
		}

		RotationPoleData&
		data()
		{
			return d_data;
		}

		GPlatesModel::PropertyValue*
		finite_rotation() const
		{
			return d_associated_finite_rotation;
		}

		void
		set_finite_rotation(
				GPlatesModel::PropertyValue* fr)
		{
			d_associated_finite_rotation = fr;
		}

	protected:
		QString
		pad_string(
				const QString& str,
				QChar pad,
				int len,
				bool pad_tail = true)
		{
			QString ret(str); 
			if(str.length() < len)
			{
				QString padding(len - str.length(), pad);
				if(pad_tail)
					ret.append(padding);
				else
					ret.insert(0, padding);
			}
			return ret;
		}
		
		RotationPoleData d_data;
		int d_plate_id_len, d_double_precision;
		GPlatesModel::PropertyValue *d_associated_finite_rotation;
	};

	
	class RotationPoleLine:
		public LineSegment
	{
	public:
		
		explicit
		RotationPoleLine(
				const RotationFileSegmentContainer& segs):
			LineSegment(segs)
		{	}


		void
		accept_visitor(
				RotationFileSegmentVisitor& v);

		RotationPoleData
		get_rotation_pole_data() const ;

		RotationPoleData&
		get_rotation_pole_data() ;

		GPlatesModel::MetadataContainer
		update_attributes(
				const GPlatesModel::MetadataContainer &metadata);
	};


	class RotationFileSegmentVisitor
	{
	public:
		virtual
		void
		visit(
				LineSegment&) = 0;
		
		virtual
		void
		visit(
				TextSegment&) = 0;
	
		virtual
		void
		visit(
				CommentSegment&) = 0;
		
		virtual
		void
		visit(
				AttributeSegment&) = 0;
		
		virtual
		void
		visit(
				RotationPoleSegment&) = 0;

		virtual
		void
		visit(
				MPRSHeaderLineSegment&) = 0;
		
		virtual
		void
		visit(
				RotationPoleLine&) = 0;


		virtual
		~RotationFileSegmentVisitor() { }
	};


	class PopulateReconstructionFeatureCollection :
		public RotationFileSegmentVisitor
	{
	public:
		explicit
		PopulateReconstructionFeatureCollection(
				GPlatesModel::FeatureCollectionHandle::weak_ref fc):
			d_fc(fc)
		{	}

		void
		visit(
				LineSegment& s) {}

		void
		visit(
				TextSegment& s) {}
	
		void
		visit(
				CommentSegment& s) {}
		
		void
		visit(
				AttributeSegment& s);

		void
		visit(
				MPRSHeaderLineSegment& s);
		
		void
		visit(
				RotationPoleSegment& s);
		
		void
		visit(
				RotationPoleLine& s);

		void
		finalize();

	protected:
		bool
		validate_pole(
				const RotationPoleData&,
				boost::optional<const RotationPoleData&> = boost::none);

		GPlatesPropertyValues::GpmlTimeSample
		create_time_sample(
				const RotationPoleData&);

		void
		create_new_trs_feature(
				const GPlatesModel::integer_plate_id_type &moving_plate_id,
				const GPlatesModel::integer_plate_id_type &fix_plate_id);

		bool
		is_new_trs(
				const RotationPoleData& pre,
				const RotationPoleData& current);


		GPlatesModel::FeatureCollectionHandle::weak_ref d_fc;
		GPlatesModel::FeatureHandle::weak_ref d_current_feature;
		GPlatesModel::FeatureHandle::weak_ref d_fc_metadata_feature;
		std::map<QString,QString> DCMeta;
		boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type> d_current_sampling;
		boost::optional<GPlatesPropertyValues::GpmlTimeSample> d_current_sample;
		RotationPoleData d_last_pole;
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> d_mprs_attrs, d_last_mprs;
		std::vector<AttributeSegment> d_attrs;
		GPlatesModel::FeatureCollectionMetadata d_fc_metadata;
	};

	
	class RotationFileReader
	{
	public:

		static 
		void
		read_file(
				File::Reference &file,
				ReadErrorAccumulation &read_errors,
				bool &contains_unsaved_changes);
		
		virtual
		void
		read(
				const QFileInfo&,
				GPlatesModel::FeatureCollectionHandle::weak_ref) = 0;

		RotationFileSegmentContainer&
		get_segments() 
		{
			return d_segmetns;
		}

		virtual
		~RotationFileReader()
		{ }
	protected:
		RotationFileSegmentContainer d_segmetns;
	};


	class RotationFileReaderV2 :
		public RotationFileReader
	{
	public:
		RotationFileReaderV2() ;

		void
		read(
				const QFileInfo&,
				GPlatesModel::FeatureCollectionHandle::weak_ref) ;

		static const char COMMENT_LEADING_CHARACTER ;
		static const char ATTRIBUTE_LEADING_CHARACTER ;
		static const char MPRS_HEADER_LEADING_CHARACTER ;
		static const char ATTR_VALUE_SEPARATOR ;
		static const char SUB_ATTR_VALUE_SEPARATOR ;
		static const QString ATTR_LONG_VALUE_SEPARATOR ;
		
		static const QString COMMENT_LINE_REGEXP;
		static const QString ROTATION_POLE_REGEXP;
		static const QString ATTRIBUTE_REGEXP;
		static const QString ATTRIBUTE_LINE_REGEXP;
		static const QString MULTI_LINE_ATTR_REGEXP;
		static const QString MPRS_HEADER_REGEXP;
		
	protected:
		void
		process_comment(
				QIODevice&, 
				RotationFileSegmentContainer&);

		void
		process_attribute_line(
				QIODevice&, 
				RotationFileSegmentContainer&);

		void
		process_mprs_header_line(
				QIODevice&, 
				RotationFileSegmentContainer&);

		void
		process_rotation_pole_line(
				QIODevice&, 
				RotationFileSegmentContainer&);

		void
		process_arbitrary_text(
				QIODevice&, 
				RotationFileSegmentContainer&);

		QString
		peek_next_line(
				QIODevice&);

		bool
		is_valid_rotation_pole_line(
				const QString&);

		bool
		parse_rotation_pole_line(
				const QString&,
				RotationPoleData&);
	protected:
		
		QRegExp d_commnet_line_rx, d_pole_rx, d_attr_rx, 
			d_multi_line_attr_rx, d_disabled_pole_rx, d_mprs_header_rx;
		
		typedef void (RotationFileReaderV2::*function)
			(QIODevice&, RotationFileSegmentContainer&) ;
		typedef std::map<QRegExp*, function> FunctionMap;
		typedef std::vector<QRegExp*> RegExpVector;

		FunctionMap d_function_map;
		RegExpVector d_regexp_vec;
		int d_last_moving_pid;
		bool d_processing_mprs;
	};

	template <class SeparatorType>
    int
    sep_length(
        	const SeparatorType&)
    {
        qWarning() << "This template function is not supposed to be called. Use template specializations.";
	    return -1;
    }

	template <>
        inline
	int
    sep_length(
        	const char&)
    {
       		return 1;
    }

    template <>
    inline
	int
    sep_length(
		  const QString& str)
    {
        return str.length();
    }


	class GrotWriterWithCfg : 
		public PlatesRotationFormatWriter

	{
	public:
		explicit
		GrotWriterWithCfg(
				File::Reference &file_ref) : 
			PlatesRotationFormatWriter(file_ref.get_file_info()),
			d_file_ref(file_ref)
		{ }
			
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			static const GPlatesModel::FeatureType
				gpmlTotalReconstructionSequence = 
				GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence"),
				gpmlAbsoluteReferenceFrame =
				GPlatesModel::FeatureType::create_gpml("AbsoluteReferenceFrame"),
				metadata =
				GPlatesModel::FeatureType::create_gpml("FeatureCollectionMetadata");

			if ((feature_handle.feature_type() != gpmlTotalReconstructionSequence)
				&& (feature_handle.feature_type() != gpmlAbsoluteReferenceFrame) 
				&&(feature_handle.feature_type() != metadata)) {
					// These are not the features you're looking for.
					return false;
			}

			// Reset the accumulator.
			d_accum = PlatesRotationFormatAccumulator();

			return true;
		}

		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);
	private:
		File::Reference& d_file_ref;
	};


	class GrotWriterWithoutCfg : 
		public PlatesRotationFormatWriter
	{
	public:
		explicit
		GrotWriterWithoutCfg(
				File::Reference &file_ref) : 
			PlatesRotationFormatWriter(file_ref.get_file_info()),
			d_file_ref(file_ref),
			d_mprs_id(0)
		{ }
		
		
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);
 		void
 		visit_gpml_metadata(
 				const GPlatesPropertyValues::GpmlMetadata &gpml_metadata);		
		void
		visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

	private:
		File::Reference& d_file_ref;
		unsigned d_mprs_id;
	};


	/**
	 * 
	 * 
	 */
	class PlatesRotationFileProxy :
		public GPlatesModel::WeakObserver<GPlatesModel::FeatureCollectionHandle>
	{
		enum ROTATION_FORMAT_VERSION
		{
			ONE,
			TWO
		};

	public:
		PlatesRotationFileProxy() : 
			d_init(false),
			d_feature_count(0),
			d_version(TWO)
		{	
			
		}
		
		void
		init(
				File::Reference &file_ref);

		/*
		* The readonly RotationMetadataRegistry is a singleton.
		*/
		static
		const RotationMetadataRegistry&
		get_metadata_registry()
		{
			return RotationMetadataRegistry::instance();
		}

		
		RotationFileSegmentContainer&
		get_segments();


		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureCollectionHandle> &visitor) 
		{
			visitor.visit_weak_reference(d_feature_collection);
		}


		boost::shared_ptr<GrotWriterWithCfg>
		create_file_writer(
				File::Reference &file_ref)
		{
			//qWarning() << "TODO: create writer according to the file version.";
			return boost::shared_ptr<GrotWriterWithCfg>(new GrotWriterWithCfg(file_ref));
		}

		void
		save_file(
				File::Reference &file_ref)
		{
			QFile rot_file(file_ref.get_file_info().get_qfileinfo().absoluteFilePath());

			if (!rot_file.open(QFile::WriteOnly | QFile::Text))
			{
				qWarning() << "Failed to open file for writing -- " + d_file_info.get_qfileinfo().absoluteFilePath();
			}
			BOOST_FOREACH(boost::shared_ptr<RotationFileSegment> seg, get_segments())
			{
				rot_file.write(seg->to_qstring().toUtf8());
			}
		}

		void
		save_feature(
				const GPlatesModel::FeatureHandle &feature_handle,
				File::Reference &file_ref)
		{
			d_feature_count++;
			if(static_cast<std::size_t>(d_feature_count) == d_feature_collection->size())
			{
				save_file(file_ref);
				d_feature_count = 0;
			}
		}

		/*
		void
		attach_callback()
		{
			d_feature_collection.attach_callback(new FeatureCollectionModified());
		}*/

		void
		update_header_metadata(
				GPlatesModel::FeatureCollectionMetadata fc_meta);


		void
		update_MPRS_metadata(
				GPlatesModel::MetadataContainer mprs_only_data,
				GPlatesModel::MetadataContainer default_pole_data,
				const QString& moving_plate_id);

		
		boost::tuple<
				RotationFileSegmentContainer::iterator,
				RotationFileSegmentContainer::iterator>
		get_mprs_range(
				const QString& moving_plate_id);

		void
		update_pole_metadata(
				const GPlatesModel::MetadataContainer &metadata,
				const RotationPoleData &pole_data);

		void
		insert_pole(
				const RotationPoleData&);

		void
		update_pole(
				const RotationPoleData& old_pole,
				const RotationPoleData& new_pole);

		void
		delete_pole(
				const RotationPoleData&);

		/*
		* Remove any dangling MPRS header(not associated with any pole data).
		* 
		*/
		void
		remove_dangling_mprs_header();

	protected:

		/*
		struct FeatureCollectionModified :
			public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureCollectionHandle>
		{
			FeatureCollectionModified()
			{  }

			void
			publisher_modified(
					const weak_reference_type &reference,
					const modified_event_type &event)
			{
				qDebug() << "TODO: feature collection modified."; 
			}

			void
			publisher_added(
					const weak_reference_type &reference,
					const added_event_type &event)
			{
				qDebug() << "TODO: new feature is added into this feature collection."; 
			}

		
			void
			publisher_deactivated(
					const weak_reference_type &reference,
					const deactivated_event_type &event)
			{
				//qDebug() << "TODO: feature is deleted from this feature collection."; 
			}
		};
		*/

		ROTATION_FORMAT_VERSION
		version()
		{
			return d_version;
		}

		void
		check_version();

		void
		create_file_reader()
		{
			//qWarning() << "TODO: create reader according to the file version.";
			d_reader_ptr.reset(new RotationFileReaderV2());
		}
		
		boost::shared_ptr<RotationFileReader> d_reader_ptr;
		boost::shared_ptr<GrotWriterWithCfg> d_writer_ptr;
		
	private:
		bool d_init;
		int d_feature_count;
		ROTATION_FORMAT_VERSION d_version;
		
		GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;
		FileInfo d_file_info;

		PlatesRotationFileProxy(
				const PlatesRotationFileProxy&);
		
		PlatesRotationFileProxy&
		operator=(
				const PlatesRotationFileProxy&);
		static const double ROTATION_EPSILON;
	};

	namespace FeatureCollectionFileFormat
	{
		class RotationFileConfiguration :
			public Configuration
		{
		public:
			typedef boost::shared_ptr<const RotationFileConfiguration> shared_ptr_to_const_type;
			typedef boost::shared_ptr<RotationFileConfiguration> shared_ptr_type;

			RotationFileConfiguration() :
				d_proxy_ptr(new PlatesRotationFileProxy)
			{

			}

			const PlatesRotationFileProxy&
			get_rotation_file_proxy() const 
			{
				return *d_proxy_ptr;
			}

			PlatesRotationFileProxy&
			get_rotation_file_proxy()  
			{
				return *d_proxy_ptr;
			}
		private:
			boost::scoped_ptr<PlatesRotationFileProxy> d_proxy_ptr;
		};
	}

	GPlatesModel::MetadataContainer
	update_attributes_and_return_new(
			const GPlatesModel::MetadataContainer &new_data,
			const RotationFileSegmentContainer &file_segs);

	void
	update_or_delete_attribute(
			GPlatesModel::MetadataContainer &new_data, 
			AttributeSegment &attr);
}
#endif  // GPLATES_FILEIO_PLATESROTATIONFILEPROXY_H

