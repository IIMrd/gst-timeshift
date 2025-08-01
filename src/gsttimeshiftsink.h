/*
 *  gst-timeshift - A GStreamer plugin for timeshifting media streams
 *  Copyright (C) 2025 Lluc Simó Margalef <lsimmar@upv.es>, Immersive
 *    Interactive Media (IIM) R&D group at Universitat Politècnica de València.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
       guint buffer_size;
     };

     GType gst_timeshift_sink_get_type (void);

G_END_DECLS
#endif /* __GST_TIMESHIFT_SINK_H__ */
