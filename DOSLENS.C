#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <conio.h>
#include <dos.h>
#include <limits.h>
#include <math.h>
#include <mem.h>

#include "types.h"
#include "fix.h"
#include "gif.h"
#include "vga.h"
#include "pal.h"

#define SIN_SIZE 512
#define COS_OFF 128
/* Lens diameter and radius */
#define LENS_SIZE 80
#define LENS_R (LENS_SIZE>>1)
#define LENS_ZOOM 16

long SIN[ SIN_SIZE + COS_OFF ];
long *COS = SIN + COS_OFF;
struct image *img = NULL;
int lens[LENS_SIZE][LENS_SIZE];
byte backup[LENS_SIZE*LENS_SIZE];

int lens_x = 120, lens_y = 50;

void init_sin()
{
    int i;
    double v;
    for(i = 0; i < SIN_SIZE + COS_OFF; ++i) {
        v = sin(2.0 * M_PI * i / (double)SIN_SIZE);
        SIN[i] = TO_FIX( v );
    }
}

void liss(int *x, int *y, int t)
{
    const long scale = TO_FIX(50);
    *x = TO_LONG(fix_mul(COS[(t*2+64) % SIN_SIZE], scale));
    *y = TO_LONG(fix_mul(COS[(t*3) % SIN_SIZE], scale));
}

void backup_rect(byte *dst, int w, int h, byte *src, int src_x, int src_y, int src_w, int src_h)
{
    int i;
    (void)src_h;
    for(i = 0; i < h; ++i) {
        memcpy(dst + i * w, src + src_x + (i + src_y) * src_w, w);
    }
}

void restore_rect(byte *src, int w, int h, byte *dst, int dst_x, int dst_y, int dst_w, int dst_h)
{
    int i;
    (void)dst_h;
    for(i = 0; i < h; ++i) {
        memcpy(dst + dst_x + (i + dst_y) * dst_w, src + i * w, w);
    }
}

void init_lens()
{
    int x, y, x2, y2, ix, iy, r2 = LENS_R * LENS_R, offset, d = LENS_ZOOM;
    const int LS = LENS_SIZE/2;
    for(y = 0; y <= LS; ++y) {
        y2 = y * y;
	for(x = 0; x <= LS; ++x) {
            x2 = x * x;
            if( x2 + y2 < r2 ) {
		float shift = (float)d / sqrt(d*d - (x2 + y2 - r2));
                ix = x * shift - x;
                iy = y * shift - y;
                offset = (iy * SCREEN_WIDTH + ix);
                lens[LS - y][LS - x] = -offset;
                lens[LS + y][LS + x] = offset;
                offset = (-iy * SCREEN_WIDTH + ix);
                lens[LS + y][LS - x] = -offset;
                lens[LS - y][LS + x] = offset;
            } else {
		lens[LS - y][LS - x] = INT_MAX;
		lens[LS + y][LS + x] = INT_MAX;
		lens[LS + y][LS - x] = INT_MAX;
		lens[LS - y][LS + x] = INT_MAX;
            }

        }
    }
    memcpy(VGA, img->data, 64000);
    backup_rect(backup, LENS_SIZE, LENS_SIZE, VGA, lens_x, lens_y, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void draw_lens(long t)
{
    static dx = 1, dy = 1;
    byte col;
    int x, y, temp, pos;
    int off;
    restore_rect(backup, LENS_SIZE, LENS_SIZE, VGA, lens_x, lens_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    lens_x += dx; lens_y += dy;
    if(lens_x <= 0 || lens_x + LENS_SIZE >= SCREEN_WIDTH) dx = -dx;
    if(lens_y <= 0 || lens_y + LENS_SIZE >= SCREEN_HEIGHT) dy = -dy;
    backup_rect(backup, LENS_SIZE, LENS_SIZE, VGA, lens_x, lens_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    for(y = 0; y < LENS_SIZE; ++y) {
        temp = lens_x + (y + lens_y) * SCREEN_WIDTH;
        for(x = 0; x < LENS_SIZE; ++x) {
	    off = lens[y][x];
	    if(off == INT_MAX) continue;
            pos = temp + x;
	    col = img->data[pos + off];
	    if( col != 0 ) {
	        col = 64 + (col >> 2);
	    }
            /* col = 1; */
	    VGA[pos] = col;
        }
    }
}

int main()
{
    long t=0;
    char kc=0;
    /* byte far *buf=farmalloc(64000); */
    /* BUF=buf; */

    img = load_gif("LENSPIC.GIF", 0);
    if(img == NULL ) {
        printf("couldn't load image\n");
        return 1;
    }
    init_sin();
    set_graphics_mode();
    set_palette((byte *)img->palette);
    init_lens();
    while(kc!=0x1b) {
        if(kbhit()) kc=getch();
        wait_for_retrace();
        draw_lens(t);
	t++;
    }
    set_text_mode();

    return 0;
}
