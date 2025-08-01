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

#ifndef __GST_TIMESHIFT_H__
#define __GST_TIMESHIFT_H__

#include <gst/gst.h>

#define RING_BUFFER_SIZE 3000   // 3000 buffers (GstBuffer)

/**
 * structures
*/

typedef struct _RingBuffer
{
  GstBuffer *buffers[RING_BUFFER_SIZE];
  guint head;
  guint tail;
  guint count;
  GMutex mutex;
  GCond cond;
} RingBuffer;

typedef struct _TimeShiftState
{
  RingBuffer ring_buffer;
  guint64 playback_position;
  guint64 total_buffers_written;
  GstClockTime duration;
} TimeShiftState;

/**
 * ring_buffer functions
 */

void ring_buffer_init (RingBuffer * ring_buffer);
void ring_buffer_destroy (RingBuffer * ring_buffer);
void ring_buffer_push (TimeShiftState * ts_state, GstBuffer * buffer);
GstBuffer *ring_buffer_read (TimeShiftState * ts_state,
    guint64 absolute_position);

#endif // __GST_TIMESHIFT_H__
