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
$ com_nam = f$environment("procedure")
$ com_dir = f$parse(com_nam,,,"directory")
$ com_dev = f$parse(com_nam,,,"device")
$ com_pat = com_dev + com_dir
$ prj_pat = com_pat - ".vms]"
$ bld_pat = prj_pat + ".out.''CONFIG']"
$ inc_pat = prj_pat + ".include]"
$ cpy_pat = prj_pat + ".include.cpython]"
$ lib_pat = prj_pat + ".lib...]"
$ vms_py_pat = prj_pat + ".modules.vms...]"
$ rdb_py_pat = prj_pat + ".modules.rdb...]"
$ vms_pat = prj_pat + ".vms]"
$ dyn_pat = prj_pat + ".out.'CONFIG'.lib-dynload...]"
$ @'com_pat'Python^.def.com
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
$ backup 'vms_pat'_sysconfigdata__OpenVMS_cpython-310-ia64-openvms.py python$root:[lib.python3^.10]
$
$ SET PROCESS/PARSE_STYLE='original_style'