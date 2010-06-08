#!/usr/bin/env python
#

import sys
import os
import string
import os.path
import re
from optparse import OptionParser

HELP_USAGE = "Usage: add_tests.py test_name\n"

__usage__ = "%prog test_name"
__description__ = "add testcase"\
                  "...\n"
	

template_header_file = "TestCase.template_h"
template_cc_file = "TestCase.template_cc"

	
def generate_code(path, test_case_name):
    
    fread = open(template_header_file, 'r')
    template_header_contents = fread.read()
    fread.close()
    fread = open(template_cc_file, 'r')
    template_cc_contents = fread.read()
    fread.close()
    #print template_header_contents
    #print template_cc_contents
 
    template_header_contents = template_header_contents.replace("$TESTCLASS_U$",test_case_name.upper() );	 
    template_header_contents = template_header_contents.replace("$TESTCLASS$",test_case_name );	
    
    #print template_header_contents
    template_cc_contents = template_cc_contents.replace("$TESTCLASS$",test_case_name );	
    #print template_cc_contents
    #print path

    fwrite = open(test_case_name + "Test.h", 'w')
    fwrite.write(template_header_contents)
    fwrite.close()
    fwrite = open(test_case_name + "Test.cc", 'w')
    fwrite.write(template_cc_contents)
    fwrite.close()

def main(argv):

    #print "hello world!\n"
    # Parse the command-line options. 
    parser = OptionParser(usage = __usage__,
                          description = __description__)
    
    # Parse command-line options.
    (options, args) = parser.parse_args()
    if len(args) > 1:
        parser.error("incorrect number of arguments")
	
    # Get the GPlates 'src/' directory from the location of this script (in 'cmake' directory).
    current_path = os.path.dirname( os.path.abspath(argv[0]) )
      
    #print  current_path
    #print __name__  
    #print argv[1]
    #print current_path.upper()
    generate_code(current_path, argv[1])

if __name__ == "__main__":
    main( sys.argv )