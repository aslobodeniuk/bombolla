#include "robj-protocol.h"

#define ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS(T)                      \
  static void                                                           \
  robj_##T##_2_transport (const GValue * src_value, GValue * dest_value) \
  {                                                                     \
    guint tsize = sizeof (g##T);                                        \
                                                                        \
    g##T val = g_value_get_##T (src_value);                             \
    g_value_take_boxed (dest_value, g_bytes_new (&val, tsize));         \
  }                                                                     \
                                                                        \
  static void                                                           \
  robj_transport_2_##T (const GValue * src_value, GValue * dest_value)  \
  {                                                                     \
    gsize size = 0;                                                     \
    gconstpointer data = g_bytes_get_data                               \
        ((GBytes *)g_value_get_boxed (src_value), &size);               \
                                                                        \
    g_return_if_fail (size == sizeof (g##T));                           \
    g_value_set_##T (dest_value,                                        \
        *(g##T*)(data));                                                \
  }                                                                     \


ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (boolean);
ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (uint);
ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (int);
ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (float);
ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (double);

ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (int64);
ROBJ_TRANSPORT_DEFINE_SIMPLE_TRANSFORMS (uint64);

G_STATIC_ASSERT (sizeof (gboolean) == 4);
G_STATIC_ASSERT (sizeof (int) == 4);
G_STATIC_ASSERT (sizeof (float) == 4);
G_STATIC_ASSERT (sizeof (double) == 8);

static void
robj_string_2_transport (const GValue * src_value, GValue * dest_value) {
  guint msg_size;
  guint str_size = 0;
  gchar *ptr = NULL;
  const gchar *str = g_value_get_string (src_value);

  /* [string size] - 4 bytes, BE. Includes 0-terminator.
     If the size == 0, then the string is NULL. */
  /* [string value] - 'string size' bytes. */

  if (str != NULL)
    str_size = strlen (str) + 1;

  msg_size = 4 + str_size;
  ptr = g_malloc (msg_size);

  ((guint32 *) ptr)[0] = GUINT32_TO_BE (str_size);
  if (str_size != 0)
    memcpy ((char *)ptr + 4, str, str_size);

  /* So the message size is at least 4 bytes long */
  g_value_take_boxed (dest_value, g_bytes_new_take (ptr, msg_size));
}

static void
robj_transport_2_string (const GValue * src_value, GValue * dest_value) {
  /* [string size] - 4 bytes, BE. Includes 0-terminator.
     If the size == 0, then the string is NULL. */
  /* [string value] - 'string size' bytes. */

  gsize size = 0;
  gconstpointer data = g_bytes_get_data
      ((GBytes *) g_value_get_boxed (src_value), &size);

  g_return_if_fail (size >= 4);
  size -= 4;

  g_value_take_string (dest_value, (size == 0) ? NULL
                       : g_strndup ((char *)data + 4, size - 1));
}

typedef GBytes RObjTransport;

G_DEFINE_BOXED_TYPE (RObjTransport, robj_transport, g_bytes_ref, g_bytes_unref);

static void
robj_transport_register () {
  G_GNUC_UNUSED long foo;
  gint t;
  static struct {
    GType type;
    GValueTransform to_transport;
    GValueTransform from_transport;
  } s[] = {
    { G_TYPE_INT, robj_int_2_transport, robj_transport_2_int },
    { G_TYPE_UINT, robj_uint_2_transport, robj_transport_2_uint },
    { G_TYPE_INT64, robj_int64_2_transport, robj_transport_2_int64 },
    { G_TYPE_UINT64, robj_uint64_2_transport, robj_transport_2_uint64 },
    { G_TYPE_BOOLEAN, robj_boolean_2_transport, robj_transport_2_boolean },
    { G_TYPE_FLOAT, robj_float_2_transport, robj_transport_2_float },
    { G_TYPE_DOUBLE, robj_double_2_transport, robj_transport_2_double },
    { G_TYPE_STRING, robj_string_2_transport, robj_transport_2_string },
    { G_TYPE_ENUM, robj_int_2_transport, robj_transport_2_int },
#if (G_MAXULONG == 0xffffffff)
    { G_TYPE_LONG, robj_int_2_transport, robj_transport_2_int },
    { G_TYPE_ULONG, robj_uint_2_transport, robj_transport_2_uint },
#elif (G_MAXULONG == 0xffffffffffffffff)
    { G_TYPE_LONG, robj_int64_2_transport, robj_transport_2_int64 },
    { G_TYPE_ULONG, robj_uint64_2_transport, robj_transport_2_uint64 },
#endif
  };

  for (t = 0; t < G_N_ELEMENTS (s); t++) {
    g_value_register_transform_func (s[t].type, ROBJ_TRANSPORT_TYPE,
                                     s[t].to_transport);
    g_value_register_transform_func (ROBJ_TRANSPORT_TYPE, s[t].type,
                                     s[t].from_transport);
  }
}

void
robj_protocol_init (void) {
  static int once;

  if (G_UNLIKELY (once == 0)) {
    robj_transport_register ();
  }
}

/* The protocol message is:
   PN header.
   ----------------------------------
   [pn] - message magic of "property notify", 2 bytes
   [object name hash] - 4 bytes
   [property name hash] - 4 bytes
   ----------------------------------
   [val] - value of ROBJ transport, size is already known beforehand, from the
   negotiation.
*/
const guint ROBJ_PROTOCOL_PN_HEADER_LEN = 2 + 4 + 4;

RObjPN *
robj_protocol_message_to_pn (RObjMap * map, GBytes * msg) {
  gsize msg_size;
  gchar *msg_data;
  RObjPN *pn;
  GValue transport_value = G_VALUE_INIT;
  guint32 ohash;
  guint32 pnhash;

  msg_data = (gchar *) g_bytes_get_data (msg, &msg_size);

  g_return_val_if_fail (msg_data != NULL, NULL);
  g_return_val_if_fail (msg_size > ROBJ_PROTOCOL_PN_HEADER_LEN, NULL);
  g_return_val_if_fail (msg_data[0] == 'p', NULL);
  g_return_val_if_fail (msg_data[1] == 'n', NULL);

  msg_data += 2;

  ohash = ((guint32 *) msg_data)[0];
  pnhash = ((guint32 *) msg_data)[1];

  pn = robj_map_lookup_pn (map, ohash, pnhash);

  if (G_UNLIKELY (pn == NULL)) {
    g_critical ("PN (%u, %u) not found!", ohash, pnhash);
    return NULL;
  }

  /* So we already update the value in the property. There might be a race
   * condition when at this moment some other code reads or sets the property,
   * but hey, if it happens this RC is in fact the same as a race condition
   * across two codes setting/reading the property at the same time, like all
   * would happen at the same PC. Can this lead to different values on different
   * machines? Let's think.
   * One machines sets A and another machine sets B, right when the A arrives.
   * So the value will be A or B.
   * If other machine was setting B, but A won, then it will receive a notification
   * with A. First machine later will also receive notification with A, but it's not
   * a problem since A is already set. */
  g_value_init (&transport_value, ROBJ_TRANSPORT_TYPE);
  g_value_take_boxed (&transport_value,
                      /* We are doing a really funny thing here.
                       * We have the GBytes of the message (msg), and now we create another
                       * GBytes for the value (in the ROBJ protocol), that are based on the msg.
                       * In other words without copying the data we are just holding a pointer shifted
                       * by the header size, and a reference to the original bytes :)
                       * Take that, C++ (kekeke). */
                      g_bytes_new_from_bytes (msg,
                                              /* FIXME: make sure there's enough size to parse the value */
                                              ROBJ_PROTOCOL_PN_HEADER_LEN,
                                              msg_size -
                                              ROBJ_PROTOCOL_PN_HEADER_LEN));
  g_mutex_lock (&pn->lock);
  /* At this line the object's property value that it remembers is updated.
   * Caller is going to set this value to the ghost or real object.
   * Ghost object also remembers the value in the same map, so there's
   * no problem. Real object remembers it where it remembers, so it's
   * a bit redundant.... */
  if (!g_value_transform (&transport_value, &pn->pval)) {
    /* Should never happen since the types are already negotiated */
    g_critical ("Could not transform %s", pn->pname);
    /* Let the caller know something went wrong */
    pn = NULL;
  }

  g_value_unset (&transport_value);
  g_mutex_unlock (&pn->lock);
  return pn;
}

/* returns transfer-full, takes transfer-full */
static GBytes *
robj_wrap_tvalue_to_pn (GValue * tval, const RObjPN * pn) {
  GBytes *bval;
  gsize vsize;
  gconstpointer vdata;
  guint msg_size;
  gchar *ptr;

  bval = (GBytes *) g_value_get_boxed (tval);
  vdata = g_bytes_get_data (bval, &vsize);

  /* This is a good question */
  g_return_val_if_fail (NULL != vdata, NULL);

  msg_size = ROBJ_PROTOCOL_PN_HEADER_LEN + vsize;
  ptr = (gchar *) malloc (msg_size);
  /* Write the pn header. */
  /* Magic: 2 bytes */
  ptr[0] = 'p';
  ptr[1] = 'n';
  /* Object name hash: 4 bytes */
  *((guint32 *) (ptr + 2)) = pn->o_hash;
  /* Property name hash: 4 bytes */
  *((guint32 *) (ptr + 2 + 4)) = pn->pn_hash;

  /* We don't allow empty NULL bytes in the transport value.
   * Otherwise it complifies much the protocol for signal handling. */
  g_return_val_if_fail (vdata != NULL, NULL);
  g_return_val_if_fail (vsize != 0, NULL);

  memcpy (ptr + ROBJ_PROTOCOL_PN_HEADER_LEN, vdata, vsize);
  /* eat the input value, we already copied the data */
  g_value_unset (tval);
  return g_bytes_new_take (ptr, msg_size);
}

/* returns transfer-full */
GBytes *
robj_protocol_pn_to_message (RObjPN * pn) {
  gboolean could_transform;
  GValue val_tval = G_VALUE_INIT;

  /* This transport value holds the value in the ROBJ protocol */
  g_value_init (&val_tval, ROBJ_TRANSPORT_TYPE);

  /* Here the magic happens and the property value is tranformed into
   * the ROBJ transport that will travel through whatever.
   * This magic could be better if we knew the size of the message
   * before writing it to bytes. Then we could avoid one memcopy and
   * some allocs. But extending the API and complifying the code
   * only for that looks redundant. I hope nobody is going to send
   * raw images as is. */
  g_mutex_lock (&pn->lock);
  could_transform = g_value_transform (&pn->pval, &val_tval);
  g_mutex_unlock (&pn->lock);

  /* Should never happen since the types are already negotiated */
  g_return_val_if_fail (could_transform, NULL);
  return robj_wrap_tvalue_to_pn (&val_tval, pn);
}
