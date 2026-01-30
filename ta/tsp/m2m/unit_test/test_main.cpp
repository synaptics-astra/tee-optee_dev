/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
//! \file test_main.cpp
//!
//! \brief AmpUnitTest entry file
//!
//!     Version    Date                     Author
//!     V 0.01,    June 20 2016             Yang Liu
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "gtest/gtest.h"

const char *gPath = NULL;
// Andorid and linux sdk both have /data/ directory
// Pure linux don't have this directory, it will fallback to stdout
const char *gOutPath = "/data"; // default output path


static void showCfg() {
    printf("Note: gPath = %s\n", gPath);
    printf("Note: gOutPath = %s\n", gOutPath);
}

static void usage() {
    printf("TAUnitTest:\n"
           "\n"
           "-h,          --help,           Show this message\n"
           "-p PATH,     --path=PATH,      Specify path of test resources\n"
           "-o PATH,     --outpath=PATH,   Specify path for output data\n"
           );
}

static int checkCreateDir(const char *dirname) {
    DIR *pDir = NULL;
    if ((pDir = opendir(dirname)) == NULL) {
        if (mkdir(dirname, S_IRWXG | S_IRWXU | S_IRWXO) < 0) {
            fprintf(stderr, "create dir %s fail\n", dirname);
            return -1;
        }
        return 0;
    }
    closedir(pDir);
    return 0;
}

static void parseCfg(int argc, char *argv[]) {
    const char *short_options = "hp:o:";
    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"path", required_argument, NULL, 'p'},
        {"outpath", required_argument, NULL, 'o'},
    };

    while (1) {
        int c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'p':
                gPath = optarg;
                break;
            case 'o':
                gOutPath = optarg;
                break;
            case 'h':
            case '?':
            default:
                usage();
                exit(-1);
        }
    }
    argc -= optind;
    argv += optind;
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    parseCfg(argc, argv);
    showCfg();
    checkCreateDir(gOutPath);
    return RUN_ALL_TESTS();
}
