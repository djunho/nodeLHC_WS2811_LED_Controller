/*
 * Based on the implementation of Frans-Willem 
 * https://github.com/Frans-Willem/EspLightNode/tree/master/user/input_protocols
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <lwip/ip_addr.h>

#include "user_config.h"
#include "mxp.h"
#include "ws2811dma.h"

#include <c_types.h>
#include <espconn.h>
#include <esp_wifi.h>
#include <esp_libc.h>

#include "artnet.h"

uint8_t artnet_enabled = 1;
char artnet_shortname[18] = "LHC";
char artnet_longname[64] = "LHC Painel de LED";
uint8_t artnet_net = 0;
uint8_t artnet_subnet = 0;
uint8_t artnet_universe = 0;

uint8_t artnet_active = 0;


static void ICACHE_FLASH_ATTR artnet_recv_opoutput(unsigned char *packet, unsigned short packetlen) {
	if (packetlen >= 8) {
		uint16_t ProtVer=((uint16_t)packet[0] << 8) | packet[1];
		if (ProtVer == 14) {
			//uint8_t Sequence = packet[2];
			//uint8_t Physical = packet[3];
			uint8_t SubUni = packet[4];
			uint8_t Net = packet[5];
		
			if (Net == artnet_net && (SubUni >> 4) == artnet_subnet && (SubUni & 0xF) == artnet_universe) {
				uint16_t Length = ((uint16_t)packet[6] << 8) | packet[7];
				if (packetlen >= 8 + Length){
					ws2811dma_put(&packet[8], MAXPIXELS, 0);
				}
			} else {
				//Not intended for us...
			}
		}
	} else {
		//Invalid length
	}
}

#define ARTNET_OpPoll		0x2000
#define ARTNET_OpPollReply	0x2100
#define ARTNET_OpOutput		0x5000

#define TTM_REPLY_MASK	0

struct ArtNetPollReply {
	uint8_t ID[8];
	uint8_t OpCode[2]; //low-first
	uint8_t IP[4];
	uint8_t Port[2]; //Low-first
	uint8_t VersInfo[2]; //High-first
	uint8_t NetSwitch;
	uint8_t SubSwitch;
	uint8_t Oem[2]; //High-first
	uint8_t Ubea_Version;
	uint8_t Status1;
	uint8_t EstaMan[2]; //Low-first
	uint8_t ShortName[18];
	uint8_t LongName[64];
	uint8_t NodeReport[64];
	uint8_t NumPorts[2]; //High-first
	uint8_t PortTypes[4];
	uint8_t GoodInput[4];
	uint8_t GoodOutput[4];
	uint8_t SwIn[4];
	uint8_t SwOut[4];
	uint8_t SwVideo;
	uint8_t SwMacro;
	uint8_t SwRemote;
	uint8_t Spare[3];
	uint8_t Style;
	uint8_t MAC[6]; //High-byte first
	uint8_t BindIp[4];
	uint8_t BindIndex;
	uint8_t Status2;
	uint8_t Filler[26];
};

#define ARTNET_SET_SHORT_LOFIRST(target,value) (target)[0] = (value) & 0xFF; (target)[1] = (value) >> 8;
#define ARTNET_SET_SHORT_HIFIRST(target,value) (target)[0] = (value) >> 8; (target)[1] = (value) & 0xFF;

static void ICACHE_FLASH_ATTR artnet_recv_oppoll(struct espconn *conn, unsigned char *packet, unsigned short packetlen) {
	if (packetlen >= 3) {
		//uint16_t ProtVer=((uint16_t)packet[0] << 8) | packet[1];
		uint8_t TalkToMe=packet[2];
		//TODO
		if (TalkToMe & TTM_REPLY_MASK) {

		} else {

		}
		struct ip_info ipconfig;
		uint8_t hwaddr[6];

		wifi_get_ip_info(STATION_IF, &ipconfig);
		wifi_get_macaddr(STATION_IF, hwaddr);

		struct ArtNetPollReply response;
		memset(&response, 0, sizeof(struct ArtNetPollReply));
		strcpy((char*)response.ID,"Art-Net");
		ARTNET_SET_SHORT_LOFIRST(response.OpCode, ARTNET_OpPollReply);
		memcpy(response.IP,&ipconfig.ip.addr,4);
		ARTNET_SET_SHORT_LOFIRST(response.Port, ARTNET_Port);
		ARTNET_SET_SHORT_HIFIRST(response.VersInfo, 0);
		response.NetSwitch = artnet_net;
		response.SubSwitch = artnet_subnet;
		ARTNET_SET_SHORT_HIFIRST(response.Oem,0);
		response.Ubea_Version = 0;
		response.Status1 = 0;
		ARTNET_SET_SHORT_LOFIRST(response.EstaMan,0);
		memcpy(response.ShortName,artnet_shortname, sizeof(response.ShortName));
		memcpy(response.LongName,artnet_longname, sizeof(response.LongName));
		strcpy((char *)response.NodeReport,"");
		ARTNET_SET_SHORT_HIFIRST(response.NumPorts,1);
		response.PortTypes[0]=0x80;
		//Not set is set to 0
		//response.GoodInput = 0;
		//response.GoodOutput = 0;
		response.SwOut[0] = artnet_universe;
		//response.SwVideo
		//response.SwMacro
		//response.SwRemote
		//response.Style
		memcpy(response.MAC,hwaddr,6);
		//response.BindIp
		//response.BindIndex
		//response.Status2
		espconn_sent(conn,(uint8_t *)&response,sizeof(struct ArtNetPollReply));
	} else {
		//Invalid length
	}
}

static void ICACHE_FLASH_ATTR artnet_recv(void *arg, char *pusrdata, unsigned short length) {
	artnet_active = 1;
	unsigned char *data =(unsigned char *)pusrdata;
	if (data && length>=10) {
		if (data[0]=='A' && data[1]=='r' && data[2]=='t' && data[3]=='-' && 
            data[4]=='N' && data[5]=='e' && data[6]=='t' && data[7]==0       ) {
			uint16_t OpCode=data[8] | ((uint16_t)data[9] << 8);
			switch (OpCode) {
			case ARTNET_OpOutput:
				return artnet_recv_opoutput(&data[10],length-10);
			case ARTNET_OpPoll:
				return artnet_recv_oppoll((struct espconn *)arg,&data[10],length-10);
			}
		} else {
			//Header invalid
		}
	} else {
		//Package too small.
	}

}

void artnet_init() {
	static struct espconn artnetconn;
	static esp_udp artnetudp;

	artnetconn.type = ESPCONN_UDP;
	artnetconn.state = ESPCONN_NONE;
	artnetconn.proto.udp = &artnetudp;
	artnetudp.local_port = ARTNET_Port;
	if (artnet_enabled) {
		espconn_regist_recvcb(&artnetconn, artnet_recv);
		espconn_create(&artnetconn);
	}
}

uint8_t artnet_is_active(void){
	return artnet_active == 1 ? 1 : 0;
}