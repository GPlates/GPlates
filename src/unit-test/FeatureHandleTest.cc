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
#include <iostream>

#ifdef WIN32
#include <windows.h>
#include <Psapi.h>
#endif

#if 0
#include <unistd.h> 
#include <ios> 
#include <iostream> 
#include <fstream> 
#include <string> 
#endif

#include <QDebug>

#include <boost/pool/singleton_pool.hpp>

#include "unit-test/FeatureHandleTest.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlKeyValueDictionary.h"

GPlatesUnitTest::FeatureHandleTestSuite::FeatureHandleTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"FeatureHandleTestSuite")
{
	init(level);
} 

namespace
{
#ifdef WIN32
	void
	print_memory_usage()
	{
		HANDLE hProcess;
		PROCESS_MEMORY_COUNTERS pmc;

		// Print the process identifier.

		DWORD processID = GetCurrentProcessId();
		printf( "\nProcess ID: %u\n", processID );

		// Print information about the memory usage of the process.

		hProcess = 
			OpenProcess(  
					PROCESS_QUERY_INFORMATION |	PROCESS_VM_READ,
					FALSE, 
					processID );
		
		if (NULL == hProcess)
		{
			std::cout << " The process ID is empty...." << std::endl;
			return;
		}

		if ( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
		{
			printf( "\tPageFaultCount: 0x%08X\n", pmc.PageFaultCount );
			printf( "\tPeakWorkingSetSize: 0x%08X\n", 
				pmc.PeakWorkingSetSize );
		//	printf( "\tWorkingSetSize: 0x%08X\n", pmc.WorkingSetSize );
			std::cout << "\tWorkingSetSize:  " << pmc.WorkingSetSize << std::endl;
			printf( "\tQuotaPeakPagedPoolUsage: 0x%08X\n", 
				pmc.QuotaPeakPagedPoolUsage );
			printf( "\tQuotaPagedPoolUsage: 0x%08X\n", 
				pmc.QuotaPagedPoolUsage );
			printf( "\tQuotaPeakNonPagedPoolUsage: 0x%08X\n", 
				pmc.QuotaPeakNonPagedPoolUsage );
			printf( "\tQuotaNonPagedPoolUsage: 0x%08X\n", 
				pmc.QuotaNonPagedPoolUsage );
			printf( "\tPagefileUsage: 0x%08X\n", pmc.PagefileUsage ); 
			printf( "\tPeakPagefileUsage: 0x%08X\n", 
				pmc.PeakPagefileUsage );
		}

		CloseHandle( hProcess );


	}
#else
	void
	print_memory_usage()
	{}
#endif

#if 0
	void
	print_memory_usage()
	{
		using std::ios_base; 
		using std::ifstream; 
		using std::string; 

		double vm_usage     = 0.0; 
		double resident_set = 0.0; 

		ifstream stat_stream("/proc/self/stat",ios_base::in); 

		string pid, comm, state, ppid, pgrp, session, tty_nr; 
		string tpgid, flags, minflt, cminflt, majflt, cmajflt; 
		string utime, stime, cutime, cstime, priority, nice; 
		string O, itrealvalue, starttime; 

		unsigned long vsize; 
		long rss; 

		stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr 
			>> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt 
			>> utime >> stime >> cutime >> cstime >> priority >> nice 
			>> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest 

		long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
		vm_usage     = vsize / 1024.0; 
		resident_set = rss * page_size_kb; 
		std::cout << "The virtual memory usage is: " <<  vm_usage <<std::endl;
		std::cout << "The resident memory usage is: " <<  resident_set <<std::endl;

	}
#endif

}


struct MyPoolTag { };
struct TestStruct{int i;long int j;long n; double d;};

typedef boost::singleton_pool<MyPoolTag, sizeof(TestStruct)> my_pool;

void* operator new(size_t size, bool flag)
{
	if(size == sizeof(TestStruct))
	{
		std::cout << "allocate memory by boost pool." << std::endl;
		return my_pool::malloc();
	}
	else
		return malloc(size);
}

void operator delete(void*p, bool flag)
{
	free(p);
}

#define new new(true)

void 
GPlatesUnitTest::FeatureHandleTest::test_case_1()
{
#if 0
	const int REPEAT_NUM = 10000;
	GPlatesModel::FeatureType feature_type = 
		GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = 
		GPlatesModel::FeatureCollectionHandle::create(d_model->root());

	print_memory_usage();
	std::cout<<"press any key to start."<<std::endl;
	std::cin.get();
/*
#if 0
	GPlatesModel::FeatureHandle::weak_ref feature = 
		GPlatesModel::FeatureHandle::create(
				feature_collection,
				feature_type);
#endif 
*/
	std::cout << "Start testing feature memory efficiency... " << std::endl;
	for(int i = 0; i < REPEAT_NUM; i++)
	{
		if( i % 10000 == 0 )
		{
			print_memory_usage();
		}
		GPlatesModel::FeatureHandle::weak_ref feature = 
			GPlatesModel::FeatureHandle::create(
					feature_collection,
					feature_type);

		GPlatesMaths::LatLonPoint llp(0,0);
		GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
		
		const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
			GPlatesPropertyValues::GmlPoint::create(p);

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(
					gml_point, 
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();

		for(int j = 0;j < 80; j++)
		{
			GPlatesPropertyValues::XsString::non_null_ptr_type key = 
				GPlatesPropertyValues::XsString::create(
						GPlatesUtils::make_icu_string_from_qstring("fieldname"));
			
			GPlatesPropertyValues::XsString::non_null_ptr_type value = 
				GPlatesPropertyValues::XsString::create(
						GPlatesUtils::make_icu_string_from_qstring("attribute.toString()"));
			
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
					key,
					value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("integer"));
			
			dictionary->elements().push_back(element);
		}

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
				dictionary));
		
		
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
				property_value));
	}
	std::cout << "End testing feature memory efficiency... " << std::endl;
std::cin.get();
	return;
#endif
}

void 
GPlatesUnitTest::FeatureHandleTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FeatureHandleTest::test_case_3()
{
#if 0
	//const int REPEAT_NUM = 100000;
	GPlatesModel::FeatureType feature_type = 
		GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = 
		GPlatesModel::FeatureCollectionHandle::create(d_model->root());

		print_memory_usage();
		std::cout<<"press any key to start testing boost pool"<<std::endl;
		std::cin.get();
		
		std::vector<TestStruct*> v;v.reserve(100000);
		for(int i=0; i<100000; ++i)
		{
		//std::cout << sizeof(TestStruct) << std::endl;
		//new TestStruct[100000];
		v.push_back(new TestStruct);
		}
		//my_pool::purge_memory();
		print_memory_usage();
		std::cout<<"press any key to continue"<<std::endl;
		std::cin.get();
		return;
#endif
}

void 
GPlatesUnitTest::FeatureHandleTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FeatureHandleTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FeatureHandleTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FeatureHandleTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::FeatureHandleTestSuite::construct_maps()
{
	boost::shared_ptr<FeatureHandleTest> instance(
		new FeatureHandleTest());

	ADD_TESTCASE(FeatureHandleTest,test_case_1);
	ADD_TESTCASE(FeatureHandleTest,test_case_2);
	ADD_TESTCASE(FeatureHandleTest,test_case_3);
	ADD_TESTCASE(FeatureHandleTest,test_case_4);
	ADD_TESTCASE(FeatureHandleTest,test_case_5);
	ADD_TESTCASE(FeatureHandleTest,test_case_6);
	ADD_TESTCASE(FeatureHandleTest,test_case_7);
}

