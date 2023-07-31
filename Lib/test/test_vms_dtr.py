import sys
import unittest

if sys.platform != 'OpenVMS':
    raise unittest.SkipTest('OpenVMS required')

try:
    import _dtr
except ImportError:
    raise unittest.SkipTest('_dtr is not built')

import _ssdef

def _show_messages(dab):
    while dab.state == _dtr.DTR_K_STL_MSG:
        print(dab.message)
        dab.cont()

class BaseTestCase(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_demo_01(self):
        """ tests demo 01 """
        # r = rec.new()
        # rec.addstr(r, None, 10, 0)
        # rec.addstr(r, None, 10, 10)
        # rec.addstr(r, None, 6, 20)
        # rec.addstr(r, None, 3, 26)
        # rec.addstr(r, None, 5, 29)
        # rec.addstr(r, None, 2, 34)
        # rec.addstr(r, None, 5, 36)

        dab = _dtr.dab(100)

        self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Initialization failed")

        dab.command("set dictionary cdd$top.dtr$lib.demo;")
        self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Command 'set dictionary' failed")
        _show_messages(dab)

        dab.command("declare port yport using yacht;")

        self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Command 'declare port' failed")
        _show_messages(dab)

        dab.command("ready yachts; ready yport write;")
        self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Command 'ready' failed")
        _show_messages(dab)

        dab.command("yport = yachts with loa > 30;")
        self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Command '=' failed")
        _show_messages(dab)

        while dab.state == _dtr.DAB_K_STL_PGET:
            port_data = dab.port
            self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Unable to retrieve data")
            self.assertEqual(len(port_data), dab.rec_length, "Invalid record length")
            _show_messages(dab)

        dab.finish()

        self.assertEqual(dab.status, _ssdef.SS__NORMAL, "Problems tidying up")

    # def test_demo_02(self):
    #     """ tests demo 02 """
    #     sts, dab = dtr.init(100, 0)

    #     self.assertEqual(sts, _ssdef.SS__NORMAL, "initialization failed")

    #     sts, cond, state = dtr.command(dab, "set dictionary cdd$top.dtr$lib.demo.rdb;")

    #     self.assertEqual(sts, _ssdef.SS__NORMAL, "Command 'set dictionary' failed")

    #     if state == dtr._K_STL_MSG:
    #         errorMsg = dtr.msg_buf(dab)
    #         dtr.cont(dab)

    #     sts, cond, state = dtr.command(dab, "ready jobs shared read;")

    #     self.assertEqual(sts, _ssdef.SS__NORMAL, "Command 'ready' failed")

    #     while state == dtr._K_STL_MSG:
    #         errorMsg = dtr.msg_buf(dab)
    #         sts, cond, state = dtr.cont(dab)

    #     sts, cond, state = dtr.command(dab, "print jobs;")

    #     self.assertEqual(sts, _ssdef.SS__NORMAL, "Command 'print' failed")

    #     sts = dtr.skip(dab, 4)

    #     fmt = "%4c   %1c   %20c %11c  %11c"

    #     while dtr.state(dab) == dtr._K_STL_LINE:
    #         row = dtr.row(dab, fmt)

    #     sts = dtr.finish(dab)

    #     self.assertEqual(sts, _ssdef.SS__NORMAL, "Problems tidying up")

if __name__ == "__main__":
    unittest.main(verbosity=2)
