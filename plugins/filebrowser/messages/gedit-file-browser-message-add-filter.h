#ifndef __GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER            (gedit_file_browser_message_add_filter_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER,\
                                                               GeditFileBrowserMessageAddFilter))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER,\
                                                               GeditFileBrowserMessageAddFilter const))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER,\
                                                               GeditFileBrowserMessageAddFilterClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ADD_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ADD_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER,\
                                                               GeditFileBrowserMessageAddFilterClass))

typedef struct _GeditFileBrowserMessageAddFilter        GeditFileBrowserMessageAddFilter;
typedef struct _GeditFileBrowserMessageAddFilterClass   GeditFileBrowserMessageAddFilterClass;
typedef struct _GeditFileBrowserMessageAddFilterPrivate GeditFileBrowserMessageAddFilterPrivate;

struct _GeditFileBrowserMessageAddFilter
{
	GeditMessage parent;

	GeditFileBrowserMessageAddFilterPrivate *priv;
};

struct _GeditFileBrowserMessageAddFilterClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_add_filter_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER_H__ */
