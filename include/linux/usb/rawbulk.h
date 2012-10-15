/*
 * Rawbulk Driver from VIA Telecom
 *
 * Copyright (C) 2011 VIA Telecom, Inc.
 * Author: Karfield Chen (kfchen@via-telecom.com)
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RAWBULK_H__
#define __RAWBULK_H__

#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

enum transfer_id {
    RAWBULK_TID_MODEM,
    RAWBULK_TID_ETS,
    RAWBULK_TID_AT,
    RAWBULK_TID_PCV,
    RAWBULK_TID_GPS,
    _MAX_TID
};

/* platform data struct for rawbulk */

struct usb_rawbulk_platform_data {
    struct rawbulk_fn_data {
        int upstream_transactions;
        int downstream_transactions;
        int upstream_buffersize;
        int downstream_buffersize;
        int downstream_splitcount;
    } fn_params[_MAX_TID];
};

typedef int (*rawbulk_intercept_t)(struct usb_interface *interface, int enable);
typedef void (*rawbulk_autostart_callback_t)(int transfer_id);

#ifdef CONFIG_USB_ANDROID_RAWBULK
/* bind/unbind for gadget */
int rawbulk_bind_function(int transfer_id, struct usb_function *function,
        struct usb_ep *bulk_out, struct usb_ep *bulk_in,
        rawbulk_autostart_callback_t autostart_callback);
void rawbulk_unbind_function(int trasfer_id);

/* bind/unbind host interfaces */
int rawbulk_bind_host_interface(struct usb_interface *interface,
        rawbulk_intercept_t inceptor);
void rawbulk_unbind_host_interface(struct usb_interface *interface);

/* operations for transactions */
int rawbulk_start_transactions(int transfer_id, int nups, int ndowns,
        int upsz, int downsz, int splitsz);
void rawbulk_stop_transactions(int transfer_id);
int rawbulk_halt_transactions(int transfer_id);
int rawbulk_resume_transactions(int transfer_id);

int rawbulk_suspend_host_interface(int transfer_id, pm_message_t message);
int rawbulk_resume_host_interface(int transfer_id);

int rawbulk_forward_ctrlrequest(const struct usb_ctrlrequest *ctrl);

/* statics and state */
int rawbulk_transfer_statistics(int transfer_id, char *buf);
int rawbulk_transfer_state(int transfer_id);

#else

static inline int return_zero(int transfer_id, ...)  {return 0;}
static inline int return_inval(int transfer_id, ...) {return -EINVAL;}
static inline void return_void(int transfer_id, ...) {;}

#define rawbulk_bind_function       return_inval
#define rawbulk_unbind_function     return_void
#define rawbulk_bind_host_interface return_inval
#define rawbulk_unbind_host_interface return_void
#define rawbulk_suspend_host_interface return_inval
#define rawbulk_resume_host_interface return_inval
#define rawbulk_start_transactions  return_inval
#define rawbulk_stop_transactions   return_void
#define rawbulk_halt_transactions   return_inval
#define rawbulk_resume_transactions return_inval
#define rawbulk_forward_ctrlrequest return_inval
#define rawbulk_transfer_statistics return_zero
#define rawbulk_transfer_state      return_inval

#endif /* CONFIG_USB_ANDROID_VIARAWBULK */

#endif /* __RAWBULK_HEADER_FILE__ */
