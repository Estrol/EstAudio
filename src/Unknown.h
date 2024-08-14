/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#ifndef __UNKNOWN_H_
#define __UNKNOWN_H_

#include <format>
#include <iostream>

enum EST_UnknownType {
    EST_UNKNOWN_NONE = 0,
    EST_UNKNOWN_CHANNEL,
    EST_UNKNOWN_SAMPLE,
    EST_UNKNOWN_DEVICE,
    EST_UNKNOWN_ENCODER,
    EST_UNKNOWN_DATA_CALLBACK,
    EST_UNKNOWN_ENCODER_CALLBACK,
    EST_UNKNOWN_FX,
    EST_UNKNOWN_FX_INSTANCE
};

const char *EST_UnknownTypeToString(EST_UnknownType type);

struct EST_Unknown
{
    EST_UnknownType type = EST_UNKNOWN_NONE;
};

#endif