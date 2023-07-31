import sys
import unittest

if sys.platform != 'OpenVMS':
    raise unittest.SkipTest('OpenVMS required')

import _ile3
import _sys
import _syidef
import _dscdef

class BaseTestCase(unittest.TestCase):

    def test_getstr(self):
        """ tests getting str """
        il = _ile3.ile3list()
        self.assertEqual(0, len(il))
        il.append(_syidef.SYI__HW_NAME, _dscdef.DSC_K_DTYPE_T, 64)
        self.assertEqual(1, len(il))
        status, _ = _sys.getsyi(il)
        self.assertEqual(1, status)
        self.assertEqual(1, len(il))
        id, type, value = il.getat(0)
        self.assertEqual(id, _syidef.SYI__HW_NAME)
        self.assertEqual(type, _dscdef.DSC_K_DTYPE_T)
        hw_name = il[0]
        self.assertEqual(hw_name, value)
        self.assertNotIn(hw_name, (None, ''))

    def test_addint_getint(self):
        """ tests add int and get int """
        il = _ile3.ile3list()
        self.assertEqual(0, len(il))
        il.append(_syidef.SYI__ARCH_TYPE, _dscdef.DSC_K_DTYPE_QU, 0x0102030405060708)
        self.assertEqual(1, len(il))
        id, type, value = il.getat(0)
        self.assertEqual(id, _syidef.SYI__ARCH_TYPE)
        self.assertEqual(type, _dscdef.DSC_K_DTYPE_QU)
        given_value = il[0]
        self.assertEqual(given_value, value)
        self.assertEqual(0x0102030405060708, given_value)

if __name__ == "__main__":
    unittest.main(verbosity=2)
