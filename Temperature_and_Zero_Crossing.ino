#include <OneWire.h>

#define loadR 4
volatile int power = 25;
volatile unsigned long t, t2, time;

OneWire ds(10); // on pin 10

void zero_crosss_int()
{
  // Trigger angle calculation: 60Hz‐> 8.33ms (1/2 cycle)
  // (8333us ‐ 8.33us) / 256 = 32 (aprox)
  int powertime = (32 * (256 - power));

  // Keeps the circuit off by powertime microseconds
  delayMicroseconds(powertime);

  // Send TRIAC signal
  digitalWrite(loadR, HIGH);

  // Wait a few microseconds for TRIAC to see the pulse
  delayMicroseconds(8.33);

  // Turn off the pulse
  digitalWrite(loadR, LOW);
}

void setup(void) {
  // initialize inputs/outputs
  // start serial port
  Serial.begin(9600);
  pinMode(loadR, OUTPUT);

  // Initializes interrupt. The number zero indicates port 2 of Arduino,
  // zero_crosss_int is the function that will be called every time pin 2
  // changes
  attachInterrupt(digitalPinToInterrupt(2), zero_crosss_int, CHANGE);
  time = millis();
  t2   = millis();
}

void loop(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  int  HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;

  ds.reset_search();

  if (!ds.search(addr)) {
    Serial.print("No more addresses.\n");
    ds.reset_search();
    return;
  }

  ds.reset();
  ds.select(addr);

  ds.write(0x4E);    // Write scratchpad command
  ds.write(0);       // TL data
  ds.write(0);       // TH data
  ds.write(0x3F);    // Configuration Register (resolution) 7F=12bits 5F=11bits
                     // 3F=10bits 1F=9bits

  ds.reset();        // This "reset" sequence is mandatory
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  t = millis();

  while ((millis() - t) <= 100) {} // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);           // Read Scratchpad

  for (i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  LowByte  = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit  = TReading & 0x8000;           // test most sig bit

  if (SignBit)                            // negative
  {
    TReading = (TReading ^ 0xffff) + 1;   // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4; // multiply by (100 * 0.0625) or 6.25

  Whole = Tc_100 / 100;                   // separate off the whole and
                                          // fractional portions
  Fract = Tc_100 % 100;

  if (SignBit)                            // If its negative
  {
    Serial.print("-");
  }
  Serial.print(Whole);
  Serial.print(".");

  if (Fract < 10)
  {
    Serial.print("0");
  }
  Serial.print(Fract);

  Serial.print("\n");

  // Serial.println(millis() - t2);

  // Sets power to different levels. If the system is connected to a lamp, it
  // will vary in brightness
  if ((millis() - t2 > 5000) && (power != 100)) {
    Serial.println("Starting sample collection...");
    power = 100;
  }

  while (millis() - time > 60000) power = 25;
}
