import sys
import unittest
import os
import tempfile

if sys.platform != 'OpenVMS':
    raise unittest.SkipTest('OpenVMS required')

import _rdb
import _decc

class BaseTestCase(unittest.TestCase):

    def setUp(self):
        self.dbname = ''
        self.dbname_vms = ''
        self.create_sql_database()

    def tearDown(self):
        if self.dbname:
            try:
                os.chmod(self.dbname, 0o777)
                os.unlink(self.dbname)
                name, ext = os.path.splitext(self.dbname)
                name = name + '.snp'
                os.chmod(name, 0o777)
                os.unlink(name)
            except:
                pass
        self.dbname = ''

    def create_sql_database(self):
        """ tests creating SQL database """

        sql_content = \
'''$!============================================================
$temp = f$search("{dbname}")
$if temp .nes. ""
$then
$   goto go_out
$endif
$mcr sql$
create database filename {dbname}
        number of users 500
        number of cluster nodes 1;
create table test_types(
        a           char(30),
        b           varchar(25),
        c           tinyint,
        d           smallint,
        e           integer,
        f           bigint,
        g           decimal,
        h           numeric,
        i           float,
        j           real,
        k           double precision,
        l           date vms,
        m           date ansi,
        n           time,
        o           timestamp,
        p           interval year to month,
        q           interval day to minute);
commit;
insert into test_types values(
        '1',
        'A',
        1,
        2,
        3,
        4,
        5,
        6,
        7.1,
        8.1,
        9.1,
        '01-JAN-1980',
        date '1980-01-02',
        time '12:13:14',
        timestamp '1980-01-02 12:13:14',
        interval '-1-2' year to month,
        interval '5:6:7' day to minute);
commit;
create table test_interval(
        a           interval year,
        b           interval month,
        c           interval day,
        d           interval hour,
        e           interval minute,
        f           interval second,
        g           interval year to month,
        h           interval day to hour,
        i           interval day to minute,
        j           interval day to second,
        k           interval hour to minute,
        l           interval hour to second,
        m           interval minute to second);
commit;
insert into test_interval values(
        interval '1' year,
        interval '2' month,
        interval '3' day,
        interval '4' hour,
        interval '5' minute,
        interval '6' second,
        interval '7-8' year to month,
        interval '9:10' day to hour,
        interval '11:12:13' day to minute,
        interval '14:15:16:17' day to second,
        interval '18:19' hour to minute,
        interval '20:21:22' hour to second,
        interval '23:24' minute to second);
commit;
insert into test_interval values(
        interval '-1' year,
        interval '-2' month,
        interval '-3' day,
        interval '-4' hour,
        interval '-5' minute,
        interval '-6' second,
        interval '-7-8' year to month,
        interval '-9:10' day to hour,
        interval '-11:12:13' day to minute,
        interval '-14:15:16:17' day to second,
        interval '-18:19' hour to minute,
        interval '-20:21:22' hour to second,
        interval '-23:24' minute to second);
commit;
insert into test_interval values(
        interval '99' year,
        interval '98' month,
        interval '97' day,
        interval '96' hour,
        interval '95' minute,
        interval '94' second,
        interval '99-11' year to month,
        interval '99:23' day to hour,
        interval '99:23:59' day to minute,
        interval '99:23:59:59' day to second,
        interval '99:59' hour to minute,
        interval '99:59:59' hour to second,
        interval '99:59' minute to second);
commit;
insert into test_interval values(
        interval '-99' year,
        interval '-98' month,
        interval '-97' day,
        interval '-96' hour,
        interval '-95' minute,
        interval '-94' second,
        interval '-99-11' year to month,
        interval '-99:23' day to hour,
        interval '-99:23:59' day to minute,
        interval '-99:23:59:59' day to second,
        interval '-99:59' hour to minute,
        interval '-99:59:59' hour to second,
        interval '-99:59' minute to second);
commit;
exit
$go_out:
$exit
'''
        fd, self.dbname = tempfile.mkstemp(suffix='.RDB')
        os.close(fd)
        os.unlink(self.dbname)
        self.dbname_vms = _decc.to_vms(self.dbname)[0]
        sql_content = sql_content.format(dbname=self.dbname_vms)
        with tempfile.NamedTemporaryFile(suffix=".COM", delete=False) as tmpfile:
            tmpfile.write(sql_content.encode())
            tmpfile.close()
            tmpfile_vms_name = _decc.to_vms(tmpfile.name)[0]
            p = os.popen('@' + tmpfile_vms_name)
            data = p.read()
            p.close()
            os.unlink(tmpfile.name)

    def test_sql_database(self):
        """ tests SQL database """

        breakpoint()

        sqlca = _rdb.sqlca()
        sqlca.attach(self.dbname_vms)
        self.assertNotEqual(sqlca.code, -1, sqlca.message)

        try:
            sqlca.set_readonly()
            self.assertNotEqual(sqlca.code, -1, sqlca.message)

            ch = sqlca.declare_cursor("C001", "select * from test_types")
            self.assertNotEqual(ch, None, sqlca.message)

            fields = ch.fields()
            self.assertNotEqual(fields, None, sqlca.message)

            ch.open_cursor()
            self.assertNotEqual(sqlca.code, -1, sqlca.message)

            pos = 0
            while ch.fetch() == 0:
                data = ch.data()
                self.assertNotEqual(data, None, sqlca.message)
                self.assertEqual(len(data), len(fields), "number of columns must be equal")
                pos += 1

            self.assertEqual(sqlca.code, _rdb.SQLCODE_EOS, sqlca.message)

            ch.close_cursor()
            self.assertNotEqual(sqlca.code, -1, sqlca.message)

            ch.release()
        finally:
            sqlca.detach()

    def test_sql_exec(self):
        sqlca = _rdb.sqlca()

        sqlca.attach(self.dbname_vms)
        self.assertNotEqual(sqlca.code, -1, sqlca.message)

        try:
            stmt = sqlca.prepare("insert into test_types (a) values (?)")
            self.assertIsNot(stmt, None, sqlca.message)

            stmt.exec('2')
            self.assertNotEqual(sqlca.code, -1, sqlca.message)

            stmt=sqlca.prepare("insert into test_interval(a,b,c,d,e,f,g,h,i,j,k,l,m) values(?,?,?,?,?,?,?,?,?,?,?,?,?)")
            self.assertIsNot(stmt, None, sqlca.message)

            stmt.exec(11,12,13,14,15,16,(17,11),(19,20),(21,22,23),(24,23,26,27),(28,29),(30,31,32),(33,34))
            self.assertNotEqual(sqlca.code, -1, sqlca.message)

            sqlca.commit()
        finally:
            sqlca.rollback()
            sqlca.detach()

    def test_sql_select(self):
        sqlca = _rdb.sqlca()

        sqlca.attach(self.dbname_vms)
        self.assertNotEqual(sqlca.code, -1, sqlca.message)

        try:
            stmt = sqlca.prepare("select * from test_types where a = ?")
            self.assertIsNot(stmt, None, sqlca.message)

            row = stmt.select('1')
            self.assertIsNot(row, None, sqlca.message)
            
            sqlca.commit()
        finally:
            sqlca.rollback()
            sqlca.detach()

if __name__ == "__main__":
    unittest.main(verbosity=2)
