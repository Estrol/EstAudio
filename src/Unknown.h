#ifndef __UNKNOWN_H_
#define __UNKNOWN_H_

enum EST_UnknownType {
    EST_UNKNOWN_NONE = 0,
    EST_UNKNOWN_CHANNEL,
    EST_UNKNOWN_SAMPLE,
    EST_UNKNOWN_DEVICE,
    EST_UNKNOWN_ENCODER,
};

struct EST_Unknown
{
    EST_UnknownType type = EST_UNKNOWN_NONE;
};

#endif