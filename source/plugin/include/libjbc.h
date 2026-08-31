#pragma once

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct jbc_cred
{
    uid_t uid;
    uid_t ruid;
    uid_t svuid;
    gid_t rgid;
    gid_t svgid;
    uintptr_t prison;
    uintptr_t cdir;
    uintptr_t rdir;
    uintptr_t jdir;
    uint64_t sceProcType;
    uint64_t sonyCred;
    uint64_t sceProcCap;
};

typedef struct jbc_cred jbc_cred;

uintptr_t jbc_get_prison0(void);
uintptr_t jbc_get_rootvnode(void);
int jbc_get_cred(struct jbc_cred*);
int jbc_jailbreak_cred(struct jbc_cred*);
int jbc_set_cred(const struct jbc_cred*);
void jbc_run_as_root(void(*fn)(void* arg), void* arg, int cwd_mode);
int jbc_mount_in_sandbox(const char* system_path, const char* mnt_name);
int jbc_unmount_in_sandbox(const char* mnt_name);

#ifdef __cplusplus
}
#endif
