/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _CMDQUEUE_C_

#include "com_type.h"
#include "OSAL_api.h"
#include "cmdqueue.h"

#define WRAPAROUND(a)    (((a)>=MAX_NUM_CMDS)?0:(a))

/***************************************************************
 * FUNCTION: make a command
 * PARAMS: *cmd - pointer to a command
 *         func - function call of this command
 *         param_num - number of parameters of func
 *         *params - pointer to function parameters
 ****************************************************************/
void make_cmd(CMD *cmd, FUNCPTR func, int param_num, int *params)
{
    cmd->func = func;
    GaloisMemcpy((void *)(cmd->params), (void *)params, sizeof(int)*param_num);
}

/*********************************************************
 * FUNCTION: reset a command queue to be empty state
 * PARAMS: *cmdq - pointer to a command queue
 *
 ********************************************************/
void cmdq_reset(CMDQUEUE *cmdq)
{
    /* reset read/write pointer of command queue */
    /* the command queue is empty when read pointer equals write pointer */
    cmdq->head = 0;
    cmdq->tail = cmdq->shadow_tail = 0;

    /* reset each command in command queue */
    GaloisMemSet(cmdq->cmds, 0, sizeof(CMD)*MAX_NUM_CMDS);

    return;
}


/*********************************************************
 * FUNCTION: push a command into a command queue
 * PARAMS: *cmdq - pointer to a command queue
 *         *cmd - pointer to a command
 * RETURN: 1 - succeed
 *         0 - command queue is full
 * NOTE: the command pushed into command queue can not
 *       be poped out until a commit is issued.
 ********************************************************/
int cmdq_push(CMDQUEUE *cmdq, CMD *cmd)
{
    /* first check whether command queue is full */
    if (WRAPAROUND(cmdq->shadow_tail+1)==cmdq->head)
        return 0;

    /* push command into command queue if not full */
    GaloisMemcpy((void *)&(cmdq->cmds[cmdq->shadow_tail]), (const void *)cmd, sizeof(CMD));

    /* update shadow write pointer */
    cmdq->shadow_tail = WRAPAROUND(cmdq->shadow_tail+1);

    return 1;
}


/******************************************************************
 * FUNCTION: pop a command out of a command queue
 * PARAMS: *cmdq - pointer to a command queue
 *         *cmd - pointer to the command
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int cmdq_pop(CMDQUEUE *cmdq, CMD *cmd)
{
    /* first check whether command queue is empty */
    if (cmdq->head==cmdq->tail)
        return 0;

    /* pop a command from command queue if not empty */
    GaloisMemcpy((void *)cmd, (void *)&(cmdq->cmds[cmdq->head]), sizeof(CMD));

    /* update read pointer */
    cmdq->head = WRAPAROUND(cmdq->head+1);

    return 1;
}

/*********************************************************************
 * FUNCTION: commit previous commands pushed into a command queue
 * PARAMS: *cmdq - pointer to a command queue
 *
 ********************************************************************/
void cmdq_commit(CMDQUEUE *cmdq)
{
    /* update write pointer with shadow write pointer */
    cmdq->tail = cmdq->shadow_tail;

    return;
}

/*********************************************************************
 * FUNCTION: remove previous commands pushed into a command queue
 *           without commit.
 * PARAMS: *cmdq - pointer to a command queue
 *
 ********************************************************************/
void cmdq_uncommit(CMDQUEUE *cmdq)
{
    /* update shadow write pointer with write pointer */
    cmdq->shadow_tail = cmdq->tail;

    return;
}

