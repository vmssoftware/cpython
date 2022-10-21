$ set verify
$
$ delete/log/noconf vsi-i64vms-pythwhls64-*.pcsi;*
$ delete/log/noconf vsi-i64vms-pythwhls64-*.pcsi$compressed;*
$
$ product package pythwhls64 -
        /source=pythwhls64.pcsi$desc -
        /destination=[] -
        /material=(python_wheels$root:) -
        /format=sequential -
        /opt=noconf -
        /log -
        /producer=VSI
$
$ product copy pythwhls64 /source=[] /dest=[] /format=compressed /opt=noconf
$ purge/log
$
$ purge/log [...]
$
$ set noverify
$
$ exit

