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

#ifndef __EXPR_INTERNAL_H__
#define __EXPR_INTERNAL_H__

#include "core/expr.h"

extern expr_dpath_t shared_expr_dpath_none;

#define EXPR_DPATH_NONE shared_expr_dpath_none

struct dpath_transform {
    expr_dpath_t dpath;
    int dpath_is_data_source;
};

bitpunch_status_t
expr_evaluate_internal(
    struct ast_node_hdl *expr, struct box *scope,
    enum expr_evaluate_flag flags,
    expr_value_t *valuep, expr_dpath_t *dpathp,
    struct browse_state *bst);

bitpunch_status_t
expr_evaluate_value_internal(
    struct ast_node_hdl *expr, struct box *scope,
    expr_value_t *valuep,
    struct browse_state *bst);

bitpunch_status_t
expr_evaluate_dpath_internal(
    struct ast_node_hdl *expr, struct box *scope,
    expr_dpath_t *dpathp,
    struct browse_state *bst);

bitpunch_status_t
expr_transform_dpath_internal(
    struct ast_node_hdl *expr, struct box *scope,
    struct dpath_transform *transformp,
    struct browse_state *bst);

bitpunch_status_t
dpath_read_value_internal(
    expr_dpath_t dpath,
    expr_value_t *expr_valuep,
    struct browse_state *bst);

enum filter_kind {
    FILTER_KIND_ITEM,
    FILTER_KIND_FILTER,
};

bitpunch_status_t
expr_evaluate_filter_type_internal(struct ast_node_hdl *filter,
                                   struct box *scope,
                                   enum filter_kind kind,
                                   struct ast_node_hdl **filter_typep,
                                   struct browse_state *bst);
bitpunch_status_t
evaluate_conditional_internal(struct ast_node_hdl *cond, struct box *scope,
                              int *evalp, struct browse_state *bst);

bitpunch_status_t
evaluate_scoped_statement_internal(
    struct box *scope,
    enum statement_type stmt_type, const struct named_statement *named_stmt,
    enum expr_evaluate_flag flags,
    expr_value_t *valuep, expr_dpath_t *dpathp,
    struct browse_state *bst);

bitpunch_status_t
expr_dpath_to_tracker_internal(expr_dpath_t dpath,
                               struct tracker **tkp,
                               struct browse_state *bst);
bitpunch_status_t
expr_dpath_to_box_internal(expr_dpath_t dpath,
                           struct box **boxp,
                           struct browse_state *bst);
bitpunch_status_t
expr_dpath_to_box_direct(expr_dpath_t dpath,
                         struct box **boxp,
                         struct browse_state *bst);
bitpunch_status_t
expr_dpath_to_container_internal(expr_dpath_t dpath,
                                 expr_dpath_t *dpathp,
                                 struct browse_state *bst);
bitpunch_status_t
expr_dpath_to_item_internal(expr_dpath_t dpath,
                            expr_dpath_t *dpathp,
                            struct browse_state *bst);
bitpunch_status_t
expr_dpath_to_dpath_internal(expr_dpath_t src_dpath,
                             enum expr_dpath_type dst_type,
                             expr_dpath_t *dst_dpathp,
                             struct browse_state *bst);
struct box *
expr_dpath_get_parent_box(expr_dpath_t dpath);

bitpunch_status_t
expr_dpath_get_size_internal(expr_dpath_t dpath,
                             int64_t *dpath_sizep,
                             struct browse_state *bst);
bitpunch_status_t
expr_dpath_get_location_internal(expr_dpath_t dpath,
                                 int64_t *offsetp, int64_t *sizep,
                                 struct browse_state *bst);
bitpunch_status_t
expr_dpath_get_filtered_data_internal(
    expr_dpath_t dpath,
    struct bitpunch_data_source **dsp, int64_t *offsetp, int64_t *sizep,
    struct box **exported_data_boxp,
    struct browse_state *bst);
bitpunch_status_t
expr_dpath_evaluate_filter_internal(
    expr_dpath_t dpath,
    struct ast_node_hdl **filter_typep,
    struct browse_state *bst);
int
expr_dpath_contains_indexed_items(expr_dpath_t dpath);
const struct ast_node_hdl *
expr_dpath_get_as_type(expr_dpath_t dpath);
const struct ast_node_hdl *
expr_dpath_get_target_filter(expr_dpath_t dpath);
struct track_path
expr_dpath_get_track_path(expr_dpath_t dpath);
int
expr_dpath_is(expr_dpath_t dpath1, expr_dpath_t dpath2);
void
expr_dpath_find_common_ancestor(expr_dpath_t dpath1,
                                expr_dpath_t dpath2,
                                expr_dpath_t *ancestor1_dpathp,
                                expr_dpath_t *ancestor2_dpathp);

#endif /* __EXPR_INTERNAL_H__ */
