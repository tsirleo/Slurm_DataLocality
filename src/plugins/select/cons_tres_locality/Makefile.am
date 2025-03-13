# Makefile for select/cons_tres_locality plugin

AUTOMAKE_OPTIONS = foreign

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

AM_CPPFLAGS = -DSLURM_PLUGIN_DEBUG -I$(top_srcdir) -I$(top_srcdir)/src/common

pkglib_LTLIBRARIES = select_cons_tres_locality.la

# Trackable RESources selection plugin.
select_cons_tres_locality_la_SOURCES =	dist_tasks.c dist_tasks.h	\
				gres_sched.c gres_sched.h	\
				job_test.c job_test.h		\
				alluxio.c alluxio.h         \
				select_cons_tres_locality.c select_cons_tres_locality.h
select_cons_tres_locality_la_LDFLAGS = $(PLUGIN_FLAGS)
select_cons_tres_locality_la_LIBADD = ../cons_common/libcons_common.la

force:
$(select_cons_tres_locality_la_LIBADD) : force
	@cd `dirname $@` && $(MAKE) `basename $@`
