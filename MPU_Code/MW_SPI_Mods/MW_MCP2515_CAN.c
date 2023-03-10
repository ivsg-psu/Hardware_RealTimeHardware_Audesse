/* Copyright 2019 The MathWorks, Inc.*/
#include "MW_MCP2515_CAN.h"

#define REG_BFPCTRL                0x0c
#define REG_TXRTSCTRL              0x0d

#define REG_CANCTRL                0x0f

#define REG_CNF3                   0x28
#define REG_CNF2                   0x29
#define REG_CNF1                   0x2a

#define REG_CANINTE                0x2b
#define REG_CANINTF                0x2c

#define REG_RXBUFFER                0x90

#define FLAG_RXnIE(n)              (0x01 << n)
#define FLAG_RXnIF(n)              (0x01 << n)
#define FLAG_TXnIF(n)              (0x04 << n)

#define REG_RXFnSIDH(n)            (0x00 + (n * 4))
#define REG_RXFnSIDL(n)            (0x01 + (n * 4))
#define REG_RXFnEID8(n)            (0x02 + (n * 4))
#define REG_RXFnEID0(n)            (0x03 + (n * 4))

#define REG_RXMnSIDH(n)            (0x20 + (n * 0x04))
#define REG_RXMnSIDL(n)            (0x21 + (n * 0x04))
#define REG_RXMnEID8(n)            (0x22 + (n * 0x04))
#define REG_RXMnEID0(n)            (0x23 + (n * 0x04))

#define REG_TXBnCTRL(n)            (0x30 + (n * 0x10))
#define REG_TXBnSIDH(n)            (0x31 + (n * 0x10))
#define REG_TXBnSIDL(n)            (0x32 + (n * 0x10))
#define REG_TXBnEID8(n)            (0x33 + (n * 0x10))
#define REG_TXBnEID0(n)            (0x34 + (n * 0x10))
#define REG_TXBnDLC(n)             (0x35 + (n * 0x10))
#define REG_TXBnD0(n)              (0x36 + (n * 0x10))

#define REG_RXBnCTRL(n)            (0x60 + (n * 0x10))
#define REG_RXBnSIDH(n)            (0x61 + (n * 0x10))
#define REG_RXBnSIDL(n)            (0x62 + (n * 0x10))
#define REG_RXBnEID8(n)            (0x63 + (n * 0x10))
#define REG_RXBnEID0(n)            (0x64 + (n * 0x10))
#define REG_RXBnDLC(n)             (0x65 + (n * 0x10))
#define REG_RXBnD0(n)              (0x66 + (n * 0x10))

#define REG_EFLG                   0x2d
#define FLAG_IDE                   0x08
#define FLAG_SRR                   0x10
#define FLAG_RTR                   0x40
#define FLAG_EXIDE                 0x08

#define FLAG_RXM0                  0x20
#define FLAG_RXM1                  0x40
   
#if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
/*Rapid Accelerator */
void MW_GetCANBaud(uint8_T* value1,uint8_T* value2,uint8_T* value3){

}

void MW_GetCANMessageWithID(uint32_T id,uint8_T* data1, uint8_T dataLength, uint8_T* status, uint8_T extended, uint8_T* remote, uint8_T* error){

}

void MW_GetCANMessageNew(uint32_T* id,uint8_T* data1, uint8_T* dataLength, uint8_T* status, uint8_T* extended, uint8_T* remote, uint8_T* error){

}

void MW_PollForCANMessage(uint32_T* id,uint8_T* data1, uint8_T* dataLength, uint8_T* status, uint8_T* extended, uint8_T* remote, uint8_T* error){

}

void MW_GetCANFilters(uint8_T* allowAll,uint8_T* buffer0Extended,uint32_T* mask0,uint32_T* filter0,uint32_T* filter1,uint8_T* buffer1Extended,uint32_T* mask1,uint32_T* filter2,uint32_T* filter3,uint32_T* filter4,uint32_T* filter5){

}

void MW_CANAssignIdAndLength(uint32_T id,uint8_T extended,uint8_T msgLength){

}

int MW_CANInitializeInterrupt(){
    return 0;
}

#else
#ifdef __MW_TARGET_USE_HARDWARE_RESOURCES_H__
#include "MW_target_hardware_resources.h"
#endif

extern MW_SPI_Status_Type MW_SPI_MasterWriteRead_8bits(
                MW_Handle_Type SPIModuleHandle, 
                const uint8_T * WriteDataPtr, 
                uint8_T * ReadDataPtr, 
                uint32_T DataLength);
static uint8_t mcpInitialized = 0;
static uint8_t mcpTxInitialized = 0;
static uint8_t mcpRxInitialized = 0;
static uint32_t CANBusSpeed[12] = {5, 10, 20, 40, 50, 80, 100, 125, 200, 250, 500, 1000};
static uint32_t MCPOscillatorFreq[2] = {8, 16};

#ifdef MW_NUM_CAN_RECEIVE
volatile CANMsgType globalCANRxBuffer[MW_NUM_CAN_RECEIVE];
#endif
static uint8_T canRxIdAssigner = 0;
volatile CANMsgType newCANRxBuffer;
static uint8_T canRxIdAssigner;
uint8_T readValue;
uint8_T rxErrors;


/* mcp initialization */
uint8_t getMCPInitStatus()
{
    return mcpInitialized;
}

void setMCPInitStatus()
{
    mcpInitialized = 1;
}

/* mcp Tx initialization */
uint8_t getMCPTxInitStatus()
{
    return mcpTxInitialized;
}

void setMCPTxInitStatus()
{
    mcpTxInitialized = 1;
}

/* mcp Rx initialization */
uint8_t getMCPRxInitStatus()
{
    return mcpRxInitialized;
}

void setMCPRxInitStatus()
{
    mcpRxInitialized = 1;
}

uint8_t getInterruptPin(){
    uint8_t pinNum = INTERRUPT_PIN;
    return pinNum;
}

uint8_t getPinConfig(){
    uint8_t pinConfig = PI_INPUT;
    return pinConfig;
}

uint8_t getPinPullup(){
    uint8_t pinPullUp = PI_PUD_UP;
    return pinPullUp;
}

uint8_t getInterruptEdge(){
    uint8_t intrEdge = RISING_EDGE;
    return intrEdge;
}

double MW_getTimeNow()
{
    struct timeval timeNow;
    double timeInSeconds;
    
    gettimeofday(&timeNow,NULL);
    timeInSeconds = timeNow.tv_sec + (timeNow.tv_usec/1000000.0);
    
    return timeInSeconds;
}

void MW_GetCANBaud(uint8_t* value1,uint8_t* value2,uint8_t* value3){
    const uint8_t* cnf = NULL;
    const struct {
        long clockFrequency;
        long baudRate;
        uint8_t cnf[3];
    } CNF_MAPPER[] = {
        {  8, 1000, { 0x00, 0x80, 0x00 } },
        {  8,  500, { 0x00, 0x90, 0x02 } },
        {  8,  250, { 0x00, 0xb1, 0x05 } },
        {  8,  200, { 0x00, 0xb4, 0x06 } },
        {  8,  125, { 0x01, 0xb1, 0x05 } },
        {  8,  100, { 0x01, 0xb4, 0x06 } },
        {  8,   80, { 0x01, 0xbf, 0x07 } },
        {  8,   50, { 0x03, 0xb4, 0x06 } },
        {  8,   40, { 0x03, 0xbf, 0x07 } },
        {  8,   20, { 0x07, 0xbf, 0x07 } },
        {  8,   10, { 0x0f, 0xbf, 0x07 } },
        {  8,    5, { 0x1f, 0xbf, 0x07 } },
        
        { 16, 1000, { 0x00, 0xd0, 0x82 } },
        { 16,  500, { 0x00, 0xf0, 0x86 } },
        { 16,  250, { 0x41, 0xf1, 0x85 } },
        { 16,  200, { 0x01, 0xfa, 0x87 } },
        { 16,  125, { 0x03, 0xf0, 0x86 } },
        { 16,  100, { 0x03, 0xfa, 0x87 } },
        { 16,   80, { 0x03, 0xff, 0x87 } },
        { 16,   50, { 0x07, 0xfa, 0x87 } },
        { 16,   40, { 0x07, 0xff, 0x87 } },
        { 16,   20, { 0x0f, 0xff, 0x87 } },
        { 16,   10, { 0x1f, 0xff, 0x87 } },
        { 16,    5, { 0x3f, 0xff, 0x87 } },
    };
    
    for (unsigned int i = 0; i < (sizeof(CNF_MAPPER) / sizeof(CNF_MAPPER[0])); i++) {
        if (CNF_MAPPER[i].clockFrequency == (long)MCPOscillatorFreq[MW_CAN_CANOSCILLATORFREQUENCY] && CNF_MAPPER[i].baudRate == (long)CANBusSpeed[MW_CAN_CANBUSSPEED]) {
            cnf = CNF_MAPPER[i].cnf;
            break;
        }
    }
    
    if (cnf == NULL) {
        *value1 = 0;
        *value2 = 0;
        *value3 = 0;
    }
    else{
        *value1 = cnf[0];
        *value2 = cnf[1];
        *value3 = cnf[2];
    }
    
}

void MW_GetCANFilters(uint8_t* allowAll,uint8_t* buffer0Extended,uint32_t* mask0,uint32_t* filter0,uint32_t* filter1,uint8_t* buffer1Extended,uint32_t* mask1,uint32_t* filter2,uint32_t* filter3,uint32_t* filter4,uint32_t* filter5){
    
    *allowAll           = (uint8_t)MW_CAN_ALLOWALLFILTER;
    *buffer0Extended    = (uint8_t)MW_CAN_BUFFER0IDTYPE;
    *mask0              = (uint32_t)MW_CAN_ACCEPTANCEMASK0;
    *filter0            = (uint32_t)MW_CAN_ACCEPTANCEFILTER0;
    *filter1            = (uint32_t)MW_CAN_ACCEPTANCEFILTER1;
    *buffer1Extended    = (uint8_t)MW_CAN_BUFFER1IDTYPE;
    *mask1              = (uint32_t)MW_CAN_ACCEPTANCEMASK1;
    *filter2            = (uint32_t)MW_CAN_ACCEPTANCEFILTER2;
    *filter3            = (uint32_t)MW_CAN_ACCEPTANCEFILTER3;
    *filter4            = (uint32_t)MW_CAN_ACCEPTANCEFILTER4;
    *filter5            = (uint32_t)MW_CAN_ACCEPTANCEFILTER5;
}

void MW_GetCANMessageWithID(uint32_T id,uint8_T* data1, uint8_T dataLength, uint8_T* status, uint8_T extended, uint8_T* remote, uint8_T* error)
{
#ifdef MW_NUM_CAN_RECEIVE
    for(int i=0;i<MW_NUM_CAN_RECEIVE;i++)
    {
        if((id == globalCANRxBuffer[i].ID) && (extended == globalCANRxBuffer[i].extended))
        {
            for(int j=0;j<8;j++)
            {
                *(data1+j) = globalCANRxBuffer[i].Data[j];
            }
            *status = globalCANRxBuffer[i].status;
            *remote = globalCANRxBuffer[i].Remote;
            globalCANRxBuffer[i].status = 0U;
            break;
        }
    }
    *error = rxErrors;
#endif
}

void MW_GetCANMessageNew(uint32_T* id,uint8_T* data1, uint8_T* dataLength, uint8_T* status, uint8_T* extended, uint8_T* remote, uint8_T* error)
{
    for(int j=0;j<8;j++)
    {
        *(data1+j) = newCANRxBuffer.Data[j];
    }
    *dataLength = newCANRxBuffer.Length;
    *status = newCANRxBuffer.status;
    *extended = newCANRxBuffer.extended;
    *remote = newCANRxBuffer.Remote;
    *id = newCANRxBuffer.ID;
    *error = rxErrors;
    newCANRxBuffer.status = 0U;
}

void MW_PollForCANMessage(uint32_T* id,uint8_T* data1, uint8_T* dataLength, uint8_T* status, uint8_T* extended, uint8_T* remote, uint8_T* error){
    SPI_dev_t *spi;
    MW_SPI_Status_Type writeReadStatus;
    MW_Handle_Type SPIModuleHandle;
    uint8_t wrDataRaw[] = {REG_RXBUFFER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t rdDataRaw[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    /* Get device handle */
    spi = &spiDev[currSPIDev]; // modified by prasanth
    
    SPIModuleHandle = (MW_Handle_Type)spi;
    printf("Starting SPI read of MCP2515\r\n");
    fflush(stdout);
    
    writeReadStatus = MW_SPI_MasterWriteRead_8bits(
            SPIModuleHandle,
            wrDataRaw,
            rdDataRaw,
            14U);
    fprintf(stderr,"Successfully requested Rx buffer from SPI\r\n");
    fprintf(stderr,"Captured message with data bytes %x %x %x %x %x %x %x %x\r\n",
            rdDataRaw[6],rdDataRaw[7],rdDataRaw[8],rdDataRaw[9],
            rdDataRaw[10],rdDataRaw[11],rdDataRaw[12],rdDataRaw[13]);
    fflush(stderr);
    
    for(int j=6;j<14;j++)
    {
        *(data1+(j-6)) = rdDataRaw[j];
    }
    //dataLength[0] = (uint8_T)0; //rdDataRaw[5];
    /**status = 0; // CEB: Not implemented
    *extended = 0; // CEB: Not implemented
    *remote = 0; // CEB: Not implemented
    *id = 0; //rdDataRaw[1] << 8 | rdDataRaw[2] >> 5;
    *error = 0; // CEB: Not implemented*/
    fprintf(stderr,"Finished CAN polling function\r\n");
    fflush(stderr);
    
}

static uint8_t readRegister(uint8_t address){
    uint8_t value;
    SPI_dev_t *spi;
    MW_SPI_Status_Type writeReadStatus;
    MW_Handle_Type SPIModuleHandle;
    uint8_t wrDataRaw[] = {3, address, 0};
    uint8_t rdDataRaw[] = {0, 0, 0};
    
    /* Get device handle */
    spi = &spiDev[currSPIDev]; // modified by prasanth
    
    SPIModuleHandle = (MW_Handle_Type)spi;
    
    writeReadStatus = MW_SPI_MasterWriteRead_8bits(
            SPIModuleHandle,
            wrDataRaw,
            rdDataRaw,
            3);
    value = rdDataRaw[2];
    return value;
}

    
static void modifyRegister(uint8_t address, uint8_t mask, uint8_t value){
    uint8_t readValue[4];
    SPI_dev_t *spi;
    MW_Handle_Type SPIModuleHandle;
    uint8_t spiModifyCmd = 5;
    
    /* Get device handle */
    spi = &spiDev[currSPIDev]; // modified by prasanth
    SPIModuleHandle = (MW_Handle_Type)spi;
    
    uint8_t writeArray[] = {spiModifyCmd, address, mask, value};
    
    MW_SPI_MasterWriteRead_8bits(
            SPIModuleHandle,
            writeArray,
            readValue,
            4);
}

static void writeRegister(uint8_t address, uint8_t value){
    uint8_t readValue[3];
    SPI_dev_t *spi;
    MW_Handle_Type SPIModuleHandle;
    uint8_t spiWriteCmd = 2;
    
    /* Get device handle */
    spi = &spiDev[currSPIDev]; // modified by prasanth
    SPIModuleHandle = (MW_Handle_Type)spi;
    
    uint8_t writeArray[] = {spiWriteCmd, address, value};
    
    MW_SPI_MasterWriteRead_8bits(
            SPIModuleHandle,
            writeArray,
            readValue,
            3);
}

static int parsePacket(){
    int32_T _rxId;
    uint8_T _rxExtended;
    uint8_T _rxRtr;
    uint8_T _rxDlc;
    uint8_T _rxLength;
    uint8_T _rxData[14];
    uint8_T receiveBuffer;
    /* Read interrupt flags */
    uint8_t intf = readRegister(REG_CANINTF);
    
    if (intf & FLAG_RXnIF(0)) {
        /* Receive Buffer 0 Full */
        receiveBuffer = 0;
    } else if (intf & FLAG_RXnIF(1)) {
        /* Receive Buffer 1 Full */
        receiveBuffer = 1;
    } else {
        /* No message received*/
        _rxId = -1;
        _rxExtended = false;
        _rxRtr = false;
        _rxLength = 0;
        return 0;
    }
    
    /*Check Extended Identifier Flag bit */
    _rxExtended = (readRegister(REG_RXBnSIDL(receiveBuffer)) & FLAG_IDE) ? true : false;
            
    /* Read Identifier */
    uint32_T idA = ((readRegister(REG_RXBnSIDH(receiveBuffer)) << 3) & 0x07f8) | ((readRegister(REG_RXBnSIDL(receiveBuffer)) >> 5) & 0x07);
    if (_rxExtended) {
        /* In case of extended, read additional bits */
        uint32_T idB = (((uint32_T)(readRegister(REG_RXBnSIDL(receiveBuffer)) & 0x03) << 16) & 0x30000) | ((readRegister(REG_RXBnEID8(receiveBuffer)) << 8) & 0xff00) | readRegister(REG_RXBnEID0(receiveBuffer));
        
        _rxId = (idA << 18) | idB;
        /* Remote request check */
        _rxRtr = (readRegister(REG_RXBnDLC(receiveBuffer)) & FLAG_RTR) ? true : false;
    } else {
        _rxId = idA;
        /* Remote request check */
        _rxRtr = (readRegister(REG_RXBnSIDL(receiveBuffer)) & FLAG_SRR) ? true : false;
    }
    /* Read datalength */
    _rxDlc = readRegister(REG_RXBnDLC(receiveBuffer)) & 0x0f;
    
    if (_rxRtr) {
        _rxLength = 0;
    } else {
        _rxLength = _rxDlc;
        for (int i = 0; i < _rxLength; i++) {
            /* Read data bytes */
            _rxData[i] = readRegister(REG_RXBnD0(receiveBuffer) + i);
        }
    }
    
    /* Store in global buffer for CANBus Type System Object */
    newCANRxBuffer.ID = _rxId;
    newCANRxBuffer.status = 1U;
    newCANRxBuffer.extended = _rxExtended;
    newCANRxBuffer.Length = _rxDlc;
    newCANRxBuffer.Remote = _rxRtr;
    newCANRxBuffer.Error = 0;		//TODO
    for(int j=0;j<8;j++)
    {
        if(j < _rxDlc)
        {
            newCANRxBuffer.Data[j] = _rxData[j];
        }
        else
        {
            newCANRxBuffer.Data[j] = 0;
        }
        
    }
#ifdef MW_NUM_CAN_RECEIVE
    /* Store in global buffer for Raw Data Type System Object */
    for(int i=0;i<MW_NUM_CAN_RECEIVE;i++)
    {
        if((globalCANRxBuffer[i].ID == _rxId) && (globalCANRxBuffer[i].extended == _rxExtended) && (globalCANRxBuffer[i].Length == _rxLength))
        {
            globalCANRxBuffer[i].status = 1U;
            globalCANRxBuffer[i].extended = _rxExtended;
            globalCANRxBuffer[i].Length = _rxDlc;
            globalCANRxBuffer[i].Remote = _rxRtr;
            globalCANRxBuffer[i].Error = 0;		//TODO
            for(int j=0;j<8;j++)
            {
                if(j < _rxDlc)
                {
                    globalCANRxBuffer[i].Data[j] = _rxData[j];
                }
                else
                {
                    globalCANRxBuffer[i].Data[j] = 0;
                }
                
            }
        }
    }
#endif
    /* Clear interrupt */
    modifyRegister(REG_CANINTF, FLAG_RXnIF(receiveBuffer), 0x00);
    return _rxDlc;
}

static void readErrors(void)
{
    readValue =  readRegister(REG_EFLG);
    /* rxErrors derived from EFLG - ERROR FLAG register. Only receive errors are extracted */
    rxErrors = ((readValue & 0x01) >> 1) | ((readValue & 0x08) >> 2) | ((readValue & 0xC0) >> 4);
	/* Clear error interrupt */
    modifyRegister(REG_CANINTF, 0xE0, 0x00);
}

void gpio_interruptCallback(int gpio, int level, uint32_t tick){
    /*Receive interrupt handler */
    /*Check if level is LOW and proceed */
    uint8_T canIntf;
    if (level == 0){
        canIntf = readRegister(REG_CANINTF);
        if (canIntf == 0) {
            return;
        }
        if((canIntf & 0x20) != 0U)
        {
            readErrors();
        }
        parsePacket();
    }
}

int MW_CANInitializeInterrupt(){
    static uint8_t isMCPInterruptRegisterd = 0;
    if (isMCPInterruptRegisterd == 0){
        /*Register callback function whenerver specified GPIO interrupt occurs*/
        gpioInitialise();
        gpioSetMode(INTERRUPT_PIN, PI_INPUT);
        gpioSetPullUpDown(INTERRUPT_PIN, PI_PUD_UP);
        gpioSetISRFunc(INTERRUPT_PIN, FALLING_EDGE, 0, gpio_interruptCallback);
        
        isMCPInterruptRegisterd = 1;
    }
    return 0;
}

void MW_CANAssignIdAndLength(uint32_t id,uint8_t extended,uint8_t msgLength)
{
#ifdef MW_NUM_CAN_RECEIVE
    globalCANRxBuffer[canRxIdAssigner].ID = id;
    globalCANRxBuffer[canRxIdAssigner].extended = extended;
    globalCANRxBuffer[canRxIdAssigner].Length = msgLength;
    canRxIdAssigner = canRxIdAssigner + 1U;
#endif
}
#endif
/* [EOF] */
