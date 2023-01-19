! build 32 bit version
! MMS/EXT/DESCR=Python3.mms/MACRO=("OUTDIR=OUT","CONFIG=RELEASE")
! build 64 bit version
! MMS/EXT/DESCR=Python3.mms/MACRO=("OUTDIR=OUT","CONFIG=RELEASE64","P64=1")
! build with native compiler
! MMS/EXT/DESCR=Python3.mms/MACRO=("OUTDIR=OUT","CONFIG=RELEASE_X86_64","X86_64=1")
! build with cross compiler
! MMS/EXT/DESCR=Python3.mms/MACRO=("OUTDIR=OUT","CONFIG=RELEASEx86","X86_HOST=BALDER","X86_DISK=$172$DKA300","X86_USER=<user>","X86_PASS=<pass>","X86_FFIDEF=define libffi$root wrk_disk:[vorfolomeev.libffi.]","X86_SSLDEF=define ssl$shared wrk_disk:[vorfolomeev.ssl111_vms13.]")

DYNLOAD_DIR = lib-dynload
PLATFORM = OpenVMS

.IF X86_64
LINK_ADD=/SEGMENT=CODE=P0
X86_64_START=@sys$login:setup_compilers
.ENDIF

.IF X86_HOST .OR X86_64
SOABI = cpython-310-x86_64-openvms
.ELSE
SOABI = cpython-310-ia64-openvms
.ENDIF

CC_QUALIFIERS = -
/FLOAT=IEEE_FLOAT/IEEE=DENORM-
/NAMES=(AS_IS,SHORTENED)-
/ACCEPT=NOVAXC_KEYWORDS-
/REENTRANCY=MULTITHREAD

.IF X86_HOST
CC_QUALIFIERS = $(CC_QUALIFIERS)-
/WARNINGS=(WARNINGS=ALL, DISABLE=(EXTRASEMI,INCLUDEINFO,MAYLOSEDATA3))
.ELSIF X86_64
CC_QUALIFIERS = $(CC_QUALIFIERS)-
/WARNINGS=(WARNINGS=ALL, DISABLE=(EXTRASEMI,MAYLOSEDATA3,UNDERFLOW))
.ELSIF P64
CC_QUALIFIERS = $(CC_QUALIFIERS)-
/WARNINGS=(WARNINGS=ALL, DISABLE=(EXTRASEMI,MAYLOSEDATA3,MAYLOSEDATA2))
.ELSE
CC_QUALIFIERS = $(CC_QUALIFIERS)-
/WARNINGS=(WARNINGS=ALL, DISABLE=(EXTRASEMI,MAYLOSEDATA3))
.ENDIF

.IF P64
CC_QUALIFIERS = $(CC_QUALIFIERS) -
/POINTER_SIZE=64  ! check 'SIZEOF_VOID_P': 8, in _sysconfigdata__OpenVMS_cpython-310-ia64-openvms.py
LIBBZ2 = oss$root:[lib]libbz2_64.olb
LIBFFI = oss$root:[lib]libffi64.olb
LIBGDBM = wrk_disk:[vorfolomeev.gdbm.vms]libgdbm64.olb
LIBLZMA = oss$root:[lib]liblzma64.olb
LIBSQLITE = oss$root:[lib]libsqlite64.olb
LIBZ = oss$root:[lib]libz64.olb
LIBREADLINE = oss$root:[lib]libreadline64.olb
OPT_SUFFIX = _64
.ELSE
LIBBZ2 = oss$root:[lib]libbz2_32.olb
LIBFFI = oss$root:[lib]libffi32.olb
LIBGDBM = wrk_disk:[vorfolomeev.gdbm.vms]libgdbm32.olb
LIBLZMA = oss$root:[lib]liblzma32.olb
LIBSQLITE = oss$root:[lib]libsqlite32.olb
LIBZ = oss$root:[lib]libz32.olb
LIBREADLINE = oss$root:[lib]libreadline32.olb
OPT_SUFFIX = _32
.ENDIF

.IF X86_HOST
LIBGDBM = oss$root:[lib]libgdbm32.olb
LIBFFI = libffi$root:[lib]libffi$shr.olb
OPT_SUFFIX = _x86
.ELSIF X86_64
LIBGDBM = oss$root:[lib]libgdbm32.olb
LIBFFI = libffi$root:[out.RELEASE_X86_64]libffi$shr.olb
.ENDIF

CC_DEFINES = -
_USE_STD_STAT, -                ! COMMON
_POSIX_EXIT, -
__STDC_FORMAT_MACROS, -
_LARGEFILE, -
_SOCKADDR_LEN, -
__SIGNED_INT_TIME_T, -
USE_ZLIB_CRC32, -               ! BINASCII
_OSF_SOURCE, -                  ! OSF
HAVE_EXPAT_CONFIG_H, -          ! EXPAT
XML_POOR_ENTROPY, -
USE_PYEXPAT_CAPI, -
CONFIG_32, -                    ! DECIMAL
ANSI, -
SOABI="""$(SOABI)""", -         ! SOABI
SHLIB_EXT=""".EXE""", -
ABIFLAGS="""""", -
MULTIARCH="""$(SOABI)""", -
PLATFORM="""$(PLATFORM)""", -
USE_SSL                         ! SSL

.IF X86_64
CC_DEFINES=$(CC_DEFINES),__NATIVE_C__
.ENDIF

! define output folder
.IF OUTDIR
! defined - ok
.ELSE
! not defined - define as OUT
OUTDIR = OUT
.ENDIF

! check DEBUG or RELEASE
.IF CONFIG
! defined - ok
.ELSE
! not defined - define as DEBUG
CONFIG = DEBUG
.ENDIF

.IF $(FINDSTRING DEBUG, $(CONFIG)) .EQ DEBUG
! debug
.IF X86_64
CC_QUALIFIERS = $(CC_QUALIFIERS)/DEBUG/NOOPTIMIZE/LIST=$(MMS$TARGET_NAME)/SHOW=EXP
.ELSE
CC_QUALIFIERS = $(CC_QUALIFIERS)/DEBUG/NOOPTIMIZE/LIST=$(MMS$TARGET_NAME)/SHOW=ALL
.ENDIF
CC_DEFINES = $(CC_DEFINES),_DEBUG
OUT_DIR = $(OUTDIR).$(CONFIG)
OBJ_DIR = $(OUT_DIR).OBJ
LINK_FLAGS = $(LINK_ADD)/NODEBUG/MAP=[.$(OUT_DIR)]$(NOTDIR $(MMS$TARGET_NAME))/TRACE/DSF=[.$(OUT_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).DSF
PYTHON$SHR_OPT = PYTHON$SHR_DBG
.ELSE
! release
CC_QUALIFIERS = $(CC_QUALIFIERS)/NODEBUG/OPTIMIZE/NOLIST
CC_DEFINES = $(CC_DEFINES),_NDEBUG
OUT_DIR = $(OUTDIR).$(CONFIG)
OBJ_DIR = $(OUT_DIR).OBJ
LINK_FLAGS = $(LINK_ADD)/NODEBUG/NOMAP/NOTRACEBACK
PYTHON$SHR_OPT = PYTHON$SHR
.ENDIF

GETPATH_DEFINES = $(CC_DEFINES),PYTHONPATH="""""",PREFIX="""/usr/local""",EXEC_PREFIX="""/usr/local""",VERSION="""3.10""",VPATH=""""""

CC_INCLUDES = -
[], -
[.Include], -
[.Include.internal], -
[.Modules.expat], -
[.Modules._decimal.libmpdec], -
[.Modules._multiprocessing], -
[.Modules._io], -
[.vms], -
oss$root:[include], -
dtr$library, -
SSL111$ROOT:[INCLUDE]

.IF X86_HOST .OR X86_64
CC_INCLUDES = $(CC_INCLUDES),-
libffi$root:[vms.x86], -
libffi$root:[include], -
libffi$root:[src.x86]
.ENDIF

CC_FLAGS = $(CC_QUALIFIERS)/DEFINE=($(CC_DEFINES))/INCLUDE_DIRECTORY=($(CC_INCLUDES))

CC_CORE_CFLAGS = $(CC_QUALIFIERS)/DEFINE=("Py_BUILD_CORE",$(CC_DEFINES))/INCLUDE_DIRECTORY=($(CC_INCLUDES))

CC_CORE_MODULE_CFLAGS = $(CC_QUALIFIERS)/DEFINE=("Py_BUILD_CORE_MODULE",$(CC_DEFINES))/INCLUDE_DIRECTORY=($(CC_INCLUDES))

CC_CORE_BUILTIN_CFLAGS = $(CC_QUALIFIERS)/DEFINE=("Py_BUILD_CORE_BUILTIN",$(CC_DEFINES))/INCLUDE_DIRECTORY=($(CC_INCLUDES))

CC_SQLITE3_MODULE_CFLAGS = $(CC_QUALIFIERS)/DEFINE=("Py_BUILD_CORE_MODULE",MODULE_NAME="""sqlite3""",$(CC_DEFINES))/INCLUDE_DIRECTORY=($(CC_INCLUDES))

CC_GETPATH_CFLAGS = $(CC_QUALIFIERS)/DEFINE=("Py_BUILD_CORE",$(GETPATH_DEFINES))/INCLUDE_DIRECTORY=($(CC_INCLUDES))

.IF X86_HOST
X86_START = @SYS$MANAGER:X86_XTOOLS$SYLOGIN
X86_LIBDEF = define sys$library X86$LIBRARY
X86_OSSDEF = define /trans=concealed oss$root DSA22:[OSS.X86.]
.ELSE
X86_START =
X86_LIBDEF =
X86_OSSDEF =
.ENDIF

.FIRST
    $(X86_64_START)
    $(X86_START)
    $(X86_LIBDEF)
    $(X86_OSSDEF)
    $(X86_FFIDEF)
    $(X86_SSLDEF)
    ! defines for nested includes, like:
    ! #include "clinic/transmogrify.h.h"
    define cpython [.Include.cpython]
    define stringlib [.Objects.stringlib]
    define clinic [.Objects.clinic],[.Python.clinic],[.Modules.clinic],[.Modules._io.clinic],[.Modules.cjkcodecs.clinic],[.Objects.stringlib.clinic],[.Modules._blake2.clinic],[.Modules._sha3.clinic],[.Modules._multiprocessing.clinic],[.Modules._sqlite.clinic],[.Modules._ssl.clinic]
    define _ssl [.Modules._ssl]
    define ctypes [.Modules._ctypes]
    define modules [.Modules]
    define readline oss$root:[include.readline]
    define lzma oss$root:[include.lzma]
    define internal [.Include.internal]
    define impl [.Modules._blake2.impl]
    define kcp  [.Modules._sha3.kcp]
    ! OPENSSL 111
    define openssl ssl111$include:
    ! SQL
    sqlmod :==  mcr sql$mod
    ! names
    BUILD_OUT_DIR = F$ENVIRONMENT("DEFAULT")-"]"+".$(OUT_DIR).]"
    BUILD_OBJ_DIR = F$ENVIRONMENT("DEFAULT")-"]"+".$(OBJ_DIR).]"
    define /trans=concealed python$build_out 'BUILD_OUT_DIR'
    define /trans=concealed python$build_obj 'BUILD_OBJ_DIR'
    ! lib 32 or 64
    define LIBBZ2 $(LIBBZ2)
    define LIBFFI $(LIBFFI)
    define LIBGDBM $(LIBGDBM)
    define LIBLZMA $(LIBLZMA)
    define LIBSQLITE $(LIBSQLITE)
    define LIBZ $(LIBZ)
    define LIBREADLINE $(LIBREADLINE)

.SUFFIXES
.SUFFIXES .EXE .OLB .OBS .OBM .OBB .OBC .C

! CORE
.C.OBC
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(CC) $(CC_CORE_CFLAGS) /OBJECT=$(MMS$TARGET) $(MMS$SOURCE)

.OBC.OLB
    @ IF F$SEARCH("$(MMS$TARGET)") .EQS. "" -
        THEN $(LIBR)/CREATE $(MMS$TARGET)
    $(LIBR) $(MMS$TARGET) $(MMS$SOURCE)

! BUILTIN MODULE
.C.OBB
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(CC) $(CC_CORE_BUILTIN_CFLAGS) /OBJECT=$(MMS$TARGET) $(MMS$SOURCE)

.OBB.OLB
    @ IF F$SEARCH("$(MMS$TARGET)") .EQS. "" -
        THEN $(LIBR)/CREATE $(MMS$TARGET)
    $(LIBR) $(MMS$TARGET) $(MMS$SOURCE)

! SHARED MODULE
.C.OBM
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(CC) $(CC_CORE_MODULE_CFLAGS) /OBJECT=$(MMS$TARGET) $(MMS$SOURCE)

! SQLITE
.C.OBS
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(CC) $(CC_SQLITE3_MODULE_CFLAGS) /OBJECT=$(MMS$TARGET) $(MMS$SOURCE)

.OBM.EXE
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

##########################################################################
TARGET : [.$(OUT_DIR)]python.exe [.$(OUT_DIR)]_testembed.exe LIB_DYNLOAD
    ! TARGET BUILT

pyconfig.h : [.vms]pyconfig.h
    copy [.vms]pyconfig.h []

[.Include]stdint.h : [.vms]stdint.h
    copy [.vms]stdint.h [.Include]stdint.h

[.Include]inttypes.h : [.vms]inttypes.h
    copy [.vms]inttypes.h [.Include]inttypes.h

[.Modules]config.c : [.vms]config.c
    copy [.vms]config.c [.Modules]config.c

##########################################################################
# Modules
MODULE_OBJS= -
[.$(OBJ_DIR).Modules]config.obc -
[.$(OBJ_DIR).Modules]getpath.obc -
[.$(OBJ_DIR).Modules]main.obc -
[.$(OBJ_DIR).Modules]gcmodule.obc

IO_H= -
[.Modules._io]_iomodule.h

##########################################################################
# Parser

PEGEN_OBJS= -
[.$(OBJ_DIR).Parser]pegen.obc -
[.$(OBJ_DIR).Parser]parser.obc -
[.$(OBJ_DIR).Parser]string_parser.obc -
[.$(OBJ_DIR).Parser]peg_api.obc

PEGEN_HEADERS= -
[.Parser]pegen.h -
[.Parser]string_parser.h

POBJS= -
[.$(OBJ_DIR).Parser]token.obc

PARSER_OBJS= -
$(POBJS) -
$(PEGEN_OBJS) -
[.$(OBJ_DIR).Parser]myreadline.obc -
[.$(OBJ_DIR).Parser]tokenizer.obc -
[.$(OBJ_DIR).vms]vms_smg_readline.obc

PARSER_HEADERS= -
$(PEGEN_HEADERS) -
[.Parser]tokenizer.h

##########################################################################
# Python

PYTHON_OBJS= -
[.$(OBJ_DIR).Python]_warnings.obc -
[.$(OBJ_DIR).Python]Python-ast.obc -
[.$(OBJ_DIR).Python]asdl.obc -
[.$(OBJ_DIR).Python]ast.obc -
[.$(OBJ_DIR).Python]ast_opt.obc -
[.$(OBJ_DIR).Python]ast_unparse.obc -
[.$(OBJ_DIR).Python]bltinmodule.obc -
[.$(OBJ_DIR).Python]ceval.obc -
[.$(OBJ_DIR).Python]codecs.obc -
[.$(OBJ_DIR).Python]compile.obc -
[.$(OBJ_DIR).Python]context.obc -
[.$(OBJ_DIR).Python]dynamic_annotations.obc -
[.$(OBJ_DIR).Python]errors.obc -
[.$(OBJ_DIR).Python]frozenmain.obc -
[.$(OBJ_DIR).Python]future.obc -
[.$(OBJ_DIR).Python]getargs.obc -
[.$(OBJ_DIR).Python]getcompiler.obc -
[.$(OBJ_DIR).Python]getcopyright.obc -
[.$(OBJ_DIR).Python]getplatform.obc -
[.$(OBJ_DIR).Python]getversion.obc -
[.$(OBJ_DIR).Python]hamt.obc -
[.$(OBJ_DIR).Python]hashtable.obc -
[.$(OBJ_DIR).Python]import.obc -
[.$(OBJ_DIR).Python]importdl.obc -
[.$(OBJ_DIR).Python]initconfig.obc -
[.$(OBJ_DIR).Python]marshal.obc -
[.$(OBJ_DIR).Python]modsupport.obc -
[.$(OBJ_DIR).Python]mysnprintf.obc -
[.$(OBJ_DIR).Python]mystrtoul.obc -
[.$(OBJ_DIR).Python]pathconfig.obc -
[.$(OBJ_DIR).Python]preconfig.obc -
[.$(OBJ_DIR).Python]pyarena.obc -
[.$(OBJ_DIR).Python]pyctype.obc -
[.$(OBJ_DIR).Python]pyfpe.obc -
[.$(OBJ_DIR).Python]pyhash.obc -
[.$(OBJ_DIR).Python]pylifecycle.obc -
[.$(OBJ_DIR).Python]pymath.obc -
[.$(OBJ_DIR).Python]pystate.obc -
[.$(OBJ_DIR).Python]pythonrun.obc -
[.$(OBJ_DIR).Python]pytime.obc -
[.$(OBJ_DIR).Python]bootstrap_hash.obc -
[.$(OBJ_DIR).Python]structmember.obc -
[.$(OBJ_DIR).Python]symtable.obc -
[.$(OBJ_DIR).Python]sysmodule.obc -
[.$(OBJ_DIR).Python]thread.obc -
[.$(OBJ_DIR).Python]traceback.obc -
[.$(OBJ_DIR).Python]getopt.obc -
[.$(OBJ_DIR).Python]pystrcmp.obc -
[.$(OBJ_DIR).Python]pystrtod.obc -
[.$(OBJ_DIR).Python]pystrhex.obc -
[.$(OBJ_DIR).Python]dtoa.obc -
[.$(OBJ_DIR).Python]formatter_unicode.obc -
[.$(OBJ_DIR).Python]fileutils.obc -
[.$(OBJ_DIR).Python]suggestions.obc -
[.$(OBJ_DIR).Python]dynload_shlib.obc
! $(LIBOBJS) -
! $(MACHDEP_OBJS) -
! $(DTRACE_OBJS)

##########################################################################
# Objects
OBJECT_OBJS= -
[.$(OBJ_DIR).Objects]abstract.obc -
[.$(OBJ_DIR).Objects]accu.obc -
[.$(OBJ_DIR).Objects]boolobject.obc -
[.$(OBJ_DIR).Objects]bytes_methods.obc -
[.$(OBJ_DIR).Objects]bytearrayobject.obc -
[.$(OBJ_DIR).Objects]bytesobject.obc -
[.$(OBJ_DIR).Objects]call.obc -
[.$(OBJ_DIR).Objects]capsule.obc -
[.$(OBJ_DIR).Objects]cellobject.obc -
[.$(OBJ_DIR).Objects]classobject.obc -
[.$(OBJ_DIR).Objects]codeobject.obc -
[.$(OBJ_DIR).Objects]complexobject.obc -
[.$(OBJ_DIR).Objects]descrobject.obc -
[.$(OBJ_DIR).Objects]enumobject.obc -
[.$(OBJ_DIR).Objects]exceptions.obc -
[.$(OBJ_DIR).Objects]genericaliasobject.obc -
[.$(OBJ_DIR).Objects]genobject.obc -
[.$(OBJ_DIR).Objects]fileobject.obc -
[.$(OBJ_DIR).Objects]floatobject.obc -
[.$(OBJ_DIR).Objects]frameobject.obc -
[.$(OBJ_DIR).Objects]funcobject.obc -
[.$(OBJ_DIR).Objects]interpreteridobject.obc -
[.$(OBJ_DIR).Objects]iterobject.obc -
[.$(OBJ_DIR).Objects]listobject.obc -
[.$(OBJ_DIR).Objects]longobject.obc -
[.$(OBJ_DIR).Objects]dictobject.obc -
[.$(OBJ_DIR).Objects]odictobject.obc -
[.$(OBJ_DIR).Objects]memoryobject.obc -
[.$(OBJ_DIR).Objects]methodobject.obc -
[.$(OBJ_DIR).Objects]moduleobject.obc -
[.$(OBJ_DIR).Objects]namespaceobject.obc -
[.$(OBJ_DIR).Objects]object.obc -
[.$(OBJ_DIR).Objects]obmalloc.obc -
[.$(OBJ_DIR).Objects]picklebufobject.obc -
[.$(OBJ_DIR).Objects]rangeobject.obc -
[.$(OBJ_DIR).Objects]setobject.obc -
[.$(OBJ_DIR).Objects]sliceobject.obc -
[.$(OBJ_DIR).Objects]structseq.obc -
[.$(OBJ_DIR).Objects]tupleobject.obc -
[.$(OBJ_DIR).Objects]typeobject.obc -
[.$(OBJ_DIR).Objects]unicodeobject.obc -
[.$(OBJ_DIR).Objects]unicodectype.obc -
[.$(OBJ_DIR).Objects]unionobject.obc -
[.$(OBJ_DIR).Objects]weakrefobject.obc

##########################################################################
# Static modules
MODOBJS= -
[.$(OBJ_DIR).Modules]posixmodule.obb -
[.$(OBJ_DIR).Modules]_functoolsmodule.obb -
[.$(OBJ_DIR).Modules]signalmodule.obb -
[.$(OBJ_DIR).Modules]timemodule.obb -
[.$(OBJ_DIR).Modules]_threadmodule.obb -
[.$(OBJ_DIR).Modules]_localemodule.obb -
[.$(OBJ_DIR).Modules._io]_iomodule.obb -
[.$(OBJ_DIR).Modules._io]iobase.obb -
[.$(OBJ_DIR).Modules._io]fileio.obb -
[.$(OBJ_DIR).Modules._io]bufferedio.obb -
[.$(OBJ_DIR).Modules._io]textio.obb -
[.$(OBJ_DIR).Modules._io]bytesio.obb -
[.$(OBJ_DIR).Modules._io]stringio.obb -
[.$(OBJ_DIR).Modules]errnomodule.obc -
[.$(OBJ_DIR).Modules]pwdmodule.obc -
[.$(OBJ_DIR).Modules]_sre.obc -
[.$(OBJ_DIR).Modules]_codecsmodule.obc -
[.$(OBJ_DIR).Modules]_weakref.obc -
[.$(OBJ_DIR).Modules]_operator.obc -
[.$(OBJ_DIR).Modules]_collectionsmodule.obc -
[.$(OBJ_DIR).Modules]_abc.obc -
[.$(OBJ_DIR).Modules]itertoolsmodule.obc -
[.$(OBJ_DIR).Modules]atexitmodule.obc -
[.$(OBJ_DIR).Modules]_stat.obc -
[.$(OBJ_DIR).Modules]faulthandler.obc -
[.$(OBJ_DIR).Modules]_tracemalloc.obc -
[.$(OBJ_DIR).Modules]symtablemodule.obc

##########################################################################
# objects that get linked into the Python library
LIBRARY_OBJS_OMIT_FROZEN= -
[.$(OBJ_DIR).Modules]getbuildinfo.obc -
[.$(OBJ_DIR).vms]vms_crtl_values.obc -
[.$(OBJ_DIR).vms]vms_select.obc -
[.$(OBJ_DIR).vms]vms_spawn_helper.obc -
[.$(OBJ_DIR).vms]vms_sleep.obc -
[.$(OBJ_DIR).vms]vms_mbx_util.obc -
[.$(OBJ_DIR).vms]vms_fcntl.obc -
$(PARSER_OBJS) -
$(OBJECT_OBJS) -
$(PYTHON_OBJS) -
$(MODULE_OBJS) -
$(MODOBJS)

LIBRARY_OBJS= -
$(LIBRARY_OBJS_OMIT_FROZEN) -
[.$(OBJ_DIR).Python]frozen.obc

##########################################################################

BYTESTR_DEPS = -
[.Objects.stringlib]count.h -
[.Objects.stringlib]ctype.h -
[.Objects.stringlib]fastsearch.h -
[.Objects.stringlib]find.h -
[.Objects.stringlib]join.h -
[.Objects.stringlib]partition.h -
[.Objects.stringlib]split.h -
[.Objects.stringlib]stringdefs.h -
[.Objects.stringlib]transmogrify.h

UNICODE_DEPS = -
[.Objects.stringlib]asciilib.h -
[.Objects.stringlib]codecs.h -
[.Objects.stringlib]count.h -
[.Objects.stringlib]fastsearch.h -
[.Objects.stringlib]find.h -
[.Objects.stringlib]find_max_char.h -
[.Objects.stringlib]localeutil.h -
[.Objects.stringlib]partition.h -
[.Objects.stringlib]replace.h -
[.Objects.stringlib]split.h -
[.Objects.stringlib]ucs1lib.h -
[.Objects.stringlib]ucs2lib.h -
[.Objects.stringlib]ucs4lib.h -
[.Objects.stringlib]undef.h -
[.Objects.stringlib]unicode_format.h -
[.Objects.stringlib]unicodedefs.h

############################################################################
# Header files

PYTHON_HEADERS= -
[.Include]stdint.h -
[.Include]inttypes.h -
[.Include]Python.h -
[.Include]abstract.h -
[.Include]bltinmodule.h -
[.Include]boolobject.h -
[.Include]bytearrayobject.h -
[.Include]bytesobject.h -
[.Include]cellobject.h -
[.Include]ceval.h -
[.Include]classobject.h -
[.Include]code.h -
[.Include]codecs.h -
[.Include]compile.h -
[.Include]complexobject.h -
[.Include]context.h -
[.Include]descrobject.h -
[.Include]dictobject.h -
[.Include]dynamic_annotations.h -
[.Include]enumobject.h -
[.Include]errcode.h -
[.Include]eval.h -
[.Include]fileobject.h -
[.Include]fileutils.h -
[.Include]floatobject.h -
[.Include]frameobject.h -
[.Include]funcobject.h -
[.Include]genobject.h -
[.Include]import.h -
[.Include]interpreteridobject.h -
[.Include]intrcheck.h -
[.Include]iterobject.h -
[.Include]listobject.h -
[.Include]longintrepr.h -
[.Include]longobject.h -
[.Include]marshal.h -
[.Include]memoryobject.h -
[.Include]methodobject.h -
[.Include]modsupport.h -
[.Include]moduleobject.h -
[.Include]namespaceobject.h -
[.Include]object.h -
[.Include]objimpl.h -
[.Include]opcode.h -
[.Include]osdefs.h -
[.Include]osmodule.h -
[.Include]patchlevel.h -
[.Include]pycapsule.h -
[.Include]pydtrace.h -
[.Include]pyerrors.h -
[.Include]pyframe.h -
[.Include]pyhash.h -
[.Include]pylifecycle.h -
[.Include]pymacconfig.h -
[.Include]pymacro.h -
[.Include]pymath.h -
[.Include]pymem.h -
[.Include]pyport.h -
[.Include]pystate.h -
[.Include]pystrcmp.h -
[.Include]pystrhex.h -
[.Include]pystrtod.h -
[.Include]pythonrun.h -
[.Include]pythread.h -
[.Include]rangeobject.h -
[.Include]setobject.h -
[.Include]sliceobject.h -
[.Include]structmember.h -
[.Include]structseq.h -
[.Include]sysmodule.h -
[.Include]token.h -
[.Include]traceback.h -
[.Include]tracemalloc.h -
[.Include]tupleobject.h -
[.Include]unicodeobject.h -
[.Include]warnings.h -
[.Include]weakrefobject.h -
pyconfig.h -
$(PARSER_HEADERS) -
[.Include.cpython]abstract.h -
[.Include.cpython]bytearrayobject.h -
[.Include.cpython]bytesobject.h -
[.Include.cpython]ceval.h -
[.Include.cpython]code.h -
[.Include.cpython]dictobject.h -
[.Include.cpython]fileobject.h -
[.Include.cpython]fileutils.h -
[.Include.cpython]frameobject.h -
[.Include.cpython]import.h -
[.Include.cpython]initconfig.h -
[.Include.cpython]interpreteridobject.h -
[.Include.cpython]listobject.h -
[.Include.cpython]methodobject.h -
[.Include.cpython]object.h -
[.Include.cpython]objimpl.h -
[.Include.cpython]pyerrors.h -
[.Include.cpython]pylifecycle.h -
[.Include.cpython]pymem.h -
[.Include.cpython]pystate.h -
[.Include.cpython]pythonrun.h -
[.Include.cpython]sysmodule.h -
[.Include.cpython]traceback.h -
[.Include.cpython]tupleobject.h -
[.Include.cpython]unicodeobject.h -
[.Include.internal]pycore_abstract.h -
[.Include.internal]pycore_accu.h -
[.Include.internal]pycore_atomic.h -
[.Include.internal]pycore_atomic_funcs.h -
[.Include.internal]pycore_asdl.h -
[.Include.internal]pycore_ast.h -
[.Include.internal]pycore_ast_state.h -
[.Include.internal]pycore_bitutils.h -
[.Include.internal]pycore_bytes_methods.h -
[.Include.internal]pycore_call.h -
[.Include.internal]pycore_ceval.h -
[.Include.internal]pycore_code.h -
[.Include.internal]pycore_condvar.h -
[.Include.internal]pycore_context.h -
[.Include.internal]pycore_dtoa.h -
[.Include.internal]pycore_fileutils.h -
[.Include.internal]pycore_format.h -
[.Include.internal]pycore_getopt.h -
[.Include.internal]pycore_gil.h -
[.Include.internal]pycore_hamt.h -
[.Include.internal]pycore_hashtable.h -
[.Include.internal]pycore_import.h -
[.Include.internal]pycore_initconfig.h -
[.Include.internal]pycore_interp.h -
[.Include.internal]pycore_list.h -
[.Include.internal]pycore_long.h -
[.Include.internal]pycore_object.h -
[.Include.internal]pycore_pathconfig.h -
[.Include.internal]pycore_pyerrors.h -
[.Include.internal]pycore_pyhash.h -
[.Include.internal]pycore_pylifecycle.h -
[.Include.internal]pycore_pymem.h -
[.Include.internal]pycore_pystate.h -
[.Include.internal]pycore_runtime.h -
[.Include.internal]pycore_symtable.h -
[.Include.internal]pycore_sysmodule.h -
[.Include.internal]pycore_traceback.h -
[.Include.internal]pycore_tuple.h -
[.Include.internal]pycore_ucnhash.h -
[.Include.internal]pycore_unionobject.h -
[.Include.internal]pycore_warnings.h -
[.Python]stdlib_module_names.h
! $(DTRACE_HEADERS)

############################################################################
# Importlib

freeze_importlib : [.$(OUT_DIR).Programs]_freeze_importlib.exe
    ! continue

[.$(OUT_DIR).Programs]_freeze_importlib.exe : [.$(OBJ_DIR).Programs]_freeze_importlib.obc [.$(OBJ_DIR).vms]vms_crtl_init.obc $(LIBRARY_OBJS_OMIT_FROZEN)
  @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_ADD)/NODEBUG/NOMAP/EXECUTABLE=python$build_out:[Programs]$(NOTDIR $(MMS$TARGET_NAME)).exe [.$(OBJ_DIR).vms]vms_crtl_init.obc,$(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

[.$(OBJ_DIR).Programs]_freeze_importlib.obc : [.Programs]_freeze_importlib.c $(PYTHON_HEADERS)

.IF X86_HOST
.ELSE
[.Python]importlib_external.h : [.Lib.importlib]_bootstrap_external.py [.$(OUT_DIR).Programs]_freeze_importlib.exe
    mcr [.$(OUT_DIR).Programs]_freeze_importlib.exe importlib._bootstrap_external Lib/importlib/_bootstrap_external.py Python/importlib_external.h

[.Python]importlib.h : [.Lib.importlib]_bootstrap.py [.$(OUT_DIR).Programs]_freeze_importlib.exe
    mcr [.$(OUT_DIR).Programs]_freeze_importlib.exe importlib._bootstrap Lib/importlib/_bootstrap.py Python/importlib.h

[.Python]importlib_zipimport.h : [.Lib]zipimport.py [.$(OUT_DIR).Programs]_freeze_importlib.exe
    mcr [.$(OUT_DIR).Programs]_freeze_importlib.exe zipimport Lib/zipimport.py Python/importlib_zipimport.h
.ENDIF
############################################################################
# Static modules
[.$(OBJ_DIR).Modules]posixmodule.obb : [.Modules]posixmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_functoolsmodule.obb : [.Modules]_functoolsmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]signalmodule.obb : [.Modules]signalmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]timemodule.obb : [.Modules]timemodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_threadmodule.obb : [.Modules]_threadmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_localemodule.obb : [.Modules]_localemodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]_iomodule.obb : [.Modules._io]_iomodule.c $(IO_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]iobase.obb : [.Modules._io]iobase.c $(IO_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]fileio.obb : [.Modules._io]fileio.c $(IO_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]bufferedio.obb : [.Modules._io]bufferedio.c $(IO_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]textio.obb : [.Modules._io]textio.c $(IO_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]bytesio.obb : [.Modules._io]bytesio.c $(IO_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._io]stringio.obb : [.Modules._io]stringio.c $(IO_HEADERS) $(PYTHON_HEADERS)

[.$(OBJ_DIR).Modules]errnomodule.obc : [.Modules]errnomodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]pwdmodule.obc : [.Modules]pwdmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_sre.obc : [.Modules]_sre.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_codecsmodule.obc : [.Modules]_codecsmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_weakref.obc : [.Modules]_weakref.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_operator.obc : [.Modules]_operator.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_collectionsmodule.obc : [.Modules]_collectionsmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_abc.obc : [.Modules]_abc.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]itertoolsmodule.obc : [.Modules]itertoolsmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]atexitmodule.obc : [.Modules]atexitmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_stat.obc : [.Modules]_stat.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]faulthandler.obc : [.Modules]faulthandler.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]_tracemalloc.obc : [.Modules]_tracemalloc.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]symtablemodule.obc : [.Modules]symtablemodule.c $(PYTHON_HEADERS)

############################################################################
# Static code

[.$(OBJ_DIR).Parser]myreadline.obc : [.Parser]myreadline.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Modules]config.obc : [.Modules]config.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]getpath.obc : [.Modules]getpath.c $(PYTHON_HEADERS)
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(CC) $(CC_GETPATH_CFLAGS) /OBJECT=$(MMS$TARGET) $(MMS$SOURCE)


[.$(OBJ_DIR).Modules]main.obc : [.Modules]main.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]gcmodule.obc : [.Modules]gcmodule.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Parser]pegen.obc : [.Parser]pegen.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Parser]parser.obc : [.Parser]parser.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Parser]string_parser.obc : [.Parser]string_parser.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Parser]peg_api.obc : [.Parser]peg_api.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Parser]token.obc : [.Parser]token.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Parser]tokenizer.obc : [.Parser]tokenizer.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Python]_warnings.obc : [.Python]_warnings.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]Python-ast.obc : [.Python]Python-ast.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]asdl.obc : [.Python]asdl.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]ast.obc : [.Python]ast.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]ast_opt.obc : [.Python]ast_opt.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]ast_unparse.obc : [.Python]ast_unparse.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]bltinmodule.obc : [.Python]bltinmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]ceval.obc : [.Python]ceval.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]codecs.obc : [.Python]codecs.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]compile.obc : [.Python]compile.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]context.obc : [.Python]context.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]dynamic_annotations.obc : [.Python]dynamic_annotations.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]errors.obc : [.Python]errors.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]frozenmain.obc : [.Python]frozenmain.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]future.obc : [.Python]future.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]getargs.obc : [.Python]getargs.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]getcompiler.obc : [.Python]getcompiler.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]getcopyright.obc : [.Python]getcopyright.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]getplatform.obc : [.Python]getplatform.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]getversion.obc : [.Python]getversion.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]hamt.obc : [.Python]hamt.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]hashtable.obc : [.Python]hashtable.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]import.obc : [.Python]import.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]importdl.obc : [.Python]importdl.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]initconfig.obc : [.Python]initconfig.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]marshal.obc : [.Python]marshal.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]modsupport.obc : [.Python]modsupport.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]mysnprintf.obc : [.Python]mysnprintf.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]mystrtoul.obc : [.Python]mystrtoul.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pathconfig.obc : [.Python]pathconfig.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]preconfig.obc : [.Python]preconfig.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pyarena.obc : [.Python]pyarena.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pyctype.obc : [.Python]pyctype.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pyfpe.obc : [.Python]pyfpe.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pyhash.obc : [.Python]pyhash.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pylifecycle.obc : [.Python]pylifecycle.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pymath.obc : [.Python]pymath.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pystate.obc : [.Python]pystate.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pythonrun.obc : [.Python]pythonrun.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pytime.obc : [.Python]pytime.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]bootstrap_hash.obc : [.Python]bootstrap_hash.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]structmember.obc : [.Python]structmember.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]symtable.obc : [.Python]symtable.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]sysmodule.obc : [.Python]sysmodule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]thread.obc : [.Python]thread.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]traceback.obc : [.Python]traceback.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]getopt.obc : [.Python]getopt.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pystrcmp.obc : [.Python]pystrcmp.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pystrtod.obc : [.Python]pystrtod.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]pystrhex.obc : [.Python]pystrhex.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]dtoa.obc : [.Python]dtoa.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]formatter_unicode.obc : [.Python]formatter_unicode.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]fileutils.obc : [.Python]fileutils.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]suggestions.obc : [.Python]suggestions.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Python]dynload_shlib.obc : [.Python]dynload_shlib.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Objects]abstract.obc : [.Objects]abstract.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]accu.obc : [.Objects]accu.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]boolobject.obc : [.Objects]boolobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]bytes_methods.obc : [.Objects]bytes_methods.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]bytearrayobject.obc : [.Objects]bytearrayobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]bytesobject.obc : [.Objects]bytesobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]call.obc : [.Objects]call.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]capsule.obc : [.Objects]capsule.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]cellobject.obc : [.Objects]cellobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]classobject.obc : [.Objects]classobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]codeobject.obc : [.Objects]codeobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]complexobject.obc : [.Objects]complexobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]descrobject.obc : [.Objects]descrobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]enumobject.obc : [.Objects]enumobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]exceptions.obc : [.Objects]exceptions.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]genericaliasobject.obc : [.Objects]genericaliasobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]genobject.obc : [.Objects]genobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]fileobject.obc : [.Objects]fileobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]floatobject.obc : [.Objects]floatobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]frameobject.obc : [.Objects]frameobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]funcobject.obc : [.Objects]funcobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]interpreteridobject.obc : [.Objects]interpreteridobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]iterobject.obc : [.Objects]iterobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]listobject.obc : [.Objects]listobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]longobject.obc : [.Objects]longobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]dictobject.obc : [.Objects]dictobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]odictobject.obc : [.Objects]odictobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]memoryobject.obc : [.Objects]memoryobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]methodobject.obc : [.Objects]methodobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]moduleobject.obc : [.Objects]moduleobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]namespaceobject.obc : [.Objects]namespaceobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]object.obc : [.Objects]object.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]obmalloc.obc : [.Objects]obmalloc.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]picklebufobject.obc : [.Objects]picklebufobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]rangeobject.obc : [.Objects]rangeobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]setobject.obc : [.Objects]setobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]sliceobject.obc : [.Objects]sliceobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]structseq.obc : [.Objects]structseq.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]tupleobject.obc : [.Objects]tupleobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]typeobject.obc : [.Objects]typeobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]unicodeobject.obc : [.Objects]unicodeobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]unicodectype.obc : [.Objects]unicodectype.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]unionobject.obc : [.Objects]unionobject.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Objects]weakrefobject.obc : [.Objects]weakrefobject.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).Modules]getbuildinfo.obc : [.Modules]getbuildinfo.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).vms]vms_smg_readline.obc : [.vms]vms_smg_readline.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).vms]vms_crtl_values.obc : [.vms]vms_crtl_values.c
[.$(OBJ_DIR).vms]vms_select.obc : [.vms]vms_select.c [.vms]vms_select.h [.vms]vms_spawn_helper.h [.vms]vms_sleep.h
[.$(OBJ_DIR).vms]vms_spawn_helper.obc : [.vms]vms_spawn_helper.c [.vms]vms_spawn_helper.h
[.$(OBJ_DIR).vms]vms_sleep.obc : [.vms]vms_sleep.c [.vms]vms_sleep.h
[.$(OBJ_DIR).vms]vms_mbx_util.obc : [.vms]vms_mbx_util.c [.vms]vms_mbx_util.h
[.$(OBJ_DIR).vms]vms_fcntl.obc : [.vms]vms_fcntl.c [.vms]vms_fcntl.h [.vms]vms_mbx_util.h

.IF X86_HOST
[.$(OBJ_DIR).Python]frozen.obc : [.Python]frozen.c -
[.$(OUT_DIR).Programs]_freeze_importlib.exe -
[.Lib.importlib]_bootstrap_external.py -
[.Lib.importlib]_bootstrap.py -
[.Lib]zipimport.py -
[.vms]frzx86.com -
$(PYTHON_HEADERS)
    backup [.vms]frzx86.com sys$login: /repl
    - type $(X86_HOST)"$(X86_USER) $(X86_PASS)"::"task=frzx86"
    @- pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(CC) $(CC_CORE_CFLAGS) /OBJECT=$(MMS$TARGET) $(MMS$SOURCE)

.ELSE
[.$(OBJ_DIR).Python]frozen.obc : [.Python]frozen.c -
[.Python]importlib.h -
[.Python]importlib_external.h -
[.Python]importlib_zipimport.h -
$(PYTHON_HEADERS)
.ENDIF
############################################################################
# Library
[.$(OUT_DIR)]LIBPYTHON.OLB : [.$(OUT_DIR)]LIBPYTHON.OLB($(LIBRARY_OBJS))
    continue

############################################################################
# Shared library
[.$(OUT_DIR)]python$shr.exe : [.$(OUT_DIR)]LIBPYTHON.OLB
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[000000]$(NOTDIR $(MMS$TARGET_NAME)).exe [.vms.opt]$(PYTHON$SHR_OPT).opt/OPT

############################################################################
# Dynamic modules

LIBDYNLOAD_VMS = -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_accdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_acldef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_acrdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_armdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_brkdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_capdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_chpdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ciadef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_clidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_cmbdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_cvtfnmdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dcdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dmtdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dpsdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dscdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dvidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dvsdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_efndef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_eradef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fabdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fdldef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fpdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fscndef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_iccdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_iledef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_impdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_initdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_iodef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_issdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_jbcmsgdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_jpidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_kgbdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lckdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_libclidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_libdtdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_libfisdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lkidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lnmdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_maildef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_mntdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_nsadef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ossdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pcbdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ppropdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pqldef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prcdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prvdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prxdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pscandef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_psldef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pxbdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_quidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rabdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_regdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rmidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rmsdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rsdmdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sdvdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sjcdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ssdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_statedef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_stenvdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_stsdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_syidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_uafdef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_uaidef.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rms.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_decc.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lib.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ile3.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sys.exe
! [.$(OUT_DIR).$(DYNLOAD_DIR)]_rec.exe

.IFDEF BUILD_RDB
LIBDYNLOAD_VMS = $(LIBDYNLOAD_VMS) -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rdb.exe
.ENDIF

.IFDEF BUILD_DTR
LIBDYNLOAD_VMS = $(LIBDYNLOAD_VMS) -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dtr.exe
.ENDIF

LIBDYNLOAD = -
$(LIBDYNLOAD_VMS) -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_asyncio.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_bisect.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_blake2.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_bz2.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_cn.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_hk.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_iso2022.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_jp.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_kr.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_tw.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_contextvars.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_crypt.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_csv.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ctypes.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ctypes_test.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_datetime.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_decimal.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_elementtree.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_gdbm.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_hashlib.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_heapq.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_json.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lsprof.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lzma.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_md5.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_multibytecodec.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_multiprocessing.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_opcode.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pickle.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_posixshmem.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_posixsubprocess.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_queue.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_random.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha1.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha256.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha3.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha512.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_socket.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sqlite3.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ssl.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_statistics.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_struct.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testbuffer.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testcapi.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testimportmultiple.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testinternalcapi.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testmultiphase.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_xxsubinterpreters.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_xxtestfuzz.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]_zoneinfo.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]array.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]audioop.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]binascii.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]cmath.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]fcntl.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]grp.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]math.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]mmap.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]pyexpat.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]readline.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]select.exe -
- ! [.$(OUT_DIR).$(DYNLOAD_DIR)]spwd.exe
[.$(OUT_DIR).$(DYNLOAD_DIR)]syslog.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]unicodedata.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]zlib.exe -
[.$(OUT_DIR).$(DYNLOAD_DIR)]xxsubtype.exe

LIB_DYNLOAD : $(LIBDYNLOAD)
    continue

#array arraymodule.c	# array objects
[.$(OBJ_DIR).Modules]arraymodule.obm : [.Modules]arraymodule.c
[.$(OUT_DIR).$(DYNLOAD_DIR)]array.exe : [.$(OBJ_DIR).Modules]arraymodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#cmath cmathmodule.c _math.c -DPy_BUILD_CORE_MODULE # -lm # complex math library functions
[.$(OBJ_DIR).Modules]_math.obm : [.Modules]_math.c [.Modules]_math.h $(PYTHON_HEADERS)

[.$(OBJ_DIR).Modules]cmathmodule.obm : [.Modules]cmathmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]cmath.exe : [.$(OBJ_DIR).Modules]cmathmodule.obm [.$(OBJ_DIR).Modules]_math.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#math mathmodule.c _math.c -DPy_BUILD_CORE_MODULE # -lm # math library functions, e.g. sin()
[.$(OBJ_DIR).Modules]mathmodule.obm : [.Modules]mathmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]math.exe : [.$(OBJ_DIR).Modules]mathmodule.obm [.$(OBJ_DIR).Modules]_math.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_contextvars _contextvarsmodule.c  # Context Variables
[.$(OBJ_DIR).Modules]_contextvarsmodule.obm : [.Modules]_contextvarsmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_contextvars.exe : [.$(OBJ_DIR).Modules]_contextvarsmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_struct _struct.c	# binary structure packing/unpacking
[.$(OBJ_DIR).Modules]_struct.obm : [.Modules]_struct.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_struct.exe : [.$(OBJ_DIR).Modules]_struct.obm

#_testcapi _testcapimodule.c    # Python C API test module
[.$(OBJ_DIR).Modules]_testcapimodule.obm : [.Modules]_testcapimodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testcapi.exe : [.$(OBJ_DIR).Modules]_testcapimodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_testinternalcapi _testinternalcapi.c -I$(srcdir)/Include/internal -DPy_BUILD_CORE_MODULE  # Python internal C API test module
[.$(OBJ_DIR).Modules]_testinternalcapi.obm : [.Modules]_testinternalcapi.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testinternalcapi.exe : [.$(OBJ_DIR).Modules]_testinternalcapi.obm

#_random _randommodule.c -DPy_BUILD_CORE_MODULE	# Random number generator
[.$(OBJ_DIR).Modules]_randommodule.obm : [.Modules]_randommodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_random.exe : [.$(OBJ_DIR).Modules]_randommodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_elementtree -I$(srcdir)/Modules/expat -DHAVE_EXPAT_CONFIG_H -DUSE_PYEXPAT_CAPI _elementtree.c	# elementtree accelerator
[.$(OBJ_DIR).Modules]_elementtree.obm : [.Modules]_elementtree.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_elementtree.exe : [.$(OBJ_DIR).Modules]_elementtree.obm

#_pickle _pickle.c	# pickle accelerator
[.$(OBJ_DIR).Modules]_pickle.obm : [.Modules]_pickle.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pickle.exe : [.$(OBJ_DIR).Modules]_pickle.obm

#_datetime _datetimemodule.c	# datetime accelerator
[.$(OBJ_DIR).Modules]_datetimemodule.obm : [.Modules]_datetimemodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_datetime.exe : [.$(OBJ_DIR).Modules]_datetimemodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_zoneinfo _zoneinfo.c -DPy_BUILD_CORE_MODULE	# zoneinfo accelerator
[.$(OBJ_DIR).Modules]_zoneinfo.obm : [.Modules]_zoneinfo.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_zoneinfo.exe : [.$(OBJ_DIR).Modules]_zoneinfo.obm

#_bisect _bisectmodule.c	# Bisection algorithms
[.$(OBJ_DIR).Modules]_bisectmodule.obm : [.Modules]_bisectmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_bisect.exe : [.$(OBJ_DIR).Modules]_bisectmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_heapq _heapqmodule.c -DPy_BUILD_CORE_MODULE	# Heap queue algorithm
[.$(OBJ_DIR).Modules]_heapqmodule.obm : [.Modules]_heapqmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_heapq.exe : [.$(OBJ_DIR).Modules]_heapqmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_asyncio _asynciomodule.c  # Fast asyncio Future
[.$(OBJ_DIR).Modules]_asynciomodule.obm : [.Modules]_asynciomodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_asyncio.exe : [.$(OBJ_DIR).Modules]_asynciomodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_json -I$(srcdir)/Include/internal -DPy_BUILD_CORE_BUILTIN _json.c	# _json speedups
[.$(OBJ_DIR).Modules]_json.obm : [.Modules]_json.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_json.exe : [.$(OBJ_DIR).Modules]_json.obm

#_statistics _statisticsmodule.c # statistics accelerator
[.$(OBJ_DIR).Modules]_statisticsmodule.obm : [.Modules]_statisticsmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_statistics.exe : [.$(OBJ_DIR).Modules]_statisticsmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#unicodedata unicodedata.c -DPy_BUILD_CORE_BUILTIN   # static Unicode character database
[.$(OBJ_DIR).Modules]unicodedata.obm : [.Modules]unicodedata.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]unicodedata.exe : [.$(OBJ_DIR).Modules]unicodedata.obm


# Modules with some UNIX dependencies -- on by default:
# (If you have a really backward UNIX, select and socket may not be
# supported...)

#fcntl fcntlmodule.c	# fcntl(2) and ioctl(2)
[.$(OBJ_DIR).Modules]fcntlmodule.obm : [.Modules]fcntlmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]fcntl.exe : [.$(OBJ_DIR).Modules]fcntlmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#spwd spwdmodule.c		# spwd(3)
! [.$(OBJ_DIR).Modules]spwdmodule.obm : [.Modules]spwdmodule.c $(PYTHON_HEADERS)
! [.$(OUT_DIR).$(DYNLOAD_DIR)]spwd.exe : [.$(OBJ_DIR).Modules]spwdmodule.obm
!     @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
!     $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#grp grpmodule.c		# grp(3)
[.$(OBJ_DIR).Modules]grpmodule.obm : [.Modules]grpmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]grp.exe : [.$(OBJ_DIR).Modules]grpmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#select selectmodule.c	# select(2); not on ancient System V
[.$(OBJ_DIR).Modules]selectmodule.obm : [.Modules]selectmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]select.exe : [.$(OBJ_DIR).Modules]selectmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Memory-mapped files (also works on Win32).
#mmap mmapmodule.c
[.$(OBJ_DIR).Modules]mmapmodule.obm : [.Modules]mmapmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]mmap.exe : [.$(OBJ_DIR).Modules]mmapmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# CSV file helper
#_csv _csv.c
[.$(OBJ_DIR).Modules]_csv.obm : [.Modules]_csv.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_csv.exe : [.$(OBJ_DIR).Modules]_csv.obm

# Socket module helper for socket(2)
#_socket socketmodule.c
[.$(OBJ_DIR).Modules]socketmodule.obm : [.Modules]socketmodule.c [.Modules]socketmodule.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_socket.exe : [.$(OBJ_DIR).Modules]socketmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Socket module helper for SSL support; you must comment out the other
# socket line above, and possibly edit the SSL variable:
#SSL=/usr/local/ssl
#_ssl _ssl.c \
#	-DUSE_SSL -I$(SSL)/include -I$(SSL)/include/openssl \
#	-L$(SSL)/lib -lssl -lcrypto
[.$(OBJ_DIR).Modules]_ssl.obm : [.Modules]_ssl.c [.Modules]socketmodule.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ssl.exe : [.$(OBJ_DIR).Modules]_ssl.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME))$(OPT_SUFFIX).opt/OPT

# The crypt module is now disabled by default because it breaks builds
# on many systems (where -lcrypt is needed), e.g. Linux (I believe).

#_crypt _cryptmodule.c # -lcrypt	# crypt(3); needs -lcrypt on some systems
[.$(OBJ_DIR).Modules]_cryptmodule.obm : [.Modules]_cryptmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_crypt.exe : [.$(OBJ_DIR).Modules]_cryptmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Some more UNIX dependent modules -- off by default, since these
# are not supported by all UNIX systems:

#nis nismodule.c -lnsl	# Sun yellow pages -- not everywhere
#termios termios.c	# Steen Lumholt's termios module
#resource resource.c	# Jeremy Hylton's rlimit interface

#_posixsubprocess  -DPy_BUILD_CORE_BUILTIN _posixsubprocess.c  # POSIX subprocess module helper
[.$(OBJ_DIR).Modules]_posixsubprocess.obm : [.Modules]_posixsubprocess.c [.vms]vms_spawn_helper.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_posixsubprocess.exe : [.$(OBJ_DIR).Modules]_posixsubprocess.obm

# Multimedia modules -- off by default.
# These don't work for 64-bit platforms!!!
# #993173 says audioop works on 64-bit platforms, though.
# These represent audio samples or images as strings:

#audioop audioop.c	# Operations on audio samples
[.$(OBJ_DIR).Modules]audioop.obm : [.Modules]audioop.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]audioop.exe : [.$(OBJ_DIR).Modules]audioop.obm

# Note that the _md5 and _sha modules are normally only built if the
# system does not have the OpenSSL libs containing an optimized version.

# The _md5 module implements the RSA Data Security, Inc. MD5
# Message-Digest Algorithm, described in RFC 1321.

#_md5 md5module.c
[.$(OBJ_DIR).Modules]md5module.obm : [.Modules]md5module.c [.Modules]hashlib.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_md5.exe : [.$(OBJ_DIR).Modules]md5module.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# The _sha module implements the SHA checksum algorithms.
# (NIST's Secure Hash Algorithms.)
#_sha1 sha1module.c
[.$(OBJ_DIR).Modules]sha1module.obm : [.Modules]sha1module.c [.Modules]hashlib.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha1.exe : [.$(OBJ_DIR).Modules]sha1module.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_sha256 sha256module.c -DPy_BUILD_CORE_BUILTIN
[.$(OBJ_DIR).Modules]sha256module.obb : [.Modules]sha256module.c [.Modules]hashlib.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha256.exe : [.$(OBJ_DIR).Modules]sha256module.obb
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_sha512 sha512module.c -DPy_BUILD_CORE_BUILTIN
[.$(OBJ_DIR).Modules]sha512module.obb : [.Modules]sha512module.c [.Modules]hashlib.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha512.exe : [.$(OBJ_DIR).Modules]sha512module.obb
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_sha3 _sha3/sha3module.c
SHA3_HEADERS = -
[.Modules._sha3.kcp]align.h -
[.Modules._sha3.kcp]KeccakHash.c -
[.Modules._sha3.kcp]KeccakHash.h -
[.Modules._sha3.kcp]KeccakP-1600-64.macros -
[.Modules._sha3.kcp]KeccakP-1600-inplace32BI.c -
[.Modules._sha3.kcp]KeccakP-1600-opt64-config.h -
[.Modules._sha3.kcp]KeccakP-1600-opt64.c -
[.Modules._sha3.kcp]KeccakP-1600-SnP-opt32.h -
[.Modules._sha3.kcp]KeccakP-1600-SnP-opt64.h -
[.Modules._sha3.kcp]KeccakP-1600-SnP.h -
[.Modules._sha3.kcp]KeccakP-1600-unrolling.macros -
[.Modules._sha3.kcp]KeccakSponge.c -
[.Modules._sha3.kcp]KeccakSponge.h -
[.Modules._sha3.kcp]KeccakSponge.inc -
[.Modules._sha3.kcp]PlSnP-Fallback.inc -
[.Modules._sha3.kcp]SnP-Relaned.h -
[.Modules]hashlib.h

[.$(OBJ_DIR).Modules._sha3]sha3module.obm : [.Modules._sha3]sha3module.c $(SHA3_HEADERS) $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sha3.exe : [.$(OBJ_DIR).Modules._sha3]sha3module.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _blake module
#_blake2 _blake2/blake2module.c _blake2/blake2b_impl.c _blake2/blake2s_impl.c
BLAKE2_OBJ_LIST = -
[.$(OBJ_DIR).Modules._blake2]blake2module.obm -
[.$(OBJ_DIR).Modules._blake2]blake2b_impl.obm -
[.$(OBJ_DIR).Modules._blake2]blake2s_impl.obm

BLAKE2_HEADERS = -
[.Modules._blake2.impl]blake2-config.h -
[.Modules._blake2.impl]blake2-dispatch.c -
[.Modules._blake2.impl]blake2-impl.h -
[.Modules._blake2.impl]blake2-kat.h -
[.Modules._blake2.impl]blake2.h -
[.Modules._blake2.impl]blake2b-load-sse2.h -
[.Modules._blake2.impl]blake2b-load-sse41.h -
[.Modules._blake2.impl]blake2b-ref.c -
[.Modules._blake2.impl]blake2b-round.h -
[.Modules._blake2.impl]blake2b-test.c -
[.Modules._blake2.impl]blake2b.c -
[.Modules._blake2.impl]blake2bp-test.c -
[.Modules._blake2.impl]blake2bp.c -
[.Modules._blake2.impl]blake2s-load-sse2.h -
[.Modules._blake2.impl]blake2s-load-sse41.h -
[.Modules._blake2.impl]blake2s-load-xop.h -
[.Modules._blake2.impl]blake2s-ref.c -
[.Modules._blake2.impl]blake2s-round.h -
[.Modules._blake2.impl]blake2s-test.c -
[.Modules._blake2.impl]blake2s.c -
[.Modules._blake2.impl]blake2sp-test.c -
[.Modules._blake2.impl]blake2sp.c -
[.Modules]hashlib.h

[.$(OBJ_DIR).Modules._blake2]blake2module.obm : [.Modules._blake2]blake2module.c $(BLAKE2_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._blake2]blake2b_impl.obm : [.Modules._blake2]blake2b_impl.c $(BLAKE2_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._blake2]blake2s_impl.obm : [.Modules._blake2]blake2s_impl.c $(BLAKE2_HEADERS) $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_blake2.exe : $(BLAKE2_OBJ_LIST)
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# The _tkinter module.
#
# The command for _tkinter is long and site specific.  Please
# uncomment and/or edit those parts as indicated.  If you don't have a
# specific extension (e.g. Tix or BLT), leave the corresponding line
# commented out.  (Leave the trailing backslashes in!  If you
# experience strange errors, you may want to join all uncommented
# lines and remove the backslashes -- the backslash interpretation is
# done by the shell's "read" command and it may not be implemented on
# every system.

# *** Always uncomment this (leave the leading underscore in!):
# _tkinter _tkinter.c tkappinit.c -DWITH_APPINIT \
# *** Uncomment and edit to reflect where your Tcl/Tk libraries are:
#	-L/usr/local/lib \
# *** Uncomment and edit to reflect where your Tcl/Tk headers are:
#	-I/usr/local/include \
# *** Uncomment and edit to reflect where your X11 header files are:
#	-I/usr/X11R6/include \
# *** Or uncomment this for Solaris:
#	-I/usr/openwin/include \
# *** Uncomment and edit for Tix extension only:
#	-DWITH_TIX -ltix8.1.8.2 \
# *** Uncomment and edit for BLT extension only:
#	-DWITH_BLT -I/usr/local/blt/blt8.0-unoff/include -lBLT8.0 \
# *** Uncomment and edit for PIL (TkImaging) extension only:
#     (See http://www.pythonware.com/products/pil/ for more info)
#	-DWITH_PIL -I../Extensions/Imaging/libImaging  tkImaging.c \
# *** Uncomment and edit for TOGL extension only:
#	-DWITH_TOGL togl.c \
# *** Uncomment and edit to reflect your Tcl/Tk versions:
#	-ltk8.2 -ltcl8.2 \
# *** Uncomment and edit to reflect where your X11 libraries are:
#	-L/usr/X11R6/lib \
# *** Or uncomment this for Solaris:
#	-L/usr/openwin/lib \
# *** Uncomment these for TOGL extension only:
#	-lGL -lGLU -lXext -lXmu \
# *** Uncomment for AIX:
#	-lld \
# *** Always uncomment this; X11 libraries to link with:
#	-lX11

# Lance Ellinghaus's syslog module
#syslog syslogmodule.c		# syslog daemon interface
[.$(OBJ_DIR).vms]syslog.obm : [.vms]syslog.c [.vms]syslog.h
[.$(OBJ_DIR).Modules]syslogmodule.obm : [.Modules]syslogmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]syslog.exe : [.$(OBJ_DIR).vms]syslog.obm,[.$(OBJ_DIR).Modules]syslogmodule.obm

# Curses support, requiring the System V version of curses, often
# provided by the ncurses library.  e.g. on Linux, link with -lncurses
# instead of -lcurses).

#_curses _cursesmodule.c -lcurses -ltermcap -DPy_BUILD_CORE_MODULE
# Wrapper for the panel library that's part of ncurses and SYSV curses.
#_curses_panel _curses_panel.c -lpanel -lncurses


# Modules that provide persistent dictionary-like semantics.  You will
# probably want to arrange for at least one of them to be available on
# your machine, though none are defined by default because of library
# dependencies.  The Python module dbm/__init__.py provides an
# implementation independent wrapper for these; dbm/dumb.py provides
# similar functionality (but slower of course) implemented in Python.

#_dbm _dbmmodule.c 	# dbm(3) may require -lndbm or similar

# Anthony Baxter's gdbm module.  GNU dbm(3) will require -lgdbm:

#_gdbm _gdbmmodule.c -I/usr/local/include -L/usr/local/lib -lgdbm
[.$(OBJ_DIR).Modules]_gdbmmodule.obm : [.Modules]_gdbmmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_gdbm.exe : [.$(OBJ_DIR).Modules]_gdbmmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Helper module for various ascii-encoders
#binascii binascii.c
[.$(OBJ_DIR).Modules]binascii.obm : [.Modules]binascii.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]binascii.exe : [.$(OBJ_DIR).Modules]binascii.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Andrew Kuchling's zlib module.
# This require zlib 1.1.3 (or later).
# See http://www.gzip.org/zlib/
#zlib zlibmodule.c -I$(prefix)/include -L$(exec_prefix)/lib -lz
[.$(OBJ_DIR).Modules]zlibmodule.obm : [.Modules]zlibmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]zlib.exe : [.$(OBJ_DIR).Modules]zlibmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Interface to the Expat XML parser
# More information on Expat can be found at www.libexpat.org.
#
#pyexpat expat/xmlparse.c expat/xmlrole.c expat/xmltok.c pyexpat.c -I$(srcdir)/Modules/expat -DHAVE_EXPAT_CONFIG_H -DXML_POOR_ENTROPY -DUSE_PYEXPAT_CAPI
EXPAT_OBJ_LIST = -
[.$(OBJ_DIR).Modules.expat]xmlparse.obm -
[.$(OBJ_DIR).Modules.expat]xmlrole.obm -
[.$(OBJ_DIR).Modules.expat]xmltok.obm

EXPAT_HEADERS = -
[.Modules.expat]ascii.h -
[.Modules.expat]asciitab.h -
[.Modules.expat]expat.h -
[.Modules.expat]expat_config.h -
[.Modules.expat]expat_external.h -
[.Modules.expat]iasciitab.h -
[.Modules.expat]internal.h -
[.Modules.expat]latin1tab.h -
[.Modules.expat]nametab.h -
[.Modules.expat]pyexpatns.h -
[.Modules.expat]siphash.h -
[.Modules.expat]utf8tab.h -
[.Modules.expat]xmlrole.h -
[.Modules.expat]xmltok.h -
[.Modules.expat]xmltok_impl.h

[.$(OBJ_DIR).Modules]pyexpat.obm : [.Modules]pyexpat.c $(EXPAT_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules.expat]xmlparse.obm : [.Modules.expat]xmlparse.c $(EXPAT_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules.expat]xmlrole.obm : [.Modules.expat]xmlrole.c $(EXPAT_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules.expat]xmltok.obm : [.Modules.expat]xmltok.c $(EXPAT_HEADERS) $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]pyexpat.exe : [.$(OBJ_DIR).Modules]pyexpat.obm,$(EXPAT_OBJ_LIST)

# Hye-Shik Chang's CJKCodecs

# multibytecodec is required for all the other CJK codec modules
#_multibytecodec cjkcodecs/multibytecodec.c
[.$(OBJ_DIR).Modules.cjkcodecs]multibytecodec.obm : [.Modules.cjkcodecs]multibytecodec.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_multibytecodec.exe : [.$(OBJ_DIR).Modules.cjkcodecs]multibytecodec.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

#_codecs_cn cjkcodecs/_codecs_cn.c
[.$(OBJ_DIR).Modules.cjkcodecs]_codecs_cn.obm : [.Modules.cjkcodecs]_codecs_cn.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_cn.exe : [.$(OBJ_DIR).Modules.cjkcodecs]_codecs_cn.obm

#_codecs_hk cjkcodecs/_codecs_hk.c
[.$(OBJ_DIR).Modules.cjkcodecs]_codecs_hk.obm : [.Modules.cjkcodecs]_codecs_hk.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_hk.exe : [.$(OBJ_DIR).Modules.cjkcodecs]_codecs_hk.obm

#_codecs_iso2022 cjkcodecs/_codecs_iso2022.c
[.$(OBJ_DIR).Modules.cjkcodecs]_codecs_iso2022.obm : [.Modules.cjkcodecs]_codecs_iso2022.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_iso2022.exe : [.$(OBJ_DIR).Modules.cjkcodecs]_codecs_iso2022.obm

#_codecs_jp cjkcodecs/_codecs_jp.c
[.$(OBJ_DIR).Modules.cjkcodecs]_codecs_jp.obm : [.Modules.cjkcodecs]_codecs_jp.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_jp.exe : [.$(OBJ_DIR).Modules.cjkcodecs]_codecs_jp.obm

#_codecs_kr cjkcodecs/_codecs_kr.c
[.$(OBJ_DIR).Modules.cjkcodecs]_codecs_kr.obm : [.Modules.cjkcodecs]_codecs_kr.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_kr.exe : [.$(OBJ_DIR).Modules.cjkcodecs]_codecs_kr.obm

#_codecs_tw cjkcodecs/_codecs_tw.c
[.$(OBJ_DIR).Modules.cjkcodecs]_codecs_tw.obm : [.Modules.cjkcodecs]_codecs_tw.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_codecs_tw.exe : [.$(OBJ_DIR).Modules.cjkcodecs]_codecs_tw.obm

# sqlite3
SQL_OBJ_LIST = -
[.$(OBJ_DIR).Modules._sqlite]cache.obs -
[.$(OBJ_DIR).Modules._sqlite]connection.obs -
[.$(OBJ_DIR).Modules._sqlite]cursor.obs -
[.$(OBJ_DIR).Modules._sqlite]microprotocols.obs -
[.$(OBJ_DIR).Modules._sqlite]module.obs -
[.$(OBJ_DIR).Modules._sqlite]prepare_protocol.obs -
[.$(OBJ_DIR).Modules._sqlite]row.obs -
[.$(OBJ_DIR).Modules._sqlite]statement.obs -
[.$(OBJ_DIR).Modules._sqlite]util.obs

[.$(OBJ_DIR).Modules._sqlite]cache.obs : [.Modules._sqlite]cache.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]connection.obs : [.Modules._sqlite]connection.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]cursor.obs : [.Modules._sqlite]cursor.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]microprotocols.obs : [.Modules._sqlite]microprotocols.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]module.obs : [.Modules._sqlite]module.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]prepare_protocol.obs : [.Modules._sqlite]prepare_protocol.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]row.obs : [.Modules._sqlite]row.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]statement.obs : [.Modules._sqlite]statement.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._sqlite]util.obs : [.Modules._sqlite]util.c $(PYTHON_HEADERS)

[.$(OUT_DIR).$(DYNLOAD_DIR)]_sqlite3.exe : $(SQL_OBJ_LIST)
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# Example -- included for reference only:
# xx xxmodule.c

# Another example -- the 'xxsubtype' module shows C-level subtyping in action
# xxsubtype xxsubtype.c
[.$(OBJ_DIR).Modules]xxsubtype.obm : [.Modules]xxsubtype.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]xxsubtype.exe : [.$(OBJ_DIR).Modules]xxsubtype.obm

# Uncommenting the following line tells makesetup that all following modules
# are not built (see above for more detail).
#
#*disabled*
#
#_sqlite3 _tkinter _curses pyexpat
#_codecs_jp _codecs_kr _codecs_tw unicodedata

############################################################################
# from Python3.mms

# _bz2 _bz2module
[.$(OBJ_DIR).Modules]_bz2module.obm : [.Modules]_bz2module.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_bz2.exe : [.$(OBJ_DIR).Modules]_bz2module.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _ctypes
CTYPES_OBJ_LIST = -
[.$(OBJ_DIR).Modules._ctypes]callbacks.obm -
[.$(OBJ_DIR).Modules._ctypes]callproc.obm -
[.$(OBJ_DIR).Modules._ctypes]stgdict.obm -
[.$(OBJ_DIR).Modules._ctypes]cfield.obm

CTYPES_HEADERS = -
[.Modules._ctypes]ctypes.h

[.$(OBJ_DIR).Modules._ctypes]_ctypes.obm : [.Modules._ctypes]_ctypes.c $(CTYPES_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._ctypes]callbacks.obm : [.Modules._ctypes]callbacks.c $(CTYPES_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._ctypes]stgdict.obm : [.Modules._ctypes]stgdict.c $(CTYPES_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._ctypes]callproc.obm : [.Modules._ctypes]callproc.c $(CTYPES_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._ctypes]cfield.obm : [.Modules._ctypes]cfield.c $(CTYPES_HEADERS) $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ctypes.exe : [.$(OBJ_DIR).Modules._ctypes]_ctypes.obm,$(CTYPES_OBJ_LIST)

# _ctypes_test _ctypes_test
[.$(OBJ_DIR).Modules._ctypes]_ctypes_test.obm : [.Modules._ctypes]_ctypes_test.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ctypes_test.exe : [.$(OBJ_DIR).Modules._ctypes]_ctypes_test.obm

# _decimal
DECIMAL_OBJ_LIST = -
[.$(OBJ_DIR).Modules._decimal.libmpdec]basearith.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]constants.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]context.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]convolute.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]crt.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]difradix2.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]fnt.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]fourstep.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]io.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]mpalloc.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]mpdecimal.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]numbertheory.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]sixstep.obm -
[.$(OBJ_DIR).Modules._decimal.libmpdec]transpose.obm

DECIMAL_HEADERS = -
[.Modules._decimal]docstrings.h -
[.Modules._decimal.libmpdec]basearith.h -
[.Modules._decimal.libmpdec]bits.h -
[.Modules._decimal.libmpdec]constants.h -
[.Modules._decimal.libmpdec]convolute.h -
[.Modules._decimal.libmpdec]crt.h -
[.Modules._decimal.libmpdec]difradix2.h -
[.Modules._decimal.libmpdec]fnt.h -
[.Modules._decimal.libmpdec]fourstep.h -
[.Modules._decimal.libmpdec]io.h -
[.Modules._decimal.libmpdec]mpalloc.h -
[.Modules._decimal.libmpdec]mpdecimal.h -
[.Modules._decimal.libmpdec]numbertheory.h -
[.Modules._decimal.libmpdec]sixstep.h -
[.Modules._decimal.libmpdec]transpose.h -
[.Modules._decimal.libmpdec]typearith.h -
[.Modules._decimal.libmpdec]umodarith.h

[.$(OBJ_DIR).Modules._decimal.libmpdec]basearith.obm : [.Modules._decimal.libmpdec]basearith.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]constants.obm : [.Modules._decimal.libmpdec]constants.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]context.obm : [.Modules._decimal.libmpdec]context.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]convolute.obm : [.Modules._decimal.libmpdec]convolute.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]crt.obm : [.Modules._decimal.libmpdec]crt.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]difradix2.obm : [.Modules._decimal.libmpdec]difradix2.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]fnt.obm : [.Modules._decimal.libmpdec]fnt.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]fourstep.obm : [.Modules._decimal.libmpdec]fourstep.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]io.obm : [.Modules._decimal.libmpdec]io.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]mpalloc.obm : [.Modules._decimal.libmpdec]mpalloc.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]mpdecimal.obm : [.Modules._decimal.libmpdec]mpdecimal.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]numbertheory.obm : [.Modules._decimal.libmpdec]numbertheory.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]sixstep.obm : [.Modules._decimal.libmpdec]sixstep.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._decimal.libmpdec]transpose.obm : [.Modules._decimal.libmpdec]transpose.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)

[.$(OBJ_DIR).Modules._decimal]_decimal.obm : [.Modules._decimal]_decimal.c $(DECIMAL_HEADERS) $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_decimal.exe : [.$(OBJ_DIR).Modules._decimal]_decimal.obm,$(DECIMAL_OBJ_LIST)

# _hashlib _hashopenssl
[.$(OBJ_DIR).Modules]_hashopenssl.obm : [.Modules]_hashopenssl.c [.Modules]hashlib.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_hashlib.exe : [.$(OBJ_DIR).Modules]_hashopenssl.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME))$(OPT_SUFFIX).opt/OPT

# _lsprof _lsprof rotatingtree
[.$(OBJ_DIR).Modules]_lsprof.obm : [.Modules]_lsprof.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules]rotatingtree.obm : [.Modules]rotatingtree.c
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lsprof.exe : [.$(OBJ_DIR).Modules]_lsprof.obm,[.$(OBJ_DIR).Modules]rotatingtree.obm

# _lzma _lzmamodule
[.$(OBJ_DIR).Modules]_lzmamodule.obm : [.Modules]_lzmamodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lzma.exe : [.$(OBJ_DIR).Modules]_lzmamodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _multiprocessing
[.$(OBJ_DIR).Modules._multiprocessing]multiprocessing.obm : [.Modules._multiprocessing]multiprocessing.c [.Modules._multiprocessing]multiprocessing.h $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._multiprocessing]semaphore.obm : [.Modules._multiprocessing]semaphore.c [.Modules._multiprocessing]multiprocessing.h $(PYTHON_HEADERS)
[.$(OBJ_DIR).vms]vms_sem_mbx.obm : [.vms]vms_sem_mbx.c [.vms]vms_sem_mbx.h
[.$(OUT_DIR).$(DYNLOAD_DIR)]_multiprocessing.exe : [.$(OBJ_DIR).Modules._multiprocessing]multiprocessing.obm [.$(OBJ_DIR).vms]vms_sem_mbx.obm [.$(OBJ_DIR).Modules._multiprocessing]semaphore.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _opcode _opcode
[.$(OBJ_DIR).Modules]_opcode.obm : [.Modules]_opcode.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_opcode.exe : [.$(OBJ_DIR).Modules]_opcode.obm

# _posixshmem
[.$(OBJ_DIR).Modules._multiprocessing]posixshmem.obm : [.Modules._multiprocessing]posixshmem.c [.Modules._multiprocessing]multiprocessing.h $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_posixshmem.exe : [.$(OBJ_DIR).Modules._multiprocessing]posixshmem.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _queue _queuemodule
[.$(OBJ_DIR).Modules]_queuemodule.obm : [.Modules]_queuemodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_queue.exe : [.$(OBJ_DIR).Modules]_queuemodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _testbuffer _testbuffer
[.$(OBJ_DIR).Modules]_testbuffer.obm : [.Modules]_testbuffer.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testbuffer.exe : [.$(OBJ_DIR).Modules]_testbuffer.obm

# _testimportmultiple _testimportmultiple
[.$(OBJ_DIR).Modules]_testimportmultiple.obm : [.Modules]_testimportmultiple.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testimportmultiple.exe : [.$(OBJ_DIR).Modules]_testimportmultiple.obm

# _testmultiphase _testmultiphase
[.$(OBJ_DIR).Modules]_testmultiphase.obm : [.Modules]_testmultiphase.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_testmultiphase.exe : [.$(OBJ_DIR).Modules]_testmultiphase.obm

# _xxtestfuzz [_xxtestfuzz]_xxtestfuzz [_xxtestfuzz]fuzzer
[.$(OBJ_DIR).Modules._xxtestfuzz]_xxtestfuzz.obm : [.Modules._xxtestfuzz]_xxtestfuzz.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).Modules._xxtestfuzz]fuzzer.obm : [.Modules._xxtestfuzz]fuzzer.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_xxtestfuzz.exe : [.$(OBJ_DIR).Modules._xxtestfuzz]_xxtestfuzz.obm,[.$(OBJ_DIR).Modules._xxtestfuzz]fuzzer.obm

# readline readline
[.$(OBJ_DIR).Modules]readline.obm : [.Modules]readline.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]readline.exe : [.$(OBJ_DIR).Modules]readline.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    - $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE_LIST),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

# _xxsubinterpreters _xxsubinterpretersmodule
[.$(OBJ_DIR).Modules]_xxsubinterpretersmodule.obm : [.Modules]_xxsubinterpretersmodule.c $(PYTHON_HEADERS)
[.$(OUT_DIR).$(DYNLOAD_DIR)]_xxsubinterpreters.exe : [.$(OBJ_DIR).Modules]_xxsubinterpretersmodule.obm
    @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/SHARE=python$build_out:[$(DYNLOAD_DIR)]$(NOTDIR $(MMS$TARGET_NAME)).exe $(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

############################################################################
# VMS specific modules

[.$(OBJ_DIR).modules.vms.accdef]_accdef.obm : [.modules.vms.accdef]_accdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.acldef]_acldef.obm : [.modules.vms.acldef]_acldef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.acrdef]_acrdef.obm : [.modules.vms.acrdef]_acrdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.armdef]_armdef.obm : [.modules.vms.armdef]_armdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.brkdef]_brkdef.obm : [.modules.vms.brkdef]_brkdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.capdef]_capdef.obm : [.modules.vms.capdef]_capdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.chpdef]_chpdef.obm : [.modules.vms.chpdef]_chpdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.ciadef]_ciadef.obm : [.modules.vms.ciadef]_ciadef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.clidef]_clidef.obm : [.modules.vms.clidef]_clidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.cmbdef]_cmbdef.obm : [.modules.vms.cmbdef]_cmbdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.cvtfnmdef]_cvtfnmdef.obm : [.modules.vms.cvtfnmdef]_cvtfnmdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dcdef]_dcdef.obm : [.modules.vms.dcdef]_dcdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dmtdef]_dmtdef.obm : [.modules.vms.dmtdef]_dmtdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dpsdef]_dpsdef.obm : [.modules.vms.dpsdef]_dpsdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dscdef]_dscdef.obm : [.modules.vms.dscdef]_dscdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dvidef]_dvidef.obm : [.modules.vms.dvidef]_dvidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dvsdef]_dvsdef.obm : [.modules.vms.dvsdef]_dvsdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.efndef]_efndef.obm : [.modules.vms.efndef]_efndef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.eradef]_eradef.obm : [.modules.vms.eradef]_eradef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.fabdef]_fabdef.obm : [.modules.vms.fabdef]_fabdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.fdldef]_fdldef.obm : [.modules.vms.fdldef]_fdldef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.fpdef]_fpdef.obm : [.modules.vms.fpdef]_fpdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.fscndef]_fscndef.obm : [.modules.vms.fscndef]_fscndef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.iccdef]_iccdef.obm : [.modules.vms.iccdef]_iccdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.iledef]_iledef.obm : [.modules.vms.iledef]_iledef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.impdef]_impdef.obm : [.modules.vms.impdef]_impdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.initdef]_initdef.obm : [.modules.vms.initdef]_initdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.iodef]_iodef.obm : [.modules.vms.iodef]_iodef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.issdef]_issdef.obm : [.modules.vms.issdef]_issdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.jbcmsgdef]_jbcmsgdef.obm : [.modules.vms.jbcmsgdef]_jbcmsgdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.jpidef]_jpidef.obm : [.modules.vms.jpidef]_jpidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.kgbdef]_kgbdef.obm : [.modules.vms.kgbdef]_kgbdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.lckdef]_lckdef.obm : [.modules.vms.lckdef]_lckdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.libclidef]_libclidef.obm : [.modules.vms.libclidef]_libclidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.libdtdef]_libdtdef.obm : [.modules.vms.libdtdef]_libdtdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.libfisdef]_libfisdef.obm : [.modules.vms.libfisdef]_libfisdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.lkidef]_lkidef.obm : [.modules.vms.lkidef]_lkidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.lnmdef]_lnmdef.obm : [.modules.vms.lnmdef]_lnmdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.maildef]_maildef.obm : [.modules.vms.maildef]_maildef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.mntdef]_mntdef.obm : [.modules.vms.mntdef]_mntdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.nsadef]_nsadef.obm : [.modules.vms.nsadef]_nsadef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.ossdef]_ossdef.obm : [.modules.vms.ossdef]_ossdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.pcbdef]_pcbdef.obm : [.modules.vms.pcbdef]_pcbdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.ppropdef]_ppropdef.obm : [.modules.vms.ppropdef]_ppropdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.pqldef]_pqldef.obm : [.modules.vms.pqldef]_pqldef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.prcdef]_prcdef.obm : [.modules.vms.prcdef]_prcdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.prdef]_prdef.obm : [.modules.vms.prdef]_prdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.prvdef]_prvdef.obm : [.modules.vms.prvdef]_prvdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.prxdef]_prxdef.obm : [.modules.vms.prxdef]_prxdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.pscandef]_pscandef.obm : [.modules.vms.pscandef]_pscandef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.psldef]_psldef.obm : [.modules.vms.psldef]_psldef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.pxbdef]_pxbdef.obm : [.modules.vms.pxbdef]_pxbdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.quidef]_quidef.obm : [.modules.vms.quidef]_quidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.rabdef]_rabdef.obm : [.modules.vms.rabdef]_rabdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.regdef]_regdef.obm : [.modules.vms.regdef]_regdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.rmidef]_rmidef.obm : [.modules.vms.rmidef]_rmidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.rmsdef]_rmsdef.obm : [.modules.vms.rmsdef]_rmsdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.rsdmdef]_rsdmdef.obm : [.modules.vms.rsdmdef]_rsdmdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.sdvdef]_sdvdef.obm : [.modules.vms.sdvdef]_sdvdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.sjcdef]_sjcdef.obm : [.modules.vms.sjcdef]_sjcdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.ssdef]_ssdef.obm : [.modules.vms.ssdef]_ssdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.statedef]_statedef.obm : [.modules.vms.statedef]_statedef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.stenvdef]_stenvdef.obm : [.modules.vms.stenvdef]_stenvdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.stsdef]_stsdef.obm : [.modules.vms.stsdef]_stsdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.syidef]_syidef.obm : [.modules.vms.syidef]_syidef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.uafdef]_uafdef.obm : [.modules.vms.uafdef]_uafdef.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.uaidef]_uaidef.obm : [.modules.vms.uaidef]_uaidef.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).modules.vms.rms]_rms.obm : [.modules.vms.rms]_rms.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.decc]_decc.obm : [.modules.vms.decc]_decc.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.lib]_lib.obm : [.modules.vms.lib]_lib.c $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.ile3]_ile3.obm : [.modules.vms.ile3]_ile3.c [.modules.vms.ile3]_ile3.h $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.sys]_sys.obm : [.modules.vms.sys]_sys.c [.modules.vms.ile3]_ile3.h $(PYTHON_HEADERS)
[.$(OBJ_DIR).modules.vms.dtr]_dtr.obm : [.modules.vms.dtr]_dtr.c $(PYTHON_HEADERS)

[.$(OBJ_DIR).modules.rdb]sql.obj : [.modules.rdb]sql.sqlmod
    sqlmod [.modules.rdb]sql.sqlmod
    rename sql.obj python$build_obj:[modules.rdb]
[.$(OBJ_DIR).modules.rdb]_rdb.obm : [.modules.rdb]_rdb.c $(PYTHON_HEADERS)

[.$(OUT_DIR).$(DYNLOAD_DIR)]_accdef.exe : [.$(OBJ_DIR).modules.vms.accdef]_accdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_acldef.exe : [.$(OBJ_DIR).modules.vms.acldef]_acldef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_acrdef.exe : [.$(OBJ_DIR).modules.vms.acrdef]_acrdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_armdef.exe : [.$(OBJ_DIR).modules.vms.armdef]_armdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_brkdef.exe : [.$(OBJ_DIR).modules.vms.brkdef]_brkdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_capdef.exe : [.$(OBJ_DIR).modules.vms.capdef]_capdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_chpdef.exe : [.$(OBJ_DIR).modules.vms.chpdef]_chpdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ciadef.exe : [.$(OBJ_DIR).modules.vms.ciadef]_ciadef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_clidef.exe : [.$(OBJ_DIR).modules.vms.clidef]_clidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_cmbdef.exe : [.$(OBJ_DIR).modules.vms.cmbdef]_cmbdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_cvtfnmdef.exe : [.$(OBJ_DIR).modules.vms.cvtfnmdef]_cvtfnmdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dcdef.exe : [.$(OBJ_DIR).modules.vms.dcdef]_dcdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dmtdef.exe : [.$(OBJ_DIR).modules.vms.dmtdef]_dmtdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dpsdef.exe : [.$(OBJ_DIR).modules.vms.dpsdef]_dpsdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dscdef.exe : [.$(OBJ_DIR).modules.vms.dscdef]_dscdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dvidef.exe : [.$(OBJ_DIR).modules.vms.dvidef]_dvidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dvsdef.exe : [.$(OBJ_DIR).modules.vms.dvsdef]_dvsdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_efndef.exe : [.$(OBJ_DIR).modules.vms.efndef]_efndef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_eradef.exe : [.$(OBJ_DIR).modules.vms.eradef]_eradef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fabdef.exe : [.$(OBJ_DIR).modules.vms.fabdef]_fabdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fdldef.exe : [.$(OBJ_DIR).modules.vms.fdldef]_fdldef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fpdef.exe : [.$(OBJ_DIR).modules.vms.fpdef]_fpdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_fscndef.exe : [.$(OBJ_DIR).modules.vms.fscndef]_fscndef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_iccdef.exe : [.$(OBJ_DIR).modules.vms.iccdef]_iccdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_iledef.exe : [.$(OBJ_DIR).modules.vms.iledef]_iledef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_impdef.exe : [.$(OBJ_DIR).modules.vms.impdef]_impdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_initdef.exe : [.$(OBJ_DIR).modules.vms.initdef]_initdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_iodef.exe : [.$(OBJ_DIR).modules.vms.iodef]_iodef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_issdef.exe : [.$(OBJ_DIR).modules.vms.issdef]_issdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_jbcmsgdef.exe : [.$(OBJ_DIR).modules.vms.jbcmsgdef]_jbcmsgdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_jpidef.exe : [.$(OBJ_DIR).modules.vms.jpidef]_jpidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_kgbdef.exe : [.$(OBJ_DIR).modules.vms.kgbdef]_kgbdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lckdef.exe : [.$(OBJ_DIR).modules.vms.lckdef]_lckdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_libclidef.exe : [.$(OBJ_DIR).modules.vms.libclidef]_libclidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_libdtdef.exe : [.$(OBJ_DIR).modules.vms.libdtdef]_libdtdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_libfisdef.exe : [.$(OBJ_DIR).modules.vms.libfisdef]_libfisdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lkidef.exe : [.$(OBJ_DIR).modules.vms.lkidef]_lkidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lnmdef.exe : [.$(OBJ_DIR).modules.vms.lnmdef]_lnmdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_maildef.exe : [.$(OBJ_DIR).modules.vms.maildef]_maildef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_mntdef.exe : [.$(OBJ_DIR).modules.vms.mntdef]_mntdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_nsadef.exe : [.$(OBJ_DIR).modules.vms.nsadef]_nsadef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ossdef.exe : [.$(OBJ_DIR).modules.vms.ossdef]_ossdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pcbdef.exe : [.$(OBJ_DIR).modules.vms.pcbdef]_pcbdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ppropdef.exe : [.$(OBJ_DIR).modules.vms.ppropdef]_ppropdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pqldef.exe : [.$(OBJ_DIR).modules.vms.pqldef]_pqldef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prcdef.exe : [.$(OBJ_DIR).modules.vms.prcdef]_prcdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prdef.exe : [.$(OBJ_DIR).modules.vms.prdef]_prdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prvdef.exe : [.$(OBJ_DIR).modules.vms.prvdef]_prvdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_prxdef.exe : [.$(OBJ_DIR).modules.vms.prxdef]_prxdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pscandef.exe : [.$(OBJ_DIR).modules.vms.pscandef]_pscandef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_psldef.exe : [.$(OBJ_DIR).modules.vms.psldef]_psldef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_pxbdef.exe : [.$(OBJ_DIR).modules.vms.pxbdef]_pxbdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_quidef.exe : [.$(OBJ_DIR).modules.vms.quidef]_quidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rabdef.exe : [.$(OBJ_DIR).modules.vms.rabdef]_rabdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_regdef.exe : [.$(OBJ_DIR).modules.vms.regdef]_regdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rmidef.exe : [.$(OBJ_DIR).modules.vms.rmidef]_rmidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rmsdef.exe : [.$(OBJ_DIR).modules.vms.rmsdef]_rmsdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rsdmdef.exe : [.$(OBJ_DIR).modules.vms.rsdmdef]_rsdmdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sdvdef.exe : [.$(OBJ_DIR).modules.vms.sdvdef]_sdvdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sjcdef.exe : [.$(OBJ_DIR).modules.vms.sjcdef]_sjcdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ssdef.exe : [.$(OBJ_DIR).modules.vms.ssdef]_ssdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_statedef.exe : [.$(OBJ_DIR).modules.vms.statedef]_statedef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_stenvdef.exe : [.$(OBJ_DIR).modules.vms.stenvdef]_stenvdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_stsdef.exe : [.$(OBJ_DIR).modules.vms.stsdef]_stsdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_syidef.exe : [.$(OBJ_DIR).modules.vms.syidef]_syidef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_uafdef.exe : [.$(OBJ_DIR).modules.vms.uafdef]_uafdef.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_uaidef.exe : [.$(OBJ_DIR).modules.vms.uaidef]_uaidef.obm

[.$(OUT_DIR).$(DYNLOAD_DIR)]_rms.exe : [.$(OBJ_DIR).modules.vms.rms]_rms.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_decc.exe : [.$(OBJ_DIR).modules.vms.decc]_decc.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_lib.exe : [.$(OBJ_DIR).modules.vms.lib]_lib.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_ile3.exe : [.$(OBJ_DIR).modules.vms.ile3]_ile3.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_sys.exe : [.$(OBJ_DIR).modules.vms.sys]_sys.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_dtr.exe : [.$(OBJ_DIR).modules.vms.dtr]_dtr.obm
[.$(OUT_DIR).$(DYNLOAD_DIR)]_rdb.exe : [.$(OBJ_DIR).modules.rdb]_rdb.obm,[.$(OBJ_DIR).modules.rdb]sql.obj

############################################################################
# testembed EXE
[.$(OBJ_DIR).Programs]_testembed.obc : [.Programs]_testembed.c $(PYTHON_HEADERS)
[.$(OUT_DIR)]_testembed.exe : [.$(OBJ_DIR).Programs]_testembed.obc [.$(OBJ_DIR).vms]vms_crtl_init.obc [.$(OUT_DIR)]python$shr.exe
   @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
    $(LINK)$(LINK_FLAGS)/EXECUTABLE=python$build_out:[000000]$(NOTDIR $(MMS$TARGET_NAME)).exe [.$(OBJ_DIR).vms]vms_crtl_init.obc,$(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT

############################################################################
# Python EXE
[.$(OBJ_DIR).vms]vms_crtl_init.obc : [.vms]vms_crtl_init.c
[.$(OBJ_DIR).Programs]python.obc : [.Programs]python.c $(PYTHON_HEADERS)

[.$(OUT_DIR)]python.exe : [.$(OBJ_DIR).Programs]python.obc [.$(OBJ_DIR).vms]vms_crtl_init.obc [.$(OUT_DIR)]python$shr.exe
   @ pipe create/dir $(DIR $(MMS$TARGET)) | copy SYS$INPUT nl:
.IF X86_64
    $(LINK)$(LINK_FLAGS)/EXECUTABLE=python$build_out:[000000]$(NOTDIR $(MMS$TARGET_NAME)).exe [.$(OBJ_DIR).vms]vms_crtl_init.obc,$(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT
.ELSE
    $(LINK)$(LINK_FLAGS)/THREADS/EXECUTABLE=python$build_out:[000000]$(NOTDIR $(MMS$TARGET_NAME)).exe [.$(OBJ_DIR).vms]vms_crtl_init.obc,$(MMS$SOURCE),[.vms.opt]$(NOTDIR $(MMS$TARGET_NAME)).opt/OPT
.ENDIF

############################################################################
CLEAN :
    del/tree [.$(OUT_DIR)...]*.*;*
