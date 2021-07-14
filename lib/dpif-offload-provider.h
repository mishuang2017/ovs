/*
 * Copyright (c) 2021, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DPIF_OFFLOAD_PROVIDER_H
#define DPIF_OFFLOAD_PROVIDER_H

#include "dp-packet.h"
#include "netlink-protocol.h"
#include "openvswitch/packets.h"
#include "openvswitch/types.h"

struct dpif;
struct registered_dpif_offload_class {
    const struct dpif_offload_class *offload_class;
    int refcount;
};

#ifdef __linux__
extern const struct dpif_offload_class dpif_offload_netlink_class;
#endif

/* When offloading sample action, userspace creates a unique ID to map
 * sFlow action and tunnel info and passes this ID to datapath instead
 * of the sFlow info. Datapath will send this ID and sampled packet to
 * userspace. Using the ID, userspace can recover the sFlow info and send
 * sampled packet to the right sFlow monitoring host.
 */
struct dpif_sflow_attr {
    const struct nlattr *action;    /* SFlow action only. */
    const struct nlattr *userdata;  /* Struct user_action_cookie. */
    const struct nlattr *actions;   /* All actions to get output tunnel. */
    size_t actions_len;             /* All actions len. */
    struct flow_tnl *tunnel;        /* Input tunnel. */
    ovs_u128 ufid;                  /* Flow ufid. */
};

/* Parse the specific dpif message to sFlow. So OVS can process it. */
struct dpif_offload_sflow {
    struct dp_packet packet;            /* Packet data. */
    uint64_t buf_stub[4096 / 8];        /* Buffer stub for packet data. */
    uint32_t iifindex;                  /* Input ifindex. */
    const struct dpif_sflow_attr *attr; /* SFlow attribute. */
};

/* Datapath interface offload structure, to be defined by each implementation
 * of a datapath interface.
 */
struct dpif_offload_class {
    /* Type of dpif offload in this class, e.g. "system", "netdev", etc.
     *
     * One of the providers should supply a "system" type, since this is
     * the type assumed if no type is specified when opening a dpif. */
    const char *type;

    /* Called when the dpif offload provider is registered. */
    int (*init)(void);

    /* Free all dpif offload resources. */
    void (*destroy)(void);

    /* Arranges for the poll loop for an upcall handler to wake up when psample
     * has a message queued to be received. */
    void (*sflow_recv_wait)(void);

    /* Polls for an upcall from psample for an upcall handler.
     * Return 0 for success. */
    int (*sflow_recv)(struct dpif_offload_sflow *sflow);
};

void dp_offload_initialize(void);
void dpif_offload_close(struct dpif *);

int dp_offload_register_provider(const struct dpif_offload_class *);
void dp_offload_class_unref(struct registered_dpif_offload_class *rc);
struct registered_dpif_offload_class *dp_offload_class_lookup(const char *);

void dpif_offload_sflow_recv_wait(const struct dpif *dpif);
int dpif_offload_sflow_recv(const struct dpif *dpif,
                            struct dpif_offload_sflow *sflow);

#endif /* DPIF_OFFLOAD_PROVIDER_H */
