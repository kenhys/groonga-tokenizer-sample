AM_CFLAGS =					\
	$(GROONGA_CFLAGS)			\
	-I/usr/include/glib-2.0			\
	-I/usr/lib/x86_64-linux-gnu/glib-2.0/include


AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(GROONGA_LIBS)				\
	-lglib-2.0 -lwordnet

tokenizers_plugins_LTLIBRARIES =
tokenizers_plugins_LTLIBRARIES += sample.la wordnet.la

tokenizers_plugins_SOURCE =			\
	sample.c				\
	wordnet.c
