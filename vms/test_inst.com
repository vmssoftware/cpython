$ define /user PYTHONUSERBASE local
$ python -m pip install --user 'P1
$ define /user PYTHONUSERBASE local
$ python -c 'P2
$ del /tree [.local...]*.*;*
