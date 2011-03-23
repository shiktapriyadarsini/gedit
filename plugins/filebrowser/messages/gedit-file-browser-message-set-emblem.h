#ifndef __GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM            (gedit_file_browser_message_set_emblem_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM,\
                                                               GeditFileBrowserMessageSetEmblem))
#define GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM,\
                                                               GeditFileBrowserMessageSetEmblem const))
#define GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM,\
                                                               GeditFileBrowserMessageSetEmblemClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_SET_EMBLEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_SET_EMBLEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM))
#define GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM,\
                                                               GeditFileBrowserMessageSetEmblemClass))

typedef struct _GeditFileBrowserMessageSetEmblem        GeditFileBrowserMessageSetEmblem;
typedef struct _GeditFileBrowserMessageSetEmblemClass   GeditFileBrowserMessageSetEmblemClass;
typedef struct _GeditFileBrowserMessageSetEmblemPrivate GeditFileBrowserMessageSetEmblemPrivate;

struct _GeditFileBrowserMessageSetEmblem
{
	GeditMessage parent;

	GeditFileBrowserMessageSetEmblemPrivate *priv;
};

struct _GeditFileBrowserMessageSetEmblemClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_set_emblem_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_H__ */
