#ifndef __GST_TIMESHIFT_SRC_H__
#define __GST_TIMESHIFT_SRC_H__

#include "gsttimeshiftsink.h"
#include <gst/base/gstbasesrc.h>
#include "gsttimeshiftsink.h"

G_BEGIN_DECLS
#define GST_TYPE_TIMESHIFT_SRC (gst_timeshift_src_get_type())
G_DECLARE_FINAL_TYPE (GstTimeShiftSrc, gst_timeshift_src, GST, TIMESHIFT_SRC,
    GstBaseSrc)
     struct _GstTimeShiftSrc
     {
       GstBaseSrc parent;

       GstTimeShiftSink *sink;

       gboolean flushing;
       gboolean discont;
       GstSegment segment;
     };

     GType gst_timeshift_src_get_type (void);

G_END_DECLS
#endif /* __GST_TIMESHIFT_SRC_H__ */
