/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2024 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file vpu_plugin.c
//!
//! \brief vpu plugin for optee.
//!
//! Purpose:
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#include "vpu_plugin.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <tee_plugin_method.h>
#include <unistd.h>

#define H1_IRQ_DEV "/dev/h1_irq"

enum {
    H1_IOCTL_POLL_INT,
};

static int g_h1_fd;

static int dev_open(void)
{
    g_h1_fd = open(H1_IRQ_DEV, O_RDWR);
    if (g_h1_fd < 0)
        return -errno;

    return 0;
}

static TEEC_Result vpu_plugin_init(void)
{
    g_h1_fd = -1;

    return TEEC_SUCCESS;
}

static TEEC_Result vpu_plugin_invoke(unsigned int cmd, unsigned int sub_cmd,
                                     void *data, size_t data_len,
                                     size_t *out_len)
{
    int ret = 0;

    switch (cmd) {
    case H1_PLUGIN_CMD_INIT:
        if (g_h1_fd == -1)
            ret = dev_open();
        break;
    case H1_PLUGIN_CMD_DEINIT:
        if (g_h1_fd >= 0) {
            close(g_h1_fd);
            g_h1_fd = -1;
        }
        break;
    case H1_PLUGIN_CMD_POLL_INT:
        cmd = H1_IOCTL_POLL_INT;
        sub_cmd = 0;
        ret = ioctl(g_h1_fd, cmd, &sub_cmd);
        break;
    default:
        ret = TEEC_ERROR_NOT_SUPPORTED;
        break;
    }

    return ret;
}

struct plugin_method plugin_method = {
    .name = "vpu_plugin",
    .uuid = VPU_PLUGIN_UUID,
    .init = vpu_plugin_init,
    .invoke = vpu_plugin_invoke,
};
