void readEfisSerial()
{
if (readEfisData)
    {
    int packetCount=0;  
    if (efisType.substring(0,3)=="VN-") // VN-300 type data
      {
              // parse VN-300 and VN-100 data
             while (Serial3.available() && packetCount<EFIS_PACKET_SIZE)
                  {
                  // receive one line of data and break                                    
                  byte vn_inByte=Serial3.read();                  
                  lastReceivedEfisTime=millis();
                  packetCount++;
                  charsreceived++;
                  if (vn_inByte == 0xFA )
                      {
                      efisPacketInProgress=true;                     
                      vnBuffer[0]=0xFA;
                      vnBufferIndex=1; // reset buffer index when sync byte is received
                      continue;
                      //return;
                      }
                  if (vnBufferIndex<127 && efisPacketInProgress)
                      {
                      // add character to buffer
                      vnBuffer[vnBufferIndex]=vn_inByte;
                      vnBufferIndex++;
                      if (vnBufferIndex==127) // 103 without lat/lon
                          {
                          // got full packet, check header
                          //byte packetHeader[8]={0xFA,0x19,0xA0,0x01,0x91,0x00,0x42,0x01}; without lat/lon
                          byte packetHeader[8]={0xFA,0x19,0xE0,0x01,0x91,0x00,0x42,0x01}; // with lat/long
                          // groups (0x19): 0001 1001: group 1 (General Purpose Group), group 4 (GPS1 Measurement Group.), group 5 (INS Group.)
//                          FA19E00191004201
//                          E0 01: 1110 0000 , 0000 0001  (6,7,8,9)
//                          91 00: 1001 0001 , 0000 0000  (1,5,8)
//                          42 01: 0100 0010 , 0000 0001  (2,7,9)
                          
                          if (memcmp(vnBuffer,packetHeader,8)!=0)
                                {
                                // bad packet header, dump packet
                                efisPacketInProgress=false;
                                Serial.println("bad VN packet header");
                                continue;
                                //return;
                                }
                          // check CRC
                          uint16_t vnCrc = 0;
                          
                          //for (int i = 1; i < 103; i++) // starting after the sync byte
                          for (int i = 1; i < 127; i++) // starting after the sync byte
                              {                                
                                vnCrc = (uint16_t) (vnCrc >> 8) | (vnCrc << 8);
                            
                                vnCrc ^= (uint8_t) vnBuffer[i];
                                vnCrc ^= (uint16_t) (((uint8_t) (vnCrc & 0xFF)) >> 4);
                                vnCrc ^= (uint16_t) ((vnCrc << 8) << 4);
                                vnCrc ^= (uint16_t) (((vnCrc & 0xFF) << 4) << 1);
                              }
                           if (vnCrc!=0)
                                {
                                // bad CRC, dump packet  
                                efisPacketInProgress=false;
                                Serial.println("bad VN packet CRC");
                                continue;                                                                                               
                                //return;
                                }

                          // process packet data
//                          for (int i=0;i<vnBufferIndex;i++)
//                              {
//                              Serial.print(vnBuffer[i],HEX);
//                              Serial.print(" ");
//                              }
                             // common
                             vnAngularRateRoll=array2float(vnBuffer,8);
                             vnAngularRatePitch=array2float(vnBuffer,12);
                             vnAngularRateYaw=array2float(vnBuffer,16);

                             vnGnssLat=array2double(vnBuffer,20);
                             vnGnssLon=array2double(vnBuffer,28);
                             //vnGnssAlt=array2double(vnBuffer,36);
                             
                             vnVelNedNorth=array2float(vnBuffer,44);
                             vnVelNedEast=array2float(vnBuffer,48);
                             vnVelNedDown=array2float(vnBuffer,52);
                             
                             vnAccelFwd=array2float(vnBuffer,56);
                             vnAccelLat=array2float(vnBuffer,60);
                             vnAccelVert=array2float(vnBuffer,64);

                            
                             // gnss
                             //vnTimeUTC= vnBuffer,68 (+8 bytes);
                             //int8_t vnYear=int8_t(vnBuffer[68]);
                             //uint8_t vnMonth=uint8_t(vnBuffer[69]);
                             //uint8_t vnDay=uint8_t(vnBuffer[70]);
                             uint8_t vnHour=uint8_t(vnBuffer[71]);
                             uint8_t vnMin=uint8_t(vnBuffer[72]);
                             uint8_t vnSec=uint8_t(vnBuffer[73]);                             
                             //uint16_t vnFracSec=(vnBuffer[75] << 8) | vnBuffer[74]; // gps fractional seconds only update at GPS update rates, 5Hz. We'll calculate our own
                             // calculate fractional seconds 1/100
                             String vnFracSec=String(int(millis()/10));
                             vnFracSec=vnFracSec.substring(vnFracSec.length()-2);
                             vnTimeUTC=String(vnHour)+":"+String(vnMin)+":"+String(vnSec)+"."+vnFracSec;
                             vnGPSFix=vnBuffer[76];
                             vnGnssVelNedNorth=array2float(vnBuffer,77);
                             vnGnssVelNedEast=array2float(vnBuffer,81);
                             vnGnssVelNedDown=array2float(vnBuffer,85);
                             
                             
                             // attitude
                             vnYaw=array2float(vnBuffer,89);
                             vnPitch=array2float(vnBuffer,93);
                             vnRoll=array2float(vnBuffer,97);
                             
                             vnLinAccFwd=array2float(vnBuffer,101);
                             vnLinAccLat=array2float(vnBuffer,105);
                             vnLinAccVert=array2float(vnBuffer,109);
                             
                             vnYawSigma=array2float(vnBuffer,113);
                             vnRollSigma=array2float(vnBuffer,117);
                             vnPitchSigma=array2float(vnBuffer,121);
                             
                             efisTimestamp=millis();
                             
                             #ifdef EFISDATADEBUG
                             Serial.printf("\nvnAngularRateRoll: %.2f,vnAngularRatePitch: %.2f,vnAngularRateYaw: %.2f,vnVelNedNorth: %.2f,vnVelNedEast: %.2f,vnVelNedDown: %.2f,vnAccelFwd: %.2f,vnAccelLat: %.2f,vnAccelVert: %.2f,vnYaw: %.2f,vnPitch: %.2f,vnRoll: %.2f,vnLinAccFwd: %.2f,vnLinAccLat: %.2f,vnLinAccVert: %.2f,vnYawSigma: %.2f,vnRollSigma: %.2f,vnPitchSigma: %.2f,vnGnssVelNedNorth: %.2f,vnGnssVelNedEast: %.2f,vnGnssVelNedDown: %.2f,vnGnssLat: %.6f,vnGnssLon: %.6f,vnGPSFix: %i,TimeUTC: %s\n",vnAngularRateRoll,vnAngularRatePitch,vnAngularRateYaw,vnVelNedNorth,vnVelNedEast,vnVelNedDown,vnAccelFwd,vnAccelLat,vnAccelVert,vnYaw,vnPitch,vnRoll,vnLinAccFwd,vnLinAccLat,vnLinAccVert,vnYawSigma,vnRollSigma,vnPitchSigma,vnGnssVelNedNorth,vnGnssVelNedEast,vnGnssVelNedDown,vnGnssLat,vnGnssLon,vnGPSFix,vnTimeUTC.c_str());
                             #endif
                          efisPacketInProgress=false;
                          break; // exit the loop when a packet is completed
                          }                        
                      }; // else if buffer overflow don't do anything  
                 } // while
       }  else
                    {                    
                    //read EFIS data
                    int efisCharCount=0;
                     while (Serial3.available()>0 && packetCount<EFIS_PACKET_SIZE)
                        {
                                                   
                        efisCharCount++;
                        efisMaxAvailable=max(Serial3.available(),efisMaxAvailable);
                        efis_inChar=Serial3.read();
                        lastReceivedEfisTime=millis();
                        packetCount++; 
                        charsreceived++;      
                        if  (efisBufferString.length()>=230)
                            {
                            efisBufferString=""; // prevent buffer overflow;
                            Serial.printf("Efis data buffer overflow");
                            }
                        if ((efisBufferString.length()>0 || last_efis_inChar== char(0x0A)))  // data line terminats with 0D0A, when buffer is empty look for 0A in the incoming stream and dump everything else
                            {            
                            efisBufferString+=efis_inChar;                               
                            if (efis_inChar == char(0x0A))
                                        {
                                        // end of line
                  if (efisType=="ADVANCED")
                      {
                        #ifdef EFISDATADEBUG
                        int lineLength=efisBufferString.length();
                        if (lineLength!=74 && lineLength!=93 && lineLength!=225)
                            {
                             Serial.print("Invalid Efis data line length: ");
                             Serial.println(lineLength);
                            }
                        #endif
                               
                                        if (efisBufferString.length()==74 && efisBufferString[0]=='!' && efisBufferString[1]=='1')
                                           {
                                           // parse Skyview AHRS data                            
                                           String parseString;
                                           //calculate CRC
                                           int calcCRC=0;
                                           for (int i=0;i<=69;i++) calcCRC+=efisBufferString[i];                     
                                           calcCRC=calcCRC & 0xFF;
                                           if (calcCRC==(int)strtol(&efisBufferString.substring(70, 72)[0],NULL,16)) // convert from hex back into integer for camparison, issue with missing leading zeros when comparing hex formats
                                              {                                                
                                               //float efisIAS                            
                                               parseString=efisBufferString.substring(23, 27); 
                                               if (parseString!="XXXX") efisIAS=parseString.toFloat()/10; else efisIAS=-1; // knots
                                               //float efisPitch                             
                                               parseString=efisBufferString.substring(11, 15);
                                               if (parseString!="XXXX") efisPitch=parseString.toFloat()/10; else efisPitch=-100; // degrees
                                               // float efisRoll
                                               parseString=efisBufferString.substring(15, 20);
                                               if (parseString!="XXXXX") efisRoll=parseString.toFloat()/10; else efisRoll=-100; // degrees
                                              // float MagneticHeading                                   
                                               parseString=efisBufferString.substring(20, 23);
                                               if (parseString!="XXX") efisHeading=parseString.toInt(); else efisHeading=-1;                             
                                               // float efisLateralG
                                               parseString=efisBufferString.substring(37, 40);
                                               if (parseString!="XXX") efisLateralG=parseString.toFloat()/100; else efisLateralG=-100;
                                               // float efisVerticalG                                   
                                               parseString=efisBufferString.substring(40, 43);
                                               if (parseString!="XXX") efisVerticalG=parseString.toFloat()/10; else efisVerticalG=-100;
                                               // int efisPercentLift                 
                                               parseString=efisBufferString.substring(43, 45);
                                               if (parseString!="XX") efisPercentLift=parseString.toInt(); else efisPercentLift=-1; // 00 to 99, percentage of stall angle.
                                               // int efisPalt
                                               parseString=efisBufferString.substring(27, 33); 
                                               if (parseString!="XXXXXX") efisPalt=parseString.toInt(); else efisPalt=-10000; // feet
                                               // int efisVSI                 
                                               parseString=efisBufferString.substring(45, 49); 
                                               if (parseString!="XXXX") efisVSI=parseString.toInt()*10; else efisVSI=-10000; // feet/min 
                                               //float efisTAS;
                                               parseString=efisBufferString.substring(52, 56);
                                               if (parseString!="XXXX") efisTAS=parseString.toFloat()/10; else efisTAS=-1; // kts             
                                               //float efisOAT;
                                               parseString=efisBufferString.substring(49, 52);
                                               if (parseString!="XXX") efisOAT=parseString.toFloat(); else efisTAS=-100; // Celsius
                                               // String efisTime
                                               efisTime=efisBufferString.substring(3, 5)+":"+efisBufferString.substring(5, 7)+":"+efisBufferString.substring(7, 9)+"."+efisBufferString.substring(9, 11);                             
                                               efisTimestamp=millis();
                                               #ifdef EFISDATADEBUG
                                               Serial.printf("SKYVIEW ADAHRS: efisIAS %.2f, efisPitch %.2f, efisRoll %.2f, efisLateralG %.2f, efisVerticalG %.2f, efisPercentLift %i, efisPalt %i, efisVSI %i, efisTAS %.2f, efisOAT %.2f, efisHeading %i ,efisTime %s\n", efisIAS, efisPitch, efisRoll, efisLateralG, efisVerticalG, efisPercentLift,efisPalt,efisVSI,efisTAS,efisOAT,efisHeading, efisTime.c_str());                        
                                               #endif
                                               #ifdef SDCARDDEBUG
                                               Serial.print(".");
                                               #endif
                                                                                              
                                              } else Serial.println("AHRS CRC FAIL");
                                              #ifdef EFISDATADEBUG
                                              else Serial.println("SKYVIEW ADAHRS CRC Failed");
                                              #endif
                                            break; // exit the loop when a packet is completed   
                                           } else
                                           
                                           if (efisBufferString.length()==225 && efisBufferString[0]=='!' && efisBufferString[1]=='3')
                                                   {
                                                   // parse Skyview EMS data
                                                   String parseString;
                                                   //calculate CRC
                                                   int calcCRC=0;
                                                   for (int i=0;i<=220;i++) calcCRC+=efisBufferString[i];
                                                   calcCRC=calcCRC & 0xFF;
                                                   if (calcCRC==(int)strtol(&efisBufferString.substring(221, 223)[0],NULL,16)) // convert from hex back into integer for camparison, issue with missing leading zeros when comparing hex formats
                                                      {
                                                      //float efisFuelRemaining=0.00;
                                                       parseString=efisBufferString.substring(44, 47);
                                                       if (parseString!="XXX") efisFuelRemaining=parseString.toFloat()/10; else efisFuelRemaining=-1; // gallons
                                                       //float efisFuelFlow=0.00;
                                                       parseString=efisBufferString.substring(29, 32);
                                                       if (parseString!="XXX") efisFuelFlow=parseString.toFloat()/10; else efisFuelFlow=-1; // gph
                                                       //float efisMAP=0.00;
                                                       parseString=efisBufferString.substring(26, 29);
                                                       if (parseString!="XXX") efisMAP=parseString.toFloat()/10; else efisMAP=-1; //inHg
                                                       // int efisRPM=0;
                                                       parseString=efisBufferString.substring(18, 22);
                                                       if (parseString!="XXXX") efisRPM=parseString.toInt(); else efisRPM=-1;
                                                       // int efisPercentPower=0;                                     
                                                       parseString=efisBufferString.substring(217, 220);
                                                       if (parseString!="XXX") efisPercentPower=parseString.toInt(); else efisPercentPower=-1;                                                            
                                                       #ifdef EFISDATADEBUG
                                                       Serial.printf("SKYVIEW EMS: efisFuelRemaining %.2f, efisFuelFlow %.2f, efisMAP %.2f, efisRPM %i, efisPercentPower %i\n", efisFuelRemaining, efisFuelFlow, efisMAP, efisRPM, efisPercentPower);                                     
                                                       #endif
                                                       #ifdef SDCARDDEBUG
                                                      Serial.print(".");
                                                      #endif
                                                                           
                                                      } else Serial.println("EMS CRC Fail");
                                                      #ifdef EFISDATADEBUG
                                                      else Serial.println("SKYVIEW EMS CRC Failed");
                                                      #endif
                                                      
                                                    break; // exit the loop when a packet is completed
                                                   }
                      } // end efisType ADVANCED
                      else
                        if (efisType=="DYNOND10") 
                           {             
                                                  if (efisBufferString.length()==DYNON_SERIAL_LEN)
                                                      {
                                                      // parse Dynon data
                                                       String parseString;
                                                       //calculate CRC
                                                       int calcCRC=0;
                                                       for (int i=0;i<=48;i++) calcCRC+=efisBufferString[i];                     
                                                       calcCRC=calcCRC & 0xFF;
                                                       if (calcCRC==(int)strtol(&efisBufferString.substring(49, 51)[0],NULL,16)) // convert from hex back into integer for camparison, issue with missing leading zeros when comparing hex formats
                                                          {
                                                          // CRC passed
                                                           parseString=efisBufferString.substring(20, 24);                                                                                                      
                                                           efisIAS=parseString.toFloat()/10*1.94384; // m/s to knots
                                                           parseString=efisBufferString.substring(8, 12);
                                                           efisPitch=parseString.toFloat()/10;
                                                           parseString=efisBufferString.substring(12, 17);
                                                           efisRoll=parseString.toFloat()/10;
                                                           parseString=efisBufferString.substring(33, 36);
                                                           efisLateralG=parseString.toFloat()/100;
                                                           parseString=efisBufferString.substring(36, 39);
                                                           efisVerticalG=parseString.toFloat()/10;
                                                           parseString=efisBufferString.substring(39, 41);
                                                           efisPercentLift=parseString.toInt(); // 00 to 99, percentage of stall angle
                                                           parseString=efisBufferString.substring(45,47);
                                                           long statusBitInt = strtol(&parseString[1], NULL, 16);                                                                                 
                                                           if (bitRead(statusBitInt, 0))
                                                                {
                                                                // when bitmask bit 0 is 1, grab pressure altitude and VSI, otherwise use previous value (skip turn rate and density altitude)
                                                                parseString=efisBufferString.substring(24, 29);
                                                                efisPalt=parseString.toInt()*3.28084; // meters to feet
                                                                parseString=efisBufferString.substring(29, 33);
                                                                efisVSI= int(parseString.toFloat()/10*60); // feet/sec to feet/min
                                                                }                                                               
                                                           efisTimestamp=millis();
                                                           efisTime=efisBufferString.substring(0, 2)+":"+efisBufferString.substring(2, 4)+":"+efisBufferString.substring(4, 6)+"."+efisBufferString.substring(6, 8);                                        
                                                           #ifdef EFISDATADEBUG                     
                                                           Serial.printf("D10: efisIAS %.2f, efisPitch %.2f, efisRoll %.2f, efisLateralG %.2f, efisVerticalG %.2f, efisPercentLift %i, efisPalt %i, efisVSI %i, efisTime %s\n", efisIAS, efisPitch, efisRoll, efisLateralG, efisVerticalG, efisPercentLift,efisPalt,efisVSI,efisTime.c_str());                                                                 
                                                           #endif
                                                           #ifdef SDCARDDEBUG
                                                           Serial.print(".");
                                                           #endif                                         
                                                          }
                                                          #ifdef EFISDATADEBUG
                                                          else Serial.println("D10 CRC Failed");
                                                          #endif
                                                        break; // exit the loop when a packet is completed
                                                        }
                           } // end efisType DYNOND10
                           else
                              if (efisType=="GARMING5")
                                  {
                                                          
                                                        
                                                        if (efisBufferString.length()==59 && efisBufferString[0]=='=' && efisBufferString[1]=='1' && efisBufferString[2]=='1')
                                                                   {
                                                                   // parse G5 data
                                                                   String parseString;
                                                                   //calculate CRC
                                                                   int calcCRC=0;                           
                                                                   for (int i=0;i<=54;i++) calcCRC+=efisBufferString[i];                     
                                                                   calcCRC=calcCRC & 0xFF;
                                                                   if (calcCRC==(int)strtol(&efisBufferString.substring(55, 57)[0],NULL,16)) // convert from hex back into integer for camparison, issue with missing leading zeros when comparing hex formats
                                                                      {
                                                                        // CRC passed                                                                                                          
                                                                       parseString=efisBufferString.substring(23, 27);
                                                                       if (parseString!="____") efisIAS=parseString.toFloat()/10;
                                                                       parseString=efisBufferString.substring(11, 15);
                                                                       if (parseString!="____") efisPitch=parseString.toFloat()/10;
                                                                       parseString=efisBufferString.substring(15, 20);
                                                                       if (parseString!="_____") efisRoll=parseString.toFloat()/10;                                                    
                                                                       parseString=efisBufferString.substring(20, 23);
                                                                       if (parseString!="___") efisHeading=parseString.toInt();
                                                                       parseString=efisBufferString.substring(37, 40);
                                                                       if (parseString!="___") efisLateralG=parseString.toFloat()/100;                            
                                                                       parseString=efisBufferString.substring(40, 43);
                                                                       if (parseString!="___") efisVerticalG=parseString.toFloat()/10;
                                                                       parseString=efisBufferString.substring(27, 33);
                                                                       if (parseString!="______") efisPalt=parseString.toInt(); // feet
                                                                       parseString=efisBufferString.substring(45, 49);
                                                                       if (parseString!="____") efisVSI=parseString.toInt()*10; //10 fpm
                                                                       efisTimestamp=millis();
                                                                       efisTime=efisBufferString.substring(3, 5)+":"+efisBufferString.substring(5, 7)+":"+efisBufferString.substring(7, 9)+"."+efisBufferString.substring(9, 11);   
                                                                       #ifdef EFISDATADEBUG
                                                                       Serial.printf("G5 data: efisIAS %.2f, efisPitch %.2f, efisRoll %.2f, efisHeading %i, efisLateralG %.2f, efisVerticalG %.2f, efisPalt %i, efisVSI %i,efisTime %s", efisIAS, efisPitch, efisRoll, efisHeading, efisLateralG, efisVerticalG,efisPalt,efisVSI,efisTime.c_str());                        
                                                                       Serial.println();
                                                                       #endif
                                                                       #ifdef SDCARDDEBUG
                                                                       Serial.print(".");
                                                                       #endif                                                      
                                                                       }
                                                                       #ifdef EFISDATADEBUG
                                                                       else Serial.println("G5 CRC Failed");
                                                                       #endif  
                                                                     break; // exit the loop when a packet is completed                                                                              
                                                                    }
                                  } // efisType GARMING5
                                  else
                                   if (efisType=="GARMING3X")
                                   {
                                                                  
                                                        // parse G3X attitude data, 10hz
                                                        if (efisBufferString.length()==59 && efisBufferString[0]=='=' && efisBufferString[1]=='1' && efisBufferString[2]=='1')
                                                                   {
                                                                   // parse G3X data
                                                                   String parseString;
                                                                   //calculate CRC
                                                                   int calcCRC=0;                           
                                                                   for (int i=0;i<=54;i++) calcCRC+=efisBufferString[i];                     
                                                                   calcCRC=calcCRC & 0xFF;
                                                                   if (calcCRC==(int)strtol(&efisBufferString.substring(55, 57)[0],NULL,16)) // convert from hex back into integer for camparison, issue with missing leading zeros when comparing hex formats
                                                                      {
                                                                      // CRC passed                                                                                                          
                                                                       parseString=efisBufferString.substring(23, 27);
                                                                       if (parseString!="____") efisIAS=parseString.toFloat()/10;
                                                                       parseString=efisBufferString.substring(11, 15);
                                                                       if (parseString!="____") efisPitch=parseString.toFloat()/10;
                                                                       parseString=efisBufferString.substring(15, 20);
                                                                       if (parseString!="_____") efisRoll=parseString.toFloat()/10;                                                    
                                                                       parseString=efisBufferString.substring(20, 23);
                                                                       if (parseString!="___") efisHeading=parseString.toInt();
                                                                       parseString=efisBufferString.substring(37, 40);
                                                                       if (parseString!="___") efisLateralG=parseString.toFloat()/100;
                                                                       parseString=efisBufferString.substring(40, 43);
                                                                       if (parseString!="___") efisVerticalG=parseString.toFloat()/10;
                                                                       parseString=efisBufferString.substring(43, 45);
                                                                       if (parseString!="__") efisPercentLift=parseString.toInt();
                                                                       parseString=efisBufferString.substring(27, 33);
                                                                       if (parseString!="______") efisPalt=parseString.toInt(); // feet
                                                                       parseString=efisBufferString.substring(49, 52);
                                                                       if (parseString!="___") efisOAT=parseString.toInt(); 
                                                                       parseString=efisBufferString.substring(45, 49); // celsius
                                                                       if (parseString!="____") efisVSI=parseString.toInt()*10; //10 fpm                                                     
                                                                       efisTimestamp=millis();                                                       
                                                                       efisTime=efisBufferString.substring(3, 5)+":"+efisBufferString.substring(5, 7)+":"+efisBufferString.substring(7, 9)+"."+efisBufferString.substring(9, 11);                                        
                                                                       #ifdef EFISDATADEBUG
                                                                       Serial.printf("G3X Attitude data: efisIAS %.2f, efisPitch %.2f, efisRoll %.2f, efisHeading %i, efisLateralG %.2f, efisVerticalG %.2f, efisPercentLift %i, efisPalt %i, efisVSI %i,efisTime %s", efisIAS, efisPitch, efisRoll, efisHeading, efisLateralG, efisVerticalG, efisPercentLift,efisPalt,efisVSI,efisTime.c_str());                        
                                                                       Serial.println();
                                                                       #endif
                                                                       #ifdef SDCARDDEBUG
                                                                       Serial.print(".");
                                                                       #endif                                                      
                                                                       }
                                                                       #ifdef EFISDATADEBUG
                                                                       else Serial.println("G3X Attitude CRC Failed");
                                                                       #endif                                                                             
                                                                     break; // exit the loop when a packet is completed
                                                                    }
                                                             // parse G3X engine data, 5Hz
                                                        if (efisBufferString.length()==221 && efisBufferString[0]=='=' && efisBufferString[1]=='3' && efisBufferString[2]=='1') 
                                                        {                                                                                          
                                                                   String parseString;
                                                                   //calculate CRC
                                                                   int calcCRC=0;                           
                                                                   for (int i=0;i<=216;i++) calcCRC+=efisBufferString[i];
                                                                   calcCRC=calcCRC & 0xFF;
                                                                   if (calcCRC==(int)strtol(&efisBufferString.substring(217, 219)[0],NULL,16)) // convert from hex back into integer for camparison, issue with missing leading zeros when comparing hex formats
                                                                      { 
                                                                      //float efisFuelRemaining=0.00;
                                                                      parseString=efisBufferString.substring(44, 47);
                                                                      if (parseString!="___") efisFuelRemaining=parseString.toFloat()/10;
                                                                      parseString=efisBufferString.substring(29, 32);
                                                                      if (parseString!="___") efisFuelFlow=parseString.toFloat()/10;                                                    
                                                                      parseString=efisBufferString.substring(26, 29);
                                                                      if (parseString!="___") efisMAP=parseString.toFloat()/10;
                                                                      parseString=efisBufferString.substring(18, 22);
                                                                      if (parseString!="____") efisRPM=parseString.toInt();                                                    
                                                                      #ifdef EFISDATADEBUG
                                                                      Serial.printf("G3X EMS: efisFuelRemaining %.2f, efisFuelFlow %.2f, efisMAP %.2f, efisRPM %i\n", efisFuelRemaining, efisFuelFlow, efisMAP, efisRPM);                                     
                                                                      #endif
                                                                      #ifdef SDCARDDEBUG
                                                                      Serial.print(".");
                                                                      #endif
                                                                      }
                                                                       #ifdef EFISDATADEBUG
                                                                       else Serial.println("G3X EMS CRC Failed");
                                                                       #endif
                                                         break; // exit the loop when a packet is completed                                                    
                                                        }
                                                                    
                      } // end efisType GARMING3X
                                        efisBufferString="";  // reset buffer
                                        }
                          } // 0x0A first
                           #ifdef EFISDATADEBUG
                           else
                                {
                                // show dropped characters  
                                Serial.print("@");
                                Serial.print(efis_inChar);
                                }         
                           #endif // efisdatadebug
                          last_efis_inChar=efis_inChar;
                        }
                  } // read efis type data
                
} // readEfisData
}
