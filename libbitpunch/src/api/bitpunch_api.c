/* -*- c-file-style: "cc-mode" -*- */
/*
 * Copyright (c) 2017, Jonathan Gramain <jonathan.gramain@gmail.com>. All
 * rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * The names of the bitpunch project contributors may not be used to
 *   endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/**
 * @file
 * @brief main API
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils/queue.h"
#include "api/bitpunch-structs.h"
#include "api/bitpunch_api.h"
#include "core/parser.h"
#include "core/ast.h"
#include "core/filter.h"
#include "core/browse.h"

#if defined DEBUG
int tracker_debug_mode = 0;
#endif

int
bitpunch_init(void)
{
    filter_class_declare_std();
    return 0;
}

void
bitpunch_cleanup(void)
{
}

static struct bitpunch_schema *
bitpunch_schema_new(void)
{
    struct bitpunch_schema *schema;

    schema = new_safe(struct bitpunch_schema);
    return schema;
}

static int
load_schema_common(struct bitpunch_schema *schema)
{
    if (-1 == bitpunch_parse_schema(schema)) {
        return -1;
    }
    if (-1 == bitpunch_compile_schema(schema)) {
        return -1;
    }
    return 0;
}

static int
schema_read_data_from_fd(struct bitpunch_schema *schema, int fd)
{
    char *buffer = NULL;
    ssize_t n_read;
    off_t cur_offset;
    char error_buf[256];

    error_buf[0] = '\0';
    buffer = malloc_safe(BITPUNCH_SCHEMA_MAX_LENGTH);
    cur_offset = 0;
    while (cur_offset < BITPUNCH_SCHEMA_MAX_LENGTH) {
        n_read = pread(fd, buffer + cur_offset,
                       BITPUNCH_SCHEMA_MAX_LENGTH - cur_offset,
                       cur_offset);
        if (-1 == n_read) {
            break ;
        }
        if (0 == n_read) {
            buffer = realloc_safe(buffer, cur_offset);
            schema->data = buffer;
            schema->data_length = cur_offset;
            return 0;
        }
        cur_offset += n_read;
    }
    if (cur_offset == BITPUNCH_SCHEMA_MAX_LENGTH) {
        snprintf(error_buf, sizeof error_buf,
                 "file too large: maximum is %d bytes",
                 BITPUNCH_SCHEMA_MAX_LENGTH);
    } else {
        strerror_r(errno, error_buf, sizeof error_buf);
    }
    fprintf(stderr, "error reading bitpunch schema file: %s", error_buf);
    free(buffer);
    return -1;
}

static int
schema_read_data_from_path(struct bitpunch_schema *schema, const char *path)
{
    char *path_dup;
    int fd;

    path_dup = strdup_safe(path);
    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        char error_buf[256];

        error_buf[0] = '\0';
        strerror_r(errno, error_buf, sizeof error_buf);
        fprintf(stderr, "error reading bitpunch schema file: %s", error_buf);
        free(path_dup);
        return -1;
    }
    if (-1 == schema_read_data_from_fd(schema, fd)) {
        close(fd);
        free(path_dup);
        return -1;
    }
    schema->file_path = path_dup;
    return 0;
}

int
bitpunch_schema_create_from_path(
    const char *path,
    struct bitpunch_schema **schemap)
{
    struct bitpunch_schema *schema;

    assert(NULL != path);
    assert(NULL != schemap);

    schema = bitpunch_schema_new();
    if (-1 == schema_read_data_from_path(schema, path)) {
        bitpunch_schema_free(schema);
        return -1;
    }
    if (-1 == load_schema_common(schema)) {
        bitpunch_schema_free(schema);
        return -1;
    }
    *schemap = schema;
    return 0;
}

int
bitpunch_schema_create_from_file_descriptor(
    int fd,
    struct bitpunch_schema **schemap)
{
    struct bitpunch_schema *schema;

    assert(-1 != fd);
    assert(NULL != schemap);

    schema = bitpunch_schema_new();
    if (-1 == schema_read_data_from_fd(schema, fd)) {
        bitpunch_schema_free(schema);
        return -1;
    }
    if (-1 == load_schema_common(schema)) {
        bitpunch_schema_free(schema);
        return -1;
    }
    *schemap = schema;
    return 0;
}

int
bitpunch_schema_create_from_buffer(
    const char *buf, size_t buf_size,
    struct bitpunch_schema **schemap)
{
    struct bitpunch_schema *schema;

    assert(NULL != schemap);

    schema = bitpunch_schema_new();
    schema->data = memcpy(malloc_safe(buf_size), buf, buf_size);
    schema->data_length = buf_size;
    if (-1 == load_schema_common(schema)) {
        bitpunch_schema_free(schema);
        return -1;
    }
    *schemap = schema;
    return 0;
}

int
bitpunch_schema_create_from_string(
    const char *str,
    struct bitpunch_schema **schemap)
{
    return bitpunch_schema_create_from_buffer(str, strlen(str), schemap);
}

void
bitpunch_schema_free(struct bitpunch_schema *schema)
{
    free(schema->data);
    free(schema->file_path);
    free(schema);
}

static int
open_data_source_from_fd(int fd,
                         struct bitpunch_file_source *fs)
{
    char *map;
    size_t map_length;

    map_length = lseek(fd, 0, SEEK_END);
    map = mmap(NULL, map_length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (NULL == map) {
        fprintf(stderr, "Unable to mmap binary file: %s\n",
                strerror(errno));
        return -1;
    }
    fs->fd = fd;
    fs->map = map;
    fs->map_length = map_length;

    fs->ds.ds_data = map;
    fs->ds.ds_data_length = map_length;

    return 0;
}

static int
data_source_close_file_path(struct bitpunch_data_source *ds)
{
    struct bitpunch_file_source *fs;

    fs = (struct bitpunch_file_source *)ds;
    if (-1 == munmap(fs->map, fs->map_length)) {
        return -1;
    }
    if (-1 == close(fs->fd)) {
        return -1;
    }
    free(fs->path);
    return 0;
}

int
bitpunch_data_source_create_from_file_path(
    const char *path,
    struct bitpunch_data_source **dsp)
{
    struct bitpunch_file_source *fs;
    int fd;

    assert(NULL != path);
    assert(NULL != dsp);

    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        fprintf(stderr, "Unable to open binary file %s: open failed: %s\n",
                path, strerror(errno));
        return -1;
    }
    fs = new_safe(struct bitpunch_file_source);
    fs->ds.backend.close = data_source_close_file_path;

    if (-1 == open_data_source_from_fd(fd, fs)) {
        fprintf(stderr, "Error loading binary file %s\n", path);
        (void)close(fd);
        free(fs);
        return -1;
    }
    fs->path = strdup_safe(path);

    *dsp = &fs->ds;
    return 0;
}

static int
data_source_close_file_descriptor(struct bitpunch_data_source *ds)
{
    struct bitpunch_file_source *fs;

    fs = (struct bitpunch_file_source *)ds;
    return munmap(fs->map, fs->map_length);
}

int
bitpunch_data_source_create_from_file_descriptor(
    int fd,
    struct bitpunch_data_source **dsp)
{
    struct bitpunch_file_source *fs;

    assert(-1 != fd);
    assert(NULL != dsp);

    fs = new_safe(struct bitpunch_file_source);
    fs->ds.backend.close = data_source_close_file_descriptor;

    if (-1 == open_data_source_from_fd(fd, fs)) {
        fprintf(stderr,
                "Error loading binary file from file descriptor fd=%d\n",
                fd);
        free(fs);
        return -1;
    }
    *dsp = &fs->ds;
    return 0;
}

static int
data_source_close_managed_memory(struct bitpunch_data_source *ds)
{
    free((char *)ds->ds_data);
    return 0;
}

int
bitpunch_data_source_create_from_memory(
    const char *data, size_t data_size, int manage_buffer,
    struct bitpunch_data_source **dsp)
{
    struct bitpunch_data_source *ds;

    ds = new_safe(struct bitpunch_data_source);
    if (manage_buffer) {
        ds->backend.close = data_source_close_managed_memory;
    }
    ds->ds_data = data;
    ds->ds_data_length = data_size;
    *dsp = ds;
    return 0;
}

int
bitpunch_data_source_close(struct bitpunch_data_source *ds)
{
    if (NULL == ds || NULL == ds->backend.close) {
        return 0;
    }
    if (0 != ds->backend.close(ds)) {
        return -1;
    }
    ds->ds_data = NULL;
    ds->ds_data_length = 0;
    return 0;
}

int
bitpunch_data_source_free(struct bitpunch_data_source *ds)
{
    if (-1 == bitpunch_data_source_close(ds))
        return -1;
    if (NULL != ds->box_cache) {
        box_cache_free(ds->box_cache);
    }
    free(ds);
    return 0;
}

int
bitpunch_resolve_expr(struct ast_node_hdl *expr, struct box *scope)
{
    return resolve_user_expr(expr, scope);
}

int
bitpunch_eval_expr(struct bitpunch_schema *schema,
                   struct bitpunch_data_source *ds,
                   const char *expr,
                   struct box *scope,
                   expr_value_t *valuep, expr_dpath_t *dpathp,
                   struct tracker_error **errp)
{
    struct ast_node_hdl *expr_node = NULL;
    struct parser_ctx *parser_ctx = NULL;
    bitpunch_status_t bt_ret;
    int ret = -1;

    assert(NULL != expr);

    if (-1 == bitpunch_parse_expr(expr, &expr_node, &parser_ctx)) {
        return -1;
    }
    if (NULL != schema && NULL != ds) {
        if (NULL == scope) {
            scope = box_new_from_file(schema, ds);
            if (NULL == scope) {
                goto end;
            }
        } else {
            box_acquire(scope);
        }
    } else {
        scope = NULL; // just in case
    }
    if (-1 == bitpunch_resolve_expr(expr_node, scope)) {
        goto end;
    }
    assert(ast_node_is_rexpr(expr_node));
    bt_ret = expr_evaluate(expr_node, scope, valuep, dpathp, errp);
    if (BITPUNCH_OK == bt_ret) {
        ret = 0;
    }

  end:
    box_delete(scope);
    /* TODO free expr_node */
    free(parser_ctx);
    return ret;
}


const char *
bitpunch_status_pretty(bitpunch_status_t bt_ret)
{
    switch (bt_ret) {
    case BITPUNCH_OK:
        return "success";
    case BITPUNCH_ERROR:
        return "error";
    case BITPUNCH_INVALID_PARAM:
        return "invalid parameter";
    case BITPUNCH_INVALID_STATE:
        return "invalid state";
    case BITPUNCH_NO_ITEM:
        return "no item";
    case BITPUNCH_NOT_CONTAINER:
        return "not a container";
    case BITPUNCH_DATA_ERROR:
        return "data error";
    case BITPUNCH_OUT_OF_BOUNDS_ERROR:
        return "out of data structure boundaries";
    case BITPUNCH_NOT_IMPLEMENTED:
        return "not implemented";
    default:
        return "unknown tracker status";
    }
    /*NOT REACHED*/
}
