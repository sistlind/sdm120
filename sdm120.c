#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

 
#include "sdm120.h"


 
int main(int argc, char* argv[]) {
    int debug = 0;
	char sqlmode =0;
    char ttydev[16] = "/dev/ttyUSB0";
	char output_colnames[250]="";//for sqloutput
	char output_values[250]="";//for sqloutput


    for(int i=1; i < argc; ++i)
    {
        if(      !strcmp( argv[i], "-v" ) ) {
            debug = 1;
            printf("'debug aktiviert.\n");
        } else if (!strcmp( argv[i], "-sql" ) ) {
            sqlmode =1;
			//printf("sql syntax.\n");
        } else {
            strcpy(ttydev,argv[i]);
            //printf("tty: %s\n", ttydev);
        }
    }
 
    struct termios serial;
    char inBuff[16];
    char outBuff[8];
    char *buffer;
    int wcount = 0;


    int fd = open(ttydev, O_RDWR | O_NOCTTY | O_NDELAY);
 
    if (fd == -1) {
        perror(argv[2]);
        return -1;
    }
 
    if (tcgetattr(fd, &serial) < 0) {
        perror("Getting configuration");
        return -1;
    }
 
    //////////////////// Set up Serial Configuration ////////////
    serial.c_iflag = 0;
    serial.c_oflag = 0;
    serial.c_lflag = 0;
    serial.c_cflag = 0;
 
    serial.c_cc[VMIN] = 0;
    serial.c_cc[VTIME] = 10;
 
    serial.c_cflag = B2400| CS8 |CREAD|CLOCAL;//8Bits, parity none
    fcntl(fd,F_SETFL,0);
    tcsetattr(fd, TCSANOW, &serial); // Apply configuration
    //////////////////////////////////////////////////////////////
 
    

   buffer = &inBuff[0];
   int i = 0;
   int j = 0;
 
   //////////////////////////         Voltage                /////////////////////////////
	for (int reading=0;reading < sizeof(reg_to_read)/sizeof(reg_to_read[0]);reading++) {
	if(sqlmode==1&&reg_to_read[reading].skip==1) continue;
	   i = 0; j = 0;
	   memset(&inBuff[0],0,sizeof(inBuff));
	   memset(&inBuff[0],0,sizeof(outBuff));

	   Modbus_Framegen(MODBUS_ADDR,MODBUS_READ,reg_to_read[reading].addr,outBuff);
	   write(fd,outBuff,sizeof(outBuff));
	   buffer = &inBuff[0];
		if (debug > 0) {
		   printf("Sent: ");
		   for(j = 0;j < sizeof(outBuff);j++)
		   {
				printf("%02x ",outBuff[j]);
		   } 
		   printf("\n");
		   printf("Received: " );
		}
	   int rcount=0;  
			while( rcount<9 && (rcount += read(fd,buffer,1)) > 0)
			{
			  if (rcount < 0) {
				   perror("Read");
				  return -1;
				}
		 
			  buffer++;
			if (debug > 0) {
			  printf("%02x ",inBuff[i]); i++;
			}   
			}
			
				uint16_t rsp_chksum=Modbus_CRC(inBuff,7);
			if (debug > 0) {
				printf("\nCalculated Response checksum:0x%04x\n",rsp_chksum);		
			}
			if (inBuff[7] != (rsp_chksum&0xff) ||inBuff[8] != ((rsp_chksum&0xff00)>>8)) {
				printf("receive checksum error");
				return -1;	
			} 
			

		u.c[3] = inBuff[3];
		u.c[2] = inBuff[4];
		u.c[1] = inBuff[5];
		u.c[0] = inBuff[6];
		if (sqlmode==1) {
			char delim[2] = "";
			if (reading>0) strcpy(delim,",");
			
			sprintf(output_colnames+strlen(output_colnames),"%s`%s`",delim,reg_to_read[reading].name);
			sprintf(output_values+strlen(output_values),"%s%f",delim,u.f);
			} else {
			printf("%10s: %10.3f %s \n",reg_to_read[reading].name,u.f,reg_to_read[reading].unit);
		}
		
		
	}
	close(fd);

	if (sqlmode==1) {
		printf("(%s)\nVALUES\n(%s);\n",output_colnames,output_values);
	}
}

void Modbus_Framegen (char addr,char cmd, uint16_t reg, char buffer[]) {
    buffer[0] = addr;
    buffer[1] = cmd;
    buffer[2] = (reg&0xff00)>>8;
    buffer[3] = (reg&0xff);
    buffer[4] = 0;
    buffer[5] = 2;
	uint16_t crc = Modbus_CRC(buffer,6);
    buffer[6] = (crc&0xff);
    buffer[7] = (crc&0xff00)>>8;


}

///////////////// Compute the MODBUS RTU CRC
uint16_t Modbus_CRC(char buf[], int len)
{
  uint16_t crc = 0xFFFF;
  
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];          // XOR byte into least sig. byte of crc
  
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}


