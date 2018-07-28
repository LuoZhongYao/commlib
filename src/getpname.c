#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

const char *getpname(void) {
    FILE *fp;
    pid_t pid = getpid();
    static char pname[256] = "invaid";
    char path[256];
    char buff[1024];
    sprintf(path, "/proc/%d/status", pid);
    if(NULL != (fp = fopen(path, "r"))) {
        if(fgets(buff, 255, fp) != NULL)
            sscanf(buff, "%*s %s", pname);
        fclose(fp);
    }
    return pname;
}
