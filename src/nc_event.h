/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NC_EVENT_H_
#define _NC_EVENT_H_

#include <nc_core.h>

/*
 * A hint to the kernel that is used to size the event backing store
 * of a given epoll instance
 */
#define EVENT_SIZE_HINT 1024

#define EVENT_NONE 0
#define EVENT_READABLE 1
#define EVENT_WRITABLE 2

struct fired_event {
    int mask;
    void *ptr;
    int fd;
};

struct evcenter {
    int                ep;            /* epoll device */
    int                nevent;        /* # epoll event */
    struct fired_event *fired_events; /* epoll event */
    void               *event;
};

struct evcenter * event_init(int size);
void event_deinit(struct evcenter *center);

int event_add_out(int ep, struct conn *c);
int event_del_out(int ep, struct conn *c);
int event_add_conn(int ep, struct conn *c);
int event_del_conn(int ep, struct conn *c);
int event_add_raw(int ep, int fd, int mask, void *data);
int event_del_raw(int ep, int fd, int mask, void *data);

int event_wait(struct evcenter *center, int timeout);

#endif
