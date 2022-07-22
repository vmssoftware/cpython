#ifndef VMS_SPAWN_HELPER_H
#define VMS_SPAWN_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned int            _pid;
    volatile int            _status;
    volatile unsigned int   _finished;
    int                     _stdout;
    int                     _stderr;
    int                     _spawned;
} vms_spawn_state_t;

/**
 * Allocates child state 
 * @return position
 */
vms_spawn_state_t* vms_spawn_alloc();

/**
 * Sets the child is finished
*/
void vms_spawn_finish(vms_spawn_state_t* pstate);

/**
 * Gets child state
 */
vms_spawn_state_t* vms_spawn_state(unsigned int pid);

/**
 * Free child state
 */
void vms_spawn_free(vms_spawn_state_t *pstate);

#ifdef __cplusplus
}
#endif

#endif
