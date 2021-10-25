/* $Id:  $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-09-06 04:45:52 +1000 (Mon, 06 Sep 2010) $
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

#ifndef GPLATES_MODEL_DCMETADATA_H
#define GPLATES_MODEL_DCMETADATA_H
#include <map>
#include <vector>

#include <boost/foreach.hpp>

#include <QString>

#include "XmlNode.h"

#include "file-io/XmlWriter.h"

#include "utils/XQueryUtils.h"

namespace GPlatesModel
{

	/*
	* This function replace fields in a string with new values.
	* The fields in a string are separated by "|". TODO
	*/
	inline
	const QString 
	replace_field_string(
			const QString &str,
			const std::vector<QString> &fields)
	{
		QString ret;
		QStringList tmp_list = str.split("|");
		int i=0;
		for(; i<tmp_list.size();i++)
		{
			QString tmp = tmp_list[i];
			if(i!=0)
			{
				ret += "|";
			}
			if(tmp.trimmed().isEmpty())
			{
				ret += fields[i]+tmp;
			}
			else
			{
				tmp.replace(tmp.trimmed(), fields[i]);
				ret += tmp;
			}
		}

		for(;static_cast<std::size_t>(i)<fields.size();i++)
		{
			ret += "|"+fields[i];
		}
		return ret;
	}

	struct DublinCoreMetadata
	{
		struct Creator
		{
			const QString 
			to_string() const
			{
				std::vector<QString> tmp_vec;
				tmp_vec.push_back(name);
				tmp_vec.push_back(email); 
				tmp_vec.push_back(url);
				tmp_vec.push_back(affiliation);

				return  replace_field_string(original_text,tmp_vec);
			}
			QString name, email, url, affiliation, original_text;
		};

		struct Contributor
		{
			const QString 
			to_string() const
			{
				std::vector<QString> tmp_vec;
				tmp_vec.push_back(id); tmp_vec.push_back(name);
				tmp_vec.push_back(email); tmp_vec.push_back(url);
				tmp_vec.push_back(address);

				return  replace_field_string(original_text,tmp_vec);
			}
			QString id, name, email, url, address, original_text;
		};
				
		struct Rights
		{
			QString license, url;
		};
		
		struct Coverage
		{
			QString temporal;
		};
		
		struct Date
		{
			QString created;
			std::vector<boost::shared_ptr<QString> > modified;
		};
		
		QString dc_namespace, title, bibliographicCitation, description;
		std::vector<Contributor> contributors;
		std::vector<Creator> creators;
		Rights rights;
		Coverage coverage;
		Date date;
	};

	struct GeoTimeScale
	{
		const QString 
		to_string() const
		{
			std::vector<QString> tmp_vec;
			tmp_vec.push_back(id); tmp_vec.push_back(pub_id);
			tmp_vec.push_back(ref); tmp_vec.push_back(bib_ref);
			return replace_field_string(original_text,tmp_vec);
		}
		QString id, pub_id, ref, bib_ref, original_text;
	};

	struct BibInfoType
	{
		QString bibfile, doibase;
	};

	struct HeaderMetadataType
	{
		QString GPLATESROTATIONFILE_version, 
				GPLATESROTATIONFILE_documentation,
				GPML_namespace; 
		std::vector<boost::shared_ptr<QString> > REVISIONHIST;
	};

	class HellData
	{
	public:
		explicit
		HellData(
				const QString &);

		HellData(
				const QString &r,
				const QString &Ns, 
				const QString &dF, 
				const QString &kappahat, 
				const QString &cov);

		double&
		get_r()
		{
			return r;
		}

		double&
		get_kappahat()
		{
			return kappahat;
		}

	private:
		double r, kappahat;
		int Ns_n, Ns_s, dF;
		std::vector<double> cov;
	};

	class FeatureCollectionMetadata
	{
	public:
		FeatureCollectionMetadata();

		FeatureCollectionMetadata(
				const GPlatesModel::XmlElementNode::non_null_ptr_type);
		
		bool 
		is_fc_metadata(
				const QString& name)
		{
			return (d_meta_func.find(name) != d_meta_func.end());
		}
		
		bool
		set_metadata(
				const QString& name,
				const QString& value);


		std::multimap<QString, QString>
		get_metadata_as_map() const;


		QString
		to_xml() const;

		void
		serialize(
				GPlatesFileIO::XmlWriter& writer) const;

		void
		serialize(
				QString& buffer) const;

		const DublinCoreMetadata&
		get_dc_data() const
		{
			return DC;
		}

		DublinCoreMetadata&
		get_dc_data()
		{
			return DC;
		}

		const BibInfoType&
		get_bibinfo() const
		{
			return BIBINFO;
		}

		BibInfoType&
		get_bibinfo() 
		{
			return BIBINFO;
		}

		const std::vector<GeoTimeScale>&
		get_geo_time_scales() const
		{
			return GEOTIMESCALE;
		}

		std::vector<GeoTimeScale>&
		get_geo_time_scales() 
		{
			return GEOTIMESCALE;
		}

		const HeaderMetadataType&
		get_header_metadata() const
		{
			return HeaderMetadata;
		}

		HeaderMetadataType&
		get_header_metadata() 
		{
			return HeaderMetadata;
		}

	protected:

		void
		init();

		void
		process_complex_xml_element(
				QXmlStreamReader&);
		
		void
		process_gpml_meta(
				QXmlStreamReader&);

		void
		process_dc_creator(
				QXmlStreamReader&);

		void
		process_dc_rights(
				QXmlStreamReader&);

		void
		process_dc_date(
				QXmlStreamReader&);

		void
		process_dc_coverage(
				QXmlStreamReader& reader);

		void
		process_dc_namespace(
				QXmlStreamReader& reader)
		{
			set_dc_namespace(reader.readElementText());
			GPlatesUtils::XQuery::next_start_element(reader);
		}
		
		void
		process_dc_title(
				QXmlStreamReader& reader)
		{
			set_dc_title(reader.readElementText());
			GPlatesUtils::XQuery::next_start_element(reader);
		}
		
		void
		process_dc_bibliographicCitation(
				QXmlStreamReader& reader)
		{
			set_dc_bibliographicCitation(reader.readElementText());
			GPlatesUtils::XQuery::next_start_element(reader);
		}
		
		void
		process_dc_description(
				QXmlStreamReader& reader)
		{
			set_dc_description(reader.readElementText());
			GPlatesUtils::XQuery::next_start_element(reader);
		}
		
		void
		process_dc_contributor(
				QXmlStreamReader& reader)
		{
			set_dc_contributor(reader.readElementText());
			GPlatesUtils::XQuery::next_start_element(reader);
		}

		void 
		set_version(
				const QString& str)
		{
			HeaderMetadata.GPLATESROTATIONFILE_version = str;
		}

		void 
		set_documentation(
				const QString& str)
		{
			HeaderMetadata.GPLATESROTATIONFILE_documentation = str;
		}

		void 
		set_dc_namespace(
				const QString& str)
		{
			DC.dc_namespace = str;
		}

		void
		set_dc_title(
				const QString& str)
		{
			DC.title = str;
		}

		void
		set_dc_creator(
				const QString& str)
		{
			DublinCoreMetadata::Creator creator;
			creator.original_text = str;
			QStringList l = str.split('|');
			if(l.size() != 4)
			{
				qWarning() << "Invalid Creator field found -- " << str; 
				return;
			}
			creator.name = l[0].trimmed();
			creator.email = l[1].trimmed();
			creator.url = l[2].trimmed();
			creator.affiliation = l[3].trimmed();
			DC.creators.push_back(creator);
		}

		
		void
		set_dc_rights_license(
				const QString& str)
		{
			DC.rights.license = str;
		}
		
		void
		set_dc_rights_url(
				const QString& str)
		{
			DC.rights.url = str;
		}
		
		void
		set_dc_date_created(
				const QString& str)
		{
			DC.date.created = str;
		}
		
		void
		set_dc_date_modified(
				const QString& str)
		{
			DC.date.modified.push_back(
					boost::shared_ptr<QString>(new QString(str)));
		}
		
		void
		set_dc_coverage_temporal(
				const QString& str)
		{
			DC.coverage.temporal = str;
		}
		
		void
		set_dc_bibliographicCitation(
				const QString& str)
		{
			DC.bibliographicCitation = str;
		}
		
		void
		set_dc_description(
				const QString& str)
		{
			DC.description = str;
		}
		
		void
		set_dc_revision_history(
				const QString& str)
		{
			HeaderMetadata.REVISIONHIST.push_back(
					boost::shared_ptr<QString>(new QString(str)));
		}
		
		void
		set_dc_bibinfo_bibfile(
				const QString& str)
		{
			BIBINFO.bibfile = str;
		}
		
		void
		set_dc_bibinfo_doibase(
				const QString& str)
		{
			BIBINFO.doibase = str;
		}
		
		void
		set_gpml_namespace(
				const QString& str)
		{
			HeaderMetadata.GPML_namespace = str;
		}
		
		void
		set_geotimescale(
				const QString& str)
		{
			GeoTimeScale scale;
			scale.original_text = str;
			QStringList l = str.split('|');
			if(l.size() != 4)
			{
				qWarning() << "Invalid GeoTimeScale field found -- " << str; 
				return;
			}
			scale.id = l[0].trimmed();
			scale.pub_id = l[1].trimmed();
			scale.ref = l[2].trimmed();
			scale.bib_ref = l[3].trimmed();
			GEOTIMESCALE.push_back(scale);
		}
		
		void
		set_dc_contributor(
				const QString& str)
		{
			DublinCoreMetadata::Contributor contr;
			contr.original_text = str;
			QStringList l = str.split('|');
			if(l.size() != 5)
			{
				qWarning() << "Invalid contributor field found -- " << str; 
				return;
			}
			contr.id = l[0].trimmed();
			contr.name = l[1].trimmed();
			contr.email = l[2].trimmed();
			contr.url = l[3].trimmed();
			contr.address = l[4].trimmed();

			DC.contributors.push_back(contr);
		}

		QString
		qualified_name(
				const QXmlStreamReader& reader)
		{
			QString prefix;
			if(reader.namespaceUri() == DC_NAMESPACE)
			{
				prefix = "dc";
			}
			else if(reader.namespaceUri() == GPML_NAMESPACE)
			{
				prefix = "gpml";
			}
			else
			{
				prefix = reader.prefix().toString();
				qWarning() << "Unexpected namespace uri: " << reader.namespaceUri();
			}

			return prefix + ":"+ reader.name().toString();
		}

		DublinCoreMetadata DC;
		BibInfoType BIBINFO;
		HeaderMetadataType HeaderMetadata;
		std::vector<GeoTimeScale> GEOTIMESCALE;

		typedef void (FeatureCollectionMetadata::*func_ptr)(const QString&);
		typedef std::map<QString,func_ptr> FuncMap;
		FuncMap d_meta_func;

		typedef void (FeatureCollectionMetadata::*xml_process_func_ptr)(QXmlStreamReader&);
		typedef std::map<QString,xml_process_func_ptr> XMLFuncMap;
		XMLFuncMap d_xml_func_map;
		std::set<QString> d_recurring_data;

		const static QString DC_NAMESPACE, GPML_NAMESPACE;

		void
		set_data(
				const QString& name,
				QXmlStreamReader& reader,
				func_ptr func)
		{
			if(qualified_name(reader) == name)
			{
				(this->*func)(reader.readElementText());
			}else
			{
				qWarning() << QString("Expecting xml element %2, but got %1.").arg(
					qualified_name(reader)).arg(name);
			}
		}
	};
		
	class Metadata
	{
	public:
		typedef boost::shared_ptr<GPlatesModel::Metadata> shared_ptr_type;
		typedef boost::shared_ptr<const GPlatesModel::Metadata> shared_const_ptr_type;
		Metadata(
				const QString& name,
				const QString& content):
			d_name(name),
			d_content(content)
			{ }

		virtual
		QString
		get_name() const 
		{
			return d_name;
		}

		virtual
		QString&
		get_name()  
		{
			return d_name;
		}

		virtual
		const QString&
		get_content() const 
		{
			return d_content;
		}

		virtual
		QString&
		get_content()  
		{
			return d_content;
		}

		virtual
		~Metadata(){ }

		static const QString DISABLED_SEQUENCE_FLAG;
		static const QString DELETE_MARK;

	protected:
		QString d_name, d_content;
	};

	typedef std::vector<Metadata::shared_ptr_type> MetadataContainer;

	class PoleMetadata:
		public Metadata
	{
	public:
		PoleMetadata(
				const QString& name,
				const QString& content):
			Metadata(name, content)
			{ }
	};


	inline
	MetadataContainer::iterator 
	find_first_of(
			const QString &name,
			MetadataContainer &container)
	{
		MetadataContainer::iterator 
			iter = container.begin(),
			end = container.end(),
			ret = end;
		for(; iter !=end; iter++)
		{
			if((*iter)->get_name() == name)
			{
				ret = iter;
				break;
			}
		}
		return ret;
	}	


	inline
	MetadataContainer 
	find_all(
			const QString &name,
			MetadataContainer &container)
	{
		MetadataContainer  ret;
		BOOST_FOREACH(MetadataContainer::value_type v, container)
		{
			if(v->get_name() == name)
			{
				ret.push_back(v);
			}
		}
		return ret;
	}	


	inline
	bool
	is_same_meta(
			Metadata::shared_ptr_type first,
			Metadata::shared_ptr_type second)
	{
		return (first->get_name() == second->get_name());
	}
}

#endif  // GPLATES_MODEL_DCMETADATA_H

