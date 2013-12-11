/*

Interface and ISP for the sensor board

See README.txt and LICENSE.txt

*/

#include <Wire.h>

#include <AzIncSensor.h>
#include <SerialCommand.h>
#include <Spi.h>


AzIncSensor sensorBoard;


void memcpy(byte *dest, byte *src, unsigned int len)
{
  for (; len > 0; len--)
    dest[len - 1] = src[len - 1];
}


#define STATE_IDLE 0
#define STATE_PROG 1
#define STATE_SAMP 2


/* internal registers */
word address;
byte state;


void setup()
{
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  state = STATE_IDLE;
  sensorBoard.init();
  commandInit();
}


void loop()
{
  int i;
  if (state == STATE_SAMP)
  {
    i = sensorBoard.sample();
    if (i < 6)
      digitalWrite(2, LOW);
    delay(100);
  }
}


/* command events */
boolean commandRead(byte reg, byte *buffer)
{
  memcpy(buffer, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 32);
  switch (reg)
  {
  case 's': /* state */
    if (state == STATE_PROG)
      memcpy(buffer, "program                         ", 32);
    else if (state == STATE_IDLE)
      memcpy(buffer, "idle                            ", 32);
    else if (state == STATE_SAMP)
      memcpy(buffer, "sample                          ", 32);
    else
    {
      memcpy(buffer, "????????????????????????????????", 32);
      return false;
    }
    return true;
  case 'a': /* address */
    *(word *)buffer = address;
    return true;
  case 'p': /* program flash */
    if (state != STATE_PROG)
      return false;
    sensorBoard.readProgWords((word *)buffer, address, 16);
    return true;
  case 'e': /* eeprom */
    if (state != STATE_PROG)
      return false;
    sensorBoard.readEepromBytes(buffer, address, 32);
    return true;
  case 'f': /* fuse */
    if (state != STATE_PROG)
      return false;
    *(long int *)buffer = sensorBoard.readFuseBits();
    return true;
  case 't': /* test */
    memcpy(buffer, "testtest hownowbrowncow testtest", 32);
    return true;
  case 'm': /* motion */
    if (state != STATE_SAMP)
      return false;
    memcpy(buffer, sensorBoard.sampleBuffer, 32);
    return true;
  default:
    return false;
  }
}

boolean commandWrite(byte reg, byte *buffer)
{
  switch (reg)
  {
  case 's': /* state */
    switch (buffer[0])
    {
    case 'i': /* enter idle mode */
      if (state == STATE_PROG)
      {
        sensorBoard.pmodeEnd();
        state = STATE_IDLE;
      }
      else if (state == STATE_SAMP)
      {
        sensorBoard.sampleEnd();
        state = STATE_IDLE;
      }
      break;

    case 'p': /* enter program mode */
      if (state != STATE_IDLE)
        return false;
      if (!sensorBoard.pmodeStart())
        return false;
      state = STATE_PROG;

      break;
    case 's': /* enter sample mode */
      if (state != STATE_IDLE)
        return false;
      sensorBoard.sampleStart();
      state = STATE_SAMP;

      break;
    case 'r': /* run reset command */
      if (state != STATE_IDLE)
        return false;
      sensorBoard.reset();

    case 'e': /* run erase command */
      if (state != STATE_PROG)
        return false;
      sensorBoard.erase();

      break;
    default:
      return false;
    }
    return true;

  case 'a': /* address */
    address = *(word *)buffer;
    return true;

  case 'p': /* program flash */
    if (state != STATE_PROG)
      return false;
    sensorBoard.writeProgPages((word *)buffer, address, 16);
    return true;

  case 'e': /* eeprom */
    if (state != STATE_PROG)
      return false;
    sensorBoard.writeEepromPages(buffer, address, 32);
    return true;

  default:
    return false;
  }
  return false;
}

void commandError()
{
}
