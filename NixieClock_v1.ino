/* Nixie Clock v1
  Custom Code and borrowed code from examples
*/
#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>
#include <Nixie.h>

//digital pins for Nixie
#define dataPin 5                                  // data or SER
#define clockPin 6                                 // clock or SCK
#define latchPin 7                                 // latch or RCK

#define numDigits 6                                // 6 digits for HR, MIN, SEC

Nixie nixie(dataPin, clockPin, latchPin);          // Create Nixie object and pass pin assignment

byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x0D, 0x3E, 0x9D }; 	   // mac address goes here
unsigned int localPort = 8888;		           // local port to listen for UDP packets
const int offsetHour = 6;
const long timeZoneOffset = (3600 * offsetHour);   // Time zone offset in seconds
IPAddress timeServer(132, 163, 4, 101);		   // time-a.timefreq.bldrdoc.gov NTP server
const int NTP_PACKET_SIZE= 48; 			   // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; 		   // buffer to hold incoming and outgoing packets 
EthernetUDP Udp;				   // A UDP instance to let us send and receive packets over UDP

time_t currentTime = 0;                            // Time object for current time

void setup()  {
  // Open serial communications and wait for port to open
  Serial.begin(9600);
  while(!Serial)  {
    ; //Wait for serial to connect
  }
  
  //start Ethernet and UDP
  if(Ethernet.begin(mac) == 0)  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // for loop of failure
    for(;;)
    ;
  }
  
  Udp.begin(localPort);                  // open local port for traffic
  setSyncProvider(getNTPTime);           // set time with getNTPTime function
  while(timeStatus() == timeNotSet)  {
    ;  //wait for time to set
  }
  
  nixie.clear(numDigits);                // clear the Nixie display
}

void loop()  {
  
  if(now() != currentTime)  {
    currentTime = now();
    Serial.print("Offset: ");
    Serial.println(timeZoneOffset);
    Serial.print("Hour: ");
    Serial.println(hour(currentTime));
    Serial.print("Minutes: ");
    Serial.println(minute(currentTime));
    Serial.print("Seconds: ");
    Serial.println(second(currentTime));
    Serial.println(currentTime);
    nixieWrite(second(currentTime));
    nixieWrite(minute(currentTime));
    nixieWrite(hour(currentTime));
  }
  
}

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)  {

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}

unsigned long getNTPTime()  {
  
  const long seventyYears = 2208988800 + timeZoneOffset;
  
  sendNTPpacket(timeServer);
  delay(1000);
  if(Udp.parsePacket())  {
    Udp.read(packetBuffer,NTP_PACKET_SIZE);          //Got a packet now read it out into the buffer
    
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    
    return secsSince1900 - seventyYears;              // Return the Epoch Time(Unix Time)
  }
  
  return 0;                                           // If no UDP packet, return 0
}


//function to add padding to single digits and write
void nixieWrite(int digits)  {
  if(digits < 10){
    nixie.writeNumLeft(digits);
    nixie.writeNumLeft(0);
  } else {
    nixie.writeNumLeft(digits);
  }
}
  
