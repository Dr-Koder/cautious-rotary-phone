/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SUNXI_NAND_BOOT_H__
#define __SUNXI_NAND_BOOT_H__

#include "sunxi_nand_partitions.h"
#include <linux/types.h>


#define PARTS3P0_MAGIC (0x7061727473337030)
#define TOC_MAIN_INFO_MAGIC 0x89119800
#define PHY_INFO_SIZE 0x8000
#define PHY_INFO_MAGIC 0xaa55a5a5
#define UBOOT_STAMP_VALUE 0xAE15BC34

#define TOC_LEN_OFFSET_BY_INT 9
#define TOC_MAGIC_OFFSET_BY_INT 4

#define NAND_BOOT0_BLK_START 0
#define NAND_BOOT0_BLK_CNT 2
#define NAND_UBOOT_BLK_START (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT)

#define NAND_UBOOT_BLK_CNT 6
#define NAND_BOOT0_PAGE_CNT_PER_COPY_1 64
#define NAND_BOOT0_PAGE_CNT_PER_COPY_2 128
#define NAND_BOOT0_PAGE_CNT_PER_COPY_4 256 //A50 boot0 size >= 128K
#define BOOT0_MAX_COPY_CNT (8)

#define UBOOT_SCAN_START_BLOCK 4

#define UBOOT_START_BLOCK_BIGNAND 4
#define UBOOT_START_BLOCK_SMALLNAND 8
#define UBOOT_MAX_BLOCK_NUM 50

#define PHYSIC_RECV_BLOCK 6

typedef unsigned long long uint64_t;

struct _uboot_info {
	unsigned int sys_mode;
	unsigned int use_lsb_page;
	unsigned int copys;

	unsigned int uboot_len;
	unsigned int total_len;
	unsigned int uboot_pages;
	unsigned int total_pages;

	unsigned int blocks_per_total;
	unsigned int page_offset_for_nand_info;
	unsigned int byte_offset_for_nand_info;
	unsigned char uboot_block[120];

	unsigned int nboot_copys;
	unsigned int nboot_len;
	unsigned int nboot_data_per_page; //43

	unsigned int nouse[64 - 43];
};

extern struct _boot_info *phyinfo_buf;

typedef union {
	unsigned char ndata[256];
	struct _uboot_info data;
} _UBOOT_INFO;

struct nand_super_block {
	unsigned short Block_NO;
	unsigned short Chip_NO;
};

struct _boot_info {
	unsigned int magic;
	unsigned int len;
	unsigned int sum;

	unsigned int no_use_block;
	unsigned int uboot_start_block;
	unsigned int uboot_next_block;
	unsigned int logic_start_block;
	unsigned int nand_specialinfo_page;
	unsigned int nand_specialinfo_offset;
	unsigned int physic_block_reserved;
	unsigned int nand_ddrtype;
	unsigned int ddr_timing_cfg;
	//unsigned int enable_crc; // ENABLE_CRC_MAGIC
	unsigned int nouse[128 - 12];

	_MBR mbr;			 //4k               offset 0.5k
	_PARTITION partition;		 //2.5k             offset 4.5k
	_NAND_STORAGE_INFO storage_info; //0.5k             offset 7k
	_FACTORY_BLOCK factory_block;    //2k               offset 7.5k
	//_UBOOT_INFO uboot_info;             //0.25K
	_NAND_SPECIAL_INFO nand_special_info; //1k               offset 9.5k
	uint64_t append_new_magic; /*parts3p0 -> 0x7061727473337030*/
	struct nand_partitions parts;
};

extern int nand_secure_storage_block_bak;
extern int nand_secure_storage_block;

extern struct _uboot_info uboot_info;
extern struct _boot_info *phyinfo_buf;

int nand_write_nboot_data(unsigned char *buf, unsigned int len);
int nand_read_nboot_data(unsigned char *buf, unsigned int len);
int nand_check_nboot(unsigned char *buf, unsigned int len);
int nand_write_uboot_one(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter);
int nand_write_uboot_data(unsigned char *buf, unsigned int len);
int nand_read_uboot_data(unsigned char *buf, unsigned int len);
int nand_check_uboot(unsigned char *buf, unsigned int len);
int nand_get_uboot_total_len(void);
int nand_get_nboot_total_len(void);
int nand_get_param(void *nand_param);
int nand_get_param_for_uboottail(void *nand_param);
int NAND_UpdatePhyArch(void);
int nand_erase_chip(unsigned int chip, unsigned int start_block, unsigned int end_block, unsigned int force_flag);
void nand_erase_special_block(void);
int nand_uboot_erase_all_chip(unsigned int force_flag);
int nand_dragonborad_test_one(unsigned char *buf, unsigned char *oob, unsigned int blk_num);
int get_default_uboot_block(unsigned int *start_block, unsigned int *next_block);
int set_uboot_start_and_end_block(void);
int get_uboot_start_block(void);
int get_uboot_next_block(void);
int get_physic_block_reserved(void);
void print_uboot_info(struct _uboot_info *uboot);
int uboot_info_init(struct _uboot_info *uboot);
int init_phyinfo_buf(void);
int get_uboot_offset(void *buf);
int is_uboot_magic(void *buf);
int check_magic_physic_info(unsigned int *mem_base, char *magic);
int check_sum_physic_info(unsigned int *mem_base, unsigned int size);
unsigned int cal_sum_physic_info(struct _boot_info *mem_base, unsigned int size);
int physic_info_get_offset(unsigned int *pages_offset);
int physic_info_get_one_copy(unsigned int start_block, unsigned int pages_offset, unsigned int *block_per_copy, unsigned int *buf);
int clean_physic_info(void);
int nand_physic_info_read(void);
int is_phyinfo_empty(struct _boot_info *info);
int add_len_to_uboot_tail(unsigned int uboot_size);
int print_physic_info(struct _boot_info *info);
int physic_info_add_to_uboot_tail(unsigned int *buf_dst, unsigned int uboot_size);
int mark_to_disable_crc_when_ota(void);
int disable_phyinfo_crc_in_buffer(struct _boot_info *phyinfo_buf_tmp);
extern unsigned int get_storage_type(void);

#endif /*SUNXI_NAND_BOOT_H*/
