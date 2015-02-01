AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = \
  freakz \
  os

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/freakz @CONTIKI_CFLAGS@

# XXX: if APPLE
AM_LDFLAGS = -Wl,-undefined,error -Wl,-all_load

lib_LTLIBRARIES = libfreakz.la
bin_PROGRAMS = test_udp test_sim

libfreakz_la_SOURCES =

test_udp_SOURCES = \
  test/native/test_udp.c \
  freakz/app/test_app.c \
  freakz/app/test_data.c \
  freakz/app/test_zcl.c \
  freakz/app/test_zdo.c \
  freakz/af/af.c \
  freakz/af/af_ep.c \
  freakz/af/af_conf.c \
  freakz/af/af_conf_tbl.c \
  freakz/af/af_rx.c \
  freakz/af/af_tx.c \
  freakz/aps/aps.c \
  freakz/aps/aps_gen.c \
  freakz/aps/aps_parse.c \
  freakz/aps/aps_retry.c \
  freakz/aps/aps_dupe.c \
  freakz/aps/aps_bind.c \
  freakz/aps/aps_grp.c \
  freakz/driver/sim/mac_hw.c \
  freakz/driver/sim/sim_drvr.c \
  freakz/freakz.c \
  freakz/mac/mac.c \
  freakz/mac/mac_gen.c \
  freakz/mac/mac_parse.c \
  freakz/mac/mac_indir.c \
  freakz/mac/mac_queue.c \
  freakz/mac/mac_start.c \
  freakz/mac/mac_reset.c \
  freakz/mac/mac_scan.c \
  freakz/mac/mac_assoc.c \
  freakz/mac/mac_poll.c \
  freakz/mac/mac_retry.c \
  freakz/misc/buf.c \
  freakz/misc/dev_dbg.c \
  freakz/misc/misc.c \
  freakz/misc/slow_clock.c \
  freakz/misc/mem_heap.c \
  freakz/nwk/nwk.c \
  freakz/nwk/nwk_gen.c \
  freakz/nwk/nwk_parse.c \
  freakz/nwk/nwk_reset.c \
  freakz/nwk/nwk_sync.c \
  freakz/nwk/nwk_rte_mesh.c \
  freakz/nwk/nwk_rte_disc_tbl.c \
  freakz/nwk/nwk_rte_tbl.c \
  freakz/nwk/nwk_pend.c \
  freakz/nwk/nwk_form.c \
  freakz/nwk/nwk_permit_join.c \
  freakz/nwk/nwk_neighbor_tbl.c \
  freakz/nwk/nwk_rte_tree.c \
  freakz/nwk/nwk_brc.c \
  freakz/nwk/nwk_disc.c \
  freakz/nwk/nwk_join.c \
  freakz/nwk/nwk_leave.c \
  freakz/nwk/nwk_addr_map.c \
  freakz/zcl/zcl.c \
  freakz/zcl/zcl_parse.c \
  freakz/zcl/zcl_gen.c \
  freakz/zcl/zcl_rpt.c \
  freakz/zcl/general/zcl_basic.c \
  freakz/zcl/general/zcl_on_off.c \
  freakz/zcl/general/zcl_id.c \
  freakz/zcl/general/zcl_level.c \
  freakz/zdo/zdo.c \
  freakz/zdo/zdo_nwk_mgr.c \
  freakz/zdo/zdo_disc.c \
  freakz/zdo/zdo_cfg.c \
  freakz/zdo/zdo_gen.c \
  freakz/zdo/zdo_parse.c \
  freakz/zdo/zdo_bind_mgr.c

test_sim_SOURCES = \
  test/native/test_sim.c \
  freakz/app/test_app.c \
  freakz/app/test_data.c \
  freakz/app/test_zcl.c \
  freakz/app/test_zdo.c

#test_udp_LDADD = \
#  libfreakz.la

test_udp_LDADD = os/libfreakzos.a

test_sim_LDADD = \
  libfreakz.la

libfreakz_la_LIBADD = \
  freakz/libfreakzfreakz.a \
  os/libfreakzos.a

dist_doc_DATA = AUTHORS ChangeLog COPYING NEWS README
