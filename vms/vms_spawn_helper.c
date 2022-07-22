#include "vms/vms_spawn_helper.h"
#include <string.h>
#include <builtins.h>

#define MAX_SPAWN 256

vms_spawn_state_t _state[MAX_SPAWN];

static vms_spawn_state_t _free = { 
    -1,     // _pid
    -1,     // _status
     0,     // _finished
    -1,     // _stdout
    -1,     // _stderr
     1      // spawned by default
};

static unsigned long _finished_counter = 1;
static unsigned long _initialized = 0;   // 0 - uninitialized, 1 - initialization, 2 - initializes

vms_spawn_state_t* vms_spawn_alloc() {
    // init table
    if (__CMP_SWAP_LONG(&_initialized, 0, 1)) {
        memset(_state, 0, sizeof(_state));
        ++_initialized;
    } else {
        while(_initialized == 1) {
            ;
        }
    }
    // find empty pid
    while(1) {
        int pos = -1;
        unsigned int oldest = 0xffffffff;
        unsigned int oldest_pid = 0;
        for(int i = 0; i < MAX_SPAWN; ++i) {
            if (__CMP_SWAP_LONG(&_state[i]._pid, 0, -1)) {
                // found an empty position
                _state[i] = _free;
                return &_state[i];
            }
            if (_state[i]._finished == 0) {
                // pid is set, process is not finished
                continue;
            }
            // pid is set, process is finished, so find the oldest
            if (_state[i]._finished < oldest && _state[i]._pid != 0xffffffff) {
                pos = i;
                oldest = _state[pos]._finished;
                oldest_pid = _state[pos]._pid;
            }
        }
        if (pos == -1) {
            // no free positions, no finished process, do another try
            continue;
        }
        // found the oldest finished process, which retcode is not poped, so overwrite it
        if (__CMP_SWAP_LONG(&_state[pos]._pid, oldest_pid, -1)) {
            _state[pos] = _free;
            return &_state[pos];
        }
        // someone beat us here, do another try
    }
    return NULL;
}

void vms_spawn_finish(vms_spawn_state_t* pstate) {
    if (_state <= pstate && pstate < _state + MAX_SPAWN) {
        pstate->_finished = __ATOMIC_INCREMENT_LONG(&_finished_counter);
        if (pstate->_finished == 0) {
            // regenerate generations, from 1 to MAX
            for(int i = 0; i < MAX_SPAWN; ++i) {
                if (_state[i]._finished) {
                    _state[i]._finished = __ATOMIC_INCREMENT_LONG(&_finished_counter);
                }
            }
            // set current as latest
            pstate->_finished = __ATOMIC_INCREMENT_LONG(&_finished_counter);
        }
    }
}

vms_spawn_state_t* vms_spawn_state(unsigned int pid) {
    for(int i = 0; i < MAX_SPAWN; ++i) {
        if (_state[i]._pid == pid) {
            return &_state[i];
        }
    }
    return NULL;
}

void vms_spawn_free(vms_spawn_state_t *pstate) {
    if (_state <= pstate && pstate < _state + MAX_SPAWN) {
        *pstate = _free;
    }
}
