/* 
 * pgm.c
 * Created: Wed Dec 15 09:45:27 1999 by tek@wiw.org
 * Revised: Sun Mar  5 17:04:01 2000 by tek@nehwon.local
 * Copyright 1999 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "steaghanmods.h"

#define PGM_MODULENAME "pgm"
#define PGM_MODULEDESC "PGM graymap"

typedef struct {
    file_t *file;
    u_int32_t w, h, dataoffset, maxintense;
    u_int8_t *data, type;
} wraphandle_t;

moduleinfo_t moduleinfo(void)
{
    moduleinfo_t mi = { PGM_MODULENAME, PGM_MODULEDESC, wrappermod, 0 };
    return mi;
}

#include "pnm-shared.c"

/* lines are max 70 characters, so we should be safe with this */
#define BUFLEN 81

void *wrapinit(file_t *file)
{
    wraphandle_t *p;
    int i;
    char buffer[BUFLEN];

    p = (wraphandle_t *)malloc(sizeof(wraphandle_t));
    if(p == NULL) return NULL;

    p->file = file;

    /* read header */
    gotonexttoken(p->file);

    readtoken(buffer, BUFLEN-1, p->file);
    if(strcmp(buffer, "P5") == 0)
        p->type = 5;
    else if(strcmp(buffer, "P2") == 0)
        p->type = 2;
    else
        return NULL;
    gotonexttoken(p->file);
    
    /* read width and height */
    readtoken(buffer, BUFLEN-1, p->file); p->w = atoi(buffer);
    gotonexttoken(p->file);
    readtoken(buffer, BUFLEN-1, p->file); p->h = atoi(buffer);

    if(p->w <= 0 || p->h <= 0) return NULL;

    gotonexttoken(p->file);

    readtoken(buffer, BUFLEN-1, p->file);
    p->maxintense = atoi(buffer);
    /* FIXME: we shouldn't ignore maximum intensity... */
    if(p->type == 5)
        p->dataoffset = (*p->file->tell)(p->file->handle);
    else {
        p->data = (u_int8_t *)malloc(p->w*p->h);
        if(p->data == NULL) return NULL;

        for(i = 0; i < p->w * p->h; i++) {
            readtoken(buffer, BUFLEN-1, p->file); gotonexttoken(p->file);
            p->data[i] = atoi(buffer);
        }
    }

    return (void *)p;
}

u_int32_t wraplen(void *p_)
{
    wraphandle_t *p = (wraphandle_t *)p_;
    return p->w*p->h;
}

u_int8_t wrapread(void *p_, u_int32_t pos)
{
    wraphandle_t *p = (wraphandle_t *)p_;
    u_int8_t c;

    if(p->type == 5) {
        (*p->file->read)(p->file->handle, pos+p->dataoffset, 1, &c);
        return c&1;
    } else if(p->type == 2) {
        return p->data[pos]&1;
    } else return 0;
}

void wrapwrite(void *p_, u_int32_t pos, u_int8_t value)
{
    wraphandle_t *p = (wraphandle_t *)p_;
    u_int8_t x;

    if(p->type == 5) {
        (*p->file->read)(p->file->handle, pos+p->dataoffset, 1, &x);
        x &= 0xFE;
        x |= value;
        (*p->file->write)(p->file->handle, pos+p->dataoffset, 1, &x);

    } else if(p->type == 2) {
        p->data[pos] = (p->data[pos]&0xFE)|(value&1);

    } else {
    }
    
    return;
}

void wrapclose(void *p_)
{
    wraphandle_t *p = (wraphandle_t *)p_;
    int i;
    FILE *fp;
    
    if(p->type == 2) {
        /* FIXME: it'd be nice not to do this */
        fp = fopen(p->file->filename, "w");

        fprintf(fp, "P2\n");
        fprintf(fp, "%d %d\n%d\n", p->w, p->h, p->maxintense);
        for(i = 0; i < p->w*p->h; i++) {
            if((i%16) == 0) fprintf(fp, "\n");
            fprintf(fp, "%d ", p->data[i]);
        }

        free(p->data);
        fclose(fp);
    }
    
    free(p);
    
    return;
}

/* EOF pgm.c */
