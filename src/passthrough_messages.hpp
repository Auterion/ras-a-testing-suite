#pragma once
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include "environment.hpp"

#define MAVLINK_MSG_PACK(MESSAGE_SHORT) mavlink_msg_##MESSAGE_SHORT##_pack
#define MAVLINK_MSG_UNPACK(MESSAGE_SHORT) mavlink_msg_##MESSAGE_SHORT##_decode
#define MAVLINK_MSG_TYPE(MESSAGE_SHORT) mavlink_##MESSAGE_SHORT##_t
#define MAVLINK_MSG_ID(MESSAGE_SHORT_UC) MAVLINK_MSG_ID_##MESSAGE_SHORT_UC

#define USE_MESSAGE(MESSAGE_SHORT, MESSAGE_SHORT_UC) \
    static constexpr int MESSAGE_SHORT_UC = MAVLINK_MSG_ID(MESSAGE_SHORT_UC);                                                 \
    template<>                                                                                \
    struct msg_helper<MESSAGE_SHORT_UC> {                             \
        using decode_type = MAVLINK_MSG_TYPE(MESSAGE_SHORT);                                          \
        static constexpr int ID = MAVLINK_MSG_ID(MESSAGE_SHORT_UC);                                                           \
        static constexpr char NAME[] = #MESSAGE_SHORT_UC;                                             \
        template<typename... Args>                                                                     \
        static void pack(Args... args) {                                                      \
            MAVLINK_MSG_PACK(MESSAGE_SHORT)(args...);                                         \
        }                                                                                     \
        static void unpack(const mavlink_message_t * msg, MAVLINK_MSG_TYPE(MESSAGE_SHORT)* result) {\
            MAVLINK_MSG_UNPACK(MESSAGE_SHORT)(msg, result);                                   \
        }                                                                                     \
    };

template<int MSG>
struct msg_helper {};

/* ----------- LIST ALL MESSAGES TO BE USED IN PASSTHROUGH TESTER BELOW ----------- */

USE_MESSAGE(param_value, PARAM_VALUE)
USE_MESSAGE(param_request_read, PARAM_REQUEST_READ)
USE_MESSAGE(param_set, PARAM_SET)
USE_MESSAGE(param_request_list, PARAM_REQUEST_LIST)
USE_MESSAGE(mission_count, MISSION_COUNT)
USE_MESSAGE(mission_request_int, MISSION_REQUEST_INT)
USE_MESSAGE(mission_item_int, MISSION_ITEM_INT)
USE_MESSAGE(mission_ack, MISSION_ACK)
USE_MESSAGE(mission_request_list, MISSION_REQUEST_LIST)
USE_MESSAGE(mission_set_current, MISSION_SET_CURRENT)
USE_MESSAGE(mission_current, MISSION_CURRENT)
USE_MESSAGE(mission_clear_all, MISSION_CLEAR_ALL)
USE_MESSAGE(mission_request_partial_list, MISSION_REQUEST_PARTIAL_LIST)
USE_MESSAGE(attitude, ATTITUDE)
USE_MESSAGE(sys_status, SYS_STATUS)
USE_MESSAGE(battery_status, BATTERY_STATUS)
USE_MESSAGE(extended_sys_state, EXTENDED_SYS_STATE)
USE_MESSAGE(gps_raw_int, GPS_RAW_INT)
USE_MESSAGE(global_position_int, GLOBAL_POSITION_INT)
USE_MESSAGE(altitude, ALTITUDE)
USE_MESSAGE(vfr_hud, VFR_HUD)
USE_MESSAGE(attitude_quaternion, ATTITUDE_QUATERNION)
USE_MESSAGE(attitude_target, ATTITUDE_TARGET)
USE_MESSAGE(heartbeat, HEARTBEAT)
USE_MESSAGE(home_position, HOME_POSITION)
USE_MESSAGE(local_position_ned, LOCAL_POSITION_NED)
USE_MESSAGE(position_target_local_ned, POSITION_TARGET_LOCAL_NED)
USE_MESSAGE(estimator_status, ESTIMATOR_STATUS)
USE_MESSAGE(command_long, COMMAND_LONG)
USE_MESSAGE(command_int, COMMAND_INT)
USE_MESSAGE(command_ack, COMMAND_ACK)
USE_MESSAGE(protocol_version, PROTOCOL_VERSION)
USE_MESSAGE(ping, PING)
USE_MESSAGE(autopilot_version, AUTOPILOT_VERSION)
USE_MESSAGE(poi_report, POI_REPORT)
USE_MESSAGE(flight_information, FLIGHT_INFORMATION)
USE_MESSAGE(message_interval, MESSAGE_INTERVAL)
USE_MESSAGE(gimbal_device_attitude_status, GIMBAL_DEVICE_ATTITUDE_STATUS)
USE_MESSAGE(gimbal_manager_information, GIMBAL_MANAGER_INFORMATION)
USE_MESSAGE(gimbal_device_information, GIMBAL_DEVICE_INFORMATION)
USE_MESSAGE(camera_information, CAMERA_INFORMATION)
USE_MESSAGE(camera_settings, CAMERA_SETTINGS)
USE_MESSAGE(storage_information, STORAGE_INFORMATION)
USE_MESSAGE(camera_image_captured, CAMERA_IMAGE_CAPTURED)
USE_MESSAGE(camera_capture_status, CAMERA_CAPTURE_STATUS)
USE_MESSAGE(video_stream_information, VIDEO_STREAM_INFORMATION)
