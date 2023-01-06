/* Copyright 2016-2017 The MathWorks, Inc.*/
/* Location (1.5.2023 on P1 laptop): C:\ProgramData\MATLAB\SupportPackages\R2020b\toolbox\target\supportpackages\raspberrypi\src\MW_SPI.c */
/* C. Beal: Changes noted below. */

#include "MW_SPI.h"
#include "MW_SPI_Helper.h"

#if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
    
    MW_Handle_Type MW_SPI_Open(uint32_T SPIModule, uint32_T MosiPin, uint32_T MisoPin, uint32_T ClockPin, uint32_T SlaveSelectPin, uint8_T ActiveLowSSPin, uint8_T spi_device_type)
    {
        return NULL;
    }

    MW_SPI_Status_Type MW_SPI_SetFormat(MW_Handle_Type SPIModuleHandle, uint8_T targetprecision, MW_SPI_Mode_type spi_mode, MW_SPI_FirstBitTransfer_Type TargetFirstBitToTransfer)
    {
        return MW_SPI_SUCCESS;
    }

    MW_SPI_Status_Type MW_SPI_SetBusSpeed(MW_Handle_Type SPIModuleHandle, uint32_T BusSpeedInHz)
    {
        return MW_SPI_SUCCESS;
    }

    MW_SPI_Status_Type MW_SPI_SetSlaveSelect(MW_Handle_Type SPIModuleHandle, uint32_T SlaveSelectPin, uint8_T ActiveLowSSPin)
    {
        return MW_SPI_SUCCESS;
    }

    MW_SPI_Status_Type MW_SPI_MasterWriteRead_8bits(MW_Handle_Type SPIModuleHandle, const uint8_T * wrData, uint8_T * rdData, uint32_T datalength)
    {
        return MW_SPI_SUCCESS;
    }

    MW_SPI_Status_Type MW_SPI_SlaveWriteRead_8bits(MW_Handle_Type SPIModuleHandle, const uint8_T * wrData, uint8_T * rdData, uint32_T datalength)
    {
        return MW_SPI_SUCCESS;
    }

    MW_SPI_Status_Type MW_SPI_GetStatus(MW_Handle_Type SPIModuleHandle)
    {
        return MW_SPI_SUCCESS;
    }

    void MW_SPI_Close(MW_Handle_Type SPIModuleHandle, uint32_T MosiPin, uint32_T MisoPin, uint32_T ClockPin, uint32_T SlaveSelectPin)
    {

    }

#else
		#ifdef __MW_TARGET_USE_HARDWARE_RESOURCES_H__
			#include "MW_target_hardware_resources.h"
		#endif

        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <unistd.h>
        #include <errno.h>
        #include <fcntl.h>
        #include <linux/spi/spidev.h>
        #include <sys/ioctl.h>
        #include <sys/types.h>
        #include <sys/stat.h>
        #include "common.h"

        /* Local defines */
        #define MAX_BUF_SIZE            (32)
        #define SPI_BUS_AVAILABLE       (1)
        #define SPI_BUS_UNAVAILABLE     (0)
        #define NUM_MAX_BUSSES          (6)
        #define SPI_DEV_BASE            "/dev/spidev"

        /* CEB 1.4.2023 increased the size of this array and filled it with identical values */
        static uint32_T SPIchannelSpeed[6] = {500000, 500000, 500000, 500000, 500000, 500000};
        
        uint32_T BusSpeed[7] = {32000, 16000, 8000, 4000, 2000, 1000, 500};
        
        /* CEB 1.5.2023 this variable is original, but probably not used any more */
        static int currSPIDev = 0;
        
        
        SPI_dev_t spiDev[6]={
            /* CEB 1.4.2023 modified to include all of the available SPI channels */
            {-1,0,0,500000,8,0,0}, // SPI0_CE0
            {-1,0,1,500000,8,0,0}, // SPI0_CE1
            {-1,0,2,500000,8,0,0}, // SPI0_CE2 (dummy for spacing, not implemented)
            {-1,1,0,500000,8,0,0}, // SPI1_CE0
            {-1,1,1,500000,8,0,0}, // SPI1_CE1
            {-1,1,2,500000,8,0,0}, // SPI1_CE2
            // fd, device, chip select, speed, bits/word, lsb first, SPI mode
        };
        
        /* Open SPI channel */
        /* CEB 1.5.2023 modified to take in the spiModule in addition to the spiSlaveSelect */
        static int SPI_open(const unsigned int spiModule, const unsigned int spiSlaveSelect)
        {
            int fd;
            char buf[MAX_BUF_SIZE];

            /* CEB 1.5.2023 modified to address by module and slave select */
            snprintf(buf, sizeof(buf), "/dev/spidev%d.%d" , spiModule, spiSlaveSelect);
            fd = open(buf, O_RDWR);
            if (fd < 0) {
                fprintf(stderr,"%s\n", buf);
                perror("SPI_open/open");
            }
            return fd;
        }

        /* Close SPI channel */
        static void SPI_close(int fd)
        {
            int ret;
            printf("SPI_close fd = %d \n",fd);
            fflush(stdout);
            ret = close(fd);
            if (ret < 0) {
                /* EBADF, EINTR, EIO: In all cases, descriptor is torn down*/
                perror("SPI_close/close");
            }
        }


        /* SVD APIs */
        MW_Handle_Type MW_SPI_Open(
                uint32_T SPIModule, 
                uint32_T MosiPin,        /* Not used*/
                uint32_T MisoPin,         /* Not used */
                uint32_T ClockPin,        /* Not used */
                uint32_T SlaveSelectPin, 
                uint8_T ActiveLowSSPin, 
                uint8_T IsSPIDeviceSlave)
        {
            int ret;
            SPI_dev_t *spi;
            MW_Handle_Type SPIModuleHandle = (MW_Handle_Type)NULL;

            /* Check parameters */
            if (IsSPIDeviceSlave) {
                fprintf(stderr,"Only SPI master mode is supported.\n");
                exit(-1);
            }

            /* CEB 1.4.2023 modified to allow access to multiple SPI modules */
            if (SPIModule != 0 && SPIModule != 1) {
                fprintf(stderr,"MW_SPI_Open: SPI module must be set to 0 or 1.\n");
                exit(-1);
            }
            

            /* CEB 1.4.2023 modified to allow access to multiple slave select pins */
            if ((SlaveSelectPin != 0) && (SlaveSelectPin != 1) && (SlaveSelectPin != 2)) {
                fprintf(stderr,"MW_SPI_Open: SlaveSelectPin must be 0, 1, or 2.\n");
                exit(-1);
            }
		
           /* CEB prevent access to the one infeasible channel (SPI0_CE2) */
           if (SPIModule == 0 && SlaveSelectPin == 2) {
                fprintf(stderr,"MW_SPI_OPEN: Incompatible SPI module and SlaveSelectPin.\n");
                exit(-1);
           }

           /* Current SPIDev */
           /* CEB 1.4.2023 this math allows selection of any of the 6 zero-indexed devices */
           currSPIDev = SPIModule*3 + SlaveSelectPin;
			
            /* Get device handle */
            spi = &spiDev[currSPIDev];
            
            if (spi->fd < 0) {
                spi->fd = SPI_open(SPIModule,SlaveSelectPin);
                if (spi->fd < 0) {
                    fprintf(stderr,"Error opening SPI bus.\n");
                    exit(-1);
                }

                /* Return handle */
                SPIModuleHandle = (MW_Handle_Type)spi;
            }
            else if (spi->fd > 0)
            {
                /* Return handle */
                SPIModuleHandle = (MW_Handle_Type)spi;
            }

            return SPIModuleHandle;
        }

        MW_SPI_Status_Type MW_SPI_SetSlaveSelect(
                MW_Handle_Type SPIModuleHandle, 
                uint32_T SlaveSelectPin, 
                uint8_T ActiveLowSSPin)
        {
            MW_SPI_Status_Type status;

            if (SPIModuleHandle == (MW_Handle_Type)NULL) {
                status = MW_SPI_BUS_NOT_AVAILABLE;
            }
            else if (((SPI_dev_t *)SPIModuleHandle)->SlaveSelectPin != SlaveSelectPin) {
                status = MW_SPI_BUS_NOT_AVAILABLE;
            }
            /* CEB 1.4.2023 this line looks like it only allows active low configurations (probably OK) */
            else if (ActiveLowSSPin != 1) {
                status = MW_SPI_BUS_NOT_AVAILABLE;
            }
            else
            {
                currSPIDev = SlaveSelectPin;
                status = MW_SPI_SUCCESS;
            }

            return status;
        }


        MW_SPI_Status_Type MW_SPI_SetFormat(
                MW_Handle_Type SPIModuleHandle, 
                uint8_T TargetPrecision,   /* Only 8-bit supported*/
                MW_SPI_Mode_type SPIMode,  /* 0 - 3*/
                MW_SPI_FirstBitTransfer_Type TargetFirstBitToTransfer)
        {
            SPI_dev_t *spi;
            int ret;

            spi = (SPI_dev_t *)SPIModuleHandle;
            
            if (SPIModuleHandle == (MW_Handle_Type)NULL)
            {
                return MW_SPI_BUS_NOT_AVAILABLE;
            }

            /* We are assuming here that the currSPIDev has been set via a call to
             MW_SPI_SetSlaveSelect */
            /* spi = &spiDev[currSPIDev]; */

            /* Set mode */
            ret = ioctl(spi->fd, SPI_IOC_WR_MODE, &SPIMode);
            if (ret < 0){
                perror("SPI_init/SPI_IOC_WR_MODE");
                return MW_SPI_BUS_ERROR;
            }
            ret = ioctl(spi->fd, SPI_IOC_RD_MODE, &SPIMode);
            if (ret < 0) {
                perror("SPI_init/SPI_IOC_RD_MODE");
                return MW_SPI_BUS_ERROR;
            }

            /* Set bits per word*/
            ret = ioctl(spi->fd, SPI_IOC_WR_BITS_PER_WORD, &TargetPrecision);
            if (ret < 0) {
                perror("SPI_init/SPI_IOC_WR_BITS_PER_WORD");
                return MW_SPI_BUS_ERROR;
            }
            ret = ioctl(spi->fd, SPI_IOC_RD_BITS_PER_WORD, &TargetPrecision);
            if (ret < 0) {
                perror("SPI_init/SPI_IOC_RD_BITS_PER_WORD");
                return MW_SPI_BUS_ERROR;
            }
            /* Ignore TargetFirstBitToTransfer as Raspberry Pi always uses MSB first*/
            (void)TargetFirstBitToTransfer;

            return MW_SPI_SUCCESS;
        }

        MW_SPI_Status_Type MW_SPI_SetBusSpeed(
                MW_Handle_Type SPIModuleHandle, 
                uint32_T BusSpeedInHz)
        {
            SPI_dev_t *spi;
            int ret;

            spi = (SPI_dev_t *)SPIModuleHandle;

            if (SPIModuleHandle == (MW_Handle_Type)NULL) {
                return MW_SPI_BUS_NOT_AVAILABLE;
            }

            /* We are assuming here that the currSPIDev has been set via a call to
             MW_SPI_SetSlaveSelect or MW_SPI_Open */
			if (0 == spi->SlaveSelectPin)
			{
				#ifdef MW_SPI_SPI0CE0BUSSPEED
                BusSpeedInHz = BusSpeed[MW_SPI_SPI0CE0BUSSPEED]*1000;
                #endif
			}
			else if (1 == spi->SlaveSelectPin)
            {
                #ifdef MW_SPI_SPI0CE1BUSSPEED
                BusSpeedInHz = BusSpeed[MW_SPI_SPI0CE1BUSSPEED]*1000;
                #endif
            }
			else
            {
                /* Consider BusSpeedInHz from function argument */
            }
            
            ret = ioctl(spi->fd, SPI_IOC_WR_MAX_SPEED_HZ, &BusSpeedInHz);
            if (ret < 0) {
                perror("SPI_init/SPI_IOC_WR_MAX_SPEED_HZ");
                return MW_SPI_BUS_NOT_AVAILABLE;
            }
            ret = ioctl(spi->fd, SPI_IOC_RD_MAX_SPEED_HZ, &BusSpeedInHz);
            if (ret < 0) {
                perror("SPI_init/SPI_IOC_RD_MAX_SPEED_HZ");
                return MW_SPI_BUS_NOT_AVAILABLE;
            }

            return MW_SPI_SUCCESS;
        }

        MW_SPI_Status_Type MW_SPI_MasterWriteRead_8bits(
                MW_Handle_Type SPIModuleHandle, 
                const uint8_T * WriteDataPtr, 
                uint8_T * ReadDataPtr, 
                uint32_T DataLength)
        {
            SPI_dev_t *spi;
            int ret;
            struct spi_ioc_transfer tr;

            spi = (SPI_dev_t *)SPIModuleHandle;

            if (SPIModuleHandle == (MW_Handle_Type)NULL) {
                return MW_SPI_BUS_NOT_AVAILABLE;
            }

            if (DataLength > 4096) {
                /* SPI data transfer size is limited to page size which is usually 4096 byte*/
                return MW_SPI_BUS_NOT_AVAILABLE;
            }

            /* We are assuming here that the currSPIDev has been set via a call to
             MW_SPI_SetSlaveSelect*/
            /* spi = &spiDev[currSPIDev]; */

            /* Setup SPI transfer structure*/
            memset(&tr, 0, sizeof(struct spi_ioc_transfer));
            tr.tx_buf = (unsigned long)WriteDataPtr;
            tr.rx_buf = (unsigned long)ReadDataPtr;
            tr.len    = DataLength;

            /* Execute IOCTL call to perform a full-duplex transfer*/
            ret = ioctl(spi->fd, SPI_IOC_MESSAGE(1), &tr);
            if (ret != DataLength) {
                perror("EXT_SPI_writeRead/ioctl");
                return MW_SPI_BUS_ERROR;
            }

            return MW_SPI_SUCCESS;
        }

        /* Initiate data communication from master to slave in slave mode */
        MW_SPI_Status_Type MW_SPI_SlaveWriteRead_8bits(MW_Handle_Type SPIModuleHandle, const uint8_T * wrData, uint8_T * rdData, uint32_T datalength)
        {
            return MW_SPI_SUCCESS;
        }


        MW_SPI_Status_Type MW_SPI_GetStatus(MW_Handle_Type SPIModuleHandle)
        {
            if (SPIModuleHandle == (MW_Handle_Type)NULL) {
                return MW_SPI_BUS_NOT_AVAILABLE;
            }

            return MW_SPI_SUCCESS;
        }

        void MW_SPI_Close(
                MW_Handle_Type SPIModuleHandle, 
                uint32_T MosiPin, 
                uint32_T MisoPin, 
                uint32_T ClockPin, 
                uint32_T SlaveSelectPin)
        {
            SPI_dev_t *spi;

            spi = (SPI_dev_t *)SPIModuleHandle;

            if (SPIModuleHandle == (MW_Handle_Type)NULL) {
                fprintf(stderr,"MW_SPI_Close:SPI module must be set to 0.\n");
                exit(-1);
            }
            
            if (spi->fd > 0) {
                SPI_close(spi->fd);
                spi->fd = -1;
            }
        }
#endif
#ifdef __cplusplus
}
#endif
/* [EOF] */