#include "gsttimeshiftsrc.h"
#include "gsttimeshift.h"

GST_DEBUG_CATEGORY_STATIC (gst_timeshift_src_debug);
#define GST_CAT_DEFAULT gst_timeshift_src_debug

enum
{
  PROP_0,
  PROP_SINK,
  PROP_LAST
};

#define gst_timeshift_src_parent_class parent_class
G_DEFINE_TYPE (GstTimeShiftSrc, gst_timeshift_src, GST_TYPE_BASE_SRC);

static GstFlowReturn gst_timeshift_src_create (GstBaseSrc * src, guint64 offset,
    guint size, GstBuffer ** buf);
static gboolean gst_timeshift_src_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_timeshift_src_is_seekable (GstBaseSrc * src);
static gboolean gst_timeshift_src_seek (GstBaseSrc * src, GstSegment * segment);
static gboolean gst_timeshift_src_event (GstBaseSrc * src, GstEvent * event);
static gboolean gst_timeshift_src_unlock (GstBaseSrc * src);
static gboolean gst_timeshift_src_unlock_stop (GstBaseSrc * src);
static void gst_timeshift_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_timeshift_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
gst_timeshift_src_init (GstTimeShiftSrc * self)
{
  gst_base_src_set_format (GST_BASE_SRC (self), GST_FORMAT_TIME);
  self->flushing = FALSE;
  self->discont = FALSE;
  gst_segment_init (&self->segment, GST_FORMAT_TIME);
  if (self->sink)
    self->sink->state->duration = GST_CLOCK_TIME_NONE;
}

static void
gst_timeshift_src_dispose (GObject * object)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (object);

  g_clear_object (&self->sink);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_timeshift_src_class_init (GstTimeShiftSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *basesrc_class = GST_BASE_SRC_CLASS (klass);

  gobject_class->set_property = gst_timeshift_src_set_property;
  gobject_class->get_property = gst_timeshift_src_get_property;
  gobject_class->dispose = gst_timeshift_src_dispose;

  g_object_class_install_property (gobject_class, PROP_SINK,
      g_param_spec_object ("sink", "Sink",
          "The GstTimeShiftSink element to link to",
          GST_TYPE_TIMESHIFT_SINK, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  basesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_timeshift_src_seek);
  basesrc_class->event = GST_DEBUG_FUNCPTR (gst_timeshift_src_event);
  basesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_timeshift_src_unlock);
  basesrc_class->unlock_stop =
      GST_DEBUG_FUNCPTR (gst_timeshift_src_unlock_stop);
  basesrc_class->query = GST_DEBUG_FUNCPTR (gst_timeshift_src_query);
  basesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gst_timeshift_src_is_seekable);
  basesrc_class->create = GST_DEBUG_FUNCPTR (gst_timeshift_src_create);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "TimeShift Source", "Source/TimeShift",
      "Provides a seekable timeshifted stream from the timestamped buffers given to GstTimeShiftSink",
      "Lluc Simó Margalef <lsimmar@upv.es>");

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_new_any ()));

  GST_DEBUG_CATEGORY_INIT (gst_timeshift_src_debug, "timeshiftsrc", 0,
      "timeshift src element");
}

static void
gst_timeshift_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (object);

  switch (prop_id) {
    case PROP_SINK:
      self->sink = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_timeshift_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (object);

  switch (prop_id) {
    case PROP_SINK:
      g_value_set_object (value, self->sink);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_timeshift_src_create (GstBaseSrc * src, guint64 offset, guint size,
    GstBuffer ** buf)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (src);
  TimeShiftState *ts_state = self->sink->state;

  if (self->flushing) {
    return GST_FLOW_FLUSHING;
  }

  g_mutex_lock (&ts_state->ring_buffer.mutex);
  while (ts_state->playback_position >= ts_state->total_buffers_written) {
    g_cond_wait (&ts_state->ring_buffer.cond, &ts_state->ring_buffer.mutex);
  }
  g_mutex_unlock (&ts_state->ring_buffer.mutex);

  *buf = ring_buffer_read (ts_state, ts_state->playback_position);

  if (*buf) {
    if (self->discont) {
      GST_BUFFER_FLAG_SET (*buf, GST_BUFFER_FLAG_DISCONT);
      self->discont = FALSE;
    }
    ts_state->playback_position++;
    return GST_FLOW_OK;
  }

  return GST_FLOW_OK;
}

static gboolean
gst_timeshift_src_is_seekable (GstBaseSrc * src)
{
  return TRUE;
}

static gboolean
gst_timeshift_src_unlock (GstBaseSrc * src)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (src);

  GST_DEBUG_OBJECT (self, "unlock");
  self->flushing = TRUE;

  return TRUE;
}

static gboolean
gst_timeshift_src_unlock_stop (GstBaseSrc * src)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (src);

  GST_DEBUG_OBJECT (self, "unlock_stop");
  self->flushing = FALSE;

  return TRUE;
}

gboolean
seek_data (GstTimeShiftSrc *self, guint64 offset)
{
  TimeShiftState *ts_state = self->sink->state;
  g_mutex_lock (&ts_state->ring_buffer.mutex);

  if (ts_state->ring_buffer.count == 0) {
    if (offset == 0) {
      // This is the initial seek, we can allow it.
      ts_state->playback_position = 0;
      g_mutex_unlock (&ts_state->ring_buffer.mutex);
      return TRUE;
    }
    g_mutex_unlock (&ts_state->ring_buffer.mutex);
    GST_WARNING_OBJECT (self, "Ring buffer is empty, cannot seek.");
    return FALSE;
  }

  guint64 first_available_pos =
      ts_state->total_buffers_written - ts_state->ring_buffer.count;
  GstBuffer *tail_buf =
      ts_state->ring_buffer.buffers[ts_state->ring_buffer.tail];
  GstBuffer *head_buf =
      ts_state->ring_buffer.buffers[(ts_state->ring_buffer.head +
          RING_BUFFER_SIZE - 1) % RING_BUFFER_SIZE];

  if (!tail_buf || !GST_BUFFER_PTS_IS_VALID (tail_buf) || !head_buf
      || !GST_BUFFER_PTS_IS_VALID (head_buf)) {
    GST_WARNING_OBJECT (self,
        "Cannot seek, buffer boundaries have invalid PTS.");
    g_mutex_unlock (&ts_state->ring_buffer.mutex);
    return FALSE;
  }

  guint64 first_pts = GST_BUFFER_PTS (tail_buf);
  guint64 last_pts = GST_BUFFER_PTS (head_buf);

  GST_DEBUG_OBJECT (self,
      "Seeking to %" G_GUINT64_FORMAT ", available range: %" G_GUINT64_FORMAT
      " - %" G_GUINT64_FORMAT, offset, first_pts, last_pts);

  if (offset < first_pts || offset > last_pts) {
    GST_WARNING_OBJECT (self,
        "Seek out of range. Requested %" G_GUINT64_FORMAT ", available %"
        G_GUINT64_FORMAT " - %" G_GUINT64_FORMAT, offset, first_pts, last_pts);
    g_mutex_unlock (&ts_state->ring_buffer.mutex);
    return FALSE;
  }

  gint found = 0;
  // Search backwards from the desired position to find the last 3 keyframes,
  // this should give the video decoder enough context, avoiding corruption
  for (gint i = ts_state->ring_buffer.count - 1; i >= 0; i--) {
    guint actual_index = (ts_state->ring_buffer.tail + i) % RING_BUFFER_SIZE;
    GstBuffer *buffer = ts_state->ring_buffer.buffers[actual_index];
    if (buffer && GST_BUFFER_PTS_IS_VALID (buffer) &&
        GST_BUFFER_PTS (buffer) <= offset &&
        !GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
      ts_state->playback_position = first_available_pos + i;
      GST_DEBUG_OBJECT (self, "Found keyframe with PTS %" G_GUINT64_FORMAT
          " at absolute position %" G_GUINT64_FORMAT " for seek",
          GST_BUFFER_PTS (buffer), ts_state->playback_position);
      if (++found >= 3)
        break;
    }
  }
  g_mutex_unlock (&ts_state->ring_buffer.mutex);

  if (found == 0) {
    GST_WARNING_OBJECT (self,
        "Could not find a suitable buffer to seek to.");
  }

  return found > 0;
}

static gboolean
gst_timeshift_src_seek (GstBaseSrc * src, GstSegment * segment)
{
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (src);
  gboolean ret = FALSE;

  GST_DEBUG_OBJECT (self, "seeking to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (segment->start));

  ret = seek_data (self, segment->start);
  if (ret) {
    self->discont = TRUE;
    gst_segment_copy_into (segment, &self->segment);
  }

  return ret;
}

static gboolean
gst_timeshift_src_event (GstBaseSrc * src, GstEvent * event)
{
  GstTimeShiftSrc *timeshiftsrc = GST_TIMESHIFT_SRC (src);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      gdouble rate;
      gdouble current_rate;
      GstFormat format;
      GstSeekFlags flags;
      GstSeekType start_type, stop_type;
      gint64 start, stop;
      gboolean res;

      gst_event_parse_seek (event, &rate, &format, &flags,
          &start_type, &start, &stop_type, &stop);

      if (flags & GST_SEEK_FLAG_INSTANT_RATE_CHANGE) {

        current_rate = GST_BASE_SRC_CAST (src)->segment.rate;

        /* instant rate change only supported if direction does not change. All
         * other requirements are already checked before creating the seek event
         * but let's double-check here to be sure */
        if ((current_rate > 0 && rate < 0) ||
            (current_rate < 0 && rate > 0) ||
            start_type != GST_SEEK_TYPE_NONE ||
            stop_type != GST_SEEK_TYPE_NONE || (flags & GST_SEEK_FLAG_FLUSH)) {
          GST_ERROR_OBJECT (timeshiftsrc,
              "Instant rate change seeks only supported in the "
              "same direction, without flushing and position change");
          return FALSE;
        }

        GST_DEBUG_OBJECT (timeshiftsrc,
            "Handling instant rate change from %g to %g", current_rate, rate);

        GstEvent *ev = gst_event_new_instant_rate_change (rate / current_rate,
            (GstSegmentFlags) (flags & GST_SEGMENT_INSTANT_FLAGS));
        gst_event_set_seqnum (ev, gst_event_get_seqnum (event));

        res = gst_pad_push_event (GST_BASE_SRC_PAD (src), ev);
        return res;
      }
    default:
      break;
  }

  return GST_BASE_SRC_CLASS (parent_class)->event (src, event);
}

static gboolean
gst_timeshift_src_query (GstBaseSrc * src, GstQuery * query)
{
  GstFormat format;
  GstTimeShiftSrc *self = GST_TIMESHIFT_SRC (src);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      gst_query_parse_duration (query, &format, NULL);
      if (format == GST_FORMAT_TIME) {
        if (self->sink && self->sink->state->duration != GST_CLOCK_TIME_NONE) {
          gst_query_set_duration (query, format, self->sink->state->duration);
          res = TRUE;
        }
      }
      break;
    }
    case GST_QUERY_SEEKING:
    {
      guint64 start, end;
      gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
      if (format == GST_FORMAT_TIME) {
        g_mutex_lock (&self->sink->state->ring_buffer.mutex);
        if (self->sink->state->ring_buffer.count > 0) {
          GstBuffer *tail_buf =
              self->sink->state->ring_buffer.
              buffers[self->sink->state->ring_buffer.tail];
          GstBuffer *head_buf =
              self->sink->state->ring_buffer.buffers[(self->sink->state->
                  ring_buffer.head + RING_BUFFER_SIZE -
                  1) % RING_BUFFER_SIZE];
          if (tail_buf && GST_BUFFER_PTS_IS_VALID (tail_buf) && head_buf
              && GST_BUFFER_PTS_IS_VALID (head_buf)) {
            start = GST_BUFFER_PTS (tail_buf);
            end = GST_BUFFER_PTS (head_buf);
            res = TRUE;
          } else {
            start = 0;
            end = -1;
            res = FALSE;
          }
        } else {
          start = 0;
          end = -1;
          res = FALSE;
        }
        g_mutex_unlock (&self->sink->state->ring_buffer.mutex);
        gst_query_set_seeking (query, format, res, start, end);
        res = TRUE;
      }
      break;
    }
    case GST_QUERY_SCHEDULING:
      gst_query_set_scheduling (query, GST_SCHEDULING_FLAG_SEEKABLE, 1, -1, 0);
      gst_query_add_scheduling_mode (query, GST_PAD_MODE_PUSH);
      res = TRUE;
      break;
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (src, query);
      break;
  }

  return res;
}
