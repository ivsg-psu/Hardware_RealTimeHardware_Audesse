
/*
 * Include Files
 *
 */
#if defined(MATLAB_MEX_FILE)
#include "tmwtypes.h"
#include "simstruc_types.h"
#else
#include "rtwtypes.h"
#endif



/* %%%-SFUNWIZ_wrapper_includes_Changes_BEGIN --- EDIT HERE TO _END */
#define OUTPUT_0_WIDTH 13
#define MESSAGE_WIDTH 64

#include <stddef.h>

static unsigned char msgBuff[MESSAGE_WIDTH];     // Global Variables to buffer serial port data
/* %%%-SFUNWIZ_wrapper_includes_Changes_END --- EDIT HERE TO _BEGIN */
#define u_width 64
#define y_width 1

/*
 * Create external references here.  
 *
 */
/* %%%-SFUNWIZ_wrapper_externs_Changes_BEGIN --- EDIT HERE TO _END */
// function for converting all of the data to doubles
double parseCharToDouble(unsigned char *startByte)
{
    return (double)*startByte;
}

// function for converting all of the data to doubles
double parseShortToDouble(unsigned char *startByte)
{
    unsigned short *output = (unsigned short *)startByte;
    return (double)*output;
}

// function for converting all of the data to doubles
double parseFloatToDouble(unsigned char *startByte)
{
    float *output = (float*)startByte;
    return (double)*output;
}

// function for converting all of the data to doubles
double parseDoubleToDouble(unsigned char *startByte)
{
    double *output = (double*)startByte;
    return (double)*output;
}

// checksum function: the algorithm takes the data and subtracts the bytes from the checksum
// provided at the end of the array so that the final value is zero if the data is correct
unsigned short checksum (unsigned char *ptr, size_t sz)
{
    unsigned short chk = *(unsigned short*)(ptr+sz); // initialize the sum with the reference checksum
    while (sz-- != 0) // keep decreasing the value of sz, the offset to the end of the array
        chk -= (unsigned short)*(ptr+sz); // subtract the value of each byte (promoting to ushort since the checksum is 2 bytes)
    return chk;
}
/* %%%-SFUNWIZ_wrapper_externs_Changes_END --- EDIT HERE TO _BEGIN */

/*
 * Output function
 *
 */
void gps_bin1_receive_Outputs_wrapper(const uint8_T *u0,
			const uint8_T *u1,
			real_T *y0,
			real_T *y1,
			void **pW)
{
/* %%%-SFUNWIZ_wrapper_Outputs_Changes_BEGIN --- EDIT HERE TO _END */
int status = u1[0];
    
    if( 0 == status )
    {
        // Once the first byte is the $ character, copy the appropriate number of bytes for the bin message into a processing buffer
        for (int input_index = 0; input_index < MESSAGE_WIDTH; input_index++)
        {
            msgBuff[input_index] = (unsigned char) u0[input_index];
        }
        if ((unsigned char)13 != msgBuff[MESSAGE_WIDTH-2] && (unsigned char)11 != msgBuff[MESSAGE_WIDTH-1])
            status = -1; // Message not complete, set the flag to avoid parsing
    }
    
    // If the status from the preceding data copy and end check is good, calculate the checksum with the 52 bytes of
    // data following the header byte in a bin1 message. Set the status flag if the data is bad.
    if( 0 == status )
    {
        if (0 != checksum(msgBuff+8,52))
            status = -2;
    }

    // If status is still good at this point, there is a good message to parse
    if (0 == status)
    {
        // Parse the portions of the message and write to the output
        y0[0] = parseCharToDouble(msgBuff+8);    // Age of differential (Error)
        y0[1] = parseCharToDouble(msgBuff+9);    // Number of Satellites used (Error)
        y0[2] = parseShortToDouble(msgBuff+10);    // GPS week
        y0[3] = parseDoubleToDouble(msgBuff+12);    // GPS time of week
        y0[4] = parseDoubleToDouble(msgBuff+20);    // Latitude
        y0[5] = parseDoubleToDouble(msgBuff+28);    // Longitude
        y0[6] = parseFloatToDouble(msgBuff+36);    // Altitude
        y0[7] = parseFloatToDouble(msgBuff+40);    // V North
        y0[8] = parseFloatToDouble(msgBuff+44);    // V East
        y0[9] = parseFloatToDouble(msgBuff+48);    // V Up
        y0[10] = parseFloatToDouble(msgBuff+52);    // Std Dev of Residuals
        y0[11] = parseShortToDouble(msgBuff+56);    // NAV mode
        y0[12] = parseShortToDouble(msgBuff+58);    // Extended age of differential
    }
    else
    {
        for (int output_index = 0; output_index < OUTPUT_0_WIDTH; output_index++)
        {
            y0[output_index] = (real_T)0;     // Zero the output if there is no valid data
        }
    }
    y1[0] = (real_T)status;     // Send out status variable with zero for good data, positive values for a pass through of a serial
    // reception error, and negative values for issues parsing the message
/* %%%-SFUNWIZ_wrapper_Outputs_Changes_END --- EDIT HERE TO _BEGIN */
}


