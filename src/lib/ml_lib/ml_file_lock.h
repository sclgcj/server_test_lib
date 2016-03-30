#ifndef ML_FILE_LOCK_H
#define ML_FILE_LOCK_H 1

int
ml_file_read_lock_by_fd(int fd, int iOffset, int iLen);


int
ml_file_write_lock_by_fd(int fd, int iOffset, int iLen);


int 
ml_file_unlock_by_fd(int fd, int iOffset, int iLen);

#endif
