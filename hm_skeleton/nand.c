//----------------------------------------------------------
//
// A simple NAND simulator
//
// Aug. 28, 2016
//
// Jin-Soo Kim (jinsookim@skku.edu)
// Computer Systems Laboratory
// Sungkyunkwan University
// http://csl.skku.edu
//
//----------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand.h"

struct nand_page
{
	u32 	data;
	u32 	spare;
};

struct nand_page *nand;
int *page_to_write;
int _nblks;
int _npages;

#define		PAGE(b,p)		((b) * _npages + (p))

#define 	BLK_FREE(b)		(page_to_write[b] == 0)
#define		BLK_WRITTEN(b)	(page_to_write[b] == _npages)

#define check_blk_page(blk, page, s)                                    		\
    if (blk < 0 || blk >= _nblks)                                       		\
    {                                                                   		\
        printf ("%s(%d,%d): failed, invalid block number\n", s, blk, page);     \
        return -1;                                                      		\
    }                                                                   		\
    if (page < 0 || page >= _npages)		                            		\
    {                                                                   		\
        printf ("%s(%d,%d): failed, invalid page number\n", s, blk, page);      \
        return -1;                                                      		\
    }

#define check_blk(blk, s)                                               		\
    if (blk < 0 || blk >= _nblks)                                       		\
    {                                                                   		\
        printf ("%s(%d): failed, invalid block number\n", s, blk);              \
        return -1;                                                      		\
    }


int nand_init (int nblks, int npages)
{
	// initialize the NAND flash memory 
	// "blks": the total number of flash blocks
	// "npages": the number of pages per block
	// returns 0 on success
	// returns -1 on errors with printing appropriate error message

	if (nblks <= 0)
	{
		printf ("init(%d,%d): failed, invalid number of blocks\n", nblks, npages);
		return -1;
	}
	if (npages <= 0)
	{
		printf ("init(%d,%d): failed, invalid number of pages\n", nblks, npages);
		return -1;
	}

	nand = (struct nand_page *) calloc (sizeof(struct nand_page), nblks * npages);
	page_to_write = (int *) calloc (sizeof(int), nblks);
	if (nand == NULL || page_to_write == NULL)
	{
		printf ("init(%d,%d): unable to allocate memory\n", nblks, npages);
		 return -1;
	}

	_nblks = nblks;
	_npages = npages;

//	printf ("NAND: %d blocks, %d pages per block, %d pages\n",
//			nblks, npages, nblks * npages);
	return 0;
}
	
int nand_write (int blk, int page, u32 data, u32 spare)
{
	// write "data" and "spare" into the NAND flash memory pointed to by "blk" and "page"
	// returns 0 on success
	// returns -1 on errors with printing appropriate error message

	check_blk_page (blk, page, "write");
	if (page < page_to_write[blk])
	{
		printf ("write(%d,%d): failed, the page was already written\n", blk, page);
		return -1;
	}

	if (page != page_to_write[blk])
	{
		printf ("write(%d,%d): failed, the page is not being sequentially written\n", blk, page);
		return -1;
	}

	nand[PAGE(blk,page)].data = data;
	nand[PAGE(blk,page)].spare = spare;
	page_to_write[blk]++;

//	printf ("write(%d,%d): data = 0x%08x, spare = 0x%08x\n", blk, page, data, spare);

	return 0;
}


int nand_read (int blk, int page, u32 *data, u32 *spare)
{
	// read "data" and "spare" from the NAND flash memory pointed to by "blk" and "page"
	// returns 0 on success
	// returns -1 on errors with printing appropriate error message

	check_blk_page (blk, page, "read");

	if (page >= page_to_write[blk])
	{
		printf ("read(%d,%d): failed, trying to read an empty page\n", blk, page);
		return -1;
	}

	*data = nand[PAGE(blk,page)].data;
	*spare = nand[PAGE(blk,page)].spare;
	//printf ("read(%d,%d): data = 0x%08x, spare = 0x%08x\n", blk, page, *data, *spare);

	return 0;
}

int nand_erase (int blk)
{
	// erase the NAND flash memory block "blk"
	// returns 0 on success
	// returns -1 on errors with printing appropriate error message

	check_blk (blk, "erase");

	if (BLK_FREE(blk))
	{
		printf ("erase(%d): failed, trying to erase a free block\n", blk);
		return -1;
	}

	bzero ((void *) &nand[PAGE(blk,0)], sizeof(struct nand_page) * _npages);
	page_to_write[blk] = 0;
//	printf ("erase(%d): block erased\n", blk);

	return 0;
}


int nand_blkdump (int blk)
{
	// dump the contents of the NAND flash memory block [blk] (for debugging purpose)
	// returns 0 on success
	// returns -1 on errors with printing appropriate error message
	
	int i;
	
	check_blk (blk, "blkdump");

	if (BLK_FREE(blk))
		printf ("Blk %5d: FREE\n", blk);
	else
	{
		printf ("Blk %5d: Total %d page(s) written\n", blk, page_to_write[blk]);
		for (i = 0; i < page_to_write[blk]; i++)
			printf ("    Page %3d: data = 0x%08x, spare = 0x%08x\n",
					i, nand[PAGE(blk, i)].data, nand[PAGE(blk, i)].spare);
	}

	return 0;
}

