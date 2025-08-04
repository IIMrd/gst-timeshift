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

#include "gsttimeshiftsink.h"

GST_DEBUG_CATEGORY_STATIC (gst_timeshift_sink_debug);
#define GST_CAT_DEFAULT gst_timeshift_sink_debug

#define DEFAULT_BUFFER_SIZE 2000

enum
{
  PROP_0,
  PROP_BUFFER_SIZE,
  PROP_LAST
};

#define gst_timeshift_sink_parent_class parent_class
G_DEFINE_TYPE (GstTimeShiftSink, gst_timeshift_sink, GST_TYPE_BASE_SINK);

static GstFlowReturn gst_timeshift_sink_render (GstBaseSink * sink,
    GstBuffer * buffer);
static void gst_timeshift_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_timeshift_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_timeshift_sink_start (GstBaseSink * sink);
static gboolean gst_timeshift_sink_stop (GstBaseSink * sink);

static void
gst_timeshift_sink_init (GstTimeShiftSink * self)
{
  self->state = g_new0 (TimeShiftState, 1);
  self->buffer_size = DEFAULT_BUFFER_SIZE;
}

static void
gst_timeshift_sink_dispose (GObject * object)
{
  GstTimeShiftSink *self = GST_TIMESHIFT_SINK (object);

  if (self->state) {
    g_free (self->state);
    self->state = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_timeshift_sink_class_init (GstTimeShiftSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *basesink_class = GST_BASE_SINK_CLASS (klass);

  gobject_class->dispose = gst_timeshift_sink_dispose;
  gobject_class->set_property = gst_timeshift_sink_set_property;
  gobject_class->get_property = gst_timeshift_sink_get_property;

  g_object_class_install_property (gobject_class, PROP_BUFFER_SIZE,
      g_param_spec_uint ("buffer-size", "Buffer Size",
          "Size of the ring buffer in buffers", 1, G_MAXUINT,
          DEFAULT_BUFFER_SIZE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  basesink_class->start = GST_DEBUG_FUNCPTR (gst_timeshift_sink_start);
  basesink_class->stop = GST_DEBUG_FUNCPTR (gst_timeshift_sink_stop);
  basesink_class->render = GST_DEBUG_FUNCPTR (gst_timeshift_sink_render);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "TimeShift Sink", "Sink/TimeShift",
      "Receives data and stores it in a ring buffer for timeshifting",
      "Lluc Simó Margalef <lsimmar@upv.es>");

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_new_any ()));

  GST_DEBUG_CATEGORY_INIT (gst_timeshift_sink_debug, "timeshiftsink", 0,
      "timeshift sink element");
}

static gboolean
gst_timeshift_sink_start (GstBaseSink * sink)
{
  GstTimeShiftSink *self = GST_TIMESHIFT_SINK (sink);

  ring_buffer_init (&self->state->ring_buffer, self->buffer_size);

  return TRUE;
}

static gboolean
gst_timeshift_sink_stop (GstBaseSink * sink)
{
  GstTimeShiftSink *self = GST_TIMESHIFT_SINK (sink);

  ring_buffer_destroy (&self->state->ring_buffer);

  return TRUE;
}

static void
gst_timeshift_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTimeShiftSink *self = GST_TIMESHIFT_SINK (object);

  switch (prop_id) {
    case PROP_BUFFER_SIZE:
      self->buffer_size = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_timeshift_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTimeShiftSink *self = GST_TIMESHIFT_SINK (object);

  switch (prop_id) {
    case PROP_BUFFER_SIZE:
      g_value_set_uint (value, self->buffer_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_timeshift_sink_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstTimeShiftSink *self = GST_TIMESHIFT_SINK (sink);

  ring_buffer_push (self->state, buffer);

  return GST_FLOW_OK;
}
