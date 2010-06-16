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

struct myData {
	char* rootdir;
	long fs_size;
};
 
#define _XOPEN_SOURCE 500
#define MAX_BLOCKS 200000
 
void getFullpath(char fpath[1024], const char* path){
	

	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
    	
	strcpy(fpath,data->rootdir);
	strncat(fpath,path,1024); 
}

long getFileSize( char fpath[1024] ){	
	
	int fd = open(fpath, O_RDONLY);
	char* tmpBuf;

	struct stat s;
	stat( fpath , &s);

	//number of chars in the file
	int chars = (int) s.st_size;
	tmpBuf = malloc( sizeof(chars) + 1 );

	//a 32bit number has max. 10 digits
	pread(fd, tmpBuf, chars , 0);
	tmpBuf[chars] = '\0';	

	//printf("read %d bytes: \n", (int) s.st_size );	 	
	printf("tmpBuf is %s \n", tmpBuf);	 	
	//interprete the file content as the file size
	char * pEnd;
	long fSize= strtol (tmpBuf,&pEnd,10);
	close(fd);

	free(tmpBuf);
	//printf("getFileSize(): size for %s is %ld \n", fpath, fSize);

	return fSize;
}
 
static int myBigFs_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
      
	struct stat s;
	memset(stbuf, 0, sizeof(struct stat));
	
	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
    	
	char fpath[1024];
	getFullpath(fpath,path) ;

	int err = stat( fpath , &s);

	char rootPath[1024];
	strcpy(rootPath, data->rootdir);
	strcat(rootPath,"/"); 	

	if(strcmp(fpath, rootPath ) == 0) {
          stbuf->st_mode = S_IFDIR | 0755;
          stbuf->st_nlink = 2;
	  return res;
     	}
	  
	if( err == 0)  {
		  
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink=1;
		stbuf->st_uid = 1000;
		stbuf->st_atime = s.st_atime;
		stbuf->st_mtime = s.st_mtime;
		stbuf->st_ctime = s.st_ctime;
		stbuf->st_size = getFileSize(fpath);
          
    } else {
          res = -ENOENT;
	}
  
      return res;
}
 

static int myBigFs_truncate (const char *path, off_t offset){
	
	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
	
	char fpath[1024];
	getFullpath(fpath,path) ;
	
	int fd = open( fpath, O_WRONLY);
	int ret = ftruncate( fd, offset );
	close(fd);

	return ret;
  }
 
 
 static int myBigFs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi)
  {
	(void) offset;
	(void) fi;
  
	if(strcmp(path, "/") != 0)
		return -ENOENT;
  
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	  
	struct dirent *dp;
	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
	char fpath[1024];
	getFullpath( fpath,path ) ;
	
	//printf("opening path %s \n", fpath);
	DIR* dirp = opendir( fpath );

	while (dirp) {
		 dp = readdir(dirp);

			if (dp != NULL ) {
				if( dp->d_type == 8 ){
					char tmp [1024];
					strcpy(tmp,path);
					strcat( tmp , dp->d_name);
					//printf("fill with %s\n", tmp);
					filler(buf, tmp + 1 , NULL, 0);
				}
			} else {
				 closedir(dirp);
				 break;
			 }
		}
  
      return 0;
  }
  
  static int myBigFs_open(const char *path, struct fuse_file_info *fi)
  {
	//open everything. we have nothing to hide.
      return 0;
  }
 



long getFileSystemSize(){

	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
	//char fpath[1024];
	//getFullpath(fpath,path) ;
	
    	struct dirent *dp;

	long bytes = 0;

	DIR* dirp = opendir( data->rootdir );
	while (dirp)
	{
		dp = readdir(dirp);
		if (dp != NULL ) {
			if( dp->d_type == 8 ){
				struct stat s;
				char tmpPath[1024];
				
				strcpy(tmpPath,data->rootdir);
				strcat(tmpPath,"/");
				strcat(tmpPath, dp->d_name);

				bytes += getFileSize(tmpPath);
			}
		} else {
			closedir(dirp);
			break;
		}
	}
	//printf("bytes before block setting: %ld",bytes);
	if( bytes / 1024 > MAX_BLOCKS ) bytes = MAX_BLOCKS*1024;
	data->fs_size = bytes;
	//printf("getFsSize returned %ld bytes", bytes);
	return bytes;		
}



 
int myBigFs_statfs(const char *path, struct statvfs *buf)
{
	int ret = 0;

    	//fixed fantasy constant
    	buf->f_bsize = 1024;
    
    	//fixed size..
    	buf->f_blocks = MAX_BLOCKS;
    
	long bytes = getFileSystemSize();
        
	//if a file is smaller then blocksize, use blocksize..
	if( bytes < 1024 ) bytes = 1024;

    	//printf("bytes: %ld \n" , bytes);
	
	buf->f_bfree =  buf->f_blocks - (bytes / 1024);
	buf->f_bavail =  buf->f_bfree;
  
	//printf("f_bfree: %ld\n", buf->f_bfree);
	////printf("bytes / 1024: %ld \n", (bytes/1024));
	//printf("f_bfree: \n", buf->f_bfree);
	  
 
    	if (ret < 0)
		ret =printf("statvfs error");
    
    
    	return ret;
}

  
  static int myBigFs_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
  {
	size_t len;
	(void) fi;
	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
	char fpath[1024];
	getFullpath(fpath,path) ;
	
      
	struct stat s;
    	int err = stat( fpath , &s);
     
    	//printf("buf has size: %ld", sizeof(buf));
     
    	if( err == 0){

			int fd = open(fpath, O_RDONLY);
			char* tmpBuf;
			
			//number of chars in the file
			int chars = (int) s.st_size;
			tmpBuf = malloc( chars+1 );
			//a 32bit number has max. 10 digits
			pread(fd, tmpBuf, chars , 0);
			tmpBuf[chars] = '\0';	
			
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
			free(tmpBuf);
			close(fd);
			
		//} else
		//	size = 0;
	}
//	printf("return size: %ld", size);  
	return size;
  }


int myBigFs_mknod(const char *path, mode_t mode, dev_t dev)
{
  
	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);
	char fpath[1024];
	getFullpath(fpath,path) ;
	

    	struct stat s;
    	int err = stat( path , &s);

	int ret = mknod(fpath, mode, dev);
	if (ret < 0)
		ret = printf("mknod failed\n");
	
    
    return ret;
}

static int myBigFs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

	struct stat s;
	struct myData *data;
	data = ((struct myData *) fuse_get_context()->private_data);

	char fpath[1024];
	getFullpath(fpath,path) ;
	
    	int err = stat( fpath , &s);

	//printf("file system size after this write in blocks: %ld \n", ( data->fs_size + size ) / 1024 ); 	
	//check if the filesystem is "full"
	if( ((data->fs_size + size) / 1024) + 1 >= MAX_BLOCKS ){
		data->fs_size = MAX_BLOCKS*1024;
		return size;
	} else {
		data->fs_size = data->fs_size + size;
	} 

	int fd = open(fpath, O_RDONLY);
	char* tmpBuf;
			
	//number of chars in the file
	int chars = (int) s.st_size;
	tmpBuf = malloc( chars+1 );
	//a 32bit number has max. 10 digits
	pread(fd, tmpBuf, chars , 0);
	tmpBuf[chars] = '\0';	
			
	//interprete the file content as the file size
	char * pEnd;
	long fSize= strtol (tmpBuf,&pEnd,10);
			
	close(fd);
	
	fd = open(fpath, O_WRONLY | O_CREAT);
	
	long newSize;
	//if(offset == 0) newSize = fSize + size; 
	if(offset + size > fSize) newSize = (offset+size); // no changes, it's an update in the middle of the file..
	
	char longBuffer[10];
	sprintf(longBuffer, "%ld", newSize);

	//printf("We're writing %s to the file \n", longBuffer );
	
	int f = pwrite(fd, longBuffer, strlen(longBuffer) , 0);

	free(tmpBuf);
	close(fd);
	
	if(f<0){
		printf("ERRNO: %i",errno);
		return -errno;
	}

	
		
     return size;
}
  
  static struct fuse_operations myBigFs_oper = {
      .getattr = myBigFs_getattr,
      .readdir = myBigFs_readdir,
      .open = myBigFs_open,
      .read = myBigFs_read,
      .truncate = myBigFs_truncate,
      .write = myBigFs_write,
      .mknod = myBigFs_mknod,
      .statfs = myBigFs_statfs,
  };
 
int main(int argc, char *argv[])
{
	struct myData *data;

	data = calloc(sizeof(struct myData), 1);
    	if (data == NULL) {
		perror("main calloc");
		abort();
    	}


	int i;
	for (i = 1; (i < argc) && (argv[i][0] == '-'); i++);
	if (i == argc)
    		printf("you've used it wrong!");

	    
	data->rootdir = realpath(argv[i],NULL);
	data->fs_size = 0;
	printf("We're operating on %s \n", realpath(argv[i], NULL));

	for (; i < argc; i++)
    		argv[i] = argv[i+1];
	argc--;

       return fuse_main(argc, argv, &myBigFs_oper, data); 
  }

