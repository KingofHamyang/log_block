//----------------------------------------------------------
//
// Project #4 : Log block FTL
// 	- Embedded Systems Design, ICE3028 (Fall, 2018)
//
// November. 22, 2018.
//
// Min-Woo Ahn, Dong-Hyun Kim (minwoo.ahn@csl.skku.edu, donghyun.kim@csl.skku.edu)
// Dong-Yun Lee (dongyun.lee@csl.skku.edu)
// Jin-Soo Kim (jinsookim@skku.edu)
// Computer Systems Laboratory
// Sungkyunkwan University
// http://csl.skku.edu/ICE3028S17
//
//---------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define SSD_SHIFT 32
#define PAGE_SHIFT 12
#define PAGES_PER_BLOCK_SHIFT 7
#define N_RUNS 10
#define NUM_LOGBLOCK 3

#define PPN_SHIFT (SSD_SHIFT - PAGE_SHIFT)
#define BLOCKS_SHIFT (PPN_SHIFT - PAGES_PER_BLOCK_SHIFT)
#define N_PAGE_SIZE (1 << PAGE_SHIFT)
#define N_PAGES_PER_BLOCK (1 << PAGES_PER_BLOCK_SHIFT)
#define N_PPNS (1 << PPN_SHIFT)
#define N_BLOCKS (1 << BLOCKS_SHIFT)
#define N_USER_BLOCKS (N_BLOCKS - (NUM_LOGBLOCK + 1))
#define N_LPNS (N_USER_BLOCKS * N_PAGES_PER_BLOCK)
#define LPN_COUNTS (N_LPNS * N_RUNS)

#ifdef MULTI_HOT_COLD

#define HOT_RATIO 60
#define WARM_RATIO 30
#define COLD_RATIO 10

#define HOT_LPN_RATIO 10
#define WARM_LPN_RATIO 30
#define COLD_LPN_RATIO 60

#define HOT_LPN ((N_LPNS * HOT_LPN_RATIO) / 100)
#define WARM_LPN ((N_LPNS * WARM_LPN_RATIO) / 100)
#define COLD_LPN ((N_LPNS * COLD_LPN_RATIO) / 100)

#elif HOT_COLD

#define HOT_RATIO 80
#define COLD_RATIO 20

#define HOT_LPN_RATIO 20
#define COLD_LPN_RATIO 80

#define HOT_LPN ((N_LPNS * HOT_LPN_RATIO) / 100)
#define COLD_LPN ((N_LPNS * COLD_LPN_RATIO) / 100)

#endif

typedef unsigned int u32;

struct hm_stat
{
	int gc;
	long host_write;
	long gc_write;
} s;

void ftl_open(void);
void ftl_read(long lpn, u32 *data);
void ftl_write(long lpn, u32 *data);
