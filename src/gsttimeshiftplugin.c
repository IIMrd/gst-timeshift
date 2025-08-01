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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include "gsttimeshiftsink.h"
#include "gsttimeshiftsrc.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "timeshiftsink", GST_RANK_NONE,
          GST_TYPE_TIMESHIFT_SINK))
    return FALSE;
  if (!gst_element_register (plugin, "timeshiftsrc", GST_RANK_NONE,
          GST_TYPE_TIMESHIFT_SRC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, timeshift,
    "A plugin for timeshifting GStreamer pipelines", plugin_init, "1.0", "LGPL",
    "gst-timeshift", "https://github.com/acrilique/gst-timeshift")
