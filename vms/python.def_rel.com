$ cur_pat = F$ENVIRONMENT("DEFAULT")
$ INSTALL_DIR = cur_pat - "]" + ".python.]"
$ define /trans=concealed python$root 'INSTALL_DIR'
$ define PYTHONHOME "/python$root"
$ define python$shr python$root:[lib]python$shr.exe
$ python :== $python$root:[bin]python.exe
