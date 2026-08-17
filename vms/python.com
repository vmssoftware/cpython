$ original_style = f$getjpi("","parse_style_perm")
$ SET PROCESS/PARSE_STYLE=EXTENDED
$
$ if "''P1'" .eqs. ""
$ then
$   CONFIG := DEBUG
$ else
$   CONFIG := 'P1'
$ endif
$ write sys$output "Prepare for ''CONFIG'"
$
$ com_nam = f$edit(f$environment("PROCEDURE"),"UPCASE")
$ com_dir = f$edit(f$parse(com_nam,,,"DIRECTORY"),"UPCASE")
$ com_dev = f$edit(f$parse(com_nam,,,"DEVICE"),"UPCASE")
$
$ com_pat = com_dev + com_dir
$ prj_pat = com_pat - ".VMS]"
$ bld_pat = prj_pat + ".OUT.''CONFIG']"
$ inc_pat = prj_pat + ".INCLUDE]"
$ cpy_pat = prj_pat + ".INCLUDE.CPYTHON]"
$ lib_pat = prj_pat + ".LIB...]"
$ vms_py_pat = prj_pat + ".MODULES.VMS...]"
$ rdb_py_pat = prj_pat + ".MODULES.RDB...]"
$ vms_pat = prj_pat + ".VMS]"
$ dyn_pat = prj_pat + ".OUT.'CONFIG'.LIB-DYNLOAD...]"
$ @'com_pat'python_def.com
$
$ pipe delete/tree python$root:[000000...]*.*;* | copy SYS$INPUT nl:
$
$ backup 'bld_pat'python.exe python$root:[bin]
$
$ backup 'inc_pat'*.h python$root:[include]
$ backup 'cpy_pat'*.h python$root:[include.cpython]
$ backup 'vms_pat'pyconfig.h python$root:[include]
$
$ backup 'lib_pat'*.* python$root:[lib.python3^.10...]
$ backup 'vms_py_pat'*.py python$root:[lib.python3^.10.vms]
$ backup 'rdb_py_pat'*.py python$root:[lib.python3^.10]
$ backup 'bld_pat'python$shr.exe python$root:[lib]
$
$ !backup 'vms_pat'python$define_root.com python$root:[000000]
$ backup 'vms_pat'python$pcsi_preconfigure.com python$root:[000000]
$ !backup 'vms_pat'python$startup.com python$root:[000000]
$ !backup 'vms_pat'python$shutdown.com python$root:[000000]
$
$ backup 'vms_pat'constraints.txt python$root:[lib]
$
$ backup 'dyn_pat'*.* python$root:[lib.python3^.10.lib-dynload...]*.*
$ backup 'bld_pat'_sysconfigdata__OpenVMS_cpython*.py python$root:[lib.python3^.10]*.*
$
$ SET PROCESS/PARSE_STYLE='original_style'