/*
 *  gst-timeshift - A GStreamer plugin for timeshifting media streams
 *  Copyright (C) 2025 Lluc Simó Margalef <lsimmar@upv.es>, Immersive
 *    Interactive Media (IIM) R&D group at Universitat Politècnica de València.
 *
 *  This library has been developed with support by the following projects:
 *  CIAICO/2022/025, from Conselleria de Innovación, Universidades, Ciencia y
 *  Sociedad Digital of the GVA (DOGV 8919/05.10.2020); and grant
 *  PID2021-126645OB-I00, funded by MICIU/AEI/10.13039/501100011033/ and by "ERDF
 *  A way of making Europe".
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
