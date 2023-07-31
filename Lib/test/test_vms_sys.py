import sys
import unittest
import time
import os
import threading

if sys.platform != 'OpenVMS':
    raise unittest.SkipTest('OpenVMS required')

import _sys as SYS
import _ile3 as ILE3
import _ssdef as SS
import _jpidef as JPI
import _syidef as SYI
import _iodef as IO
import _lnmdef as LNM
import _dscdef as DSC
import _psldef as PSL
import _dvsdef as DVS
import _dcdef as DC
import _dvidef as DVI
import _lkidef as LKI
import _quidef as QUI
import _jbcmsgdef as JBC
import _rmidef as RMI
import _uaidef as UAI

class BaseTestCase(unittest.TestCase):

    def setUp(self):
        self.context = 0

    def tearDown(self):
        if self.context:
            SYS.finish_rdb(self.context)

    def test_asctim(self):
        """ tests asctim """
        vms_time = 51022884689600846
        status, result = SYS.asctim(vms_time, 0)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual('24-JUL-2020 06:21:08.96', result)
        status, result = SYS.asctim(vms_time, 1)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual('06:21:08.96', result)

    def test_bintim(self):
        """ tests bintim """
        time_str = '24-JUL-2020 06:21:08.96'
        status, vms_time =  SYS.bintim(time_str)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual(51022884689600000, vms_time)

    def test_id(self):
        """ tests asctoid and idtoasc """
        name = 'SYSTEM'
        status, result_id, result_attrib = SYS.asctoid(name)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertGreater(result_id, 0)
        self.assertNotEqual(result_attrib, None)
        status, result_name, result_id, result_attrib, self.context = SYS.idtoasc(result_id, 0)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertNotEqual(self.context, 0)
        self.assertEqual(result_name, name)
        status = SYS.finish_rdb(self.context)
        self.context = 0
        self.assertEqual(status, SS.SS__NORMAL)

    def test_mbx(self):
        """ test mailbox """
        test_bytes = b'test bytes\n'
        mbx_name = "PythonTest"
        status, mbx_channel = SYS.crembx( \
            0,      # temporary/permanent
            0,      # maxmsg
            0,      # bufquo
            0,      # promsk
            0,      # acmode
            mbx_name,   # logname
            0)      # flags
        self.assertEqual(status, SS.SS__NORMAL)
        status, written, iostatus = SYS.writevblk(mbx_channel, test_bytes, 0, IO.IO_M_NOW | IO.IO_M_STREAM)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual(1, iostatus)
        self.assertEqual(len(test_bytes), written)
        status, read_channel = SYS.assign(mbx_name, 0, None, 0)
        self.assertEqual(status, SS.SS__NORMAL)
        status, read_bytes, iostatus = SYS.readvblk(read_channel, len(test_bytes), 0, IO.IO_M_NOW | IO.IO_M_STREAM)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual(1, iostatus)
        self.assertEqual(read_bytes, test_bytes)
        status = SYS.dassgn(read_channel)
        self.assertEqual(status, SS.SS__NORMAL)
        status = SYS.delmbx(mbx_channel)
        self.assertEqual(status, SS.SS__NORMAL)

    def test_cancel(self):
        """ test cancel """
        test_bytes = b'test bytes'
        mbx_name = "PythonTest"
        status, mbx_channel = SYS.crembx(0, 0, 0, 0, 0, mbx_name, 0)
        self.assertEqual(status, SS.SS__NORMAL)

        def cancel_thread_fn(retstat):
            time.sleep(1.0)
            retstat.append(SYS.cancel(mbx_channel))

        cancel_thread_retstat = []
        cancel_thread = threading.Thread(target=cancel_thread_fn, args=(cancel_thread_retstat,))
        cancel_thread.start()

        status, read_bytes, iostatus = SYS.readvblk(mbx_channel, len(test_bytes), 0, IO.IO_M_STREAM)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertIn(iostatus, (SS.SS__ABORT, SS.SS__CANCEL))
        self.assertEqual(0, len(read_bytes))

        cancel_thread.join()
        self.assertEqual(1, len(cancel_thread_retstat))
        self.assertEqual(1, cancel_thread_retstat[0])

        status = SYS.delmbx(mbx_channel)
        self.assertEqual(status, SS.SS__NORMAL)

    def test_logical_name(self):
        """ test create, translate and delete logical name """
        log_name = 'PythonTestCRELNM'
        log_value = 'Value of PythonTestCRELNM'
        log_table = 'LNM$PROCESS_TABLE'
        il = ILE3.ile3list()
        il.append(LNM.LNM__STRING, DSC.DSC_K_DTYPE_T, log_value)
        status = SYS.crelnm(log_name, log_table, il)
        self.assertIn(status, (SS.SS__NORMAL, SS.SS__SUPERSEDE))

        il = ILE3.ile3list()
        il.append(LNM.LNM__INDEX, DSC.DSC_K_DTYPE_LU)
        il.append(LNM.LNM__ATTRIBUTES, DSC.DSC_K_DTYPE_LU)
        il.append(LNM.LNM__LENGTH, DSC.DSC_K_DTYPE_LU)
        il.append(LNM.LNM__STRING, DSC.DSC_K_DTYPE_T, 255)
        il.append(LNM.LNM__TABLE, DSC.DSC_K_DTYPE_T, 32)
        status = SYS.trnlnm(log_name, log_table, il)
        attributes = il[1]
        value_length = il[2]
        value_str = il[3]
        table_str = il[4]
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertTrue(attributes & LNM.LNM_M_EXISTS)
        self.assertEqual(value_length, len(log_value))
        self.assertEqual(value_str, log_value)
        self.assertEqual(table_str, log_table)

        status = SYS.dellnm(log_name, log_table)
        self.assertEqual(status, SS.SS__NORMAL)

    def test_device_scan(self):
        """ test device_scan """
        il = ILE3.ile3list()
        il.append(DVS.DVS__DEVCLASS, DSC.DSC_K_DTYPE_LU, DC.DC__DISK)

        devices = []
        status, dev_name, context = SYS.device_scan('*', il)
        while status == SS.SS__NORMAL:
            devices.append(dev_name)
            status, dev_name, context = SYS.device_scan('*', il, context)

        self.assertEqual(status, SS.SS__NOMOREDEV)
        self.assertGreater(len(devices), 0)

    def test_uicstr(self):
        """ test uicstr """
        status, ret_str = SYS.uicstr(123)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual(ret_str, '[0,173]')

    def test_getdvi(self):
        """ test getdvi """
        il = ILE3.ile3list()
        il.append(DVS.DVS__DEVCLASS, DSC.DSC_K_DTYPE_LU, DC.DC__DISK)
        status, dev_name, _ = SYS.device_scan('*', il, 0)
        self.assertIn(status, (SS.SS__NORMAL, SS.SS__NOMOREDEV))

        il = ILE3.ile3list()
        il.append(DVI.DVI__DEVCHAR, DSC.DSC_K_DTYPE_LU, 0)

        status = SYS.getdvi(dev_name, il)
        characteristics = il[0]

        self.assertEqual(status, SS.SS__NORMAL)
        DEV_M_DIR = 0x8     # has directories
        self.assertTrue(characteristics & DEV_M_DIR)
        DEV_M_FOD = 0x4000  # is file oriented
        self.assertTrue(characteristics & DEV_M_FOD)

    def test_getjpi(self):
        """ test getjpi """
        il = ILE3.ile3list()
        il.append(JPI.JPI__PPGCNT, DSC.DSC_K_DTYPE_LU)
        status, pid = SYS.getjpi(il, 0)
        ppgcnt = il[0]

        self.assertEqual(status, SS.SS__NORMAL)
        self.assertGreater(pid, 0)
        self.assertGreater(ppgcnt, 0)

    def test_getlki(self):
        """ test getlki """
        locks = 0
        il = ILE3.ile3list()
        il.append(LKI.LKI__LOCKID, DSC.DSC_K_DTYPE_LU)
        status, lkid = SYS.getlki(il)
        while status == SS.SS__NORMAL:
            locks = locks + 1
            lockid = il[0]
            self.assertEqual(lockid & 0xffff, lkid & 0xffff)
            status, lkid = SYS.getlki(il, lkid)
        self.assertEqual(status, SS.SS__NOMORELOCK)

    def test_getmsg(self):
        """ test getmsg """
        msgid = SS.SS__HBMMCREPOSTMRG
        flags = 0x0f
        status, message, optional = SYS.getmsg(msgid, flags)
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual(message, '%SYSTEM-I-HBMMCREPOSTMRG, HBMM master bitmaps will be created after shadow set merge completes')
        self.assertEqual(optional, (0, 0))

    def test_getqui(self):
        """ test getqui """
        il = ILE3.ile3list()
        il.append(QUI.QUI__SEARCH_NAME, DSC.DSC_K_DTYPE_T, '*')
        il.append(QUI.QUI__QUEUE_NAME, DSC.DSC_K_DTYPE_T, 32)
        context = -1
        status, context = SYS.getqui(QUI.QUI__DISPLAY_QUEUE, context, il)
        qui_name = il[1]

        self.assertIn(status, (JBC.JBC__NOMOREQUE, SS.SS__NORMAL))
        self.assertNotEqual(qui_name, '')
        if status == SS.SS__NORMAL:
            status, context = SYS.getqui(QUI.QUI__CANCEL_OPERATION, context)
            self.assertEqual(status, SS.SS__NORMAL)

    def test_getrmi(self):
        """ test getrmi """
        il = ILE3.ile3list()
        il.append(RMI.RMI__CPUIDLE, DSC.DSC_K_DTYPE_QU)
        status = SYS.getrmi(il)
        cpu_idle = il[0]

        self.assertEqual(status, SS.SS__NORMAL)
        self.assertGreater(cpu_idle, 0)

    def test_getsyi(self):
        """ test getsyi """
        il = ILE3.ile3list()
        il.append(SYI.SYI__ARCH_NAME, DSC.DSC_K_DTYPE_T, 16)
        il.append(SYI.SYI__ARCH_TYPE, DSC.DSC_K_DTYPE_LU)
        status, csid = SYS.getsyi(il)
        arch_name = il[0]
        arch_type = il[1]

        self.assertEqual(status, SS.SS__NORMAL)
        self.assertNotEqual(arch_name, '')
        self.assertIn(arch_type, (1,2,3))

    def test_gettim(self):
        """ test gettim """
        status, time1 = SYS.gettim()
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertGreater(time1, 0)
        time.sleep(0.1)
        status, time2 = SYS.gettim()
        self.assertEqual(status, SS.SS__NORMAL)
        self.assertGreater(time2, time1)

    def test_getuai(self):
        """ test getuai """
        # setuai requires BYPASS or SYSPRV
        il = ILE3.ile3list()
        il.append(JPI.JPI__ACCOUNT, DSC.DSC_K_DTYPE_T, 8)
        il.append(JPI.JPI__USERNAME, DSC.DSC_K_DTYPE_T, 12)
        status, pid = SYS.getjpi(il, 0)
        jpi_account = il[0]
        jpi_username = il[1]

        self.assertEqual(status, SS.SS__NORMAL)
        self.assertNotEqual(jpi_account, '')
        self.assertNotEqual(jpi_username, '')
        self.assertNotEqual(pid, 0)

        il = ILE3.ile3list()
        il.append(UAI.UAI__ACCOUNT, DSC.DSC_K_DTYPE_T, 32)
        il.append(UAI.UAI__DEFDIR, DSC.DSC_K_DTYPE_VT, 64)
        il.append(UAI.UAI__UIC, DSC.DSC_K_DTYPE_LU)
        status = SYS.getuai(jpi_username, il)
        uai_account = il[0]
        uai_defdir = il[1]
        uai_uic = il[2]

        self.assertEqual(status, SS.SS__NORMAL)
        self.assertEqual(jpi_account.strip(), uai_account.strip())
        self.assertNotEqual(uai_defdir, '')
        self.assertNotEqual(uai_uic, 0)

    def test_hiber(self):
        """ test hiber and schdwk """
        hiber_time = 10000000   # in 100 nanoseconds
        status, _ = SYS.schdwk(-hiber_time)
        self.assertEqual(status, SS.SS__NORMAL)
        # status, time1 = SYS.gettim()
        status = SYS.hiber()
        self.assertEqual(status, SS.SS__NORMAL)
        # status, time2 = SYS.gettim()
        # self.assertLessEqual(abs((time2 - time1)-hiber_time), 10000)

    # # Write access to the rights database is required.
    # def test_ident(self):
    #     """ test add_ident, rem_ident """
    #     status, res_id = SYS.add_ident('PythonTestID', 0, 0)
    #     self.assertEqual(status, SS.SS__NORMAL)
    #     status, res_id = SYS.rem_ident(res_id)
    #     self.assertEqual(status, SS.SS__NORMAL)

if __name__ == "__main__":
    unittest.main(verbosity=2)
