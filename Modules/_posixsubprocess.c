/* Authors: Gregory P. Smith & Jeffrey Yasskin */
#include "Python.h"
#include "pycore_fileutils.h"
#if defined(HAVE_PIPE2) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#if defined(HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif /* HAVE_GRP_H */

#include "posixmodule.h"

#ifdef _Py_MEMORY_SANITIZER
# include <sanitizer/msan_interface.h>
#endif

#if defined(__ANDROID__) && __ANDROID_API__ < 21 && !defined(SYS_getdents64)
# include <sys/linux-syscalls.h>
# define SYS_getdents64  __NR_getdents64
#endif

#if defined(__linux__) && defined(HAVE_VFORK) && defined(HAVE_SIGNAL_H) && \
    defined(HAVE_PTHREAD_SIGMASK) && !defined(HAVE_BROKEN_PTHREAD_SIGMASK)
/* If this is ever expanded to non-Linux platforms, verify what calls are
 * allowed after vfork(). Ex: setsid() may be disallowed on macOS? */
# include <signal.h>
# define VFORK_USABLE 1
#endif

#if defined(__sun) && defined(__SVR4)
/* readdir64 is used to work around Solaris 9 bug 6395699. */
# define readdir readdir64
# define dirent dirent64
# if !defined(HAVE_DIRFD)
/* Some versions of Solaris lack dirfd(). */
#  define dirfd(dirp) ((dirp)->dd_fd)
#  define HAVE_DIRFD
# endif
#endif

#if defined(__FreeBSD__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__DragonFly__)
# define FD_DIR "/dev/fd"
#else
# define FD_DIR "/proc/self/fd"
#endif

#ifdef __VMS
// OpenVMS defines NGROUPS_MAX as 0... WHAT?!
#undef NGROUPS_MAX
#define NGROUPS_MAX 64
#endif
#ifdef NGROUPS_MAX
#define MAX_GROUPS NGROUPS_MAX
#else
#define MAX_GROUPS 64
#endif

#undef _MAKE_INHERIT
#ifdef __VMS
#include "vms/vms_fcntl.h"
#include "vms/vms_dsc.h"
#define _IGNORE_FCNTL_BUSY
#ifndef _IGNORE_FCNTL_BUSY
#define _MAKE_INHERIT(fd, flg, atomic) vms_fcntl((fd), F_SETFD, (flg) ? 0 : FD_CLOEXEC)
#else
inline int _MAKE_INHERIT(fd, flg, atomic) {
    int ret = vms_fcntl((fd), F_SETFD, (flg) ? 0 : FD_CLOEXEC);
    if (ret == -1 && errno == EBUSY) {
        PyErr_Clear();
        errno = 0;
        ret = 0;
    }
    return ret;
}
#endif
#else
    _MAKE_INHERIT(fd, flg, atomic) _Py_set_inheritable_async_safe((int)(fd), (flg), (atomic))
#endif

#define POSIX_CALL(call)   do { if ((call) == -1) goto error; } while (0)

static struct PyModuleDef _posixsubprocessmodule;

/* Convert ASCII to a positive int, no libc call. no overflow. -1 on error. */
static int
_pos_int_from_ascii(const char *name)
{
    int num = 0;
    while (*name >= '0' && *name <= '9') {
        num = num * 10 + (*name - '0');
        ++name;
    }
    if (*name)
        return -1;  /* Non digit found, not a number. */
    return num;
}


#if defined(__FreeBSD__) || defined(__DragonFly__)
/* When /dev/fd isn't mounted it is often a static directory populated
 * with 0 1 2 or entries for 0 .. 63 on FreeBSD, NetBSD, OpenBSD and DragonFlyBSD.
 * NetBSD and OpenBSD have a /proc fs available (though not necessarily
 * mounted) and do not have fdescfs for /dev/fd.  MacOS X has a devfs
 * that properly supports /dev/fd.
 */
static int
_is_fdescfs_mounted_on_dev_fd(void)
{
    struct stat dev_stat;
    struct stat dev_fd_stat;
    if (stat("/dev", &dev_stat) != 0)
        return 0;
    if (stat(FD_DIR, &dev_fd_stat) != 0)
        return 0;
    if (dev_stat.st_dev == dev_fd_stat.st_dev)
        return 0;  /* / == /dev == /dev/fd means it is static. #fail */
    return 1;
}
#endif


/* Returns 1 if there is a problem with fd_sequence, 0 otherwise. */
static int
_sanity_check_python_fd_sequence(PyObject *fd_sequence)
{
    Py_ssize_t seq_idx;
    long prev_fd = -1;
    for (seq_idx = 0; seq_idx < PyTuple_GET_SIZE(fd_sequence); ++seq_idx) {
        PyObject* py_fd = PyTuple_GET_ITEM(fd_sequence, seq_idx);
        long iter_fd;
        if (!PyLong_Check(py_fd)) {
            return 1;
        }
        iter_fd = PyLong_AsLong(py_fd);
        if (iter_fd < 0 || iter_fd <= prev_fd || iter_fd > INT_MAX) {
            /* Negative, overflow, unsorted, too big for a fd. */
            return 1;
        }
        prev_fd = iter_fd;
    }
    return 0;
}


/* Is fd found in the sorted Python Sequence? */
static int
_is_fd_in_sorted_fd_sequence(int fd, PyObject *fd_sequence)
{
    /* Binary search. */
    Py_ssize_t search_min = 0;
    Py_ssize_t search_max = PyTuple_GET_SIZE(fd_sequence) - 1;
    if (search_max < 0)
        return 0;
    do {
        long middle = (search_min + search_max) / 2;
        long middle_fd = PyLong_AsLong(PyTuple_GET_ITEM(fd_sequence, middle));
        if (fd == middle_fd)
            return 1;
        if (fd > middle_fd)
            search_min = middle + 1;
        else
            search_max = middle - 1;
    } while (search_min <= search_max);
    return 0;
}

static int
make_inheritable(PyObject *py_fds_to_keep, int errpipe_write)
{
    Py_ssize_t i, len;

    len = PyTuple_GET_SIZE(py_fds_to_keep);
    for (i = 0; i < len; ++i) {
        PyObject* fdobj = PyTuple_GET_ITEM(py_fds_to_keep, i);
        long fd = PyLong_AsLong(fdobj);
        assert(!PyErr_Occurred());
        assert(0 <= fd && fd <= INT_MAX);
        if (fd == errpipe_write) {
            /* errpipe_write is part of py_fds_to_keep. It must be closed at
               exec(), but kept open in the child process until exec() is
               called. */
            continue;
        }
        if (_MAKE_INHERIT((int)fd, 1, NULL) < 0)
            return -1;
    }
    return 0;
}


/* Get the maximum file descriptor that could be opened by this process.
 * This function is async signal safe for use between fork() and exec().
 */
static long
safe_get_max_fd(void)
{
    long local_max_fd;
#if defined(__NetBSD__)
    local_max_fd = fcntl(0, F_MAXFD);
    if (local_max_fd >= 0)
        return local_max_fd;
#endif
#if defined(HAVE_SYS_RESOURCE_H) && defined(__OpenBSD__)
    struct rlimit rl;
    /* Not on the POSIX async signal safe functions list but likely
     * safe.  TODO - Someone should audit OpenBSD to make sure. */
    if (getrlimit(RLIMIT_NOFILE, &rl) >= 0)
        return (long) rl.rlim_max;
#endif
#ifdef _SC_OPEN_MAX
    local_max_fd = sysconf(_SC_OPEN_MAX);
    if (local_max_fd == -1)
#endif
        local_max_fd = 256;  /* Matches legacy Lib/subprocess.py behavior. */
    return local_max_fd;
}


/* Close all file descriptors in the range from start_fd and higher
 * except for those in py_fds_to_keep.  If the range defined by
 * [start_fd, safe_get_max_fd()) is large this will take a long
 * time as it calls close() on EVERY possible fd.
 *
 * It isn't possible to know for sure what the max fd to go up to
 * is for processes with the capability of raising their maximum.
 */
static void
_close_fds_by_brute_force(long start_fd, PyObject *py_fds_to_keep)
{
    long end_fd = safe_get_max_fd();
    Py_ssize_t num_fds_to_keep = PyTuple_GET_SIZE(py_fds_to_keep);
    Py_ssize_t keep_seq_idx;
    /* As py_fds_to_keep is sorted we can loop through the list closing
     * fds in between any in the keep list falling within our range. */
    for (keep_seq_idx = 0; keep_seq_idx < num_fds_to_keep; ++keep_seq_idx) {
        PyObject* py_keep_fd = PyTuple_GET_ITEM(py_fds_to_keep, keep_seq_idx);
        int keep_fd = PyLong_AsLong(py_keep_fd);
        if (keep_fd < start_fd)
            continue;
        _Py_closerange(start_fd, keep_fd - 1);
        start_fd = keep_fd + 1;
    }
    if (start_fd <= end_fd) {
        _Py_closerange(start_fd, end_fd);
    }
}


#if defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)
/* It doesn't matter if d_name has room for NAME_MAX chars; we're using this
 * only to read a directory of short file descriptor number names.  The kernel
 * will return an error if we didn't give it enough space.  Highly Unlikely.
 * This structure is very old and stable: It will not change unless the kernel
 * chooses to break compatibility with all existing binaries.  Highly Unlikely.
 */
struct linux_dirent64 {
   unsigned long long d_ino;
   long long d_off;
   unsigned short d_reclen;     /* Length of this linux_dirent */
   unsigned char  d_type;
   char           d_name[256];  /* Filename (null-terminated) */
};

/* Close all open file descriptors in the range from start_fd and higher
 * Do not close any in the sorted py_fds_to_keep list.
 *
 * This version is async signal safe as it does not make any unsafe C library
 * calls, malloc calls or handle any locks.  It is _unfortunate_ to be forced
 * to resort to making a kernel system call directly but this is the ONLY api
 * available that does no harm.  opendir/readdir/closedir perform memory
 * allocation and locking so while they usually work they are not guaranteed
 * to (especially if you have replaced your malloc implementation).  A version
 * of this function that uses those can be found in the _maybe_unsafe variant.
 *
 * This is Linux specific because that is all I am ready to test it on.  It
 * should be easy to add OS specific dirent or dirent64 structures and modify
 * it with some cpp #define magic to work on other OSes as well if you want.
 */
static void
_close_open_fds_safe(int start_fd, PyObject* py_fds_to_keep)
{
    int fd_dir_fd;

    fd_dir_fd = _Py_open_noraise(FD_DIR, O_RDONLY);
    if (fd_dir_fd == -1) {
        /* No way to get a list of open fds. */
        _close_fds_by_brute_force(start_fd, py_fds_to_keep);
        return;
    } else {
        char buffer[sizeof(struct linux_dirent64)];
        int bytes;
        while ((bytes = syscall(SYS_getdents64, fd_dir_fd,
                                (struct linux_dirent64 *)buffer,
                                sizeof(buffer))) > 0) {
            struct linux_dirent64 *entry;
            int offset;
#ifdef _Py_MEMORY_SANITIZER
            __msan_unpoison(buffer, bytes);
#endif
            for (offset = 0; offset < bytes; offset += entry->d_reclen) {
                int fd;
                entry = (struct linux_dirent64 *)(buffer + offset);
                if ((fd = _pos_int_from_ascii(entry->d_name)) < 0)
                    continue;  /* Not a number. */
                if (fd != fd_dir_fd && fd >= start_fd &&
                    !_is_fd_in_sorted_fd_sequence(fd, py_fds_to_keep)) {
                    close(fd);
                }
            }
        }
        close(fd_dir_fd);
    }
}

#define _close_open_fds _close_open_fds_safe

#else  /* NOT (defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)) */


/* Close all open file descriptors from start_fd and higher.
 * Do not close any in the sorted py_fds_to_keep tuple.
 *
 * This function violates the strict use of async signal safe functions. :(
 * It calls opendir(), readdir() and closedir().  Of these, the one most
 * likely to ever cause a problem is opendir() as it performs an internal
 * malloc().  Practically this should not be a problem.  The Java VM makes the
 * same calls between fork and exec in its own UNIXProcess_md.c implementation.
 *
 * readdir_r() is not used because it provides no benefit.  It is typically
 * implemented as readdir() followed by memcpy().  See also:
 *   http://womble.decadent.org.uk/readdir_r-advisory.html
 */
static void
_close_open_fds_maybe_unsafe(long start_fd, PyObject* py_fds_to_keep)
{
    DIR *proc_fd_dir;
#ifndef HAVE_DIRFD
    while (_is_fd_in_sorted_fd_sequence(start_fd, py_fds_to_keep)) {
        ++start_fd;
    }
    /* Close our lowest fd before we call opendir so that it is likely to
     * reuse that fd otherwise we might close opendir's file descriptor in
     * our loop.  This trick assumes that fd's are allocated on a lowest
     * available basis. */
    close(start_fd);
    ++start_fd;
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
    if (!_is_fdescfs_mounted_on_dev_fd())
        proc_fd_dir = NULL;
    else
#endif
        proc_fd_dir = opendir(FD_DIR);
    if (!proc_fd_dir) {
        /* No way to get a list of open fds. */
        _close_fds_by_brute_force(start_fd, py_fds_to_keep);
    } else {
        struct dirent *dir_entry;
#ifdef HAVE_DIRFD
        int fd_used_by_opendir = dirfd(proc_fd_dir);
#else
        int fd_used_by_opendir = start_fd - 1;
#endif
        errno = 0;
        while ((dir_entry = readdir(proc_fd_dir))) {
            int fd;
            if ((fd = _pos_int_from_ascii(dir_entry->d_name)) < 0)
                continue;  /* Not a number. */
            if (fd != fd_used_by_opendir && fd >= start_fd &&
                !_is_fd_in_sorted_fd_sequence(fd, py_fds_to_keep)) {
                close(fd);
            }
            errno = 0;
        }
        if (errno) {
            /* readdir error, revert behavior. Highly Unlikely. */
            _close_fds_by_brute_force(start_fd, py_fds_to_keep);
        }
        closedir(proc_fd_dir);
    }
}

#define _close_open_fds _close_open_fds_maybe_unsafe

#endif  /* else NOT (defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)) */


#ifdef VFORK_USABLE
/* Reset dispositions for all signals to SIG_DFL except for ignored
 * signals. This way we ensure that no signal handlers can run
 * after we unblock signals in a child created by vfork().
 */
static void
reset_signal_handlers(const sigset_t *child_sigmask)
{
    struct sigaction sa_dfl = {.sa_handler = SIG_DFL};
    for (int sig = 1; sig < _NSIG; sig++) {
        /* Dispositions for SIGKILL and SIGSTOP can't be changed. */
        if (sig == SIGKILL || sig == SIGSTOP) {
            continue;
        }

        /* There is no need to reset the disposition of signals that will
         * remain blocked across execve() since the kernel will do it. */
        if (sigismember(child_sigmask, sig) == 1) {
            continue;
        }

        struct sigaction sa;
        /* C libraries usually return EINVAL for signals used
         * internally (e.g. for thread cancellation), so simply
         * skip errors here. */
        if (sigaction(sig, NULL, &sa) == -1) {
            continue;
        }

        /* void *h works as these fields are both pointer types already. */
        void *h = (sa.sa_flags & SA_SIGINFO ? (void *)sa.sa_sigaction :
                                              (void *)sa.sa_handler);
        if (h == SIG_IGN || h == SIG_DFL) {
            continue;
        }

        /* This call can't reasonably fail, but if it does, terminating
         * the child seems to be too harsh, so ignore errors. */
        (void) sigaction(sig, &sa_dfl, NULL);
    }
}
#endif /* VFORK_USABLE */

#ifdef __VMS
/*
 * This function is code executed in the child process immediately after vfork
 * to set things up and call exec().
 *
 */
#include <builtins.h>
#include <clidef.h>
#include <descrip.h>
#include <efndef.h>
#include <errno.h>
#include <lib$routines.h>
#include <processes.h>
#include <stsdef.h>
#include <unixio.h>
#include <unixlib.h>

#include "vms/vms_spawn_helper.h"
#include "vms/vms_sleep.h"
#include "vms/vms_mbx_util.h"

PyDoc_STRVAR(subprocess_proc_status_doc,
"proc_status(pid: int, remove = True)\n\
\n\
Pop a retcode of a process spawned via lib$spawn.\n\
\n\
Returns: tuple (found, status).\n\
\n\
");

static PyObject*
subprocess_proc_status(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("proc_status", nargs, 1, 2)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("proc_status", "args[0]", "long", args[0]);
        return NULL;
    }
    int remove = 1;
    if (nargs > 1 && args[1] != Py_None) {
        if (args[1] == Py_True) {
            remove = 1;
        } else if (args[1] == Py_False) {
            remove = 0;
        } else if (PyLong_Check(args[1])) {
            remove = PyLong_AsLong(args[1]);
        } else {
            _PyArg_BadArgument("proc_status", "args[1]", "bool or long", args[1]);
            return NULL;
        }
    }
    unsigned int pid = PyLong_AsUnsignedLong(args[0]);
    int status = -1;
    unsigned int finished = 0;
    int found = (-1 != vms_spawn_status(pid, &status, &finished, remove));
    return Py_BuildValue("(NNi)", PyBool_FromLong(found), PyBool_FromLong(finished), status);
}

// set generation on child complete
static void child_complete(int arg) {
    vms_spawn_finish((unsigned int *)arg);
}

static int
exec_dcl(char *const argv[], int p2cread, int c2pwrite) {
    int status = -1;
    int pid = -1;
    unsigned char efn = EFN$C_ENF;
    int flags = CLI$M_NOWAIT;
    $DESCRIPTOR(execute, "");
    $DESCRIPTOR(input, "");
    $DESCRIPTOR(output, "");
    char input_name[PATH_MAX + 1] = "";
    char output_name[PATH_MAX + 1] = "";
    __void_ptr32 input_ptr = NULL;
    __void_ptr32 output_ptr = NULL;

    if (p2cread != -1) {
        if (getname(p2cread, input_name, 1)) {
            input.dsc$w_length = strlen(input_name);
            set_dsc_string(input, input_name);
            input_ptr = &input;
        }
    }

    if (c2pwrite != -1) {
        if (getname(c2pwrite, output_name, 1)) {
            output.dsc$w_length = strlen(output_name);
            set_dsc_string(output, output_name);
            output_ptr = &output;
        }
    }

    // TODO: enclose each parameter in quotes, tripling existing quotes
    int i = 1;  // skip DCL
    int exec_len = 0;
    while (argv[i]) {
        exec_len += strlen(argv[i]) + 1;
        ++i;
    }

    char *execute_str = alloca(exec_len + 1);

    i = 1;
    execute_str[0] = 0;
    while (argv[i]) {
        if (i > 1) {
            strcat(execute_str, " ");
        }
        strcat(execute_str, argv[i]);
        ++i;
    }

    execute.dsc$w_length = strlen(execute_str);
    set_dsc_string(execute, execute_str);

    unsigned int *ppid, *pfinished;
    int *pstatus;

    if (vms_spawn_alloc(&ppid, &pstatus, &pfinished) != -1) {
        status = lib$spawn(
            &execute,
            input_ptr,
            output_ptr,
            &flags,
            NULL,
            ppid,
            pstatus,
            &efn,
            &child_complete,
            pfinished);

        if ($VMS_STATUS_SUCCESS(status)) {
            pid = (int)*ppid;
        }
    }

    if (input_ptr) {
        free_dsc_string(input);
    }
    if (output_ptr) {
        free_dsc_string(output);
    }
    free_dsc_string(execute);

    return pid;
}

static void check_and_change_to_non_inheritable(int fd_num, PyObject *py_fds_to_make_inherit) {
    int result = vms_fcntl(fd_num, F_GETFD, 0);
    if (result != -1 && !(result & FD_CLOEXEC)) {
        PyObject *py_fd_to_make = PyLong_FromLong(fd_num);
        if (py_fd_to_make) {
            PyList_Append(py_fds_to_make_inherit, py_fd_to_make);
            Py_DECREF(py_fd_to_make);
        }
        vms_fcntl(fd_num, F_SETFD, result | FD_CLOEXEC);
    }
#ifdef _IGNORE_FCNTL_BUSY
    if (errno == EBUSY) {
        PyErr_Clear();
        errno = 0;
    }
#endif
}

static PyObject* make_fds_non_inheritable(
    PyObject *py_fds_to_keep,
    int p2cread,
    int c2pwrite,
    int errwrite)
{
    PyObject *py_tuple_to_keep = py_fds_to_keep;
    if (p2cread > 0 || c2pwrite > 0 || errwrite > 0) {
        Py_ssize_t num_fds_to_keep = PyTuple_GET_SIZE(py_fds_to_keep);
        PyObject *py_list_keep = PyList_New(num_fds_to_keep);
        for(int i = 0; i < num_fds_to_keep; ++i) {
            PyObject *item = PyTuple_GET_ITEM(py_fds_to_keep, i);
            Py_INCREF(item);
            PyList_SET_ITEM(py_list_keep, i, item);
        }
        if (p2cread > 0) {
            PyObject *item = PyLong_FromLong(p2cread);
            PyList_Append(py_list_keep, item);
        }
        if (c2pwrite > 0) {
            PyObject *item = PyLong_FromLong(c2pwrite);
            PyList_Append(py_list_keep, item);
        }
        if (errwrite > 0) {
            PyObject *item = PyLong_FromLong(errwrite);
            PyList_Append(py_list_keep, item);
        }
        PyList_Sort(py_list_keep);
        py_tuple_to_keep = PyList_AsTuple(py_list_keep);
        Py_DECREF(py_list_keep);
    }
    int start_fd = 3;
    int end_fd = 256;
    int len = PyTuple_GET_SIZE(py_tuple_to_keep);
    PyObject *py_fds_to_make_inherit = PyList_New(0);
    for (int i = 0; i < len; ++i) {
        int keep_fd = PyLong_AsUnsignedLong(PyTuple_GET_ITEM(py_tuple_to_keep, i));
        for (int fd_num = start_fd; fd_num < keep_fd; ++fd_num) {
            check_and_change_to_non_inheritable(fd_num, py_fds_to_make_inherit);
        }
        start_fd = keep_fd + 1;
    }
    while (start_fd < end_fd) {
        check_and_change_to_non_inheritable(start_fd, py_fds_to_make_inherit);
        ++start_fd;
    }
    if (py_tuple_to_keep != py_fds_to_keep) {
        Py_DECREF(py_tuple_to_keep);
    }
    return py_fds_to_make_inherit;
}

static int
vms_child_exec(
    char *const exec_array[],
    char *const argv[],
    char *const envp[],
    const char *cwd,
    int p2cread, int p2cwrite,
    int c2pread, int c2pwrite,
    int errread, int errwrite,
    int errpipe_read, int errpipe_write,
    int close_fds, int restore_signals,
    int call_setsid,
    int call_setgid, gid_t gid,
    int call_setgroups, size_t groups_size, const gid_t *groups,
    int call_setuid, uid_t uid, int child_umask,
    const void *child_sigmask,
    PyObject *py_fds_to_keep,
    PyObject *preexec_fn,
    PyObject *preexec_fn_args_tuple)
{
    int pid = -1;

    if (argv && *argv && strcmp(*argv, "DCL") == 0) {
        pid = exec_dcl(argv, p2cread, c2pwrite);
        if (pid > 0) {
            map_fd_to_child(c2pread, -pid);
            map_fd_to_child(errread, -pid);
        }
        return pid;
    }

    static pthread_mutex_t _child_exec_vfork_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&_child_exec_vfork_mutex);

    // we should always set CWD, even if it is NULL - to restore default value
    decc$set_child_default_dir(cwd);

    int exec_error = 0;
    PyObject *py_fds_to_make_inherit = NULL;

    if (make_inheritable(py_fds_to_keep, errpipe_write) < 0) {
        goto egress;
    }

    // make the parent ends non-inheritable
    if (p2cwrite >= 0 && _MAKE_INHERIT(p2cwrite, 0, 0) < 0) goto egress;
    if (c2pread >= 0 && _MAKE_INHERIT(c2pread, 0, 0) < 0) goto egress;
    if (errread >= 0 && _MAKE_INHERIT(errread, 0, 0) < 0) goto egress;
    if (errpipe_read >= 0 && _MAKE_INHERIT(errpipe_read, 0, 0) < 0) goto egress;
    if (errpipe_write >= 0 && _MAKE_INHERIT(errpipe_write, 0, 0) < 0) goto egress;

    // make inherited
    if (p2cread >= 0 && _MAKE_INHERIT(p2cread, 1, 0) < 0) goto egress;
    if (c2pwrite >= 0 && _MAKE_INHERIT(c2pwrite, 1, 0) < 0) goto egress;
    if (errwrite >= 0 && _MAKE_INHERIT(errwrite, 1, 0) < 0) goto egress;

    if (close_fds && py_fds_to_keep && PyTuple_CheckExact(py_fds_to_keep)) {
        // TODO: see RTLS-187
        py_fds_to_make_inherit = make_fds_non_inheritable(py_fds_to_keep, p2cread, c2pwrite, errwrite);
    }

    decc$set_child_standard_streams(p2cread, c2pwrite, errwrite);
    pid = vfork();
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
    __char_ptr_ptr32 argv32 = 0, envp32 = 0;
#endif
    if (pid == 0) {
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
        int n = 0;
        while (argv[n]) {
            ++n;
        }
        argv32 = _malloc32((n + 1)* sizeof(__void_ptr32));
        n = 0;
        while (argv[n]) {
            argv32[n] = _strdup32(argv[n]);
            ++n;
        }
        argv32[n] = NULL;
        if (envp) {
            int n = 0;
            while (envp[n]) {
                ++n;
            }
            envp32 = _malloc32((n + 1)* sizeof(__void_ptr32));
            n = 0;
            while (envp[n]) {
                envp32[n] = _strdup32(envp[n]);
                ++n;
            }
            envp32[n] = NULL;
        }
#endif
        for (int i = 0; exec_array[i] != NULL; ++i) {
            const char *executable = exec_array[i];
            if (envp) {
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
                execve(executable, argv32, envp32);
#else
                execve(executable, argv, envp);
#endif
            } else {
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
                execv(executable, argv32);
#else
                execv(executable, argv);
#endif
            }
            if (errno != ENOENT && errno != ENOTDIR) {
                break;
            }
        }
        exec_error = errno;
        if (!exec_error) {
            exec_error = -1;
        }
        exit(EXIT_FAILURE);
    }

egress:

#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
    if (argv32) {
        int n = 0;
        while(argv32[n]) {
            free(argv32[n]);
            ++n;
        }
        free(argv32);
    }
    if (envp32) {
        int n = 0;
        while(envp32[n]) {
            free(envp32[n]);
            ++n;
        }
        free(envp32);
    }
#endif

    // Test if exec() is failed
    if (exec_error) {
        if (pid > 0) {
            waitpid(pid, NULL, 0);
        }
        pid = -1;
        errno = exec_error;
    }

    if (py_fds_to_make_inherit) {
        int len = PyList_GET_SIZE(py_fds_to_make_inherit);
        for (int i = 0; i < len; ++i) {
            int make_fd_inherit = PyLong_AsUnsignedLong(PyList_GET_ITEM(py_fds_to_make_inherit, i));
            _MAKE_INHERIT(make_fd_inherit, 1, 0);   // ignore errors
        }
        Py_DECREF(py_fds_to_make_inherit);
    }

    if (pid > 0) {
        map_fd_to_child(c2pread, pid);
        map_fd_to_child(errread, pid);
    }

    pthread_mutex_unlock(&_child_exec_vfork_mutex);
    return pid;
}
#else   /*  not __VMS*/
/*
 * This function is code executed in the child process immediately after
 * (v)fork to set things up and call exec().
 *
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 *
 * This restriction is documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/fork.html.
 *
 * If this function is called after vfork(), even more care must be taken.
 * The lack of preparations that C libraries normally take on fork(),
 * as well as sharing the address space with the parent, might make even
 * async-signal-safe functions vfork-unsafe. In particular, on Linux,
 * set*id() and setgroups() library functions must not be called, since
 * they have to interact with the library-level thread list and send
 * library-internal signals to implement per-process credentials semantics
 * required by POSIX but not supported natively on Linux. Another reason to
 * avoid this family of functions is that sharing an address space between
 * processes running with different privileges is inherently insecure.
 * See bpo-35823 for further discussion and references.
 *
 * In some C libraries, setrlimit() has the same thread list/signalling
 * behavior since resource limits were per-thread attributes before
 * Linux 2.6.10. Musl, as of 1.2.1, is known to have this issue
 * (https://www.openwall.com/lists/musl/2020/10/15/6).
 *
 * If vfork-unsafe functionality is desired after vfork(), consider using
 * syscall() to obtain it.
 */
_Py_NO_INLINE static void
child_exec(char *const exec_array[],
           char *const argv[],
           char *const envp[],
           const char *cwd,
           int p2cread, int p2cwrite,
           int c2pread, int c2pwrite,
           int errread, int errwrite,
           int errpipe_read, int errpipe_write,
           int close_fds, int restore_signals,
           int call_setsid,
           int call_setgid, gid_t gid,
           int call_setgroups, size_t groups_size, const gid_t *groups,
           int call_setuid, uid_t uid, int child_umask,
           const void *child_sigmask,
           PyObject *py_fds_to_keep,
           PyObject *preexec_fn,
           PyObject *preexec_fn_args_tuple)
{
    int i, saved_errno, reached_preexec = 0;
    PyObject *result;
    const char* err_msg = "";
    /* Buffer large enough to hold a hex integer.  We can't malloc. */
    char hex_errno[sizeof(saved_errno)*2+1];

    if (make_inheritable(py_fds_to_keep, errpipe_write) < 0)
        goto error;

    /* Close parent's pipe ends. */
    if (p2cwrite != -1)
        POSIX_CALL(close(p2cwrite));
    if (c2pread != -1)
        POSIX_CALL(close(c2pread));
    if (errread != -1)
        POSIX_CALL(close(errread));
    POSIX_CALL(close(errpipe_read));

    /* When duping fds, if there arises a situation where one of the fds is
       either 0, 1 or 2, it is possible that it is overwritten (#12607). */
    if (c2pwrite == 0) {
        POSIX_CALL(c2pwrite = dup(c2pwrite));
        /* issue32270 */
        if (_Py_set_inheritable_async_safe(c2pwrite, 0, NULL) < 0) {
            goto error;
        }
    }
    while (errwrite == 0 || errwrite == 1) {
        POSIX_CALL(errwrite = dup(errwrite));
        /* issue32270 */
        if (_Py_set_inheritable_async_safe(errwrite, 0, NULL) < 0) {
            goto error;
        }
    }

    /* Dup fds for child.
       dup2() removes the CLOEXEC flag but we must do it ourselves if dup2()
       would be a no-op (issue #10806). */
    if (p2cread == 0) {
        if (_Py_set_inheritable_async_safe(p2cread, 1, NULL) < 0)
            goto error;
    }
    else if (p2cread != -1)
        POSIX_CALL(dup2(p2cread, 0));  /* stdin */

    if (c2pwrite == 1) {
        if (_Py_set_inheritable_async_safe(c2pwrite, 1, NULL) < 0)
            goto error;
    }
    else if (c2pwrite != -1)
        POSIX_CALL(dup2(c2pwrite, 1));  /* stdout */

    if (errwrite == 2) {
        if (_Py_set_inheritable_async_safe(errwrite, 1, NULL) < 0)
            goto error;
    }
    else if (errwrite != -1)
        POSIX_CALL(dup2(errwrite, 2));  /* stderr */

    /* We no longer manually close p2cread, c2pwrite, and errwrite here as
     * _close_open_fds takes care when it is not already non-inheritable. */

    if (cwd)
        POSIX_CALL(chdir(cwd));

    if (child_umask >= 0)
        umask(child_umask);  /* umask() always succeeds. */

    if (restore_signals)
        _Py_RestoreSignals();

#ifdef VFORK_USABLE
    if (child_sigmask) {
        reset_signal_handlers(child_sigmask);
        if ((errno = pthread_sigmask(SIG_SETMASK, child_sigmask, NULL))) {
            goto error;
        }
    }
#endif

#ifdef HAVE_SETSID
    if (call_setsid)
        POSIX_CALL(setsid());
#endif

#ifdef HAVE_SETGROUPS
    if (call_setgroups)
        POSIX_CALL(setgroups(groups_size, groups));
#endif /* HAVE_SETGROUPS */

#ifdef HAVE_SETREGID
    if (call_setgid)
        POSIX_CALL(setregid(gid, gid));
#endif /* HAVE_SETREGID */

#ifdef HAVE_SETREUID
    if (call_setuid)
        POSIX_CALL(setreuid(uid, uid));
#endif /* HAVE_SETREUID */


    reached_preexec = 1;
    if (preexec_fn != Py_None && preexec_fn_args_tuple) {
        /* This is where the user has asked us to deadlock their program. */
        result = PyObject_Call(preexec_fn, preexec_fn_args_tuple, NULL);
        if (result == NULL) {
            /* Stringifying the exception or traceback would involve
             * memory allocation and thus potential for deadlock.
             * We've already faced potential deadlock by calling back
             * into Python in the first place, so it probably doesn't
             * matter but we avoid it to minimize the possibility. */
            err_msg = "Exception occurred in preexec_fn.";
            errno = 0;  /* We don't want to report an OSError. */
            goto error;
        }
        /* Py_DECREF(result); - We're about to exec so why bother? */
    }

    /* close FDs after executing preexec_fn, which might open FDs */
    if (close_fds) {
        /* TODO HP-UX could use pstat_getproc() if anyone cares about it. */
        _close_open_fds(3, py_fds_to_keep);
    }

    /* This loop matches the Lib/os.py _execvpe()'s PATH search when */
    /* given the executable_list generated by Lib/subprocess.py.     */
    saved_errno = 0;
    for (i = 0; exec_array[i] != NULL; ++i) {
        const char *executable = exec_array[i];
        if (envp) {
            execve(executable, argv, envp);
        } else {
            execv(executable, argv);
        }
        if (errno != ENOENT && errno != ENOTDIR && saved_errno == 0) {
            saved_errno = errno;
        }
    }
    /* Report the first exec error, not the last. */
    if (saved_errno)
        errno = saved_errno;

error:
    saved_errno = errno;
    /* Report the posix error to our parent process. */
    /* We ignore all write() return values as the total size of our writes is
       less than PIPEBUF and we cannot do anything about an error anyways.
       Use _Py_write_noraise() to retry write() if it is interrupted by a
       signal (fails with EINTR). */
    if (saved_errno) {
        char *cur;
        _Py_write_noraise(errpipe_write, "OSError:", 8);
        cur = hex_errno + sizeof(hex_errno);
        while (saved_errno != 0 && cur != hex_errno) {
            *--cur = Py_hexdigits[saved_errno % 16];
            saved_errno /= 16;
        }
        _Py_write_noraise(errpipe_write, cur, hex_errno + sizeof(hex_errno) - cur);
        _Py_write_noraise(errpipe_write, ":", 1);
        if (!reached_preexec) {
            /* Indicate to the parent that the error happened before exec(). */
            _Py_write_noraise(errpipe_write, "noexec", 6);
        }
        /* We can't call strerror(saved_errno).  It is not async signal safe.
         * The parent process will look the error message up. */
    } else {
        _Py_write_noraise(errpipe_write, "SubprocessError:0:", 18);
        _Py_write_noraise(errpipe_write, err_msg, strlen(err_msg));
    }
}
#endif /* __VMS */

/* The main purpose of this wrapper function is to isolate vfork() from both
 * subprocess_fork_exec() and child_exec(). A child process created via
 * vfork() executes on the same stack as the parent process while the latter is
 * suspended, so this function should not be inlined to avoid compiler bugs
 * that might clobber data needed by the parent later. Additionally,
 * child_exec() should not be inlined to avoid spurious -Wclobber warnings from
 * GCC (see bpo-35823).
 */
_Py_NO_INLINE static pid_t
do_fork_exec(char *const exec_array[],
             char *const argv[],
             char *const envp[],
             const char *cwd,
             int p2cread, int p2cwrite,
             int c2pread, int c2pwrite,
             int errread, int errwrite,
             int errpipe_read, int errpipe_write,
             int close_fds, int restore_signals,
             int call_setsid,
             int call_setgid, gid_t gid,
             int call_setgroups, size_t groups_size, const gid_t *groups,
             int call_setuid, uid_t uid, int child_umask,
             const void *child_sigmask,
             PyObject *py_fds_to_keep,
             PyObject *preexec_fn,
             PyObject *preexec_fn_args_tuple)
{

    pid_t pid;

#ifdef VFORK_USABLE
    if (child_sigmask) {
        /* These are checked by our caller; verify them in debug builds. */
        assert(!call_setuid);
        assert(!call_setgid);
        assert(!call_setgroups);
        assert(preexec_fn == Py_None);

        pid = vfork();
    } else
#endif

#ifdef __VMS
    return vms_child_exec(exec_array, argv, envp, cwd,
               p2cread, p2cwrite, c2pread, c2pwrite,
               errread, errwrite, errpipe_read, errpipe_write,
               close_fds, restore_signals, call_setsid,
               call_setgid, gid, call_setgroups, groups_size, groups,
               call_setuid, uid, child_umask, child_sigmask,
               py_fds_to_keep, preexec_fn, preexec_fn_args_tuple);
#else
    {
        pid = fork();
    }

    if (pid != 0) {
        return pid;
    }

    /* Child process.
     * See the comment above child_exec() for restrictions imposed on
     * the code below.
     */

    if (preexec_fn != Py_None) {
        /* We'll be calling back into Python later so we need to do this.
         * This call may not be async-signal-safe but neither is calling
         * back into Python.  The user asked us to use hope as a strategy
         * to avoid deadlock... */
        PyOS_AfterFork_Child();
    }

    child_exec(exec_array, argv, envp, cwd,
               p2cread, p2cwrite, c2pread, c2pwrite,
               errread, errwrite, errpipe_read, errpipe_write,
               close_fds, restore_signals, call_setsid,
               call_setgid, gid, call_setgroups, groups_size, groups,
               call_setuid, uid, child_umask, child_sigmask,
               py_fds_to_keep, preexec_fn, preexec_fn_args_tuple);
    _exit(255);
    return 0;  /* Dead code to avoid a potential compiler warning. */
#endif /* __VMS */
}


static PyObject *
subprocess_fork_exec(PyObject *module, PyObject *args)
{
    PyObject *gc_module = NULL;
    PyObject *executable_list, *py_fds_to_keep;
    PyObject *env_list, *preexec_fn;
    PyObject *process_args, *converted_args = NULL, *fast_args = NULL;
    PyObject *preexec_fn_args_tuple = NULL;
    PyObject *groups_list;
    PyObject *uid_object, *gid_object;
    int p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite;
    int errpipe_read, errpipe_write, close_fds, restore_signals;
    int call_setsid;
    int call_setgid = 0, call_setgroups = 0, call_setuid = 0;
    uid_t uid;
    gid_t gid, *groups = NULL;
    int child_umask;
    PyObject *cwd_obj, *cwd_obj2 = NULL;
    const char *cwd;
    pid_t pid = -1;
    int need_to_reenable_gc = 0;
    char *const *exec_array, *const *argv = NULL, *const *envp = NULL;
    Py_ssize_t arg_num, num_groups = 0;
    int need_after_fork = 0;
    int saved_errno = 0;

    if (!PyArg_ParseTuple(
            args, "OOpO!OOiiiiiiiiiiOOOiO:fork_exec",
            &process_args, &executable_list,
            &close_fds, &PyTuple_Type, &py_fds_to_keep,
            &cwd_obj, &env_list,
            &p2cread, &p2cwrite, &c2pread, &c2pwrite,
            &errread, &errwrite, &errpipe_read, &errpipe_write,
            &restore_signals, &call_setsid,
            &gid_object, &groups_list, &uid_object, &child_umask,
            &preexec_fn))
        return NULL;

    if ((preexec_fn != Py_None) &&
            (PyInterpreterState_Get() != PyInterpreterState_Main())) {
        PyErr_SetString(PyExc_RuntimeError,
                        "preexec_fn not supported within subinterpreters");
        return NULL;
    }

    if (close_fds && errpipe_write < 3) {  /* precondition */
        PyErr_SetString(PyExc_ValueError, "errpipe_write must be >= 3");
        return NULL;
    }
    if (_sanity_check_python_fd_sequence(py_fds_to_keep)) {
        PyErr_SetString(PyExc_ValueError, "bad value(s) in fds_to_keep");
        return NULL;
    }

    PyInterpreterState *interp = PyInterpreterState_Get();
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);
    if (config->_isolated_interpreter) {
        PyErr_SetString(PyExc_RuntimeError,
                        "subprocess not supported for isolated subinterpreters");
        return NULL;
    }

    /* We need to call gc.disable() when we'll be calling preexec_fn */
    if (preexec_fn != Py_None) {
        need_to_reenable_gc = PyGC_Disable();
    }

    exec_array = _PySequence_BytesToCharpArray(executable_list);
    if (!exec_array)
        goto cleanup;

    /* Convert args and env into appropriate arguments for exec() */
    /* These conversions are done in the parent process to avoid allocating
       or freeing memory in the child process. */
    if (process_args != Py_None) {
        Py_ssize_t num_args;
        /* Equivalent to:  */
        /*  tuple(PyUnicode_FSConverter(arg) for arg in process_args)  */
        fast_args = PySequence_Fast(process_args, "argv must be a tuple");
        if (fast_args == NULL)
            goto cleanup;
        num_args = PySequence_Fast_GET_SIZE(fast_args);
        converted_args = PyTuple_New(num_args);
        if (converted_args == NULL)
            goto cleanup;
        for (arg_num = 0; arg_num < num_args; ++arg_num) {
            PyObject *borrowed_arg, *converted_arg;
            if (PySequence_Fast_GET_SIZE(fast_args) != num_args) {
                PyErr_SetString(PyExc_RuntimeError, "args changed during iteration");
                goto cleanup;
            }
            borrowed_arg = PySequence_Fast_GET_ITEM(fast_args, arg_num);
            if (PyUnicode_FSConverter(borrowed_arg, &converted_arg) == 0)
                goto cleanup;
            PyTuple_SET_ITEM(converted_args, arg_num, converted_arg);
        }

        argv = _PySequence_BytesToCharpArray(converted_args);
        Py_CLEAR(converted_args);
        Py_CLEAR(fast_args);
        if (!argv)
            goto cleanup;
    }

    if (env_list != Py_None) {
        envp = _PySequence_BytesToCharpArray(env_list);
        if (!envp)
            goto cleanup;
    }

    if (cwd_obj != Py_None) {
        if (PyUnicode_FSConverter(cwd_obj, &cwd_obj2) == 0)
            goto cleanup;
        cwd = PyBytes_AsString(cwd_obj2);
    } else {
        cwd = NULL;
    }

    if (groups_list != Py_None) {
#ifdef HAVE_SETGROUPS
        Py_ssize_t i;
        gid_t gid;

        if (!PyList_Check(groups_list)) {
            PyErr_SetString(PyExc_TypeError,
                    "setgroups argument must be a list");
            goto cleanup;
        }
        num_groups = PySequence_Size(groups_list);

        if (num_groups < 0)
            goto cleanup;

        if (num_groups > MAX_GROUPS) {
            PyErr_SetString(PyExc_ValueError, "too many groups");
            goto cleanup;
        }

        if ((groups = PyMem_RawMalloc(num_groups * sizeof(gid_t))) == NULL) {
            PyErr_SetString(PyExc_MemoryError,
                    "failed to allocate memory for group list");
            goto cleanup;
        }

        for (i = 0; i < num_groups; i++) {
            PyObject *elem;
            elem = PySequence_GetItem(groups_list, i);
            if (!elem)
                goto cleanup;
            if (!PyLong_Check(elem)) {
                PyErr_SetString(PyExc_TypeError,
                                "groups must be integers");
                Py_DECREF(elem);
                goto cleanup;
            } else {
                if (!_Py_Gid_Converter(elem, &gid)) {
                    Py_DECREF(elem);
                    PyErr_SetString(PyExc_ValueError, "invalid group id");
                    goto cleanup;
                }
                groups[i] = gid;
            }
            Py_DECREF(elem);
        }
        call_setgroups = 1;

#else /* HAVE_SETGROUPS */
        PyErr_BadInternalCall();
        goto cleanup;
#endif /* HAVE_SETGROUPS */
    }

    if (gid_object != Py_None) {
#ifdef HAVE_SETREGID
        if (!_Py_Gid_Converter(gid_object, &gid))
            goto cleanup;

        call_setgid = 1;

#else /* HAVE_SETREGID */
        PyErr_BadInternalCall();
        goto cleanup;
#endif /* HAVE_SETREUID */
    }

    if (uid_object != Py_None) {
#ifdef HAVE_SETREUID
        if (!_Py_Uid_Converter(uid_object, &uid))
            goto cleanup;

        call_setuid = 1;

#else /* HAVE_SETREUID */
        PyErr_BadInternalCall();
        goto cleanup;
#endif /* HAVE_SETREUID */
    }

    /* This must be the last thing done before fork() because we do not
     * want to call PyOS_BeforeFork() if there is any chance of another
     * error leading to the cleanup: code without calling fork(). */
    if (preexec_fn != Py_None) {
        preexec_fn_args_tuple = PyTuple_New(0);
        if (!preexec_fn_args_tuple)
            goto cleanup;
    #ifdef HAVE_FORK
        PyOS_BeforeFork();
    #endif
        need_after_fork = 1;
    }

    /* NOTE: When old_sigmask is non-NULL, do_fork_exec() may use vfork(). */
    const void *old_sigmask = NULL;
#ifdef VFORK_USABLE
    /* Use vfork() only if it's safe. See the comment above child_exec(). */
    sigset_t old_sigs;
    if (preexec_fn == Py_None &&
        !call_setuid && !call_setgid && !call_setgroups) {
        /* Block all signals to ensure that no signal handlers are run in the
         * child process while it shares memory with us. Note that signals
         * used internally by C libraries won't be blocked by
         * pthread_sigmask(), but signal handlers installed by C libraries
         * normally service only signals originating from *within the process*,
         * so it should be sufficient to consider any library function that
         * might send such a signal to be vfork-unsafe and do not call it in
         * the child.
         */
        sigset_t all_sigs;
        sigfillset(&all_sigs);
        if ((saved_errno = pthread_sigmask(SIG_BLOCK, &all_sigs, &old_sigs))) {
            goto cleanup;
        }
        old_sigmask = &old_sigs;
    }
#endif

    pid = do_fork_exec(exec_array, argv, envp, cwd,
                       p2cread, p2cwrite, c2pread, c2pwrite,
                       errread, errwrite, errpipe_read, errpipe_write,
                       close_fds, restore_signals, call_setsid,
                       call_setgid, gid, call_setgroups, num_groups, groups,
                       call_setuid, uid, child_umask, old_sigmask,
                       py_fds_to_keep, preexec_fn, preexec_fn_args_tuple);

    /* Parent (original) process */
    if (pid == -1) {
        /* Capture errno for the exception. */
        saved_errno = errno;
    }

#ifdef VFORK_USABLE
    if (old_sigmask) {
        /* vfork() semantics guarantees that the parent is blocked
         * until the child performs _exit() or execve(), so it is safe
         * to unblock signals once we're here.
         * Note that in environments where vfork() is implemented as fork(),
         * such as QEMU user-mode emulation, the parent won't be blocked,
         * but it won't share the address space with the child,
         * so it's still safe to unblock the signals.
         *
         * We don't handle errors here because this call can't fail
         * if valid arguments are given, and because there is no good
         * way for the caller to deal with a failure to restore
         * the thread signal mask. */
        (void) pthread_sigmask(SIG_SETMASK, old_sigmask, NULL);
    }
#endif

    if (need_after_fork) {
    #ifdef HAVE_FORK
        PyOS_AfterFork_Parent();
    #endif
    }

cleanup:
    if (saved_errno != 0) {
        errno = saved_errno;
        /* We can't call this above as PyOS_AfterFork_Parent() calls back
         * into Python code which would see the unreturned error. */
        PyErr_SetFromErrno(PyExc_OSError);
    }

    Py_XDECREF(preexec_fn_args_tuple);
    PyMem_RawFree(groups);
    Py_XDECREF(cwd_obj2);
    if (envp)
        _Py_FreeCharPArray(envp);
    Py_XDECREF(converted_args);
    Py_XDECREF(fast_args);
    if (argv)
        _Py_FreeCharPArray(argv);
    if (exec_array)
        _Py_FreeCharPArray(exec_array);

    if (need_to_reenable_gc) {
        PyGC_Enable();
    }
    Py_XDECREF(gc_module);

    return pid == -1 ? NULL : PyLong_FromPid(pid);
}


PyDoc_STRVAR(subprocess_fork_exec_doc,
"fork_exec(args, executable_list, close_fds, pass_fds, cwd, env,\n\
          p2cread, p2cwrite, c2pread, c2pwrite,\n\
          errread, errwrite, errpipe_read, errpipe_write,\n\
          restore_signals, call_setsid,\n\
          gid, groups_list, uid,\n\
          preexec_fn)\n\
\n\
Forks a child process, closes parent file descriptors as appropriate in the\n\
child and dups the few that are needed before calling exec() in the child\n\
process.\n\
\n\
If close_fds is true, close file descriptors 3 and higher, except those listed\n\
in the sorted tuple pass_fds.\n\
\n\
The preexec_fn, if supplied, will be called immediately before closing file\n\
descriptors and exec.\n\
WARNING: preexec_fn is NOT SAFE if your application uses threads.\n\
         It may trigger infrequent, difficult to debug deadlocks.\n\
\n\
If an error occurs in the child process before the exec, it is\n\
serialized and written to the errpipe_write fd per subprocess.py.\n\
\n\
Returns: the child process's PID.\n\
\n\
Raises: Only on an error in the parent process.\n\
");

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"A POSIX helper for the subprocess module.");

static PyMethodDef module_methods[] = {
    {"fork_exec", subprocess_fork_exec, METH_VARARGS, subprocess_fork_exec_doc},
#ifdef __VMS
    {"proc_status", (PyCFunction) subprocess_proc_status, METH_FASTCALL, subprocess_proc_status_doc},
#endif
    {NULL, NULL}  /* sentinel */
};

static PyModuleDef_Slot _posixsubprocess_slots[] = {
    {0, NULL}
};

static struct PyModuleDef _posixsubprocessmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "_posixsubprocess",
        .m_doc = module_doc,
        .m_size = 0,
        .m_methods = module_methods,
        .m_slots = _posixsubprocess_slots,
};

PyMODINIT_FUNC
PyInit__posixsubprocess(void)
{
    return PyModuleDef_Init(&_posixsubprocessmodule);
}