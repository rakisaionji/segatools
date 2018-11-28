#include <windows.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "hook/pe.h"

#include "util/dprintf.h"
#include "util/spike.h"

/* Spike functions. Their "style" is named after the libc function they bear
   the closest resemblance to. */

static void spike_fn_puts(const char *msg)
{
    char line[512];

    sprintf_s(line, _countof(line), "%s\n", msg);
    OutputDebugStringA(line);
}

static void spike_fn_fputs(const char *msg)
{
    OutputDebugStringA(msg);
}

static void spike_fn_printf(const char *fmt, ...)
{
    char line[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf_s(line, _countof(line), fmt, ap);
    strcat(line, "\n");
    OutputDebugStringA(line);
}

static void spike_fn_vprintf(
        const char *proc,
        int line_no,
        const char *fmt,
        va_list ap)
{
    char msg[512];
    char line[512];

    vsprintf_s(msg, _countof(msg), fmt, ap);
    sprintf_s(line, _countof(line), "%s:%i: %s", proc, line_no, msg);
    OutputDebugStringA(line);
}

static void spike_fn_vwprintf(
        const wchar_t *proc,
        int line_no,
        const wchar_t *fmt,
        va_list ap)
{
    wchar_t msg[512];
    wchar_t line[512];

    vswprintf_s(msg, _countof(msg), fmt, ap);
    swprintf_s(line, _countof(line), L"%s:%i: %s", proc, line_no, msg);
    OutputDebugStringW(line);
}

static void spike_fn_perror(
        int a1,
        int a2,
        int error,
        const char *file,
        int line_no,
        const char *msg)
{
    char line[512];

    sprintf_s(
            line,
            _countof(line),
            "%s:%i:%08x: %s\n",
            file,
            line_no,
            error,
            msg);

    OutputDebugStringA(line);
}

/* Spike inserters */

static void spike_insert_jmp(ptrdiff_t rva, void *proc)
{
    uint8_t *base;
    uint8_t *target;
    uint8_t *func_ptr;
    uint32_t delta;

    base = (uint8_t *) GetModuleHandleW(NULL);

    target = base + rva;
    func_ptr = proc;
    delta = func_ptr - target - 4; /* -4: EIP delta, after end of target insn */

    pe_patch(target, &delta, sizeof(delta));
}

static void spike_insert_ptr(ptrdiff_t rva, void *ptr)
{
    uint8_t *base;
    uint8_t *target;

    base = (uint8_t *) GetModuleHandleW(NULL);
    target = base + rva;

    pe_patch(target, &ptr, sizeof(ptr));
}

static void spike_insert_log_levels(ptrdiff_t rva, size_t count)
{
    uint8_t *base;
    uint32_t *levels;
    size_t i;

    base = (uint8_t *) GetModuleHandleW(NULL);
    levels = (uint32_t *) (base + rva);

    for (i = 0 ; i < count ; i++) {
        levels[i] = 255;
    }
}

/* Config reader */

void spike_hook_init(const char *path)
{
    int match;
    int count;
    int rva;
    char line[80];
    char *ret;
    FILE *f;

    f = fopen(path, "r");

    if (f == NULL) {
        return;
    }

    dprintf("Found spike config, inserting spikes\n");

    for (;;) {
        ret = fgets(line, sizeof(line), f);

        if (ret == NULL) {
            break;
        }

        if (line[0] == '#' || line[0] == '\r' || line[0] == '\n') {
            continue;
        }

        match = sscanf(line, "levels %i %i", &rva, &count);

        if (match == 2) {
            spike_insert_log_levels((ptrdiff_t) rva, count);
        }

        match = sscanf(line, "j_vprintf %i", &rva);

        if (match == 1) {
            spike_insert_jmp((ptrdiff_t) rva, spike_fn_vprintf);
        }

        match = sscanf(line, "j_vwprintf %i", &rva);

        if (match == 1) {
            spike_insert_jmp((ptrdiff_t) rva, spike_fn_vwprintf);
        }

        match = sscanf(line, "j_printf %i", &rva);

        if (match == 1) {
            spike_insert_jmp((ptrdiff_t) rva, spike_fn_printf);
        }

        match = sscanf(line, "j_puts %i", &rva);

        if (match == 1) {
            spike_insert_jmp((ptrdiff_t) rva, spike_fn_puts);
        }

        match = sscanf(line, "j_perror %i", &rva);

        if (match == 1) {
            spike_insert_jmp((ptrdiff_t) rva, spike_fn_perror);
        }

        match = sscanf(line, "c_fputs %i", &rva); /* c == "callback" */

        if (match == 1) {
            spike_insert_ptr((ptrdiff_t) rva, spike_fn_fputs);
        }
    }

    dprintf("Spike insertion complete\n");
    fclose(f);
}
