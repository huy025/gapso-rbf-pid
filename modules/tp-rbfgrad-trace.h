#ifndef _TP_RBFGRAD_TRACE_H
#define _TP_RBFGRAD_TRACE_H

#include <linux/tracepoint.h>
#include <net/pkt_sched.h>
DECLARE_TRACE(rbfgrad_output,
	TP_PROTO(struct tc_rbfgrad_qopt *ctl),
	TP_ARGS(ctl));
#endif
