/*
 * Rawbulk Gadget Function Driver from VIA Telecom
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

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#define DRIVER_AUTHOR   "Karfield Chen <kfchen@via-telecom.com>"
#define DRIVER_DESC     "Rawbulk Gadget - transport data from CP to Gadget"
#define DRIVER_VERSION  "1.0.1"
#define DRIVER_NAME     "usb_rawbulk"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/rawbulk.h>

#ifdef DEBUG
#define ldbg(f, a...) printk(KERN_DEBUG "%s - " f "\n", __func__, ##a)
#else
#define ldbg(...) {}
#endif

#define F_NAME_MODEM    "rawbulk-modem"
#define F_NAME_ETS      "rawbulk-ets"
#define F_NAME_AT       "rawbulk-at"
#define F_NAME_PCV      "rawbulk-pcv"
#define F_NAME_GPS      "rawbulk-gps"

#define DEFAULT_UPSTREAM_COUNT          16
#define DEFAULT_DOWNSTREAM_COUNT        4
#define DEFAULT_UPSTREAM_BUFFER_SIZE    64
#define DEFAULT_DOWNSTREAM_BUFFER_SIZE  1024
#define DEFAULT_DOWNSTREAM_SPLIT_SIZE   64

#define INTF_DESC       0
#define BULKIN_DESC     1
#define BULKOUT_DESC    2
#define MAX_DESC_ARRAY  4

#define NAME_BUFFSIZE   64
#define MAX_ATTRIBUTES    10

#define MAX_TTY_RX          4
#define MAX_TTY_RX_PACKAGE  512
#define MAX_TTY_TX          8
#define MAX_TTY_TX_PACKAGE  64

struct rawbulk_function {
    int transfer_id;
    char *longname;
    char *shortname;
    struct device dev;

    /* Controls */
    spinlock_t lock;
    int enable: 1;
    int activated: 1; /* set when usb enabled */
    int tty_opened: 1;

    struct work_struct activator; /* asynic transaction starter */

    struct wake_lock keep_awake;

    /* USB Gadget related */
    struct usb_function function;
    struct usb_composite_dev *cdev;
    struct usb_ep *bulk_out, *bulk_in;

	int rts_state;		/* Handshaking pins (outputs) */
	int dtr_state;
	int cts_state;		/* Handshaking pins (inputs) */
	int dsr_state;
	int dcd_state;
	int ri_state;

    /* TTY related */
    struct tty_struct *tty;
	struct tty_port port;
    spinlock_t tx_lock;
    struct list_head tx_free;
    struct list_head tx_inproc;
    spinlock_t rx_lock;
    struct list_head rx_free;
    struct list_head rx_inproc;
    struct list_head rx_throttled;
    unsigned int last_pushed;

    /* Transfer Controls */
    int nups;
    int ndowns;
    int upsz;
    int downsz;
    int splitsz;

    int autostart;

    /* Descriptors and Strings */
    struct usb_descriptor_header *fs_descs[MAX_DESC_ARRAY];
    struct usb_descriptor_header *hs_descs[MAX_DESC_ARRAY];
    struct usb_string string_defs[2];
    struct usb_gadget_strings string_table;
    struct usb_gadget_strings *strings[2];
    struct usb_interface_descriptor interface;
    struct usb_endpoint_descriptor fs_bulkin_endpoint;
    struct usb_endpoint_descriptor hs_bulkin_endpoint;
    struct usb_endpoint_descriptor fs_bulkout_endpoint;
    struct usb_endpoint_descriptor hs_bulkout_endpoint;

    /* Sysfs Accesses */
    int max_attrs;
    struct device_attribute attr[MAX_ATTRIBUTES];
} *_fn_table[_MAX_TID] = { NULL };

/* USB gadget framework is not very strong, and not very compatiable with some
 * controller, such as musb.
 * in musb driver, the usb_request's member list is used internally, but in some
 * applications it used in function driver too. to avoid this, here we
 * re-wrap the usb_request */
struct usb_request_wrapper {
    struct list_head list;
    struct usb_request *request;
    int length;
    char buffer[0];
};

static struct usb_request_wrapper *get_wrapper(struct usb_request *req) {
    if (!req->buf)
        return NULL;
    return container_of(req->buf, struct usb_request_wrapper, buffer);
}

/* Internal TTY functions/data */
static int rawbulk_tty_stop_io(struct rawbulk_function *fn);
static int rawbulk_tty_start_io(struct rawbulk_function *fn);
static int rawbulk_tty_alloc_request(struct rawbulk_function *fn);
static void rawbulk_tty_free_request(struct rawbulk_function *fn);
static int rawbulk_tty_activate(struct tty_port *port, struct tty_struct
        *tty);
static void rawbulk_tty_deactivate(struct tty_port *port);
static int rawbulk_tty_install(struct tty_driver *driver, struct tty_struct
        *tty);
static void rawbulk_tty_cleanup(struct tty_struct *tty);
static int rawbulk_tty_open(struct tty_struct *tty, struct file *flip);
static void rawbulk_tty_hangup(struct tty_struct *tty);
static void rawbulk_tty_close(struct tty_struct *tty, struct file *flip);
static int rawbulk_tty_write(struct tty_struct *tty, const unsigned char *buf,
        int count);
static int rawbulk_tty_write_room(struct tty_struct *tty);
static int rawbulk_tty_chars_in_buffer(struct tty_struct *tty);
static void rawbulk_tty_throttle(struct tty_struct *tty);
static void rawbulk_tty_unthrottle(struct tty_struct *tty);
static void rawbulk_tty_set_termios(struct tty_struct *tty, struct ktermios
        *old);
static int rawbulk_tty_tiocmget(struct tty_struct *tty, struct file *file);
static int rawbulk_tty_tiocmset(struct tty_struct *tty, struct file *file,
        unsigned int set, unsigned int clear);

static struct tty_port_operations rawbulk_tty_port_ops = {
	.activate = rawbulk_tty_activate,
	.shutdown = rawbulk_tty_deactivate,
};

static struct tty_operations rawbulk_tty_ops = {
	.open =			rawbulk_tty_open,
	.close =		rawbulk_tty_close,
	.write =		rawbulk_tty_write,
	.hangup = 		rawbulk_tty_hangup,
	.write_room =	rawbulk_tty_write_room,
	.set_termios =	rawbulk_tty_set_termios,
	.throttle =		rawbulk_tty_throttle,
	.unthrottle =	rawbulk_tty_unthrottle,
	.chars_in_buffer =	rawbulk_tty_chars_in_buffer,
	.tiocmget =		rawbulk_tty_tiocmget,
	.tiocmset =		rawbulk_tty_tiocmset,
	.cleanup = 		rawbulk_tty_cleanup,
	.install = 		rawbulk_tty_install,
};

struct tty_driver *rawbulk_tty_driver;

static inline struct rawbulk_function *rawbulk_get_by_index(int index) {
    if (index >= 0 && index < _MAX_TID)
        return _fn_table[index];
    return NULL;
}

static inline int check_enable_state(struct rawbulk_function *fn) {
    int enab;
    unsigned long flags;
    spin_lock_irqsave(&fn->lock, flags);
    enab = fn->enable? 1: 0;
    spin_unlock_irqrestore(&fn->lock, flags);
    return enab;
}

static inline int check_tty_opened(struct rawbulk_function *fn) {
    int opened;
    unsigned long flags;
    spin_lock_irqsave(&fn->lock, flags);
    opened = fn->tty_opened? 1: 0;
    spin_unlock_irqrestore(&fn->lock, flags);
    return opened;
}

static inline void set_enable_state(struct rawbulk_function *fn, int enab) {
    unsigned long flags;
    spin_lock_irqsave(&fn->lock, flags);
    fn->enable = !!enab;
    spin_unlock_irqrestore(&fn->lock, flags);
}

static inline void set_tty_opened(struct rawbulk_function *fn, int opened) {
    unsigned long flags;
    spin_lock_irqsave(&fn->lock, flags);
    fn->tty_opened = !!opened;
    spin_unlock_irqrestore(&fn->lock, flags);
}

#define port_to_rawbulk(p) container_of(p, struct rawbulk_function, port)
#define function_to_rawbulk(f) container_of(f, struct rawbulk_function, function)

static struct usb_request_wrapper *get_req(struct list_head *head, spinlock_t
        *lock) {
    unsigned long flags;
    struct usb_request_wrapper *req = NULL;
    spin_lock_irqsave(lock, flags);
    if (!list_empty(head)) {
		req = list_first_entry(head, struct usb_request_wrapper, list);
		list_del(&req->list);
    }
    spin_unlock_irqrestore(lock, flags);
    return req;
}

static void put_req(struct usb_request_wrapper *req, struct list_head *head,
        spinlock_t *lock) {
    unsigned long flags;
    spin_lock_irqsave(lock, flags);
	list_add_tail(&req->list, head);
    spin_unlock_irqrestore(lock, flags);
}

static void move_req(struct usb_request_wrapper *req, struct list_head *head,
        spinlock_t *lock) {
    unsigned long flags;
    spin_lock_irqsave(lock, flags);
	list_move_tail(&req->list, head);
    spin_unlock_irqrestore(lock, flags);
}

static void insert_req(struct usb_request_wrapper *req, struct list_head *head,
        spinlock_t *lock) {
    unsigned long flags;
    spin_lock_irqsave(lock, flags);
	list_add(&req->list, head);
    spin_unlock_irqrestore(lock, flags);
}

static int control_dtr(int set) {
    struct usb_ctrlrequest ctrl = {
        .bRequestType = USB_DIR_OUT | USB_TYPE_VENDOR, //0x40
        .bRequest = 0x01,
        .wLength = 0,
        .wIndex = 0,
    };

    ctrl.wValue = cpu_to_le16(!!set);
    return rawbulk_forward_ctrlrequest(&ctrl);
}

static void init_endpoint_desc(struct usb_endpoint_descriptor *epdesc, int in,
        int maxpacksize) {
    struct usb_endpoint_descriptor template = {
        .bLength =      USB_DT_ENDPOINT_SIZE,
        .bDescriptorType =  USB_DT_ENDPOINT,
        .bmAttributes =     USB_ENDPOINT_XFER_BULK,
    };

    *epdesc = template;
    if (in)
        epdesc->bEndpointAddress = USB_DIR_IN;
    else
        epdesc->bEndpointAddress = USB_DIR_OUT;
    epdesc->wMaxPacketSize = cpu_to_le16(maxpacksize);
}

static void init_interface_desc(struct usb_interface_descriptor *ifdesc) {
    struct usb_interface_descriptor template = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = 0xff,//USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass = 0xff,
        .bInterfaceProtocol = 0xff,
        .iInterface = 0,
    };

    *ifdesc = template;
}

static ssize_t rawbulk_attr_show(struct device *dev, struct device_attribute
        *attr, char *buf);
static ssize_t rawbulk_attr_store(struct device *dev, struct device_attribute
        *attr, const char *buf, size_t count);

static inline void add_device_attr(struct rawbulk_function *fn, int n, const char
        *name, int mode) {
    if (n < MAX_ATTRIBUTES) {
        fn->attr[n].attr.name = name;
        fn->attr[n].attr.mode = mode;
        fn->attr[n].show = rawbulk_attr_show;
        fn->attr[n].store = rawbulk_attr_store;
    }
}

static void simple_setup_complete(struct usb_ep *ep,
                struct usb_request *req) {
    ;//DO NOTHING
}

static int rawbulk_function_setup(struct usb_function *f, const struct
        usb_ctrlrequest *ctrl) {
    struct rawbulk_function *fn = function_to_rawbulk(f);

    if (!check_enable_state(fn) || rawbulk_transfer_state(fn->transfer_id) < 0) {
        struct usb_composite_dev *cdev = f->config->cdev;
        struct usb_request *req = cdev->req;
        if (ctrl->bRequestType == (USB_DIR_IN | USB_TYPE_VENDOR)) {
            /* handle request when the function is disabled */
            if (ctrl->bRequest == 0x02 || ctrl->bRequest == 0x05) {
                /* return CD status or connection status */
                *(unsigned char *)req->buf = 0x0;
                req->length = 1;
                req->complete = simple_setup_complete;
            } else if (ctrl->bRequest == 0x04) /* return CBP ID */
                req->length = sprintf(req->buf, "CBP_KUNLUN");
            else req->length = 0;
            req->zero = 0;
            return usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
        } else
            /* Complete the status stage */
            return usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
    }

    if (check_enable_state(fn)) {
        return rawbulk_forward_ctrlrequest(ctrl);
    }

    return -EOPNOTSUPP;
}

static int which_attr(struct rawbulk_function *fn, struct device_attribute
        *attr) {
    int n;
    for (n = 0; n < fn->max_attrs; n ++) {
        if (attr == &fn->attr[n])
            return n;
    }
    return -1;
}

static ssize_t rawbulk_attr_show(struct device *dev, struct device_attribute *attr,
        char *buf) {
    struct rawbulk_function *fn = dev_get_drvdata(dev);
    int enab = check_enable_state(fn);
    switch (which_attr(fn, attr)) {
        case 0: return sprintf(buf, "%d", enab);
        case 1: return rawbulk_transfer_statistics(fn->transfer_id, buf);
        case 2: return sprintf(buf, "%d", enab? fn->nups: -1);
        case 3: return sprintf(buf, "%d", enab? fn->ndowns: -1);
        case 4: return sprintf(buf, "%d", enab? fn->upsz: -1);
        case 5: return sprintf(buf, "%d", enab? fn->downsz: -1);
        case 6: return sprintf(buf, "%d", enab? fn->splitsz: -1);
        case 7: return sprintf(buf, "%s", fn->autostart ? "enable": "disable");
    }
    return 0;
}

static ssize_t rawbulk_attr_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count) {
    int rc = 0;
    struct rawbulk_function *fn = dev_get_drvdata(dev);
    int enable;
    int nups = fn->nups;
    int ndowns = fn->ndowns;
    int upsz = fn->upsz;
    int downsz = fn->downsz;
    int splitsz = fn->splitsz;

    switch (which_attr(fn, attr)) {
        case 0:
            enable = simple_strtoul(buf, NULL, 10);
            printk("%s - tid %d enable %d activated %d\n", __func__,
                    fn->transfer_id, enable, fn->activated);
            if (check_enable_state(fn) != (!!enable)) {
                set_enable_state(fn, enable);
                if (!!enable && fn->activated) {
                    if (fn->transfer_id == RAWBULK_TID_MODEM) {
                        /* clear DTR to endup last session between AP and CP */
                        control_dtr(1);
                        msleep(10);
                        control_dtr(0);
                    }

                    /* Stop TTY i/o */
                    rawbulk_tty_stop_io(fn);

                    /* Start rawbulk transfer */
                    wake_lock(&fn->keep_awake);
                    rc = rawbulk_start_transactions(fn->transfer_id, nups,
                            ndowns, upsz, downsz, splitsz);
                    if (rc < 0) {
                        set_enable_state(fn, !enable);
                        rawbulk_tty_start_io(fn);
                    }
                } else {
                    /* Stop rawbulk transfer */
                    rawbulk_stop_transactions(fn->transfer_id);
                    if (fn->transfer_id == RAWBULK_TID_MODEM) {
                        /* clear DTR automatically when disable modem rawbulk */
                        control_dtr(1);
                        msleep(10);
                        control_dtr(0);
                    }
                    wake_unlock(&fn->keep_awake);

                    /* Start TTY i/o */
                    rawbulk_tty_start_io(fn);
                }
            }
            goto exit;
        case 2:
            nups = simple_strtoul(buf, NULL, 10);
            break;
        case 3:
            ndowns = simple_strtoul(buf, NULL, 10);
            break;
        case 4:
            upsz = simple_strtoul(buf, NULL, 10);
            break;
        case 5:
            downsz = simple_strtoul(buf, NULL, 10);
            break;
        case 6:
            splitsz = simple_strtoul(buf, NULL, 10);
            break;
        case 7:
            if (!strncmp(buf, "enable", 5))
                fn->autostart = 1;
            else if (!strncmp(buf, "disable", 6))
                fn->autostart = 0;
            goto exit;
        case 8:
            if (fn->transfer_id == RAWBULK_TID_MODEM) {
                if (check_enable_state(fn))
                    control_dtr(simple_strtoul(buf, NULL, 10));
            }
        default:
            goto exit;
    }

    if (!check_enable_state(fn))
        goto exit;

    if (!fn->activated)
        goto exit;

    rawbulk_stop_transactions(fn->transfer_id);
    rc = rawbulk_start_transactions(fn->transfer_id, nups, ndowns, upsz, downsz,
            splitsz);
    if (rc >= 0) {
        fn->nups = nups;
        fn->ndowns = ndowns;
        fn->upsz = upsz;
        fn->downsz = downsz;
        fn->splitsz = splitsz;
    } else {
        rawbulk_stop_transactions(fn->transfer_id);
        wake_unlock(&fn->keep_awake);
        set_enable_state(fn, 0);
    }

exit:
    return count;
}

static void rawbulk_auto_start(int transfer_id) {
    int rc;
    struct rawbulk_function *fn = rawbulk_get_by_index(transfer_id);
    if (!fn || fn->autostart == 0)
        return;

    if (check_enable_state(fn) && fn->activated) {
        printk(KERN_DEBUG "start %s again automatically.\n", fn->longname);
        rc = rawbulk_start_transactions(transfer_id, fn->nups,
                fn->ndowns, fn->upsz, fn->downsz, fn->splitsz);
        if (rc < 0) {
            set_enable_state(fn, 0);
            rawbulk_tty_start_io(fn);
        }
    }
}

static int rawbulk_create_files(struct rawbulk_function *fn) {
    int n, rc;
    for (n = 0; n < fn->max_attrs; n ++){
        rc = device_create_file(&fn->dev, &fn->attr[n]);
        if (rc < 0) {
            while (--n >= 0)
                device_remove_file(&fn->dev, &fn->attr[n]);
            return rc;
        }
    }
    return 0;
}

static void rawbulk_remove_files(struct rawbulk_function *fn) {
    int n = fn->max_attrs;
    while (--n >= 0)
        device_remove_file(&fn->dev, &fn->attr[n]);
}

static int rawbulk_function_bind(struct usb_configuration *c, struct
        usb_function *f) {
    int rc, ifnum;
    struct device *dev;
    struct rawbulk_function *fn = function_to_rawbulk(f);
    struct usb_gadget *gadget = c->cdev->gadget;
    struct usb_ep *ep_out, *ep_in;

    rc = usb_interface_id(c, f);
    if (rc < 0)
        return rc;
    ifnum = rc;

    fn->interface.bInterfaceNumber = cpu_to_le16(ifnum);

    ep_out = usb_ep_autoconfig(gadget, (struct usb_endpoint_descriptor *)
            fn->fs_descs[BULKOUT_DESC]);
    if (!ep_out)
        return -ENOMEM;

    ep_in = usb_ep_autoconfig(gadget, (struct usb_endpoint_descriptor *)
            fn->fs_descs[BULKIN_DESC]);
    if (!ep_in) {
        usb_ep_disable(ep_out);
        return -ENOMEM;
    }

    ep_out->driver_data = fn;
    ep_in->driver_data = fn;
    fn->bulk_out = ep_out;
    fn->bulk_in = ep_in;

    if (gadget_is_dualspeed(gadget)) {
        fn->hs_bulkin_endpoint.bEndpointAddress =
            fn->fs_bulkin_endpoint.bEndpointAddress;
        fn->hs_bulkout_endpoint.bEndpointAddress =
            fn->fs_bulkout_endpoint.bEndpointAddress;
    }

    fn->cdev = c->cdev;
    fn->activated = 0;

    spin_lock_init(&fn->tx_lock);
    spin_lock_init(&fn->rx_lock);
    INIT_LIST_HEAD(&fn->tx_free);
    INIT_LIST_HEAD(&fn->rx_free);
    INIT_LIST_HEAD(&fn->tx_inproc);
    INIT_LIST_HEAD(&fn->rx_inproc);
    INIT_LIST_HEAD(&fn->rx_throttled);

    fn->tty_opened = 0;
    fn->last_pushed = 0;
    tty_port_init(&fn->port);
    fn->port.ops = &rawbulk_tty_port_ops;

    /* Bring up the tty device */
    dev = tty_register_device(rawbulk_tty_driver, fn->transfer_id, &fn->dev);
    if (IS_ERR(dev))
        printk(KERN_ERR "Failed to attach ttyRB%d to %s device.\n",
                fn->transfer_id, fn->longname);

    return rawbulk_bind_function(fn->transfer_id, f, ep_out, ep_in,
            rawbulk_auto_start);
}

static void rawbulk_function_unbind(struct usb_configuration *c, struct
        usb_function *f) {
    struct rawbulk_function *fn = function_to_rawbulk(f);

    rawbulk_remove_files(fn);
    device_unregister(&fn->dev);
    rawbulk_unbind_function(fn->transfer_id);
}

static void do_activate(struct work_struct *data) {
    struct rawbulk_function *fn = container_of(data, struct rawbulk_function,
            activator);
    int rc;
    printk("Rawbulk async work: tid %d to be %s has enabled? %d\n",
            fn->transfer_id, fn->activated? "activted": "detached",
            check_enable_state(fn));
    if (fn->activated) { /* enumerated */
        /* enabled endpoints */
        rc = usb_ep_enable(fn->bulk_out, ep_choose(fn->cdev->gadget,
                    &fn->hs_bulkout_endpoint, &fn->fs_bulkout_endpoint));
        if (rc < 0)
            return;
        rc = usb_ep_enable(fn->bulk_in, ep_choose(fn->cdev->gadget,
                    &fn->hs_bulkin_endpoint, &fn->fs_bulkin_endpoint));
        if (rc < 0) {
            usb_ep_disable(fn->bulk_out);
            return;
        }

        fn->bulk_out->driver_data = fn;
        fn->bulk_in->driver_data = fn;

        /* start rawbulk if enabled */
        if (check_enable_state(fn)) {
            wake_lock(&fn->keep_awake);
            rc = rawbulk_start_transactions(fn->transfer_id, fn->nups,
                    fn->ndowns, fn->upsz, fn->downsz, fn->splitsz);
            if (rc < 0)
                set_enable_state(fn, 0);
        }

        /* start tty io */
        rc = rawbulk_tty_alloc_request(fn);
        if (rc < 0)
            return;
        if (!check_enable_state(fn))
            rawbulk_tty_start_io(fn);
    } else { /* disconnect */
        if (check_enable_state(fn)) {
            if (fn->transfer_id == RAWBULK_TID_MODEM) {
                /* this in interrupt, but DTR need be set firstly then clear it
                 * */
                //control_dtr(0);
            }
            rawbulk_stop_transactions(fn->transfer_id);
            /* keep the enable state, so we can enable again in next time */
            //set_enable_state(fn, 0); 
            wake_unlock(&fn->keep_awake);
        } else
            rawbulk_tty_stop_io(fn);

        rawbulk_tty_free_request(fn);

        usb_ep_disable(fn->bulk_out);
        usb_ep_disable(fn->bulk_in);

        fn->bulk_out->driver_data = NULL;
        fn->bulk_in->driver_data = NULL;
    }
}

static int rawbulk_function_setalt(struct usb_function *f, unsigned intf,
        unsigned alt) {
    struct rawbulk_function *fn = function_to_rawbulk(f);
    fn->activated = 1;
    schedule_work(&fn->activator);
    return 0;
}

static void rawbulk_function_disable(struct usb_function *f) {
    struct rawbulk_function *fn = function_to_rawbulk(f);
    fn->activated = 0;
    schedule_work(&fn->activator);
}

static int rawbulk_function_add(struct usb_configuration *c,
        int transfer_id) {
    int rc;
    struct rawbulk_function *fn = rawbulk_get_by_index(transfer_id);

    printk(KERN_INFO "add %s to config.\n", fn->longname);

    if (!fn)
        return -ENOMEM;

    rc = usb_string_id(c->cdev);
    if (rc < 0)
        return rc;

    fn->string_defs[transfer_id].id = rc;
    fn->function.name = fn->longname;

    return usb_add_function(c, &fn->function);
}

/******************************************************************************/

/* Call this after all request has detached! */
static void rawbulk_tty_free_request(struct rawbulk_function *fn) {
    struct usb_request_wrapper *req;

    while ((req = get_req(&fn->rx_free, &fn->rx_lock))) {
        usb_ep_free_request(fn->bulk_out, req->request);
        kfree(req);
    }

    while ((req = get_req(&fn->tx_free, &fn->tx_lock))) {
        usb_ep_free_request(fn->bulk_in, req->request);
        kfree(req);
    }
}

static void rawbulk_tty_rx_complete(struct usb_ep *ep, struct usb_request *req);
static void rawbulk_tty_tx_complete(struct usb_ep *ep, struct usb_request *req);

static int rawbulk_tty_alloc_request(struct rawbulk_function *fn) {
    int n;
    struct usb_request_wrapper *req;
    unsigned long flags;

    spin_lock_irqsave(&fn->lock, flags);
    if (!fn->bulk_out || !fn->bulk_in) {
        spin_unlock_irqrestore(&fn->lock, flags);
        return -ENODEV;
    }
    spin_unlock_irqrestore(&fn->lock, flags);

    /* Allocate and init rx request */
    for (n = 0; n < MAX_TTY_RX; n ++) {
        req = kmalloc(sizeof(struct usb_request_wrapper) +
                MAX_TTY_RX_PACKAGE, GFP_KERNEL);
        if (!req)
            break;

        req->request = usb_ep_alloc_request(fn->bulk_out, GFP_KERNEL);
        if (!req->request) {
            kfree(req);
            break;
        }

        INIT_LIST_HEAD(&req->list);
        req->length = MAX_TTY_RX_PACKAGE;
        req->request->buf = req->buffer;
        req->request->length = MAX_TTY_RX_PACKAGE;
        req->request->complete = rawbulk_tty_rx_complete;
        put_req(req, &fn->rx_free, &fn->rx_lock);
    }

    if (n < MAX_TTY_RX) {
        /* free allocated request */
        rawbulk_tty_free_request(fn);
        return -ENOMEM;
    }

    /* Allocate and init tx request */
    for (n = 0; n < MAX_TTY_TX; n ++) {
        req = kmalloc(sizeof(struct usb_request_wrapper) +
                MAX_TTY_TX_PACKAGE, GFP_KERNEL);
        if (!req)
            break;

        req->request = usb_ep_alloc_request(fn->bulk_in, GFP_KERNEL);
        if (!req->request) {
            kfree(req);
            break;
        }

        INIT_LIST_HEAD(&req->list);
        req->length = MAX_TTY_TX_PACKAGE;
        req->request->zero = 0;
        req->request->buf = req->buffer;
        req->request->complete = rawbulk_tty_tx_complete;
        put_req(req, &fn->tx_free, &fn->tx_lock);
    }

    if (n < MAX_TTY_TX) {
        /* free allocated request */
        rawbulk_tty_free_request(fn);
        return -ENOMEM;
    }

    return 0;
}

static int rawbulk_tty_stop_io(struct rawbulk_function *fn) {
    struct usb_request_wrapper *req;

    while ((req = get_req(&fn->rx_inproc, &fn->rx_lock))) {
        if (req->request->status == -EINPROGRESS) {
            put_req(req, &fn->rx_inproc, &fn->rx_lock);
            usb_ep_dequeue(fn->bulk_out, req->request);
        }
    }

    while ((req = get_req(&fn->tx_inproc, &fn->tx_lock))) {
        if (req->request->status == -EINPROGRESS) {
            put_req(req, &fn->tx_inproc, &fn->tx_lock);
            usb_ep_dequeue(fn->bulk_in, req->request);
        }
    }
    return 0;
}

static int rawbulk_tty_start_io(struct rawbulk_function *fn) {
    int ret;
    struct usb_request_wrapper *req;

    if (!check_tty_opened(fn))
        return -ENODEV;

    while ((req = get_req(&fn->rx_free, &fn->rx_lock))) {
        put_req(req, &fn->rx_inproc, &fn->rx_lock);
        ret = usb_ep_queue(fn->bulk_out, req->request, GFP_ATOMIC);
        if (ret < 0) {
            move_req(req, &fn->rx_free, &fn->rx_lock);
            return ret;
        }
    }

    return 0;
}

/* Complete the data send stage */
static void rawbulk_tty_tx_complete(struct usb_ep *ep, struct usb_request *req) {
    struct rawbulk_function *fn = ep->driver_data;
    struct usb_request_wrapper *r = get_wrapper(req);

    move_req(r, &fn->tx_free, &fn->tx_lock);
}

static void rawbulk_tty_push_data(struct rawbulk_function *fn) {
    struct usb_request_wrapper *r;
    struct tty_struct *tty;

    tty = tty_port_tty_get(&fn->port);
    tty->minimum_to_wake = 1; /* wakeup to read even pushed only once */
    /* walk the throttled list, push all the data to TTY */
    while ((r = get_req(&fn->rx_throttled, &fn->rx_lock))) {
        char *sbuf;
        int push_count;
        struct usb_request *req = r->request;

        if (req->status < 0) {
            put_req(r, &fn->rx_free, &fn->rx_lock);
            continue;
        }
        sbuf = req->buf + fn->last_pushed;
        push_count = req->actual - fn->last_pushed;
        if (push_count) {
            int count = tty_insert_flip_string(tty, sbuf, push_count);
            if (count < push_count) {
                /* We met throttled again */
                fn->last_pushed += count;
                if (count)
                    tty_flip_buffer_push(tty);
                insert_req(r, &fn->rx_throttled, &fn->rx_lock);
                break;
            }
            tty_flip_buffer_push(tty);
            fn->last_pushed = 0;
        }
        put_req(r, &fn->rx_free, &fn->rx_lock);
    }
    tty_kref_put(tty);
}

/* Recieve data from host */
static void rawbulk_tty_rx_complete(struct usb_ep *ep, struct usb_request *req) {
    int ret;
    struct usb_request_wrapper *r = get_wrapper(req);
    struct rawbulk_function *fn = ep->driver_data;
    struct tty_struct *tty = fn->tty;

    if (req->status < 0 || !check_tty_opened(fn)) {
        move_req(r, &fn->rx_free, &fn->rx_lock);
        return;
    }

    if (test_bit(TTY_THROTTLED, &tty->flags)) {
        /* Give up to resubmitted the request */
        move_req(r, &fn->rx_free, &fn->rx_lock);
        return;
    } else {
        /* Move the request to the throttled queue */
        move_req(r, &fn->rx_throttled, &fn->rx_lock);
        /* Start to push data */
        rawbulk_tty_push_data(fn);
    }

    /* Re-queue the request again */
    while ((r = get_req(&fn->rx_free, &fn->rx_lock))) {
        put_req(r, &fn->rx_inproc, &fn->rx_lock);
        ret = usb_ep_queue(fn->bulk_out, r->request, GFP_ATOMIC);
        if (ret < 0) {
            move_req(r, &fn->rx_free, &fn->rx_lock);
            break;
        }
    }
}

/* Start to queue request on BULK OUT endpoint, when tty is try to open */
static int rawbulk_tty_activate(struct tty_port *port, struct tty_struct
        *tty) {
    struct rawbulk_function *fn = port_to_rawbulk(port);
    /* Low latency for tty */
    tty->low_latency = 1;
    tty->termios->c_cc[VMIN] = 1;
    return rawbulk_tty_start_io(fn);
}

static void rawbulk_tty_deactivate(struct tty_port *port) {
    struct usb_request_wrapper *req;
    struct rawbulk_function *fn = port_to_rawbulk(port);

    /* This is a little different from stop_io */
    while ((req = get_req(&fn->rx_inproc, &fn->rx_lock))) {
        put_req(req, &fn->rx_inproc, &fn->rx_lock);
        usb_ep_dequeue(fn->bulk_out, req->request);
    }

    while ((req = get_req(&fn->rx_throttled, &fn->rx_lock)))
        put_req(req, &fn->rx_free, &fn->rx_lock);

    while ((req = get_req(&fn->tx_inproc, &fn->tx_lock))) {
        put_req(req, &fn->tx_inproc, &fn->tx_lock);
        usb_ep_dequeue(fn->bulk_in, req->request);
    }
}

static int rawbulk_tty_write(struct tty_struct *tty, const unsigned char *buf,
        int count) {
    int ret;
    int submitted = 0;
    struct usb_request_wrapper *req;
    struct rawbulk_function *fn = tty->driver_data;

    if (check_enable_state(fn))
        return -EBUSY;

    /* Get new request(s) that freed, fill it, queue it to the endpoint */
    while ((req = get_req(&fn->tx_free, &fn->tx_lock))) {
        int length = count - submitted;
        if (length <= 0) {
            put_req(req, &fn->tx_free, &fn->tx_lock);
            break;
        }
        if (length > MAX_TTY_TX_PACKAGE)
            length = MAX_TTY_TX_PACKAGE;
        memcpy(req->buffer, buf + submitted, length);
        req->request->length = length;
        put_req(req, &fn->tx_inproc, &fn->tx_lock);
        ret = usb_ep_queue(fn->bulk_in, req->request, GFP_ATOMIC);
        if (ret < 0) {
            move_req(req, &fn->tx_free, &fn->tx_lock);
            return ret;
        }
        submitted += length;
    }

    return submitted;
}

static int rawbulk_tty_write_room(struct tty_struct *tty) {
    int room = 0;
    unsigned long flags;
    struct usb_request_wrapper *req;
    struct rawbulk_function *fn = tty->driver_data;

    if (check_enable_state(fn))
        return 0;

    spin_lock_irqsave(&fn->tx_lock, flags);
    list_for_each_entry(req, &fn->tx_free, list)
        room += req->length;
    spin_unlock_irqrestore(&fn->tx_lock, flags);

    return room;
}

static int rawbulk_tty_chars_in_buffer(struct tty_struct *tty) {
    int chars = 0;
    unsigned long flags;
    struct usb_request_wrapper *req;
    struct rawbulk_function *fn = tty->driver_data;

    if (check_enable_state(fn))
        return 0;

    spin_lock_irqsave(&fn->rx_lock, flags);
    list_for_each_entry(req, &fn->rx_throttled, list) {
        if (req->request->status < 0)
            continue;
        chars += req->request->actual;
    }
    chars -= fn->last_pushed;
    spin_unlock_irqrestore(&fn->rx_lock, flags);

    return chars;
}

static void rawbulk_tty_throttle(struct tty_struct *tty) {
    struct usb_request_wrapper *req;
    struct rawbulk_function *fn = tty->driver_data;

    /* Stop the processing requests */
    while ((req = get_req(&fn->rx_inproc, &fn->rx_lock))) {
        if (req->request->status == -EINPROGRESS) {
            put_req(req, &fn->rx_inproc, &fn->rx_lock);
            usb_ep_dequeue(fn->bulk_out, req->request);
        }
    }
}

static void rawbulk_tty_unthrottle(struct tty_struct *tty) {
    int ret;
    struct usb_request_wrapper *req;
    struct rawbulk_function *fn = tty->driver_data;

    /* Try to push the throttled requests' data to TTY */
    rawbulk_tty_push_data(fn);

    /* Restart the free requests */
    while ((req = get_req(&fn->rx_free, &fn->rx_lock))) {
        put_req(req, &fn->rx_inproc, &fn->rx_lock);
        ret = usb_ep_queue(fn->bulk_out, req->request, GFP_ATOMIC);
        if (ret < 0) {
            move_req(req, &fn->rx_free, &fn->rx_lock);
            break;
        }
    }
}

static void rawbulk_tty_set_termios(struct tty_struct *tty, struct ktermios
        *old) {
    //struct rawbulk_function *fn = tty->driver_data;
}

static int rawbulk_tty_tiocmget(struct tty_struct *tty, struct file *file) {
    //struct rawbulk_function *fn = tty->driver_data;
    return 0;
}

static int rawbulk_tty_tiocmset(struct tty_struct *tty, struct file *file,
        unsigned int set, unsigned int clear) {
    //struct rawbulk_function *fn = tty->driver_data;
    return 0;
}

static int rawbulk_tty_install(struct tty_driver *driver, struct tty_struct
        *tty) {
    int ret = 0;
    struct rawbulk_function *fn = rawbulk_get_by_index(tty->index);

    if (!fn)
        return -ENODEV;

    ret = tty_init_termios(tty);
    if (ret < 0)
        return ret;

    tty->driver_data = fn;
    fn->tty = tty;

	/* Final install (we use the default method) */
	tty_driver_kref_get(driver);
	tty->count++;
	driver->ttys[tty->index] = tty;
    return ret;
}

static void rawbulk_tty_cleanup(struct tty_struct *tty) {
    struct rawbulk_function *fn = rawbulk_get_by_index(tty->index);
    tty->driver_data = NULL;
    if (fn)
        fn->tty = NULL;
}

static int rawbulk_tty_open(struct tty_struct *tty, struct file *flip) {
    int ret;
    struct rawbulk_function *fn = rawbulk_get_by_index(tty->index);

    if (!fn)
        return -ENODEV;

    tty->driver_data = fn;
    fn->tty = tty;

    if (check_enable_state(fn))
        return -EBUSY;

    if (check_tty_opened(fn))
        return -EBUSY;

    set_tty_opened(fn, 1);

    ret = tty_port_open(&fn->port, tty, flip);
    if (ret < 0)
        return ret;
    return 0;
}

static void rawbulk_tty_hangup(struct tty_struct *tty) {
    struct rawbulk_function *fn = tty->driver_data;
    if (!fn)
        return;
    tty_port_hangup(&fn->port);
}

static void rawbulk_tty_close(struct tty_struct *tty, struct file *flip) {
    struct rawbulk_function *fn = tty->driver_data;
    if (!fn)
        return;

    set_tty_opened(fn, 0);
    tty_port_close(&fn->port, tty, flip);
}

static __init int rawbulk_tty_init(void) {
    int ret;
	struct tty_driver *drv = alloc_tty_driver(_MAX_TID);
	if (!drv)
		return -ENOMEM;

	drv->owner = THIS_MODULE;
	drv->driver_name = "rawbulkport";
	drv->name = "ttyRB";//prefix
	drv->type = TTY_DRIVER_TYPE_SERIAL;
	drv->subtype = SERIAL_TYPE_NORMAL;
	drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	drv->init_termios = tty_std_termios;
	drv->init_termios.c_cflag =
			B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	drv->init_termios.c_lflag = 0;
	drv->init_termios.c_ispeed = 9600;
	drv->init_termios.c_ospeed = 9600;

	tty_set_operations(drv, &rawbulk_tty_ops);

	ret = tty_register_driver(drv);
    if (ret < 0) {
        put_tty_driver(drv);
    } else
        rawbulk_tty_driver = drv;
    return ret;
}

/******************************************************************************/

static void rawbulk_function_release(struct device *dev) {
    ;//DO NOTHING
}

static struct device_driver rawbulk_function_dev_driver = {
    .name = "rawbulk_function",
    .owner = THIS_MODULE,
};

static int rawbulk_platform_probe(struct platform_device *pdev) {
    struct usb_rawbulk_platform_data *pdata = pdev->dev.platform_data;

    printk(KERN_INFO "probe rawbulk platdev\n");

    if (pdata) {
        int n;
        int rc;
        for (n = 0;  n < _MAX_TID; n ++) {
            struct rawbulk_function *fn = rawbulk_get_by_index(n);

            if (pdata->fn_params[n].upstream_transactions > 0)
                fn->nups = pdata->fn_params[n].upstream_transactions;

            if (pdata->fn_params[n].downstream_transactions > 0)
                fn->ndowns = pdata->fn_params[n].downstream_transactions;

            if (pdata->fn_params[n].upstream_buffersize > DEFAULT_UPSTREAM_BUFFER_SIZE)
                fn->upsz = pdata->fn_params[n].upstream_buffersize;

            if (pdata->fn_params[n].downstream_buffersize > DEFAULT_DOWNSTREAM_BUFFER_SIZE)
                fn->downsz = pdata->fn_params[n].downstream_buffersize;

            if (pdata->fn_params[n].downstream_splitcount > 1)
                fn->splitsz = pdata->fn_params[n].downstream_splitcount;

            fn->dev.release = rawbulk_function_release;
            fn->dev.parent = &pdev->dev;
            fn->dev.driver = &rawbulk_function_dev_driver;
            dev_set_drvdata(&fn->dev, fn);
            dev_set_name(&fn->dev, "%s", fn->shortname);

            rc = device_register(&fn->dev);
            if (rc < 0)
                continue;

            rc = rawbulk_create_files(fn);
            if (rc < 0)
                device_unregister(&fn->dev);
        }
    }

    return 0;
}

static struct platform_driver rawbulk_platform_driver = {
    .driver = { .name = "usb_rawbulk", },
    .probe = rawbulk_platform_probe,
};

/* Wrapper for rawbulk modem function */
static int rawbulk_modem_bind_config(struct usb_configuration * c) {
    return rawbulk_function_add(c, RAWBULK_TID_MODEM);
}

static struct android_usb_function rawbulk_modem_function = {
    .name = F_NAME_MODEM,
    .bind_config = rawbulk_modem_bind_config,
};

/* Wrapper for rawbulk ets function */
static int rawbulk_ets_bind_config(struct usb_configuration * c) {
    return rawbulk_function_add(c, RAWBULK_TID_ETS);
}

static struct android_usb_function rawbulk_ets_function = {
    .name = F_NAME_ETS,
    .bind_config = rawbulk_ets_bind_config,
};

/* Wrapper for rawbulk at function */
static int rawbulk_at_bind_config(struct usb_configuration * c) {
    return rawbulk_function_add(c, RAWBULK_TID_AT);
}

static struct android_usb_function rawbulk_at_function = {
    .name = F_NAME_AT,
    .bind_config = rawbulk_at_bind_config,
};

/* Wrapper for rawbulk pcv function */
static int rawbulk_pcv_bind_config(struct usb_configuration * c) {
    return rawbulk_function_add(c, RAWBULK_TID_PCV);
}

static struct android_usb_function rawbulk_pcv_function = {
    .name = F_NAME_PCV,
    .bind_config = rawbulk_pcv_bind_config,
};

/* Wrapper for rawbulk gps function */
static int rawbulk_gps_bind_config(struct usb_configuration * c) {
    return rawbulk_function_add(c, RAWBULK_TID_GPS);
}

static struct android_usb_function rawbulk_gps_function = {
    .name = F_NAME_GPS,
    .bind_config = rawbulk_gps_bind_config,
};

static void __init init_rawbulk_container(struct rawbulk_function *fn,
        int transfer_id) {
    /* init default features of rawbulk */
    fn->nups = DEFAULT_UPSTREAM_COUNT;
    fn->ndowns = DEFAULT_DOWNSTREAM_COUNT;
    fn->upsz = DEFAULT_UPSTREAM_BUFFER_SIZE;
    fn->downsz = DEFAULT_DOWNSTREAM_BUFFER_SIZE;
    fn->splitsz = DEFAULT_DOWNSTREAM_SPLIT_SIZE;

    /* start transfer if CBP re-connect automatically */
    fn->autostart = 1;

    /* init descriptors */
    init_interface_desc(&fn->interface);
    init_endpoint_desc(&fn->fs_bulkin_endpoint, 1, 64);
    init_endpoint_desc(&fn->hs_bulkin_endpoint, 1, 64);
    init_endpoint_desc(&fn->fs_bulkout_endpoint, 0, 64);
    init_endpoint_desc(&fn->hs_bulkout_endpoint, 0, 64);

    fn->fs_descs[INTF_DESC] = (struct usb_descriptor_header *) &fn->interface;
    fn->fs_descs[BULKIN_DESC] = (struct usb_descriptor_header *) &fn->fs_bulkin_endpoint;
    fn->fs_descs[BULKOUT_DESC] = (struct usb_descriptor_header *) &fn->fs_bulkout_endpoint;

    fn->hs_descs[INTF_DESC] = (struct usb_descriptor_header *) &fn->interface;
    fn->hs_descs[BULKIN_DESC] = (struct usb_descriptor_header *) &fn->hs_bulkin_endpoint;
    fn->hs_descs[BULKOUT_DESC] = (struct usb_descriptor_header *) &fn->hs_bulkout_endpoint;

    /* init strings */
    switch (transfer_id) {
        case RAWBULK_TID_MODEM:
            fn->longname = "rawbulk-modem";
            fn->shortname = "data";
            fn->string_defs[0].s = "Modem Port";
            break;
        case RAWBULK_TID_ETS:
            fn->longname = "rawbulk-ets";
            fn->shortname = "ets";
            fn->string_defs[0].s = "ETS Port";
            break;
        case RAWBULK_TID_AT:
            fn->longname = "rawbulk-at";
            fn->shortname = "atc";
            fn->string_defs[0].s = "AT Command Channel";
            break;
        case RAWBULK_TID_PCV:
            fn->longname = "rawbulk-pcv";
            fn->shortname = "pcv";
            fn->string_defs[0].s = "PCM Voice";
            break;
        case RAWBULK_TID_GPS:
            fn->longname = "rawbulk-gps";
            fn->shortname = "gps";
            fn->string_defs[0].s = "LBS GPS"; break;
        default:
            fn->shortname = (fn->longname = "unknown");
            fn->string_defs[0].s = "Undefined Interface";
    }

    fn->string_table.language = 0x0409;
    fn->string_table.strings = fn->string_defs;
    fn->strings[0] = &fn->string_table;
    fn->strings[1] = NULL;

    fn->transfer_id = transfer_id;

    /* init function callbacks */
    fn->function.strings = fn->strings;
    fn->function.descriptors = fn->fs_descs;
    fn->function.hs_descriptors = fn->hs_descs;
    fn->function.setup = rawbulk_function_setup;
    fn->function.bind = rawbulk_function_bind;
    fn->function.unbind = rawbulk_function_unbind;
    fn->function.set_alt = rawbulk_function_setalt;
    fn->function.disable = rawbulk_function_disable;

    /* init device attributes */
    add_device_attr(fn, 0, "enable", 0777);
    add_device_attr(fn, 1, "statistics", 0666);
    add_device_attr(fn, 2, "nups", 0777);
    add_device_attr(fn, 3, "ndowns", 0777);
    add_device_attr(fn, 4, "ups_size", 0777);
    add_device_attr(fn, 5, "downs_size", 0777);
    add_device_attr(fn, 6, "split_size", 0777);
    add_device_attr(fn, 7, "autostart", 0777);

    fn->max_attrs = 8;

    if (transfer_id == RAWBULK_TID_MODEM) {
        add_device_attr(fn, 8, "dtr", 0222);
        fn->max_attrs ++;
    }

    INIT_WORK(&fn->activator, do_activate);

    spin_lock_init(&fn->lock);
    wake_lock_init(&fn->keep_awake, WAKE_LOCK_SUSPEND, fn->longname);
}

static int __init init(void) {
    int n;
    int rc;

    printk(KERN_INFO "rawbulk functions init.\n");

    for (n = 0; n < _MAX_TID; n ++) {
        /* Prealloc function's buffer */
        struct rawbulk_function *fn = kzalloc(sizeof *fn, GFP_KERNEL);
        if (!fn) {
            while (n --)
                kfree(_fn_table[n]);
            return -ENOMEM;
        }

        _fn_table[n] = fn;
        init_rawbulk_container(fn, n);
    }

    rc = rawbulk_tty_init();
    if (rc < 0) {
        printk(KERN_ERR "failed to init rawbulk tty driver.\n");
        return rc;
    }

	rc = platform_driver_register(&rawbulk_platform_driver);
    if (rc < 0) {
        tty_unregister_driver(rawbulk_tty_driver);
        put_tty_driver(rawbulk_tty_driver);
        return rc;
    }

	android_register_function(&rawbulk_modem_function);
	android_register_function(&rawbulk_ets_function);
	android_register_function(&rawbulk_at_function);
	android_register_function(&rawbulk_pcv_function);
	android_register_function(&rawbulk_gps_function);
    return 0;
}

module_init(init);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
