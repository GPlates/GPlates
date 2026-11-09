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

    def test_insert_out_of_range(self):
        # Like 'list.insert()', an out-of-range index is clamped (rather than raising IndexError).
        expected_list = list(self.original_time_samples)
        for index, element_index in ((100, 1), (-100, 2), (-1, 3), (len(expected_list) + 2, 0)):
            self.gpml_time_sample_list.insert(index, self.original_time_samples[element_index])
            expected_list.insert(index, self.original_time_samples[element_index])
            self.assertTrue(list(self.gpml_time_sample_list) == expected_list)

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
        # An empty search range is just "not found" (as for a Python list).
        self.assertRaises(ValueError, self.gpml_time_sample_list.index, self.original_time_samples[2], 2, 1)

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

    def test_matches_list(self):
        # 'index()', 'remove()' and slice assignment behave like a Python list.
        values = [0, 1, 2, 1, 3]
        def make():
            return pygplates.GpmlArray([pygplates.XsInteger(value) for value in values]), list(values)
        def contents(array):
            return [element.get_integer() for element in array]
        def outcome(function):
            try:
                return 'returned', function()
            except Exception as exc:
                return 'raised', type(exc)

        index_cases = [
                (1,), (1, 2), (1, -3), (1, 0, 0), (1, 2, 1), (1, -100, 100), (1, 4, 2), (1, 100),
                (9,), (3, -1), (3, -2, -1), (0, 0, -5), (1, 3, 100)]
        for args in index_cases:
            array, list_ = make()
            self.assertEqual(
                    outcome(lambda: array.index(pygplates.XsInteger(args[0]), *args[1:])),
                    outcome(lambda: list_.index(args[0], *args[1:])),
                    args)
        # Not found, including in an empty list (and an element of the wrong type).
        array, list_ = make()
        self.assertRaises(ValueError, array.index, pygplates.XsDouble(1))
        del array[:]
        self.assertRaises(ValueError, array.index, pygplates.XsInteger(1))
        self.assertRaises(ValueError, array.remove, pygplates.XsInteger(1))

        slices = [slice(2, 2, 2), slice(2, 2), slice(None, None, -1), slice(1, 4, 2), slice(0, 0, -1), slice(None, None, 0)]
        new_values_cases = [[], [7], [7, 8], [7, 8, 9, 10, 11]]
        for slice_ in slices:
            for new_values in new_values_cases:
                array, list_ = make()
                array_outcome = outcome(lambda: array.__setitem__(slice_, [pygplates.XsInteger(value) for value in new_values]))
                list_outcome = outcome(lambda: list_.__setitem__(slice_, new_values))
                self.assertEqual(array_outcome, list_outcome, (slice_, new_values))
                self.assertEqual(contents(array), list_, (slice_, new_values))
        # A zero step raises ValueError (as for a list), whether getting, setting or deleting.
        array, list_ = make()
        self.assertRaises(ValueError, array.__getitem__, slice(None, None, 0))
        self.assertRaises(ValueError, array.__delitem__, slice(None, None, 0))

    def test_empty(self):
        # Operations that replace all elements also work on an empty list (previously AssertionFailureError).
        del self.gpml_time_sample_list[:]
        self.gpml_time_sample_list.reverse()
        self.gpml_time_sample_list.sort(key = lambda x: x.get_time())
        del self.gpml_time_sample_list[:]
        self.assertTrue(len(self.gpml_time_sample_list) == 0)

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
