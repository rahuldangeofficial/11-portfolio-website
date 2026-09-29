/* Browser adapter for the unmodified, vendored MIMFS implementation (MIT).
 * All filesystem operations below call the actual C library. */
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include "mimfs/mimfs.h"
#include "core/mimfs_layout.h"
#include "core/mimfs_inode_allocator.h"
#include "support/mimfs_bitmap.h"

#define REGION_SIZE (8 * 1024 * 1024)
static _Alignas(64) unsigned char region[REGION_SIZE];
static struct mimfs_volume *volume;
static char output[262144];
static size_t used;
static int last_status;
static void emit(const char *format, ...) {
    if (used >= sizeof(output)-1) return;
    va_list args; va_start(args, format);
    int n = vsnprintf(output+used, sizeof(output)-used, format, args);
    va_end(args);
    if(n>0) used += (size_t)n < sizeof(output)-used ? (size_t)n : sizeof(output)-used-1;
}
static int check(long status) {
    if(status<0) { last_status=(int)status; emit("Error: %s (%ld)\n",mimfs_describe_status((int)status),status); return 0; }
    return 1;
}
EMSCRIPTEN_KEEPALIVE int shell_status(void) { return last_status; }
EMSCRIPTEN_KEEPALIVE int shell_init(void) {
    if(volume) mimfs_unmount(volume);
    int status=0;
    volume=mimfs_format(region,sizeof(region),NULL,&status);
    return volume ? 0 : -status;
}
static void listing(const char *path, int recursive, int detailed, unsigned depth, unsigned *budget) {
    if(depth>32 || !*budget) { emit("[listing limit reached]\n"); return; }
    int fd=mimfs_open_directory(volume,path);
    if(!check(fd)) return;
    struct mimfs_directory_entry_information entry;
    int result;
    while((result=mimfs_read_directory(volume,fd,&entry))>0 && *budget) {
        if(!strcmp(entry.name,".") || !strcmp(entry.name,"..")) continue;
        (*budget)--;
        char child[4096];
        if(snprintf(child,sizeof(child),"%s%s%s",path,!strcmp(path,"/")?"":"/",entry.name)>=(int)sizeof(child)) { emit("[path too long]\n"); continue; }
        struct mimfs_status_information st;
        if(!check(mimfs_get_link_status(volume,child,&st))) break;
        if(detailed) emit("%06o %u %" PRIu64 " ",st.file_mode,st.hard_link_count,st.file_size_in_bytes);
        emit("%s%s",recursive?child:entry.name,MIMFS_MODE_IS_DIRECTORY(st.file_mode)?"/":"");
        if(MIMFS_MODE_IS_SYMBOLIC_LINK(st.file_mode)) {
            char target[4096]; long n=mimfs_read_symbolic_link(volume,child,target,sizeof(target)-1);
            if(n>=0) { target[n]=0; emit(" -> %s",target); }
        }
        emit("\n");
        if(recursive && MIMFS_MODE_IS_DIRECTORY(st.file_mode)) listing(child,1,detailed,depth+1,budget);
    }
    check(result); mimfs_close_directory(volume,fd);
}
#ifdef __EMSCRIPTEN_PTHREADS__
struct job { int id; int status; const char *path; };
static void *append_worker(void *argument) {
    struct job *job=argument;
    int fd=mimfs_open(volume,job->path,MIMFS_OPEN_CREATE|MIMFS_OPEN_WRITE_ONLY|MIMFS_OPEN_APPEND,0644);
    if(fd<0) { job->status=fd; return NULL; }
    for(int i=0;i<100;i++) {
        char record[64]; int n=snprintf(record,sizeof(record),"worker %d record %03d\n",job->id,i);
        long written=mimfs_write(volume,fd,record,(size_t)n);
        if(written!=n) { job->status=written<0?(int)written:-MIMFS_STATUS_INPUT_OUTPUT_ERROR; break; }
    }
    mimfs_close(volume,fd); return NULL;
}
#endif
EMSCRIPTEN_KEEPALIVE const char *shell_exec(const char *cmd,const char *a,const char *b) {
    used=0;output[0]=0;last_status=0;
    if(!volume && !check(shell_init())) return output;
    if(!strcmp(cmd,"format")) { if(check(shell_init())) emit("Formatted real MIMFS volume: 8 MiB in WebAssembly memory.\n"); }
    else if(!strcmp(cmd,"mkdir")) { if(check(mimfs_create_directory(volume,a,0755))) emit("Directory created.\n"); }
    else if(!strcmp(cmd,"touch") || !strcmp(cmd,"write") || !strcmp(cmd,"append")) {
        unsigned flags=MIMFS_OPEN_CREATE|MIMFS_OPEN_WRITE_ONLY;
        if(!strcmp(cmd,"write")) flags|=MIMFS_OPEN_TRUNCATE;
        if(!strcmp(cmd,"append")) flags|=MIMFS_OPEN_APPEND;
        int fd=mimfs_open(volume,a,flags,0644);
        if(check(fd)) {
            if(strcmp(cmd,"touch")) { long n=mimfs_write(volume,fd,b,strlen(b)); if(check(n)) emit("%s: %ld bytes written\n",a,n); }
            else emit("File ready; existing contents preserved.\n");
            check(mimfs_close(volume,fd));
        }
    } else if(!strcmp(cmd,"cat") || !strcmp(cmd,"hexdump")) {
        int fd=mimfs_open(volume,a,MIMFS_OPEN_READ_ONLY,0);
        if(check(fd)) {
            unsigned char data[16385];long n=mimfs_read(volume,fd,data,sizeof(data));
            if(check(n)) {
                long shown=n>16384?16384:n;
                if(!strcmp(cmd,"cat")) { for(long i=0;i<shown;i++) { if(data[i]==0) emit("\\0"); else emit("%c",data[i]); } }
                else for(long i=0;i<shown;i+=16) {
                    emit("%08lx  ",i);
                    for(long j=i;j<i+16 && j<shown;j++) emit("%02x ",data[j]);
                    emit("\n");
                }
                if(n>shown) emit("\n[output limited to first 16 KiB]\n");
            }
            mimfs_close(volume,fd);
        }
    } else if(!strcmp(cmd,"ls") || !strcmp(cmd,"tree")) {
        unsigned budget=1024;listing(*a?a:"/",!strcmp(cmd,"tree"),!strcmp(b,"long"),0,&budget);
        if(!used) emit("(empty directory)\n");
    } else if(!strcmp(cmd,"stat")) {
        struct mimfs_status_information st;
        if(check(mimfs_get_link_status(volume,a,&st))) emit("File: %s\nInode: %u | Links: %u\nMode: %06o\nSize: %" PRIu64 " bytes\nAllocated blocks: %" PRIu64 "\n",a,st.inode_number,st.hard_link_count,st.file_mode,st.file_size_in_bytes,st.allocated_block_count);
    } else if(!strcmp(cmd,"df")) {
        struct mimfs_volume_statistics st;
        if(check(mimfs_query_volume_statistics(volume,&st))) emit("Real MIMFS · 8 MiB region\nData blocks: %" PRIu64 " | Free: %" PRIu64 " | Available: %" PRIu64 "\nBlock size: %u bytes | Allocated inodes: %" PRIu64 "\n",st.total_block_count,st.free_block_count,st.available_block_count,st.block_size_in_bytes,st.total_inode_count-st.free_inode_count);
    } else if(!strcmp(cmd,"ln")) { if(check(mimfs_create_hard_link(volume,a,b))) emit("Hard link created.\n"); }
    else if(!strcmp(cmd,"symlink")) { if(check(mimfs_create_symbolic_link(volume,a,b))) emit("Symbolic link created.\n"); }
    else if(!strcmp(cmd,"mv")) { if(check(mimfs_rename(volume,a,b))) emit("Renamed.\n"); }
    else if(!strcmp(cmd,"rm")) {
        struct mimfs_status_information st;
        if(check(mimfs_get_link_status(volume,a,&st)) && check(MIMFS_MODE_IS_DIRECTORY(st.file_mode)?mimfs_remove_directory(volume,a):mimfs_remove_link(volume,a))) emit("Removed.\n");
    } else if(!strcmp(cmd,"truncate")) {
        char *end;unsigned long long size=strtoull(b,&end,10);
        if(!*b || *end || *b=='-' || size>64*1024*1024) check(-MIMFS_STATUS_INVALID_ARGUMENT);
        else if(check(mimfs_truncate_by_path(volume,a,size))) emit("File length set to %llu bytes (sparse growth).\n",size);
    } else if(!strcmp(cmd,"concurrency")) {
#ifdef __EMSCRIPTEN_PTHREADS__
        pthread_t threads[4];struct job jobs[4];int started=0;
        static unsigned sequence;char path[64];snprintf(path,sizeof(path),"/concurrency-%u.log",++sequence);
        for(int i=0;i<4;i++) {
            jobs[i]=(struct job){i,0,path};
            int status=pthread_create(&threads[i],NULL,append_worker,&jobs[i]);
            if(status) { check(-status);break; } started++;
        }
        for(int i=0;i<started;i++) { pthread_join(threads[i],NULL);check(jobs[i].status); }
        if(!last_status) emit("%d real POSIX threads each appended 100 records to %s.\n",started,path);
#else
        last_status=-MIMFS_STATUS_NOT_IMPLEMENTED;
        emit("Real filesystem is available, but threads require cross-origin isolation. Enable COOP/COEP headers to run this demo.\n");
#endif
    } else { last_status=-MIMFS_STATUS_INVALID_ARGUMENT;emit("Unknown command. Type help.\n"); }
    return output;
}
EMSCRIPTEN_KEEPALIVE const char *shell_snapshot(void) {
    used=0;output[0]=0;
    if(!volume) return "null";
    struct mimfs_volume_statistics st;mimfs_query_volume_statistics(volume,&st);
    emit("{\"totalBlocks\":%" PRIu64 ",\"freeBlocks\":%" PRIu64 ",\"inodeCount\":%" PRIu64 ",\"inodes\":{",st.total_block_count,st.free_block_count,st.total_inode_count-st.free_inode_count);
    unsigned emitted=0;
    for(uint64_t id=2;id<=volume->total_inode_count && emitted<128;id++) {
        if(!mimfs_inode_allocator_inode_is_allocated(volume,id)) continue;
        struct mimfs_inode *n=mimfs_inode_at(volume,id);
        const char *type=MIMFS_MODE_IS_DIRECTORY(n->file_mode)?"dir":MIMFS_MODE_IS_SYMBOLIC_LINK(n->file_mode)?"symlink":"file";
        emit("%s\"%" PRIu64 "\":{\"id\":%" PRIu64 ",\"type\":\"%s\",\"mode\":%u,\"size\":%" PRIu64 ",\"linkCount\":%u,\"directBlocks\":[",emitted++?",":"",id,id,type,n->file_mode,n->file_size_in_bytes,n->hard_link_count);
        unsigned count=0;
        if(!MIMFS_MODE_IS_SYMBOLIC_LINK(n->file_mode)) for(unsigned j=0;j<12;j++) {
            unsigned block=n->block_map_root.block_pointer_slot[j];if(block) emit("%s%u",count++?",":"",block);
        }
        emit("]}");
    }
    emit("},\"blocks\":[");
    for(uint64_t block=0;block<48 && block<volume->total_block_count;block++) {
        const char *type=mimfs_bitmap_bit_is_set(mimfs_block_allocation_bitmap(volume),block)?"allocated":"free";
        uint64_t owner=0;
        for(uint64_t id=2;id<=volume->total_inode_count;id++) {
            if(!mimfs_inode_allocator_inode_is_allocated(volume,id)) continue;
            struct mimfs_inode *n=mimfs_inode_at(volume,id);
            if(MIMFS_MODE_IS_SYMBOLIC_LINK(n->file_mode)) continue;
            for(unsigned j=0;j<12;j++) if(block && n->block_map_root.block_pointer_slot[j]==block) { owner=id;type=MIMFS_MODE_IS_DIRECTORY(n->file_mode)?"data_dir":"data_file"; }
        }
        emit("%s{\"id\":%" PRIu64 ",\"type\":\"%s\",\"ownerInode\":%" PRIu64 "}",block?",":"",block,type,owner);
    }
    emit("]}");return output;
}
