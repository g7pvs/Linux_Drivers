/*
 * Copyright (C) 2011  Warren Jasper
 * All rights reserved.
 *
 * Updates and modifications by Trev Jackson <trev.g7pvs@ntlworld.com>
 * Copyright (C) 2026
 */

/***************************************************************************
 *
 *  test-pcim-das1602-16.c
 *
 *  This program is used to test the PCIM-DAS1602/16 or PCIe-DAS1602/16
 *  Analog to Digital Linux loadable module.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <asm/types.h>
#include "pcim-das1602-16.h"

float value2Volts( int value );


/***************************************************************************
 *
 *  Global Data
 *
 ***************************************************************************/

char *DevName = "/dev/pcim-das1602-16/ad0_0";
int  Gain     = P_10_00V;
int  ADC_Mode = ADC_SOFT_CONVERT;
int  ADC_Type = ADC_ACQ_NORMAL;
int  Count    = 1;
int  NoStop   = 0;
int  Print    = 1;
int  freq_A2D = 1000;
int  Status;
char UB;

int fdDAC0, fdDAC1;          /* D/A file descriptors */
int fdADC, fdADC1;           /* A/D file descriptors */
int fdDIOA, fdDIOB, fdDIOC, fdDIOA1;  /* DIO file descriptors */

void Usage( void )
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: adcread \'options\'\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr,"   [-dev /dev/pcim-das1602-16/adc0_#]  - Specify device file.\n");
  fprintf(stderr,"   [-range (0-8)]                      - Voltage input range (see manual)\n");
  fprintf(stderr,"   [-ct ##]                            - Number of samples to read\n");
  fprintf(stderr,"   [-soft]                             - Use software triggers\n");
  fprintf(stderr,"   [-extern]                           - Use extnernal clock triggers\n");
  fprintf(stderr,"   [-pacer]                            - Use pacer clock triggers\n");
  fprintf(stderr,"   [-ringbuf]                          - Use continuous acquisition\n");
  fprintf(stderr,"   [-noprint]                          - Don't print samples\n");
  fprintf(stderr,"   [-nostop]                           - Sample forever\n");
  fprintf(stderr,"   [-A2Dfreq ##]                       - Sample A/D Frequency used with pacer triggers\n");
  fprintf(stderr,"\n");
  exit(1);
}

void DoCommandLine(int argc, char **argv)
{
  int i = 1;

  while (i < argc) {
    if (strcmp(argv[i], "-dev") == 0) {
      i++;
      if (i == argc) {
        Usage();
      } else {
        DevName = argv[i];
      }
    } else if (strcmp(argv[i], "-range") == 0) {
      i++;
      if (i == argc) {
        Usage();
      } else {
        Gain = atoi(argv[i]);
      }
    } else if (strcmp(argv[i], "-ct") == 0) {
      i++;
      if (i == argc) {
        Usage();
      } else {
        Count = atoi(argv[i]);
      }
    } else if (strcmp(argv[i], "-soft") == 0) {
      ADC_Mode = ADC_SOFT_CONVERT;
    } else if (strcmp(argv[i], "-pacer") == 0) {
      ADC_Mode = ADC_PACER_CLOCK;
    } else if (strcmp(argv[i], "-ringbuf") == 0) {
      ADC_Type = ADC_ACQ_CONTINUOUS;
    } else if (strcmp(argv[i], "-extern") == 0) {
      ADC_Mode = ADC_EXTERNAL_PACER_FALLING;
    } else if (strcmp(argv[i], "-noprint") == 0) {
      Print = 0;
    } else if (strcmp(argv[i], "-nostop") == 0) {
      NoStop = 1;
    } else if (strcmp(argv[i], "-A2Dfreq") == 0) {
      i++;
      if (i == argc) {
        Usage();
      } else {
        freq_A2D = atoi(argv[i]);
      }
    } else {
      Usage();
    }
    i++;
  }
}

void DoOpenDevices()
{
  char str[80];
  long arg;

  if ((fdADC = open(DevName, O_RDWR)) < 0) {
    perror(DevName);
    printf("error opening device %s\n", DevName);
    exit(2);
  }
  ioctl(fdADC, ADC_SET_MODE, ADC_Mode);
  ioctl(fdADC, ADC_SET_ACQ_TYPE, ADC_Type);


  strcpy(str, "/dev/pcim-das1602-16/ad1_0");
  if ((fdADC1 = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening 2nd device %s\n", str);
    exit(2);
  }
  ioctl(fdADC1, ADC_SET_MODE, ADC_Mode);
  ioctl(fdADC1, ADC_SET_ACQ_TYPE, ADC_Type);

  
  strcpy(str, "/dev/pcim-das1602-16/da0_0");
  if ((fdDAC0 = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening device %s\n", str);
    exit(2);
  }

  strcpy(str, "/dev/pcim-das1602-16/da0_1");
  if ((fdDAC1 = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening device %s\n", str);
    exit(2);
  }

  strcpy(str, "/dev/pcim-das1602-16/dio0_A");
  if ((fdDIOA = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening device %s\n", str);
    exit(2);
  }

  strcpy(str, "/dev/pcim-das1602-16/dio1_A");
  if ((fdDIOA1 = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening 2nd device %s\n", str);
    exit(2);
  }

  strcpy(str, "/dev/pcim-das1602-16/dio0_B");
  if ((fdDIOB = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening device %s\n", str);
    exit(2);
  }

  strcpy(str, "/dev/pcim-das1602-16/dio0_C");
  if ((fdDIOC = open(str, O_RDWR)) < 0) {
    perror(str);
    printf("error opening device %s\n", str);
    exit(2);
  }

  ioctl(fdADC, ADC_GET_POLARITY, &arg);
  if (arg == 'U') {
    printf("The Analog Input Polarity Switch is set to Unipolar.\n");
    UB = 'U';
  } else {
    printf("The Analog Input Polarity Switch is set to Bipolar.\n");
    UB = 'B';
  }

  ioctl(fdADC, ADC_GET_FRONT_END, &arg);
  if (arg == 16) {
    printf("The Analog Input Mode Switch is set to 16 single-ended channels.\n");
  } else {
    printf("The Analog Input Mode Switch is set to 8 differential channels.\n");
  }

  ioctl(fdADC, ADC_GET_CLK_FREQ, &arg);
  printf("The Pacer Clock jumper is set to %ld Hz.\n", arg);
}

void testDIO()
{
  unsigned short value;
  uint8_t bReg;
  char str[80];

  printf("***********************************************************\n");
  printf("* The program writes values on port A and reads them on   *\n");
  printf("* port B (8 bits), and writes on the low nibble of port C *\n");
  printf("* (4 bits) and reads them on the high nibble of port C.   *\n");
  printf("***********************************************************\n\n");

  ioctl(fdDIOA, DIO_SET_DIRECTION, PORT_OUTPUT);
  ioctl(fdDIOB, DIO_SET_DIRECTION, PORT_INPUT);
  ioctl(fdDIOC, DIO_SET_DIRECTION, LOW_PORT_OUTPUT);
  ioctl(fdDIOC, DIO_SET_DIRECTION, HIGH_PORT_INPUT);

  while (1) {
      printf("Enter value for port A in hex: ");
      scanf("%hx", &value);
      write(fdDIOA, &value, 1);
      read(fdDIOB, &bReg, 1);
      printf("Port B value = %#hx\n", (short int) bReg);
      printf("Enter value for port C in hex: ");
      scanf("%hx", &value);
      value &= 0x0f;          /* mask off high nibble */
      write(fdDIOC, &value, 1);
      read(fdDIOC, &bReg, 1);
      bReg >>= 4;
      printf("Port C value = %#hx\n", (short int) bReg);

      printf("\n Continue? ");
      scanf("%s", str);
      if (str[0] == 'n' || str[0] == 'N') return;
  }
}

#define MAX_COUNT 16384
void testDAC()
{
  uint16_t value;
  char str[80];

  while (1) {
    printf("Enter value for DAC0 [0-0xfff]: ");
    scanf("%hx", &value);
    write(fdDAC0, &value, 1);
    printf("Enter value for DAC1[0-0xfff]: ");
    scanf("%hx", &value);
    write(fdDAC1, &value, 1);
    printf("\n Continue? ");
    scanf("%s", str);
    if (str[0] == 'n' || str[0] == 'N') return;
  }
}

void testADC()
{
  uint16_t value[16384];
  char str[80];
  int i;
  int bytesRead;
  double mean, sd;
  uint16_t max, min;
  uint8_t lowChan, hiChan;
  uint64_t diff;
  double accum;
  struct timespec start, end;
  int timerok = 1;

  printf("Enter low channel: ");
  scanf("%hhd", &lowChan);
  printf("Enter hight channel: ");
  scanf("%hhd", &hiChan);
  if (Count == 1) {
    printf("Enter number of samples [1-16834]: ");
    scanf("%d", &Count);
  }
  
  ioctl(fdADC, ADC_SET_PACER_FREQ, freq_A2D);
  ioctl(fdADC, ADC_SET_MUX_LOW, lowChan);
  ioctl(fdADC, ADC_SET_MUX_HIGH, hiChan);
  ioctl(fdADC, ADC_SET_ACQ_TYPE, ADC_ACQ_CONTINUOUS);
  
  while (1) {
    for (i = 0; i < 16384; i++) {
      value[i] = 0xbeef;
    }

    if (clock_gettime(CLOCK_REALTIME, &start) == -1) timerok = 0;
    bytesRead = read(fdADC, value, Count);
    if (clock_gettime(CLOCK_REALTIME, &end) == -1) timerok = 0;
    diff = 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
    accum = end.tv_nsec - start.tv_nsec;
    accum = accum / 1E9;
    accum += (end.tv_sec - start.tv_sec);
    
    if (bytesRead != Count) {
      printf("testADC: Error on read() \n");
    }

    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }

    printf("elasped time = %llu nsec\n", (long long unsigned int) diff);
    printf("elapsed time = %lf sec -- ok:%d\n", accum, timerok);
    printf("%d samples read\n", bytesRead);
    printf("\n*******  Statistics ******\n");
    mean = 0.0;
    max = 0x0;
    min = 0xffff;

    for ( i = 0; i < Count; i++ ) {
      mean += value[i];
      if (value[i] > max) max = value[i];
      if (value[i] < min) min = value[i];
    }
    mean /= Count;
    sd = 0.0;
    for ( i = 0; i < Count; i++ ) {
      sd += (value[i] - mean)*(value[i] - mean);
    }
    sd = sqrt(sd / (Count - 1));
    printf("Mean = %f,  Standard deviation = %f.\n", mean, sd);
    printf("Max = %#x,  Min = %#x,  Avg = %#x\n", max, min, (uint16_t) (mean+0.5));

    printf("\n\n Continue? ");
    scanf("%s", str);
    if (str[0] == 'n' || str[0] == 'N') return;
  }
}

void testADC2(){

  uint16_t value[16384];
  char str[80];
  int i;
  int bytesRead;
  uint8_t lowChan, hiChan;

  printf("Enter low channel: ");
  scanf("%hhd", &lowChan);
  printf("Enter hight channel: ");
  scanf("%hhd", &hiChan);
  if (Count == 1) {
    printf("Enter number of samples [1-16834]: ");
    scanf("%d", &Count);
  }

  ioctl(fdADC, ADC_SET_PACER_FREQ, freq_A2D);
  ioctl(fdADC, ADC_SET_MUX_LOW, lowChan);
  ioctl(fdADC, ADC_SET_MUX_HIGH, hiChan);
  ioctl(fdADC, ADC_SET_MODE, ADC_PACER_CLOCK);
  ioctl(fdADC, ADC_SET_ACQ_TYPE, ADC_ACQ_CONTINUOUS);

    for (i = 0; i < 16384; i++) {
      value[i] = 0xbeef;
    }
    bytesRead = read(fdADC, value, Count);
    if (bytesRead != Count) {
      printf("testADC: Error on read() \n");
    }

    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }
    printf("%d samples read\n", bytesRead);
    bytesRead = read(fdADC, value, Count);
    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }
    printf("%d samples read\n", bytesRead);
    sleep(1);

    bytesRead = read(fdADC, value, Count);
    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }
    printf("%d samples read\n", bytesRead);
    bytesRead = read(fdADC, value, Count);
    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }
    printf("%d samples read\n", bytesRead);
    bytesRead = read(fdADC, value, Count);
    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }
    printf("%d samples read\n", bytesRead);
    printf("\n\n Continue? ");
    scanf("%s", str);

    ioctl(fdADC, ADC_SET_MODE, ADC_SOFT_CONVERT);   /*  DON'T LEAVE IN CONTINOUS      */
  ioctl(fdADC, ADC_SET_ACQ_TYPE, ADC_ACQ_NORMAL); /*  CHANGE SETTINGS WHEN FINISHED */
}

void testADC3()
{
  uint16_t value[16384];
  char str[80];
  int timeout_msecs = 50;
  int bytesRead,i,ret,p,pollcount,tot_timeout;
  double mean, sd;
  uint16_t max, min;
  uint8_t lowChan, hiChan;
  uint64_t diff;
  double accum;
  struct timespec start, end;
  int timerok = 1;
  struct pollfd fds[1];

  printf("Enter low channel: ");
  scanf("%hhd", &lowChan);
  printf("Enter hight channel: ");
  scanf("%hhd", &hiChan);
  if (Count == 1) {
    printf("Enter number of samples [1-16834]: ");
    scanf("%d", &Count);
  }


  ioctl(fdADC, ADC_SET_PACER_FREQ, freq_A2D);
  ioctl(fdADC, ADC_SET_MODE, ADC_PACER_CLOCK);
  ioctl(fdADC, ADC_SET_ACQ_TYPE, ADC_ACQ_NORMAL);
  ioctl(fdADC, ADC_SET_MUX_LOW, lowChan);
  ioctl(fdADC, ADC_SET_MUX_HIGH, hiChan);

  while (1) {
    for (i = 0; i < 16384; i++) {
      value[i] = 0xbeef;
    }

    p = fork();
    if (p==0)
    {
      fds[0].fd = fdADC;
      fds[0].events = POLLIN;
      tot_timeout = 0;
    for (pollcount=0;pollcount<16;pollcount=pollcount+1){
      ret = poll(fds,1,timeout_msecs);
      if (ret>0){
        printf("read finished.\n");
      }
      else {
        tot_timeout += timeout_msecs;
        printf("not finished after %dmS.\n",tot_timeout);
      }
    }
     exit(0);
    }
    else
    {
      if (clock_gettime(CLOCK_REALTIME, &start) == -1) timerok = 0;
      bytesRead = read(fdADC, value, Count);
      if (clock_gettime(CLOCK_REALTIME, &end) == -1) timerok = 0;
      wait(NULL);
    }

    diff = 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
    accum = end.tv_nsec - start.tv_nsec;
    accum = accum / 1E9;
    accum += (end.tv_sec - start.tv_sec);
    if (bytesRead != Count) {
      printf("testADC: Error on read() \n");
    }
    for (i = 0; i < Count; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }

    printf("elasped time = %llu nsec\n", (long long unsigned int) diff);
    printf("elapsed time = %lf sec -- ok:%d\n", accum, timerok);
    printf("%d samples read\n", bytesRead);
    printf("\n*******  Statistics ******\n");
    mean = 0.0;
    max = 0x0;
    min = 0xffff;

    for ( i = 0; i < Count; i++ ) {
      mean += value[i];
      if (value[i] > max) max = value[i];
      if (value[i] < min) min = value[i];
    }
    mean /= Count;
    sd = 0.0;
    for ( i = 0; i < Count; i++ ) {
      sd += (value[i] - mean)*(value[i] - mean);
    }
    sd = sqrt(sd / (Count - 1));
    printf("Mean = %f,  Standard deviation = %f.\n", mean, sd);
    printf("Max = %#x,  Min = %#x,  Avg = %#x\n", max, min, (uint16_t) (mean+0.5));

    printf("\n\n Continue? ");
    scanf("%s", str);
    if (str[0] == 'n' || str[0] == 'N') return;
  }
}


void Dig_to_ADC()
{
  unsigned short dio_value;
  uint16_t value[2000], value2[2000];
  char str[80];
  int bytesRead, bytesRead2, i, timercounter;
  pid_t p,q;

  ioctl(fdDIOA1, MODE_IO);
  ioctl(fdDIOA1, DIO_SET_DIRECTION, PORT_OUTPUT);
  dio_value = 0;
  write(fdDIOA1,&dio_value,1); //clear outputs on Port A

  ioctl(fdADC1, ADC_SET_PACER_FREQ, 1000); //frequency in Hertz
  ioctl(fdADC1, ADC_SET_MODE, ADC_PACER_CLOCK);
  ioctl(fdADC1, ADC_SET_ACQ_TYPE, ADC_ACQ_NORMAL);
  ioctl(fdADC1, ADC_SET_MUX_LOW, 0);
  ioctl(fdADC1, ADC_SET_MUX_HIGH, 4);
  ioctl(fdADC1, ADC_SET_TRIGGER, GATE_LATCH); //GATE_EN=0, GATE_POL=0, GATE_LATCH=1

  printf("Connect pin 25 (Ext Trigger) of Analogue port of both cards to pin 36 (A0) of Digital port on 2nd Card\n");

  printf("\nType y Continue? ");
  scanf("%s", str);

  p=fork();

  if(p==0)
  {
    bytesRead = read(fdADC, value, 1025);
    if (bytesRead != 1025) {
      printf("testADC: Error on read() \n");
    }
    else
    {
      printf("read successful");
    }
    for (i = 0; i < 1024; i++) {
      printf("Value number %d is %#6x volts = %f\n", i+1, value[i], value2Volts((int) value[i]));
    }
    exit(0);
  }
  else
  {
    q=fork();
    if (q==0)
    {
      bytesRead2 = read(fdADC1, value2, 1025);
      if (bytesRead2 != 1025) {
        printf("testADC: Error on read2() \n");
      }
      else
      {
        printf("read2 successful");
      }
      for (i = 0; i < 1024; i++) {
        printf("Value2 number %d is %#6x volts = %f\n", i+1, value2[i], value2Volts((int) value2[i]));
      }
      exit(0);
    }
    else
    {
      for(timercounter=0;timercounter<2;timercounter++){
      sleep(1);
      //scanf("%d", &dio_value);
      dio_value = 0xff;
      write(fdDIOA1,&dio_value,1); // write dio_value to output
      sleep(1);
      dio_value = 0x00;
      write(fdDIOA1,&dio_value,1); // write dio_value to output
      }
      wait(NULL);
      printf("\nType y Continue? ");
      scanf("%s", str);
    }
  }
}

void ChangeADCGains()
{
    int choice;

    system("/usr/bin/clear");
    printf("Select from the following choices:\n");
    printf("  1.   Bipolar +/- 10.00V or Unipolar 0-10V.\n");
    printf("  2.   Bipolar +/-  5.00V or Unipolar 0-5V.\n");
    printf("  3.   Bipolar +/-  2.50V or Unipolar 0-2.5V.\n");
    printf("  4.   Bipolar +/-  1.25V or Unipolar 0-1.25V.\n");
    printf("\nOption: ");
    scanf("%d", &choice);
    switch(choice){
    case 1:
      ioctl(fdADC, ADC_SET_GAINS, P_10_00V);
      break;
    case 2:
      ioctl(fdADC, ADC_SET_GAINS, P_5_00V);
      break;
    case 3:
      ioctl(fdADC, ADC_SET_GAINS, P_2_50V);
      break;
    case 4:
      ioctl(fdADC, ADC_SET_GAINS, P_1_25V);
      break;
    default:
      printf("Illegal option.\n");
    }
}

void SetADPacer()
{
  printf("***********************************************************\n");
  printf("* This program sets the A/D Pacer Frequency which can be  *\n");
  printf("* seen with a scope on Pin 25.                            *\n");
  printf("***********************************************************\n\n");
  
  printf("Input the desired AD Pacer Frequency (Hz): ");
  scanf("%d", &freq_A2D);
  ioctl(fdADC, ADC_STOP_PACER, 0);
  ioctl(fdADC, ADC_SET_PACER_FREQ, freq_A2D);
  ioctl(fdADC, ADC_GET_PACER_FREQ, &freq_A2D);
  ioctl(fdADC, ADC_START_PACER, 0);
  printf("\nThe AD Pacer Frequency is set to %d\n", freq_A2D);
  sleep(3);
}

float value2Volts( int value )
{
  long  gain;

  ioctl(fdADC, ADC_GET_GAINS, &gain);
  if (UB == 'B'){
  switch (gain) {
  case P_10_00V:
    return (value - 0x7fff) * 10.0 / 0x7fff;
    break;
  case P_5_00V:
    return (value - 0x7fff) * 5.0 / 0x7fff;
    break;
  case P_2_50V:
    return (value - 0x7fff) * 2.5 / 0x7fff;
    break;
  case P_1_25V:
    return (value - 0x7fff) * 1.25 / 0x7fff;
    break;
  }
  }else{
  switch (gain) {
  case P_10_00V:
    return (value * 10.0 / 0xffff);
    break;
  case P_5_00V:
    return (value * 5.0 / 0xffff);
    break;
  case P_2_50V:
    return (value * 2.5 / 0xffff);
    break;
  case P_1_25V:
    return (value * 1.25 / 0xffff);
    break;
  }
  }
  return -1.0;
}

void Domenu()
{
  int choice;

  while(1) {
    system("/usr/bin/clear");
    printf("Select from the following choices:\n");
    printf("    1. Test Digital I/O.\n");
    printf("    2. Test Digital to Analog Converter.\n");
    printf("    3. Test Analog to Digital Converter.\n");
    printf("    4. Test Analog to Digital Converter (Continuous).\n");
    printf("    5. Test Analog to Digital Converter (interrupts)\n");
    printf("    6. Change A/D Gains.\n");
    printf("    7. Set A/D Internal Pacer.\n");
    printf("    8. Use Digital I/O to trigger A/D on 2 cards.\n");
    printf("    9. Exit.\n");
    printf("\nOption: ");
   
    scanf("%d", &choice);
    switch(choice){
      case 1:  
        testDIO();
        break;
      case 2:  
        testDAC();
        break;
      case 3:  
        testADC();
        break;
      case 4:
        testADC2();
        break;
      case 5:
        testADC3();
        break;
      case 6:
        ChangeADCGains();
        break;
      case 7:
        SetADPacer();
        break;
      case 8:
        Dig_to_ADC();
        break;
      case 9:
        return;
        break;
      default:
        break;
    }
  }    
}

int main(int argc, char **argv)
{
  DoCommandLine(argc, argv);
  DoOpenDevices();
  Domenu();
  close(fdDIOA);
  close(fdDIOA1);
  close(fdDIOB);
  close(fdDIOC);
  close(fdDAC0);
  close(fdDAC1);
  close(fdADC);
  close(fdADC1);
  return(1);
}
