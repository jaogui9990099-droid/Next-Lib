// NextLib
// Android ARM64 SFS2X response listener for My Singing Monsters.
// Load libNextLib.so after libmonsters.so, or call nextlib_logger_install() after libmonsters.so is loaded.
// Output: logcat tag NextLib_LOGGER plus JSON files in the app files/msm_json folder.

typedef unsigned long long u64;
typedef long long i64;
typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned long usize;
typedef long ssize;


extern int __android_log_write(int prio, const char* tag, const char* text);
typedef unsigned long pthread_t_min;
extern int pthread_create(pthread_t_min* thread, const void* attr, void* (*start)(void*), void* arg);
extern int pthread_detach(pthread_t_min thread);
typedef unsigned short u16;
struct dl_phdr_info_min {
    u64 dlpi_addr;
    const char* dlpi_name;
    const void* dlpi_phdr;
    u16 dlpi_phnum;
};
extern int dl_iterate_phdr(int (*callback)(struct dl_phdr_info_min*, usize, void*), void* data);
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_ERROR 6

#define NULL ((void*)0)
#define AT_FDCWD (-100)
#define O_RDONLY 0
#define O_WRONLY 1
#define O_CREAT 64
#define O_TRUNC 512
#define O_APPEND 1024
#define O_CLOEXEC 02000000
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32

#define SYS_OPENAT 56
#define SYS_CLOSE 57
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_MMAP 222
#define SYS_MPROTECT 226
#define SYS_NANOSLEEP 101
#define SYS_MKDIRAT 34

#define MAX_MAPS 16384
#define MAPS_BUF_SIZE 4194304
#define LINE_BUF_SIZE 4096
#define RAW_DUMP_MAX 8192
#define MEM_DUMP_MAX 256
#define STR_SCAN_MAX 8192

struct timespec_min { long tv_sec; long tv_nsec; };
struct Range { u64 start; u64 end; int r; int w; int x; };

static char g_maps_buf[MAPS_BUF_SIZE];
static struct Range g_ranges[MAX_MAPS];
static int g_range_count = 0;
static int g_fd = -1;
static int g_summary_fd = -1;
static u64 g_lib_base = 0;
static volatile int g_install_done = 0;
static volatile int g_log_guard = 0;
static char g_current_cmd[128] = "?";

static long sc0(long n) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0");
    asm volatile("svc #0" : "=r"(x0) : "r"(x8) : "memory");
    return x0;
}
static long sc1(long n, long a0) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0") = a0;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
}
static long sc2(long n, long a0, long a1) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0") = a0;
    register long x1 asm("x1") = a1;
    asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    return x0;
}
static long sc3(long n, long a0, long a1, long a2) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0") = a0;
    register long x1 asm("x1") = a1;
    register long x2 asm("x2") = a2;
    asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
}
static long sc4(long n, long a0, long a1, long a2, long a3) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0") = a0;
    register long x1 asm("x1") = a1;
    register long x2 asm("x2") = a2;
    register long x3 asm("x3") = a3;
    asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
    return x0;
}
static long sc6(long n, long a0, long a1, long a2, long a3, long a4, long a5) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0") = a0;
    register long x1 asm("x1") = a1;
    register long x2 asm("x2") = a2;
    register long x3 asm("x3") = a3;
    register long x4 asm("x4") = a4;
    register long x5 asm("x5") = a5;
    asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8) : "memory");
    return x0;
}

static usize slen(const char* s) { usize n=0; if(!s) return 0; while(s[n]) n++; return n; }
static int starts_with(const char* s, const char* p) { while(*p) { if(*s++ != *p++) return 0; } return 1; }
static int contains_n(const char* s, usize n, const char* needle) {
    usize m = slen(needle); if(!m || n < m) return 0;
    for(usize i=0;i+m<=n;i++) { usize j=0; while(j<m && s[i+j]==needle[j]) j++; if(j==m) return 1; }
    return 0;
}
static int contains_cstr(const char* s, const char* needle) {
    if(!s || !needle) return 0;
    usize n = slen(s);
    return contains_n(s, n, needle);
}

__attribute__((visibility("hidden"))) void* memcpy(void* d, const void* s, unsigned long n) { unsigned char* dd=(unsigned char*)d; const unsigned char* ss=(const unsigned char*)s; for(unsigned long i=0;i<n;i++) dd[i]=ss[i]; return d; }
__attribute__((visibility("hidden"))) void* memset(void* d, int v, unsigned long n) { unsigned char* dd=(unsigned char*)d; for(unsigned long i=0;i<n;i++) dd[i]=(unsigned char)v; return d; }

static void mem_cpy(void* d, const void* s, usize n) { u8* dd=(u8*)d; const u8* ss=(const u8*)s; for(usize i=0;i<n;i++) dd[i]=ss[i]; }
static void mem_set(void* d, int v, usize n) { u8* dd=(u8*)d; for(usize i=0;i<n;i++) dd[i]=(u8)v; }

static void fd_write(int fd, const char* s, usize n) { if(fd >= 0 && s && n) sc3(SYS_WRITE, fd, (long)s, (long)n); }
static int open_path(const char* path) { return (int)sc4(SYS_OPENAT, AT_FDCWD, (long)path, O_WRONLY|O_CREAT|O_APPEND|O_CLOEXEC, 0666); }
static void mkdir_path(const char* path) { sc3(SYS_MKDIRAT, AT_FDCWD, (long)path, 0777); }
static void ensure_log_dirs(void) {
    mkdir_path("/sdcard/Android/data/com.bigbluebubble.singingmonsters.full");
    mkdir_path("/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files");
    mkdir_path("/storage/emulated/0/Android/data/com.bigbluebubble.singingmonsters.full");
    mkdir_path("/storage/emulated/0/Android/data/com.bigbluebubble.singingmonsters.full/files");
    mkdir_path("/sdcard/Download");
    mkdir_path("/storage/emulated/0/Download");
    mkdir_path("/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/msm_json");
    mkdir_path("/storage/emulated/0/Android/data/com.bigbluebubble.singingmonsters.full/files/msm_json");
}
static int open_path_trunc(const char* path) { return (int)sc4(SYS_OPENAT, AT_FDCWD, (long)path, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0666); }
static void open_log_if_needed(void) {
    if(g_fd >= 0) return;
    ensure_log_dirs();
    const char* paths[] = {
        "/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/nextlib_client.log",
        "/storage/emulated/0/Android/data/com.bigbluebubble.singingmonsters.full/files/nextlib_client.log",
        "/sdcard/Download/nextlib_client.log",
        "/storage/emulated/0/Download/nextlib_client.log",
        "/data/user/0/com.bigbluebubble.singingmonsters.full/files/nextlib_client.log",
        "/data/data/com.bigbluebubble.singingmonsters.full/files/nextlib_client.log",
        "/data/local/tmp/nextlib_client.log",
        0
    };
    for(int i=0; paths[i]; i++) { int fd = open_path(paths[i]); if(fd >= 0) { g_fd = fd; return; } }
}
static void open_summary_if_needed(void) {
    if(g_summary_fd >= 0) return;
    ensure_log_dirs();
    const char* paths[] = {
        "/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/nextlib_summary.log",
        "/storage/emulated/0/Android/data/com.bigbluebubble.singingmonsters.full/files/nextlib_summary.log",
        "/sdcard/Download/nextlib_summary.log",
        "/storage/emulated/0/Download/nextlib_summary.log",
        "/data/local/tmp/nextlib_summary.log",
        0
    };
    for(int i=0; paths[i]; i++) { int fd = open_path(paths[i]); if(fd >= 0) { g_summary_fd = fd; return; } }
}
static void log_to_logcat(const char* s) {
    if(!s) return;
    __android_log_write(ANDROID_LOG_INFO, "NextLib_LOGGER", s);
}
static void log_raw(const char* s) {
    if(!s) return;
    usize n = slen(s);
    open_log_if_needed();
    fd_write(g_fd, s, n);
    fd_write(2, s, n);
    log_to_logcat(s);
}
static void log_buf(const char* s, usize n) {
    open_log_if_needed();
    fd_write(g_fd, s, n);
    fd_write(2, s, n);
}
static void log_summary(const char* s) {
    if(!s) return;
    usize n = slen(s);
    open_summary_if_needed();
    fd_write(g_summary_fd, s, n);
    open_log_if_needed();
    fd_write(g_fd, s, n);
    if(n < 3900) log_to_logcat(s);
}

static void append_ch(char* b, int* p, int cap, char c) { if(*p < cap-1) b[(*p)++] = c; b[*p] = 0; }
static void append_str(char* b, int* p, int cap, const char* s) { if(!s) s="(null)"; while(*s) append_ch(b,p,cap,*s++); }
static void append_hex_digit(char* b, int* p, int cap, int v) { append_ch(b,p,cap,(char)(v<10?'0'+v:'a'+v-10)); }
static void append_hex64(char* b, int* p, int cap, u64 v) { append_str(b,p,cap,"0x"); for(int i=15;i>=0;i--) append_hex_digit(b,p,cap,(int)((v>>(i*4))&0xf)); }
static void append_hex_byte(char* b, int* p, int cap, u8 v) { append_hex_digit(b,p,cap,(v>>4)&0xf); append_hex_digit(b,p,cap,v&0xf); }
static void append_u64_dec(char* b, int* p, int cap, u64 v) {
    char tmp[32]; int n=0; if(v==0) { append_ch(b,p,cap,'0'); return; }
    while(v && n<31) { tmp[n++] = (char)('0' + (v%10)); v/=10; }
    while(n) append_ch(b,p,cap,tmp[--n]);
}
static void append_i64_dec(char* b, int* p, int cap, i64 v) { if(v<0) { append_ch(b,p,cap,'-'); append_u64_dec(b,p,cap,(u64)(-v)); } else append_u64_dec(b,p,cap,(u64)v); }

static int is_hex_char(char c) { return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'); }
static int hex_val(char c) { if(c>='0'&&c<='9') return c-'0'; if(c>='a'&&c<='f') return c-'a'+10; if(c>='A'&&c<='F') return c-'A'+10; return -1; }
static u64 parse_hex_ptr(const char** pp) { const char* p=*pp; u64 v=0; while(is_hex_char(*p)) { v=(v<<4)|(u64)hex_val(*p); p++; } *pp=p; return v; }
static void skip_spaces(const char** pp) { const char* p=*pp; while(*p==' '||*p=='\t') p++; *pp=p; }
static int find_libmonsters_phdr(struct dl_phdr_info_min* info, usize size, void* data);

static void refresh_maps(void) {
    g_range_count = 0;
    if(!g_lib_base) {
        dl_iterate_phdr(find_libmonsters_phdr, 0);
    }
    int fd = (int)sc4(SYS_OPENAT, AT_FDCWD, (long)"/proc/self/maps", O_RDONLY|O_CLOEXEC, 0);
    if(fd < 0) return;
    long n = sc3(SYS_READ, fd, (long)g_maps_buf, MAPS_BUF_SIZE-1);
    sc1(SYS_CLOSE, fd);
    if(n <= 0) return;
    g_maps_buf[n] = 0;
    const char* p = g_maps_buf;
    while(*p) {
        const char* line = p;
        while(*p && *p != '\n') p++;
        usize line_len = (usize)(p - line);
        if(*p == '\n') p++;
        const char* q = line;
        if(!is_hex_char(*q)) continue;
        u64 start = parse_hex_ptr(&q);
        if(*q != '-') continue; q++;
        u64 end = parse_hex_ptr(&q);
        skip_spaces(&q);
        int r = (q[0]=='r'); int w = (q[1]=='w'); int x = (q[2]=='x');
        if(g_range_count < MAX_MAPS) {
            g_ranges[g_range_count].start=start; g_ranges[g_range_count].end=end; g_ranges[g_range_count].r=r; g_ranges[g_range_count].w=w; g_ranges[g_range_count].x=x; g_range_count++;
        }
        if(contains_n(line, line_len, "libmonsters")) {
            const char* m = q + 4; skip_spaces(&m);
            u64 off = parse_hex_ptr(&m);
            if((x || off == 0) && start >= off) {
                if(g_lib_base == 0 || off == 0) g_lib_base = start - off;
            }
        }
    }
}
static int readable(const void* ptr, usize n) {
    if(!ptr || n == 0) return 0;
    u64 a=(u64)ptr, e=a+n;
    if(e < a) return 0;
    if(g_lib_base && a >= g_lib_base && e <= g_lib_base + 0x1c00000ULL) return 1;
    for(int i=0;i<g_range_count;i++) if(g_ranges[i].r && a >= g_ranges[i].start && e <= g_ranges[i].end) return 1;
    return 0;
}
static int executable_addr(const void* ptr) {
    u64 a=(u64)ptr;
    if(g_lib_base && a >= g_lib_base + 0x0b611c0ULL && a < g_lib_base + 0x1a50000ULL) return 1;
    for(int i=0;i<g_range_count;i++) if(g_ranges[i].x && a >= g_ranges[i].start && a < g_ranges[i].end) return 1;
    return 0;
}
static void log_hook_fail(const char* hook_name, const char* reason, void* target, long value) {
    char line[768]; int p=0;
    append_str(line,&p,sizeof(line),"[NextLib_LOGGER] hook detail FAIL ");
    append_str(line,&p,sizeof(line),hook_name ? hook_name : "?");
    append_str(line,&p,sizeof(line)," reason=");
    append_str(line,&p,sizeof(line),reason ? reason : "?");
    append_str(line,&p,sizeof(line)," target=");
    append_hex64(line,&p,sizeof(line),(u64)target);
    append_str(line,&p,sizeof(line)," value=");
    append_i64_dec(line,&p,sizeof(line),(i64)value);
    append_str(line,&p,sizeof(line)," maps=");
    append_i64_dec(line,&p,sizeof(line),(i64)g_range_count);
    append_ch(line,&p,sizeof(line),'\n');
    log_buf(line,(usize)p);
}

static int find_libmonsters_phdr(struct dl_phdr_info_min* info, usize size, void* data) {
    (void)size; (void)data;
    if(info && contains_cstr(info->dlpi_name, "libmonsters.so")) {
        g_lib_base = (u64)info->dlpi_addr;
        return 1;
    }
    return 0;
}

static int mostly_printable(const char* s, usize n) {
    if(!s || n > 4096) return 0;
    int good=0;
    for(usize i=0;i<n;i++) { unsigned char c=(unsigned char)s[i]; if((c>=32&&c<=126)||c=='\t'||c=='\r'||c=='\n') good++; }
    return n==0 || good*100 >= (int)n*85;
}
static void copy_clean(char* out, int cap, const char* data, usize n) {
    int p=0; if(cap<=0) return;
    for(usize i=0;i<n && p<cap-1;i++) { unsigned char c=(unsigned char)data[i]; out[p++] = (c>=32&&c<=126)?(char)c:'.'; }
    out[p]=0;
}
static u64 read_u64(const void* p) { u64 v=0; mem_cpy(&v,p,8); return v; }
static int decode_libcpp_string(const void* sp, char* out, int outcap) {
    if(outcap <= 0) return 0; out[0]=0;
    if(!readable(sp, 24)) return 0;
    const u8* p=(const u8*)sp;
    
    usize n1 = (usize)(p[0] >> 1);
    if((p[0] & 1) == 0 && n1 <= 22 && readable(p+1,n1) && mostly_printable((const char*)p+1,n1)) { copy_clean(out,outcap,(const char*)p+1,n1); return 1; }
    
    if(p[0] & 1) {
        usize n2 = (usize)read_u64(p+8);
        const char* data = (const char*)read_u64(p+16);
        if(n2 < 65536 && readable(data, n2?n2:1) && mostly_printable(data, n2 < 256 ? n2 : 256)) { copy_clean(out,outcap,data,n2); return 1; }
    }
    
    usize n3 = (usize)p[23];
    if(n3 <= 22 && readable(p,n3) && mostly_printable((const char*)p,n3)) { copy_clean(out,outcap,(const char*)p,n3); return 1; }
    n3 = (usize)(p[23] >> 1);
    if(n3 <= 22 && readable(p,n3) && mostly_printable((const char*)p,n3)) { copy_clean(out,outcap,(const char*)p,n3); return 1; }
    
    const char* data4 = (const char*)read_u64(p);
    usize n4 = (usize)read_u64(p+8);
    if(n4 < 65536 && readable(data4, n4?n4:1) && mostly_printable(data4, n4 < 256 ? n4 : 256)) { copy_clean(out,outcap,data4,n4); return 1; }
    return 0;
}
static void set_current_cmd_from_string_ptr(const void* strp) {
    char tmp[128];
    if(decode_libcpp_string(strp,tmp,sizeof(tmp))) { int i=0; while(tmp[i] && i<127) { g_current_cmd[i]=tmp[i]; i++; } g_current_cmd[i]=0; }
}

static void log_hexdump(const char* tag, const void* data, usize len, usize cap) {
    if(!data || !readable(data, len < cap ? len : cap)) return;
    if(len > cap) len = cap;
    char line[256];
    for(usize off=0; off<len; off+=16) {
        int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] "); append_str(line,&p,sizeof(line),tag); append_str(line,&p,sizeof(line)," +"); append_u64_dec(line,&p,sizeof(line),off); append_str(line,&p,sizeof(line),": ");
        for(int i=0;i<16;i++) { if(off+(usize)i < len) append_hex_byte(line,&p,sizeof(line),((const u8*)data)[off+i]); else append_str(line,&p,sizeof(line),"  "); append_ch(line,&p,sizeof(line),' '); }
        append_str(line,&p,sizeof(line)," | ");
        for(int i=0;i<16 && off+(usize)i<len;i++) { u8 c=((const u8*)data)[off+i]; append_ch(line,&p,sizeof(line),(c>=32&&c<=126)?(char)c:'.'); }
        append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p);
    }
}
static void log_hexdump_direct(const char* tag, const void* data, usize len, usize cap) {
    if(!data) return;
    if(len > cap) len = cap;
    char line[256];
    for(usize off=0; off<len; off+=16) {
        int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] "); append_str(line,&p,sizeof(line),tag); append_str(line,&p,sizeof(line)," +"); append_u64_dec(line,&p,sizeof(line),off); append_str(line,&p,sizeof(line),": ");
        for(int i=0;i<16;i++) { if(off+(usize)i < len) append_hex_byte(line,&p,sizeof(line),((const u8*)data)[off+i]); else append_str(line,&p,sizeof(line),"  "); append_ch(line,&p,sizeof(line),' '); }
        append_str(line,&p,sizeof(line)," | ");
        for(int i=0;i<16 && off+(usize)i<len;i++) { u8 c=((const u8*)data)[off+i]; append_ch(line,&p,sizeof(line),(c>=32&&c<=126)?(char)c:'.'); }
        append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p);
    }
}
static void log_ascii_strings(const char* tag, const void* data, usize len, usize cap) {
    if(!data) return;
    if(len > cap) len = cap;
    if(!readable(data, len)) return;
    const u8* p=(const u8*)data;
    char line[512];
    usize i=0;
    while(i<len) {
        while(i<len && !(p[i]>=32 && p[i]<=126)) i++;
        usize st=i;
        while(i<len && (p[i]>=32 && p[i]<=126)) i++;
        usize l=i-st;
        if(l >= 4) {
            int pos=0; append_str(line,&pos,sizeof(line),"[NextLib_LOGGER] "); append_str(line,&pos,sizeof(line),tag); append_str(line,&pos,sizeof(line)," ascii+"); append_u64_dec(line,&pos,sizeof(line),st); append_str(line,&pos,sizeof(line)," len="); append_u64_dec(line,&pos,sizeof(line),l); append_str(line,&pos,sizeof(line)," '");
            usize max = l < 350 ? l : 350; for(usize k=0;k<max;k++) append_ch(line,&pos,sizeof(line),(char)p[st+k]); if(l>max) append_str(line,&pos,sizeof(line),"..."); append_str(line,&pos,sizeof(line),"'\n"); log_buf(line,(usize)pos);
        }
    }
}
static void log_ascii_strings_direct(const char* tag, const void* data, usize len, usize cap) {
    if(!data) return;
    if(len > cap) len = cap;
    const u8* p=(const u8*)data;
    char line[512];
    usize i=0;
    while(i<len) {
        while(i<len && !(p[i]>=32 && p[i]<=126)) i++;
        usize st=i;
        while(i<len && (p[i]>=32 && p[i]<=126)) i++;
        usize l=i-st;
        if(l >= 4) {
            int pos=0; append_str(line,&pos,sizeof(line),"[NextLib_LOGGER] "); append_str(line,&pos,sizeof(line),tag); append_str(line,&pos,sizeof(line)," ascii+"); append_u64_dec(line,&pos,sizeof(line),st); append_str(line,&pos,sizeof(line)," len="); append_u64_dec(line,&pos,sizeof(line),l); append_str(line,&pos,sizeof(line)," '");
            usize max = l < 350 ? l : 350; for(usize k=0;k<max;k++) append_ch(line,&pos,sizeof(line),(char)p[st+k]); if(l>max) append_str(line,&pos,sizeof(line),"..."); append_str(line,&pos,sizeof(line),"'\n"); log_buf(line,(usize)pos);
        }
    }
}
static void log_ptr_line(const char* tag, const char* key, u64 a, u64 b, u64 c, u64 d) {
    char line[512]; int p=0;
    append_str(line,&p,sizeof(line),"[NextLib_LOGGER] "); append_str(line,&p,sizeof(line),tag); append_str(line,&p,sizeof(line)," cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"'");
    if(key) { append_str(line,&p,sizeof(line)," key='"); append_str(line,&p,sizeof(line),key); append_ch(line,&p,sizeof(line),'\''); }
    append_str(line,&p,sizeof(line)," a="); append_hex64(line,&p,sizeof(line),a);
    append_str(line,&p,sizeof(line)," b="); append_hex64(line,&p,sizeof(line),b);
    append_str(line,&p,sizeof(line)," c="); append_hex64(line,&p,sizeof(line),c);
    append_str(line,&p,sizeof(line)," d="); append_hex64(line,&p,sizeof(line),d);
    append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p);
}
static int useful_key(const char* key) {
    if(!key || !key[0] || (key[0] == '?' && key[1] == 0)) return 0;
    int good = 0;
    for(int i=0; key[i] && i<96; i++) {
        char c = key[i];
        if((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='_' || c=='-' || c=='.') good++;
        else return 0;
    }
    return good > 0;
}
static int useful_cmd(void) {
    return g_current_cmd[0] && !(g_current_cmd[0] == '?' && g_current_cmd[1] == 0);
}
static void set_current_cmd_clean(const char* src, usize n) {
    if(!src || n == 0 || n > 127) { g_current_cmd[0]='?'; g_current_cmd[1]=0; return; }
    for(usize i=0;i<n;i++) {
        char c = src[i];
        if(c < 32 || c > 126) { g_current_cmd[0]='?'; g_current_cmd[1]=0; return; }
        g_current_cmd[i] = c;
    }
    g_current_cmd[n] = 0;
}
static int parse_onmessage_cmd(const u8* data, u64 len, char* out, int outcap, u32* obj_type) {
    if(outcap <= 0) return 0;
    out[0] = '?'; out[1] = 0;
    if(obj_type) *obj_type = 0;
    if(!data || len < 3) return 0;
    u32 n = ((u32)data[0] << 8) | (u32)data[1];
    if(n == 0 || n > 127 || (u64)n + 2 > len) return 0;
    for(u32 i=0;i<n;i++) {
        char c = (char)data[2+i];
        if(c < 32 || c > 126) return 0;
        if(i + 1 < (u32)outcap) out[i] = c;
    }
    if(n < (u32)outcap) out[n] = 0; else out[outcap-1] = 0;
    set_current_cmd_clean((const char*)data + 2, n);
    if(obj_type && (u64)n + 2 < len) *obj_type = data[2+n];
    return 1;
}

static u16 read_be16_at(const u8* data, u64 len, u64* pos) {
    if(!data || !pos || *pos + 2 > len) return 0;
    u16 v = ((u16)data[*pos] << 8) | (u16)data[*pos + 1];
    *pos += 2;
    return v;
}
static u32 read_be32_at(const u8* data, u64 len, u64* pos) {
    if(!data || !pos || *pos + 4 > len) return 0;
    u32 v = ((u32)data[*pos] << 24) | ((u32)data[*pos + 1] << 16) | ((u32)data[*pos + 2] << 8) | (u32)data[*pos + 3];
    *pos += 4;
    return v;
}
static u64 read_be64_at(const u8* data, u64 len, u64* pos) {
    if(!data || !pos || *pos + 8 > len) return 0;
    u64 v = 0;
    for(int i=0;i<8;i++) v = (v << 8) | (u64)data[*pos + (u64)i];
    *pos += 8;
    return v;
}
static void fd_json_str(int fd, const char* s, usize n) {
    fd_write(fd, "\"", 1);
    for(usize i=0;i<n;i++) {
        char c = s ? s[i] : 0;
        if(c == '"' || c == '\\') {
            fd_write(fd, "\\", 1);
            fd_write(fd, &c, 1);
        } else if(c == '\n') {
            fd_write(fd, "\\n", 2);
        } else if(c == '\r') {
            fd_write(fd, "\\r", 2);
        } else if(c == '\t') {
            fd_write(fd, "\\t", 2);
        } else if((unsigned char)c < 32) {
            fd_write(fd, ".", 1);
        } else {
            fd_write(fd, &c, 1);
        }
    }
    fd_write(fd, "\"", 1);
}
static void fd_json_cstr(int fd, const char* s) { fd_json_str(fd, s ? s : "", slen(s)); }
static void fd_write_u64(int fd, u64 v) {
    char b[32]; int p=0;
    append_u64_dec(b,&p,sizeof(b),v);
    fd_write(fd,b,(usize)p);
}
static void fd_write_i64(int fd, i64 v) {
    char b[32]; int p=0;
    append_i64_dec(b,&p,sizeof(b),v);
    fd_write(fd,b,(usize)p);
}
static void fd_write_hex64_json(int fd, u64 v) {
    char b[32]; int p=0;
    append_hex64(b,&p,sizeof(b),v);
    fd_json_str(fd,b,(usize)p);
}
static void sanitize_filename(const char* in, char* out, int cap) {
    int p=0;
    if(cap <= 0) return;
    for(int i=0; in && in[i] && p<cap-1; i++) {
        char c = in[i];
        if((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='_' || c=='-') out[p++] = c;
        else out[p++] = '_';
    }
    if(p == 0 && cap > 1) out[p++] = 'x';
    out[p] = 0;
}

static int parse_sfs_value_json(int fd, const u8* data, u64 len, u64* pos, u8 type, int depth);

static int parse_sfs_object_json(int fd, const u8* data, u64 len, u64* pos, int depth) {
    if(depth > 24 || !pos || *pos + 2 > len) { fd_write(fd,"null",4); return 0; }
    u16 count = read_be16_at(data,len,pos);
    fd_write(fd,"{",1);
    for(u16 i=0;i<count;i++) {
        if(*pos + 3 > len) { if(i) fd_write(fd,",",1); fd_write(fd,"\"__truncated\":true",18); break; }
        u16 klen = read_be16_at(data,len,pos);
        if(*pos + (u64)klen + 1 > len) { if(i) fd_write(fd,",",1); fd_write(fd,"\"__truncated\":true",18); break; }
        if(i) fd_write(fd,",",1);
        fd_json_str(fd,(const char*)data + *pos,klen);
        *pos += klen;
        fd_write(fd,":",1);
        u8 type = data[*pos];
        *pos += 1;
        if(!parse_sfs_value_json(fd,data,len,pos,type,depth+1)) {
            fd_write(fd,",\"__parse_stopped\":true",23);
            break;
        }
    }
    fd_write(fd,"}",1);
    return 1;
}
static int parse_sfs_array_json(int fd, const u8* data, u64 len, u64* pos, int depth) {
    if(depth > 24 || !pos || *pos + 2 > len) { fd_write(fd,"null",4); return 0; }
    u16 count = read_be16_at(data,len,pos);
    fd_write(fd,"[",1);
    for(u16 i=0;i<count;i++) {
        if(*pos + 1 > len) { if(i) fd_write(fd,",",1); fd_write(fd,"{\"__truncated\":true}",20); break; }
        if(i) fd_write(fd,",",1);
        u8 type = data[*pos];
        *pos += 1;
        if(!parse_sfs_value_json(fd,data,len,pos,type,depth+1)) {
            fd_write(fd,",{\"__parse_stopped\":true}",25);
            break;
        }
    }
    fd_write(fd,"]",1);
    return 1;
}
static int parse_sfs_primitive_array_json(int fd, const u8* data, u64 len, u64* pos, u8 type) {
    if(!pos) { fd_write(fd,"[]",2); return 0; }
    if(type == 0x0a) {
        if(*pos + 4 > len) { fd_write(fd,"[]",2); return 0; }
        u32 byte_count = read_be32_at(data,len,pos);
        fd_write(fd,"[",1);
        for(u32 i=0;i<byte_count;i++) {
            if(i) fd_write(fd,",",1);
            if(*pos + 1 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); break; }
            fd_write_i64(fd,(i64)(signed char)data[*pos]);
            *pos += 1;
        }
        fd_write(fd,"]",1);
        return 1;
    }
    if(*pos + 2 > len) { fd_write(fd,"[]",2); return 0; }
    u32 count = (u32)read_be16_at(data,len,pos);
    fd_write(fd,"[",1);
    for(u32 i=0;i<count;i++) {
        if(i) fd_write(fd,",",1);
        switch(type) {
            case 0x09: {
                if(*pos + 1 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                fd_write(fd,data[*pos] ? "true" : "false", data[*pos] ? 4 : 5);
                *pos += 1;
                break;
            }
            case 0x0b: {
                if(*pos + 2 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                fd_write_i64(fd,(i64)(short)read_be16_at(data,len,pos));
                break;
            }
            case 0x0c: {
                if(*pos + 4 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                fd_write_i64(fd,(i64)(int)read_be32_at(data,len,pos));
                break;
            }
            case 0x0d: {
                if(*pos + 8 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                fd_write_i64(fd,(i64)read_be64_at(data,len,pos));
                break;
            }
            case 0x0e: {
                if(*pos + 4 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                u32 bits = read_be32_at(data,len,pos);
                fd_write(fd,"{\"__float_bits\":",16); fd_write_hex64_json(fd,(u64)bits); fd_write(fd,"}",1);
                break;
            }
            case 0x0f: {
                if(*pos + 8 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                u64 bits = read_be64_at(data,len,pos);
                fd_write(fd,"{\"__double_bits\":",17); fd_write_hex64_json(fd,bits); fd_write(fd,"}",1);
                break;
            }
            case 0x10: {
                if(*pos + 2 > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                u16 sl = read_be16_at(data,len,pos);
                if(*pos + (u64)sl > len) { fd_write(fd,"null",4); fd_write(fd,",{\"__truncated\":true}",21); i=count; break; }
                fd_json_str(fd,(const char*)data + *pos,sl);
                *pos += sl;
                break;
            }
            default:
                fd_write(fd,"null",4);
                break;
        }
    }
    fd_write(fd,"]",1);
    return 1;
}
static int parse_sfs_value_json(int fd, const u8* data, u64 len, u64* pos, u8 type, int depth) {
    if(!pos || *pos > len) { fd_write(fd,"null",4); return 0; }
    switch(type) {
        case 0x00: fd_write(fd,"null",4); return 1;
        case 0x01: {
            if(*pos + 1 > len) { fd_write(fd,"null",4); return 0; }
            fd_write(fd,data[*pos] ? "true" : "false", data[*pos] ? 4 : 5);
            *pos += 1;
            return 1;
        }
        case 0x02: {
            if(*pos + 1 > len) { fd_write(fd,"null",4); return 0; }
            fd_write_i64(fd,(i64)(signed char)data[*pos]);
            *pos += 1;
            return 1;
        }
        case 0x03: fd_write_i64(fd,(i64)(short)read_be16_at(data,len,pos)); return 1;
        case 0x04: fd_write_i64(fd,(i64)(int)read_be32_at(data,len,pos)); return 1;
        case 0x05: fd_write_i64(fd,(i64)read_be64_at(data,len,pos)); return 1;
        case 0x06: {
            u32 bits = read_be32_at(data,len,pos);
            fd_write(fd,"{\"__float_bits\":",16); fd_write_hex64_json(fd,(u64)bits); fd_write(fd,"}",1);
            return 1;
        }
        case 0x07: {
            u64 bits = read_be64_at(data,len,pos);
            fd_write(fd,"{\"__double_bits\":",17); fd_write_hex64_json(fd,bits); fd_write(fd,"}",1);
            return 1;
        }
        case 0x08: {
            if(*pos + 2 > len) { fd_write(fd,"null",4); return 0; }
            u16 sl = read_be16_at(data,len,pos);
            if(*pos + (u64)sl > len) { fd_write(fd,"null",4); return 0; }
            fd_json_str(fd,(const char*)data + *pos,sl);
            *pos += sl;
            return 1;
        }
        case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: case 0x10: {
            return parse_sfs_primitive_array_json(fd,data,len,pos,type);
        }
        case 0x11: return parse_sfs_array_json(fd,data,len,pos,depth+1);
        case 0x12: return parse_sfs_object_json(fd,data,len,pos,depth+1);
        default:
            fd_write(fd,"{\"__unknown_type\":",18); fd_write_u64(fd,(u64)type); fd_write(fd,"}",1);
            return 0;
    }
}
static void save_sfs_json_payload(const char* cmd, const u8* data, u64 len) {
    static u64 seq = 0;
    if(!cmd || !data || len < 3) return;
    ensure_log_dirs();
    char safe[96]; sanitize_filename(cmd,safe,sizeof(safe));
    char path[512]; int p=0;
    append_str(path,&p,sizeof(path),"/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/msm_json/");
    append_u64_dec(path,&p,sizeof(path),++seq);
    append_ch(path,&p,sizeof(path),'_');
    append_str(path,&p,sizeof(path),safe);
    append_str(path,&p,sizeof(path),".json");
    int fd = open_path_trunc(path);
    if(fd < 0) return;
    fd_write(fd,"{\"cmd\":",7); fd_json_cstr(fd,cmd);
    fd_write(fd,",\"len\":",7); fd_write_u64(fd,len);
    fd_write(fd,",\"payload\":",11);
    u64 pos = 0;
    u16 clen = read_be16_at(data,len,&pos);
    if(pos + (u64)clen + 1 <= len) {
        pos += clen;
        u8 type = data[pos++];
        parse_sfs_value_json(fd,data,len,&pos,type,0);
        if(pos < len) {
            fd_write(fd,",\"__remaining_bytes\":",21);
            fd_write_u64(fd,len-pos);
        }
    } else {
        fd_write(fd,"null",4);
    }
    fd_write(fd,"}\n",2);
    sc1(SYS_CLOSE, fd);

    char summary[640]; int sp=0;
    append_str(summary,&sp,sizeof(summary),"[NextLib_SUMMARY] JSON ");
    append_str(summary,&sp,sizeof(summary),cmd);
    append_str(summary,&sp,sizeof(summary)," -> ");
    append_str(summary,&sp,sizeof(summary),path);
    append_ch(summary,&sp,sizeof(summary),'\n');
    log_summary(summary);
}

static void flush_icache(void* addr, usize len) {
    u64 start = ((u64)addr) & ~((u64)63);
    u64 end = ((u64)addr + len + 63) & ~((u64)63);
    for(u64 p=start; p<end; p+=64) asm volatile("dc cvau, %0" :: "r"(p) : "memory");
    asm volatile("dsb ish" ::: "memory");
    for(u64 p=start; p<end; p+=64) asm volatile("ic ivau, %0" :: "r"(p) : "memory");
    asm volatile("dsb ish\nisb" ::: "memory");
}
static void write_abs_jump(void* at, void* to) {
    u32* ins=(u32*)at;
    ins[0] = 0x58000050; 
    ins[1] = 0xd61f0200; 
    *((u64*)((u8*)at + 8)) = (u64)to;
}

struct Hook { const char* name; u64 off; void* repl; void* tramp; u8 orig[16]; int installed; };

typedef void (*Fn_OnMessage)(void*, const u8*, u64);
typedef void (*Fn_OnExt)(void*, void*, void*, void*);
typedef void (*Fn_GotMsg)(void*, void*);
typedef void* (*Fn_GetBase)(void*, void*);
typedef int (*Fn_GetInt)(void*, void*, int);
typedef short (*Fn_GetShort)(void*, void*, short);
typedef long long (*Fn_GetLong)(void*, void*, long long);
typedef int (*Fn_GetBool)(void*, void*, int);
typedef float (*Fn_GetFloat)(void*, void*, float);
typedef double (*Fn_GetDouble)(void*, void*, double);
typedef void (*Fn_LoadTick)(void*, float);
typedef void (*Fn_GameStartupMsg)(void*, void*);

static Fn_OnMessage orig_OnMessage = 0;
static Fn_OnExt orig_OnExt = 0;
static Fn_GotMsg orig_GotMsg = 0;
static Fn_GetBase orig_GetBase = 0;
static Fn_GetInt orig_GetInt = 0;
static Fn_GetShort orig_GetShort = 0;
static Fn_GetLong orig_GetLong = 0;
static Fn_GetBool orig_GetBool = 0;
static Fn_GetFloat orig_GetFloat = 0;
static Fn_GetDouble orig_GetDouble = 0;
static Fn_GetLong orig_GetIntegerNumber = 0;
static Fn_GetDouble orig_GetRealNumber = 0;
static Fn_LoadTick orig_LoadTick = 0;
static Fn_GameStartupMsg orig_GotMsgStartLoad = 0;
static Fn_GameStartupMsg orig_GotMsgLoadContext = 0;
static Fn_GameStartupMsg orig_GsChangeIsland = 0;
static u32 read_u32_unchecked(const void* p);

static void log_gamestartup_state(const char* label, void* self, void* msg, const char* phase) {
    if(!readable(self, 0xe0)) return;
    u8* base = (u8*)self;
    char line[768]; int p=0;
    append_str(line,&p,sizeof(line),"[NextLib_LOGGER] ");
    append_str(line,&p,sizeof(line),label);
    append_str(line,&p,sizeof(line),".");
    append_str(line,&p,sizeof(line),phase);
    append_str(line,&p,sizeof(line)," self=");
    append_hex64(line,&p,sizeof(line),(u64)self);
    append_str(line,&p,sizeof(line)," msg=");
    append_hex64(line,&p,sizeof(line),(u64)msg);
    append_str(line,&p,sizeof(line)," load_flag=");
    append_u64_dec(line,&p,sizeof(line),(u64)base[0x38]);
    append_str(line,&p,sizeof(line)," context_state=");
    append_u64_dec(line,&p,sizeof(line),(u64)read_u32_unchecked(base + 0x3c));
    append_str(line,&p,sizeof(line)," start_kind=");
    append_u64_dec(line,&p,sizeof(line),(u64)read_u32_unchecked(base + 0xa8));
    append_ch(line,&p,sizeof(line),'\n');
    log_buf(line,(usize)p);
}

static void hook_GotMsgStartLoad(void* self, void* msg) {
    if(!g_log_guard) { g_log_guard=1; refresh_maps(); log_gamestartup_state("GameStartup.gotMsgStartLoad", self, msg, "before"); g_log_guard=0; }
    if(orig_GotMsgStartLoad) orig_GotMsgStartLoad(self, msg);
    if(!g_log_guard) { g_log_guard=1; refresh_maps(); log_gamestartup_state("GameStartup.gotMsgStartLoad", self, msg, "after"); g_log_guard=0; }
}

static void hook_GotMsgLoadContext(void* self, void* msg) {
    if(!g_log_guard) { g_log_guard=1; refresh_maps(); log_gamestartup_state("GameStartup.gotMsgLoadContext", self, msg, "before"); g_log_guard=0; }
    if(orig_GotMsgLoadContext) orig_GotMsgLoadContext(self, msg);
    if(!g_log_guard) { g_log_guard=1; refresh_maps(); log_gamestartup_state("GameStartup.gotMsgLoadContext", self, msg, "after"); g_log_guard=0; }
}

static void hook_GsChangeIsland(void* self, void* msg) {
    if(!g_log_guard) {
        g_log_guard=1;
        refresh_maps();
        char line[512]; int p=0;
        append_str(line,&p,sizeof(line),"[NextLib_LOGGER] NetworkHandler.gsChangeIsland.before self=");
        append_hex64(line,&p,sizeof(line),(u64)self);
        append_str(line,&p,sizeof(line)," msg=");
        append_hex64(line,&p,sizeof(line),(u64)msg);
        append_ch(line,&p,sizeof(line),'\n');
        log_buf(line,(usize)p);
        g_log_guard=0;
    }
    if(orig_GsChangeIsland) orig_GsChangeIsland(self, msg);
    if(!g_log_guard) {
        g_log_guard=1;
        char line[512]; int p=0;
        append_str(line,&p,sizeof(line),"[NextLib_LOGGER] NetworkHandler.gsChangeIsland.after self=");
        append_hex64(line,&p,sizeof(line),(u64)self);
        append_str(line,&p,sizeof(line)," msg=");
        append_hex64(line,&p,sizeof(line),(u64)msg);
        append_ch(line,&p,sizeof(line),'\n');
        log_buf(line,(usize)p);
        g_log_guard=0;
    }
}

static u32 read_u32_unchecked(const void* p) { u32 v=0; mem_cpy(&v,p,4); return v; }
static u64 vector_count_8(void* self, u64 off) {
    u8* base = (u8*)self;
    if(!readable(base + off, 16)) return 0;
    u64 a = read_u64(base + off);
    u64 b = read_u64(base + off + 8);
    if(b < a) return 0;
    return (b - a) / 8;
}
static u64 vector_count_16(void* self, u64 off) {
    u8* base = (u8*)self;
    if(!readable(base + off, 16)) return 0;
    u64 a = read_u64(base + off);
    u64 b = read_u64(base + off + 8);
    if(b < a) return 0;
    return (b - a) / 16;
}
static void hook_LoadTick(void* self, float dt) {
    static void* last_self = 0;
    static u32 last_state = 0xffffffffU;
    static u32 last_pending = 0xffffffffU;
    static u64 last_index = 0xffffffffffffffffULL;
    static u64 last_prepare_count = 0xffffffffffffffffULL;
    static u64 last_resource_count = 0xffffffffffffffffULL;
    static u64 last_pending_count = 0xffffffffffffffffULL;
    static u32 quiet_ticks = 0;

    if(!g_log_guard) {
        g_log_guard = 1;
        refresh_maps();
        if(readable(self, 0x90)) {
            u8* base = (u8*)self;
            u64 index = read_u64(base + 0x78);
            u32 state = read_u32_unchecked(base + 0x80);
            u32 pending = read_u32_unchecked(base + 0x84);
            u64 prepare_count = vector_count_8(self, 0x30);
            u64 resource_count = vector_count_16(self, 0x48);
            u64 pending_count = vector_count_16(self, 0x60);
            quiet_ticks++;
            int changed = self != last_self || state != last_state || pending != last_pending || index != last_index ||
                          prepare_count != last_prepare_count || resource_count != last_resource_count ||
                          pending_count != last_pending_count || quiet_ticks >= 60;
            if(changed) {
                char line[768]; int p=0;
                append_str(line,&p,sizeof(line),"[NextLib_LOGGER] LoadContext.tick self=");
                append_hex64(line,&p,sizeof(line),(u64)self);
                append_str(line,&p,sizeof(line)," state=");
                append_u64_dec(line,&p,sizeof(line),(u64)state);
                append_str(line,&p,sizeof(line)," index=");
                append_u64_dec(line,&p,sizeof(line),index);
                append_str(line,&p,sizeof(line),"/");
                append_u64_dec(line,&p,sizeof(line),prepare_count);
                append_str(line,&p,sizeof(line)," pending=");
                append_u64_dec(line,&p,sizeof(line),(u64)pending);
                append_str(line,&p,sizeof(line)," resources=");
                append_u64_dec(line,&p,sizeof(line),resource_count);
                append_str(line,&p,sizeof(line)," queued=");
                append_u64_dec(line,&p,sizeof(line),pending_count);
                append_ch(line,&p,sizeof(line),'\n');
                log_buf(line,(usize)p);
                last_self = self;
                last_state = state;
                last_pending = pending;
                last_index = index;
                last_prepare_count = prepare_count;
                last_resource_count = resource_count;
                last_pending_count = pending_count;
                quiet_ticks = 0;
            }
        }
        g_log_guard = 0;
    }
    if(orig_LoadTick) orig_LoadTick(self, dt);
}

static void hook_OnMessage(void* self, const u8* data, u64 len) {
    if(!g_log_guard) {
        g_log_guard = 1;
        refresh_maps();
        char cmd[128]; u32 obj_type=0;
        int has_cmd = parse_onmessage_cmd(data, len, cmd, sizeof(cmd), &obj_type);
        char summary[512]; int sp=0;
        append_str(summary,&sp,sizeof(summary),"[NextLib_SUMMARY] IN cmd=");
        append_str(summary,&sp,sizeof(summary),has_cmd ? cmd : "?");
        append_str(summary,&sp,sizeof(summary)," len=");
        append_u64_dec(summary,&sp,sizeof(summary),len);
        append_str(summary,&sp,sizeof(summary)," obj=");
        append_hex64(summary,&sp,sizeof(summary),(u64)obj_type);
        append_ch(summary,&sp,sizeof(summary),'\n');
        log_summary(summary);
        if(has_cmd) save_sfs_json_payload(cmd, data, len);
        char line[640]; int p=0;
        append_str(line,&p,sizeof(line),"[NextLib_LOGGER] >>> OnMessage ");
        append_str(line,&p,sizeof(line),has_cmd ? "CMD='" : "RAW cmd='");
        append_str(line,&p,sizeof(line),cmd);
        append_str(line,&p,sizeof(line),"' self=");
        append_hex64(line,&p,sizeof(line),(u64)self);
        append_str(line,&p,sizeof(line)," len=");
        append_u64_dec(line,&p,sizeof(line),len);
        append_str(line,&p,sizeof(line)," obj_type=");
        append_hex64(line,&p,sizeof(line),(u64)obj_type);
        append_str(line,&p,sizeof(line)," data=");
        append_hex64(line,&p,sizeof(line),(u64)data);
        append_ch(line,&p,sizeof(line),'\n');
        log_buf(line,(usize)p);
        g_log_guard = 0;
    }
    if(orig_OnMessage) orig_OnMessage(self, data, len);
}
static void hook_OnExt(void* self, void* cmdStr, void* objOrSharedA, void* objOrSharedB) {
    if(!g_log_guard) {
        g_log_guard = 1;
        refresh_maps();
        set_current_cmd_from_string_ptr(cmdStr);
        char cmd[128]; if(!decode_libcpp_string(cmdStr, cmd, sizeof(cmd))) { cmd[0]='?'; cmd[1]=0; }
        char summary[512]; int sp=0;
        append_str(summary,&sp,sizeof(summary),"[NextLib_SUMMARY] EXT cmd=");
        append_str(summary,&sp,sizeof(summary),cmd);
        append_ch(summary,&sp,sizeof(summary),'\n');
        log_summary(summary);
        char line[768]; int p=0;
        append_str(line,&p,sizeof(line),"[NextLib_LOGGER] >>> OnExtensionResponse cmd='"); append_str(line,&p,sizeof(line),cmd); append_str(line,&p,sizeof(line),"' self="); append_hex64(line,&p,sizeof(line),(u64)self); append_str(line,&p,sizeof(line)," cmdStr="); append_hex64(line,&p,sizeof(line),(u64)cmdStr); append_str(line,&p,sizeof(line)," arg2="); append_hex64(line,&p,sizeof(line),(u64)objOrSharedA); append_str(line,&p,sizeof(line)," arg3="); append_hex64(line,&p,sizeof(line),(u64)objOrSharedB); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p);
        g_log_guard = 0;
    }
    if(orig_OnExt) orig_OnExt(self, cmdStr, objOrSharedA, objOrSharedB);
}
static void hook_GotMsg(void* self, void* msg) {
    if(!g_log_guard) {
        g_log_guard = 1;
        refresh_maps();
        char cmd[128]; cmd[0]='?'; cmd[1]=0;
        if(readable((u8*)msg + 0x10, 24) && decode_libcpp_string((u8*)msg + 0x10, cmd, sizeof(cmd))) { int i=0; while(cmd[i]&&i<127){ g_current_cmd[i]=cmd[i]; i++; } g_current_cmd[i]=0; }
        char summary[512]; int sp=0;
        append_str(summary,&sp,sizeof(summary),"[NextLib_SUMMARY] HANDLER cmd=");
        append_str(summary,&sp,sizeof(summary),cmd);
        append_ch(summary,&sp,sizeof(summary),'\n');
        log_summary(summary);
        char line[768]; int p=0;
        append_str(line,&p,sizeof(line),"[NextLib_LOGGER] >>> NetworkHandler::gotMsgOnExtensionResponse cmd='"); append_str(line,&p,sizeof(line),cmd); append_str(line,&p,sizeof(line),"' self="); append_hex64(line,&p,sizeof(line),(u64)self); append_str(line,&p,sizeof(line)," msg="); append_hex64(line,&p,sizeof(line),(u64)msg); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p);
        g_log_guard = 0;
    }
    if(orig_GotMsg) orig_GotMsg(self, msg);
}

static void* hook_GetBase(void* obj, void* keyStr) {
    char key[128]; key[0]='?'; key[1]=0;
    if(!g_log_guard) { g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0; }
    void* ret = orig_GetBase ? orig_GetBase(obj, keyStr) : 0;
    if(useful_key(key) && useful_cmd() && !g_log_guard) { g_log_guard=1; log_ptr_line("SFSObjectWrapper::get", key, (u64)obj, (u64)keyStr, (u64)ret, 0); g_log_guard=0; }
    return ret;
}
static int hook_GetInt(void* obj, void* keyStr, int defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    int ret = orig_GetInt ? orig_GetInt(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getInt cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' obj="); append_hex64(line,&p,sizeof(line),(u64)obj); append_str(line,&p,sizeof(line)," key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' def="); append_i64_dec(line,&p,sizeof(line),defv); append_str(line,&p,sizeof(line)," => "); append_i64_dec(line,&p,sizeof(line),ret); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static short hook_GetShort(void* obj, void* keyStr, short defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    short ret = orig_GetShort ? orig_GetShort(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getShort cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' def="); append_i64_dec(line,&p,sizeof(line),defv); append_str(line,&p,sizeof(line)," => "); append_i64_dec(line,&p,sizeof(line),ret); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static long long hook_GetLong(void* obj, void* keyStr, long long defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    long long ret = orig_GetLong ? orig_GetLong(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getLong cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' def="); append_i64_dec(line,&p,sizeof(line),defv); append_str(line,&p,sizeof(line)," => "); append_i64_dec(line,&p,sizeof(line),ret); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static long long hook_GetIntegerNumber(void* obj, void* keyStr, long long defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    long long ret = orig_GetIntegerNumber ? orig_GetIntegerNumber(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getIntegerNumber cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' def="); append_i64_dec(line,&p,sizeof(line),defv); append_str(line,&p,sizeof(line)," => "); append_i64_dec(line,&p,sizeof(line),ret); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static int hook_GetBool(void* obj, void* keyStr, int defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    int ret = orig_GetBool ? orig_GetBool(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getBool cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' def="); append_i64_dec(line,&p,sizeof(line),defv&1); append_str(line,&p,sizeof(line)," => "); append_i64_dec(line,&p,sizeof(line),ret&1); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static float hook_GetFloat(void* obj, void* keyStr, float defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    float ret = orig_GetFloat ? orig_GetFloat(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; u32 bits=0; mem_cpy(&bits,&ret,4); char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getFloat cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' => bits="); append_hex64(line,&p,sizeof(line),(u64)bits); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static double hook_GetDouble(void* obj, void* keyStr, double defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    double ret = orig_GetDouble ? orig_GetDouble(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; u64 bits=0; mem_cpy(&bits,&ret,8); char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getDouble cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' => bits="); append_hex64(line,&p,sizeof(line),bits); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}
static double hook_GetRealNumber(void* obj, void* keyStr, double defv) {
    char key[128]; key[0]='?'; key[1]=0; if(!g_log_guard){g_log_guard=1; decode_libcpp_string(keyStr,key,sizeof(key)); g_log_guard=0;}
    double ret = orig_GetRealNumber ? orig_GetRealNumber(obj,keyStr,defv) : defv;
    if(useful_key(key) && useful_cmd() && !g_log_guard){ g_log_guard=1; u64 bits=0; mem_cpy(&bits,&ret,8); char line[512]; int p=0; append_str(line,&p,sizeof(line),"[NextLib_LOGGER] SFS.getRealNumber cmd='"); append_str(line,&p,sizeof(line),g_current_cmd); append_str(line,&p,sizeof(line),"' key='"); append_str(line,&p,sizeof(line),key); append_str(line,&p,sizeof(line),"' => bits="); append_hex64(line,&p,sizeof(line),bits); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p); g_log_guard=0; }
    return ret;
}

static struct Hook g_hooks[] = {
    {"SFSTomcatClient::OnMessage", 0x123a410ULL, (void*)hook_OnMessage, 0, {0}, 0},
    {"SFSTomcatClient::OnExtensionResponse", 0x123b790ULL, (void*)hook_OnExt, 0, {0}, 0},
    {"NetworkHandler::gotMsgOnExtensionResponse", 0x0c57eccULL, (void*)hook_GotMsg, 0, {0}, 0},
    {"SFSObjectWrapper::get", 0x122d638ULL, (void*)hook_GetBase, 0, {0}, 0},
    {"SFSObjectWrapper::getBool", 0x122d67cULL, (void*)hook_GetBool, 0, {0}, 0},
    {"SFSObjectWrapper::getIntegerNumber", 0x122d744ULL, (void*)hook_GetIntegerNumber, 0, {0}, 0},
    {"SFSObjectWrapper::getFloat", 0x122d7fcULL, (void*)hook_GetFloat, 0, {0}, 0},
    {"SFSObjectWrapper::getRealNumber", 0x122d8ccULL, (void*)hook_GetRealNumber, 0, {0}, 0},
    {"SFSObjectWrapper::getDouble", 0x122d9a4ULL, (void*)hook_GetDouble, 0, {0}, 0},
    {"SFSObjectWrapper::getShort", 0x122da7cULL, (void*)hook_GetShort, 0, {0}, 0},
    {"SFSObjectWrapper::getInt", 0x122db5cULL, (void*)hook_GetInt, 0, {0}, 0},
    {"SFSObjectWrapper::getLong", 0x122dc3cULL, (void*)hook_GetLong, 0, {0}, 0},
    {"NetworkHandler::gsChangeIsland", 0x0c8d0e0ULL, (void*)hook_GsChangeIsland, 0, {0}, 0},
    {"GameStartup::gotMsgStartLoad", 0x0c42490ULL, (void*)hook_GotMsgStartLoad, 0, {0}, 0},
    {"GameStartup::gotMsgLoadContext", 0x0c43254ULL, (void*)hook_GotMsgLoadContext, 0, {0}, 0},
    {"LoadContext::tick", 0x0dd29f8ULL, (void*)hook_LoadTick, 0, {0}, 0},
};

static void* make_trampoline(void* target, u8* original16) {
    void* mem = (void*)sc6(SYS_MMAP, 0, 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if((long)mem < 0) return 0;
    mem_cpy(mem, original16, 16);
    write_abs_jump((u8*)mem + 16, (u8*)target + 16);
    flush_icache(mem, 32);
    return mem;
}
static int install_one(struct Hook* h) {
    if(!g_lib_base) { log_hook_fail(h->name, "no-lib-base", 0, 0); return 0; }
    void* target = (void*)(g_lib_base + h->off);
    if(!readable(target, 16)) { log_hook_fail(h->name, "target-not-readable", target, 0); return 0; }
    if(!executable_addr(target)) { log_hook_fail(h->name, "target-not-executable-continuing", target, 0); }
    mem_cpy(h->orig, target, 16);
    h->tramp = make_trampoline(target, h->orig);
    if(!h->tramp) { log_hook_fail(h->name, "mmap-trampoline-failed", target, 0); return 0; }
    u64 page = ((u64)target) & ~((u64)0x3fff); 
    long mp = sc3(SYS_MPROTECT, (long)page, 0x8000, PROT_READ|PROT_WRITE|PROT_EXEC);
    if(mp < 0) { log_hook_fail(h->name, "mprotect-rwx-failed", target, mp); return 0; }
    write_abs_jump(target, h->repl);
    flush_icache(target, 16);
    sc3(SYS_MPROTECT, (long)page, 0x8000, PROT_READ|PROT_EXEC);
    h->installed = 1;
    return 1;
}

__attribute__((visibility("default"))) int nextlib_logger_install(void) {
    if(g_install_done) return 1;
    open_log_if_needed();
    refresh_maps();
    char line[512]; int p=0;
    append_str(line,&p,sizeof(line),"[NextLib_LOGGER] install start lib_base="); append_hex64(line,&p,sizeof(line),g_lib_base); append_ch(line,&p,sizeof(line),'\n'); log_buf(line,(usize)p);
    if(!g_lib_base) { log_raw("[NextLib_LOGGER] libmonsters not found. Load this .so AFTER libmonsters.so or call nextlib_logger_install() later.\n"); return 0; }
    int count = (int)(sizeof(g_hooks)/sizeof(g_hooks[0]));
    for(int i=0;i<count;i++) {
        void* target = (void*)(g_lib_base + g_hooks[i].off);
        if(!readable(target, 16)) {
            log_hook_fail(g_hooks[i].name, "precheck-target-not-readable-retry", target, 0);
            return 0;
        }
    }
    int ok_count = 0;
    for(int i=0;i<count;i++) {
        int ok = install_one(&g_hooks[i]);
        if(ok) ok_count++;
        char l[512]; int q=0;
        append_str(l,&q,sizeof(l),"[NextLib_LOGGER] hook "); append_str(l,&q,sizeof(l),ok?"OK ":"FAIL "); append_str(l,&q,sizeof(l),g_hooks[i].name); append_str(l,&q,sizeof(l)," target="); append_hex64(l,&q,sizeof(l),g_lib_base + g_hooks[i].off); append_str(l,&q,sizeof(l)," tramp="); append_hex64(l,&q,sizeof(l),(u64)g_hooks[i].tramp); append_ch(l,&q,sizeof(l),'\n'); log_buf(l,(usize)q);
    }
    if(ok_count != count) {
        log_raw("[NextLib_LOGGER] install incomplete; will retry later.\n");
        return 0;
    }
    orig_OnMessage = (Fn_OnMessage)g_hooks[0].tramp;
    orig_OnExt = (Fn_OnExt)g_hooks[1].tramp;
    orig_GotMsg = (Fn_GotMsg)g_hooks[2].tramp;
    orig_GetBase = (Fn_GetBase)g_hooks[3].tramp;
    orig_GetBool = (Fn_GetBool)g_hooks[4].tramp;
    orig_GetIntegerNumber = (Fn_GetLong)g_hooks[5].tramp;
    orig_GetFloat = (Fn_GetFloat)g_hooks[6].tramp;
    orig_GetRealNumber = (Fn_GetDouble)g_hooks[7].tramp;
    orig_GetDouble = (Fn_GetDouble)g_hooks[8].tramp;
    orig_GetShort = (Fn_GetShort)g_hooks[9].tramp;
    orig_GetInt = (Fn_GetInt)g_hooks[10].tramp;
    orig_GetLong = (Fn_GetLong)g_hooks[11].tramp;
    orig_GsChangeIsland = (Fn_GameStartupMsg)g_hooks[12].tramp;
    orig_GotMsgStartLoad = (Fn_GameStartupMsg)g_hooks[13].tramp;
    orig_GotMsgLoadContext = (Fn_GameStartupMsg)g_hooks[14].tramp;
    orig_LoadTick = (Fn_LoadTick)g_hooks[15].tramp;
    g_install_done = 1;
    log_raw("[NextLib_LOGGER] install finished. Logging current-build SFS packets/responses, SFSObject reads, change-island handler, GameStartup load messages, and LoadContext state.\n");
    return 1;
}


__attribute__((visibility("default"))) int JNI_OnLoad(void* vm, void* reserved) {
    log_raw("[NextLib_LOGGER] JNI_OnLoad called. Installing logger now.\n");
    nextlib_logger_install();
    return 0x00010006;
}

static void* delayed_install_thread(void* arg) {
    (void)arg;
    for(int i=0; i<80 && !g_install_done; i++) {
        if(nextlib_logger_install()) break;
        struct timespec_min ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 250000000L;
        sc2(SYS_NANOSLEEP, (long)&ts, 0);
    }
    return 0;
}

__attribute__((constructor)) static void ctor(void) {
    open_log_if_needed();
    log_raw("[NextLib_LOGGER] constructor loaded. Starting delayed installer thread. V7 LoadContext build.\n");
    pthread_t_min thread = 0;
    if(pthread_create(&thread, 0, delayed_install_thread, 0) == 0) {
        pthread_detach(thread);
    } else {
        log_raw("[NextLib_LOGGER] pthread_create failed; trying immediate install only.\n");
        nextlib_logger_install();
    }
}
