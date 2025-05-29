/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _CMDQUEUE_H_
#define _CMDQUEUE_H_

#include "vpp_com.h"

#ifdef __cplusplus
extern "C" {
#endif

/* structure of a command */
typedef struct CMD_T {
    FUNCPTR func;      // command function pointer
    int params[MAX_NUM_PARAMS];    // command function parameters
} CMD;

typedef struct CMDQUEUE_T {
    CMD cmds[MAX_NUM_CMDS];    // cmds array
    int head;           // read pointer of queue, update with pop
    int tail;           // write pointer of queue, update with shadow_tail with commit
    int shadow_tail;    // shadow write pointer of queue, update with push
} CMDQUEUE;

/******* command queue APIs ********/
/***************************************************************
 * FUNCTION: make a command
 * PARAMS: *cmd - pointer to a command
 *         func - function call of this command
 *         aram_num - number of parameters of func
 *         *params - pointer to function parameters
 ****************************************************************/
void make_cmd(CMD *cmd, FUNCPTR func, int param_num, int *params);

/*********************************************************
 * FUNCTION: reset a command queue to be empty state
 * PARAMS: *cmdq - pointer to a command queue
 *
 ********************************************************/
void cmdq_reset(CMDQUEUE *cmdq);

/*********************************************************
 * FUNCTION: push a command into a command queue
 * PARAMS: *cmdq - pointer to a command queue
 *         *cmd - pointer to a command
 * RETURN: 1 - succeed
 *         0 - command queue is full
 * NOTE: the command pushed into command queue can not
 *       be poped out until a commit is issued.
 ********************************************************/
int cmdq_push(CMDQUEUE *cmdq, CMD *cmd);

/******************************************************************
 * FUNCTION: pop a command out of a command queue
 * PARAMS: *cmdq - pointer to a command queue
 *         *cmd - pointer to the command
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int cmdq_pop(CMDQUEUE *cmdq, CMD *cmd);

/*********************************************************************
 * FUNCTION: commit previous commands pushed into a command queue
 * PARAMS: *cmdq - pointer to a command queue
 *
 ********************************************************************/
void cmdq_commit(CMDQUEUE *cmdq);

/*********************************************************************
 * FUNCTION: remove previous commands pushed into a command queue
 *           without commit.
 * PARAMS: *cmdq - pointer to a command queue
 *
 ********************************************************************/
void cmdq_uncommit(CMDQUEUE *cmdq);


#ifdef __cplusplus
}
#endif

#endif
