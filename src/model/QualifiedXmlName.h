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

#include <boost/operators.hpp>
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
	class QualifiedXmlName :
			// NOTE: Base class chaining is used to avoid increasing sizeof(QualifiedXmlName) due to multiple
			// inheritance from several empty base classes...
			public boost::less_than_comparable<QualifiedXmlName<SingletonType>,
					boost::equality_comparable<QualifiedXmlName<SingletonType> > >
	{
	public:

		// NOTE: The GPGIM namespace is not part of the feature readers but is placed here
		// in order to re-use a lot of the XML parsing machinery when reading the GPGIM XML file.
		static
		const QualifiedXmlName
		create_gpgim(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_gpgim_namespace(),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		// NOTE: The GPGIM namespace is not part of the feature readers but is placed here
		// in order to re-use a lot of the XML parsing machinery when reading the GPGIM XML file.
		static
		const QualifiedXmlName
		create_gpgim(
				const QString &namespace_alias,
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_gpgim_namespace(), 
					GPlatesUtils::make_icu_string_from_qstring(namespace_alias),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gpml(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_gpml_namespace(),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gpml(
				const QString &namespace_alias,
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_gpml_namespace(), 
					GPlatesUtils::make_icu_string_from_qstring(namespace_alias),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gml(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_gml_namespace(),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_gml(
				const QString &namespace_alias,
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_gml_namespace(), 
					GPlatesUtils::make_icu_string_from_qstring(namespace_alias),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		static
		const QualifiedXmlName
		create_xsi(
				const QString &name) {
			return QualifiedXmlName(
					GPlatesUtils::XmlNamespaces::get_xsi_namespace(),
					GPlatesUtils::make_icu_string_from_qstring(name));
		}


		/**
		 * Constructor for when other QualifiedXmlName<U> type is different to 'this'.
		 *
		 * Compiler should only use this when U != SingletonType (see http://www.gotw.ca/gotw/016.htm).
		 */
		template<typename U>
		explicit 
		QualifiedXmlName(
				const QualifiedXmlName<U> &other) :
			d_namespace(other.get_namespace_iterator()), 
			d_namespace_alias(other.get_namespace_alias_iterator()),
			d_name(SingletonType::instance().insert(other.get_name()))
		{ }


		/**
		 * Copy constructor for when other type is same as 'this'.
		 *
		 * Compiler should implicitly generate this for us even though we've defined the
		 * templated constructor above (that looks like a copy-constructor but is not).
		 * However we'll explicitly define a copy constructor just in case.
		 *
		 * This is more efficient than the template constructor above since it doesn't require
		 * an insertion (because 'other' object references the same string set singletons as 'this').
		 */
		QualifiedXmlName(
				const QualifiedXmlName &other) :
			d_namespace(other.d_namespace), 
			d_namespace_alias(other.d_namespace_alias),
			d_name(other.d_name)
		{ }


		/**
		 * Instantiate a new QualifiedXmlName instance for the given namespace and name.
		 */
		QualifiedXmlName(
				const QString &namespace_uri,
				const QString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(namespace_uri))),
			d_namespace_alias(d_namespace),
			d_name(SingletonType::instance().insert(
						GPlatesUtils::make_icu_string_from_qstring(name)))
		{
			set_namespace_alias();
		}


		QualifiedXmlName(
				const GPlatesUtils::UnicodeString &namespace_uri,
				const GPlatesUtils::UnicodeString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(namespace_uri)),
			d_namespace_alias(d_namespace),
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
				const GPlatesUtils::UnicodeString &namespace_uri,
				boost::optional<const GPlatesUtils::UnicodeString &> namespace_alias,
				const GPlatesUtils::UnicodeString &name) :
			d_namespace(StringSetSingletons::xml_namespace_instance().insert(namespace_uri)), 
			d_namespace_alias(d_namespace),
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
		const GPlatesUtils::UnicodeString &
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
		const GPlatesUtils::UnicodeString &
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
		const GPlatesUtils::UnicodeString &
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
		const GPlatesUtils::UnicodeString
		build_aliased_name() const {
			return get_namespace_alias() + GPlatesUtils::UnicodeString(":") + get_name();
		}

		/**
		 * Determine whether another QualifiedXmlName instance contains the same qualified name
		 * as this instance.
		 */
		bool
		is_equal_to(
				const QualifiedXmlName<SingletonType> &other) const {
			return (d_name == other.d_name)
				&& (d_namespace == other.d_namespace);
		}

		/**
		 * Equality comparison operator - inequality operator provided by 'boost::equality_comparable'.
		 */
		bool
		operator==(
				const QualifiedXmlName &other) const {
			return is_equal_to(other);
		}

		/**
		 * Less-than operator - provided so @a QualifiedXmlName can be used as a key in std::map.
		 *
		 * The other comparison operators are provided by boost::less_than_comparable.
		 */
		bool
		operator<(
				const QualifiedXmlName &other) const
		{
			// Test optimised code path first (where both namespaces match) since this is an
			// efficient shared iterator comparison.
			if (d_namespace == other.d_namespace)
			{
				// Do a string comparison on the unqualified names.
				return GPLATES_ICU_BOOL(*d_name < *other.d_name);
			}

			// Do expensive namespace string comparison second.
			//
			// We can't use the shorter namespace alias because it's not fully qualified
			// (ie, it's possible, although shouldn't happen, that two different aliases refer
			// to the same namespace or the same alias referred to two different namespaces
			// in two different parts of the XML file).
			return GPLATES_ICU_BOOL(*d_namespace < *other.d_namespace);
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


	/**
	 * Convenience function to convert a @a QualifiedXmlName to a QString as:
	 *    "<namespace_alias>:<name>".
	 */
	template <class QualifiedXmlNameType>
	QString
	convert_qualified_xml_name_to_qstring(
			const QualifiedXmlNameType &qualified_xml_name)
	{
		return GPlatesUtils::make_qstring_from_icu_string(
				qualified_xml_name.build_aliased_name());
	}


	/**
	 * Converts a QString, represented as "<namespace_alias>:<name>", to a @a QualifiedXmlName.
	 *
	 * For example:
	 *     convert_qstring_to_qualified_xml_name<FeatureType>(qualified_string);
	 *
	 * If the "<namespace_alias>" part of the specified string is not one of the standard namespaces
	 * used in GPlates then the "gpml" namespace is assumed.
	 */
	template <class QualifiedXmlNameType>
	boost::optional<QualifiedXmlNameType>
	convert_qstring_to_qualified_xml_name(
			const QString &qualified_string)
	{
		QStringList tokens = qualified_string.split(':');

		// We expect the string to be qualified.
		if (tokens.count() == 2)
		{
			return QualifiedXmlNameType(
					*GPlatesUtils::XmlNamespaces::get_namespace_for_standard_alias(
							GPlatesUtils::make_icu_string_from_qstring(tokens.at(0))),
					GPlatesUtils::make_icu_string_from_qstring(tokens.at(1)));
		}

		// If the string is not qualified then assume the "gpml" namespace - which is pretty much
		// what 'XmlNamespaces::get_namespace_for_standard_alias()' does.
		if (tokens.count() == 1)
		{
			return QualifiedXmlNameType(
					GPlatesUtils::XmlNamespaces::get_gpml_namespace_qstring(),
					tokens.at(0));
		}

		// Some 'overqualified' name.
		return boost::none;
	}
}


namespace GPlatesUtils
{
	// Specialisation of Parse for the QualifiedXmlName.
	template<typename SingletonType>
	struct Parse<GPlatesModel::QualifiedXmlName<SingletonType> >
	{
		typedef GPlatesModel::QualifiedXmlName<SingletonType> qualified_xml_name_type;

		qualified_xml_name_type
		operator()(
				const QString &s)
		{
			boost::optional<qualified_xml_name_type> qualified_xml_name =
					GPlatesModel::convert_qstring_to_qualified_xml_name<qualified_xml_name_type>(s);
			if (!qualified_xml_name)
			{
				throw ParseError();
			}

			return qualified_xml_name.get();
		}
	};
}


#endif  // GPLATES_MODEL_QUALIFIEDXMLNAME_H
