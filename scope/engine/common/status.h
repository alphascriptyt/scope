#ifndef STATUS_H
#define STATUS_H

typedef enum
{
	STATUS_OK,
	STATUS_FAILURE,
	STATUS_INVALID_ARGUMENT,
	STATUS_WIN32_FAILURE,
	STATUS_ALLOC_FAILURE

} Status;

inline const char* status_to_str(Status status)
{
    switch (status) 
    {
        case STATUS_OK:                 return "Success";
        case STATUS_FAILURE:            return "Failure";
        case STATUS_INVALID_ARGUMENT:   return "Invalid Argument";
        case STATUS_WIN32_FAILURE:      return "Win32 Failure";
        case STATUS_ALLOC_FAILURE:      return "Alloc Failure";
        default:                        return "Unknown Status";
    }
}

#endif