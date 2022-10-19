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

1. install wheel: $ python -m pip install wheel
2. see instruction in _[abcde]_*.txt
