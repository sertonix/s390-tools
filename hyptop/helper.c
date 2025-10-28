/*
 * hyptop - Show hypervisor performance data on System z
 *
 * Helper functions
 *
 * Copyright IBM Corp. 2010, 2017
 *
 * s390-tools is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <iconv.h>
#include <limits.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "lib/util_fmt.h"
#include "lib/util_libc.h"

#include "helper.h"
#include "hyptop.h"
#include "sd.h"

/*
 * Globals
 */
static iconv_t	l_iconv_ebcdic_ascii;
static int	l_underline_cnt;
static int	l_reverse_cnt;
static int	l_bold_cnt;

/*
 * Print time of day
 */
void ht_print_time(void)
{
	char time_str[40];
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
	hyptop_printf("%s", time_str);
}

/*
 * Alloc uninitialized memory and exit on failure
 */
void *ht_alloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr)
		ERR_EXIT("Out of memory (%zu Kb)", size / 1024);
	return ptr;
}

/*
 * Alloc memory initialized with "0" and exit on failure
 */
void *ht_zalloc(size_t size)
{
	void *ptr;

	ptr = calloc(1, size);
	if (!ptr)
		ERR_EXIT("Out of memory (%zu Kb)", size / 1024);
	return ptr;
}

/*
 * Realloc memory and exit on failure
 */
void *ht_realloc(void *old_ptr, size_t size)
{
	void *ptr;

	assert(size != 0);
	if (old_ptr)
		ptr = realloc(old_ptr, size);
	else
		ptr = calloc(1, size);
	if (!ptr)
		ERR_EXIT("Out of memory (%lu Kb)", (unsigned long) size / 1024);
	return ptr;
}

/*
 * Convert EBCDIC string to ASCII
 */
void ht_ebcdic_to_ascii(char *in, char *out, size_t size)
{
	size_t size_out = size;
	size_t size_in = size;
	size_t rc;

	rc = iconv(l_iconv_ebcdic_ascii, &in, &size_in, &out, &size_out);
	if (rc == (size_t) -1)
		ERR_EXIT_ERRNO("Code page translation EBCDIC-ASCII failed");
}

/*
 * Get mount point for file system tye "fs_type"
 */
char *ht_mount_point_get(const char *fs_type)
{
	struct mntent *mntbuf;
	FILE *mounts;

	mounts = setmntent(_PATH_MOUNTED, "r");
	if (!mounts)
		ERR_EXIT_ERRNO("Could not find \"%s\" mount point", fs_type);
	while ((mntbuf = getmntent(mounts)) != NULL) {
		if (strcmp(mntbuf->mnt_type, fs_type) == 0) {
			endmntent(mounts);
			return ht_strdup(mntbuf->mnt_dir);
		}
	}
	endmntent(mounts);
	return NULL;
}

/*
 * Remove all trailing blanks and reture pointer to first non blank character
 */
char *ht_strstrip(char *s)
{
	size_t size;
	char *end;

	size = strlen(s);

	if (!size)
		return s;

	end = s + size - 1;
	while (end >= s && isspace(*end))
		end--;
	*(end + 1) = '\0';

	while (*s && isspace(*s))
		s++;

	return s;
}

/*
 * Return copy of string
 */
char *ht_strdup(const char *str)
{
	char *rc;

	rc = ht_alloc(strlen(str) + 1);
	strcpy(rc, str);
	return rc;
}

/*
 * Print help icon in current line
 */
void ht_print_help_icon(void)
{
	hyptop_print_seek_back(6);
	ht_underline_on();
	hyptop_printf("?");
	ht_underline_off();
	hyptop_printf("=help");
}

/*
 * Print headline
 */
void ht_print_head(const char *sys)
{
	struct sd_cpu_type *cpu_type;
	int i;

	ht_print_time();
	hyptop_printf(" ");
	if (sys) {
		ht_bold_on();
		hyptop_printf("%s", sys);
		ht_bold_off();
		hyptop_printf(" ");
	}
	hyptop_printf("cpu-");
	ht_underline_on();
	hyptop_printf("t");
	ht_underline_off();
	hyptop_printf(": ");

	sd_cpu_type_iterate(cpu_type, i) {
		if (!sd_cpu_type_selected(cpu_type))
			continue;
		hyptop_printf("%s(%i) ", sd_cpu_type_id(cpu_type),
			      sd_cpu_type_cpu_cnt(cpu_type));
	}
	ht_print_help_icon();
	hyptop_print_nl();
}

/*
 * Curses attribute functions
 */
static void ht_attr_on(int attr)
{
	if (g.o.batch_mode_specified)
		return;
	attron(attr);
}

static void ht_attr_off(int attr)
{
	if (g.o.batch_mode_specified)
		return;
	attroff(attr);
}

void ht_bold_on(void)
{
	if (l_bold_cnt == 0)
		ht_attr_on(A_BOLD);
	l_bold_cnt++;
}

void ht_bold_off(void)
{

	l_bold_cnt--;
	if (l_bold_cnt == 0)
		ht_attr_off(A_BOLD);
}

void ht_underline_on(void)
{
	if (l_underline_cnt == 0)
		ht_attr_on(A_UNDERLINE);
	l_underline_cnt++;
}

void ht_underline_off(void)
{
	l_underline_cnt--;
	if (l_underline_cnt == 0)
		ht_attr_off(A_UNDERLINE);
}

void ht_reverse_on(void)
{
	if (l_reverse_cnt == 0)
		ht_attr_on(A_REVERSE);
	l_reverse_cnt++;
}

void ht_reverse_off(void)
{
	l_reverse_cnt--;
	if (l_reverse_cnt == 0)
		ht_attr_off(A_REVERSE);
}

/*
 * Print scroll bar
 */
void ht_print_scroll_bar(int row_cnt, int row_start, int rows_add_top,
			     int rows_add_bottom, int can_scroll_up,
			     int can_scroll_down, int with_border)
{
	int row_cnt_displ, bar_len, start, i;
	double scale1, scale2;

	row_cnt_displ = MIN(row_cnt, g.c.row_cnt - rows_add_top
			    - rows_add_bottom);
	if (row_cnt_displ <= 0)
		return;
	/* scale1: Scaling factor virtual screen to physical screen */
	scale1 = ((double) row_cnt_displ) / ((double) row_cnt);
	/* scale2: Scaling factor physical screen to scroll bar size */
	scale2 = ((double) row_cnt_displ - 2) / row_cnt_displ;
	bar_len = MAX(((double) row_cnt_displ * scale1 * scale2 + 0.5), 1);
	/* start: Start row in scroll bar */
	start = ((double) row_start) * scale1 * scale2 + 0.5;

	if (row_cnt_displ - 2 - start < bar_len)
		start = row_cnt_displ - 2 - bar_len;

	ht_reverse_on();

	if (with_border) {
		ht_underline_on();
		hyptop_printf_pos(rows_add_top - 1, g.c.col_cnt - 1, " ");
		ht_underline_off();
		hyptop_printf_pos(row_cnt_displ + rows_add_top,
				  g.c.col_cnt - 1, " ");
	}

	ht_underline_on();
	if (can_scroll_up) {
		ht_bold_on();
		hyptop_printf_pos(rows_add_top, g.c.col_cnt - 1, "^");
		ht_bold_off();
	} else {
		hyptop_printf_pos(rows_add_top, g.c.col_cnt - 1, "^");
	}
	ht_underline_off();

	if (row_cnt_displ == 1)
		goto out;

	ht_underline_on();
	if (can_scroll_down) {
		ht_bold_on();
		hyptop_printf_pos(row_cnt_displ - 1 + rows_add_top,
				  g.c.col_cnt - 1, "v");
		ht_bold_off();
	} else {
		hyptop_printf_pos(row_cnt_displ - 1 + rows_add_top,
				  g.c.col_cnt - 1, "v");
	}
	ht_underline_off();

	if (row_cnt_displ == 2)
		goto out;

	for (i = 0; i < row_cnt_displ - 2; i++)
		hyptop_printf_pos(i + rows_add_top + 1, g.c.col_cnt - 1,
				  " ");
	ht_underline_on();
	hyptop_printf_pos(i + rows_add_top, g.c.col_cnt - 1, " ");
	ht_underline_off();

	ht_bold_on();
	for (i = 0; i < bar_len; i++) {
		if (i + start == row_cnt_displ - 3)
			ht_underline_on();
		hyptop_printf_pos(i + start + 1 + rows_add_top,
				  g.c.col_cnt - 1, "#");
		if (i + start == row_cnt_displ - 3)
			ht_underline_off();
	}
	ht_bold_off();
out:
	ht_reverse_off();
}

/*
 * Convert ext TOD to microseconds
 */
u64 ht_ext_tod_2_us(void *tod_ext)
{
	char *tod_ptr = tod_ext;
	u64 us, *tod1, *tod2;

	tod1 = (u64 *) tod_ptr;
	tod2 = (u64 *) &tod_ptr[8];
	us = *tod1 << 8;
	us |= *tod2 >> 58;
	us = us >> 12;

	return us;
}

/*
 * Initialize helper module
 */
void hyptop_helper_init(void)
{
	l_iconv_ebcdic_ascii = iconv_open("ISO-8859-1", "EBCDIC-US");
	if (l_iconv_ebcdic_ascii == (iconv_t) -1)
		ERR_EXIT("Could not initialize iconv\n");
}

/*
 * Calculate real SMT utilization
 * @core_us: core utilization in us
 * @thr_us: thread utilization in us
 * @mgm_us: management utilization in us
 * @thread_per_core: SMT thread count per core
 */
s64 ht_calculate_smt_util(u64 core_us, u64 thr_us, u64 mgm_us, int thread_per_core)
{
	s64 component1, component2, smt_us;
	double smt_factor = g.o.smt_factor;

	component1 = thread_per_core * core_us - thr_us;
	if (thread_per_core > 1)
		component1 /= smt_factor;
	component2 = thr_us - core_us;
	smt_us = G0(component1 + component2 + mgm_us);

	return smt_us;
}

/*
 * Add two new key value pairs containing the current time as UNIX epoch and formatted string to a
 * structured output object.
 */
void ht_fmt_time(void)
{
	struct timeval tv;
	struct tm *tm;
	char str[30];

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	if (!tm)
		return;
	util_fmt_pair(FMT_PERSIST, "time_epoch", "%lld", mktime(tm));
	strftime(str, sizeof(str), "%F %T%z", tm);
	util_fmt_pair(FMT_PERSIST | FMT_QUOTE, "time", "%s", str);
}

/*
 * Add a new object for available CPU types to a structured output object.
 */
void ht_fmt_cpu_types(void)
{
	struct sd_cpu_type *cpu_type;
	int i;

	util_fmt_obj_start(FMT_DEFAULT, "cputypes");
	sd_cpu_type_iterate(cpu_type, i) {
		char *cpu_type_str = sd_cpu_type_id(cpu_type);
		util_str_tolower(cpu_type_str);
		util_fmt_pair(FMT_PERSIST, cpu_type_str, "%i",
			      sd_cpu_type_cpu_cnt(cpu_type));
	}
	util_fmt_obj_end(); /* cpus{} */
}
