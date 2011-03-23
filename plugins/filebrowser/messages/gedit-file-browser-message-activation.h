#ifndef __GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION            (gedit_file_browser_message_activation_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION,\
                                                               GeditFileBrowserMessageActivation))
#define GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION,\
                                                               GeditFileBrowserMessageActivation const))
#define GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION,\
                                                               GeditFileBrowserMessageActivationClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ACTIVATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ACTIVATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION))
#define GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                               GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION,\
                                                               GeditFileBrowserMessageActivationClass))

typedef struct _GeditFileBrowserMessageActivation        GeditFileBrowserMessageActivation;
typedef struct _GeditFileBrowserMessageActivationClass   GeditFileBrowserMessageActivationClass;
typedef struct _GeditFileBrowserMessageActivationPrivate GeditFileBrowserMessageActivationPrivate;

struct _GeditFileBrowserMessageActivation
{
	GeditMessage parent;

	GeditFileBrowserMessageActivationPrivate *priv;
};

struct _GeditFileBrowserMessageActivationClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_activation_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION_H__ */
