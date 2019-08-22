
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  SerialUSB.begin(9600);
  delay(3000);
  SerialUSB.println("NDEF before1"); 
  SerialUSB.println("NDEF before2");
  nfc.begin();  
  SerialUSB.println("NDEF before3");
#if ARDUINO_ARCH_AVR
    SerialUSB.println("\nDARDUINO_ARCH_AVR\n");
#endif

#if ARDUINO_ARCH_SAMD
    SerialUSB.println("\nDARDUINO_ARCH_SAMD\n");
#endif
}

void loop(void) {

    //SerialUSB.println("\nScan a NFC tag\n");
    SerialUSB.println("\nScan a NFC tag112\n");
    if (nfc.tagPresent())
    {
      // SerialUSB.println("\n read\n");
        NfcTag tag = nfc.read();
        tag.print();
    }
    delay(1000);  
}
