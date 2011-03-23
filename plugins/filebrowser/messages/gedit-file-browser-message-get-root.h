#ifndef __GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT            (gedit_file_browser_message_get_root_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT,\
                                                             GeditFileBrowserMessageGetRoot))
#define GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT,\
                                                             GeditFileBrowserMessageGetRoot const))
#define GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT,\
                                                             GeditFileBrowserMessageGetRootClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_GET_ROOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_GET_ROOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT))
#define GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT,\
                                                             GeditFileBrowserMessageGetRootClass))

typedef struct _GeditFileBrowserMessageGetRoot        GeditFileBrowserMessageGetRoot;
typedef struct _GeditFileBrowserMessageGetRootClass   GeditFileBrowserMessageGetRootClass;
typedef struct _GeditFileBrowserMessageGetRootPrivate GeditFileBrowserMessageGetRootPrivate;

struct _GeditFileBrowserMessageGetRoot
{
	GeditMessage parent;

	GeditFileBrowserMessageGetRootPrivate *priv;
};

struct _GeditFileBrowserMessageGetRootClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_get_root_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT_H__ */
