/* Browser adapter. Including the unmodified session implementation gives this
 * adapter read-only access to the real arena registry and execution counters.
 * Build excludes that translation unit separately to avoid duplicate symbols. */
#include <emscripten/emscripten.h>
#include <math.h>
#include "../llfpl/src/interpreter/interpreter_session.c"

static LlfplInterpreterSession *browser_session;
static FILE *results;
static FILE *diagnostics;
static char text_buffer[65537];

EMSCRIPTEN_KEEPALIVE int browser_run(const char *source) {
    if (browser_session) { llfpl_interpreter_session_destroy(browser_session); browser_session=NULL; }
    if(results) fclose(results);
    if(diagnostics) fclose(diagnostics);
    results=tmpfile();diagnostics=tmpfile();
    if(!results || !diagnostics) return -1;
    FILE *program=fopen("/program.LLFPL","wb");
    if(!program) return -1;
    size_t length=strlen(source);
    if(length>32768) { fclose(program);return -1; }
    size_t written=fwrite(source,1,length,program);
    fclose(program);
    if(written!=length) return -1;
    LlfplInterpreterOptions options;
    llfpl_interpreter_options_initialise(&options);
    options.result_stream=results;options.diagnostic_stream=diagnostics;
    options.verbose_output_is_enabled=1;
    /* Physical CPU-register synchronization is not meaningful in a browser. */
    options.hardware_synchronisation_is_enabled=0;
    options.additional_module_directories[0]="/lib/llfpl";
    options.additional_module_directory_count=1;
    LlfplStatus status=llfpl_interpreter_session_create(&options,&browser_session);
    if(!llfpl_status_is_ok(status)) return -1;
    status=llfpl_interpreter_session_execute_source_file(browser_session,"/program.LLFPL");
    llfpl_interpreter_session_write_summary(browser_session,diagnostics);
    fflush(results);fflush(diagnostics);
    return llfpl_status_is_ok(status) && !llfpl_interpreter_session_error_count(browser_session) ? 0 : 1;
}
EMSCRIPTEN_KEEPALIVE const char *browser_output(int diagnostic) {
    FILE *stream=diagnostic?diagnostics:results;
    if(!stream) return "Runtime stream unavailable.";
    rewind(stream);size_t n=fread(text_buffer,1,65536,stream);text_buffer[n]=0;
    return text_buffer;
}
EMSCRIPTEN_KEEPALIVE double browser_stat(int kind) {
    if(!browser_session) return 0;
    if(kind==0) return browser_session->execution_environment.committed_expression_count;
    if(kind==1) return (double)llfpl_vector_register_file_retired_cycles(&browser_session->register_file);
    return (double)browser_session->execution_environment.total_elapsed_nanoseconds;
}
EMSCRIPTEN_KEEPALIVE int browser_arena_count(void) { return browser_session?(int)browser_session->arena_registry.arena_count:0; }
EMSCRIPTEN_KEEPALIVE const char *browser_arena_name(int i) {
    return browser_session && i>=0 && i<browser_arena_count()?browser_session->arena_registry.arenas[i].name:"";
}
EMSCRIPTEN_KEEPALIVE int browser_arena_size(int i) {
    return browser_session && i>=0 && i<browser_arena_count()?(int)browser_session->arena_registry.arenas[i].capacity_in_bytes:0;
}
EMSCRIPTEN_KEEPALIVE double browser_arena_value(int i,int slot) {
    double value=NAN;
    if(browser_session && i>=0 && i<browser_arena_count() && slot>=0 && slot<4)
        llfpl_memory_arena_load_double(&browser_session->arena_registry.arenas[i],(uint32_t)slot*8,&value);
    return value;
}
