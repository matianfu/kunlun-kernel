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
/*
 * Rawbulk is transfer performer between CBP host driver and Gadget driver
 *
 * 1 rawbulk transfer can be consist by serval upstream and/or downstream
 * transactions.
 *
 * upstream:    CBP Driver ---> Gadget IN
 * downstream:  Gadget OUT ---> CBP Driver
 *
 * upstream flowchart:
 *
 * -x-> usb_submit_urb -> complete -> usb_ep_queue -> complete -,
 *  |                                                           |
 *  `----------------------(control)----------------------------'
 *
 * downstream flowchart:
 *
 * -x-> usb_ep_queue -> complete -> usb_submit_urb -> complete -,
 *  |                                                           |
 *  `----------------------(control)----------------------------'
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#define DRIVER_AUTHOR   "Karfield Chen <kfchen@via-telecom.com>"
#define DRIVER_DESC     "Rawbulk Driver - perform bypass for Kunlun"
#define DRIVER_VERSION  "1.0.3"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/usb/rawbulk.h>

#ifdef VERBOSE_DEBUG
#define ldbg(fmt, args...) \
    printk(KERN_INFO "%s: " fmt "\n", __func__, ##args)
#define tdbg(t, fmt, args...) \
    printk(KERN_INFO "Rawbulk %s: " fmt "\n", t->name, ##args)
#else
#define ldbg(args...)
#define tdbg(args...)
#endif

#define lerr(fmt, args...) \
    printk(KERN_ERR "%s: " fmt "\n", __func__, ##args)
#define terr(t, fmt, args...) \
    printk(KERN_ERR "Rawbulk [%s]:" fmt "\n", t->name,  ##args)

#define FREE_STALLED        1
#define FREE_IDLED          2
#define FREE_RETRIVING      4
#define FREE_ALL (FREE_STALLED | FREE_IDLED | FREE_RETRIVING)

#define STOP_UPSTREAM   0x1
#define STOP_DOWNSTREAM 0x2

struct rawbulk_transfer {
    enum transfer_id id;
    spinlock_t lock;
    int control;
    struct usb_anchor submitted;
    struct usb_function *function;
    struct usb_interface *interface;
    struct mutex mutex; /* protection for API calling */
    rawbulk_autostart_callback_t autostart;
    rawbulk_intercept_t inceptor;
    spinlock_t suspend_lock;
    int suspended;
    struct {
        int ntrans;
        struct list_head transactions;
        struct usb_ep *ep;
        struct usb_host_endpoint *host_ep;
    } upstream, downstream;
};

static inline int get_epnum(struct usb_host_endpoint *ep) {
    return (int)(ep->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
}

static inline int get_maxpacksize(struct usb_host_endpoint *ep) {
    return (int)(le16_to_cpu(ep->desc.wMaxPacketSize));
}

#define MAX_RESPONSE    32

static struct rawbulk_core {
    struct usb_device *udev;
    struct usb_composite_dev *cdev;
    char ctrl_response[MAX_RESPONSE];
    struct usb_ctrlrequest forward_dr;
    struct urb *forwarding_urb;
    struct rawbulk_transfer transfer[_MAX_TID];
} *rawbulk;

static struct rawbulk_transfer *id_to_transfer(int transfer_id) {
    if (transfer_id < 0 || transfer_id >= _MAX_TID)
        return NULL;
    return &rawbulk->transfer[transfer_id];
}

/*
 * upstream
 */

#define UPSTREAM_STAT_FREE          0
#define UPSTREAM_STAT_RETRIEVING    1
#define UPSTREAM_STAT_UPLOADING     2

struct upstream_transaction {
    int state;
    int stalled;
    char name[32];
    struct list_head tlist;
    struct delayed_work delayed;
    struct rawbulk_transfer *transfer;
    struct usb_request *req;
    struct urb *urb;
    int buffer_length;
    unsigned char buffer[0];
};

static void upstream_delayed_work(struct work_struct *work);
static struct upstream_transaction *alloc_upstream_transaction(
        struct rawbulk_transfer *transfer, int bufsz) {

    struct upstream_transaction *t;

    if (bufsz < get_maxpacksize(transfer->upstream.host_ep))
        return NULL;

    t = kzalloc(sizeof *t + bufsz * sizeof(unsigned char), GFP_KERNEL);
    if (!t)
        return NULL;

    t->buffer_length = bufsz;

    t->req = usb_ep_alloc_request(transfer->upstream.ep, GFP_KERNEL);
    if (!t->req)
        goto failto_alloc_usb_request;

    t->urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!t->urb)
        goto failto_alloc_urb;

    t->name[0] = 0;
    sprintf(t->name, "U%d (H:ep%din, G:%s)", transfer->upstream.ntrans,
            get_epnum(transfer->upstream.host_ep),
            transfer->upstream.ep->name);

    INIT_LIST_HEAD(&t->tlist);
    list_add_tail(&t->tlist, &transfer->upstream.transactions);
    transfer->upstream.ntrans ++;
    t->transfer = transfer;

    INIT_DELAYED_WORK(&t->delayed, upstream_delayed_work);

    t->state = UPSTREAM_STAT_FREE;
    return t;

failto_alloc_urb:
    usb_ep_free_request(transfer->upstream.ep, t->req);
failto_alloc_usb_request:
    kfree(t);
    return NULL;
}

static void free_upstream_transaction(struct rawbulk_transfer *transfer, int option) {
    struct list_head *p, *n;
    list_for_each_safe(p, n, &transfer->upstream.transactions) {
        struct upstream_transaction *t = list_entry(p, struct
                upstream_transaction, tlist);

        if ((option & FREE_RETRIVING) &&
                (t->state == UPSTREAM_STAT_RETRIEVING)) {
            usb_unlink_urb(t->urb);
            goto free;
        }

        if ((option & FREE_STALLED) && t->stalled)
            goto free;

        if ((option & FREE_IDLED) && (t->state == UPSTREAM_STAT_FREE))
            goto free;

        continue;
free:
        list_del(p);
        usb_free_urb(t->urb);
        usb_ep_free_request(transfer->upstream.ep, t->req);
        kfree(t);

        transfer->upstream.ntrans --;
    }
}

static void upstream_second_stage(struct urb *urb);
static void upstream_final_stage(struct usb_ep *ep, struct usb_request
        *req);

static int start_upstream(struct upstream_transaction *t) {
    int rc;
    unsigned long flags;
    struct rawbulk_transfer *transfer = t->transfer;

    spin_lock_irqsave(&transfer->lock, flags);
    if (transfer->control & STOP_UPSTREAM) {
        spin_unlock_irqrestore(&transfer->lock, flags);
        cancel_delayed_work(&t->delayed);
        t->state = UPSTREAM_STAT_FREE;
        t->stalled = 1;
        return -EPIPE;
    }
    spin_unlock_irqrestore(&transfer->lock, flags);

    t->stalled = 0;
    t->state = UPSTREAM_STAT_FREE;

    usb_fill_bulk_urb(t->urb, rawbulk->udev,
            usb_rcvbulkpipe(rawbulk->udev,
                get_epnum(transfer->upstream.host_ep)),
            t->buffer, t->buffer_length,
            upstream_second_stage, t);
    usb_anchor_urb(t->urb, &transfer->submitted);

    usb_mark_last_busy(rawbulk->udev);
    rc = usb_submit_urb(t->urb, GFP_ATOMIC);
    if (rc < 0) {
        terr(t, "fail to submit urb %d", rc);
        /* FIXME: if we met the host is suspending yet, we may add re-submit
         * this urb in a ashman thread. so what is a ashman? this can deal any
         * error that occur in this driver, and maintain the driver's health, I
         * will complete this code in future, here we just stall the transaction
         */
        t->stalled = 1;
        usb_unanchor_urb(t->urb);
    }

    t->state = UPSTREAM_STAT_RETRIEVING;
    return rc;
}

static void upstream_delayed_work(struct work_struct *work) {
    int rc;
    int stop;
    unsigned long flags;
    struct delayed_work *dw = container_of(work, struct delayed_work, work);
    struct upstream_transaction *t = container_of (work, struct
            upstream_transaction, delayed.work);
    struct rawbulk_transfer *transfer = t->transfer;

    spin_lock_irqsave(&transfer->lock, flags);
    stop = !!(transfer->control & STOP_UPSTREAM);
    spin_unlock_irqrestore(&transfer->lock, flags);

    rc = start_upstream(t);
    if (rc < 0 && !stop)
        schedule_delayed_work(dw, HZ);
}

static void upstream_second_stage(struct urb *urb) {
    int rc;
    struct upstream_transaction *t = urb->context;
    struct rawbulk_transfer *transfer = t->transfer;

    if (urb->status < 0) {
        /* start again atomatically */
        rc = start_upstream(t);
#if 0
        if (rc < 0)
            schedule_delayed_work(&t->delayed, HZ);
#endif
        return;
    }

    /* allow zero-length package to pass
       if (!urb->actual_length) {
       terr(t, "urb actual_length 0");
       start_upstream(t);
       return;
       }
     */

    t->req->status = 0;
    t->req->buf = t->buffer;
    t->req->length = urb->actual_length;
    t->req->complete = upstream_final_stage;

    rc = usb_ep_queue(transfer->upstream.ep, t->req, GFP_ATOMIC);
    if (rc < 0) {
        terr(t, "fail to queue request, %d", rc);
        t->stalled = 1;
    }
    t->state = UPSTREAM_STAT_UPLOADING;
}

static void upstream_final_stage(struct usb_ep *ep,
        struct usb_request *req) {
    struct upstream_transaction *t = container_of(req->buf,
            struct upstream_transaction, buffer);

    if (req->status < 0) {
        if (req->status != -ECONNRESET)
            terr(t, "req status %d", req->status);
        if (req->status == -ESHUTDOWN) {
            t->stalled = 1;
            return;
        }
    }

    if (!req->actual)
        terr(t, "req actual 0");

    start_upstream(t);
}

static void stop_upstream(struct upstream_transaction *t) {
    struct rawbulk_transfer *transfer = t->transfer;
    if (t->state == UPSTREAM_STAT_RETRIEVING) {
        cancel_delayed_work(&t->delayed);
        usb_unlink_urb(t->urb);
    } else if (t->state == UPSTREAM_STAT_UPLOADING)
        usb_ep_dequeue(transfer->upstream.ep, t->req);
    t->stalled = 1;
}

/*
 * downstream
 */

#define MAX_SPLIT_LIMITED   64

#define DOWNSTREAM_STAT_FREE        0
#define DOWNSTREAM_STAT_RETRIEVING  1
#define DOWNSTREAM_STAT_DOWNLOADING 2

struct downstream_transaction {
    int state;
    int stalled;
    char name[32];
    struct list_head tlist;
    struct rawbulk_transfer *transfer;
    struct usb_request *req;
    int nurb;
    struct urb *urb[MAX_SPLIT_LIMITED];
    int urb_length;
    int buffer_length;
    unsigned char buffer[0];
};

static void downstream_second_stage(struct usb_ep *ep,
        struct usb_request *req);
static void downstream_final_stage(struct urb *urb);

static struct downstream_transaction *alloc_downstream_transaction(
        struct rawbulk_transfer *transfer, int bufsz, int urbsz) {
    int n = 0;
    struct downstream_transaction *t;
    int maxurbs = bufsz / urbsz;

    if (urbsz < get_maxpacksize(transfer->downstream.host_ep))
        return NULL;

    if (maxurbs > MAX_SPLIT_LIMITED)
        return NULL;

    t = kzalloc(sizeof *t + bufsz * sizeof(unsigned char), GFP_KERNEL);
    if (!t)
        return NULL;

    t->buffer_length = bufsz;
    t->urb_length = urbsz;

    t->req = usb_ep_alloc_request(transfer->downstream.ep, GFP_KERNEL);
    if (!t->req)
        goto failto_alloc_usb_request;

    for (n = 0; n < maxurbs; n ++) {
        t->urb[n] = usb_alloc_urb(0, GFP_KERNEL);
        if (!t->urb[n])
            goto failto_alloc_urb;
    }
    t->nurb = 0;

    t->name[0] = 0;
    sprintf(t->name, "D%d (G:%s, H:ep%dout)", transfer->downstream.ntrans,
            transfer->downstream.ep->name,
            get_epnum(transfer->downstream.host_ep));

    INIT_LIST_HEAD(&t->tlist);
    list_add_tail(&t->tlist, &transfer->downstream.transactions);
    transfer->downstream.ntrans ++;
    t->transfer = transfer;

    t->state = DOWNSTREAM_STAT_FREE;
    return t;

failto_alloc_urb:
    while (n --)
        usb_free_urb(t->urb[n]);
    usb_ep_free_request(transfer->downstream.ep, t->req);
failto_alloc_usb_request:
    kfree(t);
    return NULL;
}

static void free_downstream_transaction(struct rawbulk_transfer *transfer, int option) {
    int i;
    int maxurbs;
    struct list_head *p, *n;
    list_for_each_safe(p, n, &transfer->downstream.transactions) {
        struct downstream_transaction *t = list_entry(p, struct
                downstream_transaction, tlist);

        if ((option & FREE_STALLED) && t->stalled)
            goto free;

        if ((option & FREE_IDLED) && (t->state == DOWNSTREAM_STAT_FREE))
            goto free;

        if ((option & FREE_RETRIVING) &&
                (t->state == DOWNSTREAM_STAT_RETRIEVING)) {
            usb_ep_dequeue(transfer->downstream.ep, t->req);
            goto free;
        }

        continue;
free:
        list_del(p);
        maxurbs = t->buffer_length / t->urb_length;
        for (i = 0; i < maxurbs; i ++)
            usb_free_urb(t->urb[i]);
        usb_ep_free_request(transfer->downstream.ep, t->req);
        kfree(t);

        transfer->downstream.ntrans --;
    }
}

static int start_downstream(struct downstream_transaction *t) {
    int rc;
    unsigned long flags;
    struct rawbulk_transfer *transfer = t->transfer;
    struct usb_request *req = t->req;

    spin_lock_irqsave(&transfer->lock, flags);
    if (transfer->control & STOP_DOWNSTREAM) {
        spin_unlock_irqrestore(&transfer->lock, flags);
        t->state = DOWNSTREAM_STAT_FREE;
        t->stalled = 1;
        return -EPIPE;
    }
    spin_unlock_irqrestore(&transfer->lock, flags);

    t->stalled = 0;
    t->state = DOWNSTREAM_STAT_FREE;

    req->buf = t->buffer;
    req->length = t->buffer_length;
    req->complete = downstream_second_stage;

    rc = usb_ep_queue(transfer->downstream.ep, req, GFP_ATOMIC);
    if (rc < 0) {
        terr(t, "fail to queue request, %d", rc);
        t->stalled = 1;
        return rc;
    }

    t->state = DOWNSTREAM_STAT_RETRIEVING;
    return 0;
}

static void downstream_second_stage(struct usb_ep *ep,
        struct usb_request *req) {
    int n = 0;
    void *next_buf = req->buf;
    int data_length = req->actual;
    struct downstream_transaction *t = container_of(req->buf,
            struct downstream_transaction, buffer);
    struct rawbulk_transfer *transfer = t->transfer;

    if (req->status) {
        if (req->status != -ECONNRESET)
            terr(t, "req status %d", req->status);
        if (req->status == -ESHUTDOWN)
            t->stalled = 1;
        else
            start_downstream(t);
        return;
    }

    /*
    if (!data_length) {
        terr(t, "req actual 0");
        start_downstream(t);
        return;
    }
    */

    /* split recieved data into servral urb size less than maxpacket of cbp
     * endpoint */
    do {
        int rc;
        struct urb *urb = t->urb[n];
        int tsize = (data_length > t->urb_length)? t->urb_length:
            data_length;

        urb->status = 0;
        usb_fill_bulk_urb(urb, rawbulk->udev,
                usb_sndbulkpipe(rawbulk->udev,
                    get_epnum(transfer->downstream.host_ep)),
                next_buf, tsize, downstream_final_stage, t);
        usb_anchor_urb(urb, &transfer->submitted);

        usb_mark_last_busy(rawbulk->udev);
        rc = usb_submit_urb(urb, GFP_ATOMIC);
        if (rc < 0) {
            terr(t, "fail to submit urb %d, n %d", rc, n);
            usb_unanchor_urb(urb);
            break;
        }

        data_length -= tsize;
        next_buf += tsize;
        n ++;
    } while (data_length > 0);

    t->nurb = n;
    t->state = DOWNSTREAM_STAT_DOWNLOADING;
}

static void downstream_final_stage(struct urb *urb) {
    int n;
    int actual_total = 0;
    struct downstream_transaction *t = urb->context;

    /* check the condition, re-queue usb_request on gadget ep again if possible */
    for (n = 0; n < t->nurb; n ++) {
        int status = t->urb[n]->status;

        if (status == -EINPROGRESS)
            return;

        if (!status)
            actual_total += t->urb[n]->actual_length;
    }

    if (actual_total < t->req->actual) {
        terr(t, "requested %d actual %d", t->req->actual, actual_total);
        t->stalled = 1;
        return;
    }

    /* completely transfered over, we can start next transfer now */
    start_downstream(t);
}

static void stop_downstream(struct downstream_transaction *t) {
    int n;
    struct rawbulk_transfer *transfer = t->transfer;
    int maxurbs = t->buffer_length / t->urb_length;

    if (t->state == DOWNSTREAM_STAT_RETRIEVING) {
        usb_ep_dequeue(transfer->downstream.ep, t->req);
    } else if (t->state == DOWNSTREAM_STAT_DOWNLOADING) {
        for (n = 0; n < maxurbs; n ++)
            usb_unlink_urb(t->urb[n]);
    }

    t->stalled = 1;
}

/*
 * Ctrlrequest forwarding
 */

static void response_complete(struct usb_ep *ep, struct usb_request *req) {
    if (req->status < 0)
        ldbg("feedback error %d\n", req->status);
}

static void forward_ctrl_complete(struct urb *urb) {
    struct usb_composite_dev *cdev = urb->context;
    struct usb_request *req = cdev->req;

    if (!cdev)
        return;

    if (urb->status >= 0) {
        if (urb->pipe & USB_DIR_IN) {
            memcpy(req->buf, urb->transfer_buffer, urb->actual_length);
            req->zero = 0;
            req->length = urb->actual_length;
            req->complete = response_complete;
            ldbg("cp echo: len %d buf %02x\n", req->length, *((char *)req->buf));
            usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
        } else /* Finish the status stage */
            usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
    }
}

int rawbulk_forward_ctrlrequest(const struct usb_ctrlrequest *ctrl) {
    unsigned int pipe;
    struct usb_device *dev;
    struct urb *urb;

    ldbg("%s req_type %02x req %02x value %04x index %04x len %d\n", __func__,
            ctrl->bRequestType, ctrl->bRequest, ctrl->wValue, ctrl->wIndex,
            ctrl->wLength);

    dev = rawbulk->udev;
    if (!dev) {
        return -ENODEV;
    }

    rawbulk->forward_dr.bRequestType = ctrl->bRequestType;
    rawbulk->forward_dr.bRequest = ctrl->bRequest;
    rawbulk->forward_dr.wValue = ctrl->wValue;
    rawbulk->forward_dr.wIndex = ctrl->wIndex;
    rawbulk->forward_dr.wLength = ctrl->wLength;

    if (!rawbulk->forwarding_urb) {
        urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!urb)
            return -ENOMEM;
        rawbulk->forwarding_urb = urb;
    } else
        urb = rawbulk->forwarding_urb;

    if (ctrl->bRequestType & USB_DIR_IN)
        pipe = usb_rcvctrlpipe(dev, 0);
    else
        pipe = usb_sndctrlpipe(dev, 0);

    usb_fill_control_urb(urb, dev, pipe, (unsigned char *)&rawbulk->forward_dr,
            rawbulk->ctrl_response, MAX_RESPONSE, forward_ctrl_complete,
            rawbulk->cdev);

    return usb_submit_urb(urb, GFP_ATOMIC);
}

EXPORT_SYMBOL_GPL(rawbulk_forward_ctrlrequest);

int rawbulk_start_transactions(int transfer_id, int nups, int ndowns, int upsz,
        int downsz, int splitsz) {
    int n;
    int rc;
    unsigned long flags = 0;
    int suspended;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    if (!rawbulk->cdev || !rawbulk->udev)
        return -ENODEV;

    if (!transfer->function || !transfer->interface)
        return -ENODEV;

    mutex_lock(&transfer->mutex);

    /* stop host transfer 1stly */
    if (transfer->inceptor)
        transfer->inceptor(transfer->interface, 1);

    for (n = 0; n < nups; n ++) {
        upstream = alloc_upstream_transaction(transfer, upsz);
        if (!upstream) {
            rc = -ENOMEM;
            ldbg("fail to allocate upstream transaction n %d", n);
            goto failto_alloc_upstream;
        }
    }

    for (n = 0; n < ndowns; n ++) {
        downstream = alloc_downstream_transaction(transfer, downsz, splitsz);
        if (!downstream) {
            rc = -ENOMEM;
            ldbg("fail to allocate downstream transaction n %d", n);
            goto failto_alloc_downstream;
        }
    }

    spin_lock_irqsave(&transfer->suspend_lock, flags);
    suspended = transfer->suspended;
    spin_unlock_irqrestore(&transfer->suspend_lock, flags);

    if (suspended) {
        ldbg("interface %d is sleeping", transfer_id);
        mutex_unlock(&transfer->mutex);
        return 0;
    }

    transfer->control &= ~STOP_UPSTREAM;
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        if (upstream->state == UPSTREAM_STAT_FREE && !upstream->stalled) {
            rc = start_upstream(upstream);
            if (rc < 0) {
                ldbg("fail to start upstream %s rc %d\n", upstream->name, rc);
                goto failto_start_upstream;
            }
        }
    }

    transfer->control &= ~STOP_DOWNSTREAM;
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist) {
        if (downstream->state == DOWNSTREAM_STAT_FREE && !downstream->stalled) {
            rc = start_downstream(downstream);
            if (rc < 0) {
                ldbg("fail to start downstream %s rc %d\n", downstream->name, rc);
                goto failto_start_downstream;
            }
        }
    }

    mutex_unlock(&transfer->mutex);
    return 0;

failto_start_downstream:
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist)
        stop_downstream(downstream);
failto_start_upstream:
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist)
        stop_upstream(upstream);
failto_alloc_downstream:
    free_downstream_transaction(transfer, FREE_ALL);
failto_alloc_upstream:
    free_upstream_transaction(transfer, FREE_ALL);
    /* recover host transfer */
    if (transfer->inceptor)
        transfer->inceptor(transfer->interface, 0);
    mutex_unlock(&transfer->mutex);
    return rc;
}

EXPORT_SYMBOL_GPL(rawbulk_start_transactions);

void rawbulk_stop_transactions(int transfer_id) {
    unsigned long flags;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return;

    mutex_lock(&transfer->mutex);
    spin_lock_irqsave(&transfer->lock, flags);
    transfer->control |= (STOP_UPSTREAM | STOP_DOWNSTREAM);
    spin_unlock_irqrestore(&transfer->lock, flags);

    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist)
        stop_upstream(upstream);
    free_upstream_transaction(transfer, FREE_ALL);
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist)
        stop_downstream(downstream);
    free_downstream_transaction(transfer, FREE_ALL);
    if (transfer->inceptor)
        transfer->inceptor(transfer->interface, 0);
    mutex_unlock(&transfer->mutex);
}

EXPORT_SYMBOL_GPL(rawbulk_stop_transactions);

int rawbulk_halt_transactions(int transfer_id) {
    unsigned long flags;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    spin_lock_irqsave(&transfer->lock, flags);
    transfer->control |= (STOP_UPSTREAM | STOP_DOWNSTREAM);
    spin_unlock_irqrestore(&transfer->lock, flags);

    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist)
        stop_upstream(upstream);
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist)
        stop_downstream(downstream);
    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_halt_transactions);

int rawbulk_resume_transactions(int transfer_id) {
    int rc;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    if (!rawbulk->cdev || !rawbulk->udev)
        return -ENODEV;

    if (!transfer->function || !transfer->interface)
        return -ENODEV;

    transfer->control &= ~STOP_UPSTREAM;
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        if (upstream->state == UPSTREAM_STAT_FREE && !upstream->stalled) {
            rc = start_upstream(upstream);
            if (rc < 0) {
                ldbg("fail to start upstream %s rc %d", upstream->name, rc);
                goto failto_start_upstream;
            }
        }
    }

    transfer->control &= ~STOP_DOWNSTREAM;
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist) {
        if (downstream->state == DOWNSTREAM_STAT_FREE && !downstream->stalled) {
            rc = start_downstream(downstream);
            if (rc < 0) {
                ldbg("fail to start downstream %s rc %d", downstream->name, rc);
                goto failto_start_downstream;
            }
        }
    }
    return 0;

failto_start_downstream:
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist)
        stop_downstream(downstream);
failto_start_upstream:
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist)
        stop_upstream(upstream);
    return rc;
}

EXPORT_SYMBOL_GPL(rawbulk_resume_transactions);

int rawbulk_transfer_state(int transfer_id) {
    int stalled = 1;
    int count = 0;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -EINVAL;

    if (!rawbulk->udev || !rawbulk->cdev)
        return -ENODEV;

    if (!transfer->interface || !transfer->function)
        return -EIO;

    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        if (!upstream->stalled)
            stalled = 0;
        count ++;
    }

    if (stalled || count == 0)
        return -EACCES;

    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist) {
        if (!downstream->stalled)
            stalled = 0;
        count ++;
    }
    if (stalled || count == 0)
        return -EACCES;

    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_transfer_state);

static char *state2string(int state, int upstream) {
    if (upstream) {
        switch (state) {
            case UPSTREAM_STAT_FREE:
                return "FREE";
            case UPSTREAM_STAT_RETRIEVING:
                return "RETRIEVING";
            case UPSTREAM_STAT_UPLOADING:
                return "UPLOADING";
            default:
                return "UNKNOW";
        }
    } else {
        switch (state) {
            case DOWNSTREAM_STAT_FREE:
                return "FREE";
            case DOWNSTREAM_STAT_RETRIEVING:
                return "RETRIEVING";
            case DOWNSTREAM_STAT_DOWNLOADING:
                return "DOWNLOADING";
            default:
                return "UNKNOW";
        }
    }
}

int rawbulk_transfer_statistics(int transfer_id, char *buf) {
    char *pbuf = buf;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return sprintf(pbuf, "-ENODEV, id %d\n", transfer_id);

    pbuf += sprintf(pbuf, "rawbulk statistics:\n");
    if (rawbulk->udev)
        pbuf += sprintf(pbuf, " host device: %d-%d\n", rawbulk->udev->bus->busnum,
            rawbulk->udev->devnum);
    else
        pbuf += sprintf(pbuf, " host device: -ENODEV\n");
    if (rawbulk->cdev)
        pbuf += sprintf(pbuf, " gadget device: %s\n",
                rawbulk->cdev->config->label);
    else
        pbuf += sprintf(pbuf, " gadget device: -ENODEV\n");
    pbuf += sprintf(pbuf, " upstreams (total %d transactions)\n",
            transfer->upstream.ntrans);
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        pbuf += sprintf(pbuf, "  %s state: %s", upstream->name,
                state2string(upstream->state, 1));
        pbuf += sprintf(pbuf, ", maxbuf: %d bytes", upstream->buffer_length);
        if (upstream->stalled)
            pbuf += sprintf(pbuf, " (stalled!)");
        pbuf += sprintf(pbuf, "\n");
    }
    pbuf += sprintf(pbuf, " downstreams (total %d transactions)\n",
            transfer->downstream.ntrans);
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist) {
        pbuf += sprintf(pbuf, "  %s state: %s", downstream->name,
                state2string(downstream->state, 0));
        pbuf += sprintf(pbuf, ", maxbuf: %d bytes", downstream->buffer_length);
        if (downstream->state == DOWNSTREAM_STAT_DOWNLOADING)
            pbuf += sprintf(pbuf, ", spliting: %d urbs(%d bytes)", downstream->nurb,
                    downstream->urb_length);
        if (downstream->stalled)
            pbuf += sprintf(pbuf, " (stalled!)");
        pbuf += sprintf(pbuf, "\n");
    }
    pbuf += sprintf(pbuf, "\n");
    return (int)(pbuf - buf);
}

EXPORT_SYMBOL_GPL(rawbulk_transfer_statistics);

int rawbulk_bind_function(int transfer_id, struct usb_function *function, struct
        usb_ep *bulk_out, struct usb_ep *bulk_in,
        rawbulk_autostart_callback_t autostart_callback) {
    struct rawbulk_transfer *transfer;

    if (!function || !bulk_out || !bulk_in)
        return -EINVAL;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    transfer->downstream.ep = bulk_out;
    transfer->upstream.ep = bulk_in;
    transfer->function = function;
    rawbulk->cdev = function->config->cdev;

    transfer->autostart = autostart_callback;
    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_bind_function);

void rawbulk_unbind_function(int transfer_id) {
    int n;
    int no_functions = 1;
    struct rawbulk_transfer *transfer;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return;

    rawbulk_stop_transactions(transfer_id);
    transfer->downstream.ep = NULL;
    transfer->upstream.ep = NULL;
    transfer->function = NULL;

    for (n = 0; n < _MAX_TID; n ++) {
        if (!!rawbulk->transfer[n].function)
            no_functions = 0;
    }

    if (no_functions)
        rawbulk->cdev = NULL;
}

EXPORT_SYMBOL_GPL(rawbulk_unbind_function);

int rawbulk_bind_host_interface(struct usb_interface *interface,
        rawbulk_intercept_t inceptor) {
    int n;
    int transfer_id;
    struct rawbulk_transfer *transfer;
    struct usb_device *udev;

    if (!interface || !inceptor)
        return -EINVAL;

    transfer_id = interface->cur_altsetting->desc.bInterfaceNumber;
    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    udev = interface_to_usbdev(interface);
    if (!udev)
        return -ENODEV;

    if (!rawbulk->udev) {
        rawbulk->udev = udev;
    }

    /* search host bulk ep for up/downstreams */
    for (n = 0; n < interface->cur_altsetting->desc.bNumEndpoints; n++) {
        struct usb_host_endpoint *endpoint =
            &interface->cur_altsetting->endpoint[n];
        if (usb_endpoint_is_bulk_out(&endpoint->desc))
            transfer->upstream.host_ep = endpoint;
        if (usb_endpoint_is_bulk_in(&endpoint->desc))
            transfer->downstream.host_ep = endpoint;
    }

    if (!transfer->upstream.host_ep || !transfer->downstream.host_ep) {
        lerr("endpoints do not match bulk pair that needed\n");
        return -EINVAL;
    }

    transfer->interface = interface;
    transfer->inceptor = inceptor;

    if (transfer->autostart)
        transfer->autostart(transfer->id);

    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_bind_host_interface);

void rawbulk_unbind_host_interface(struct usb_interface *interface) {
    int n;
    int no_interfaces = 1;
    struct rawbulk_transfer *transfer;
    int transfer_id = interface->cur_altsetting->desc.bInterfaceNumber;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return;

    rawbulk_stop_transactions(transfer_id);
    transfer->upstream.host_ep = NULL;
    transfer->downstream.host_ep = NULL;
    transfer->interface = NULL;
    transfer->inceptor = NULL;

    for (n = 0; n < _MAX_TID; n ++) {
        if(!!rawbulk->transfer[n].interface)
            no_interfaces = 0;
    }

    if (no_interfaces) {
        usb_kill_urb(rawbulk->forwarding_urb);
        usb_free_urb(rawbulk->forwarding_urb);
        rawbulk->forwarding_urb = NULL;
        rawbulk->udev = NULL;
    }
}

EXPORT_SYMBOL_GPL(rawbulk_unbind_host_interface);

int rawbulk_suspend_host_interface(int transfer_id, pm_message_t message) {
    unsigned long flags = 0;
    struct rawbulk_transfer *transfer;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -EINVAL;

    spin_lock_irqsave(&transfer->suspend_lock, flags);
    transfer->suspended = 1;
    spin_unlock_irqrestore(&transfer->suspend_lock, flags);

    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_suspend_host_interface);

int rawbulk_resume_host_interface(int transfer_id) {
    int rc;
    unsigned long flags = 0;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -EINVAL;

    spin_lock_irqsave(&transfer->suspend_lock, flags);
    transfer->suspended = 0;
    spin_unlock_irqrestore(&transfer->suspend_lock, flags);

    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        if (upstream->stalled) {
            transfer->control &= ~STOP_UPSTREAM;
            /* restart transaction again */
            ldbg("restart upstream: %s", upstream->name);
            stop_upstream(upstream);
            rc = start_upstream(upstream);
            if (rc < 0) {
                ldbg("fail to start upstream %s rc %d", upstream->name, rc);
                goto failto_start_upstream;
            }
        }
    }

    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist) {
        if (downstream->stalled) {
            transfer->control &= ~STOP_DOWNSTREAM;
            ldbg("restart downstream: %s", downstream->name);
            stop_downstream(downstream);
            rc = start_downstream(downstream);
            if (rc < 0) {
                ldbg("fail to start downstream %s rc %d", downstream->name, rc);
                goto failto_start_downstream;
            }
        }
    }

    return 0;
failto_start_downstream:
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist)
        stop_downstream(downstream);
failto_start_upstream:
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist)
        stop_upstream(upstream);
    return rc;
}

EXPORT_SYMBOL_GPL(rawbulk_resume_host_interface);

static __init int rawbulk_init(void) {
    int n;

    rawbulk = kzalloc(sizeof *rawbulk, GFP_KERNEL);
    if (!rawbulk)
        return -ENOMEM;

    for (n = 0; n < _MAX_TID; n ++) {
        struct rawbulk_transfer *t = &rawbulk->transfer[n];

        t->id = n;
        INIT_LIST_HEAD(&t->upstream.transactions);
        INIT_LIST_HEAD(&t->downstream.transactions);

        mutex_init(&t->mutex);
        spin_lock_init(&t->lock);
        spin_lock_init(&t->suspend_lock);
        t->suspended = 0;
        t->control = STOP_UPSTREAM | STOP_DOWNSTREAM;

        init_usb_anchor(&t->submitted);
    }

    return 0;
}

module_init (rawbulk_init);

static __exit void rawbulk_exit(void) {
    int n;
    for (n = 0; n < _MAX_TID; n ++)
        rawbulk_stop_transactions(n);

    if (rawbulk->forwarding_urb) {
        usb_kill_urb(rawbulk->forwarding_urb);
        usb_free_urb(rawbulk->forwarding_urb);
        rawbulk->forwarding_urb = NULL;
    }

    kfree(rawbulk);
}

module_exit (rawbulk_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

