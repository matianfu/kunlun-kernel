#ifndef _CHARGE_H
#define _CHARGE_H

typedef enum{
    TYPE_UNKNOW = 0,
    TYPE_AC,
    TYPE_USB,
    CHARGE_TYPE_MAX
}charge_type;

extern int bci_report_charge_type(charge_type type);

#endif
