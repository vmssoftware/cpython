#include "vms/vms_spawn_helper.h"
#include <string.h>
#include <errno.h>
#include <builtins.h>

// #define _DO_TRACE_FILE_ "SPW"
#include "vms/trace.h"

#define MAX_SPAWN 256

#define PID_EMPTY       0           // the slot is free
#define PID_RESERVED    0xffffffff  // the slot is allocated, pid is not set yet

// _finished and _status are modified asynchronously by lib$spawn and AST
// NOTE: arrays must be in 32-bit P0 space, lib$spawn and AST argument accept only 32-bit addresses
volatile unsigned int   _pid[MAX_SPAWN];
volatile int            _status[MAX_SPAWN];
volatile unsigned int   _finished[MAX_SPAWN];

static volatile unsigned int _finished_counter = 1;
static volatile unsigned int _initialized = 0;   // 0 - uninitialized, 1 - initialization, 2 - initialized

static int _init_pos(int pos, unsigned int **pppid, int **ppstatus, unsigned int **ppfinished) {
    _status[pos] = -1;
    _finished[pos] = 0;
    *pppid = (unsigned int *)(_pid + pos);
    *ppstatus = (int *)(_status + pos);
    *ppfinished = (unsigned int *)(_finished + pos);
    _TRACE_LINE_V_("_init_pos %i\n", pos);
    return pos;
}

int vms_spawn_alloc(unsigned int **pppid, int **ppstatus, unsigned int **ppfinished) {
    // init table
    if (__CMP_SWAP_LONG(&_initialized, 0, 1)) {
        memset((void *)_pid, 0, sizeof(_pid));
        memset((void *)_status, -1, sizeof(_status));
        memset((void *)_finished, 0, sizeof(_finished));
        __MB();
        _initialized = 2;
    } else {
        while(_initialized == 1) {
            ;
        }
    }
    // find empty pid
    while(1) {
        int pos = -1;
        unsigned int oldest = 0;
        unsigned int oldest_pid = 0;
        for(int i = 0; i < MAX_SPAWN; ++i) {
            if (__CMP_SWAP_LONG(_pid + i, PID_EMPTY, PID_RESERVED)) {
                // found an empty position
                _TRACE_LINE_("vms_spawn_alloc found empty\n");
                return _init_pos(i, pppid, ppstatus, ppfinished);
            }
            unsigned int finished = _finished[i];
            unsigned int pid = _pid[i];
            if (finished == 0 || pid == PID_EMPTY || pid == PID_RESERVED) {
                // process is not finished or the slot is being changed
                continue;
            }
            // pid is set, process is finished, so find the oldest
            if (pos == -1 || finished < oldest) {
                pos = i;
                oldest = finished;
                oldest_pid = pid;
            }
        }
        if (pos == -1) {
            // no free positions, no finished process
            break;
        }
        // found the oldest finished process, which retcode is not poped, so overwrite it
        if (__CMP_SWAP_LONG(_pid + pos, oldest_pid, PID_RESERVED)) {
            _TRACE_LINE_V_("vms_spawn_alloc found oldest %X\n", oldest_pid);
            return _init_pos(pos, pppid, ppstatus, ppfinished);
        }
        // someone beat us here, do another try
    }
    _TRACE_LINE_("vms_spawn_alloc failed\n");
    errno = EAGAIN;
    return -1;
}

void vms_spawn_release(int idx) {
    if (idx >= 0 && idx < MAX_SPAWN) {
        _TRACE_LINE_V_("vms_spawn_release idx %i\n", idx);
        _finished[idx] = 0;
        _status[idx] = -1;
        __MB();
        _pid[idx] = PID_EMPTY;
    }
}

int vms_spawn_finish(unsigned int *pfinished) {
    if (pfinished >= _finished && pfinished < _finished + MAX_SPAWN) {
        int idx = pfinished - _finished;
        _TRACE_LINE_V_("vms_spawn_finish idx %i\n", idx);
        unsigned int generation = __ATOMIC_INCREMENT_LONG(&_finished_counter);
        if (generation == 0) {
            // regenerate generations, from 1 to MAX
            _TRACE_LINE_("vms_spawn_finish regenerate\n");
            for(int i = 0; i < MAX_SPAWN; ++i) {
                if (_finished[i]) {
                    _finished[i] = __ATOMIC_INCREMENT_LONG(&_finished_counter);
                }
            }
            // set current as latest
            generation = __ATOMIC_INCREMENT_LONG(&_finished_counter);
        }
        // the status is already written by lib$spawn, make it visible before the generation
        __MB();
        _finished[idx] = generation;
        _TRACE_LINE_V_("vms_spawn_finish finished %u\n", generation);
        return idx;
    }
    return -1;
}

int vms_spawn_status(unsigned int pid, int *pstatus, unsigned int *pfinished, int free) {
    if (pid == PID_EMPTY || pid == PID_RESERVED) {
        _TRACE_LINE_V_("vms_spawn_status invalid pid %X\n", pid);
        return -1;
    }
    for(int i = 0; i < MAX_SPAWN; ++i) {
        if (_pid[i] == pid) {
            // read finished before status, the status is valid only if finished is set
            unsigned int finished = _finished[i];
            __MB();
            int status = finished ? _status[i] : -1;
            _TRACE_LINE_V_("vms_spawn_status pid %X idx %i status %i finished %X\n", pid, i, status, finished);
            if (pstatus) {
                *pstatus = status;
            }
            if (pfinished) {
                *pfinished = finished;
            }
            if (free && finished) {
                // free only finished process, otherwise AST will write into reused slot
                _finished[i] = 0;
                __CMP_SWAP_LONG(_pid + i, pid, PID_EMPTY);
            }
            return i;
        }
    }
    _TRACE_LINE_V_("vms_spawn_status failed pid %X\n", pid);
    return -1;
}
