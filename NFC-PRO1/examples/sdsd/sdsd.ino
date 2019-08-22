#include "PN532_HSU.h"
#include "PN532.h"
#include "NfcAdapter.h"
#include "Arduino.h"

PN532_HSU interface(Serial);
NfcAdapter nfc = NfcAdapter(interface);

void setup(void) {
     SerialUSB.begin(115200);

     delay(3000);
     SerialUSB.println("uart Reader1");
     SerialUSB.println("uart Reader2");
    nfc.begin();
    SerialUSB.println("uart Reader3");
}

void loop(void) {
    SerialUSB.println("\nuart Scan a NFC tag\n");
  if (nfc.tagPresent())
    {
      SerialUSB.println("\n  read---------------- \n");
        NfcTag tag = nfc.read();
        tag.print();
    }
    delay(5000);
}
