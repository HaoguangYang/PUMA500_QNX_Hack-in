/*
 * $QNXLicenseC: 
 * Copyright 2013, QNX Software Systems.
 * Copyright 2013, Adeneo Embedded.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $
 */


/* 
 * The purpose of this module is to control the access to shared variables
 * to shared mutexes that are used to implement synchronization control for
 * multiple instances of the dma library.
 */

#include "dma.h"

/* local variables */
static int fd;
static dma_shmem_t * shmem_ptr;

/* This function opens and maps shared memory that was created */
int dmasync_init(void) {

    fd = shm_open("/dma_mutex",O_RDWR , 0666);
    if (fd == -1) {
        goto fail1;
    }

    /* Map it to our control structure */
    shmem_ptr = mmap(    0,
                         sizeof(dma_shmem_t),
                         PROT_READ|PROT_WRITE,
                         MAP_SHARED,
                         fd,
                         0          );
    if (shmem_ptr == MAP_FAILED) {
        goto fail2;
    }

    return 0;
fail2:
    close(fd);
fail1:
    return -1;
}


void dmasync_fini(void) {
    munmap(shmem_ptr,sizeof(dma_shmem_t));
    close(fd);
}

/* Shared-memory variable access functions */

pthread_mutex_t * dmasync_cmdmutex_get() {
    return &(shmem_ptr->command_mutex);
}

pthread_mutex_t * dmasync_libinit_mutex_get() {
    return &(shmem_ptr->libinit_mutex);
}

pthread_mutex_t * dmasync_regmutex_get() {
    return &(shmem_ptr->register_mutex);
}


int dmasync_is_first_process() {
    if (shmem_ptr->process_cnt == 1)
        return 1;
    else
        return 0;
}

int dmasync_is_last_process() {
    if (shmem_ptr->process_cnt == 0)
        return 1;
    else
        return 0;
}

void dmasync_process_cnt_incr() {
    shmem_ptr->process_cnt++;
}

void dmasync_process_cnt_decr() {
    shmem_ptr->process_cnt--;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
