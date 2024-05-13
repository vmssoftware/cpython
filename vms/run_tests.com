$ on error then $ gosub on_error
$ do_tests = 1
$ ! P1 = failed test name
$ if "''P1'" .nes. "" then do_tests = 0
$ if f$search("test_names.txt") .eqs. "" then $ pipe python -m test --list-tests > test_names.txt
$ close test_names
$ open test_names test_names.txt
$ loop:
$ read/end_of_file=file_end test_names name
$ if do_tests .eq. 1 
$ then 
$ show time
$ if f$search("test_python_*.dir") .nes. ""
$ then
$   delete/tree [.test_python_*...]*.*;* /nolog
$   free$ test_python_*.dir;1
$   delete test_python_*.dir;1
$ endif
$ python -m test -W 'name'
$ endif
$ ! skip all until failed test, do the next
$ if F$EDIT("''P1'", "TRIM, UPCASE") .eqs. F$EDIT("''name'", "TRIM, UPCASE") then do_tests = 1
$ goto loop
$ file_end:
$ close test_names
$ del test_names.txt;*
$ exit
$ on_error:
$ on error then $ gosub on_error
$ return
