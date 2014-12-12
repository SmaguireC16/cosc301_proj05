//Sean Maguire
//& the internet
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"

void print_indent(int indent) { 
    for (int i = 0; i < indent*4; i++) 
	printf(" "); 
 } 

int cmp(uint16_t nclus, struct direntry *dirent, uint8_t * image_buf, struct bpb33 * bpb){
	uint32_t size=getulong(dirent->deFileSize);
	uint32_t clustersize= bpb->bpbSecPerClust*bpb->bpbBytesPerSec;
	int expected_num_clus =(size+511)/clustersize;
	uint16_t fatent = get_fat_entry(nclus, image_buf, bpb);
	uint16_t pfatent = fatent;
	int count = 1;
	while(!is_end_of_file(fatent)) { 
 		if (count==expected_num_clus) { 
 			set_fat_entry(pfatent,FAT12_MASK&CLUST_EOFS, image_buf, bpb); 
 			set_fat_entry(fatent, FAT12_MASK&CLUST_FREE, image_buf, bpb); 
 			fatent=get_fat_entry(fatent, image_buf, bpb); 
 			count++; 
 			} 
 		else if (count> expected_num_clus) { 
 			set_fat_entry(fatent, FAT12_MASK&CLUST_FREE, image_buf, bpb); 
 			fatent=get_fat_entry(fatent, image_buf, bpb); 
 			count++;
 
			} 
 		else { 
 			pfatent=fatent; 
 			fatent=get_fat_entry(fatent,image_buf, bpb); 
 			count++; 
 			} 
 			 
 		} 
 	if (expected_num_clus>count){ 
 		uint32_t clustersize = bpb->bpbSecPerClust*bpb->bpbBytesPerSec; 
 		putulong(dirent->deFileSize, (count*clustersize)); 
 		printf( "%d", getulong(dirent->deFileSize)); 
	} 
	return count;  
} 


void n_dir(uint16_t cluster, int indent, uint8_t *image_buf, struct bpb33* bpb){
	while (is_valid_cluster(cluster, bpb)){
		struct direntry * dirent = (struct direntry*)cluster_to_addr(cluster,image_buf, bpb);
	int numDirEntries = (bpb->bpbBytesPerSec * bpb-> bpbSecPerClust) / sizeof(struct direntry);
	for (int i=0; i < numDirEntries; i++){
		uint16_t nclus = getfollowclust(dirent, indent);
		if (getushort(dirent->deStartCluster) != 0){
			int count =cmp(nclus, dirent, image_buf, bpb);
			printf("%i", count);
		}
		if (nclus) n_dir(nclus, indent+1, image_buf, bpb);
		dirent++;
	}
	cluster = get_fat_entry(cluster, image_buf, bpb);
	}
}


uint16_t print_dirent(struct direntry* dirent, int indent){
	uint16_t followclust=0;
	
	int i;
	char name[9];
	char extension[4];
	uint32_t size;
	uint64_t file_cluster;
	name[8]= ' ';
	extension [3] = ' ';
	memcpy(name, &(dirent->deName[0]), 8);
	memcpy(extension, dirent->deExtension, 3);
	if (name[0] == SLOT_EMPTY) return followclust; //empty
	if (((uint8_t)name[0]) == SLOT_DELETED) return followclust; // deleted
	if (((uint8_t)name[0]) == 0x2E) return followclust; //dot entry
     		for (i = 8; i > 0; i--){  
			if (name[i] == ' ')  
	    		name[i] = '\0'; 
 		else break; 
     } 
  	 for (i = 3; i > 0; i--){ 
 		if (extension[i] == ' ') extension[i] = '\0'; 
		else break; 
		if ((dirent->deAttributes & ATTR_WIN95LFN) == ATTR_WIN95LFN) printf("ignore\n"); 
    		else if ((dirent->deAttributes & ATTR_VOLUME) != 0)printf("Volume: %s\n", name); 
		else if ((dirent->deAttributes & ATTR_DIRECTORY) != 0){ 
		if ((dirent->deAttributes & ATTR_HIDDEN) != ATTR_HIDDEN){	     
          		file_cluster = getushort(dirent->deStartCluster); 
          		followclust = file_cluster; 
			
	}	
 	else{ 
		//normal entry
		} 
    } 
}

	
	return followclust;
}
void usage(char *progname) {
    fprintf(stderr, "usage: %s <imagename>\n", progname);
    exit(1);
}


int main(int argc, char** argv) {
    uint8_t *image_buf;
    int fd;
    struct bpb33* bpb;
    if (argc < 2) {
	usage(argv[0]);
    }

    image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);
	
    // your code should start here...
	uint16_t clus=0;
	struct direntry *dirent = (struct direntry*)cluster_to_addr(clus, image_buf, bpb);
	for (int i=0; i < bpb->bpbRootDirEnts; i++){
		uint16_t nclus = print_dirent(dirent, 0);
		if (is_valid_cluster(nclus, bpb)) n_dir(nclus, 1, image_buf, bpb);
		dirent++;
	}

    unmmap_file(image_buf, &fd);
    return 0;
}
