#ifndef FILE_MIRRORING_OPERATIONS_H
#define FILE_MIRRORING_OPERATIONS_H

#define MAX_PATH_SIZE 256

typedef enum
{
    FT_NONE = 0,
    FT_FILE,
    FT_FOLDER,
} FileType_t;

typedef struct
{
    char path[MAX_PATH_SIZE];
    time_t lastModifiedTime;
    FileType_t fileType;
} PathData_t;

typedef enum
{
    FMO_SEND_NEXT_PATH = 1,
    FMO_SEND_LAST_PATH_FILE_CONTENT,
} FileMirroringOperations_t;

#endif // FILE_MIRRORING_OPERATIONS_H