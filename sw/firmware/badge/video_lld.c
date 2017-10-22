#include "ch.h"
#include "hal.h"
#include "hal_spi.h"

#include "gfx.h"
#include "src/gdisp/gdisp_driver.h"

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#define VID_THD_NEXT_BUF	0xFFFFFFFF
#define VID_THD_EXIT		0xFFFFFFFE

#define VID_PIXELS_PER_LINE	160
#define VID_LINES_PER_FRAME	120

#define VID_CHUNK_LINES 24
#define VID_CHUNK (VID_PIXELS_PER_LINE * VID_CHUNK_LINES)

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

	chThdSetPriority (NORMALPRIO - 1);

	GDISP->p.x = 0;
	GDISP->p.y = 0;
	GDISP->p.cx = gdispGetWidth ();
	GDISP->p.cy = gdispGetHeight ();

	gdisp_lld_write_start (GDISP);

	p = vid_buf;
	vid_p = vid_buf + VID_CHUNK;

	/* Launch the reader thread */

	pThread = chThdCreateStatic (waFsThread, sizeof(waFsThread), NORMALPRIO,
	    fsThread, NULL);

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
	/*chThdRelease (pThread);*/

	pThread = NULL;

	/* Release memory */

	chHeapFree (vid_buf);

	return (0);
}
