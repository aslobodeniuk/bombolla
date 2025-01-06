#ifndef _ROBJ_PROTOCOL_H
#  define _ROBJ_PROTOCOL_H

#  include "robj-brain.h"

RObjPN *robj_protocol_message_to_pn (GBytes * msg);
GBytes *robj_protocol_pn_to_message (RObjPN * pn);

//GBytes *robj_protocol_get_negotiate (GObject *obj);

void robj_protocol_init (void);

/* Can be used to register transform functions for custom types */
#  define ROBJ_TRANSPORT_TYPE (robj_transport_get_type ())
GType robj_transport_get_type (void);

#endif
