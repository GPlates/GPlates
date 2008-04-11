/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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


#include <algorithm>
#include <boost/bind.hpp>
#include <iostream>
#include "XmlWriter.h"
#include "model/StringSetSingletons.h"


namespace
{
	bool
	compare_ns_decls(
			const GPlatesFileIO::XmlWriter::NamespaceDeclaration &ns_decl_1,
			const GPlatesFileIO::XmlWriter::NamespaceDeclaration &ns_decl_2)
	{
		return ns_decl_1.first == ns_decl_2.first;
	}


	bool
	compare_ns_and_decl(
			const GPlatesUtils::StringSet::SharedIterator &namespace_uri,
			const GPlatesFileIO::XmlWriter::NamespaceDeclaration &ns_decl) 
	{
		return namespace_uri == ns_decl.first;
	}


	bool
	compare_aliases(
			const GPlatesUtils::StringSet::SharedIterator &namespace_alias,
			const GPlatesFileIO::XmlWriter::NamespaceDeclaration &ns_decl)
	{
		return namespace_alias == ns_decl.second;
	}
}


GPlatesFileIO::XmlWriter::XmlWriter(
		QIODevice *target) :
	d_writer(target) 
{ 
	d_writer.setAutoFormatting(true);
}

void
GPlatesFileIO::XmlWriter::writeEndElement(
		bool pop_ns_stack)
{
	if (pop_ns_stack) {
		// XXX: temporary
		std::cout << "Popping namespace stack." << std::endl;
		if (d_ns_stack.empty()) {
			// FIXME: throw exception.
			std::cerr << "XXX: Attempt to pop empty namespace stack!" << std::endl;
		} else {
			d_ns_stack.pop_back();
		}
	}
	d_writer.writeEndElement();
}


void
GPlatesFileIO::XmlWriter::writeNamespace(
		const QString &namespace_uri,
		const QString &namespace_alias) 
{
	GPlatesUtils::StringSet::SharedIterator ns =
		GPlatesModel::StringSetSingletons::xml_namespace_instance().insert(
				GPlatesUtils::make_icu_string_from_qstring(namespace_uri));
	GPlatesUtils::StringSet::SharedIterator alias =
		GPlatesModel::StringSetSingletons::xml_namespace_alias_instance().insert(
				GPlatesUtils::make_icu_string_from_qstring(namespace_alias));
	d_ns_stack.push_back(std::make_pair(ns, alias));
	d_writer.writeNamespace(namespace_uri, namespace_alias);
}


const UnicodeString
GPlatesFileIO::XmlWriter::getAliasForNamespace(
		const GPlatesUtils::StringSet::SharedIterator namespace_uri) const
{
	NamespaceStack::const_reverse_iterator iter = std::find_if(
			d_ns_stack.rbegin(), d_ns_stack.rend(), 
			boost::bind(compare_ns_and_decl, namespace_uri, _1));
	// N.B.  We can't use bind1st in the find_if call above since it takes
	// a reference to it's argument which, in this case, is already a (const)
	// reference (and one can't take the reference of a reference).  The same
	// reasoning lies behind the subsequent uses of boost::bind(...) in this file.

	if (iter != d_ns_stack.rend()) 
	{
		return *(iter->second);
	}
	return *namespace_uri;
}


bool
GPlatesFileIO::XmlWriter::declare_namespace_if_necessary(
		const NamespaceDeclaration &ns_decl)
{
	bool namespace_added = false;
	NamespaceStack::reverse_iterator iter = std::find_if(
			d_ns_stack.rbegin(), d_ns_stack.rend(), 
			boost::bind(compare_ns_decls, ns_decl, _1));

	// Declare the namespace if...
	bool declare_namespace = 
			// ...the namespace hasn't been declared yet:
		(iter == d_ns_stack.rend())  
			// ...or if the namespace has been declared, but it was given a different alias:
		|| (iter->second != ns_decl.second)
			// ...or if the namespace was declared with the same alias, but it has been 
			// shadowed by another declaration:
		|| (iter != std::find_if(
						d_ns_stack.rbegin(), iter,
						boost::bind(compare_aliases, ns_decl.second, _1)));

	if (declare_namespace)
	{
		// XXX: temporary
		std::cout << "Declaring namespace: " 
			<< GPlatesUtils::make_qstring_from_icu_string(*ns_decl.first).toStdString()
			<< " with alias "
			<< GPlatesUtils::make_qstring_from_icu_string(*ns_decl.second).toStdString() << std::endl;
		d_writer.writeNamespace(
				GPlatesUtils::make_qstring_from_icu_string(*ns_decl.first),
				GPlatesUtils::make_qstring_from_icu_string(*ns_decl.second));
		namespace_added = true;
	}
	
	return namespace_added;
}
