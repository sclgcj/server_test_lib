#include <fcntl.h>
#include <unistd.h>
#include "ml_comm.h"
#include "ml_file_lock.h"

static int
ml_fcntl(
	int fd,
	int iType,
	int iOffset,
	int iLen
)
{
	struct flock struFL;

	memset( &struFL, 0, sizeof(struFL) ); 

	ML_DEBUG("iTYpe = %d\n", iType);
	ML_DEBUG("offset = %d\n", iOffset);
	ML_DEBUG("len = %d\n", iLen);
	struFL.l_type = iType;
	struFL.l_whence = SEEK_SET;
	struFL.l_start = iOffset;
	struFL.l_len = iLen;
	//struFL.l_pid = getpid();
	
	if( fcntl(fd, F_SETLK, &struFL) )
	{
		ML_ERROR( "fcntl type = %d, offset = %d, len =%d: %s\n", iType, iOffset, iLen,  strerror(errno) );
		return ML_ERR;
	}
		
	return ML_OK;
}

static int 
ml_file_lock(
	int  fd,
	int  iType,
	int  iOffset,
	int  iLen
)
{
	int iFcntlType = 0;

	iFcntlType = iType;
	return ml_fcntl(fd, iFcntlType, iOffset, iLen);
}	

int 
ml_file_read_lock_by_fd(
	int fd,
	int iOffset,
	int iLen
)
{
	return ml_fcntl(fd, F_RDLCK, iOffset, iLen);
}

int
ml_file_write_lock_by_fd(
	int fd,
	int iOffset,
	int iLen
)
{
	return ml_fcntl(fd, F_WRLCK, iOffset, iLen);
}

int
ml_file_unlock_by_fd(
	int  fd,
	int  iOffset,
	int  iLen
)
{
	return ml_fcntl(fd, F_UNLCK, iOffset, iLen);
}

