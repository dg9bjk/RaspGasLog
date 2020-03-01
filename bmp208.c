#include <wiringPi.h>
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
#include <math.h>

#define ADS_TEMP  0x76

#define VERSION "1.0.0"

#define bufferlength 10
#define WAITTIME 1
#define LedRot    0
#define ALTITUDE 10
#define MAXBME280DATA	119

#define DEBUG 1

//######################################################
int main()
  {
  int fd1;                 // Device-Handle i2c
  int n;		   // temp. Laufvariable
  long count;		   // Anzahl Daten
  int PRG_OK;		   // Programmmlauf OK

  char data[MAXBME280DATA+2] = {0};	// RawData
  int T[3] = {0};			// Temperatur Kalibrirung
  int P[9] = {0};			// Luftdruck Kalibrirung
  int H[6] = {0};			// Feuchte Kalibrirung
  int device;				// devicehandle
  double temperature;			// final temperature
  double pressure;			// final pressure
  double huminity;			// final huminity
  int i;				// loop counter
  double temp1, temp2, temp3;		// calculating temperature
  double press1, press2, press3;	// calculating pressure
  double hum1;                   // Feuchte
  char reg[1] = {0};			// reg[], config[] for I2C I/O
  char config[2] = {0};
  long adc_a;				// Raw-Werte aus dem AD-Wandler
  long adc_p;				// Raw-Werte aus dem AD-Wandler
  long adc_t;
  long adc_h;
  int VME280config;			// Kalibrieung

  // open device on /dev/i2c-1
  if ((fd1 = open("/dev/i2c-1", O_RDWR)) < 0)
  {
    printf("Error: Couldn't open device! %d\n", fd1);
    close(fd1);
    exit(-1);
  }
  PRG_OK = 1;
  VME280config = 0;

  printf("Hauptprogramm "VERSION"\n");  

  while(PRG_OK) // loop forever
  {
      if (ioctl(fd1, I2C_SLAVE, ADS_TEMP) < 0)
      {
        printf("Error: Couldn't find device on address!\n");
        PRG_OK = 0;
      }
      
      temperature  = 0.0;
      huminity     = 0.0;
      pressure     = 0.0;

      if(! VME280config)
      {
        reg[0] = 0x88;
        write(fd1, reg, 1);
        if(read(fd1, data, MAXBME280DATA) != MAXBME280DATA)
        {
          printf("Unable to read data from i2c bus(2)\n");
          PRG_OK = 0;
        }

        if(DEBUG)
        {
          printf("\nRaw-Data 0x88 - 0xFF\n");
          for(n=0;n<MAXBME280DATA;n++)
          {
            printf("Raw[%x]=%d-%X;\n",n+0x88,data[n],data[n]);
          }
        }

        // Temperatur
        T[0] = data[1] * 256 + data[0];
        T[1] = data[3] * 256 + data[2];
        if(T[1] > 32767)
        { 
          T[1] -= 65536;
        }
        T[2] = data[5] * 256 + data[4];
        if(T[2] > 32767)
        {
          T[2] -= 65536;
        }
        
        // Luftdruck
        P[0] = data[7] * 256 + data[6];
        for (i = 0; i < 8; i++)
        {
          P[i+1] = data[2*i+9]*256 + data[2*i+8];
          if(P[i+1] > 32767)
          { 
            P[i+1] -= 65536;
          }
        }
        
        // Feuchte
        H[0] = data[25];
        H[1] = data[90] * 256 + data[89];
        if(H[1] > 32767)
        { 
          H[1] -= 65536;
        }
        H[2] = data[91];
        H[3] = data[92] * 16 + (data[93] & 0x0F);
        H[4] = data[94] * 16 + ((data[93] & 0xF0)/16);
        H[5] = data[95];
        
        VME280config = 1;
        if(DEBUG)
        {
          printf("\nCal-Daten decodiert:\n");
          printf("T[0]=%d-%X;\nT[1]=%d-%X;\nT[2]=%d-%X;\n",T[0],T[0],T[1],T[1],T[2],T[2]);
          printf("P[0]=%d-%X;\nP[1]=%d-%X;\nP[2]=%d-%X;\nP[3]=%d-%X;\nP[4]=%d-%X;\nP[5]=%d-%X;\nP[6]=%d-%X;\nP[7]=%d-%X;\nP[8]=%d-%X;\n",P[0],P[0],P[1],P[1],P[2],P[2],P[3],P[3],P[4],P[4],P[5],P[5],P[6],P[6],P[7],P[7],P[8],P[8]);
          printf("H[0]=%d-%X;\nH[1]=%d-%X;\nH[2]=%d-%X;\nH[3]=%d-%X;\nH[4]=%d-%X;\nH[5]=%d-%X;\n",H[0],H[0],H[1],H[1],H[2],H[2],H[3],H[3],H[4],H[4],H[5],H[5]);
        }
      }

        // Select control measurement register(0xF4)
        // normal mode, temp and pressure oversampling rate = 1(0x27)
        config[0] = 0xF4;
        config[1] = 0x27;
        write(fd1, config, 2);
        sleep(1);
        
        // select config register(0xF5)
        // stand_by time = 1000 ms(0xA0)
        config[0] = 0xF5;
        config[1] = 0xA0;
        write(fd1, config, 2);
        sleep(1);
        
        // Read 6 bytes of data from register(0xF7)
        reg[0] = 0xF4;
        write(fd1, reg, 1);
        if(read(fd1, data, 10) != 10)
        {
          printf("Unable to read data from i2c bus (1)\n");
          PRG_OK = 0;
        }
        // Convert pressure and temperature data to 19-bits
        adc_a = (((long)data[0] << 16) + ((long)data[1] << 8) + ((long)data[2]));
        adc_p = (((long)data[3] << 12) + ((long)data[4] << 4) + ((long)(data[5] >> 4)));
        adc_t = (((long)data[6] << 12) + ((long)data[7] << 4) + ((long)(data[8] >> 4)));
        adc_h = (((long)data[9] << 8) + ((long)data[10]));

        if(DEBUG)
        {
          printf("\nRaw-Werte ab 0xf4 :\n");
          printf("testwert:\ndata[0]=%d-%X;\ndata[1]=%d-%X;\ndata[2]=%d-%X;\ntemperatur:\ndata[3]=%d-%X;\ndata[4]=%d-%X;\ndata[5]=%d-%X;\nluftdruck:\ndata[6]=%d-%X;\ndata[7]=%d-%X;\ndata[8]=%d-%X;\nfeuchte:\ndata[9]=%d-%X;\ndata[10]=%d-%X;\n",data[0],data[0],data[1],data[1],data[2],data[2],data[3],data[3],data[4],data[4],data[5],data[5],data[6],data[6],data[7],data[7],data[8],data[8],data[9],data[9],data[10],data[10]);
          printf("\nDecode Werte:\n");
          printf("Testwert raw  : %ld-%X\n",adc_a,adc_a);
          printf("Temperatur raw: %ld-%X\n",adc_t,adc_t);
          printf("Luftdruck raw : %ld-%X\n",adc_p,adc_p);
          printf("Feuchte raw   : %ld-%X\n",adc_h,adc_h);
        }
        
        // Temperatur kompensiert
        temp1 = (((double)adc_t)/16384.0 - ((double)T[0])/1024.0)*((double)T[1]);
        temp3 = ((double)adc_t)/131072.0 - ((double)T[0])/8192.0;
        temp2 = temp3*temp3*((double)T[2]);
        temperature = (temp1 + temp2)/5120.0;
        
        // Luftfdruck kompensiert
        press1 = ((temp1 + temp2)/2.0) - 64000.0;
        press2 = press1*press1*((double)P[5])/32768.0;
        press2 = press2 + press1*((double)P[4])*2.0;
        press2 = (press2/4.0) + (((double)P[3])*65536.0);
        press1 = (((double) P[2])*press1*press1/524288.0 + ((double) P[1])*press1)/524288.0;
        press1 = (1.0 + press1/32768.0)*((double)P[0]);
        press3 = 1048576.0 - (double)adc_p;
        if (press1 != 0.0) // avoid error: division by 0
        {
          press3 = (press3 - press2/4096.0)*6250.0/press1;
          press1 = ((double) P[8])*press3*press3/2147483648.0;
          press2 = press3 * ((double) P[7])/32768.0;
          pressure = (press3 + (press1 + press2 + ((double)P[6]))/16.0)/100;
        }
        else
        {
          pressure = 0.0;
        }
        
        // Feuchte
        hum1 = ((temp1 + temp2) - 76800.0);
        hum1 = (adc_h -(((double)H[3]) * 64.0 + ((double)H[4]) / 16384.0 * hum1)) *
                (((double)H[1]) / 65535.0 * (1.0 + ((double)H[5]) / 67108864.0 * hum1 *
                (1.0 + ((double)H[2]) / 67108864.0 * hum1)));
        hum1 = hum1 * (1.0 - ((double)H[0]) * hum1 / 524288.0);

        if(hum1 > 100.0)
          hum1 = 100.0;
        else if(hum1 < 0.0)
          hum1 = 0.0;
        
        huminity = hum1;        

        // Output data to screen
        if(DEBUG)
        {
          printf("Temperatur       : %.2f °C \n", temperature);
          printf("Luftdruck        : %.2f hPa \n", pressure);
          printf("Luftfeuchte      : %.2f %rH \n", huminity);
          printf("\n");
        }

      /* pause 10.0 s */
      //usleep(10000000);
      sleep(10);
  }

  close(fd1);
  return(1);
}
