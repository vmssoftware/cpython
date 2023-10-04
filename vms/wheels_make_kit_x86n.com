$ set verify
$
$ delete/log/noconf vsi-x86vms-pythwhls*.pcsi;*
$ delete/log/noconf vsi-x86vms-pythwhls*.pcsi$compressed;*
$
$ product package pythwhls -
        /source=pythwhls_x86n.pcsi$desc -
        /destination=[] -
        /material=(python_wheels$root:) -
        /format=sequential -
        /opt=noconf -
        /log -
        /producer=VSI
$
$ product copy pythwhls/source=[]/dest=[]/format=compressed/opt=noconf
$ purge/log
$
$ purge/log [...]
$
$ set noverify
$
$ exit

