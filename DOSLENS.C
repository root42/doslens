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

#define SIN_SIZE 1024
#define COS_OFF 256

long SIN[ SIN_SIZE + COS_OFF ];
long *COS = SIN + COS_OFF;

#define LENS_SIZE 80
#define LENS_R (LENS_SIZE>>1)
#define LENS_ZOOM 16

struct image *img = NULL;
int lens[LENS_SIZE][LENS_SIZE];
byte backup[LENS_SIZE*LENS_SIZE];
int lens_x, lens_y;

void init_sin()
{
    int i;
    double v;
    for(i = 0; i < SIN_SIZE + COS_OFF; ++i) {
        v = sin(2.0 * M_PI * i / (double)SIN_SIZE);
        SIN[i] = TO_FIX( v );
    }
}

void liss(int *x, int *y, long t)
{
    const long scale_x = TO_FIX(110);
    const long scale_y = TO_FIX(60);
    *x = 120 + TO_LONG(fix_mul(COS[(t*2+64) % SIN_SIZE], scale_x));
    *y = 60 + TO_LONG(fix_mul(COS[(t*3) % SIN_SIZE], scale_y));
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

void init_pal()
{
    int i;
    for(i = 0; i < 64; ++i) {
	img->palette[i+64][0] = img->palette[i][0];
	img->palette[i+64][1] = img->palette[i][1];
	img->palette[i+64][2] = MIN(img->palette[i][2] + 16, 63);
    }
    set_palette((byte *)img->palette);
}

void init_lens()
{
    float d = LENS_ZOOM;
    int x, y, x2, y2, r2 = LENS_R * LENS_R, ix, iy, offset;
    const int LS = LENS_SIZE / 2;
    for(y = 0; y <= LS; ++y) {
	y2 = y*y;
	for(x = 0; x <= LS; ++x) {
	    x2 = x*x;
	    if( x2 + y2 < r2 ) {
		float shift = d / sqrt(d*d - (x2 + y2 - r2));
		ix = x * (shift - 1);
		iy = y * (shift - 1);
		offset = iy * SCREEN_WIDTH + ix;
		lens[LS - y][LS - x] = -offset;
		lens[LS + y][LS + x] =  offset;
		offset = -iy * SCREEN_WIDTH + ix;
		lens[LS + y][LS - x] = -offset;
		lens[LS - y][LS + x] =  offset;
	    } else {
		lens[LS - y][LS - x] = INT_MAX;
		lens[LS + y][LS + x] = INT_MAX;
		lens[LS + y][LS - x] = INT_MAX;
		lens[LS - y][LS + x] = INT_MAX;
	    }
	}
    }
    memcpy(VGA, img->data, 64000);
    liss(&lens_x, &lens_y, 0);
    backup_rect(backup, LENS_SIZE, LENS_SIZE, VGA, lens_x, lens_y, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void draw_lens(long t)
{
    int temp, x, y, off;
    restore_rect(backup, LENS_SIZE, LENS_SIZE, VGA, lens_x, lens_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    liss(&lens_x, &lens_y, t);
    backup_rect(backup, LENS_SIZE, LENS_SIZE, VGA, lens_x, lens_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    temp = lens_x + lens_y * SCREEN_WIDTH;
    for(y = 0; y < LENS_SIZE; ++y, temp += SCREEN_WIDTH) {
	for(x = 0; x < LENS_SIZE; ++x) {
	    off = lens[y][x];
	    if(off == INT_MAX) continue;
	    VGA[temp + x] = img->data[temp + x + off] + 64;
	}
    }
}

int main()
{
    long t=1;
    char kc=0;

    img = load_gif("LENSPIC.GIF", 0);
    if(img == NULL ) {
        printf("couldn't load image\n");
        return 1;
    }
    init_sin();
    set_graphics_mode();
    init_pal();
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