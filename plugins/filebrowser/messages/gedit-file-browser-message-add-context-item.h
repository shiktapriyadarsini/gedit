#ifndef __GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM            (gedit_file_browser_message_add_context_item_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItem))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItem const))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItemClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItemClass))

typedef struct _GeditFileBrowserMessageAddContextItem        GeditFileBrowserMessageAddContextItem;
typedef struct _GeditFileBrowserMessageAddContextItemClass   GeditFileBrowserMessageAddContextItemClass;
typedef struct _GeditFileBrowserMessageAddContextItemPrivate GeditFileBrowserMessageAddContextItemPrivate;

struct _GeditFileBrowserMessageAddContextItem
{
	GeditMessage parent;

	GeditFileBrowserMessageAddContextItemPrivate *priv;
};

struct _GeditFileBrowserMessageAddContextItemClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_add_context_item_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_H__ */
