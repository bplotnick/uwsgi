#include "../../uwsgi.h"

#include <glib-object.h>

#include <thrift/c_glib/protocol/thrift_binary_protocol.h>
#include <thrift/c_glib/transport/thrift_framed_transport.h>
#include <thrift/c_glib/transport/thrift_socket.h>

#include "gen-c_glib/scribe.h"

extern struct uwsgi_server uwsgi;

struct uwsgi_scribe_state {
  ThriftSocket *socket;
  ThriftTransport *transport;
  ThriftProtocol *protocol;
  scribeIf *client;
  char *topic;
};

ssize_t uwsgi_scribe_logger(struct uwsgi_logger *ul, char *message, size_t len) {

  struct uwsgi_scribe_state *uscribelog = NULL;
  char *port;
  int portn;

  LogEntry *entry;
  ResultCode rc;
  GPtrArray *messages = g_ptr_array_new();
  GError *error = NULL;

  int exit_status = 0;

  if (!ul->data) {
    ul->data = uwsgi_calloc(sizeof(struct uwsgi_scribe_state));
  }

  uscribelog = (struct uwsgi_scribe_state *) ul->data;

  if (!ul->configured) {

    char *topic = strchr(ul->arg, ',');
    if (!topic) {
      uwsgi_log_safe("invalid scribe syntax\n");
      exit(1);
    }

    uscribelog->topic = topic + 1;
    *topic = 0;

    port = strchr(ul->arg, ':');
    if (!port) {
      uwsgi_log_safe("invalid scribe syntax\n");
      exit(1);
    }
    portn = atoi(port+1);

    *port = 0;

    uscribelog->socket = g_object_new (THRIFT_TYPE_SOCKET,
				       "hostname",  ul->arg,
				       "port",      portn,
				       NULL);

    *port = ':';

    uscribelog->transport = g_object_new (THRIFT_TYPE_FRAMED_TRANSPORT,
			      "transport", uscribelog->socket,
			      NULL);
    uscribelog->protocol = g_object_new (THRIFT_TYPE_BINARY_PROTOCOL,
			      "transport", uscribelog->transport,
			      NULL);
    thrift_transport_open (uscribelog->transport, &error);
    if (error) {
      uwsgi_log_safe("ERROR: ");
      uwsgi_log_safe(error->message);
      uwsgi_log_safe("\n");
      exit(1);
    }


    uscribelog->client = g_object_new (TYPE_SCRIBE_CLIENT,
			   "input_protocol",  uscribelog->protocol,
			   "output_protocol", uscribelog->protocol,
			   NULL);
    ul->configured = 1;
  }

  entry = g_object_new(TYPE_LOG_ENTRY, NULL);

  g_object_set(entry,
	       "category", uscribelog->topic,
	       "message", message,
	       NULL);

  g_ptr_array_add(messages, entry);

  scribe_if_log(uscribelog->client, &rc, messages, &error);
  if (error) {
    uwsgi_log_safe(error->message);
    exit(1);
  }

  g_ptr_array_remove(messages, entry);
  g_object_unref(entry);
  g_ptr_array_unref(messages);

  /*
  thrift_transport_close (uscribelog->transport, NULL);

  g_object_unref (uscribelog->client);
  g_object_unref (uscribelog->protocol);
  g_object_unref (uscribelog->transport);
  g_object_unref (uscribelog->socket);
  */
  return exit_status;
}

void uwsgi_scribe_register() {
  uwsgi_register_logger("scribe", uwsgi_scribe_logger);
}

struct uwsgi_plugin scribe_plugin = {
  .name = "scribe",
  .on_load = uwsgi_scribe_register,
};
