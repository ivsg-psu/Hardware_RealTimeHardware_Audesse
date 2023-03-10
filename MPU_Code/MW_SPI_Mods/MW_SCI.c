/* Copyright 2016-2018 The MathWorks, Inc. */

#include "MW_SCI.h"

#if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )

    MW_Handle_Type MW_SCI_Open(void * SCIModule, uint8_T isString, uint32_T RxPin, uint32_T TxPin)
    {
        return 0;
    }
    
    MW_SCI_Status_Type MW_SCI_ConfigureHardwareFlowControl(MW_Handle_Type SCIModuleHandle, MW_SCI_HardwareFlowControl_Type HardwareFlowControl, uint32_T RtsDtrPin, uint32_T CtsDtsPin)
    {
        return MW_SCI_SUCCESS;
    }

    MW_SCI_Status_Type MW_SCI_SetBaudrate(MW_Handle_Type SCIModuleHandle, uint32_T Baudrate)
    {
        return MW_SCI_SUCCESS;
    }

    MW_SCI_Status_Type MW_SCI_SetFrameFormat(MW_Handle_Type SCIModuleHandle, uint8_T DataBitsLength, MW_SCI_Parity_Type Parity, MW_SCI_StopBits_Type StopBits)
    {
        return MW_SCI_SUCCESS;
    }

    MW_SCI_Status_Type MW_SCI_Receive(MW_Handle_Type SCIModuleHandle, uint8_T * RxDataPtr, uint32_T RxDataLength)
    {
        return MW_SCI_SUCCESS;
    }

    MW_SCI_Status_Type MW_SCI_Transmit(MW_Handle_Type SCIModuleHandle, uint8_T * TxDataPtr, uint32_T TxDataLength)
    {
        return MW_SCI_SUCCESS;
    }

    MW_SCI_Status_Type MW_SCI_GetStatus(MW_Handle_Type SCIModuleHandle)
    {
        return MW_SCI_SUCCESS;
    }

    MW_SCI_Status_Type MW_SCI_SendBreak(MW_Handle_Type SCIModuleHandle)
    {
        return MW_SCI_SUCCESS;
    }

    void MW_SCI_Close(MW_Handle_Type SCIModuleHandle)
    {

    }
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <sys/unistd.h>
    #include <sys/stat.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <math.h>
    #include "common.h"
    
    /* Local defines */   
    #define NUM_MAX_SCI_DEV      (10) /*a random choice - no limit on serial devices*/
    #define MAX_DEV_NAME         (64)
    
    typedef struct {
        int fd;
        char* portname;
        uint32_T baudrate;
        uint8_T databits;
        MW_SCI_Parity_Type parity;
        MW_SCI_StopBits_Type stopbits;
        uint8_T busy;
    }SCI_dev_t;
    
    static SCI_dev_t sciDev[NUM_MAX_SCI_DEV] = {   
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0},
       {-1,NULL,9600,8,0,1,0}
    } ;
     
    /* Open SERIAL channel*/
    static int SERIAL_open(const char *port)
    {
        int fd;

        /* O_NDELAY: disregard DCD signal line state
         O_NOCTTY: we don't want to be the controlling terminal */
        fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
        if (fd == -1) {
            perror("SERIAL_open/open");
        }
		
		/*flush both data received but not read, and data written but not transmitted.*/
        tcflush(fd, TCIOFLUSH);	
        return fd;
    }

    /* Close SERIAL channel*/
    static void SERIAL_close(int fd)
    {
        int ret;

        ret = close(fd);
        if (ret < 0) {
            /* EBADF, EINTR, EIO: In all cases, descriptor is torn down*/
            perror("SERIAL_close/close");
        }
    }
    
    /* Return device ID given serial port name */
    int getCurrSciDev(const char *name)
    {
        int i;

        for (i = NUM_MAX_SCI_DEV - 1; i > -1; i--) {
            if ((name != NULL) && (sciDev[i].portname != NULL) &&
                    (strncmp(name, sciDev[i].portname, MAX_DEV_NAME) == 0)) {
                break;
            }
        }

        return i;
    }
     /* Allocate device*/
    int Serial_alloc(const char *name)
    {
        int i;

        for (i = 0; i < NUM_MAX_SCI_DEV; i++) {
            if (sciDev[i].portname == NULL) {
                break;
            }
        }
        if (i >= NUM_MAX_SCI_DEV) {
            fprintf(stderr, "Cannot allocate a new device for %s: [%d]\n", 
                    name, i);
            return -1;
        }
        sciDev[i].portname = strndup(name, MAX_DEV_NAME);

        return i;
    }
    
    /* Initialize a SCI */
    MW_Handle_Type MW_SCI_Open(void * SCIModule,
            uint8_T isString,
            uint32_T RxPin, /* Not used */
            uint32_T TxPin)/*Not Used*/
    {
        MW_Handle_Type SCIHandle = (MW_Handle_Type)NULL;
        char * port;
        int currSciDev;
        
        /* Check parameters */
        if (0 == isString) {
                fprintf(stderr,"Only string as SCI Module name is supported.\n");
                exit(-1);
            }
        else{
            /* Initialize the SCI Module*/
            port = (char*)SCIModule;
            currSciDev = getCurrSciDev(port);
            fprintf(stdout, "INIT: sciDevNo = %d\n", currSciDev);
            if (currSciDev == -1) {
                currSciDev = Serial_alloc(port);
                fprintf(stdout, "ALLOC: devNo = %d\n", currSciDev);
                if (currSciDev == -1) {
                    fprintf(stderr,"Error opening Serial bus (SERIAL_init/Alloc).\n");
                    exit(-1);
                }
            }

            if (sciDev[currSciDev].fd < 0) {
                sciDev[currSciDev].fd = SERIAL_open(port);
                if (sciDev[currSciDev].fd < 0) {
                    fprintf(stderr,"Error opening Serial bus (SERIAL_init/Open).\n");
                    exit(-1);
                }
            }
            else{
                /* Maintaining the behavior same as Raspi I/O */
                /* Raspi I/O throws error on creating a serial device object with same port */
                /* Codegen returns the same handle on opening the same bus multiple times */
                /* Making Codegen throw a run time error on trying to open the same serial port multiple times */
                fprintf(stderr,"An active connection to serial device at %s already exists. You cannot create another connection.\n",port);
                exit(-1);
            }
            
            SCIHandle = (MW_Handle_Type)&sciDev[currSciDev];        
        }
  
        return SCIHandle;
    }

    /* Set SCI frame format */
    MW_SCI_Status_Type MW_SCI_ConfigureHardwareFlowControl(MW_Handle_Type SCIModuleHandle, MW_SCI_HardwareFlowControl_Type HardwareFlowControl, uint32_T RtsDtrPin, uint32_T CtsDtsPin)
    {
        return MW_SCI_SUCCESS;
    }

    /* Set the SCI bus speed */
    MW_SCI_Status_Type MW_SCI_SetBaudrate(MW_Handle_Type SCIModuleHandle, uint32_T Baudrate)
    {
        SCI_dev_t *sci;
        struct termios options;
        speed_t optBaud;
        
        if (NULL != (void *)SCIModuleHandle){
            sci = (SCI_dev_t *)SCIModuleHandle; 

            /* Set parameters of the serial connection*/
            switch (Baudrate)
            {
                case     50: optBaud =      B50; break;
                case     75: optBaud =     B75; break;
                case    110: optBaud =    B110; break;
                case    134: optBaud =    B134; break;
                case    150: optBaud =    B150; break;
                case    200: optBaud =    B200; break;
                case    300: optBaud =    B300; break;
                case    600: optBaud =    B600; break;
                case   1200: optBaud =   B1200; break;
                case   1800: optBaud =   B1800; break;
                case   2400: optBaud =   B2400; break;
                case   4800: optBaud =   B4800; break;
                case   9600: optBaud =   B9600; break;
                case  19200: optBaud =  B19200; break;
                case  38400: optBaud =  B38400; break;
                case  57600: optBaud =  B57600; break;
                case 115200: optBaud = B115200; break;
                case 230400: optBaud = B230400; break;
                default:
                    perror("SERIAL_SetBaudrate");
                    return MW_SCI_BUS_ERROR;
            }

            tcgetattr(sci->fd, &options);
            /* Enable the receiver and set local mode*/
            options.c_cflag |= (CLOCAL | CREAD);

            /* Set baud rate*/
            cfsetispeed(&options, optBaud);
            cfsetospeed(&options, optBaud);

            /* Set attributes*/
            tcsetattr(sci->fd, TCSANOW, &options);
        }
        
        return MW_SCI_SUCCESS;
    }

    /* Set SCI frame format */
    MW_SCI_Status_Type MW_SCI_SetFrameFormat(MW_Handle_Type SCIModuleHandle, uint8_T DataBitsLength, MW_SCI_Parity_Type Parity, MW_SCI_StopBits_Type StopBits)
    {
        SCI_dev_t *sci;
        struct termios options;
        speed_t optBaud;
        
        if (NULL != (void *)SCIModuleHandle){
            sci = (SCI_dev_t *)SCIModuleHandle; 
            tcgetattr(sci->fd, &options);

            /* Set data bits*/
            options.c_cflag &= ~CSIZE;
            switch (DataBitsLength) {
                case 5:
                    options.c_cflag |= CS5;
                    break;
                case 6:
                    options.c_cflag |= CS6;
                    break;
                case 7:
                    options.c_cflag |= CS7;
                    break;
                case 8:
                    options.c_cflag |= CS8;
                    break;
                default:
                    perror("SERIAL_SetFrameFormat/DataBitsLength");
                    return MW_SCI_BUS_ERROR;
            }
            
            /*Set input flags for raw data
             * All special processing of terminal input and output 
             * characters is disabled.
             * IGNBRK,BRKINT : Ignore the effect of BREAK condition on input.
             * ICRNL,INLCR,IGNCR : Carriage return <--> New line conversion
             */
            options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ICRNL | ISTRIP | INLCR | IGNCR | IXON);

            /* Set parity*/
            switch (Parity) {
                case MW_SCI_PARITY_NONE:
                    options.c_cflag &= ~PARENB;
                    break;
                case MW_SCI_PARITY_EVEN:
                    options.c_cflag |= PARENB;
                    options.c_cflag &= ~PARODD;
                    options.c_iflag |= (INPCK | ISTRIP); /* Check and strip parity bit*/
                    break;
                case MW_SCI_PARITY_ODD:
                    options.c_cflag |= PARENB;
                    options.c_cflag |= PARODD;
                    options.c_iflag |= (INPCK | ISTRIP); /* Check and strip parity bit*/
                    break;
                default:
                    perror("SERIAL_SetFrameFormat/Parity");
                    return MW_SCI_BUS_ERROR;
            }

            /* Set stop bits (1 or 2)*/
            switch (StopBits) {
                case MW_SCI_STOPBITS_1:
                    options.c_cflag &= ~CSTOPB;
                    break;
                case MW_SCI_STOPBITS_2:
                    options.c_cflag |= CSTOPB;
                    break;
                default:
                    perror("SERIAL_SetFrameFormat/StopBits");
                    return MW_SCI_BUS_ERROR;
            }    

            /* Local options. Configure for RAW input*/
            options.c_lflag &= ~(ICANON | ISIG | ECHO | ECHONL | ECHOE | IEXTEN);

            /* Output options: RAW output*/
            options.c_oflag &= ~OPOST;

            /* Set character read options*/
            options.c_cc[VMIN]  = 0;
            options.c_cc[VTIME] = 100;  /*10 seconds*/

            /* Set attributes*/
            tcsetattr(sci->fd, TCSANOW, &options);
        }
        return MW_SCI_SUCCESS;
    }

    /* Receive the data over SCI */
    MW_SCI_Status_Type MW_SCI_Receive(MW_Handle_Type SCIModuleHandle, uint8_T * RxDataPtr, uint32_T RxDataLength)
    {
        SCI_dev_t *sci;
        int out;
        ssize_t ret;
        uint32_T databytesAvailable, count;
        
        if (NULL != (void *)SCIModuleHandle){
            sci = (SCI_dev_t *)SCIModuleHandle;
            
            sci->busy = 1; /*Set the busy flag*/
            out = ioctl (sci->fd, FIONREAD, &databytesAvailable);
            if (out < 0){
                perror("SERIAL_read/ioctl");
                return MW_SCI_DATA_NOT_AVAILABLE;
            }
            sci->busy = 0; /*Reset the busy flag*/
           /* fprintf(stdout,"Data bytes available %lu.\n", databytesAvailable);*/ /*FOR DEBUG*/
            if (databytesAvailable < RxDataLength){
                /*Output 0 when available no. of data bytes is less than RxDataLength*/
                for (count = 0; count < RxDataLength; count++){
                    *RxDataPtr++ = 0;
                }
                return MW_SCI_DATA_NOT_AVAILABLE;
            }
            else{
                sci->busy = 1; /*Set the busy flag*/
                ret = read(sci->fd, RxDataPtr, RxDataLength);
                sci->busy = 0; /*Reset the busy flag*/
                if (ret < 0) {
                    perror("SERIAL_read/read");
                    return MW_SCI_DATA_NOT_AVAILABLE;
                }
                return MW_SCI_SUCCESS;
            }
        }
      
    }

    /* Transmit the data over SCI */
    MW_SCI_Status_Type MW_SCI_Transmit(MW_Handle_Type SCIModuleHandle, uint8_T * TxDataPtr, uint32_T TxDataLength)
    {
            SCI_dev_t *sci;
            int ret;
            
            if (NULL != (void *)SCIModuleHandle){
                sci = (SCI_dev_t *)SCIModuleHandle;
                sci->busy = 2; /*Set the busy flag*/
                ret = write(sci->fd, TxDataPtr, TxDataLength);
                sci->busy = 0; /*Reset the busy flag*/
                if (ret < 0) {
                    perror("SERIAL_write/write");
                    return MW_SCI_DATA_NOT_AVAILABLE;
                }
            }
            return MW_SCI_SUCCESS;
    }

    /* Get the status of SCI device */
    MW_SCI_Status_Type MW_SCI_GetStatus(MW_Handle_Type SCIModuleHandle)
    {
        SCI_dev_t *sci;
        uint8_T ret;
        if (NULL != (void *)SCIModuleHandle){
            sci = (SCI_dev_t *)SCIModuleHandle;
            ret = sci->busy;
            switch(ret){
                case 1:
                    return MW_SCI_RX_BUSY;
                    break;
                case 2:
                    return MW_SCI_TX_BUSY;
                    break;
                default:
                    return MW_SCI_SUCCESS;
            }
        }
        
    }

    /* Send break command */
    MW_SCI_Status_Type MW_SCI_SendBreak(MW_Handle_Type SCIModuleHandle)
    {
        return MW_SCI_SUCCESS;
    }

    /* Release SCI module */
    void MW_SCI_Close(MW_Handle_Type SCIModuleHandle)
    {
         SCI_dev_t *sci;
         if (NULL != (void *)SCIModuleHandle){
             sci = (SCI_dev_t *)SCIModuleHandle;
             if (sci->fd > 0) {
                    SERIAL_close(sci->fd);
                    sci->fd = -1;
                }
         }
    }
#endif
#ifdef __cplusplus
}
#endif

/* LocalWords:  NDELAY DCD NOCTTY EBADF EINTR EIO dev Baudrate IGNBRK BRKINT
 * LocalWords:  ICRNL INLCR IGNCR databytes
 */
