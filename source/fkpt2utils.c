/* ==============================================================================
 MODULE: fkpt2utils.c			    [fkPT2]
 Written by: M.A. Rodriguez-Meza
 Starting date:	31 october 2025
 Purpose: fk perturbation theory utils computation
 Language: C
 Use:
 Major revisions:
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

// Work to do in order to use with boxes not centered at (0,0,...)

//
// lines where there is a "//B socket:" string are places to include module files
//  that can be found in addons/addons_include folder
//

#include "globaldefs.h"

#include <string.h>

#include <errno.h>
#include <sys/wait.h>

global int cballs_system_checked(struct cmdline_data *cmd,
                                 string routineName,
                                 string command)
{
    int status = system(command);

    if (status == -1) {
        snprintf(cmd->error_message, _ERRORMSGSIZE_,
                 "%s: system command failed: %s: %s",
                 routineName, command, strerror(errno));
        return FAILURE;
    }

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0)
            return SUCCESS;

        snprintf(cmd->error_message, _ERRORMSGSIZE_,
                 "%s: system command exited with status %d: %s",
                 routineName, exit_status, command);
        return FAILURE;
    }

    if (WIFSIGNALED(status)) {
        snprintf(cmd->error_message, _ERRORMSGSIZE_,
                 "%s: system command killed by signal %d: %s",
                 routineName, WTERMSIG(status), command);
        return FAILURE;
    }

    snprintf(cmd->error_message, _ERRORMSGSIZE_,
             "%s: system command returned unexpected status %d: %s",
             routineName, status, command);
    return FAILURE;
}

global int cballs_stream_close_checked(struct cmdline_data *cmd,
                                       const char *routineName,
                                       stream *stream_ptr,
                                       const char *filename)
{
    int failed = 0;
    int saved_errno = 0;

    if (stream_ptr == NULL || *stream_ptr == NULL)
        return SUCCESS;

    if (ferror(*stream_ptr)) {
        failed = 1;
        saved_errno = errno;
    }

    if (fclose(*stream_ptr) != 0) {
        failed = 1;
        if (saved_errno == 0)
            saved_errno = errno;
    }

    *stream_ptr = NULL;

    if (failed) {
        snprintf(cmd->error_message, _ERRORMSGSIZE_,
                 "%s: error closing '%s': %s",
                 routineName, filename,
                 strerror(saved_errno ? saved_errno : EIO));
        return FAILURE;
    }

    return SUCCESS;
}


#define CLOSE_STREAM(s)        \
    do {                       \
        if ((s) != NULL) {     \
            fclose(s);         \
            (s) = NULL;        \
        }                      \
    } while (0)



//B Calculate number of threads
// Use in the terminal:
// export OMP_NUM_THREADS=8
// to set the maximum number of threads that can be used in a run
global int ThreadCount(struct  cmdline_data* cmd, struct  global_data* gd,
                       INTEGER nbody, int cat1) {
    int nthreads=0;
#pragma omp parallel
    {
#pragma omp atomic
        nthreads++;
    }
    verb_print(cmd->verbose, "\nUsing %d threads \n",nthreads);

//B socket:
#ifdef ADDONS
#include "cballsutils_include_01.h"
#endif
//E

    return SUCCESS;
}
//E




//B section of several routines to do pre/post processing


//B socket:
#ifdef ADDONS
#include "cballsutils_include_02.h"
#endif
//E
