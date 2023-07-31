import sys
import unittest

if sys.platform != 'OpenVMS':
    raise unittest.SkipTest('OpenVMS required')

import os
import time
from test.support.os_helper import TESTFN

import _decc as DECC
import _lib as LIB
import _syidef as SYIDEF
import _fabdef as FABDEF

class BaseTestCase(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_record_file(self):
        """ tests write-read record file"""

        lines = [
            b"Line #1\n",
            b"\n",
            b"Line #3\n",
        ]
        
        fd = os.open(TESTFN, os.O_RDWR | os.O_CREAT, rms=("rfm=var",))
        self.assertNotEqual(fd, -1)

        for line in lines:
            n = os.write(fd, line)
            self.assertEqual(n, len(line))
        os.close(fd)

        st = os.stat(TESTFN)
        self.assertEqual(st.st_fab_rfm, FABDEF.FAB_C_VAR)

        fp = open(TESTFN, "r")
        self.assertNotEqual(fp, None)

        for line in lines:
            line_read = fp.readline()
            self.assertEqual(line.decode(), line_read)

        fp.close()

        os.remove(TESTFN)

    def test_dlopen_test(self):
        """ tests if shared image is accessible """
        self.assertEqual(1, DECC.dlopen_test("python$shr"))

    def test_fix_time(self):
        """ converts vms time to unix time (GMT) """
        self.assertEqual(1595490469, DECC.fix_time(51022072693664076))

    def test_from_vms(self):
        """ converts vms path to available unix paths """
        from_vms_1 = list(map(lambda x: x.lower(), DECC.from_vms("python$root:[lib]python$shr.exe", 0)))
        from_vms_2 = list(map(lambda x: x.lower(), DECC.from_vms("python$root:[lib]python$shr.*", 1)))
        self.assertEqual(len(from_vms_1), 1)
        self.assertEqual(len(from_vms_2), 1)
        self.assertEqual(from_vms_1, from_vms_2)

    def test_getenv(self):
        """ try to get PYTHONHOME """
        self.assertEqual('/python$root', DECC.getenv('PYTHONHOME', None))

    def test_sleep(self):
        """ sleep one second """
        start = time.time()
        DECC.sleep(1)
        diff = time.time() - start
        self.assertGreaterEqual(diff, 1)
        self.assertLess(diff, 2)

    def test_sysconf(self):
        """ try to get PAGESIZE """
        pagesize_decc = DECC.sysconf(DECC.DECC_SC_PAGESIZE)
        status, pagesize_sys, _ = LIB.getsyi(SYIDEF.SYI__PAGE_SIZE, None)
        self.assertEqual(1, status)
        pagesize_sys = int(pagesize_sys)
        self.assertEqual(pagesize_decc, pagesize_sys)

    def test_to_vms(self):
        """ converts unix path to vms path """
        to_vms_1 = list(map(lambda x: x.lower(), DECC.to_vms('/python$root/lib/python$shr.exe', 0, 0)))
        to_vms_2 = list(map(lambda x: x.lower(), DECC.to_vms('/python$root/lib/python$shr.*', 1, 0)))
        self.assertEqual(len(to_vms_1), 1)
        self.assertEqual(len(to_vms_2), 1)
        self.assertEqual(to_vms_1, to_vms_2)

    def test_unixtime(self):
        """ converts vms time to unix time (using time zone) """
        DECC.unixtime(51022072693664076)

    def test_vmstime(self):
        """ converts unix time to vms time (GMT) """
        self.assertEqual(35067168000000000, DECC.vmstime(0))
        self.assertEqual(0, DECC.fix_time(DECC.vmstime(0)))

if __name__ == "__main__":
    unittest.main(verbosity=2)
