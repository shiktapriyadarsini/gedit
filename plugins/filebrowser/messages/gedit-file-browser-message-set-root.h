#ifndef __GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT            (gedit_file_browser_message_set_root_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT,\
                                                             GeditFileBrowserMessageSetRoot))
#define GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT,\
                                                             GeditFileBrowserMessageSetRoot const))
#define GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT,\
                                                             GeditFileBrowserMessageSetRootClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_SET_ROOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_SET_ROOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT))
#define GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT,\
                                                             GeditFileBrowserMessageSetRootClass))

typedef struct _GeditFileBrowserMessageSetRoot        GeditFileBrowserMessageSetRoot;
typedef struct _GeditFileBrowserMessageSetRootClass   GeditFileBrowserMessageSetRootClass;
typedef struct _GeditFileBrowserMessageSetRootPrivate GeditFileBrowserMessageSetRootPrivate;

struct _GeditFileBrowserMessageSetRoot
{
	GeditMessage parent;

	GeditFileBrowserMessageSetRootPrivate *priv;
};

struct _GeditFileBrowserMessageSetRootClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_set_root_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT_H__ */
