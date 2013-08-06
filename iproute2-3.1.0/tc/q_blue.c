/*
 * q_blue.c		BLUE.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Junlong Qiao, <zheolong@126.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"

#include "tc_blue.h"
static void explain(void)
{
	fprintf(stderr, "Usage: ... blue limit BYTES qref PACKETS qold PACKETS a COEFFICIENT b COEFFICIENT\n");
	fprintf(stderr, "               proba PROBABILITY proba_init PROBABILITY proba_max PROBABILITY sampl_period INTEGER [ ecn ]\n");
}


static int double2fixpoint(double d)
{
	int i, ret=0;
	for(i = 1; i < 8*sizeof(int); i++){
		ret <<= 1;
		d *= 2.0;
		if (d >= 1){
			ret |= 1;
			d -= 1.0;		
		}
	}
	return ret;
}

static double fixpoint2double(int f)
{
	int i;
	double ret = 0;
	for(i = 1; i < 8*sizeof(int); i++){
		if(f&1)
			ret += 1.0;
		ret /= 2.0;
		f >>= 1;
	}
	return ret;
}

static int blue_parse_opt(struct qdisc_util *qu, int argc, char **argv, struct nlmsghdr *n)
{
	struct tc_blue_qopt opt;
	int ecn_ok = 0;
	__u8 sbuf[256];
	struct rtattr *tail;

	double qref, qold, proba, a, b, proba_init, proba_max, sampl_period;/* qjl */
	
	memset(&opt, 0, sizeof(opt));

	/* qjl */
	proba = 0.02;
	qref = 50;
	qold = 0;
	a = 0.00001822;
	b = 0.00001816;
	proba_init = 0.02;
	proba_max = 1.0;
	sampl_period = 1;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_size(&opt.limit, *argv)) {
				fprintf(stderr, "Illegal \"limit\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "qref") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%d", &opt.qref) != 1) {
				fprintf(stderr, "Illegal \"qref\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "qold") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%d", &opt.qold) != 1) {
				fprintf(stderr, "Illegal \"qold\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "a") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg", &a) != 1) {
				fprintf(stderr, "Illegal \"a\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "b") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg", &b) != 1) {
				fprintf(stderr, "Illegal \"b\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "proba") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg", &proba) != 1) {
				fprintf(stderr, "Illegal \"proba\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "proba_init") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg", &proba_init) != 1) {
				fprintf(stderr, "Illegal \"proba_init\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "proba_max") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg", &proba_max) != 1) {
				fprintf(stderr, "Illegal \"proba_max\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "sampl_period") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%d", &opt.sampl_period) != 1) {
				fprintf(stderr, "Illegal \"sampl_period\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "ecn") == 0) {
			ecn_ok = 1;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "What is \"%s\"?\n", *argv);
			explain();
			return -1;
		}
		argc--; argv++;
	}
	
	/*其中有些元素本来就是1，所以这个条件判断有问题*/
//	if (!opt.limit || !opt.qref || !opt.qold || !opt.a || !opt.b || !opt.proba || !opt.proba_init || !opt.proba_max || !opt.sampl_period ) {
	//	fprintf(stderr, "Required parameter (limit, qref, qold, a, b, proba, proba_init, proba_max, sampl_period) is missing\n");
	//	return -1;
	//}

	if (opt.qref <= 0) {
		fprintf(stderr, "BLUE: qref must be > 0.\n");
		return -1;
	}

	if (opt.proba_init < 0 || opt.proba_init > 1)
		fprintf(stderr, "BLUE: the init probability must be between 0.0 and 1.0.\n");

	if (opt.proba_max < 0 || opt.proba_max > 1)
		fprintf(stderr, "BLUE: the max probability must be between 0.0 and 1.0.\n");

	if (opt.a < 0 || opt.a > 0.001)
		fprintf(stderr, "BLUE: a must be between 0.0 and 0.001.\n");
	
	opt.proba = double2fixpoint(proba);
	opt.proba_init = double2fixpoint(proba_init);
	opt.proba_max = double2fixpoint(proba_max);
	opt.a = double2fixpoint(a);
	opt.b = double2fixpoint(b);

	if (ecn_ok) {
#ifdef TC_BLUE_ECN
		opt.flags |= TC_BLUE_ECN;
#else
		fprintf(stderr, "BLUE: ECN support is missing in this binary.\n");
		return -1;
#endif
	}

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	addattr_l(n, 1024, TCA_BLUE_PARMS, &opt, sizeof(opt));
	addattr_l(n, 1024, TCA_BLUE_STAB, sbuf, 256);
	tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;
	return 0;
}

static int blue_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_BLUE_STAB+1];
	struct tc_blue_qopt *qopt;

	double proba_init, proba_max, proba, a, b; /*qjl*/

	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_BLUE_STAB, opt);

	if (tb[TCA_BLUE_PARMS] == NULL)
		return -1;
	qopt = RTA_DATA(tb[TCA_BLUE_PARMS]);
	if (RTA_PAYLOAD(tb[TCA_BLUE_PARMS])  < sizeof(*qopt))
		return -1;
	fprintf(f, "limit %s",
		sprint_size(qopt->limit, b1));
#ifdef TC_BLUE_ECN
	if (qopt->flags & TC_BLUE_ECN)
		fprintf(f, "ecn ");
#endif
	/*qjl*/
	proba_init = fixpoint2double(qopt->proba_init);
	proba_max = fixpoint2double(qopt->proba_max);	
	proba = fixpoint2double(qopt->proba);
	a = fixpoint2double(qopt->a);
	b = fixpoint2double(qopt->b);
	if (show_details) {
		fprintf(f, "qref %d qold %d a %lg b %lg proba %lg proba_init %lg proba_max %lg sampl_period %d",
			qopt->qref, qopt->qold, qopt->a, qopt->b, qopt->proba, qopt->proba_init, qopt->proba_max, qopt->sampl_period);
	}
	return 0;
}

static int blue_print_xstats(struct qdisc_util *qu, FILE *f, struct rtattr *xstats)
{
#ifdef TC_BLUE_ECN
	struct tc_blue_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);
	fprintf(f, "  marked %u early %u pdrop %u other %u",
		st->marked, st->early, st->pdrop, st->other);
	return 0;

#endif
	return 0;
}


struct qdisc_util blue_qdisc_util = {
	.id		= "blue",
	.parse_qopt	= blue_parse_opt,
	.print_qopt	= blue_print_opt,
	.print_xstats	= blue_print_xstats,
};
