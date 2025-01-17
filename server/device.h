#ifndef __WINE_SERVER_DEVICE_H
#define __WINE_SERVER_DEVICE_H

#include "wine/server_protocol.h"
#include "thread.h"
#include "object.h"

struct device_manager;

void queue_callback(krnl_cbdata_t *cb, struct unicode_str *string_param, struct object **done_event);
struct thread *device_manager_client_thread(struct device_manager *dev_mgr, struct thread *thread);

#endif
