#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_PATH 256
/* make sure to use syserror() when a system call fails. see common.h */
void
usage()
{
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

void copy_file(const char* src_pathname, const char* dest_pathname){
    int fd1, flags;
    flags = O_RDONLY;
    fd1 = open(src_pathname, flags);
    if(fd1 < 0){
        syserror(open, src_pathname);
        return;
    }
    
    int ret;
    
    struct stat* src_info = (struct stat*)malloc(sizeof(struct stat));
    ret = stat(src_pathname, src_info);
    if(ret < 0){
        syserror(stat, src_pathname);
        ret = close(fd1);
        return;
    }
    
    mode_t mode = src_info->st_mode;
    size_t size = src_info->st_size;
    free(src_info);
    char buf[size];
    int fd2;
    fd2 = creat(dest_pathname, mode);
    if(fd2 < 0){
        syserror(creat, dest_pathname);
        ret = close(fd1);
        return;
    }
    
    /*ret = close(fd2);
    if(ret<0){
        syserror(close, dest_pathname);
    }
    
    fd2 = open(dest_pathname, O_WRONLY | O_APPEND | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd2 < 0){
        syserror(open, dest_pathname);
    }*/
    
    int ret2;
    
    ret = read(fd1, buf, size);
    while(ret > 0){
        ret2= write(fd2, buf, size);
        if(ret2 < 0){
            syserror(write, dest_pathname);
            ret = close(fd1);
            ret = close(fd2);
            return;
        }
        ret = read(fd1, buf, size);
        if(ret < 0){
            syserror(read, src_pathname);
            ret = close(fd1);
            ret = close(fd2);
            return;
        }
    }
    chmod(dest_pathname, mode);
    
    ret = close(fd1);
    ret2 = close(fd2);
    
    //ret = read(fd1, buf, 4096);
    return;
}

void copy_dir(const char* src_pathname, const char* dest_pathname){
    DIR *src = opendir(src_pathname);
    if(src == NULL){
        syserror(opendir, src_pathname);
    }
    
    DIR *dest = opendir(dest_pathname);
    if(dest == NULL){
        syserror(opendir, dest_pathname);
    }
    
    struct dirent* src_dirent;
    
   char full_dest_pathname[MAX_PATH];
   realpath(dest_pathname, full_dest_pathname);
   
   char full_src_pathname[MAX_PATH];
   realpath(src_pathname, full_src_pathname);
   //printf("%s\n", full_dest_pathname);
    while((src_dirent=readdir(src)) != NULL){
        int parents = 1;
        for(int i = 0; i < strlen(src_dirent->d_name); i++){
            if(src_dirent->d_name[i] != '.'){
                parents = 0;
            }
        }
        if(parents != 1){
            /*char full_src_pathname[MAX_PATH];
            realpath(src_dirent->d_name, full_src_pathname);
            printf("Hey %s\n", full_src_pathname);*/
            struct stat* src_info2 = (struct stat*)malloc(sizeof(struct stat));
            
            
            char buf1[MAX_PATH];
            buf1[0] = '\0';
            char buf2[] = {"/"};
            strcat(buf1, src_pathname);
            strcat(buf1, buf2);
            strcat(buf1, src_dirent->d_name);
            //printf("%s\n", buf1);
            stat(buf1, src_info2);
            
            char buf3[MAX_PATH];
            buf3[0] = '\0';
            char buf4[] = {"/"};
            strcat(buf3,full_dest_pathname);
            strcat(buf3, buf4);
            strcat(buf3, src_dirent->d_name);
            //printf("%s\n", buf3);

            if(S_ISREG(src_info2->st_mode)){
              /*char buf[MAX_PATH];
              realpath(src_dirent->d_name, buf);
              //printf("Hey %s\n", buf);
              char buf3[MAX_PATH];
              char buf4[] = {"/"};
              strcat(buf3,full_dest_pathname);
              strcat(buf3, buf4);
              strcat(buf3, src_dirent->d_name);
              printf("%s\n", buf3);*/
              copy_file(buf1, buf3);
              //buf3[0] = '\0'; 
            }else if(S_ISDIR(src_info2->st_mode)){
               //printf("whoops\n");
               mode_t mode = src_info2->st_mode;
               int ret = mkdir(buf3, 0777);
               if(ret < 0){
                   syserror(mkdir, buf3);
               }else{
                   copy_dir(buf1, buf3);
               }
               chmod(buf3, mode);
            }
            full_src_pathname[0]='\0';
            buf3[0] = '\0';
            free(src_info2);
        }
    }
    
    closedir(src);
    closedir(dest);
    return;
}

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		usage();
	}
        
        int ret;
        struct stat* src_info = (struct stat*)malloc(sizeof(struct stat));
        ret = stat(argv[1], src_info);
        if(ret < 0){
            syserror(stat, argv[1]);
            return 0;
        }
     
        if(S_ISREG(src_info->st_mode)){
            copy_file(argv[1], argv[2]);
        }else if(S_ISDIR(src_info->st_mode)){
            mode_t mode = src_info->st_mode;
            ret = mkdir(argv[2], 0777);
            if(ret < 0){
                syserror(mkdir, argv[2]);
                return 0;
            }
            
            copy_dir(argv[1], argv[2]);
            chmod(argv[3], mode);

        }
        //printf("%d\n", ret);
        free(src_info);
	return 0;
}
