#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define SERIAL_BAUDRATE B9600           /* 9600 baudrate will bu use */
#define SERIAL_DEVICE "/dev/ttyUSB0"    /* serial device address definition */
#define SERIAL_BUFFER_SIZE 1024         /* usb serial buffer size definition*/

void signal_handler_IO (int status);    /* definition of signal handler */

int wait_flag=true;                     /* TRUE while no signal received */
volatile bool STOP=false;               /* definition of test variable */

/**
 * @brief main main program entry point, async serial port reader application
 * @return none
*/
int main()
{
   int fd, available_bytes;                 /* definition of file descriptor and bytes counter*/
   struct termios oldtio,newtio;            /* definition of port structs*/
   struct sigaction saio;                   /* definition of signal action */
   char Serial_Buffer[SERIAL_BUFFER_SIZE];  /* definition of serial buffer*/

   //open the device to be non-blocking (read will return immediatly)
   fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);

   //if file descriptor < 0 then exit
   if (fd <0) {

#ifdef CONSOLE_DEBUG
  perror(MODEMDEVICE); //print error code if available
#endif
       exit(-1);  //exit with -1 code
   }

   /* install the signal handler before making the device asynchronous */
   saio.sa_handler = signal_handler_IO;     //define callback function of signal handler
   saio.sa_flags = 0;                       //we dont use special flags(e.g SA_NOCLDSTOP),
   saio.sa_restorer = NULL;                 //restorer callback for signal handler, we will not use

   /* register signal handler callback for standart I/O operations, null for old sigaction struct*/
   sigaction(SIGIO, &saio, NULL);

   /* allow the process to receive SIGIO */
   fcntl(fd, F_SETOWN, getpid());

   /* Make the file descriptor asynchronous (the manual page says only
      O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
   fcntl(fd, F_SETFL, FASYNC);

   tcgetattr(fd,&oldtio); /* save current port settings */

   /* set new port settings for canonical input processing */
   newtio.c_cflag = SERIAL_BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
   newtio.c_iflag = IGNPAR | ICRNL;
   newtio.c_oflag = 0;
   newtio.c_lflag = ICANON;
   newtio.c_cc[VMIN]=1;
   newtio.c_cc[VTIME]=0;

   /* flush data descriptor for new application*/
   tcflush(fd, TCIFLUSH);

   /* set new port settings*/
   tcsetattr(fd,TCSANOW,&newtio);

   /* loop while waiting for input. normally we would do something
      useful here */
   while (STOP==false) {

     /* after receiving SIGIO, wait_flag = FALSE, input is available
        and can be read */
     if (wait_flag==false) {
       available_bytes = read(fd,Serial_Buffer,SERIAL_BUFFER_SIZE);
       if(available_bytes > 2) {
          Serial_Buffer[available_bytes] = 0;
          printf("Available :%d: Data is : %s", available_bytes,(char*) Serial_Buffer );
        }

       wait_flag = true;      /* wait for new input */
     }

   }
   /* restore old port settings */
   tcsetattr(fd,TCSANOW,&oldtio);
 }


/**
 * @brief signal_handler_IO this function used to indicate main loop for new
 * characters have been received by operating system
 * @param status  signal handler status code
 */
void signal_handler_IO (int status)
{

  //print debug message if it necessary for you
#ifdef CONSOLE_DEBUG
    printf("Signal Handler Received Status Code %d\r\n",(int)status );
#endif

  wait_flag = false;   //set flag false to continue main loop
  return;
}
