/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *	
 *	Tiny File System
 *
 *	File:	tfs.c
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
bitmap_t inodeBM; 
bitmap_t dataBM;

struct superblock *SB;
/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	// Step 2: Traverse inode bitmap to find an available slot
	// Step 3: Update inode bitmap and write to disk 

	printf("Getting next available inode\n");

	int i;
	int get = 0; 
	char buf[BLOCK_SIZE];
	/*Read from disk*/
	if(bio_read(1, buf) <= 0){
		perror("Error reading from disk in get_avail_ino\n");
		return -1;
	}
	/*Bring inode bitmap from disk into memory*/
	for(i = 0; i < MAX_INUM; i++){
		if(buf[i] == '1'){
			set_bitmap(inodeBM, i);
		}else{
			if(!get) get = i;
			unset_bitmap(inodeBM, i);
		}
	}
	/*Update inode bitmap*/
	set_bitmap(inodeBM, get);
	
	/*Write inode bitmap from memory to disk*/
	for(i = 0; i < MAX_INUM; i++){
		if(get_bitmap(inodeBM, i)){
			buf[i] = '1';
		}else buf[i] = '0';
	}
	if(bio_write(1, buf) < 0){
		perror("Error writing to disk in get_avail_ino\n");
		return -1;
	}

	return get;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	// Step 2: Traverse data block bitmap to find an available slot
	// Step 3: Update data block bitmap and write to disk 
	
	printf("Getting next available data block\n");

	int i;
	int get = 0; 
	char buf[BLOCK_SIZE];
	/*Read from disk*/
	if(bio_read(2, buf) <= 0){
		perror("Error reading from disk in get_avail_blkno\n");
		return -1;
	}
	/*Bring inode bitmap from disk into memory*/
	for(i = 0; i < MAX_DNUM; i++){
		if(buf[i] == '1'){
			set_bitmap(dataBM, i);
		}else{
			if(!get) get = i;
			unset_bitmap(dataBM, i);
		}
	}
	/*Update data bitmap*/
	set_bitmap(dataBM, get);
	
	/*Write data bitmap from memory to disk*/
	for(i = 0; i < MAX_DNUM; i++){
		if(get_bitmap(dataBM, i)){
			buf[i] = '1';
		}else buf[i] = '0';
	}
	if(bio_write(2, buf) < 0){
		perror("Error writing to disk in get_avail_blkno\n");
		return -1;
	}

	return get;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
  // Step 1: Get the inode's on-disk block number
  // Step 2: Get offset of the inode in the inode on-disk block
  // Step 3: Read the block from disk and then copy into inode structure


	printf("Reading inode %hd\n", ino);

	/*Get the inode block*/
	uint16_t block = (ino / 16) + 3;	

	/*Read in the block*/
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	if(bio_read(block, buf) <= 0){
		perror("Error in readi\n");
		return -1;
	}

	printf("Inode region: %s\n", buf);

	/*Parse the block, and extract desired inode*/
	char *tok = strtok(buf, "|");
	while(tok != NULL){
		int curInode;
		sscanf(tok, "%d", &curInode);
		printf("Current inode: %d\n", curInode);
		if(curInode == ino){
			/*Found inode, start copying*/
			printf("FOUND INODE:\t%s\n", tok);
			sscanf(tok, "%hd;%hd;%d;%d;%d;", &inode->ino, &inode->valid, &inode->size, &inode->type, &inode->link);
			char *tok2 = strtok(tok, ";");
			int count = 0;
			char dptrs[50];
			char idptrs[50];
			while(tok2 != NULL){
				if(count == 5){
					strcpy(dptrs, tok2);
				}
				else if(count == 6){
					strcpy(idptrs, tok2);
				}
				else if(count == 7){
					sscanf(tok2, "%ld", &inode->vstat.st_atime);
					sscanf(tok2, "%ld", &inode->vstat.st_mtime);
				}
				count++;
				tok2 = strtok(NULL, ";");
			}
			/*Get direct pointers*/
			char *dtok = strtok(dptrs, " \t\r\n\v\f");
			int i = 0;
			while(dtok != NULL){
				sscanf(dtok, "%d", &inode->direct_ptr[i]);
				i++;
				dtok = strtok(NULL, " \t\r\n\v\f");
			}
			i = 0;
			/*Get indirect pointers*/
			char *idtok = strtok(idptrs, " \t\r\n\v\f");
			while(idtok != NULL){
				sscanf(idtok, "%d", &inode->indirect_ptr[i]);
				i++;
				idtok = strtok(NULL, " \t\r\n\v\f");
			}
			return 0;
		}
		tok = strtok(NULL, "|");
	}

	/*Inode not found*/
	perror("Couldnt locate inode\n");	
	return -1;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	// Step 2: Get the offset in the block where this inode resides on disk
	// Step 3: Write inode to disk 

	printf("Writing inode %d\n", ino);

	/*Get the inode block*/
	uint16_t block = (ino / 16) + 3;	

	char buf[256];
	
	sprintf(buf, "%d;%d;%d;%d;%d;", inode->ino, inode->valid, inode->size, inode->type, inode->link);
	int i;
	for(i = 0; i < 16; i++){
		char dirptr[10];
		sprintf(dirptr, "%d ", inode->direct_ptr[i]);
		strcat(buf, dirptr);
	}
	for(i = 0; i < 8; i++){
		char indirptr[10];
		sprintf(indirptr, "%d ", inode->indirect_ptr[i]);
		strcat(buf, indirptr);
	}

	char stat[50];
	sprintf(stat, "%d;%d|", (int)inode->vstat.st_atime, (int)inode->vstat.st_mtime);
	strcat(buf, stat);

	printf("Final write: %s\n", buf);

	/*Write inode to disk*/
	if(bio_write(block, buf) < 0){
		perror("Error writing inode in writei\n");
		return -1;
	}

	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)
  // Step 2: Get data block of current directory from inode
  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

	printf("----RUNNING dir_find()-----\n");
	
	/*Call readi and get inode info of current directory*/
	struct inode *inode = malloc(sizeof(struct inode));
	if(readi(ino, inode) < 0){
		perror("Error reading inode in dir_find\n");
		free(inode);
		return -1;
	}
	
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	int i;
	for(i = 0; i < 16; i++){
		/*Scan each data block*/
		if(inode->direct_ptr[i] != -1){
			if(bio_read(inode->direct_ptr[i], buf) <= 0){
				perror("Error reading block in dir_find\n");
				return -1;
			}
			//TODO Check if buf is empty
			printf("DATA BLOCK FOR INODE %d: %s\n", ino, buf);
			/*Search for fname in data block*/
			if(strstr(buf, fname)){ //fname can still be a subset of the found name
				/*Copy data into dirent*/
				/*inode data is stored as such => 'fname': ino*/
				char name[256];
				int inum = 0;
				sscanf(buf, "%s: %d", name, &inum);
				/*Double check to ensure file names are exact matches*/
				if(strcmp(name, fname) == 0){
					dirent->ino = inum;
					dirent->valid = 1;
					strncpy(dirent->name, fname, name_len);
				
					free(inode);
					printf("Successfully completed dir_find()\n");
					return 0;
				}
			}
			memset(buf, 0, BLOCK_SIZE);
		}
	}
	
	/*Not found*/
	free(inode);
	printf("Empty directory\n");
	return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	// Step 2: Check if fname (directory name) is already used in other entries
	// Step 3: Add directory entry in dir_inode's data block and write to disk
	// Allocate a new data block for this directory if it does not exist
	// Update directory inode
	// Write directory entry

	printf("----RUNNING dir_add-----\n");	
	
	/*Call readi and get inode info*/
	struct inode *inode = malloc(sizeof(struct inode));
	if(readi(dir_inode.ino, inode) < 0){
		perror("Error reading inode in dir_add\n");
		return -1;
	}
	
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	int i, avail = 0;
	for(i = 0; i < 16; i++){
		/*Scan each data block*/
		if(inode->direct_ptr[i] != -1){
			if(bio_read(inode->direct_ptr[i], buf) <= 0){
				perror("Error reading block in dir_add\n");

				free(inode);
				return -1;
			}
			/*Search for fname in data block*/
			if(strstr(buf, fname)){
				char name[256];
				int ino;
				sscanf(buf, "%s: %d", name, &ino);
				if(strcmp(name, fname) == 0){
					perror("Directory already exists\n");
	
					free(inode);
					return -1;
				}
			}
			memset(buf, 0, BLOCK_SIZE);
		}else avail = i;
	}
	memset(buf, 0, BLOCK_SIZE);
	/*Create data block contents*/
	sprintf(buf, "%s: %hd", fname, f_ino);
	if(avail != -1){
		/*Find next data block to put new inode info*/
		int nextDB = get_avail_blkno();
		if(nextDB == -1){
			perror("No available blocks\n");
			free(inode);
			return -1;
		}
		inode->direct_ptr[avail] = nextDB;

		/*Update current inode*/
		if(writei(inode->ino, inode) < 0){
			perror("Error writing inode in dir_add\n");
			free(inode);
			return -1;
		}
		/*Write to new data block*/
		if(bio_write(nextDB, buf) < 0){
			perror("Error writing block in dir_add\n");
			free(inode);
			return -1;
		}
		
	}else{
		perror("No available pointers\n");
		free(inode);
		return -1;
	}

	printf("----FINISHED dir_add-----\n");
	return -1;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	// Step 2: Check if fname exist
	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	printf("-----RUNNING dir_remove-----\n");

	struct inode *inode  = malloc(sizeof(struct inode));
	if(readi(dir_inode.ino, inode) <= 0){
		perror("Error reading inode in dir_remove\n");
		return -1;
	}
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	int i;
	for(i = 0; i < 16; i++){
		/*Scan each data block*/
		if(inode->direct_ptr[i] != -1){
			if(bio_read(inode->direct_ptr[i], buf) <= 0){
				perror("Error reading block in dir_add\n");

				free(inode);
				return -1;
			}
			/*Search for fname in data block*/
			if(strstr(buf, fname)){
				char name[256];
				int ino;
				sscanf(buf, "%s: %d", name, &ino);
				if(strcmp(name, fname) == 0){
					/*Free up spot in data bitmap*/
					unset_bitmap(dataBM, inode->direct_ptr[i]);
					/*Invalidate pointer to that block*/
					inode->direct_ptr[i] = -1;
					/*Write inode to disk*/
					if(writei(inode->ino, inode) < 0){
						perror("Error writing to inode in dir_remove\n");
						free(inode);
						return -1;
					}
					/*Don't have to remove contents in data block, will be overwritten by future data*/	
					free(inode);
					return 0;
				}
			}
			memset(buf, 0, BLOCK_SIZE);
		}
	}

	/*Not found*/
	perror("Couldn't find block\n");
	free(inode);

	printf("----FINISHED dir_remove------\n");
	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	printf("-----RUNNNING get_node_by_path-----\n");	
	
	printf("Getting node of %s\n", path);
	size_t totalLength = strlen(path);

	/*Start at root*/
	struct dirent *root = malloc(sizeof(struct dirent));
	if(dir_find(0, "/", 1, root) < 0){
		perror("Couldnt find the directory from root\n");
		free(root);
		return -1;
	}

	/*Find the next directory*/
	struct dirent *cur = root;	
	int i, start = 0, end = 0 ;
	for(i = 1; i <= totalLength; i++){
		if(path[i] == '/' || i == totalLength){
			end = i;
			strncpy(cur->name, path+start+1, end-start-1);
			if(dir_find(cur->ino, cur->name, strlen(cur->name), cur) < 0){
				perror("Couldnt find the directory\n");
				free(root);
				return -1;
			} 
			start = end;
		}
	}		
		
	free(root);
	
	/*Copy into inode*/
	if(readi(cur->ino, inode) < 0){
		perror("Couldn't read node in get_node_by_path\n");
		return -1;
	}

	printf("-----FINISHED RUNNING get_node_by_path--------\n");
	return cur->ino;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	// write superblock information
	// initialize inode bitmap
	// initialize data block bitmap
	// update bitmap information for root directory
	// update inode for root directory

	printf("---Running tfs_mkfs()---\n");

	/*Create diskfile*/
	dev_init(diskfile_path);
	
	printf("Done running dev_init()\n");


	printf("Attempting to open new diskfile\n");
	/*Open diskfile*/	
	if(dev_open(diskfile_path) < 0){
		perror("Error opening diskfile\n");
		return -1;
	}
		

	printf("Initializing bitmaps...\n");

	/*Initialize bitmaps*/
	inodeBM = malloc(MAX_INUM);
	dataBM = malloc(MAX_DNUM);
	memset(inodeBM, 0, MAX_INUM);
	memset(dataBM, 0, MAX_DNUM);

	printf("Updating bitmap and root inode\n");

	/*Update inode bitmap for root directory*/
	set_bitmap(inodeBM, 0);

	/*Update inode for root directory*/
	struct inode *root = malloc(sizeof(struct inode));
	root->ino = 0;
	root->valid = 1;
	root->size = 256;
	root->type = 0; //0 for directory 1 for files
	root->link = 0;
	int i;
	for(i = 0; i < 16; i++){
		root->direct_ptr[i] = -1;
	}
	for(i = 0; i < 8; i++){
		root->indirect_ptr[i] = -1;
	}
	time(&root->vstat.st_atime);
	time(&root->vstat.st_mtime);

	/* Write superblock info to diskfile
	   Block 0 is for the superblock
 	   Block 1 is for the inode bitmap
	   Block 2 is for the data bitmap
	   Blocks 3-66 are for the inode blocks
	   Blocks 67+ are for the data blocks
	*/
	
	if(!SB) SB = malloc(sizeof(struct superblock));

	printf("Writing superblock...\n");
	/*Write superblock*/
	char sb[BLOCK_SIZE];
	sprintf(sb, "%d;%hd;%hd;%d;%d;%d;%d|", MAGIC_NUM, MAX_INUM, MAX_DNUM, 1, 2, 6, 70);
	if(bio_write(0, sb) < 0){
		perror("Error writing superblock\n");
		return -1;
	}
	/*Write inode bitmap*/
	char ibm[MAX_INUM];
	memset(ibm, 0, MAX_INUM);
	ibm[0] = '1'; // for root 
	if(bio_write(1, ibm) < 0){
		perror("Error writing inode bitmap\n");
		return -1;
	}
	/*Write data bitmap*/
	char dbm[MAX_DNUM];
	memset(dbm, 0, MAX_DNUM);
	if(bio_write(2, dbm) < 0){
		perror("Error writing data bitmap\n");
		return -1;
	}
	/*Write root inode*/
	if(writei(0, root) < 0){
		perror("Error writing root inode\n");
		return -1;
	}	
	printf("----FINISHED tfs_mkfs()----\n");

	free(root);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs
  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk

	int ret = dev_open(diskfile_path);
	if(ret == -1){
		printf("Creating filesystem\n");
		if(tfs_mkfs() < 0){
			perror("Error making diskfile\n");	
			exit(0);
		}
	}else{
		printf("Diskfile already exists\n");
		printf("Initializing Data Structures\n");
	
		printf("Initializing bitmaps...\n");
	
		/*Initialize bitmaps*/
		inodeBM = malloc(MAX_INUM);
		dataBM = malloc(MAX_DNUM);
		memset(inodeBM, 0, MAX_INUM);
		memset(dataBM, 0, MAX_DNUM);
	
		printf("Reading superblock from disk\n");		

		/*Read superblock*/
		char buf[BLOCK_SIZE];
		memset(buf, 0, BLOCK_SIZE);
		if(bio_read(0, buf) <= 0){
			perror("Error reading superblock in tfs_init\n");
			exit(0);
		}
	
		if(!SB) SB = malloc(sizeof(struct superblock));
	
		printf("BUFFER: %s\n", buf); 
		sscanf(buf, "%d;%hd;%hd;%d;%d;%d;%d", &SB->magic_num, &SB->max_inum, &SB->max_dnum, &SB->i_bitmap_blk, &SB->d_bitmap_blk, &SB->i_start_blk, &SB->d_start_blk);
		if(SB->magic_num != MAGIC_NUM){
			perror("This is not the correct superblock\n");
			exit(0);
		}
	
		printf("Reading bitmaps...\n");
	
		/*Read bitmaps*/
		memset(buf, 0, BLOCK_SIZE);
		if(bio_read(1, buf) <= 0){
			perror("Error reading inode bitmap in tfs_init\n");
			exit(0);
		}
		printf("Reading inode bitmap...\n");
		int i;
		for(i = 0; i < MAX_INUM; i++){
			if(buf[i] == '1') set_bitmap(inodeBM, i);
			else unset_bitmap(inodeBM, i);
		}
		printf("Done reading inode bitmap\n");

		/*Since MAX_DNUM = 16384, we need to store it across 4 blocks*/
		for(i = 2; i < 6; i++){
			memset(buf, 0, BLOCK_SIZE);
			if(bio_read(i, buf) <= 0){
				perror("Error reading data bitmap in tfs_init\n");
				exit(0);
			}
			int j;
			for(j = 0; j < BLOCK_SIZE; j++){
				if(buf[j] == '1') set_bitmap(dataBM, j + i * 4096);
				else unset_bitmap(dataBM, j + i * 4096);
			}
		}
	}
		
	printf("Finished initializing tfs\n");

	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	// Step 2: Close diskfile
	
	printf("\n---RUNNING tfs_destroy()---\n");	

	free(inodeBM);
	free(dataBM);
	free(SB);
	dev_close();
	printf("----FINISHED tfs_destroy----\n");
}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path
	// Step 2: fill attribute of file into stbuf from inode

	printf("---RUNNING tfs_getattr()---\n");

	printf("Looking for attributes of %s\n", path);

	struct inode *inode = malloc(sizeof(struct inode));	
	int ino = get_node_by_path(path, 0, inode); 
	if(ino < 0){
		perror("Couldn't get node in tfs_getattr()\n");
		free(inode);
		return -ENOENT;
	}

	stbuf->st_ino = ino;
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	stbuf->st_size = inode->size;
	stbuf->st_atime = time(NULL);
	stbuf->st_mtime = time(NULL);

	if(strcmp(path, "/") == 0){
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}else{
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
	}

	printf("---FINISHED RUNNING tfs_getattr()---\n");
	free(inode);
	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	// Step 2: If not find, return -1

	printf("---RUNNING tfs_opendir()---\n");

	/*Check if path is valid*/
	struct inode *inode = malloc(sizeof(struct inode));
	int ino = get_node_by_path(path, 0, inode);
	if(ino < 0){
		perror("Couldn't get node in tfs_opendir()\n");
		free(inode);
		return -ENOENT;
	}
	if(readi(ino, inode) < 0){
		perror("Error readin ginode in tfs_opendir()\n");
		free(inode);	
		return -1;
	}

	/*Path is valid, change to directory*/
	if(chdir(path) < 0){
		perror("Error changing directories\n");
		return -1;
	}
	
	printf("----FINISHED RUNNING tfs_opendir()----\n");
	free(inode);
    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	// Step 2: Read directory entries from its data blocks, and copy them to filler

	printf("---RUNNING tfs_readdir()---\n");
	
	printf("READING path %s\n", path);
	/*Get inode and read it*/
	struct inode *inode = malloc(sizeof(struct inode));
	int ino = get_node_by_path(path, 0, inode);
	if(ino < 0){
		perror("Error getting inode in tfs_readdir\n");
		free(inode);
		return -ENOENT;
	}
	if(readi(ino, inode) < 0){
		perror("Error reading inode in tfs_readdir\n");
		return -1;
	}

	/*Fill in current directory and parent directory*/
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	int i;
	if(strcmp(path, "/") == 0){
		/*Read the dir entries in root directory*/		
		for(i = 0; i < 16; i++){
			if(inode->direct_ptr[i] != -1){
				if(bio_read(inode->direct_ptr[i], buf) <= 0){
					perror("Error reading block in tfs_readdir\n");
					return -1;
				}
				char name[256];
				memset(name, 0, 256);
				sprintf(buf, "%s ", name);
				filler(buffer, buf, NULL, 0); 
			}
		}
	}else{
		/*Read the dir entries in the path*/		
		filler(buffer, "ducks", NULL, 0);
	}

	printf("---FINISHED tfs_readdir()---\n");
	free(inode);
	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	// Step 2: Call get_node_by_path() to get inode of parent directory
	// Step 3: Call get_avail_ino() to get an available inode number
	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	// Step 5: Update inode for target directory
	// Step 6: Call writei() to write inode to disk

	printf("---RUNNNING tfs_mkdir()---\n");

	/*Extract dirname and basename*/	
	char dirname[PATH_MAX];
	char basename[256];

	int i;
	for(i = strlen(path)-1; i >=0; i--){
		if(path[i] == '/' && i != strlen(path)-1){
			strncpy(dirname, path, i+1);
			strncpy(basename, path+i+i, strlen(path)-i);
			strcat(dirname, "\0");
			strcat(basename, "\0");
			break;
		}
	}	
	
	/*Look for inode*/
	struct inode inode;
	int ino = get_node_by_path(path, 0, &inode);
	if(ino < 0){
		perror("Error getting node in tfs_mkdir\n");
		return -ENOENT;
	}
	
	/*Get available inode*/
//	int avail = get_avail_ino();
	
	/*Add directory*/
//	if(dir_add(inode, ) < 0){
//		perror("Error adding directory\n");
//		return -1;
//	}


	printf("---FINISHED tfs_mkdir()---\n");
	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	// Step 2: Call get_node_by_path() to get inode of target directory
	// Step 3: Clear data block bitmap of target directory
	// Step 4: Clear inode bitmap and its data block
	// Step 5: Call get_node_by_path() to get inode of parent directory
	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	printf("---RUNNING tfs_rmdir()---\n");


	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	// Step 2: Call get_node_by_path() to get inode of parent directory
	// Step 3: Call get_avail_ino() to get an available inode number
	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	// Step 5: Update inode for target file
	// Step 6: Call writei() to write inode to disk

	printf("---RUNNING tfs_create()---\n");

	/*Extract dirname and basename*/
	char dirname[PATH_MAX];
	char basename[256];

	int i;
	for(i = strlen(path)-1; i >=0; i--){
		if(path[i] == '/' && i != strlen(path)-1){
			strncpy(dirname, path, i+1);
			strncpy(basename, path+i+i, strlen(path)-i);
			strcat(dirname, "\0");
			strcat(basename, "\0");
			break;
		}
	}	


	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	// Step 2: If not find, return -1

	printf("---RUNNING tfs_open()---\n");

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path
	// Step 2: Based on size and offset, read its data blocks from disk
	// Step 3: copy the correct amount of data from offset to buffer
	// Note: this function should return the amount of bytes you copied to buffer

	printf("---RUNNING tfs_read()---\n");

	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	// Step 2: Based on size and offset, read its data blocks from disk
	// Step 3: Write the correct amount of data from offset to disk
	// Step 4: Update the inode info and write it to disk
	// Note: this function should return the amount of bytes you write to disk

	printf("---RUNNING tfs_write()---\n");


	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	// Step 2: Call get_node_by_path() to get inode of target file
	// Step 3: Clear data block bitmap of target file
	// Step 4: Clear inode bitmap and its data block
	// Step 5: Call get_node_by_path() to get inode of parent directory
	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	printf("---RUNNING tfs_unlink()---\n");

	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	printf("Current path: %s\n", diskfile_path);

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}

