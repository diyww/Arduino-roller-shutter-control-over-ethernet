#include <EtherCard.h>

#define STATIC 0  // enable or disable DHCP

#if STATIC
static byte myip[] = { 192, 168, 0, 77 }; // interface ip address
static byte gwip[] = { 192, 168, 0, 1 }; // gateway ip address
#endif

static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 }; // ethernet mac address

const int relaySwitchOnOff = 7;   // Relais 1 und 2 für nn/off
const int relaySwitchUpDown = 9;  // Relais 3 für rauf und runter
const int tasterPinUp = 4;        // Taste für rauf
const int tasterPinDown = 6;      // Taste für runter
const int tasterPinOff = 2;       // Taste für aus
const int ledPinUp = 3;           // Kontroll LED für rauf
const int ledPinDown = 5;         // Kontroll LED für runter

const int autoOffTime = 10 * 1000;  // Sekunden für das automatische abschalten
unsigned long timerStart;           // Startzeit für das automatische abschalten

byte Ethernet::buffer[500];         // tcp/ip send and receive buffer

const char page[] PROGMEM =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Retry-After: 600\r\n"
  "\r\n"
  "<html>"
  "<head>"
  "<title>SmartHome Basic</title>"
  "</head>"
  "<body>"
  "<a href=?rs=2>up</a><br>"
  "<a href=?rs=0>off</a><br>"
  "<a href=?rs=1>down</a><br>"
  "</body>"
  "</html>"
  ;

void setup() {
  Serial.begin(57600);
  Serial.println("Rolladensteuerung Setup");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println( "Failed to access Ethernet controller");

#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
#endif

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  pinMode(tasterPinUp, INPUT);
  pinMode(tasterPinDown, INPUT);
  pinMode(tasterPinOff, INPUT);

  pinMode(ledPinUp, OUTPUT);
  pinMode(ledPinDown, OUTPUT);
  pinMode(relaySwitchOnOff, OUTPUT);
  pinMode(relaySwitchUpDown, OUTPUT);

  switchStatus(0);
}

void switchStatus(int relayStatus) {
  if (((digitalRead(ledPinUp) == HIGH)) || ((digitalRead(ledPinDown) == HIGH))) {
    digitalWrite(ledPinDown, LOW);
    digitalWrite(ledPinUp, LOW);
    digitalWrite(relaySwitchOnOff, HIGH);
    digitalWrite(relaySwitchUpDown, HIGH);
    delay(1000);
  }

  switch (relayStatus) {
    case 2: {
        digitalWrite(ledPinUp, HIGH);
        digitalWrite(ledPinDown, LOW);
        digitalWrite(relaySwitchOnOff, LOW);
        digitalWrite(relaySwitchUpDown, LOW);
        timerStart = millis();
      }
      break;
    case 1: {
        digitalWrite(ledPinUp, LOW);
        digitalWrite(ledPinDown, HIGH);
        digitalWrite(relaySwitchOnOff, LOW);
        digitalWrite(relaySwitchUpDown, HIGH);
        timerStart = millis();
      }
      break;
    default: {
        digitalWrite(ledPinDown, LOW);
        digitalWrite(ledPinUp, LOW);
        digitalWrite(relaySwitchOnOff, HIGH);
        digitalWrite(relaySwitchUpDown, HIGH);
      }
      break;
  }
}

void loop() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos) {
    if (strstr((char *)Ethernet::buffer + pos, "GET /?rs=2") != 0) {
      switchStatus(2);
    }
    else if (strstr((char *)Ethernet::buffer + pos, "GET /?rs=1") != 0) {
      switchStatus(1);
    }
    else if (strstr((char *)Ethernet::buffer + pos, "GET /?rs=0") != 0) {
      switchStatus(0);
    }

    memcpy_P(ether.tcpOffset(), page, sizeof page);
    ether.httpServerReply(sizeof page - 1);
  }

  if (digitalRead(tasterPinUp) == HIGH) {
    switchStatus(2);
  }
  else if (digitalRead(tasterPinDown) == HIGH) {
    switchStatus(1);
  }
  else if (digitalRead(tasterPinOff) == HIGH) {
    switchStatus(0);
  }

  if ((((digitalRead(ledPinUp) == HIGH)) || (digitalRead(ledPinDown) == HIGH)) && (millis() - timerStart > autoOffTime)) {
    switchStatus(0);
  }
}
