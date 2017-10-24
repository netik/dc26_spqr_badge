/*-
 * Copyright (c) 2017
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ch.h"
#include "hal.h"
#include "hal_spi.h"

#include "gfx.h"
#include "src/gdisp/gdisp_driver.h"

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "video_lld.h"

static thread_t * pThread;

static pixel_t * vid_buf;

static FIL vid_f;
static volatile UINT vid_br;
static pixel_t * vid_p;

static thread_reference_t fsReference;

static THD_WORKING_AREA(waFsThread, 256);
static THD_FUNCTION(fsThread, arg)
{
	UINT br;
        (void) arg;

        chRegSetThreadName ("FsEvent");

        while (1) {
		osalSysLock ();
                vid_br = br;
                osalThreadSuspendS (&fsReference);
		osalSysUnlock ();
		if (vid_br == VID_THD_EXIT)
			break;
                f_read(&vid_f, vid_p, VID_CHUNK * 2, &br);
        	if (vid_p == vid_buf)
                	vid_p += VID_CHUNK;
        	else
                	vid_p = vid_buf;
        }

	chThdExit (MSG_OK);

        return;
}

int
videoPlay (char * fname)
{
	int i;
	int j;
	pixel_t * p;
	UINT br;

	vid_buf = chHeapAlloc (NULL, VID_CHUNK * 2);

	if (vid_buf == NULL)
 		return (-1);

	if (f_open(&vid_f, fname, FA_READ) != FR_OK) {
		chHeapFree (vid_buf);
		return (-1);
	}

	GDISP->p.x = 0;
	GDISP->p.y = 0;
	GDISP->p.cx = gdispGetWidth ();
	GDISP->p.cy = gdispGetHeight ();

	gdisp_lld_write_start (GDISP);

	p = vid_buf;
	vid_p = vid_buf + VID_CHUNK;

	/*
	 * Launch the reader thread
	 * Note: it should be at least one notch higher in priority
	 * that the current thread.
	 */

	pThread = chThdCreateStatic (waFsThread, sizeof(waFsThread),
            chThdGetPriorityX() + 1, fsThread, NULL);

	/* Pre-load initial chunk */

	f_read(&vid_f, p, VID_CHUNK * 2, &br);

	/* Now perform first async read */

        osalSysLock ();
        vid_br = VID_THD_NEXT_BUF;
        osalThreadResumeS (&fsReference, MSG_OK);
        osalSysUnlock ();

	while (1) {

		if (vid_br == 0)
			break;

		gptStartContinuous (&GPTD2, NRF5_GPT_FREQ_16MHZ);

		for (j = 0; j < VID_CHUNK_LINES; j++) {
			for (i = 0; i < 160; i++) {
				GDISP->p.color = p[i + (160 * j)];
				gdisp_lld_write_color(GDISP);
				gdisp_lld_write_color(GDISP);
			}
			for (i = 0; i < 160; i++) {
				GDISP->p.color = p[i + (160 * j)];
				gdisp_lld_write_color(GDISP);
				gdisp_lld_write_color(GDISP);
			}
		}

        	while (vid_br == VID_THD_NEXT_BUF)
                	chThdSleep (1);

		while (gptGetCounterX (&GPTD2) < VID_CHUNK_INTERVAL)
			chThdSleep (1);

		/* Switch to next waiting chunk */

		if (p == vid_buf)
			p += VID_CHUNK;
		else
			p = vid_buf;

		/* Start next async read */

		osalSysLock ();
		vid_br = 0xFFFFFFFF;
		osalThreadResumeS (&fsReference, MSG_OK);
		osalSysUnlock ();

	}

        gdisp_lld_write_stop (GDISP);
	f_close (&vid_f);

	/* Terminate the thread */

	osalSysLock ();
	vid_br = VID_THD_EXIT;
	osalThreadResumeS (&fsReference, MSG_OK);
	osalSysUnlock ();

	chThdWait (pThread);
	pThread = NULL;

	/* Stop the timer */

	gptStopTimer (&GPTD2);

	/* Release memory */

	chHeapFree (vid_buf);

	return (0);
}
