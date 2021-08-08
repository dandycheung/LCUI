/* settings.c -- global settings.
 *
 * Copyright (c) 2020, James Duong <duong.james@gmail.com> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of LCUI nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <LCUI_Build.h>
#include <LCUI/types.h>
#include <LCUI/util.h>
#include <LCUI/settings.h>

static lcui_settings_t lcui_settings;

/* Initialize settings with the current global settings. */
void lcui_get_settings(lcui_settings_t *settings)
{
	*settings = lcui_settings;
}

/* Update global settings with the given input. */
void lcui_apply_settings(lcui_settings_t *settings)
{
	lcui_settings = *settings;
	lcui_settings.frame_rate_cap = y_max(lcui_settings.frame_rate_cap, 1);
	lcui_settings.parallel_rendering_threads =
	    y_max(lcui_settings.parallel_rendering_threads, 1);
}

/* Reset global settings to their defaults. */
void lcui_reset_settings(void)
{
	lcui_settings_t settings = {
		.frame_rate_cap = LCUI_MAX_FRAMES_PER_SEC,
		.parallel_rendering_threads = 4,
		.record_profile = FALSE,
		.fps_meter = FALSE,
		.paint_flashing = FALSE
	};
	lcui_apply_settings(&settings);
}
