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
            ts = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(i),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(i)))
            self.original_time_samples.append(ts)
            
        # Store the actual GpmlTimeSampleList (ie, RevisionedVector) for use by the test cases.
        gpml_irregular_sampling = pygplates.GpmlIrregularSampling.create(self.original_time_samples)
        self.gpml_time_sample_list = gpml_irregular_sampling.get_time_samples()

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
        ts = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(100),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(100)))
        self.gpml_time_sample_list[2] = ts
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(self.gpml_time_sample_list[2] == ts)

    def test_set_slice(self):
        ts1 = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(100),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(100)))
        ts2 = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(200),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(200)))
        ts3 = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(300),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(300)))
        self.gpml_time_sample_list[1:3] = [ts1, ts2, ts3]
        # The length should increase by one.
        self.assertTrue(len(self.gpml_time_sample_list) == 5)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == ts1)
        self.assertTrue(self.gpml_time_sample_list[2] == ts2)
        self.assertTrue(self.gpml_time_sample_list[3] == ts3)
        self.assertTrue(self.gpml_time_sample_list[4] == self.original_time_samples[3])

    def test_set_extended_slice(self):
        ts1 = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(100),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(100)))
        ts2 = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(200),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(200)))
        ts3 = pygplates.GpmlTimeSample.create(
                pygplates.XsInteger.create(300),
                pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(300)))
        self.gpml_time_sample_list[1::2] = [ts1, ts2]
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == ts1)
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[2])
        self.assertTrue(self.gpml_time_sample_list[3] == ts2)
        with self.assertRaises(ValueError):
            # Sequence size not equal to extended slice size.
            self.gpml_time_sample_list[1::2] = [ts1, ts2, ts3]

    def test_get_item(self):
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[2])

    def test_get_slice(self):
        slice = self.gpml_time_sample_list[1:]
        self.assertTrue(len(slice) == 3)
        self.assertTrue(slice[0] == self.original_time_samples[1])
        self.assertTrue(slice[1] == self.original_time_samples[2])
        self.assertTrue(slice[2] == self.original_time_samples[3])

    def test_get_extended_slice(self):
        slice = self.gpml_time_sample_list[1::2]
        self.assertTrue(len(slice) == 2)
        self.assertTrue(slice[0] == self.original_time_samples[1])
        self.assertTrue(slice[1] == self.original_time_samples[3])

    def test_del_item(self):
        del self.gpml_time_sample_list[2]
        self.assertTrue(len(self.gpml_time_sample_list) == 3)
        self.assertTrue(self.gpml_time_sample_list[0] == self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list[1] == self.original_time_samples[1])
        self.assertTrue(self.gpml_time_sample_list[2] == self.original_time_samples[3])

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
        
        ts = pygplates.GpmlTimeSample.create(
            pygplates.XsInteger.create(100),
            pygplates.GmlTimeInstant.create(pygplates.GeoTimeInstant(100)))
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
        self.gpml_time_sample_list.extend(self.gpml_time_sample_list)
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
        self.gpml_time_sample_list.insert(len(self.gpml_time_sample_list), self.original_time_samples[2])
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
        self.assertTrue(self.gpml_time_sample_list.index(self.original_time_samples[2], 1, 3) == 2)
        # Value not in limited range.
        with self.assertRaises(ValueError):
            self.gpml_time_sample_list.index(self.original_time_samples[2], 1, 2)
        # Indices of limited range are incorrect.
        with self.assertRaises(IndexError):
            self.gpml_time_sample_list.index(self.original_time_samples[2], 2, 1)

    def test_count(self):
        self.assertTrue(self.gpml_time_sample_list.count(self.original_time_samples[1]) == 1)
        self.gpml_time_sample_list.append(self.original_time_samples[3])
        self.assertTrue(self.gpml_time_sample_list.count(self.original_time_samples[3]) == 2)
        self.gpml_time_sample_list += (self.original_time_samples[0], self.original_time_samples[0])
        self.assertTrue(self.gpml_time_sample_list.count(self.original_time_samples[0]) == 3)

    def test_reverse(self):
        self.gpml_time_sample_list.reverse()
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        self.assertTrue(list(self.gpml_time_sample_list) == list(reversed(self.original_time_samples)))

    def test_sort(self):
        # Change the time of one element so the sequence is no longer ordered by time.
        # Times of [0, 1, 2, 3] -> [0, 2.5, 2, 1]
        self.gpml_time_sample_list[1].get_valid_time().set_time_position(pygplates.GeoTimeInstant(2.5))
        self.gpml_time_sample_list[3].get_valid_time().set_time_position(pygplates.GeoTimeInstant(1))
        self.assertTrue(len(self.gpml_time_sample_list) == 4)
        # Sort by time from present day to past times ('reverse=True').
        # Times of [0, 2.5, 2, 1] -> [0, 1, 2, 2.5]
        # Note that 2Ma < 1Ma because 2Ma is further in the past (which is why we need to reverse the ordering).
        self.gpml_time_sample_list.sort(key = lambda x: x.get_valid_time().get_time_position(), reverse=True)
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
