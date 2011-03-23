#include "gedit-file-browser-message-id.h"

#define GEDIT_FILE_BROWSER_MESSAGE_ID_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID, GeditFileBrowserMessageIdPrivate))

enum
{
	PROP_0,

	PROP_ID,
};

struct _GeditFileBrowserMessageIdPrivate
{
	guint id;
};

G_DEFINE_TYPE (GeditFileBrowserMessageId, gedit_file_browser_message_id, GEDIT_TYPE_MESSAGE)

static void
gedit_file_browser_message_id_get_property (GObject    *obj,
                                            guint       prop_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
	GeditFileBrowserMessageId *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ID (obj);

	switch (prop_id)
	{
		case PROP_ID:
			g_value_set_uint (value, msg->priv->id);
			break;
	}
}

static void
gedit_file_browser_message_id_set_property (GObject      *obj,
                                            guint         prop_id,
                                            GValue const *value,
                                            GParamSpec   *pspec)
{
	GeditFileBrowserMessageId *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ID (obj);

	switch (prop_id)
	{
		case PROP_ID:
			msg->priv->id = g_value_get_uint (value);
			break;
	}
}

static void
gedit_file_browser_message_id_class_init (GeditFileBrowserMessageIdClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->get_property = gedit_file_browser_message_id_get_property;
	object_class->set_property = gedit_file_browser_message_id_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_ID,
	                                 g_param_spec_uint ("id",
	                                                    "Id",
	                                                    "Id",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    0,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditFileBrowserMessageIdPrivate));
}

static void
gedit_file_browser_message_id_init (GeditFileBrowserMessageId *message)
{
	message->priv = GEDIT_FILE_BROWSER_MESSAGE_ID_GET_PRIVATE (message);
}
