#ifndef __GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW            (gedit_file_browser_message_get_view_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW,\
                                                             GeditFileBrowserMessageGetView))
#define GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW,\
                                                             GeditFileBrowserMessageGetView const))
#define GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW,\
                                                             GeditFileBrowserMessageGetViewClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_GET_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_GET_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW))
#define GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                             GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW,\
                                                             GeditFileBrowserMessageGetViewClass))

typedef struct _GeditFileBrowserMessageGetView        GeditFileBrowserMessageGetView;
typedef struct _GeditFileBrowserMessageGetViewClass   GeditFileBrowserMessageGetViewClass;
typedef struct _GeditFileBrowserMessageGetViewPrivate GeditFileBrowserMessageGetViewPrivate;

struct _GeditFileBrowserMessageGetView
{
	GeditMessage parent;

	GeditFileBrowserMessageGetViewPrivate *priv;
};

struct _GeditFileBrowserMessageGetViewClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_get_view_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_H__ */
