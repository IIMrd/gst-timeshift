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

#include "gsttimeshift.h"

void
ring_buffer_init (RingBuffer * ring_buffer, guint size)
{
  ring_buffer->head = 0;
  ring_buffer->tail = 0;
  ring_buffer->count = 0;
  ring_buffer->size = size;
  ring_buffer->buffers = g_new0 (GstBuffer *, size);
  g_mutex_init (&ring_buffer->mutex);
  g_cond_init (&ring_buffer->cond);
}

void
ring_buffer_destroy (RingBuffer * ring_buffer)
{
  g_mutex_lock (&ring_buffer->mutex);
  for (guint i = 0; i < ring_buffer->size; i++) {
    if (ring_buffer->buffers[i]) {
      gst_buffer_unref (ring_buffer->buffers[i]);
      ring_buffer->buffers[i] = NULL;
    }
  }
  g_free (ring_buffer->buffers);
  g_mutex_unlock (&ring_buffer->mutex);
  g_mutex_clear (&ring_buffer->mutex);
  g_cond_clear (&ring_buffer->cond);
}

void
ring_buffer_push (TimeShiftState * ts_state, GstBuffer * buffer)
{
  g_mutex_lock (&ts_state->ring_buffer.mutex);

  if (ts_state->ring_buffer.buffers[ts_state->ring_buffer.head]) {
    gst_buffer_unref (ts_state->ring_buffer.buffers[ts_state->
            ring_buffer.head]);
  }

  ts_state->ring_buffer.buffers[ts_state->ring_buffer.head] =
      gst_buffer_ref (buffer);
  ts_state->ring_buffer.head =
      (ts_state->ring_buffer.head + 1) % ts_state->ring_buffer.size;

  if (ts_state->ring_buffer.count < ts_state->ring_buffer.size) {
    ts_state->ring_buffer.count++;
  } else {
    ts_state->ring_buffer.tail =
        (ts_state->ring_buffer.tail + 1) % ts_state->ring_buffer.size;
  }
  ts_state->total_buffers_written++;

  if (ts_state->ring_buffer.count > 1) {
    GstBuffer *head_buf =
        ts_state->ring_buffer.buffers[(ts_state->ring_buffer.head +
            ts_state->ring_buffer.size - 1) % ts_state->ring_buffer.size];
    GstBuffer *tail_buf =
        ts_state->ring_buffer.buffers[ts_state->ring_buffer.tail];
    if (GST_BUFFER_PTS_IS_VALID (head_buf) &&
        GST_BUFFER_PTS_IS_VALID (tail_buf)) {
      ts_state->duration =
          GST_BUFFER_PTS (head_buf) - GST_BUFFER_PTS (tail_buf);
    }
  }

  g_cond_signal (&ts_state->ring_buffer.cond);
  g_mutex_unlock (&ts_state->ring_buffer.mutex);
}

GstBuffer *
ring_buffer_read (TimeShiftState * ts_state, guint64 absolute_position)
{
  g_mutex_lock (&ts_state->ring_buffer.mutex);

  guint64 first_available_pos =
      ts_state->total_buffers_written - ts_state->ring_buffer.count;
  if (absolute_position < first_available_pos ||
      absolute_position >= ts_state->total_buffers_written) {
    g_mutex_unlock (&ts_state->ring_buffer.mutex);
    return NULL;
  }

  guint relative_index = absolute_position - first_available_pos;
  guint actual_index =
      (ts_state->ring_buffer.tail +
      relative_index) % ts_state->ring_buffer.size;
  GstBuffer *buffer =
      gst_buffer_ref (ts_state->ring_buffer.buffers[actual_index]);

  g_mutex_unlock (&ts_state->ring_buffer.mutex);
  return buffer;
}
