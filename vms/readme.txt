Creating python kit (do not forget change the version in create_kit_files.py if it is required)

1. upload project
2. select build type (release, release_64 ...)
3. build
4. $ set def [.vms]
5. $ @python.com release
5.64 $ @python.com release_64
6. $ python -m ensurepip --default-pip
7. $ python -m compileall ../out/python
8. $ python create_kit_files.py
8.64 $ python create_kit_files64.py
9. $ @make_kit.com
9.64 $ @make_kit64.com

Creating wheels kit (do not forget change the version in wheels_create_kit_files[64].py if it is required)

0. clear pip cache: $ python -m pip cache purge
1. NO ----------> install wheel: $ python -m pip install wheel <------- NO
1.1 If Python or Wheels are already installed, re-define PIP_NO_INDEX and PIP_FIND_LINKS
2. see instruction in _[abcde]_*.txt
3. $ define /tran=conc python_wheels$root <wheels folder>
4. $ python wheels_create_kit_files.py
4.64 $ python wheels_create_kit_files64.py
5. $ @wheels_make_kit.com
5.64 $ @wheels_make_kit64.com
