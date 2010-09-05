/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_QUALIFIEDXMLNAME_H
#define GPLATES_MODEL_QUALIFIEDXMLNAME_H

#include <boost/optional.hpp>
#include <QString>
#include <QStringList>

#include "StringSetSingletons.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h" // For GPLATES_ICU_BOOL
#include "utils/XmlNamespaces.h"


namespace GPlatesModel
{
	/**
	 * This class provides an efficient means of containing the qualifed name of an element
	 * or attribute occuring in an XML document
	 *
	 * Since many elements and attributes share the same name, this class minimises memory
	 * usage for the storage of all these names, by allowing them all to share a single string;
	 * each QualifiedXmlName instance stores an iterator to the shared string for namespace,
	 * namespace alias and the name associated with the element or attribute.
	 *
	 * Accessing the details is as inexpensive as dereferencing the iterator.
	 *
	 * Since the strings are unique in the StringSet, comparison for equality of qualified XML
	 * names is as simple as comparing three pairs of iterators for equality.
	 *
	 * Since StringSet uses a 'std::set' for storage, testing whether an arbitrary Unicode
	 * string is a member of the StringSet has O(log n) cost.  Further, since all loaded names
	 * are stored within the StringSet, it is inexpensive to test whether a desired name is
	 * even loaded, without needing to iterate through all properties of all features.
	 *
	 * The SingletonType template parameter should be the singleton StringSet that is used
	 * for the name of this QualifiedXmlName.  See "PropertyName.h" for an example.
	 */
	template<typename SingletonType>
	class QualifiedXmlName
	{
	public:

		static
		const QualifiedXmlName
		create_gpml(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::GPML_NAMESPACE,
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gpml(
				const QString &namespace_alias,
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::GPML_NAMESPACE, 
					GPlatesUtils::make_icu_string_from_qstring(namespace_alias),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gml(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::GML_NAMESPACE,
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gml(
				const QString &namespace_alias,
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::GML_NAMESPACE, 
					GPlatesUtils::make_icu_string_from_qstring(namespace_alias),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_xsi(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::XSI_NAMESPACE,
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		template<typename U>
		explicit 
		QualifiedXmlName(
				const QualifiedXmlName<U> &other) :
			d_namespace(other.get_namespace_iterator()), 
			d_namespace_alias(other.get_namespace_alias_iterator()),
			d_name(SingletonType::instance().insert(other.get_name()))
		{ }


		/**
		 * Instantiate a new QualifiedXmlName instance for the given namespace and name.
		 */
		QualifiedXmlName(
				const QString &namespace_uri,
				const QString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(namespace_uri))),
			d_name(SingletonType::instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(name)))
		{
			set_namespace_alias();
		}


		QualifiedXmlName(
				const UnicodeString &namespace_uri,
				const UnicodeString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(namespace_uri)), 
			d_name(SingletonType::instance().insert(name))
		{
			set_namespace_alias();
		}


		/**
		 * Instantiate a new QualifiedXmlName instance for the given namespace, alias and name.
		 */
		QualifiedXmlName(
				const QString &namespace_uri,
				const QString &namespace_alias,
				const QString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(namespace_uri))),
			d_namespace_alias(StringSetSingletons::xml_namespace_alias_instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(namespace_alias))),
			d_name(SingletonType::instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(name)))
		{  }


		QualifiedXmlName(
				const UnicodeString &namespace_uri,
				boost::optional<const UnicodeString &> namespace_alias,
				const UnicodeString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(namespace_uri)), 
			d_name(SingletonType::instance().insert(name))
		{
			if (namespace_alias) {
				d_namespace_alias = *namespace_alias;
			} else {
				set_namespace_alias();
			}
		}


		/**
		 * Access the Unicode string of the namespace for this instance.
		 */
		const UnicodeString &
		get_namespace() const {
			return *d_namespace;
		}

		/**
		 * Access the underlying StringSet iterator of the namespace.
		 */
		const GPlatesUtils::StringSet::SharedIterator
		get_namespace_iterator() const {
			return d_namespace;
		}

		/**
		 * Access the Unicode string of the namespace alias for this instance.
		 */
		const UnicodeString &
		get_namespace_alias() const {
			return *d_namespace_alias;
		}

		/**
		 * Access the underlying StringSet iterator of the namespace alias for 
		 * this instance.
		 */
		GPlatesUtils::StringSet::SharedIterator
		get_namespace_alias_iterator() const {
			return d_namespace_alias;
		}

		/**
		 * Access the Unicode string of the name for this instance.
		 */
		const UnicodeString &
		get_name() const {
			return *d_name;
		}

		/**
		 * Return a copy of this qualified name in the form of a string "alias:name"
		 * where @a alias is the result of get_namespace_alias() and @a name is the 
		 * result of get_name().
		 * 
		 * Note that this undermines many of the performance benefits
		 * gained from using QualifiedXmlName in the first place.
		 */
		const UnicodeString
		build_aliased_name() const {
			return get_namespace_alias() + UnicodeString(":") + get_name();
		}

		/**
		 * Determine whether another QualifiedXmlName instance contains the same property name
		 * as this instance.
		 */
		bool
		is_equal_to(
				const QualifiedXmlName<SingletonType> &other) const {
			return (d_name == other.d_name)
				&& (d_namespace == other.d_namespace);
		}

	private:

		GPlatesUtils::StringSet::SharedIterator d_namespace;
		GPlatesUtils::StringSet::SharedIterator d_namespace_alias;
		GPlatesUtils::StringSet::SharedIterator d_name;

		void
		set_namespace_alias() {
			d_namespace_alias = 
				GPlatesUtils::XmlNamespaces::get_standard_alias_for_namespace(*d_namespace);
		}
	};


	template<typename SingletonType>
	inline
	bool
	operator==(
			const QualifiedXmlName<SingletonType> &qn1,
			const QualifiedXmlName<SingletonType> &qn2) {
		return qn1.is_equal_to(qn2);
	}


	template<typename SingletonType>
	inline
	bool
	operator!=(
			const QualifiedXmlName<SingletonType> &qn1,
			const QualifiedXmlName<SingletonType> &qn2) {
		return ! qn1.is_equal_to(qn2);
	}


	/**
	 * Used when QualifiedXmlNames are used as keys in a @c std::map.
	 */
	template<typename SingletonType>
	inline
	bool
	operator<(
			const QualifiedXmlName<SingletonType> &qn1,
			const QualifiedXmlName<SingletonType> &qn2) {
		return GPLATES_ICU_BOOL(qn1.get_name() < qn2.get_name());
	}

}


namespace GPlatesUtils
{
	// Specialisation of Parse for the QualifiedXmlName.
	template<typename SingletonType>
	struct Parse<GPlatesModel::QualifiedXmlName<SingletonType> >
	{
		GPlatesModel::QualifiedXmlName<SingletonType>
		operator()(
				const QString &s)
		{
			QStringList tokens = s.split(':');
			if (tokens.count() == 2)
			{
				return GPlatesModel::QualifiedXmlName<SingletonType>(
						*GPlatesUtils::XmlNamespaces::get_namespace_for_standard_alias(
							GPlatesUtils::make_icu_string_from_qstring(tokens.at(0))),
						GPlatesUtils::make_icu_string_from_qstring(tokens.at(1)));
			}
			else
			{
				throw ParseError();
			}
		}
	};
}


#endif  // GPLATES_MODEL_QUALIFIEDXMLNAME_H
