#ifndef FILE_MIRRORING_OPERATIONS_H
#define FILE_MIRRORING_OPERATIONS_H

typedef enum
{
    FMO_SEND_NEXT_PATH = 1,
    FMO_SEND_LAST_PATH_FILE_CONTENT,
    FMO_OPERATION_COUNT
} FileMirroringOperations_t;

typedef enum
{
    FT_NONE = 0,
    FT_FILE,
    FT_FOLDER,
    FILE_TYPES_COUNT
} FileType_t;

#endif // FILE_MIRRORING_OPERATIONS_H