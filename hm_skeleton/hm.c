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

#include "hm.h"
#include "nand.h"

static void garbage_collection(void);

/***************************************
No restriction about return type and arguments in the following functions
***************************************/
static void switch_merge(int, int);
static void partial_merge(int, int, int);
static void full_merge(int, int);

int *L2B_data;
int *L2B_log;

int *invalid_count_per_block;
int *invalid_pages;
int *written_pages_per_block;
int *log_index;
int **log_PMT;

int spare_block = N_BLOCKS - 1;
int log_block_cnt = 0;

void ftl_open(void)
{
	L2B_data = (int *)malloc(sizeof(int) * N_USER_BLOCKS);
	L2B_log = (int *)malloc(sizeof(int) * N_USER_BLOCKS);

	invalid_count_per_block = (int *)malloc(sizeof(int) * N_BLOCKS);
	written_pages_per_block = (int *)malloc(sizeof(int) * N_BLOCKS);
	invalid_pages = (int *)malloc(sizeof(int) * N_PAGES_PER_BLOCK * N_BLOCKS);
	log_index = (int *)malloc(sizeof(int) * NUM_LOGBLOCK);

	log_PMT = (int **)malloc(sizeof(int *) * NUM_LOGBLOCK);
	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		log_PMT[i] = (int *)malloc(sizeof(int) * N_PAGES_PER_BLOCK);
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		L2B_data[i] = -1;
		L2B_log[i] = -1;
	}
	for (int i = 0; i < N_BLOCKS; i++)
	{
		written_pages_per_block[i] = 0;
		invalid_count_per_block[i] = 0;
	}
	for (int i = 0; i < N_BLOCKS * N_PAGES_PER_BLOCK; i++)
	{
		invalid_pages[i] = 0;
	}

	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		for (int j = 0; j < N_PAGES_PER_BLOCK; j++)
		{
			log_PMT[i][j] = -1;
		}
	}
	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		log_index[i] = -1;
	}
	nand_init(N_BLOCKS, N_PAGES_PER_BLOCK);

	return;
}

void ftl_read(long lpn, u32 *read_buffer)
{
	int block_n;
	int index;

	u32 spare;
	block_n = lpn / N_PAGES_PER_BLOCK;
	index = lpn % N_PAGES_PER_BLOCK;

	if (L2B_log[block_n] == -1)
	{
		//	printf("real read\n");
		if (nand_read(L2B_data[block_n], index, read_buffer, &spare) == -1)
		{
			printf("read error at real error");
		}
		return;
	}
	else
	{
		int P_Block = L2B_data[block_n];
		if (invalid_pages[P_Block * N_PAGES_PER_BLOCK + index] == 0)
		{
			//	printf("real read\n");
			if (nand_read(P_Block, index, read_buffer, &spare) == -1)
			{
				printf("read error at real error");
			}
			return;
		}
		else
		{
			int log_mem = 0;
			int log_block = L2B_log[block_n];
			for (int i = 0; i < NUM_LOGBLOCK; i++)
			{
				if (log_index[i] == log_block)
				{
					log_mem = i;
					break;
				}
			}
			//printf("real read\n");
			if (nand_read(log_block, log_PMT[log_mem][index], read_buffer, &spare) == -1)
			{
				printf("read error at real error");
			}
			return;
		}
	}
	return;
}

int get_free_block()
{

	for (int i = 0; i < N_BLOCKS; i++)
	{
		if (written_pages_per_block[i] == 0 && i != spare_block)
		{
			return i;
		}
	}
	return -1;
}
void log_block_merging(int data_block, int log_block, long lpn)
{
	if (invalid_count_per_block[log_block] == 0)
	{
		//	printf("switche merge	%d", written_pages_per_block[log_block]);
		int flag = 0;
		int log_mem;
		for (int i = 0; i < NUM_LOGBLOCK; i++)
		{
			if (log_index[i] == log_block)
			{
				log_mem = i;
				break;
			}
		}
		for (int i = 0; i < N_PAGES_PER_BLOCK - 1; i++)
		{
			if (log_PMT[log_mem][i] >= log_PMT[log_mem][i + 1])
			{
				flag = 1;
				break;
			}
		}
		if (flag == 0)
		{
			switch_merge(data_block, log_block);
			return;
		}
	}

	full_merge(data_block, log_block);
	return;
}

void ftl_write(long lpn, u32 *write_buffer)
{
	int block_n;
	int index;
	unsigned int lpn_to_write = lpn;
	block_n = lpn / N_PAGES_PER_BLOCK;
	index = lpn % N_PAGES_PER_BLOCK;
	if (L2B_data[block_n] == -1)
	{
		int new_data_block = get_free_block();
		if (new_data_block == -1)
		{
			//printf("there's no remain free block\n");
			garbage_collection();
			new_data_block = get_free_block();
		}
		L2B_data[block_n] = new_data_block;
		if (index == 0)
		{
			nand_write(new_data_block, index, *write_buffer, lpn_to_write);
			written_pages_per_block[new_data_block] = index + 1;
		}
		else
		{
			//	printf("asdf %d\n", block_n);
			for (int i = 0; i < index; i++)
			{
				nand_write(new_data_block, i, *write_buffer, lpn_to_write);
			}
			nand_write(new_data_block, index, *write_buffer, lpn_to_write);
			written_pages_per_block[new_data_block] = index + 1;
		}
	}
	else
	{

		int P_Block = L2B_data[block_n];
		if (written_pages_per_block[P_Block] == index)
		{

			nand_write(P_Block, index, *write_buffer, lpn_to_write);
			written_pages_per_block[P_Block]++;
		}
		else if (written_pages_per_block[P_Block] < index)
		{

			for (int i = written_pages_per_block[P_Block]; i < index; i++)
			{
				nand_write(P_Block, i, *write_buffer, lpn_to_write);
				written_pages_per_block[P_Block]++;
			}
			nand_write(P_Block, index, *write_buffer, lpn_to_write);
			written_pages_per_block[P_Block]++;
		}
		else
		{

			invalid_count_per_block[P_Block]++;
			invalid_pages[P_Block * N_PAGES_PER_BLOCK + index] = 1;

			int log_block = L2B_log[block_n];
			int log_mem = 0;

			if (log_block == -1)
			{
				log_block_cnt = 0;
				for (int i = 0; i < N_USER_BLOCKS; i++)
				{
					if (L2B_log[i] != -1)
						log_block_cnt++;
				}

				//	printf("asdf4 %d %d %d %d \n ", P_Block, index, log_block_cnt, log_block);
				if (log_block_cnt < NUM_LOGBLOCK)
				{

					log_block = get_free_block();
					if (log_block == -1)
					{
						//printf("No remain blocks for log \n");
						garbage_collection();
						log_block = get_free_block();
					}
					L2B_log[block_n] = log_block;
					for (int i = 0; i < NUM_LOGBLOCK; i++)
					{
						if (log_index[i] == -1)
						{
							log_index[i] = log_block;
							break;
						}
					}
				}
				else
				{
					//	printf("merging ");
					garbage_collection();
					log_block = get_free_block();
					L2B_log[block_n] = log_block;
					for (int i = 0; i < NUM_LOGBLOCK; i++)
					{
						if (log_index[i] == -1)
						{
							log_index[i] = log_block;
							break;
						}
					}
				}
			}
			else if (written_pages_per_block[log_block] == N_PAGES_PER_BLOCK)
			{
				//	printf("asdf******");
				log_block_merging(P_Block, log_block, lpn);
				log_block = get_free_block();
				L2B_log[block_n] = log_block;
				for (int i = 0; i < NUM_LOGBLOCK; i++)
				{
					if (log_index[i] == -1)
					{
						log_index[i] = log_block;
						break;
					}
				}
			}
			//	printf("gett %d\n", log_block);

			for (int i = 0; i < NUM_LOGBLOCK; i++)
			{
				if (log_index[i] == log_block)
				{
					log_mem = i;
					break;
				}
			}
			unsigned int temp = written_pages_per_block[log_block];

			nand_write(log_block, written_pages_per_block[log_block], *write_buffer, temp);
			if (log_PMT[log_mem][index] == -1)
			{
				log_PMT[log_mem][index] = written_pages_per_block[log_block];
			}
			else
			{
				invalid_count_per_block[log_block]++;
				invalid_pages[log_block * N_PAGES_PER_BLOCK + log_PMT[log_mem][index]] = 1;

				log_PMT[log_mem][index] = written_pages_per_block[log_block];
			}
			written_pages_per_block[log_block]++;
		}
	}
	return;
}

static void garbage_collection(void)
/***************************************
You can add some arguments and change
return type to anything you want
***************************************/
{
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		int data_block, log_block;
		data_block = L2B_data[i];
		log_block = L2B_log[i];
		if (log_block != -1 && data_block != -1)
		{
			//	printf("garbagecoolectas\n");
			if (invalid_count_per_block[log_block] == 0)
			{
				//		printf("switchmerged####\n");
				int flag = 0;
				int log_mem;
				for (int i = 0; i < NUM_LOGBLOCK; i++)
				{
					if (log_index[i] == log_block)
					{
						log_mem = i;
						break;
					}
				}
				for (int i = 0; i < N_PAGES_PER_BLOCK - 1; i++)
				{
					if (log_PMT[log_mem][i] >= log_PMT[log_mem][i + 1])
					{
						flag = 1;
						break;
					}
				}
				if (flag == 0)
				{
					//	printf("switchmerged\n");
					switch_merge(data_block, log_block);
					return;
				}
			}

			int flag = 0;
			int log_mem;
			for (int i = 0; i < NUM_LOGBLOCK; i++)
			{
				if (log_index[i] == log_block)
				{
					log_mem = i;
					break;
				}
			}
			for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
			{
				if (log_PMT[log_mem][i] == i)
				{
					flag++;
				}
				else
				{
					break;
				}
			}
			int flag2 = 0;
			for (int i = flag; i < written_pages_per_block[data_block]; i++)
			{
				if (invalid_pages[data_block * N_PAGES_PER_BLOCK + i] != 0)
				{
					flag2 = 1;

					break;
				}
			}
			if (flag2 == 0)
			{
				if (flag != 1)
					printf("partial!! %d %d %d %d %d\n", data_block, log_block, flag, written_pages_per_block[data_block], written_pages_per_block[log_block]);
				/*for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
				{
					printf("%d ", log_PMT[log_mem][i]);
				}*/
				//partial_merge(data_block, log_block, flag);
				full_merge(data_block, log_block);
				//assert(0);
				return;
			}
		}
	}
	s.gc++;

	/***************************************
Add



for every nand_write call (every valid page copy)
that you issue in this function
***************************************/

	return;
}
static void switch_merge(int data_block, int log_block)
{
	int log_mem;
	int origin_l2b_data;
	int origin_l2b_log;

	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		if (log_index[i] == log_block)
		{
			log_mem = i;
			break;
		}
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		if (L2B_data[i] == data_block)
		{
			origin_l2b_data = i;
			break;
		}
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		if (L2B_log[i] == log_block)
		{
			origin_l2b_log = i;
			break;
		}
	}

	nand_erase(data_block);

	written_pages_per_block[data_block] = 0;
	invalid_count_per_block[data_block] = 0;

	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		if (log_index[i] == log_block)
		{
			log_index[i] = -1;
			break;
		}
	}
	for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
	{
		log_PMT[log_mem][i] = -1;
	}

	L2B_data[origin_l2b_data] = log_block;
	L2B_log[origin_l2b_log] = -1;
}
static void partial_merge(int data_block, int log_block, int PMT_valid)
{
	int log_mem;
	int origin_l2b_data;
	int origin_l2b_log;

	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		if (log_index[i] == log_block)
		{
			log_mem = i;
			break;
		}
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		if (L2B_data[i] == data_block)
		{
			origin_l2b_data = i;
			break;
		}
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		if (L2B_log[i] == log_block)
		{
			origin_l2b_log = i;
			break;
		}
	}
	u32 data;
	u32 spare;
	for (int i = PMT_valid; i < written_pages_per_block[data_block]; i++)
	{

		if (nand_read(data_block, i, &data, &spare) == -1)
		{
			printf("read error at partial merge\n");
		}
		nand_write(log_block, i, data, spare);

		written_pages_per_block[log_block]++;
	}
	L2B_data[origin_l2b_data] = log_block;
	L2B_log[origin_l2b_log] = -1;
	nand_erase(data_block);

	written_pages_per_block[data_block] = 0;
	for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
	{
		log_PMT[log_mem][i] = -1;
	}

	for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
	{
		invalid_pages[data_block * N_PAGES_PER_BLOCK + i] = 0;
	}

	invalid_count_per_block[data_block] = 0;
	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		if (log_index[i] == log_block)
		{
			log_index[i] = -1;
			break;
		}
	}

	//TODO partial merger 다시짜기
	//valid copy 고르기. 정책 다시짜기
}

static void full_merge(int data_block, int log_block)
{
	int log_mem;
	int origin_l2b_data;
	int origin_l2b_log;

	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		if (log_index[i] == log_block)
		{
			log_mem = i;
			break;
		}
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		if (L2B_data[i] == data_block)
		{
			origin_l2b_data = i;
			break;
		}
	}
	for (int i = 0; i < N_USER_BLOCKS; i++)
	{
		if (L2B_log[i] == log_block)
		{
			origin_l2b_log = i;
			break;
		}
	}
	written_pages_per_block[spare_block] = 0;
	for (int i = 0; i < written_pages_per_block[data_block]; i++)
	{
		u32 data;
		u32 spare;
		if (invalid_pages[data_block * N_PAGES_PER_BLOCK + i] == 0)
		{

			if (-1 == nand_read(data_block, i, &data, &spare))
			{
				printf("merge read 2]\n");
			}
			nand_write(spare_block, i, data, spare);
			s.gc_write++;
		}
		else
		{
			invalid_pages[data_block * N_PAGES_PER_BLOCK + i] = 0;
			int page_to_read = log_PMT[log_mem][i];

			if (nand_read(log_block, page_to_read, &data, &spare))
			{
				printf("merge read1 %d  %d %d \n", i, written_pages_per_block[log_block], written_pages_per_block[data_block]);
			}
			nand_write(spare_block, i, data, spare);
			s.gc_write++;
		}
		written_pages_per_block[spare_block]++;
	}
	for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
	{
		invalid_pages[log_block * N_PAGES_PER_BLOCK + i] = 0;
		invalid_pages[data_block * N_PAGES_PER_BLOCK + i] = 0;
	}
	nand_erase(log_block);
	nand_erase(data_block);
	written_pages_per_block[log_block] = 0;
	written_pages_per_block[data_block] = 0;
	invalid_count_per_block[log_block] = 0;
	invalid_count_per_block[data_block] = 0;

	for (int i = 0; i < NUM_LOGBLOCK; i++)
	{
		if (log_index[i] == log_block)
		{
			log_index[i] = -1;
			break;
		}
	}
	for (int i = 0; i < N_PAGES_PER_BLOCK; i++)
	{
		log_PMT[log_mem][i] = -1;
	}

	L2B_data[origin_l2b_data] = spare_block;
	L2B_log[origin_l2b_log] = -1;

	spare_block = log_block;
}
