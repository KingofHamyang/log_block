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


typedef unsigned int 	u32;


// function prototypes
int nand_init (int nblks, int npages);
int nand_read (int blk, int page, u32 *data, u32 *spare);
int nand_write (int blk, int page, u32 data, u32 spare);
int nand_erase (int blk);
int nand_blkdump (int blk);

