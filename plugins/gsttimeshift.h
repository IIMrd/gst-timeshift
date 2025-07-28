#ifndef __GST_TIMESHIFT_H__
#define __GST_TIMESHIFT_H__

#include <gst/gst.h>

#define RING_BUFFER_SIZE 2000   // 2000 buffers (GstBuffer)

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
GstBuffer *ring_buffer_read (TimeShiftState * ts_state, guint64 absolute_position);

#endif // __GST_TIMESHIFT_H__
