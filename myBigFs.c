 #define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/statvfs.h>

struct dirent *readdir(DIR *dir);

#define _XOPEN_SOURCE 500
 
  static const char *hello_str = "Hello World!\n";
  static const char *hello_path = "/hello";
  static const char *file_path = "/file";
  static const char *file_str = "example\n";
  
  
  void strappnd(char *dest, char *src)
{
	/* appends one string to another */
             while(*dest != '\0')
             {
                     *dest++;
             }
	while (*src != '\0')
		*dest++ = *src++;

	*dest = '\0';
}
  
  
  static int hello_getattr(const char *path, struct stat *stbuf)
  {
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
      
	struct stat s;
	char p [1024] = ".";
	char* localPath = p;
	strappnd(localPath, path);
	printf("path: %s\n",localPath);
	int err = stat( localPath , &s);
	  
	if(strcmp(path, "/") == 0) {
          stbuf->st_mode = S_IFDIR | 0755;
          stbuf->st_nlink = 2;
		  return res;
     }
	  
    if( err == 0)  {
		  
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_uid = 1000;
		stbuf->st_atime = s.st_atime;
		stbuf->st_mtime = s.st_mtime;
		stbuf->st_ctime = s.st_ctime;
		
		int fd = open(localPath, O_RDONLY);
			
		char tmpBuf[10];
		char *b = tmpBuf;
		//a 32bit number has max. 10 digits
		pread(fd, tmpBuf, 10 , 0);
			
		//interprete the file content as the file size
		char * pEnd;
		long fSize= strtol (tmpBuf,&pEnd,10);
			
		close(fd);
		
		stbuf->st_size = fSize;
          
    } else {
          res = -ENOENT;
	}
  
      return res;
  }
 

static int hello_truncate (const char *path, off_t offset){

	char p [1024] = ".";
	char* localPath = p;
	strappnd(localPath, path);
	
	int fd = open( localPath, O_WRONLY);
	int ret = ftruncate( fd, offset );
	close(fd);

return ret;
  }
 
 
 static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi)
  {
		(void) offset;
		(void) fi;
  
		if(strcmp(path, "/") != 0)
			return -ENOENT;
  
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
	  
		struct dirent *dp;
	   
		char p [1024] = ".";
		char* localPath = p;
		strappnd(localPath, path);
		//printf("opening path %s", localPath);
		DIR* dirp = opendir( localPath );

		while (dirp) {
			 dp = readdir(dirp);

			if (dp != NULL ) {
				if( dp->d_type == 8 ){
					char tmp [1024];
					strcpy(tmp,localPath);
					strcat( tmp , dp->d_name);
					//printf("fill with %s\n", tmp);
					filler(buf, tmp + 2 , NULL, 0);
				}
			} else {
				 closedir(dirp);
				 break;
			 }
		}
  
      return 0;
  }
  
  static int hello_open(const char *path, struct fuse_file_info *fi)
  {
	//open everything. we have nothing to hide.
      return 0;
  }
  
  int hello_statfs(const char *path, struct statvfs *buf)
{
    int ret = 0;
        
    char p [1024] = ".";
	char* localPath = p;
	strappnd(localPath, path);

	printf("path : %s \n ",localPath);
	
	int fd = open(localPath, O_RDONLY);
    //ret = fstatvfs(fd, buf);
    close(fd);
    
    printf("ret : %d  \n",ret);
    
    //fixed fantasy constant
    buf->f_bsize = 1024;
    
    //fixed size..
    buf->f_blocks = 200000;
    
    struct dirent *dp;

	long bytes = 0;

	DIR* dirp = opendir(".");
	while (dirp)
	{
		dp = readdir(dirp);

		if (dp != NULL ) {
			if( dp->d_type == 8 ){
				struct stat s;
				stat( dp->d_name , &s);
				bytes += s.st_size;
			}
		} else {
			closedir(dirp);
			break;
		}
	}
		
    printf("bytes: %ld \n" , bytes);
	buf->f_bfree =  buf->f_blocks - (bytes / 1024);
	buf->f_bavail =  buf->f_bfree;
    
    if (ret < 0)
		ret =printf("statvfs error");
    
    
    return ret;
}

  
  static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
  {
	size_t len;
	(void) fi;
      
	struct stat s;
	char p [1024] = ".";
	char* localPath = p;
	strappnd(localPath, path);
    int err = stat( localPath , &s);
     
    //printf("buf has size: %ld", sizeof(buf));
     
    if( err == 0){

			int fd = open(localPath, O_RDONLY);
			
			char tmpBuf[10];
			char *b = tmpBuf;
			//a 32bit number has max. 10 digits
			pread(fd, tmpBuf, 10 , 0);
			
			//interprete the file content as the file size
			char * pEnd;
			long fSize= strtol (tmpBuf,&pEnd,10);
			//printf("size is %ld\n", size);
			//printf("yeah, i just read %ld\n", fSize);

			if( fSize <= size ){
				//printf("fSize <= size: memset buf 80 %ld", fSize); 
				memset(buf, 65 , fSize);
			}
			
			if( fSize > size ){
				//printf("fSize > size: memset buf 80 %ld", size); 
				memset(buf, 65, size );
			}
			
			//printf("buf %s \n", buf);
		
			close(fd);
			
		//} else
		//	size = 0;
	}
	//printf("return size: %ld", size);  
	return size;
  }


int hello_mknod(const char *path, mode_t mode, dev_t dev)
{
  

    struct stat s;
	char p [1024] = ".";
	char* localPath = p;
	strappnd(localPath, path);
    int err = stat( localPath , &s);

	int ret = mknod(localPath, mode, dev);
	if (ret < 0)
		ret = printf("mknod failed\n");
	
    
    return ret;
}

static int hello_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

	struct stat s;
	char p [1024] = ".";
	char* localPath = p;
	strappnd(localPath, path);
    int err = stat( localPath , &s);

	//printf("we should write %ld bytes from offset %ld \n", size,offset);

	int fd = open(localPath, O_RDONLY);
			
	char tmpBuf[10];
	char *b = tmpBuf;
	//a 32bit number has max. 10 digits
	pread(fd, tmpBuf, 10 , 0);
			
	//interprete the file content as the file size
	char * pEnd;
	long fSize= strtol (tmpBuf,&pEnd,10);
			
	close(fd);
	
	fd = open(localPath, O_WRONLY | O_CREAT);
	
	long newSize;
	//if(offset == 0) newSize = fSize + size; 
	if(offset + size > fSize) newSize = (offset+size); // no changes, it's an update in the middle of the file..
	
	char longBuffer[10];
	sprintf(longBuffer, "%ld", newSize);

	//printf("We're writing %s to the file \n", longBuffer );
	
	int f = pwrite(fd, longBuffer, strlen(longBuffer) , 0);
	
	
	close(fd);
	
	if(f<0){
		printf("ERRNO: %i",errno);
		return -errno;
	}

     return size;
}
  
  static struct fuse_operations hello_oper = {
      .getattr = hello_getattr,
      .readdir = hello_readdir,
      .open = hello_open,
      .read = hello_read,
      .truncate = hello_truncate,
      .write = hello_write,
      .mknod = hello_mknod,
      .statfs = hello_statfs,
  };
  
  int main(int argc, char *argv[])
  {
      return fuse_main(argc, argv, &hello_oper, NULL);
  }

