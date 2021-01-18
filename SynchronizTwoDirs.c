#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>

#define constbuff 64*1024*1024

char *filename = NULL;
int cproc = 0;
int currentcproc = 0;

void copy_files(const char *curr_dir1, const char *cur r_dir2, int *bcount) 
{
    mode_t mode;
    struct stat finfo;

    if (access(curr_dir1, R_OK) ==  -1)
    {
        fprintf(stderr, "%d %s: %s %s \n", getpid(), filename, strerror(errno), curr_dir1);
        return;
    }

    if ((stat(curr_dir1, &finfo) == -1))
    {
        fprintf(stderr, "%d %s: %s %s \n", getpid(), filename, strerror(errno), curr_dir1);
        return;
    }

    mode = finfo.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO);
    int fsrc = open(curr_dir1, O_RDONLY);
    int fdest = open(curr_dir2, O_WRONLY|O_CREAT, mode);

    if (fsrc == -1)
    {
        fprintf(stderr, "%d %s: Error opening file %s\n", getpid(), filename, curr_dir1);
        return;
    }

    if (fdest == -1)
    {
        fprintf(stderr, "%d %s: Error opening file %s\n", getpid(), filename, curr_dir2);
        return;
    }

    *bcount = 0;
    char *buff;

    if ((buff = (char *) malloc(constbuff)) == NULL)
    {
        fprintf(stderr, "%d %s: Memory allocation error\n", getpid(), filename);
    return;
    }

    ssize_t read_val, write_val;
    
    while (((read_val = read(fsrc, buff, constbuff)) != 0) && (read_val != -1)) 
    { 
        if ((write_val = write(fdest, buff, (size_t)read_val)) ==  -1)
        {
            fprintf(stderr, "%d %s: Error writing to file %s\n", getpid(), filename, curr_dir2);
            
            if (close(fsrc) == -1)
            {
                fprintf(stderr, "%d %s: Error closing file %s\n", getpid(), filename, curr_dir1);
                return;
            }

            if (close(fdest) == -1)
            {
                fprintf(stderr, "%d %s: Error closing file %s\n", getpid(), filename, curr_dir2);
                return;
            }

            free(buff);
            return;
        }
        
        *bcount += write_val;
    }

    if (close(fsrc) == -1)
    {
        fprintf(stderr, "%d %s: Error closing file %s\n", getpid(), filename, curr_dir1);
        return;
    }

    if (close(fdest) == -1)
    {
        fprintf(stderr, "%d %s: Error closing file %s\n", getpid(), filename, curr_dir2);
        return;
    }
}

void process_directory(char *dir1, char *dir2)
{
    DIR *dir = opendir(dir1);

    if (!dir)
    {
        fprintf(stderr, "%d %s: %s %s \n", getpid(), filename, strerror(errno), dir1);
        return; 
    }

    struct dirent *select_dir;
    struct stat info1, info2;
    char *curr_dir1 = alloca(strlen(dir1) + NAME_MAX + 3), 
    *curr_dir2 = alloca(strlen(dir2) + NAME_MAX + 3);
    curr_dir1[0] = 0;
    curr_dir2[0] = 0;
    strcat(strcat(curr_dir1, dir1), "/");
    strcat(strcat(curr_dir2, dir2), "/");
    size_t curr_len1 = strlen(curr_dir1), curr_len2 = strlen(curr_dir2);
    errno = 0;

    while ((select_dir = readdir(dir)) != NULL )
    {
        curr_dir1[curr_len1] = 0;
        strcat(curr_dir1, select_dir->d_name);
        curr_dir2[curr_len2] = 0;
        strcat(curr_dir2, select_dir->d_name);
        
        if ((lstat(curr_dir1, &info1) == -1))
        {
            fprintf(stderr, "%d %s: %s %s \n", getpid(), filename, strerror(errno), curr_dir1);
            continue;
        }

        if (S_ISDIR(info1.st_mode))
        {
            if ((strcmp(select_dir->d_name, ".") != 0) && (strcmp(select_dir->d_name, "..") != 0) )
            {
                if (stat(curr_dir2, &info2) != 0)
                {
                    mkdir(curr_dir2, info1.st_mode);
                }

                process_directory(curr_dir1, curr_dir2);
            }
        }
        else if (S_ISREG(info1.st_mode))
        {
            if (currentcproc >= cproc)
            {
                int status;

                if (wait(&status) != 0)
                {
                    --currentcproc;
                }
            }

            if (stat(curr_dir2, &info2) != 0)
            {
                pid_t pid = fork();
                
                if (pid == 0)
                {
                    int bcount = 0;
                    copy_files(curr_dir1, curr_dir2, &bcount);

                    if (bcount != 0)
                    {
                        printf("%d %s %d\n", getpid(), curr_dir1, bcount);
                    }
                
                    exit(0);
                }
                else if (pid < 0)
                {
                    fprintf(stderr, "%d %s: Process fork error\n", getpid(), filename);
                }
    
                currentcproc++;
            } 
            else
                fprintf(stderr, "%d %s: This file already exists %s\n", 
            
            getpid(), filename, curr_dir2);
        }
    }

    if (closedir(dir) == -1)
    {
        fprintf(stderr, "%d %s: %s %s \n", getpid(), filename, strerror(errno), dir1);
    }
}

int main(int argc, char *argv[])
{
    filename = basename(argv[0]);
    char *src = realpath(argv[1], NULL);
    char *dest = realpath(argv[2], NULL);

    if (src == NULL)
    {
        fprintf(stderr, "%d %s: Error opening catalog %s\n", getpid(), filename, argv[1]);
        return 1;
    }

    if (dest == NULL)
    { 
        fprintf(stderr, "%d %s: Error opening catalog %s\n", getpid(), filename, argv[2]);
        return 1;
    }

    cproc = atoi(argv[3]);

    if (cproc == 0)
    {
        fprintf(stderr, "%d %s: You have entered 0 processes\n", getpid(), filename);
        return 1;
    }

    process_directory(src, dest);

    while (wait(NULL) > 0) {}
    
    return 0;
}