import os
import re
import sys

def spec_replacer(match):
    if match.group(0) == ' ':
        return '^_'
    return '^' + match.group(0)

def create_content(type, major, minor, level, edit):
    python_dir = '/python$root'
    python_dir_len = len(python_dir)
    all_dirs = []
    all_files = []
    spec_pattern = re.compile('([. ^+()])')
    for root, dirs, files in os.walk(python_dir):
        inner_dirs = list(filter(lambda x: x != '', spec_pattern.sub(spec_replacer, root[python_dir_len:]).split('/')))
        kit_dir = '[' + '.'.join(['python'] + inner_dirs) + ']'
        all_dirs.append('directory "' + kit_dir + '" version limit 1;')
        for file in files:
            file_name, file_ext = os.path.splitext(file)
            if file_ext == '':
                file_ext = '.'
            file_name = spec_pattern.sub(spec_replacer, file_name)
            all_files.append('file "' + \
                kit_dir + file_name + file_ext + \
                '" source "' + \
                kit_dir + file_name + file_ext + \
                '";')
            if file == '_sysconfigdata__OpenVMS_cpython-310-x86_64-openvms.py':
                full_name = os.path.join(root, file)
                with open(full_name, 'r') as file_:
                    lines = file_.readlines()
                rewrite = False
                for idx, line in enumerate(lines):
                    if 'SIZEOF_VOID_P' in line:
                        if line != " 'SIZEOF_VOID_P': 4,\n":
                            lines[idx] = " 'SIZEOF_VOID_P': 4,\n"
                            rewrite = True
                        break
                if rewrite:
                    with open(full_name, 'w') as file_:
                        file_.writelines(lines)

    kit_template = '''--
-- (C) Copyright 2022 VMS Software Inc.
--
product VSI X86VMS PYTHON {type}{major}.{minor}-{level}{edit} FULL ;

--
--  Execute the preconfigure procedure
--
    execute preconfigure "@pcsi$source:[python]python$pcsi_preconfigure.com" uses [python]python$pcsi_preconfigure.com ;

--
--  Make sure appropriate VMS version is installed
--
    if ((not <software VSI X86VMS VMS version minimum E9.2>)) ;
        error NO_MIN_VMS abort ;
    end if ;

--
--  ODS-5 Disk(s) should be available on this system
--
    if (<logical name PYTHON$ODS5_AVAIL equals 0 table LNM$JOB>) ;
        error NO_ODS5_DISKS ;
    end if ;

--
-- Directories...
--

    {dirs}

--
-- Files...
--

    {files}

--
-- Start-up and shutdown scripts
--

--
-- Release notes
--
-- (none)


--
-- Do post-install tasks
--
    execute postinstall (
        "root = f$trnlmn(""pcsi$destination"") - ""]"" + ""python.]""",
        "define/system/trans=concealed python$root 'root'",
        "define/system python$shr python$root:[lib]python$shr.exe",
        "define/system PYTHONHOME ""/python$root""",
        "python :== $python$root:[bin]python.exe",
        "open/write fd sys$startup:python$startup.com",
        "write fd ""$!Define logical names for Python""",
        "write fd ""$python :== $python$root:[bin]python.exe""",
        "write fd ""$define/system/trans=concealed python$root ''root'""",
        "write fd ""$define/system python$shr python$root:[lib]python$shr.exe""",
        "write fd ""$define/system PYTHONHOME """"/python$root""""",
        "write fd ""$exit""",
        "close fd",
        "set file /prot=(W:RE) sys$startup:python$startup.com"
     );

--
-- Okay, done.  Tell the user what to do next.
--
   information POST_INSTALL  phase after with helptext;

--
-- All done
--

end product;
'''
    # type, major, minor, level, edit must be the same as in pythlib.pcsi$text
    kit_content = kit_template.format(
        type=type,
        major=major,
        minor=minor,
        level=level,
        edit=edit,
        dirs='\n    '.join(all_dirs),
        files='\n    '.join(all_files))
    with open('python_x86n.pcsi$desc', 'w') as file:
        file.write(kit_content)

    text_template = '''=product VSI X86VMS PYTHON {type}{major}.{minor}-{level}{edit} full
1 'PRODUCT
=prompt Python for OpenVMS is based on Python Version 3.10

1 'PRODUCER
=prompt VMS Software Inc.

1 'NOTICE
=prompt (C) Copyright 2022 VMS Software Inc.

1 NO_MIN_VMS
=prompt Minimum OpenVMS software version not found on this system, abort instalation
This kit requires a minimum of OpenVMS I64 V8.4.

1 NO_ODS5_DISKS
=prompt ODS-5 disk(s) not found on this system, abort installation
This kit requires an ODS-5 disk to be correctly installed in this system.

1 POST_INSTALL
=prompt Post-installation tasks are required.
To define the Python runtime at system boot time, add the
following lines to SYS$MANAGER:SYSTARTUP_VMS.COM:

    $ file := SYS$STARTUP:PYTHON$STARTUP.COM
    $ if f$search("''file'") .nes. "" then @'file'

'''
    text_content = text_template.format(
        type=type,
        major=major,
        minor=minor,
        level=level,
        edit=edit,
        dirs='\n    '.join(all_dirs),
        files='\n    '.join(all_files))
    with open('python_x86n.pcsi$text', 'w') as file:
        file.write(text_content)

if __name__ == "__main__":

    import getopt
    import datetime

    opts, args = getopt.getopt(sys.argv[1:], '', ['type=', 'major=', 'minor=', 'level=', 'edit='])

    type = 'A'
    major = '3'
    minor = '10'
    level = '0'
    edit = 'release007'

    for opt, optarg in opts:
        if opt in ['--type']:
            type = optarg
        elif opt in ['--major']:
            major = optarg
        elif opt in ['--minor']:
            minor = optarg
        elif opt in ['--level']:
            level = optarg
        elif opt in ['--edit']:
            edit = optarg
        else:
            print('Unknown option %s' % opt)

    create_content(
        type,
        major,
        minor,
        level,
        edit,
    )
