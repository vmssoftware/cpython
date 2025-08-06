
1. Clone project
2. Build (for example, RELEASE_X86_64):
        $ mms/ext [.vms]cpython.mms /MACRO=("OUTDIR=OUT","CONFIG=RELEASE_X86_64","X86_64=1")
3. Create working set (for example, RELEASE_X86_64):
        $ set default [.vms]
        $ @python.com RELEASE_X86_64