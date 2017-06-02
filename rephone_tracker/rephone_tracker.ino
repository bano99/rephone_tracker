#include <LCFile.h>
#include <LBattery.h>
#include <LGSM.h>
#include <LGPS.h>
#include <stdio.h>
#include "vmpwr.h"
#include "vmgsm_sim.h"

// Bluetooth
#include <LBT.h>
#include <LBTServer.h>
#define SPP_SVR "Rocket_BLE" // it is the server's visible name, customize it yourself.
#define ard_log Serial.printf

#define PHONENR  "004367683839397"

/*
    GPS Tracker
    By Denis Banovic @ 30-5-2016
    
   This program is for the Traceable Dog Collar Application that is 
   built with Xadow GSM+BLE, Xadow Audio and Xadow GPS v2. 
   Functions achieved:
   1. Your RePhone will automatically  reply with an SMS when you call him or send him an SMS
   
   for problems with reset, see this:
   http://www.seeed.cc/RePhone-GSM%2BBLE-reset-t-5971.html
   
   Aktueller Status: Das Problem das sich das Gerät nicht mit GSM verbindet lässt sich irgendwie 
   nicht mehr nachvollziehen, da zumindest laut blootooth es immer funktioniert
   
   
*/


char num[20] = {0};

char buffer[160] = {0,};
int sendsms = 0;    

int sent_ok = 0; // did I sent a Initial SMS OK
int loopcounter = 0;

//Set up
void setup(){
    Serial.begin(115200);
    Serial.print("\r\n");
//    LFile.Create("gps_log.txt");
    bt_setup(); // run bluetooth setup code!
}

//set a loop
void loop() {


    Serial.print("Dog Test\r\n"); 
    
    if(LVoiceCall.getVoiceCallStatus() == RECEIVINGCALL) {              // if receives an incoming call
      
        LVoiceCall.retrieveCallingNumber(num,20);
        sprintf(buffer, "Call come in, number is %s \r\n", num);
        Serial.println(buffer);
//        LVoiceCall.answerCall();                                      // pick up the phone call
        sendsms = 1;
        delay(3000);   
        Serial.println("HangUp after 3 Seconds");
        LVoiceCall.hangCall();
    }
    
    if(LSMS.available()) {                                               // if received a SMS from another cellphone
        LSMS.remoteNumber(num, 20); 
//        LSMS.remoteContent(buf_contex, 50);
//        sprintf(buffer, "Get new sms, content: %s, number: %s \r\n", buf_contex, num);
        Serial.println("Got SMS, going to answer");
        sendsms = 1;
    }
    if(sendsms){
      if(get_position()){
        if(send_position()){
          sendsms = 0;
        }
      }
    }
    
    
    // sobald gsm und gps online, einmalig SMS schicken
    if(sent_ok < 4 && LVoiceCall.ready()){
      if((LGPS.get_position_fix() > 0) && get_position()){
        if(LGPS.get_sate_used() > 0 ) {
          if(sent_ok ==3) {
            if(send_ok()){
               sent_ok = 5;
            }
          } else {
             delay(2000); // we want to have a fix for at least 3x in 6 seconds!
             sent_ok++;
          }
        }
      }
    }
    
    delay(100);

    loopcounter++;
    
    if(!(loopcounter % 150 )) { // every 15 seconds write GPS into a file
        char bt_update[100] = {0,};

// delivers bullshit too!
//   signed char network_buffer[100] = {0,};
//   int network_found = 1;     
//   network_found = vm_gsm_sim_get_network_plmn(vm_gsm_sim_get_active_sim_card(),network_buffer, 100);


        
//        VM_GSM_SIM_STATUS status; // provides some bullshit info
//        status = vm_gsm_sim_get_card_status(vm_gsm_sim_get_active_sim_card());
        
//        VM_GSM_SIM_ID = vm_gsm_sim_get_active_sim_card();
//        sprintf(bt_update, "GSM_%d_%d_GPS_%c_%c", LVoiceCall.ready(), (char*)status, LGPS.get_position_fix(), LGPS.get_sate_used());
//        sprintf(bt_update, "GSM_%d_%d_GPS_%c_%c", LVoiceCall.ready(), network_found, LGPS.get_position_fix(), LGPS.get_sate_used());
        sprintf(bt_update, "GSM_%d_GPS_%c_%c", LVoiceCall.ready(), LGPS.get_position_fix(), LGPS.get_sate_used());
        restart_bt_server(bt_update);
    }
    
    if(!(loopcounter % 300 )) { // every 30 seconds write GPS into a file
      Serial.println("Getting Position for log-file");
          if(get_position()){
          Serial.println("debug-11");
          char writedata[40] = {0};
          
          unsigned char sysTime = 0;
          Serial.println("debug-12");
//          LFile.Write("gps_log.txt", buffer);
          Serial.println("log-file written");
      }
    }
    
    
     if(!(loopcounter % 600 ) && !(LVoiceCall.ready())) { // every 60 check if we have GSM, if not, restart
        
/*  Diese Sequenz funktioniert derzeit nicht wirklich so wie es sollte, da wm_pwr_reboot nicht wirklich das ganze neu startet */
        vm_pwr_reboot(); // Startet nicht wirklich was neu, zumindest nicht so das es wahrnehmbar ist
//     vm_pwr_shutdown(2);
        char bt_update[100] = {0,};
        sprintf(bt_update, "restarted_GSM");
        restart_bt_server(bt_update);
        delay(30000);

        loopcounter=1;

    }
    
}

int send_position(){

  if(LSMS.ready()) {
          Serial.println("Sms available");
          LSMS.beginSMS(num);
          Serial.println(num);          
          LSMS.print(buffer);
          Serial.println(buffer);                    
          if(LSMS.endSMS()){ 
            Serial.println("SMS sent ok!");
            delay(10000);   // wait 10 seconds before receiving another command
            return 1;
          }  else {
            Serial.println("SMS send fail!");
          }
   } else {
     Serial.println("SMS no ready!");
   }
   
   return 0;
  
}

int send_ok(){
  
  if(LSMS.ready()) {
          LSMS.beginSMS(PHONENR);
          LSMS.print(buffer);
          Serial.println(buffer);                    
          if(LSMS.endSMS()){ 
            Serial.println("SMS sent ok!");
            delay(10000);   // wait 10 seconds before receiving another command
            return 1;
          }  else {
            Serial.println("SMS send fail!");
            return 0;
          }
   } else {
     Serial.println("SMS no ready!");
   }
   
   return 0;
  
}

int get_position(){

  unsigned char *utc_date_time = 0;  
  unsigned int batteryLlevel;
  int batterycharging = 0;
  Serial.println("debug-1");
  if(LBattery.isCharging()){
    batterycharging = 1;
  }
  
  batteryLlevel = LBattery.level();
  Serial.println("debug-");
  
  if(LGPS.check_online()) {
      utc_date_time = LGPS.get_utc_date_time();
      sprintf(buffer, "UTC:%d-%d-%d %d:%d:%d,\r\n%c:%f,\r\n%c:%f,\r\nALT:%f,\r\nSpeed:%f,\r\nAkku:%d, Chrging: %d, fix:%c, sat:%c", 
                       utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4],utc_date_time[5], 
                       LGPS.get_ns(), LGPS.get_latitude(), LGPS.get_ew(), LGPS.get_longitude(), LGPS.get_altitude(), LGPS.get_speed(), batteryLlevel, batterycharging, LGPS.get_position_fix(), LGPS.get_sate_used());
//      Serial.println(buffer);
      Serial.println("debug-2");

      return 1;
  } else { // no GPS Lock yet, wait till lock comes
    return 0;
  }
    Serial.println("debug-3");
 
}

// ################################################### Bluetooth code

/*
  Copyright (c) 2014 MediaTek Inc.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License..

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
   See the GNU Lesser General Public License for more details.
*/

void restart_bt_server(char* server_name){
  LBTServer.end();
  delay(100);
  bool success = LBTServer.begin((uint8_t*) server_name);
  if( !success ) {
      ard_log("Cannot begin Bluetooth Server successfully\n");
  } else {
      LBTDeviceInfo info;
      if (LBTServer.getHostDeviceInfo(&info)) {
          ard_log("LBTServer.getHostDeviceInfo [%02x:%02x:%02x:%02x:%02x:%02x]", 
            info.address.nap[1], info.address.nap[0], info.address.uap, info.address.lap[2], info.address.lap[1], info.address.lap[0]);
      } else {
          ard_log("LBTServer.getHostDeviceInfo failed\n");
      }
      
      ard_log("Bluetooth Server begin successfully\n");
  }
 
  // waiting for Spp Client to connect
  bool connected = LBTServer.accept(1800);
 
  if( !connected ) {
      ard_log("No connection request yet\n");
      // don't do anything else
      delay(0xffffffff);
  } else {
      ard_log("Connected\n");
  }
  
  
}

void bt_setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  ard_log("start BTS\n");

  bool success = LBTServer.begin((uint8_t*)SPP_SVR);
  if( !success )
  {
      ard_log("Cannot begin Bluetooth Server successfully\n");
      // don't do anything else
      delay(0xffffffff);
  }
  else
  {
      LBTDeviceInfo info;
      if (LBTServer.getHostDeviceInfo(&info))
      {
          ard_log("LBTServer.getHostDeviceInfo [%02x:%02x:%02x:%02x:%02x:%02x]", 
            info.address.nap[1], info.address.nap[0], info.address.uap, info.address.lap[2], info.address.lap[1], info.address.lap[0]);
      }
      else
      {
          ard_log("LBTServer.getHostDeviceInfo failed\n");
      }
      ard_log("Bluetooth Server begin successfully\n");
  }
 
  // waiting for Spp Client to connect
  bool connected = LBTServer.accept(1800);
 
  if( !connected )
  {
      ard_log("No connection request yet\n");
      // don't do anything else
      delay(0xffffffff);
  }
  else
  {
      ard_log("Connected\n");
  }
}

int sent = 0;
int read_size = 0;


void bt_loop() {
  // put your main code here, to run repeatedly:
  if (!sent)
  {
      char buffer[32] = {0};
      //int read_size = LBTServer.read((uint8_t*)buffer, 32);
      //vm_log_info("spec read size [%d][%s]", read_size);
      
      char* buffer_w = "LinkIt BT Server";
      int write_size = LBTServer.write((uint8_t*)buffer_w, strlen(buffer_w));
      ard_log("write_size [%d]", write_size);
      

      memset(buffer, 0, sizeof(buffer));
      while(1)
      {
        if(LBTServer.available())
        {
          read_size = LBTServer.readBytes((uint8_t*)buffer, 32);
          break;
        }
        delay(100);
      }
      ard_log("read size [%d][%s]", read_size, buffer);

      memset(buffer, 0, sizeof(buffer));
      while(1)
      {
        if(LBTServer.available())
        {
          read_size = LBTServer.readBytes((uint8_t*)buffer, 32);
          break;
        }
        delay(100);
      }
      ard_log("read size [%d][%s]", read_size, buffer);

    
      sent = 1;
  }
  ard_log("loop server\n");
//  delay(2000);
}



