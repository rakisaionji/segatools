#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "amex/config.h"

#include "chunihook/config.h"

#include "hooklib/config.h"

#include "platform/config.h"

void slider_config_load(struct slider_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"slider", L"enable", 1, filename);
}

void chuni_hook_config_load(
        struct chuni_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    memset(cfg, 0, sizeof(*cfg));

    nu_config_load(&cfg->nu, filename);
    amex_config_load(&cfg->amex, filename);
    gfx_config_load(&cfg->gfx, filename);
    slider_config_load(&cfg->slider, filename);
}
