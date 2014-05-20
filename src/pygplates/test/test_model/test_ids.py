"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class FeatureIdCase(unittest.TestCase):
    def setUp(self):
        self.feature_id1 = pygplates.FeatureId.create_unique_id()
        self.feature_id2 = pygplates.FeatureId.create_unique_id()

    def test_ids(self):
        self.assertTrue(self.feature_id1 != self.feature_id2)
        # The format of a feature Id is 'GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'
        # where each x is a hexadecimal digit (0-9 ... a-f).
        self.assertTrue(len(self.feature_id1.get_string()) == len('GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'))
        self.assertTrue(self.feature_id1.get_string()[:7] == 'GPlates')


# Not including RevisionId yet since it is not really needed in the python API user (we can add it later though)...
#class RevisionIdCase(unittest.TestCase):
#    def setUp(self):
#        self.revision_id1 = pygplates.RevisionId.create_unique_id()
#        self.revision_id2 = pygplates.RevisionId.create_unique_id()
#
#    def test_ids(self):
#        self.assertTrue(self.revision_id1 != self.revision_id2)
#        # The format of a feature Id is 'GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'
#        # where each x is a hexadecimal digit (0-9 ... a-f).
#        self.assertTrue(len(self.revision_id1.get_string()) == len('GPlates-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'))
#        self.assertTrue(self.revision_id1.get_string()[:7] == 'GPlates')


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            FeatureIdCase
# Not including RevisionId yet since it is not really needed in the python API user (we can add it later though)...
#            , RevisionIdCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite
