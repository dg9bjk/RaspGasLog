#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/i2c-dev.h>

#define ADS_ADDR1 0x48
#define ADS_ADDR2 0x49
#define ADS_ADDR3 0x50
#define ADS_ADDR4 0x51

#define DEBUG 1

//######################################################
int convert(int fd,uint8_t buf[])
{
    if (write(fd, buf, 3) != 3)
      {
      perror("Write to register 1");
      return(-1);
      }

    /* wait for conversion complete (msb == 0) */
    do {
      if (read(fd, buf, 2) != 2)
        {
        perror("Read conversion");
        return(-1);
        }
      } while (!(buf[0] & 0x80));

    /* read conversion register (reg. 0) */
    buf[0] = 0;
    if (write(fd, buf, 1) != 1)
      {
      perror("Write register select");
      return(-1);
      }
    if (read(fd, buf, 2) != 2)
      {
      perror("Read conversion");
      return(-1);
      }
    return(0);
}

//######################################################
int main()
  {
  time_t akttime;
  time_t baktime;
  struct tm *timeset;
  int update;
  
  int fd1;                 // Device-Handle i2c
  int fdlog;		   // File for Logging
  uint8_t buf1[10];        // I/O buffer 1a
  uint8_t buf2[10];        // I/O buffer 2a
  uint8_t buf3[10];        // I/O buffer 3a
  uint8_t buf4[10];        // I/O buffer 4a
  uint8_t buf5[10];        // I/O buffer 1b
  uint8_t buf6[10];        // I/O buffer 2b
  uint8_t buf7[10];        // I/O buffer 3b
  uint8_t buf8[10];        // I/O buffer 4b
  int16_t val1;            // Result (int) channel 1a
  int16_t val2;            // Result (int) channel 2a
  int16_t val3;            // Result (int) channel 3a
  int16_t val4;            // Result (int) channel 4a
  int16_t val5;            // Result (int) channel 1b
  int16_t val6;            // Result (int) channel 2b
  int16_t val7;            // Result (int) channel 3b
  int16_t val8;            // Result (int) channel 4b

  // open device on /dev/i2c-1
  if ((fd1 = open("/dev/i2c-1", O_RDWR)) < 0)
  {
    printf("Error: Couldn't open device! %d\n", fd1);
    close(fd1);
    close(fdlog);
    exit(-1);
  }
  fdlog = -1;

  for (;;) // loop forever
  {
    if (baktime != akttime)
    {
      baktime = akttime;
      update = 1;
    }
    else
      update = 0;
      
    time(&akttime);
    timeset = localtime(&akttime);  
    if(DEBUG)
      printf("-- Jahr: %d ,Monat: %d ,Tag: %d ,Stunde: %d ,Minute: %d ,Sekunde: %d ,Tage: %d --\n",timeset->tm_year+1900,timeset->tm_mon+1,timeset->tm_mday,timeset->tm_hour,timeset->tm_min,timeset->tm_sec,timeset->tm_yday);
      
    
    
    // connect to first ads1115 as i2c slave
    if (ioctl(fd1, I2C_SLAVE, ADS_ADDR1) < 0)
    {
      printf("Error: Couldn't find device on address!\n");
      close(fd1);
      close(fdlog);
      exit(-1);;
    }
    
      // set config register (reg. 1) and start conversion
      // AIN0 and GND, 4.096 V, 128 samples/s
      buf1[0] = 1;
      buf1[1] = 0xc1;
      buf1[2] = 0x85;
      
      if(convert(fd1,buf1) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }
      
      buf2[0] = 1;
      buf2[1] = 0xd1;
      buf2[2] = 0x85;
      
      if(convert(fd1,buf2) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }
      
      buf3[0] = 1;
      buf3[1] = 0xe1;
      buf3[2] = 0x85;
      
      if(convert(fd1,buf3) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }
      
      buf4[0] = 1;
      buf4[1] = 0xf1;
      buf4[2] = 0x85;
      
      if(convert(fd1,buf4) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }
      
      // connect to second ads1115 as i2c slave 
      if (ioctl(fd1, I2C_SLAVE, ADS_ADDR2) < 0)
      {
        printf("Error: Couldn't find device on address!\n");
        close(fd1);
        close(fdlog);
        exit(-1);
      }
        
      // set config register (reg. 1) and start conversion
      // AIN0 and GND, 4.096 V, 128 samples/s
      buf5[0] = 1;
      buf5[1] = 0xc1;
      buf5[2] = 0x85;
      
      if(convert(fd1,buf5) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }
      
      buf6[0] = 1;
      buf6[1] = 0xd1;
      buf6[2] = 0x85;
      
      if(convert(fd1,buf6) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }

      buf7[0] = 1;
      buf7[1] = 0xe1;
      buf7[2] = 0x85;

      if(convert(fd1,buf7) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }

      buf8[0] = 1;
      buf8[1] = 0xf1;
      buf8[2] = 0x85;

      if(convert(fd1,buf8) < 0)
      {
        close(fd1);
        close(fdlog);
        exit(-1);
      }

//#################

      /* convert buffer to int value */
      val1 = (int16_t)buf1[0]*256 + (uint16_t)buf1[1];
      val2 = (int16_t)buf2[0]*256 + (uint16_t)buf2[1];
      val3 = (int16_t)buf3[0]*256 + (uint16_t)buf3[1];
      val4 = (int16_t)buf4[0]*256 + (uint16_t)buf4[1];
      val5 = (int16_t)buf5[0]*256 + (uint16_t)buf5[1];
      val6 = (int16_t)buf6[0]*256 + (uint16_t)buf6[1];
      val7 = (int16_t)buf7[0]*256 + (uint16_t)buf7[1];
      val8 = (int16_t)buf8[0]*256 + (uint16_t)buf8[1];

      /* display results */
      // Display for 4.096 V range
      //    printf("Ch 1: %.2f V - Ch 2: %.2f V - Ch 3: %.2f V - Ch 4: %.2f V - Ch 5: %.2f V - Ch 6: %.2f V - Ch 7: %.2f V - Ch 8: %.2f V -- %s",(float)val1*4.096/32768.0,(float)val2*4.096/32768.0,(float)val3*4.096/32768.0,(float)val4*4.096/32768.0,(float)val5*4.096/32768.0,(float)val6*4.096/32768.0,(float)val7*4.096/32768.0,(float)val8*4.096/32768.0,ctime(&akttime));
      // Display for 6.144 V range
      printf("Ch 1: %.3f V - Ch 2: %.3f V - Ch 3: %.3f V - Ch 4: %.3f V - Ch 5: %.3f V - Ch 6: %.3f V - Ch 7: %.3f V - Ch 8: %.3f V -- %s",(float)val1*6.144/32768.0,(float)val2*6.144/32768.0,(float)val3*6.144/32768.0,(float)val4*6.144/32768.0,(float)val5*6.144/32768.0,(float)val6*6.144/32768.0,(float)val7*6.144/32768.0,(float)val8*6.144/32768.0,ctime(&akttime));
      
      /* pause 1.0 s */
      usleep(1000000);
  }

  close(fd1);
  close(fdlog);
  return(1);
}
