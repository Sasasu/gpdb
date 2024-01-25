PG_CONFIG = pg_config

MODULE_big = data_invert
OBJS = data_invert.o

REGRESS = data_invert

ifdef USE_PGXS
  PGXS := $(shell pg_config --pgxs)
  include $(PGXS)
else
  top_builddir = ../..
  include $(top_builddir)/src/Makefile.global
  include $(top_srcdir)/contrib/contrib-global.mk
endif
