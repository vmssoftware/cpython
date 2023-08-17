$ original_style = f$getjpi("","parse_style_perm")
$ SET PROCESS/PARSE_STYLE=EXTENDED
$
$ com_nam = f$environment("procedure")
$ com_dir = f$parse(com_nam,,,"directory")
$ com_dev = f$parse(com_nam,,,"device")
$ src_pat = com_dev + com_dir - "]" + "._internal]"
$ copy 'src_pat'configuration.py PYTHON$ROOT:[lib.python3^.10.site-packages.pip._internal]
$
$ SET PROCESS/PARSE_STYLE='original_style'