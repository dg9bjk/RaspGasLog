#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/i2c-dev.h>

#define ADS_ADDR1 0x48
#define ADS_ADDR2 0x49
#define ADS_ADDR3 0x4A
#define ADS_ADDR4 0x4B

#define	filenamelength 30
#define filestringlength 200
#define bufferlength 10
#define WAITTIME 1

#define DEBUG 0

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
  time_t akttime;	   // Zeitstempel
  time_t baktime;
  struct tm *timeset;	   // Zeitstruktur zerlegt in Elemente
  int oldday;		   // Altertag für Umschaltung
  int update;		   // Update der Daten im Minutentakt
  
  int fd1;                 // Device-Handle i2c
  FILE *fdlog;		   // File for Logging
  char filename[filenamelength]; // Filename
  int logready;		   // Status Logfile
  int n;		   // temp. Laufvariable
  char filehead[filestringlength];	   // Kopf der CSV-Datei
  char fileinput[filestringlength];	   // Datenstring für Log  
  long count;		   // Anzahl Daten
  int PRG_OK;		   // Programmmlauf OK
  
  uint8_t buf1[bufferlength];        // I/O buffer 1a
  uint8_t buf2[bufferlength];        // I/O buffer 2a
  uint8_t buf3[bufferlength];        // I/O buffer 3a
  uint8_t buf4[bufferlength];        // I/O buffer 4a
  uint8_t buf5[bufferlength];        // I/O buffer 1b
  uint8_t buf6[bufferlength];        // I/O buffer 2b
  uint8_t buf7[bufferlength];        // I/O buffer 3b
  uint8_t buf8[bufferlength];        // I/O buffer 4b
  uint8_t buf9[bufferlength];        // I/O buffer 1c
  uint8_t buf10[bufferlength];        // I/O buffer 2c
  uint8_t buf11[bufferlength];        // I/O buffer 3c
  uint8_t buf12[bufferlength];        // I/O buffer 4c

  int16_t val1;            // Result (int) channel 1a
  int16_t val2;            // Result (int) channel 2a
  int16_t val3;            // Result (int) channel 3a
  int16_t val4;            // Result (int) channel 4a
  int16_t val5;            // Result (int) channel 1b
  int16_t val6;            // Result (int) channel 2b
  int16_t val7;            // Result (int) channel 3b
  int16_t val8;            // Result (int) channel 4b
  int16_t val9;            // Result (int) channel 1c
  int16_t val10;            // Result (int) channel 2c
  int16_t val11;            // Result (int) channel 3c
  int16_t val12;            // Result (int) channel 4c

  // open device on /dev/i2c-1
  if ((fd1 = open("/dev/i2c-1", O_RDWR)) < 0)
  {
    printf("Error: Couldn't open device! %d\n", fd1);
    close(fd1);
    exit(-1);
  }
  logready = 0;
  oldday = -1;
  update = 0;
  count = 0;
  PRG_OK = 1;
  for(n=0;n<filenamelength;n++)
    filename[n]=0x0;
  for(n=0;n<filestringlength;n++)
    fileinput[n]=0x0;
  for(n=0;n<filestringlength;n++)
    filehead[n]=0x0;

  sprintf(filehead,"Zeit;Anzahl;Ch.1;Ch.2;Ch.3;Ch.4;Ch.5;Ch.6;Ch.7;Ch.8;Ch.9;Ch.10;Ch.11;Ch.12;\n\0");

  printf("Hauptprogramm\n");  

  while(PRG_OK) // loop forever
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
        printf("Debug: Jahr: %04d ,Monat: %02d ,Tag: %02d ,Stunde: %02d ,Minute: %02d ,Sekunde: %02d ,Tage: %03d \n",timeset->tm_year+1900,timeset->tm_mon+1,timeset->tm_mday,timeset->tm_hour,timeset->tm_min,timeset->tm_sec,timeset->tm_yday);
      
      sprintf(filename,"Log_%04d%02d%02d.csv\0",timeset->tm_year+1900,timeset->tm_mon+1,timeset->tm_mday);
      if(DEBUG)
        printf("Debug: %s\n",filename);

      // Wenn kein Zeiger auf jeden fall versuchen
      if(! logready)
      {
        if((fdlog = fopen(filename,"a")) >= 0)
        {
          printf("Datei %s geöffnet(1)\n",filename);
          fprintf(fdlog,"%s",filehead);
          fflush(fdlog);
          logready = 1;
          oldday = timeset->tm_yday;
        }
        else
        {
          printf("Datei %s kann nicht geöffnet werden(1)\n",filename);
          logready = 0;
        }
      }
      
      // Tageswechsel
      if((logready)&(timeset->tm_yday != oldday))
      {
        fflush(fdlog);
        fclose(fdlog);
        if((fdlog = fopen(filename,"w")) >= 0)
        {
          printf("Datei %s geöffnet(2)\n",filename);
          fprintf(fdlog,"%s",filehead);
          fflush(fdlog);
          logready = 1;
          oldday = timeset->tm_yday;
        }
        else
        {
          printf("Datei %s kann nicht geöffnet werden(2)\n",filename);
          logready = 0;
        }
      }
    
      if(DEBUG)
        printf("Sensor1\n");

      // connect to first ads1115 as i2c slave
      if (ioctl(fd1, I2C_SLAVE, ADS_ADDR1) < 0)
      {
        printf("Error: Couldn't find device on address!\n");
        PRG_OK = 0;
      }
      
      // set config register (reg. 1) and start conversion
      // AIN0 and GND, 4.096 V, 128 samples/s
      buf1[0] = 1;
      buf1[1] = 0xc1;
      buf1[2] = 0x85;
      
      if(convert(fd1,buf1) < 0)
        PRG_OK = 0;
      
      buf2[0] = 1;
      buf2[1] = 0xd1;
      buf2[2] = 0x85;
      
      if(convert(fd1,buf2) < 0)
        PRG_OK = 0;
      
      buf3[0] = 1;
      buf3[1] = 0xe1;
      buf3[2] = 0x85;
      
      if(convert(fd1,buf3) < 0)
        PRG_OK = 0;
      
      buf4[0] = 1;
      buf4[1] = 0xf1;
      buf4[2] = 0x85;
      
      if(convert(fd1,buf4) < 0)
        PRG_OK = 0;
      
      if(DEBUG)
        printf("Sensor2\n");

      // connect to second ads1115 as i2c slave 
      if (ioctl(fd1, I2C_SLAVE, ADS_ADDR2) < 0)
      {
        printf("Error: Couldn't find device on address!\n");
        PRG_OK = 0;
      }

      // set config register (reg. 1) and start conversion
      // AIN0 and GND, 4.096 V, 128 samples/s
      buf5[0] = 1;
      buf5[1] = 0xc1;
      buf5[2] = 0x85;
      
      if(convert(fd1,buf5) < 0)
        PRG_OK = 0;
      
      buf6[0] = 1;
      buf6[1] = 0xd1;
      buf6[2] = 0x85;
      
      if(convert(fd1,buf6) < 0)
        PRG_OK = 0;

      buf7[0] = 1;
      buf7[1] = 0xe1;
      buf7[2] = 0x85;

      if(convert(fd1,buf7) < 0)
        PRG_OK = 0;

      buf8[0] = 1;
      buf8[1] = 0xf1;
      buf8[2] = 0x85;

      if(convert(fd1,buf8) < 0)
        PRG_OK = 0;

      if(DEBUG)
        printf("Sensor3\n");

      // connect to second ads1115 as i2c slave 
      if (ioctl(fd1, I2C_SLAVE, ADS_ADDR4) < 0)
      {
        printf("Error: Couldn't find device on address!\n");
        PRG_OK = 0;
      }
        
      // set config register (reg. 1) and start conversion
      // AIN0 and GND, 4.096 V, 128 samples/s
      buf9[0] = 1;
      buf9[1] = 0xc1;
      buf9[2] = 0x85;
      
      if(convert(fd1,buf9) < 0)
        PRG_OK = 0;
      
      buf10[0] = 1;
      buf10[1] = 0xd1;
      buf10[2] = 0x85;
      
      if(convert(fd1,buf10) < 0)
        PRG_OK = 0;

      buf11[0] = 1;
      buf11[1] = 0xe1;
      buf11[2] = 0x85;

      if(convert(fd1,buf11) < 0)
        PRG_OK = 0;

      buf12[0] = 1;
      buf12[1] = 0xf1;
      buf12[2] = 0x85;

      if(convert(fd1,buf12) < 0)
        PRG_OK = 0;

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
      val9 = (int16_t)buf9[0]*256 + (uint16_t)buf9[1];
      val10 = (int16_t)buf10[0]*256 + (uint16_t)buf10[1];
      val11 = (int16_t)buf11[0]*256 + (uint16_t)buf11[1];
      val12 = (int16_t)buf12[0]*256 + (uint16_t)buf12[1];

      /* display results */
      // Display for 4.096 V range
      //    printf("Ch 1: %.2f V - Ch 2: %.2f V - Ch 3: %.2f V - Ch 4: %.2f V - Ch 5: %.2f V - Ch 6: %.2f V - Ch 7: %.2f V - Ch 8: %.2f V -- %s",(float)val1*4.096/32768.0,(float)val2*4.096/32768.0,(float)val3*4.096/32768.0,(float)val4*4.096/32768.0,(float)val5*4.096/32768.0,(float)val6*4.096/32768.0,(float)val7*4.096/32768.0,(float)val8*4.096/32768.0,ctime(&akttime));
      // Display for 6.144 V range
      printf("Ch 1: %.3f V - Ch 2: %.3f V - Ch 3: %.3f V - Ch 4: %.3f V - Ch 5: %.3f V - Ch 6: %.3f V - Ch 7: %.3f V - Ch 8: %.3f V - Ch 9: %.3f V - Ch 10: %.3f V - Ch 11: %.3f V - Ch 12: %.3f V -- %s",(float)val1*6.144/32768.0,(float)val2*6.144/32768.0,(float)val3*6.144/32768.0,(float)val4*6.144/32768.0,(float)val5*6.144/32768.0,(float)val6*6.144/32768.0,(float)val7*6.144/32768.0,(float)val8*6.144/32768.0,(float)val9*6.144/32768.0,(float)val10*6.144/32768.0,(float)val11*6.144/32768.0,(float)val12*6.144/32768.0,ctime(&akttime));
      
      if(logready)
      {
        sprintf(fileinput,"%02d-%02d-%04d %02d:%02d:%02d;%ld;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;\n\0",timeset->tm_mday,timeset->tm_mon+1,timeset->tm_year+1900,timeset->tm_hour,timeset->tm_min,timeset->tm_sec,count,(float)val1*6.144/32768.0,(float)val2*6.144/32768.0,(float)val3*6.144/32768.0,(float)val4*6.144/32768.0,(float)val5*6.144/32768.0,(float)val6*6.144/32768.0,(float)val7*6.144/32768.0,(float)val8*6.144/32768.0,(float)val9*6.144/32768.0,(float)val10*6.144/32768.0,(float)val11*6.144/32768.0,(float)val12*6.144/32768.0);
        if(DEBUG)
          printf("Debug: %s\n",fileinput);
        fprintf(fdlog,"%s",fileinput);
        count++;
        fflush(fdlog);
      }
      
      /* pause 10.0 s */
      //usleep(10000000);
      sleep(1);
  }

  close(fd1);
  fclose(fdlog);
  return(1);
}
