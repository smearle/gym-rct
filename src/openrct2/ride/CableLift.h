/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef _CABLE_LIFT_H_
#define _CABLE_LIFT_H_

#include "../common.h"
#include "Vehicle.h"

Vehicle* cable_lift_segment_create(
    Ride& ride, int32_t x, int32_t y, int32_t z, int32_t direction, uint16_t var_44, int32_t remaining_distance, bool head);
void cable_lift_update(Vehicle* vehicle);
int32_t cable_lift_update_track_motion(Vehicle* cableLift);

#endif
