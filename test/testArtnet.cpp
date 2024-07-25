#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

extern "C" {
    #include "artnet.h"

    bool artnet_recv_opoutput(unsigned char *packet, ssize_t packetlen);

    void callbackToHandleLED(uint8_t buffer[], uint16_t pixels, uint16_t offset)
    {
        (void)buffer;
        (void)pixels;
        (void)offset;
        mock().actualCall(__FUNCTION__)
            .withParameter("buffer", buffer)
            .withParameter("pixels", pixels)
            .withParameter("offset", offset);
    }
}

TEST_GROUP(artnet)
{
    void setup()
    {
        mock().disable();
        artnet_init(callbackToHandleLED, 8);
        mock().enable();
    }
    void teardown()
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(artnet, checking_packet_lengh_under_size)
{
    unsigned char packet[] = "a";
    ssize_t len = 1;
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_lengh_under_size2)
{
    unsigned char packet[] = "aaaaaaa";
    ssize_t len = 7;
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_correct_but_empty)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0e, 0x00, 0x00, 0x00,      0x00,   0x00, 0x00};
    ssize_t len = sizeof(packet);
    mock().expectOneCall("callbackToHandleLED")
        .withParameter("buffer", &packet[8])
        .withParameter("pixels", 0)
        .withParameter("offset", 0);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(true, ret);
}

TEST(artnet, checking_packet_incorrect_version)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0f, 0x00, 0x00, 0x00,      0x00,   0x00, 0x00};
    ssize_t len = sizeof(packet);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_incorrect_subUni)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0f, 0x00, 0x00, 0x01,      0x00,   0x00, 0x00};
    ssize_t len = sizeof(packet);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_incorrect_Net)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0f, 0x00, 0x00, 0x00,      0x01,   0x00, 0x00};
    ssize_t len = sizeof(packet);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_incorrect_Net_and_subUni)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0f, 0x00, 0x00, 0x01,      0x01,   0x00, 0x00};
    ssize_t len = sizeof(packet);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_correct_but_incorrect_led_lenght)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0e, 0x00, 0x00, 0x00,      0x00,   0x00, 0x01, 0x00, 0x00};
    ssize_t len = sizeof(packet);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(false, ret);
}

TEST(artnet, checking_packet_correct_1_led)
{
    //                        ProtVer[2],  NotUsed[2], SubUni[1], Net[1]   Leghth[2], Data[]
    unsigned char packet[] = { 0x00, 0x0e, 0x00, 0x00, 0x00,      0x00,   0x00, 0x01, 0x00, 0x00, 0x00};
    ssize_t len = sizeof(packet);
    mock().expectOneCall("callbackToHandleLED")
        .withParameter("buffer", &packet[8])
        .withParameter("pixels", 1)
        .withParameter("offset", 0);
    bool ret = artnet_recv_opoutput(packet, len);
    CHECK_EQUAL(true, ret);
}

TEST_GROUP(artnet_init)
{
    void setup()
    {
    }
    void teardown()
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(artnet_init, checking_init)
{
    mock().expectOneCall("xTaskCreate")
        .withParameter("pcName", "Artnet UDP port")
        .ignoreOtherParameters()
        .andReturnValue(0);
    artnet_init(callbackToHandleLED, 8);

}
