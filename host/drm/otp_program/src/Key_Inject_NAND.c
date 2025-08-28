
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define MEMGETINFO _IOR('M', 1, struct mtd_info_user)
#define MEMERASE   _IOW('M', 2, struct erase_info_user)

#define NAND_BLOCK_SIZE                 (262144)

struct mtd_info_user {
    unsigned char type;
    unsigned int  flags;
    unsigned int  size;
    unsigned int  erasesize;
    unsigned int  writesize;
    unsigned int  oobsize;
    unsigned long long padding;
};

struct erase_info_user {
    unsigned int start;
    unsigned int length;
};

static int mtd_erase(int fd, unsigned int addr, unsigned int len)
{
    int ret;
    struct erase_info_user erase_info;

    erase_info.start = addr;
    erase_info.length = len;
    return ioctl(fd, MEMERASE, &erase_info);
}

int mtd_read(unsigned char* buffer, const char* path, unsigned int addr, unsigned int size)
{
    int fd;
    int ret = 0;
    struct mtd_info_user info;

    if (buffer == NULL) {
        return -1;
    }
    if (path == NULL) {
        return -1;
    }
    fd = open(path, O_RDWR);
    if (fd < 0) {
        printf("open mtd device fail: %d\n", fd);
        return -1;
    }

    ret = ioctl(fd, MEMGETINFO, &info);
    if (ret) {
        printf("mtd getmeminfo fail\n");
        return -1;
    } else {
        printf("mtd device type[%d], flags[%x] "
        "size[%x] erasesize[%x]\n",
        info.type, info.flags, info.size,
        info.erasesize);
    }

    if ((addr + size) > info.size) {
        printf("out of mtd size\n");
        return -1;
    }
    ret = lseek(fd, addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek fail\n");
        return -1;
    }
    ret = read(fd, buffer, size);
    if (ret < 0) {
        printf("read data fail\n");
        return -1;
    }

    close(fd);
    return 0;
}

int mtd_write(unsigned char* buffer, const char* path, unsigned int addr, unsigned int size)
{
    int ret = 0;
    int fd;
    struct mtd_info_user info;
    unsigned char *backup_buffer = NULL;
    unsigned int block_start = 0, block_num = 0;
    unsigned int block_size = 0;
    unsigned int block_align_addr = 0, block_align_size = 0;

    if (buffer == NULL) {
        return -1;
    }
    if (path == NULL) {
        return -1;
    }
    fd = open(path, O_RDWR);
    if (fd < 0) {
        printf("open mtd device fail\n");
        return -1;
    }

    ret = ioctl(fd, MEMGETINFO, &info);
    if (ret) {
        printf("mtd getmeminfo fail\n");
        goto exit;
    } else {
        printf("mtd device type[%d], flags[%x] "
        "size[%x] erasesize[%x]\n",
        info.type, info.flags, info.size,
        info.erasesize);
        block_size = info.erasesize;
    }

    if ((addr + size) > info.size) {
        printf("out of mtd size\n");
        ret = -1;
        goto exit;
    }

    if ((addr % block_size == 0) && (size <= block_size)) {
        block_align_addr = addr;
        block_align_size = block_size;
    } else {
        block_start = addr / block_size;
        block_num = (addr + size + block_size) / block_size - block_start;
        block_align_addr = block_size * block_start;
        block_align_size = block_size * block_num;
    }

    backup_buffer = (unsigned char*)malloc(block_align_size);
    if (backup_buffer == NULL) {
        printf("out of memory\n");
        ret = -1;
        goto exit;
    }

    /* read original data */
    ret = lseek(fd, block_align_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek fail\n");
        goto exit;
    }
    ret = read(fd, backup_buffer, block_align_size);
    if (ret < 0) {
        printf("read data fail\n");
        goto exit;
    }

    /* update area */
    memcpy(backup_buffer + addr - block_align_addr, buffer, size);

    ret = mtd_erase(fd, block_align_addr, block_align_size);
    if (ret) {
        printf("mtd erase fail, block_align_addr = %x, block_align_size = %x\n", block_align_addr, block_align_size);
        goto exit;
    }

    /* write data */
    ret = lseek(fd, block_align_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek fail\n");
        goto exit;
    }

    ret = write(fd, backup_buffer, block_align_size);
    if (ret < 0) {
        printf("write data fail\n");
        goto exit;
    }

    /* read back data */
    ret = lseek(fd, block_align_addr, SEEK_SET);
    if (ret < 0) {
        printf("lseek fail\n");
        goto exit;
    }

    ret = read(fd, backup_buffer, block_align_size);
    if (ret < 0) {
        printf("read data fail\n");
        goto exit;
    }
    ret = 0;
    
exit:
    close(fd);
    if (backup_buffer != NULL) {
        free(backup_buffer);
        backup_buffer = NULL;
    }

    return ret;
}

int mtd_preboot_write(unsigned char* buffer, unsigned int size)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < 8; i++) {
        ret = mtd_write(buffer, "/dev/mtd1", NAND_BLOCK_SIZE * i, size);
        if (ret < 0) {
            printf("write preboot block %d error\n", i);
            return ret;
        }
    }

    return ret;
}

int mtd_keys_write(unsigned char* buffer, unsigned int size)
{
    int ret = 0;

    ret = mtd_write(buffer, "/dev/mtd12", 0, size);
    if (ret < 0) {
        printf("write key 1st error\n");
        return ret;
    }
    ret = mtd_write(buffer, "/dev/mtd13", 0, size);
    if (ret < 0) {
        printf("write key 2nd error\n");
        return ret;
    }

    return ret;
}
