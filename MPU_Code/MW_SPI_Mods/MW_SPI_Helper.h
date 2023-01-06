/*MW_SPI_Helper*/
/* Location (1.5.2023 on P1 laptop): C:\ProgramData\MATLAB\SupportPackages\R2020b\toolbox\target\supportpackages\raspberrypi\include\MW_SPI_Helper.h */
/* C. Beal: Changes noted below. */

#ifndef _MW_SPI_HELPER_H_
#define _MW_SPI_HELPER_H_

/* CEB 1.4.2023 added these macros - I don't think they are needed*/
#ifndef CS0
#define CS0 0
#define CS1 1
#define CS2 2
#endif

/* CEB 1.4.2023 added these macros - I don't think they are needed*/
#ifndef SPI0
#define SPI0 0
#define SPI1 1
#endif

/* CEB 1.4.2023 these are original - I don't think they are needed*/
#ifndef SPI0_CE0
#define SPI0_CE0 0
#define SPI0_CE1 1
#define SPI1_CE0 3
#define SPI1_CE1 4
#define SPI1_CE2 5
#endif

/* CEB 1.5.2023 this type definition should be exactly as original */
typedef struct {
    int fd;
    uint32_T SPIModule;
    uint32_T SlaveSelectPin;
    uint32_T speed;
    uint8_T bitsPerWord;
    MW_SPI_Mode_type lsbFirst;
    MW_SPI_FirstBitTransfer_Type mode;
} SPI_dev_t;

/* CEB 1.5.2023 increased the size of this array - may not be needed */
extern SPI_dev_t spiDev[6];

#endif