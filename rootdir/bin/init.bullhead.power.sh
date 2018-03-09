#!/system/bin/sh

################################################################################
# helper functions to allow Android init like script

function write() {
    echo -n $2 > $1
}

function copy() {
    cat $1 > $2
}

function get-set-forall() {
    for f in $1 ; do
        cat $f
        write $f $2
    done
}

################################################################################

# Ensure the A57s are offline (they should be from the kernel cmdline)
write /sys/devices/system/cpu/cpu4/online 0
write /sys/devices/system/cpu/cpu5/online 0

# disable thermal bcl hotplug to switch governor
write /sys/module/msm_thermal/core_control/enabled 0
get-set-forall /sys/devices/soc.0/qcom,bcl.*/mode disable
bcl_hotplug_mask=`get-set-forall /sys/devices/soc.0/qcom,bcl.*/hotplug_mask 0`
bcl_hotplug_soc_mask=`get-set-forall /sys/devices/soc.0/qcom,bcl.*/hotplug_soc_mask 0`
get-set-forall /sys/devices/soc.0/qcom,bcl.*/mode enable

# some files in /sys/devices/system/cpu are created after the restorecon of
# /sys/. These files receive the default label "sysfs".
# Restorecon again to give new files the correct label.
restorecon -R /sys/devices/system/cpu

# Disable CPU retention
write /sys/module/lpm_levels/system/a53/cpu0/retention/idle_enabled 0
write /sys/module/lpm_levels/system/a53/cpu1/retention/idle_enabled 0
write /sys/module/lpm_levels/system/a53/cpu2/retention/idle_enabled 0
write /sys/module/lpm_levels/system/a53/cpu3/retention/idle_enabled 0
write /sys/module/lpm_levels/system/a57/cpu4/retention/idle_enabled 0
write /sys/module/lpm_levels/system/a57/cpu5/retention/idle_enabled 0

# Disable L2 retention
write /sys/module/lpm_levels/system/a53/a53-l2-retention/idle_enabled 0
write /sys/module/lpm_levels/system/a57/a57-l2-retention/idle_enabled 0

# configure governor settings for little cluster
write /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor interactive
restorecon -R /sys/devices/system/cpu # must restore after interactive
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay "25000 1094400:50000"
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load 90
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate 30000
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq 998400
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy 0
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads "1 800000:85 998400:90 1094400:80"
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time 50000
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/max_freq_hysteresis 50000
write /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 384000

# input boost configuration
write /sys/module/cpu_boost/parameters/input_boost_freq "0:960000"
write /sys/module/cpu_boost/parameters/input_boost_ms 40

# Setting B.L scheduler parameters
write /proc/sys/kernel/sched_window_stats_policy 3
write /proc/sys/kernel/sched_ravg_hist_size 3
write /proc/sys/kernel/sched_small_task 20
write /proc/sys/kernel/sched_mostly_idle_load 30
write /proc/sys/kernel/sched_mostly_idle_nr_run 3

#enable rps static configuration
write /sys/class/net/rmnet_ipa0/queues/rx-0/rps_cpus 8

get-set-forall  /sys/class/devfreq/qcom,cpubw*/governor bw_hwmon

# re-enable thermal and BCL hotplug
write /sys/module/msm_thermal/core_control/enabled 1
get-set-forall /sys/devices/soc.0/qcom,bcl.*/mode disable
get-set-forall /sys/devices/soc.0/qcom,bcl.*/hotplug_mask $bcl_hotplug_mask
get-set-forall /sys/devices/soc.0/qcom,bcl.*/hotplug_soc_mask $bcl_hotplug_soc_mask
get-set-forall /sys/devices/soc.0/qcom,bcl.*/mode enable

# set GPU default power level to 5 (180MHz) instead of 4 (305MHz)
write /sys/class/kgsl/kgsl-3d0/default_pwrlevel 5
