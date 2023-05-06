"""
Unit tests for the pygplates property values.
"""

import os
import unittest
import pygplates

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), '..', 'fixtures')


class GpmlTimeSampleListCase(unittest.TestCase):
    """Test case for a RevisionedVector of GpmlTimeSamples (known as GpmlTimeSampleList)
    
    We only need to test one type of C++ template instantiation of RevisionedVector<>,
    so we choose RevisionedVector<GpmlTimeSample>."""
    
    def setUp(self):
        self.original_time_samples = []
        for i in range(0,4):
            ts = pygplates.GpmlTimeSample(pygplates.XsInteger(i), i)
            self.original_time_samples.append(ts)
            
        # Store the actual GpmlTimeSampleList (ie, RevisionedVector) for use by the test cases.
        self.gpml_irregular_sampling = pygplates.GpmlIrregularSampling(self.original_time_samples)
        self.gpml_time_sample_list = self.gpml_irregular_sampling.get_time_samples()

    def test_equality(self):
        gpml_irregular_sampling_clone = self.gpml_irregular_sampling.clone()
        self.assertTrue(gpml_irregular_sampling_clone.get_time_samples() == self.gpml_irregular_sampling.get_time_samples())
    
    def test_iter(self):
        """Test iteration over revisioned vector."""
        
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        
        time_samples = []
        for ts in self.gpml_time_sample_list:
            time_samples.append(ts)
        self.assertTrue(len(time_samples) == 4)
        
        # Test the iterated time sample values are correct.
        for i in range(0,4):
            self.assertTrue(time_samples[i] == self.original_time_samples[i])

    def test_set_item(self):
        ts = pygplates.GpmlTimeSample(pygplates.XsInteger(100), 100)
        self.gpml_time_sample_list[2] = ts
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(self.gpml_time_sample_list[2] == ts)

    def test_set_slice(self):
        ts1 = pygplates.GpmlTimeSample(pygplates.XsInteger(100), 100)
        ts2 = pygplates.GpmlTimeSample(pygplates.XsInteger(200), 200)
        ts3 = pygplates.GpmlTimeSample(pygplates.XsInteger(300), 300)
        self.gpml_time_sample_list[1:3] = [ts1, ts2, ts3]
        # The length should increase by one.
        self.assertTrue(len(self.gpml_time_sample_list) == 5)
        self.assertTrue(list(self.gpml_time_sample_list) == [
                self.original_time_samples[0], ts1, ts2, ts3, self.original_time_samples[3]])
        ts4 = pygplates.GpmlTimeSample(pygplates.XsInteger(400), 400)
        # Insert an entry (without deleting any).
        self.gpml_time_sample_list[1:1] = [ts4]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                self.original_time_samples[0], ts4, ts1, ts2, ts3, self.original_time_samples[3]])
        self.gpml_time_sample_list[0:0] = [ts1]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                ts1, self.original_time_samples[0], ts4, ts1, ts2, ts3, self.original_time_samples[3]])
        self.gpml_time_sample_list[:0] = [ts2]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                ts2, ts1, self.original_time_samples[0], ts4, ts1, ts2, ts3, self.original_time_samples[3]])
        self.gpml_time_sample_list[-1:-1] = [ts4]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                ts2, ts1, self.original_time_samples[0], ts4, ts1, ts2, ts3, ts4, self.original_time_samples[3]])
        self.gpml_time_sample_list[100:100] = [ts2]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                ts2, ts1, self.original_time_samples[0], ts4, ts1, ts2, ts3, ts4, self.original_time_samples[3], ts2])
        self.gpml_time_sample_list[-100:-100] = [ts4]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                ts4, ts2, ts1, self.original_time_samples[0], ts4, ts1, ts2, ts3, ts4, self.original_time_samples[3], ts2])

    def test_set_extended_slice(self):
        ts1 = pygplates.GpmlTimeSample(pygplates.XsInteger(100), 100)
        ts2 = pygplates.GpmlTimeSample(pygplates.XsInteger(200), 200)
        ts3 = pygplates.GpmlTimeSample(pygplates.XsInteger(300), 300)
        self.gpml_time_sample_list[1::2] = [ts1, ts2]
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == ts1)
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[3] == ts2)
        # Sequence size not equal to extended slice size.
        def assign_slice1():
            self.gpml_time_sample_list[1::2] = [ts1, ts2, ts3]
        self.assertRaises(ValueError, assign_slice1)

    def test_get_item(self):
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[2])

    def test_get_slice(self):
        slice = self.gpml_time_sample_list[1:]
        self.assertTrue(len(slice) == 3)
        self.assertTrue(slice[0] == self.original_time_samples[1])
        self.assertTrue(slice[1] == self.original_time_samples[2])
        self.assertTrue(slice[2] == self.original_time_samples[3])
        # Return empty list.
        self.assertTrue(list(self.gpml_time_sample_list[:0]) == [])
        self.assertTrue(list(self.gpml_time_sample_list[0:0]) == [])
        self.assertTrue(list(self.gpml_time_sample_list[5:5]) == [])
        self.assertTrue(list(self.gpml_time_sample_list[-1:-1]) == [])

    def test_get_extended_slice(self):
        slice = self.gpml_time_sample_list[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.original_time_samples[1])
        self.assertTrue(slice[1] == self.original_time_samples[3])

    def test_del_item(self):
        del self.gpml_time_sample_list[2]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                self.original_time_samples[0], self.original_time_samples[1], self.original_time_samples[3]])
        # Deleting empty range shouldn't change anything.
        del self.gpml_time_sample_list[1:1]
        self.assertTrue(list(self.gpml_time_sample_list) == [
                self.original_time_samples[0], self.original_time_samples[1], self.original_time_samples[3]])

    def test_del_slice(self):
        del self.gpml_time_sample_list[1:3]
        self.assertTrue(len(self.gpml_time_sample_list) == 2)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[3])

    def test_del_extended_slice(self):
        del self.gpml_time_sample_list[::2]
        self.assertTrue(len(self.gpml_time_sample_list) == 2)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[1])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[3])

    def test_contains(self):
        for i in range(0, len(self.original_time_samples)):
            self.assertTrue(self.original_time_samples[i] in self.gpml_time_sample_list)
        
        ts = pygplates.GpmlTimeSample(pygplates.XsInteger(100), 100)
        self.assertFalse(ts in self.gpml_time_sample_list)

    def test_iadd1(self):
        # Add a regular list.
        self.gpml_time_sample_list += self.original_time_samples
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[i])

    def test_iadd2(self):
        # Add a revisioned vector.
        self.gpml_time_sample_list += self.gpml_time_sample_list
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[i])

    def test_add1(self):
        # Add a revisioned vector and a revisioned vector.
        self.gpml_time_sample_list[:] = reversed(self.gpml_time_sample_list) + self.gpml_time_sample_list
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[3-i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[i])

    def test_add2(self):
        # Add a revisioned vector and a regular list.
        self.gpml_time_sample_list[:] = self.gpml_time_sample_list + reversed(self.original_time_samples)
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[3-i])

    def test_add3(self):
        # Add a regular list and a revisioned vector.
        self.gpml_time_sample_list[:] = reversed(self.original_time_samples) + self.gpml_time_sample_list
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[3-i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[i])

    def test_append(self):
        self.gpml_time_sample_list.append(self.original_time_samples[2])
        self.assertTrue(len(self.gpml_time_sample_list) == 5)
        self.assertTrue(self.gpml_time_sample_list[4] == self.original_time_samples[2])

    def test_extend1(self):
        # Add a regular list.
        self.gpml_time_sample_list.extend(self.original_time_samples)
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[i])

    def test_extend2(self):
        # Add a revisioned vector.
        self.gpml_time_sample_list.extend(t=self.gpml_time_sample_list)
        self.assertTrue(len(self.gpml_time_sample_list) == 8)
        for i in range(0,4):
            self.assertTrue(self.gpml_time_sample_list[i] == self.original_time_samples[i])
            self.assertTrue(self.gpml_time_sample_list[i+4] == self.original_time_samples[i])

    def test_insert(self):
        self.gpml_time_sample_list.insert(1, self.original_time_samples[3])
        self.assertTrue(len(self.gpml_time_sample_list) == 5)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[3])
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[1])
        self.assertTrue(self.gpml_time_sample_list[3] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[4] == self.original_time_samples[3])
        
        # Insert at the end.
        self.gpml_time_sample_list.insert(i=len(self.gpml_time_sample_list), x=self.original_time_samples[2])
        self.assertTrue(len(self.gpml_time_sample_list) == 6)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[3])
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[1])
        self.assertTrue(self.gpml_time_sample_list[3] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[4] == self.original_time_samples[3])
        self.assertTrue(self.gpml_time_sample_list[5] == self.original_time_samples[2])

    def test_remove(self):
        self.gpml_time_sample_list.remove(self.original_time_samples[1])
        self.assertTrue(len(self.gpml_time_sample_list) == 3)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[3])

    def test_pop(self):
        self.gpml_time_sample_list.pop(0)
        self.assertTrue(len(self.gpml_time_sample_list) == 3)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[1])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[3])
        
        # Pop the last element (default).
        self.gpml_time_sample_list.pop()
        self.assertTrue(len(self.gpml_time_sample_list) == 2)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[1])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[2])

    def test_index(self):
        self.assertTrue(self.gpml_time_sample_list.index(self.original_time_samples[1]) == 1)
        self.assertTrue(self.gpml_time_sample_list.index(self.original_time_samples[2], 1) == 2)
        self.assertTrue(self.gpml_time_sample_list.index(x=self.original_time_samples[2], i=1, j=3) == 2)
        # Value not in limited range.
        self.assertRaises(ValueError, self.gpml_time_sample_list.index, self.original_time_samples[2], 1, 2)
        # Indices of limited range are incorrect.
        self.assertRaises(IndexError, self.gpml_time_sample_list.index, self.original_time_samples[2], 2, 1)

    def test_count(self):
        self.assertTrue(self.gpml_time_sample_list.count(self.original_time_samples[1]) == 1)
        self.gpml_time_sample_list.append(self.original_time_samples[3])
        self.assertTrue(self.gpml_time_sample_list.count(self.original_time_samples[3]) == 2)
        self.gpml_time_sample_list += (self.original_time_samples[0], self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list.count(x=self.original_time_samples[0]) == 3)

    def test_reverse(self):
        self.gpml_time_sample_list.reverse()
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(list(self.gpml_time_sample_list) == list(reversed(self.original_time_samples)))

    def test_sort(self):
        # Change the time of one element so the sequence is no longer ordered by time.
        # Times of [0, 1, 2, 3] -> [0, 2.5, 2, 1]
        self.gpml_time_sample_list[1].set_time(2.5)
        self.gpml_time_sample_list[3].set_time(1)
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        # Sort by time.
        # Times of [0, 2.5, 2, 1] -> [0, 1, 2, 2.5]
        self.gpml_time_sample_list.sort(key = lambda x: x.get_time())
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[3])
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[3] == self.original_time_samples[1])


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            GpmlTimeSampleListCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    return suite