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

#include <unistd.h>
#include <sys/epoll.h>

#include <nc_core.h>
#include <nc_event.h>

struct evcenter *
event_init(int size)
{
    int status, ep;
    struct epoll_event *event;
    struct fired_event *fired_events;
    struct evcenter *center;

    ASSERT(ctx->ep < 0);
    ASSERT(ctx->nevent != 0);
    ASSERT(ctx->event == NULL);

    center = nc_zalloc(sizeof(struct evcenter));
    if (center == NULL) {
        log_error("center create failed: %s", strerror(errno));
        return NULL;
    }

    ep = epoll_create(size);
    if (ep < 0) {
        log_error("epoll create of size %d failed: %s", size, strerror(errno));
        return -1;
    }

    event = nc_calloc(size, sizeof(struct epoll_event));
    fired_events = nc_calloc(size, sizeof(struct fired_event));
    if (event == NULL || fired_events == NULL) {
        status = close(ep);
        nc_free(center);
        if (event != NULL)
            nc_free(event);
        if (fired_events != NULL)
            nc_free(fired_events);
        if (status < 0) {
            log_error("close e %d failed, ignored: %s", ep, strerror(errno));
        }
        return NULL;
    }

    center->ep = ep;
    center->event = event;
    center->nevent = size;
    center->fired_events = fired_events;

    log_debug(LOG_INFO, "e %d with nevent %d timeout %d", center->ep,
              center->nevent, center->timeout);

    return 0;
}

void
event_deinit(struct evcenter *center)
{
    int status;

    ASSERT(ctx->ep >= 0);

    nc_free(center->event);

    status = close(center->ep);
    if (status < 0) {
        log_error("close e %d failed, ignored: %s", center->ep, strerror(errno));
    }
    center->ep = -1;
}

int
event_add_out(int ep, struct conn *c)
{
    int status;
    struct epoll_event event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        return 0;
    }

    event.events = (uint32_t)(EPOLLIN | EPOLLOUT | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_MOD, c->sd, &event);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 1;
    }

    return status;
}

int
event_del_out(int ep, struct conn *c)
{
    int status;
    struct epoll_event event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (!c->send_active) {
        return 0;
    }

    event.events = (uint32_t)(EPOLLIN | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_MOD, c->sd, &event);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 0;
    }

    return status;
}

int
event_add_conn(int ep, struct conn *c)
{
    int status;
    struct epoll_event event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    event.events = (uint32_t)(EPOLLIN | EPOLLOUT | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_ADD, c->sd, &event);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 1;
        c->recv_active = 1;
    }

    return status;
}

int
event_del_conn(int ep, struct conn *c)
{
    int status;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    status = epoll_ctl(ep, EPOLL_CTL_DEL, c->sd, NULL);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->recv_active = 0;
        c->send_active = 0;
    }

    return status;
}

int
event_add_raw(int ep, int fd, int mask, void *data)
{
    struct epoll_event event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    if (mask & EVENT_READABLE) event.events |= EPOLLIN;
    if (mask & EVENT_WRITABLE) event.events |= EPOLLOUT;
    event.data.u64 = 0; /* avoid valgrind warning */
    event.data.fd = fd;
    event.data.ptr = data;

    if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &event) == -1 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int
event_del_raw(int ep, int fd, int mask, void *data)
{
    struct epoll_event event;

    ASSERT(ep > 0);

    event.events = 0;
    event.data.u64 = 0; /* avoid valgrind warning */
    event.data.fd = fd;
    /* Note, Kernel < 2.6.9 requires a non null event pointer even for
     * EPOLL_CTL_DEL. */
    if (epoll_ctl(ep, EPOLL_CTL_DEL, fd, &event) == -1
            && errno != ENOENT && errno != EBADF) {
        return -1;
    }

    return 0;
}

int
event_wait(struct evcenter *center, int timeout)
{
    int nsd, numevents, j;
    struct epoll_event *events = center->event;

    ASSERT(ep > 0);
    ASSERT(event != NULL);
    ASSERT(nevent > 0);

    for (;;) {
        nsd = epoll_wait(center->ep, events, center->nevent, timeout);
        if (nsd > 0) {
            numevents = nsd;
            for (j = 0; j < numevents; j++) {
                int mask = 0;
                struct epoll_event *e = events+j;

                if (e->events & EPOLLIN) mask |= EVENT_READABLE;
                if (e->events & EPOLLOUT) mask |= EVENT_WRITABLE;
                if (e->events & EPOLLERR) mask |= EVENT_WRITABLE;
                if (e->events & EPOLLHUP) mask |= EVENT_WRITABLE;
                center->fired_events[j].ptr = e->data.ptr;
                center->fired_events[j].mask = mask;
                center->fired_events[j].fd = e->data.fd;
            }

        }

        if (nsd == 0) {
            if (timeout == -1) {
               log_error("epoll wait on e %d with %d events and %d timeout "
                         "returned no events", center->ep, center->nevent, timeout);
                return -1;
            }

            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        log_error("epoll wait on e %d with %d events failed: %s", center->ep, center->nevent,
                  strerror(errno));

        return -1;
    }

    NOT_REACHED();
}
