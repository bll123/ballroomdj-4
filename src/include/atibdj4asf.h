#ifndef INC_ATIBDJ4ASF_H
#define INC_ATIBDJ4ASF_H

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  ASF_GUID_LEN = 16,
};

typedef struct {
  const char  *guid;
  const char  *nm;
} guidlist_t;

typedef struct {
  int32_t   a;
  int16_t   b;
  int16_t   c;
  char      d [8];
} guid_t;

typedef struct {
  uint32_t  numobj;
  uint8_t   reservea;
  uint8_t   reserveb;
} __attribute__((packed)) asfheader_t;

typedef struct {
  guid_t    fileid;
  uint64_t  fsz;
  uint64_t  creationdate;
  uint64_t  packetcount;
  uint64_t  duration;
  uint64_t  senddur;
  uint64_t  preroll;
  uint32_t  flags;
  uint32_t  minpacketsize;
  uint32_t  maxpacketsize;
  uint32_t  maxbitrate;
} asffileprop_t;

typedef struct {
  guid_t    reservea;
  uint16_t  reserveb;
  uint32_t  datasz;
} __attribute__((packed)) asfheaderext_t;

typedef struct {
  uint16_t  count;
} asfmetadata_t;

enum {
  ASF_DATA_UTF8 = 0,
  ASF_DATA_BIN  = 1,
  ASF_DATA_BOOL16 = 2,
  ASF_DATA_U32 = 3,
  ASF_DATA_U64 = 4,
  ASF_DATA_U16 = 5,
  ASF_DATA_GUID = 6,
  ASF_DATA_BOOL32 = 12,
} asfdata_t;

typedef struct {
  uint16_t  reserved;
  uint16_t  streamnum;
  uint16_t  nmlen;
  uint16_t  datatype;
  uint32_t  datalen;
} asfmetadatadesc_t;

typedef struct {
  uint16_t  nmlen;
} asfcontentnm_t;

typedef struct {
  uint16_t  datatype;
  uint16_t  datalen;
} asfcontentdata_t;

enum {
  ASF_CONTENT_TITLE,
  ASF_CONTENT_AUTHOR,
  ASF_CONTENT_COPYRIGHT,
  ASF_CONTENT_DESC,
  ASF_CONTENT_RATING,
  ASF_CONTENT_MAX,
};

typedef struct {
  uint16_t  len [ASF_CONTENT_MAX];
} asfcontent_t;

enum {
  ASF_GUID_HEADER,
  ASF_GUID_FILE_PROP,
  ASF_GUID_HEADER_EXT,
  ASF_GUID_METADATA,
  ASF_GUID_METADATA_LIBRARY,
  ASF_GUID_PADDING,
  ASF_GUID_EXT_CONTENT_DESC,
  ASF_GUID_CONTENT_DESC,
  ASF_GUID_CODEC_LIST,
  ASF_GUID_STREAM_PROP,
  ASF_GUID_STREAM_BITRATE_PROP,
  ASF_GUID_CONTENT_ENC,
  ASF_GUID_EXT_CONTENT_ENC,
  ASF_GUID_AUDIO_MEDIA,
  ASF_GUID_VIDEO_MEDIA,
  ASF_GUID_MAX,
  ASF_GUID_OTHER,
};

static const char *asf_content_names [ASF_CONTENT_MAX] = {
  [ASF_CONTENT_TITLE] = "WM/Title",
  [ASF_CONTENT_AUTHOR] = "WM/Author",
  [ASF_CONTENT_COPYRIGHT] = "Copyright",
  [ASF_CONTENT_DESC] = "Description",
  [ASF_CONTENT_RATING] = "Rating",
};

static guidlist_t asf_guids [ASF_GUID_MAX] = {
  [ASF_GUID_HEADER] =
      { "75B22630-668E-11CF-A6D9-00AA0062CE6C", "header" },
  [ASF_GUID_FILE_PROP] =
      { "8CABDCA1-A947-11CF-8EE4-00C00C205365", "file-prop" },
  [ASF_GUID_HEADER_EXT] =
      { "5FBF03B5-A92E-11CF-8EE3-00C00C205365", "header-ext" },
  [ASF_GUID_STREAM_PROP] =
      { "B7DC0791-A9B7-11CF-8EE6-00C00C205365", "stream-prop" },
  [ASF_GUID_STREAM_BITRATE_PROP] =
      { "7BF875CE-468D-11D1-8D82-006097C9A2B2", "stream-bitrate-prop" },
  [ASF_GUID_CODEC_LIST] =
      { "86D15240-311D-11D0-A3A4-00A0C90348F6", "codec-list" },
  [ASF_GUID_CONTENT_DESC] =
      { "75B22633-668E-11CF-A6D9-00AA0062CE6C", "content-desc" },
  [ASF_GUID_EXT_CONTENT_DESC] =
      { "D2D0A440-E307-11D2-97F0-00A0C95EA850", "ext-content-desc" },
  [ASF_GUID_PADDING] =
      { "1806D474-CADF-4509-A4BA-9AABCB96AAE8", "padding" },
  [ASF_GUID_METADATA] =
      { "C5F8CBEA-5BAF-4877-8467-AA8C44FA4CCA", "metadata" },
  [ASF_GUID_METADATA_LIBRARY] =
      { "44231C94-9498-49D1-A141-1D134E457054", "metadata-library" },
  [ASF_GUID_CONTENT_ENC] =
      { "2211B3FB-BD23-11D2-B4B7-00A0C955FC6E", "content-enc" },
  [ASF_GUID_EXT_CONTENT_ENC] =
      { "298AE614-2622-4C17-B935-DAE07EE9289C", "ext-content-enc" },
  [ASF_GUID_AUDIO_MEDIA] =
      { "F8699E40-5B4D-11CF-A8FD-00805F5C442B", "audio-media" },
  [ASF_GUID_VIDEO_MEDIA] =
      { "BC19EFC0-5B4D-11CF-A8FD-00805F5C442B", "video-media" },
};

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ATIBDJ4ASF_H */
