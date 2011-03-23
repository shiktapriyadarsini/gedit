#ifndef __GEDIT_FILE_BROWSER_MESSAGE_ID_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_ID_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID            (gedit_file_browser_message_id_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_ID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                       GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID,\
                                                       GeditFileBrowserMessageId))
#define GEDIT_FILE_BROWSER_MESSAGE_ID_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                       GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID,\
                                                       GeditFileBrowserMessageId const))
#define GEDIT_FILE_BROWSER_MESSAGE_ID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                       GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID,\
                                                       GeditFileBrowserMessageIdClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                       GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                       GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID))
#define GEDIT_FILE_BROWSER_MESSAGE_ID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                       GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID,\
                                                       GeditFileBrowserMessageIdClass))

typedef struct _GeditFileBrowserMessageId        GeditFileBrowserMessageId;
typedef struct _GeditFileBrowserMessageIdClass   GeditFileBrowserMessageIdClass;
typedef struct _GeditFileBrowserMessageIdPrivate GeditFileBrowserMessageIdPrivate;

struct _GeditFileBrowserMessageId
{
	GeditMessage parent;

	GeditFileBrowserMessageIdPrivate *priv;
};

struct _GeditFileBrowserMessageIdClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_id_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_ID_H__ */
