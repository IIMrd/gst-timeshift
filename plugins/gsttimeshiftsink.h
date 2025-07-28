#ifndef __GST_TIMESHIFT_SINK_H__
#define __GST_TIMESHIFT_SINK_H__

#include "gsttimeshift.h"
#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS
#define GST_TYPE_TIMESHIFT_SINK (gst_timeshift_sink_get_type())
G_DECLARE_FINAL_TYPE (GstTimeShiftSink, gst_timeshift_sink, GST, TIMESHIFT_SINK,
    GstBaseSink)
     struct _GstTimeShiftSink
     {
       GstBaseSink parent;

       TimeShiftState *state;
     };

     GType gst_timeshift_sink_get_type (void);

G_END_DECLS
#endif /* __GST_TIMESHIFT_SINK_H__ */
