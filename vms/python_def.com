$ com_nam = f$edit(f$environment("PROCEDURE"),"UPCASE")
$ com_dir = f$edit(f$parse(com_nam,,,"DIRECTORY"),"UPCASE")
$ com_dev = f$edit(f$parse(com_nam,,,"DEVICE"),"UPCASE")
$ com_pat = com_dev + com_dir
$ INSTALL_DIR = com_pat - ".VMS]" + ".OUT.PYTHON.]"
$ define /trans=concealed python$root 'INSTALL_DIR'
$ define PYTHONHOME "/python$root"
$ define python$shr python$root:[lib]python$shr.exe
$ python :== $python$root:[bin]python.exe
