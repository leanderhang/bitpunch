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

#ifndef __FILTER_ARRAY_INDEX_CACHE_H__
#define __FILTER_ARRAY_INDEX_CACHE_H__

#include "utils/bloom.h"
#include "core/expr.h"
#include "core/browse.h"

struct index_cache_mark_offset {
    int64_t item_offset;
};

struct array_cache {
    struct bloom_book *cache_by_key;
    ARRAY_HEAD(index_cache_mark_offset_repo,
               struct index_cache_mark_offset) mark_offsets;
    int mark_offsets_exists;
    int64_t last_cached_index;
    struct ast_node_hdl *last_cached_item;
    int64_t last_cached_item_offset;
#define BOX_INDEX_CACHE_DEFAULT_LOG2_N_KEYS_PER_MARK 5
    int cache_log2_n_keys_per_mark;
};

struct index_cache_iterator {
    struct bloom_book_cookie bloom_cookie;
    struct tracker *xtk;
    expr_value_t key;
    bloom_book_mark_t mark;
    struct track_path in_slice_path;
    bloom_book_mark_t from_mark;
    int first;
};

int64_t
array_get_index_mark(struct array_cache *cache, int64_t index);
int
index_cache_exists(struct array_cache *cache);

bitpunch_status_t
array_index_cache_init(struct array_cache *cache, struct box *scope,
                       struct ast_node_hdl *filter, struct browse_state *bst);
void
array_index_cache_destroy(struct array_cache *cache);

bitpunch_status_t
tracker_index_cache_add_item(struct tracker *tk, expr_value_t item_key,
                             struct browse_state *bst);
bitpunch_status_t
box_index_cache_lookup_key_twins(struct box *box,
                                 expr_value_t item_key,
                                 struct track_path in_slice_path,
                                 struct index_cache_iterator *iterp,
                                 struct browse_state *bst);
bitpunch_status_t
index_cache_iterator_next_twin(struct index_cache_iterator *iter,
                               struct track_path *item_pathp,
                               struct browse_state *bst);
void
index_cache_iterator_done(struct index_cache_iterator *iter);

bitpunch_status_t
tracker_index_cache_goto_twin(struct tracker *tk,
                              expr_value_t item_key,
                              int nth_twin,
                              struct track_path in_slice_path,
                              int *last_twinp,
                              struct browse_state *bst);
bitpunch_status_t
tracker_index_cache_lookup_current_twin_index(
    struct tracker *tk,
    expr_value_t item_key,
    struct track_path in_slice_path,
    int *nth_twinp,
    struct browse_state *bst);
bitpunch_status_t
tracker_goto_mark_internal(struct tracker *tk,
                           int64_t mark,
                           struct browse_state *bst);
void
tracker_goto_last_cached_item_internal(struct tracker *tk,
                                       struct browse_state *bst);

#endif
