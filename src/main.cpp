//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Observe scanner-key on brother scanner/printer/multi-function-device.
*/
//---------------------------------------------------------------------------------------------------------------------


/* -- Includes ------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
   #include <winsock2.h>
   typedef int socklen_t;
#else
   #include <unistd.h>
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
   #include <netdb.h>
   #include <netdb.h>
   #include <fcntl.h>
#endif
#include <iostream>
#include <thread>
#include "cJSON.h"
#include "snmp.h"
#include "netutils.h"



/* -- Defines -------------------------------------------------------------- */

/* -- Types ---------------------------------------------------------------- */

/* -- Prototypes ----------------------------------------------------------- */
static int main_task(cJSON * configJson);
static void observer_task(cJSON * configJson);
static const char * _config_json_get_cmd(cJSON * configJson, const char * deviceIp, const char * function, const char * cmdId);
static char * _cjson_read_file(const char *filename);



/* -- Variables ------------------------------------------------------------ */


/* -- Implementation ------------------------------------------------------- */


int main(int argc, char * argv[])
{
   const char * configStr = _cjson_read_file("config.json");
   if (configStr != nullptr)
   {
      cJSON * configJson = cJSON_Parse(configStr);
      free((void *)configStr); //i don't need the raw string any more...
      if (configJson != nullptr)
      {
         main_task(configJson);
         cJSON_Delete(configJson);
      }
   }
   return 0;
}




static int main_task(cJSON * configJson)
{
#ifdef WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif


   //create an extra thread, that receives (and processes) the key event messages
   std::thread observer(&observer_task, configJson);


   //SNMP buffer
   uint8_t snmpBuffer[256]; //for our messages, this should be enough...

   //setup SNMP header
   snmp_msg_header snmpHeader;
   snmpHeader.snmp_ver = 0;
   snmpHeader.community = "internal"; //brothers scanner key is "internal" (hat brother halt so definiert)
   snmpHeader.pdu_type = SNMP_DATA_T_PDU_SET_REQUEST;
   snmpHeader.request_id = 0;
   snmpHeader.error_status = 0;
   snmpHeader.error_index = 0;

   //perpare var-binding to register to function  brRegisterKeyInfo
   //according to http://mibs.observium.org/mib/BROTHER-MIB this function has OID-address 1.3.6.1.4.1.2435.2.3.9.2.11.1.1
   snmp_varbind brRegisterKeyInfo;
   brRegisterKeyInfo.oid[0] = 1;
   brRegisterKeyInfo.oid[1] = 3;
   brRegisterKeyInfo.oid[2] = 6;
   brRegisterKeyInfo.oid[3] = 1;
   brRegisterKeyInfo.oid[4] = 4;
   brRegisterKeyInfo.oid[5] = 1;
   brRegisterKeyInfo.oid[6] = 2435;
   brRegisterKeyInfo.oid[7] = 2;
   brRegisterKeyInfo.oid[8] = 3;
   brRegisterKeyInfo.oid[9] = 9;
   brRegisterKeyInfo.oid[10] = 2;
   brRegisterKeyInfo.oid[11] = 11;
   brRegisterKeyInfo.oid[12] = 1;
   brRegisterKeyInfo.oid[13] = 1;
   brRegisterKeyInfo.oid[14] = 0;
   brRegisterKeyInfo.oid[15] = SNMP_MSG_OID_END;
   brRegisterKeyInfo.value_type = SNMP_DATA_T_OCTET_STRING;
   //brRegisterKeyInfo.value.s = "TYPE=BR;BUTTON=SCAN;USER=\"JPG 300\";FUNC=IMAGE;HOST=192.168.178.26:54925;APPNUM=1;DURATION=300;BRID=;'"


   //dummy - print out the json content
   do
   {
      const cJSON * device;
      cJSON_ArrayForEach(device, configJson) //for each device/scanner object in the config array
      {
         //get IP
         const cJSON * ip = cJSON_GetObjectItemCaseSensitive(device, "ip");
         if ((ip == nullptr) || !cJSON_IsString(ip))
         {
            continue; //invalid object
         }
         // printf("Scanner at %s\n", ip->valuestring);

         //create udp socket
         int sock = socket(AF_INET, SOCK_DGRAM, 0);
         if (sock < 0)
         {
            return -1; //cant create socket -> OS error
         }

         //configure receive timeout of socket
         {
         #ifdef _WIN32
            unsigned long t = 3000; //milli-seconds
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof(t));
         #else
            struct timeval t;
            t.tv_sec = 3;
            t.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof(t));
         #endif
         }

         //we use the "connected API" of UDP
         //that means we are using connect + send/recv instead of sendto/recvFrom (without an connect)
         //i do this, because the connected API offers me an convenient way to find out the local ip
         //(which is needed later to be sent in the udp paket payload...)
         //
         //although UDP is connectionless, we can do a "connect". This will not establish a "real connection" to that host
         //but tricks the OS to lock into the routing table for the the network interface the UDP packet would have to be sent to...
         struct sockaddr_in address;
         uint32_t localIp = (uint32_t)-1; //preset with broadcast addres 255.255.255.255
         address.sin_addr.s_addr = inet_addr(ip->valuestring);
         address.sin_port = htons(161);            //snmp port
         address.sin_family = AF_INET;
         int status = connect(sock, (struct sockaddr *)&address, sizeof(address));
         if (status == 0) //connected successfully
         {
            unsigned int len = sizeof(address);
            status = getsockname(sock, (struct sockaddr *)&address, &len); //read back the address the OS has assigned to me
            if (status == 0) //on success
            {
               localIp = address.sin_addr.s_addr;
            }
         }

         //get the registrations for the respective function
         for (unsigned int f = 0; f < 4; ++f)
         {
            static const char * _function[] =
            {
               "FILE",
               "IMAGE",
               "OCR",
               "EMAIL"
            };

            //get function array
            const cJSON * function = cJSON_GetObjectItemCaseSensitive(device, _function[f]);
            if ((function == nullptr) || !cJSON_IsArray(function))
            {
               continue; //invalid
            }
            const cJSON * registration;
            cJSON_ArrayForEach(registration, function) //for each registration for the respective function
            {
               const cJSON * id = cJSON_GetObjectItemCaseSensitive(registration, "id");
               if ((id == nullptr) || !cJSON_IsString(id))
               {
                  continue; //invalid object
               }
               const cJSON * cmd = cJSON_GetObjectItemCaseSensitive(registration, "cmd");
               if ((cmd == nullptr) || !cJSON_IsString(cmd))
               {
                  continue; //invalid object
               }
               // printf("Register %s for %s\n", id->valuestring, cmd->valuestring);

               char payload[128]; //buffer for the brRegisterKeyInfo (1.3.6.1.4.1.2435.2.3.9.2.11.1.1.0) payload string
               const uint8_t * const _localIp = (uint8_t *)&localIp;
               sprintf(payload,
                  "TYPE=BR;BUTTON=SCAN;USER=\"%s\";FUNC=%s;HOST=%u.%u.%u.%u:54925;APPNUM=1;DURATION=300;BRID=;",
                  id->valuestring, //USER
                  _function[f],    //FUNC
                  _localIp[0], _localIp[1], _localIp[2], _localIp[3] //HOST
               );
               //puts(payload); //debug output only
               brRegisterKeyInfo.value.s = payload;
               snmpHeader.request_id++;
               uint8_t * snmpMsg = snmp_encode_msg(&snmpBuffer[sizeof(snmpBuffer) - 1], &snmpHeader, 1, &brRegisterKeyInfo);
               size_t snmpMsgLen = (size_t)(&snmpBuffer[sizeof(snmpBuffer)] - snmpMsg);

               //send to the device
               send(sock, snmpMsg, snmpMsgLen, 0);

               //wait for response (timeout as configured above)
               recv(sock, snmpBuffer, sizeof(snmpBuffer), 0);
            }
         }

         //close socket
         #ifdef _WIN32
            closesocket((SOCKET)sock);
         #else
            close(sock);
         #endif
      }

      //wait before registration renewal
      std::this_thread::sleep_for(std::chrono::seconds(280)); //280 < 300, as registration expiers (as configured) after 300s
   } while (false /*true*/);


   //wait for the observer thread
   observer.join();


   //cleanup
#ifdef _WIN32
   WSACleanup();
#endif
   return 0;
}




static void observer_task(cJSON * configJson)
{
   //create udp socket
   int sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock >= 0)
   {
      struct sockaddr_in address;
      address.sin_addr.s_addr = INADDR_ANY; //accept messages from all IPs
      address.sin_port = htons(54925);      //device will broadcast key events to that port
      address.sin_family = AF_INET;
      int status = bind(sock, (struct sockaddr *)&address, sizeof(address));
      if (status == 0) //bound successfully
      {
         char sender[16];
         uint8_t buffer[256];
         unsigned int seq = 0;
         while (true)
         {
            unsigned int len = sizeof(address);
            int rx = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, &len);
            if (rx > 4)
            {
               buffer[rx] = 0; //ensure zero termination
               //get ip of sender
               const uint8_t * ip = (uint8_t *)&address.sin_addr.s_addr;
               sprintf(sender, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
               //parse message payload
               char * payload = (char *)&buffer[4];
               char * s;
               s = strstr(payload, "SEQ=");
               if (s != nullptr)
               {
                  unsigned int _seq = strtoul(s + 4, nullptr, 0);
                  if (_seq != seq)
                  {
                     seq = _seq;
                     //find user and function (this must be done first, as i will modify the payload afterwards ...)
                     char * user = strstr(payload, "USER=\"");
                     char * func = strstr(payload, "FUNC=");
                     //"extract" user
                     if (user != nullptr)
                     {
                        user += 6;
                        s = user;
                        char c;
                        while (((c = *s) != 0) && (c != '"')) ++s; //move forward until to the termnating quotation mark or the user definition
                        *s = 0; //set zero temination
                     }
                     //"extract" function
                     if (func != nullptr)
                     {
                        func += 5;
                        s = func;
                        char c;
                        while (((c = *s) != 0) && (c != ';')) ++s; //move forward until to the termnating semicolon of the function definition
                        *s = 0; //set zero temination
                     }

                     //now - find the respective command in the json
                     if ((user != nullptr) && (func != nullptr))
                     {
                        const char * cmd = _config_json_get_cmd(configJson, sender, func, user);
                        if (cmd != nullptr)
                        {
                           // puts(cmd);
                           system(cmd);
                        }
                     }
                  }
               }

               // printf("Message from %s: ", sender);
               // if (rx > 2) puts((char *)&buffer[2]); //debug only
            }
         }
      }
   }
}



static const char * _config_json_get_cmd(cJSON * configJson, const char * deviceIp, const char * functionTag, const char * cmdId)
{
   const cJSON * device;
   cJSON_ArrayForEach(device, configJson) //for each device/scanner object in the config array
   {
      //get IP
      const cJSON * ip = cJSON_GetObjectItemCaseSensitive(device, "ip");
      if ((ip == nullptr) || !cJSON_IsString(ip) || (strcmp(ip->valuestring, deviceIp) != 0))
      {
         continue; //invalid object or ip missmatch
      }

      //get function array
      const cJSON * function = cJSON_GetObjectItemCaseSensitive(device, functionTag);
      if ((function == nullptr) || !cJSON_IsArray(function))
      {
         continue; //invalid or function missmatch
      }
      const cJSON * registration;
      cJSON_ArrayForEach(registration, function) //for each registration for the respective function
      {
         const cJSON * id = cJSON_GetObjectItemCaseSensitive(registration, "id");
         if ((id == nullptr) || !cJSON_IsString(id) || (strcmp(id->valuestring, cmdId) != 0))
         {
            continue; //invalid object or id missmatch
         }
         const cJSON * cmd = cJSON_GetObjectItemCaseSensitive(registration, "cmd");
         if ((cmd == nullptr) || !cJSON_IsString(cmd))
         {
            break; //invalid object
         }
         return cmd->valuestring;
      }
   }
   //not found
   return nullptr;
}





static char * _cjson_read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0)
    {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char*)malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';


cleanup:
    if (file != NULL)
    {
        fclose(file);
    }

    return content;
}