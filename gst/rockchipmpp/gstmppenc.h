/*
 * Copyright 2017 Rockchip Electronics Co., Ltd
 *     Author: Randy Li <randy.li@rock-chips.com>
 *
 * Copyright 2021 Rockchip Electronics Co., Ltd
 *     Author: Jeffy Chen <jeffy.chen@rock-chips.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef  __GST_MPP_ENC_H__
#define  __GST_MPP_ENC_H__

#include <gst/video/gstvideoencoder.h>

#include "gstmpp.h"

G_BEGIN_DECLS;

#define GST_TYPE_MPP_ENC (gst_mpp_enc_get_type())
G_DECLARE_FINAL_TYPE (GstMppEnc, gst_mpp_enc, GST, MPP_ENC, GstVideoEncoder);

#define GST_MPP_ENC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    GST_TYPE_MPP_ENC, GstMppEnc))

struct _GstMppEnc
{
  GstVideoEncoder parent;

  GMutex mutex;
  GstAllocator *allocator;
  GstVideoCodecState *input_state;

  /* final input video info */
  GstVideoInfo info;

  /* stop handling new frame when flushing */
  gboolean flushing;

  /* drop frames when flushing but not draining */
  gboolean draining;

  /* frame system numbers that are ready for sending to MPP */
  GList *frames;

  /* Max number of pending frames */
  guint32 max_pending;

  guint pending_frames;
  GMutex event_mutex;
  GCond event_cond;

  /* flow return from pad task */
  GstFlowReturn task_ret;

  MppEncHeaderMode header_mode;
  MppEncRcMode rc_mode;
  MppEncSeiMode sei_mode;

  gint rotation;
  gint width;
  gint height;

  gint gop;
  guint max_reenc;

  guint bps;
  guint bps_min;
  guint bps_max;

  gint fps_out;           /* output framerate numerator (0 = same as input) */
  gint drop_mode;         /* MppEncRcDropFrmMode: 0=disabled, 1=normal, 2=pskip */
  guint drop_threshold;   /* % over bps_max that triggers drop (default 50) */

  /* Rolling intra refresh: spreads I-macroblocks across frames instead of
   * periodic IDR spikes. 0 = disabled, else number of MB rows refreshed per
   * frame. Tuned for lossy low-latency streaming. */
  guint intra_refresh;

  /* Super-frame handling: bound the size of a single coded frame so a scene
   * cut / keyframe cannot spike the send buffer. */
  gint super_mode;        /* MppEncRcSuperFrameMode: 0=none, 1=drop, 2=reenc */
  guint super_i_thd;      /* I-frame size threshold in bytes (0 = auto) */
  guint super_p_thd;      /* P-frame size threshold in bytes (0 = auto) */

  /* De-breathing: smooths the GOP bitrate "breathing" oscillation. */
  gboolean debreath;
  guint debreath_strength; /* [0, 35] */

  /* Content-adaptive tuning. */
  gint scene_mode;        /* 0=default, 1=ipc, 2=ipc-ptz */
  guint anti_flicker;     /* temporal anti-flicker strength [0, 3], 0 = off */

  gboolean zero_copy_pkt;

  gboolean arm_afbc;

  gboolean prop_dirty;

  /* Set when width/height changes while the encoder is already running, so the
   * next frame reconfigures the scaler in place instead of rejecting the change. */
  gboolean res_dirty;

  MppEncCfg mpp_cfg;
  MppFrame mpp_frame;

  MppCodingType mpp_type;
  MppCtx mpp_ctx;
  MppApi *mpi;
};

#define MPP_ENC_IN_FORMATS \
    "NV12, I420, YUY2, UYVY, " \
    "BGR16, RGB16, BGR, RGB, " \
    "ABGR, ARGB, BGRA, RGBA, xBGR, xRGB, BGRx, RGBx"

#ifdef HAVE_RGA
#define MPP_ENC_FORMATS MPP_ENC_IN_FORMATS "," GST_RGA_FORMATS
#else
#define MPP_ENC_FORMATS MPP_ENC_IN_FORMATS
#endif

gboolean gst_mpp_enc_apply_properties (GstVideoEncoder * encoder);
gboolean gst_mpp_enc_set_src_caps (GstVideoEncoder * encoder, GstCaps * caps);

gboolean gst_mpp_enc_supported (MppCodingType mpp_type);

G_END_DECLS;

#endif /* __GST_MPP_ENC_H__ */
