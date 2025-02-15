//  Copyright (c) 2003-2023 Movella Technologies B.V. or subsidiaries worldwide.
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//  1.	Redistributions of source code must retain the above copyright notice,
//  	this list of conditions, and the following disclaimer.
//  
//  2.	Redistributions in binary form must reproduce the above copyright notice,
//  	this list of conditions, and the following disclaimer in the documentation
//  	and/or other materials provided with the distribution.
//  
//  3.	Neither the names of the copyright holders nor the names of their contributors
//  	may be used to endorse or promote products derived from this software without
//  	specific prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
//  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR
//  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.THE LAWS OF THE NETHERLANDS 
//  SHALL BE EXCLUSIVELY APPLICABLE AND ANY DISPUTES SHALL BE FINALLY SETTLED UNDER THE RULES 
//  OF ARBITRATION OF THE INTERNATIONAL CHAMBER OF COMMERCE IN THE HAGUE BY ONE OR MORE 
//  ARBITRATORS APPOINTED IN ACCORDANCE WITH SAID RULES.
//  


#ifndef STATUSPUBLISHER_H
#define STATUSPUBLISHER_H

#include "packetcallback.h"
#include "xsens_mti_ros2_driver/msg/xs_status_word.hpp"


struct StatusPublisher : public PacketCallback
{
    rclcpp::Publisher<xsens_mti_ros2_driver::msg::XsStatusWord>::SharedPtr pub;
    //std::string frame_id = DEFAULT_FRAME_ID;

    StatusPublisher(rclcpp::Node &node)
    {
        int pub_queue_size = 5;

        node.get_parameter("publisher_queue_size", pub_queue_size);
        //node.get_parameter("frame_id", frame_id);

        pub = node.create_publisher<xsens_mti_ros2_driver::msg::XsStatusWord>("/status", pub_queue_size);
    }

    void parseToMessage(xsens_mti_ros2_driver::msg::XsStatusWord &msg, uint32_t status)
    {
        msg.selftest = status & (1 << 0);
        msg.filter_valid = status & (1 << 1);
        msg.gnss_fix = status & (1 << 2);
        msg.no_rotation_update_status = (status >> 3) & 0x3; //status & 0x18;
        msg.representative_motion = status & (1 << 5);
        msg.clock_bias_estimation = status & (1 << 6);
        msg.clipflag_acc_x = status & (1 << 8);
        msg.clipflag_acc_y = status & (1 << 9);
        msg.clipflag_acc_z = status & (1 << 10);
        msg.clipflag_gyr_x = status & (1 << 11);
        msg.clipflag_gyr_y = status & (1 << 12);
        msg.clipflag_gyr_z = status & (1 << 13);
        msg.clipflag_mag_x = status & (1 << 14);
        msg.clipflag_mag_y = status & (1 << 15);
        msg.clipflag_mag_z = status & (1 << 16);
        msg.clipping_indication = status & (1 << 19);
        msg.syncin_marker = status & (1 << 21);
        msg.syncout_marker = status & (1 << 22);
        msg.filter_mode = (status >> 23) & 0x7;//status & 0x03800000;
        msg.have_gnss_time_pulse = status & (1 << 26);
        msg.rtk_status = (status >> 27) & 0x3;//status & 0x18000000;
    }

    void operator()(const XsDataPacket &packet, rclcpp::Time timestamp)
    {
        if (packet.containsStatus())
        {
            xsens_mti_ros2_driver::msg::XsStatusWord msg;

            uint32_t status = packet.status();
            parseToMessage(msg, status);

            pub->publish(msg);
        }
    }
};

#endif
