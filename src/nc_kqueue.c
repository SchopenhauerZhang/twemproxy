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

#include <sys/event.h>

#include <nc_event.h>
#include <nc_core.h>

struct evcenter *
event_init(int size)
{
    int status, ep;
    struct kevent *event;
    struct fired_event *fired_events;
    struct evcenter *center;

    center = nc_zalloc(sizeof(struct evcenter));
    if (center == NULL) {
        log_error("center create failed: %s", strerror(errno));
        return NULL;
    }

    ep = kqueue();
    if (ep < 0) {
        nc_free(center);
        log_error("kqueue create failed: %s", strerror(errno));
        return NULL;
    }

    event = nc_calloc(size, sizeof(struct kevent));
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

    return center;
}

void
event_deinit(struct evcenter *center)
{
    int status;

    ASSERT(center->ep >= 0);

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
    struct kevent event;

    ASSERT(center->ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        return 0;
    }

    status = event_add_raw(ep, c->sd, EVENT_WRITABLE, c);
    if (status < 0) {
        log_error("kqueue ctl on e %d sd %d failed: %s", ep, c->sd,
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
    struct kevent event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (!c->send_active) {
        return 0;
    }

    status = event_del_raw(ep, c->sd, EVENT_WRITABLE, c);
    if (status < 0) {
        log_error("kevent on e %d sd %d failed: %s", ep, c->sd,
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
    struct kevent event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    status = event_add_raw(ep, c->sd, EVENT_READABLE|EVENT_WRITABLE, c);
    if (status < 0) {
        log_error("kqueue on e %d sd %d failed: %s", ep, c->sd,
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
    struct kevent event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    status = event_del_raw(ep, c->sd, EVENT_READABLE|EVENT_WRITABLE, c);
    if (status < 0) {
        log_error("kqueue on e %d sd %d failed: %s", ep, c->sd,
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
    struct kevent event;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    if (mask & EVENT_READABLE) {
        EV_SET(&event, fd, EVFILT_READ|EV_CLEAR, EV_ADD, 0, 0, data);
        if (kevent(ep, &event, 1, NULL, 0, NULL) == -1) return -1;
    }
    if (mask & EVENT_WRITABLE) {
        EV_SET(&event, fd, EVFILT_WRITE|EV_CLEAR, EV_ADD, 0, 0, data);
        if (kevent(ep, &event, 1, NULL, 0, NULL) == -1) return -1;
    }

    return 0;
}

int
event_del_raw(int ep, int fd, int mask, void *data)
{
    struct kevent event;

    ASSERT(ep > 0);

    if (mask & EVENT_READABLE) {
        EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, data);
        if (kevent(ep, &event, 1, NULL, 0, NULL) == -1) return -1;
    }
    if (mask & EVENT_WRITABLE) {
        EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, data);
        if (kevent(ep, &event, 1, NULL, 0, NULL) == -1) return -1;
    }

    return 0;
}

int
event_wait(struct evcenter *center, int timeout)
{
    int nsd, numevents;
    struct kevent *events = center->event;

    ASSERT(ep > 0);
    ASSERT(event != NULL);
    ASSERT(nevent > 0);

    for (;;) {
        if (timeout > 0) {
            struct timespec tv;
            tv.tv_sec = timeout / 1000;
            tv.tv_nsec = (timeout - tv.tv_sec * 1000) * 1000;
            nsd = kevent(center->ep, NULL, 0, events, center->nevent, &tv);
        } else
            nsd = kevent(center->ep, NULL, 0, events, center->nevent, NULL);

        if (nsd > 0) {
            int j;

            numevents = nsd;
            for(j = 0; j < numevents; j++) {
                int mask = 0;
                struct kevent *e = events + j;

                if (e->filter == EVFILT_READ) mask |= EVENT_READABLE;
                if (e->filter == EVFILT_WRITE) mask |= EVENT_WRITABLE;
                center->fired_events[j].ptr = e->udata;
                center->fired_events[j].mask = mask;
                center->fired_events[j].fd = e->ident;
            }
            return nsd;
        }

        if (nsd == 0) {
            if (timeout == -1) {
               log_error("kqueue wait on e %d with %d events and %d timeout "
                         "returned no events", center->ep, center->nevent, timeout);
                return -1;
            }

            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        log_error("status: %d kqueue wait on e %d with %d events failed: %s", nsd, center->ep, center->nevent,
                  strerror(errno));

        return -1;
    }

    NOT_REACHED();
}
