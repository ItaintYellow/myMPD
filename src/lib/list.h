/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIST_H
#define MYMPD_LIST_H

#include <stdbool.h>

#include "../../dist/src/sds/sds.h"

struct t_list_node {
    sds key;
    sds value_p;
    long value_i;
    void *user_data;
    struct t_list_node *next;
};

struct t_list {
    unsigned length;
    struct t_list_node *head;
    struct t_list_node *tail;
};

struct t_list *list_new(void);
void list_init(struct t_list *l);
void list_clear(struct t_list *l);
void list_clear_keep_user_data(struct t_list *l);
bool list_push(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data);
bool list_push_len(struct t_list *l, const char *key, int key_len, long value_i, const char *value_p, int value_len, void *user_data);
bool list_insert(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data);
bool list_insert_sorted_by_key(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data, bool order);
bool list_insert_sorted_by_value_i(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data, bool order);
bool list_shift(struct t_list *l, unsigned idx);
bool list_replace(struct t_list *l, unsigned pos, const char *key, long value_i, const char *value_p, void *user_data);
long list_get_value_i(const struct t_list *l, const char *key);
sds list_get_value_p(const struct t_list *l, const char *key);
void *list_get_user_data(const struct t_list *l, const char *key);
struct t_list_node *list_get_node(const struct t_list *l, const char *key);
bool list_shuffle(struct t_list *l);
bool list_sort_by_value_i(struct t_list *l, bool order);
bool list_sort_by_value_p(struct t_list *l, bool order);
bool list_sort_by_key(struct t_list *l, bool order);
bool list_swap_item(struct t_list_node *n1, struct t_list_node *n2);
bool list_swap_item_pos(struct t_list *l, unsigned index1, unsigned index2);
bool list_move_item_pos(struct t_list *l, unsigned from, unsigned to);
struct t_list_node *list_node_at(const struct t_list * l, unsigned index);
struct t_list_node *list_shift_first(struct t_list *l);
bool list_node_free(struct t_list_node *n);
bool list_node_free_keep_user_data(struct t_list_node *n);
#endif
