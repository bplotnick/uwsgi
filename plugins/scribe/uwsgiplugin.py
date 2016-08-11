import os

NAME = 'scribe'

CFLAGS = os.popen('pkg-config --cflags gobject-2.0 glib-2.0 thrift_c_glib').read().rstrip().split()
LDFLAGS = []
LIBS = os.popen('pkg-config --libs gobject-2.0 glib-2.0 thrift_c_glib').read().rstrip().split()
GCC_LIST = ['scribe_plugin','gen-c_glib/facebook_service','gen-c_glib/fb303_types','gen-c_glib/scribe','gen-c_glib/scribe_types']
