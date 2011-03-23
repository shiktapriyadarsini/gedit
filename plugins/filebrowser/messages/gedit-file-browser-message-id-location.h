#ifndef __GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION            (gedit_file_browser_message_id_location_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                                GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,\
                                                                GeditFileBrowserMessageIdLocation))
#define GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                                GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,\
                                                                GeditFileBrowserMessageIdLocation const))
#define GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                                GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,\
                                                                GeditFileBrowserMessageIdLocationClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ID_LOCATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                                GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ID_LOCATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                                GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION))
#define GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                                GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,\
                                                                GeditFileBrowserMessageIdLocationClass))

typedef struct _GeditFileBrowserMessageIdLocation        GeditFileBrowserMessageIdLocation;
typedef struct _GeditFileBrowserMessageIdLocationClass   GeditFileBrowserMessageIdLocationClass;
typedef struct _GeditFileBrowserMessageIdLocationPrivate GeditFileBrowserMessageIdLocationPrivate;

struct _GeditFileBrowserMessageIdLocation
{
	GeditMessage parent;

	GeditFileBrowserMessageIdLocationPrivate *priv;
};

struct _GeditFileBrowserMessageIdLocationClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_id_location_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION_H__ */
