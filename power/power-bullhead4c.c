/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *    * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define LOG_NIDEBUG 0

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdlib.h>

#define LOG_TAG "QCOM PowerHAL"
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/power.h>

#include "utils.h"
#include "metadata-defs.h"
#include "hint-data.h"
#include "performance.h"
#include "power-common.h"

int get_number_of_profiles() {
    return 5;
}

static int current_power_profile = PROFILE_BALANCED;

static void set_power_profile(int profile) {

    if (profile == current_power_profile)
        return;

    ALOGV("%s: profile=%d", __func__, profile);

    if (current_power_profile != PROFILE_BALANCED) {
        undo_hint_action(DEFAULT_PROFILE_HINT_ID);
        ALOGV("%s: hint undone", __func__);
    }

    if (profile == PROFILE_POWER_SAVE) {
        int resource_values[] = { CPUS_ONLINE_MAX_LIMIT_2,
            CPU0_MAX_FREQ_NONTURBO_MAX, CPU1_MAX_FREQ_NONTURBO_MAX,
            CPU2_MAX_FREQ_NONTURBO_MAX, CPU3_MAX_FREQ_NONTURBO_MAX };
        perform_hint_action(DEFAULT_PROFILE_HINT_ID,
            resource_values, ARRAY_SIZE(resource_values));
        ALOGD("%s: set powersave", __func__);
    } else if (profile == PROFILE_HIGH_PERFORMANCE) {
        int resource_values[] = { SCHED_BOOST_ON, CPUS_ONLINE_MAX_LIMIT_4,
            ALL_CPUS_PWR_CLPS_DIS, 0x0901,
            CPU0_MIN_FREQ_TURBO_MAX, CPU1_MIN_FREQ_TURBO_MAX,
            CPU2_MIN_FREQ_TURBO_MAX, CPU3_MIN_FREQ_TURBO_MAX };
        perform_hint_action(DEFAULT_PROFILE_HINT_ID,
            resource_values, ARRAY_SIZE(resource_values));
        ALOGD("%s: set performance mode", __func__);
    }

    current_power_profile = profile;
}

extern void interaction(int duration, int num_args, int opt_list[]);

static void process_video_decode_hint(void *metadata)
{
    char governor[80];
    struct video_decode_metadata_t video_decode_metadata;

    if (get_scaling_governor(governor, sizeof(governor)) == -1) {
        ALOGE("Can't obtain scaling governor.");

        return;
    }

    if (metadata) {
        ALOGI("Processing video decode hint. Metadata: %s", (char *)metadata);
    }

    /* Initialize encode metadata struct fields. */
    memset(&video_decode_metadata, 0, sizeof(struct video_decode_metadata_t));
    video_decode_metadata.state = -1;
    video_decode_metadata.hint_id = DEFAULT_VIDEO_DECODE_HINT_ID;

    if (metadata) {
        if (parse_video_decode_metadata((char *)metadata, &video_decode_metadata) ==
            -1) {
            ALOGE("Error occurred while parsing metadata.");
            return;
        }
    } else {
        return;
    }

    if (video_decode_metadata.state == 1) {
        if ((strncmp(governor, ONDEMAND_GOVERNOR, strlen(ONDEMAND_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(ONDEMAND_GOVERNOR))) {
            int resource_values[] = {THREAD_MIGRATION_SYNC_OFF};

            perform_hint_action(video_decode_metadata.hint_id,
                    resource_values, ARRAY_SIZE(resource_values));
        } else if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            int resource_values[] = {TR_MS_30, HISPEED_LOAD_90, HS_FREQ_1026};

            perform_hint_action(video_decode_metadata.hint_id,
                    resource_values, ARRAY_SIZE(resource_values));
        }
    } else if (video_decode_metadata.state == 0) {
        if ((strncmp(governor, ONDEMAND_GOVERNOR, strlen(ONDEMAND_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(ONDEMAND_GOVERNOR))) {
        } else if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            undo_hint_action(video_decode_metadata.hint_id);
        }
    }
}

static int process_video_encode_hint(void *metadata)
{
    char governor[80];
    struct video_encode_metadata_t video_encode_metadata;

    if (get_scaling_governor(governor, sizeof(governor)) == -1) {
        ALOGE("Can't obtain scaling governor.");

        return HINT_NONE;
    }

    /* Initialize encode metadata struct fields */
    memset(&video_encode_metadata, 0, sizeof(struct video_encode_metadata_t));
    video_encode_metadata.state = -1;
    video_encode_metadata.hint_id = DEFAULT_VIDEO_ENCODE_HINT_ID;

    if (metadata) {
        if (parse_video_encode_metadata((char *)metadata, &video_encode_metadata) ==
            -1) {
            ALOGE("Error occurred while parsing metadata.");
            return HINT_NONE;
        }
    } else {
        return HINT_NONE;
    }

    if (video_encode_metadata.state == 1) {
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            /* sched and cpufreq params
             * hispeed freq - 768 MHz
             * target load - 90
             * above_hispeed_delay - 40ms
             * sched_small_tsk - 50
             */
            int resource_values[] = {0x2C07, 0x2F5A, 0x2704, 0x4032};

            perform_hint_action(video_encode_metadata.hint_id,
                    resource_values, ARRAY_SIZE(resource_values));
            return HINT_HANDLED;
        }
    } else if (video_encode_metadata.state == 0) {
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            undo_hint_action(video_encode_metadata.hint_id);
            return HINT_HANDLED;
        }
    }
    return HINT_NONE;
}

int power_hint_override(__attribute__((unused)) struct power_module *module,
        power_hint_t hint, void *data)
{
    if (hint == POWER_HINT_SET_PROFILE) {
        set_power_profile(*(int32_t *)data);
    }

    // Skip other hints in custom power modes
    if (current_power_profile != PROFILE_BALANCED) {
        return HINT_HANDLED;
    }

    if (hint == POWER_HINT_INTERACTION) {
        int duration = 500, duration_hint = 0;
        static struct timespec s_previous_boost_timespec;
        struct timespec cur_boost_timespec;
        long long elapsed_time;

        if (data) {
            duration_hint = *((int *)data);
        }

        duration = duration_hint > 0 ? duration_hint : 500;

        clock_gettime(CLOCK_MONOTONIC, &cur_boost_timespec);
        elapsed_time = calc_timespan_us(s_previous_boost_timespec, cur_boost_timespec);
        if (elapsed_time > 750000)
            elapsed_time = 750000;
        // don't hint if it's been less than 250ms since last boost
        // also detect if we're doing anything resembling a fling
        // support additional boosting in case of flings
        else if (elapsed_time < 250000 && duration <= 750)
            return HINT_HANDLED;

        s_previous_boost_timespec = cur_boost_timespec;

        if (duration >= 1500) {
            int resources[] = {
                ALL_CPUS_PWR_CLPS_DIS,
                SCHED_BOOST_ON,
                SCHED_PREFER_IDLE_DIS,
                0x20D
            };
            interaction(duration, ARRAY_SIZE(resources), resources);
        } else {
            int resources[] = {
                ALL_CPUS_PWR_CLPS_DIS,
                SCHED_PREFER_IDLE_DIS,
                0x20D
            };
            interaction(duration, ARRAY_SIZE(resources), resources);
        }
        return HINT_HANDLED;
    }

    if (hint == POWER_HINT_LAUNCH) {
        int duration = 2000;
        int resources[] = {
            ALL_CPUS_PWR_CLPS_DIS,
            SCHED_BOOST_ON,
            SCHED_PREFER_IDLE_DIS,
            0x20F,
            0x1C00,
            0x4001,
            0x4101,
            0x4201
        };

        interaction(duration, ARRAY_SIZE(resources), resources);

        return HINT_HANDLED;
	}

    if (hint == POWER_HINT_CPU_BOOST) {
        int duration = *(int32_t *)data / 1000;
        int resources[] = {
            ALL_CPUS_PWR_CLPS_DIS,
            SCHED_BOOST_ON,
            SCHED_PREFER_IDLE_DIS,
            0x20D
        };

        if (duration > 0)
            interaction(duration, ARRAY_SIZE(resources), resources);

        return HINT_HANDLED;
	}

    if (hint == POWER_HINT_VIDEO_ENCODE) {
        process_video_encode_hint(data);
        return HINT_HANDLED;
    }

    if (hint == POWER_HINT_VIDEO_DECODE) {
        process_video_decode_hint(data);
        return HINT_HANDLED;
    }

	return HINT_NONE;
}

int set_interactive_override(__attribute__((unused)) struct power_module *module, int on)
{
    char governor[80];

    ALOGI("Got set_interactive hint");
    if (get_scaling_governor_check_cores(governor, sizeof(governor),CPU0) == -1) {
        if (get_scaling_governor_check_cores(governor, sizeof(governor),CPU1) == -1) {
            if (get_scaling_governor_check_cores(governor, sizeof(governor),CPU2) == -1) {
                if (get_scaling_governor_check_cores(governor, sizeof(governor),CPU3) == -1) {
                    ALOGE("Can't obtain scaling governor.");
                    return HINT_HANDLED;
                }
            }
        }
    }

    if (!on) {
        /* Display off */
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
            (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            // sched upmigrate = 99, sched downmigrate = 95
            // keep the big cores around, but make them very hard to use
            int resource_values[] = {TR_MS_50, THREAD_MIGRATION_SYNC_OFF};
            perform_hint_action(DISPLAY_STATE_HINT_ID,
                    resource_values, ARRAY_SIZE(resource_values));
            return HINT_HANDLED;
        }
    } else {
        /* Display on */
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
            (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            undo_hint_action(DISPLAY_STATE_HINT_ID);
            return HINT_HANDLED;
        }
    }
    return HINT_NONE;
}